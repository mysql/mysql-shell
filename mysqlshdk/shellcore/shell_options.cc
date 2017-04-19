/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mysqlshdk/include/shellcore/shell_options.h"
#include <stdlib.h>
#include <cstdio>
#include <iostream>
#include <limits>
#include "mysqlshdk/libs/db/uri_common.h"
#include "shellcore/ishell_core.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

namespace mysqlsh {

using mysqlshdk::db::Transport_type;

Shell_options::Storage::Storage() {
#ifdef HAVE_V8
  initial_mode = shcore::IShell_core::Mode::JavaScript;
#else
#ifdef HAVE_PYTHON
  initial_mode = shcore::IShell_core::Mode::Python;
#else
  initial_mode = shcore::IShell_core::Mode::SQL;
#endif
#endif

  log_level = ngcommon::Logger::LOG_INFO;
  password = nullptr;
  session_type = mysqlsh::SessionType::Auto;

  default_session_type = true;
  print_cmd_line_helper = false;
  print_version = false;
  print_version_extra = false;
  force = false;
  interactive = false;
  full_interactive = false;
  passwords_from_stdin = false;
  prompt_password = false;
  recreate_database = false;
  trace_protocol = false;
  wizards = true;
  admin_mode = false;
  log_to_stderr = false;
  db_name_cache = true;
  devapi_schema_object_handles = true;

  port = 0;

  exit_code = 0;
}

bool Shell_options::Storage::has_connection_data() const {
  return !uri.empty() ||
    !user.empty() ||
    !host.empty() ||
    !schema.empty() ||
    !sock.empty() ||
    port != 0 ||
    password != NULL ||
    prompt_password ||
    ssl_options.has_data();
}

static mysqlsh::SessionType get_session_type(
    const mysqlshdk::db::Connection_options &opt) {
  if (!opt.has_scheme()) {
    return mysqlsh::SessionType::Auto;
  } else {
    std::string scheme = opt.get_scheme();
    if (scheme == "mysqlx")
      return mysqlsh::SessionType::X;
    else if (scheme == "mysql")
      return mysqlsh::SessionType::Classic;
    else
      throw std::invalid_argument("Unknown MySQL URI type "+scheme);
  }
}

/**
 * Builds Connection_options from options given by user through command line
 */
mysqlshdk::db::Connection_options
Shell_options::Storage::connection_options() const {
  mysqlshdk::db::Connection_options target_server;
  if (!uri.empty()) {
    target_server = shcore::get_connection_options(uri);
  }

  shcore::update_connection_data(&target_server, user, password, host, port,
                                 sock, schema, ssl_options, auth_method);

  // If a scheme is given on the URI the session type must match the URI
  // scheme
  if (target_server.has_scheme()) {
    mysqlsh::SessionType uri_session_type = get_session_type(target_server);
    std::string error;

    if (session_type != mysqlsh::SessionType::Auto &&
        session_type != uri_session_type) {
      if (session_type == mysqlsh::SessionType::Classic)
        error = "The given URI conflicts with the --mysql session type option.";
      else if (session_type == mysqlsh::SessionType::X)
        error =
            "The given URI conflicts with the --mysqlx session type "
            "option.";
    }
    if (!error.empty())
      throw shcore::Exception::argument_error(error);
  } else {
    switch (session_type) {
      case mysqlsh::SessionType::Auto:
        target_server.clear_scheme();
        break;
      case mysqlsh::SessionType::X:
        target_server.set_scheme("mysqlx");
        break;
      case mysqlsh::SessionType::Classic:
        target_server.set_scheme("mysql");
        break;
    }
  }

  // TODO(rennox): Analize if this should actually exist... or if it
  // should be done right before connection for the missing data
  shcore::set_default_connection_data(&target_server);
  return target_server;
}

static std::string hide_password_in_uri(std::string uri,
                                        const std::string& username) {
  std::size_t pwd_start = uri.find(username) + username.length() + 1;
  std::size_t pwd_size = uri.find('@', pwd_start) - pwd_start;
  return uri.replace(pwd_start, pwd_size, pwd_size, '*');
}

static std::string get_session_type_name(mysqlsh::SessionType type) {
  std::string label;
  switch (type) {
    case mysqlsh::SessionType::X:
      label = "X protocol";
      break;
    case mysqlsh::SessionType::Classic:
      label = "Classic";
      break;
    case mysqlsh::SessionType::Auto:
      label = "";
      break;
  }

  return label;
}

using std::placeholders::_1;
using std::placeholders::_2;
using mysqlshdk::db::Ssl_options;
using shcore::opts::deprecated;
using shcore::opts::cmdline;
using shcore::opts::assign_value;

Shell_options::Shell_options(int argc, char** argv)
    : Options(false,
              std::bind(&Shell_options::custom_cmdline_handler,
                        this, _1, _2)) {
  std::string home = shcore::get_home_dir();
#ifdef WIN32
  home += ("MySQL\\mysql-sandboxes");
#else
  home += ("mysql-sandboxes");
#endif
  add_startup_options()
    (&storage.print_cmd_line_helper, false,
        cmdline("-?", "--help"), "Display this help and exit.")
    (&storage.execute_statement, "", cmdline("-e", "--execute=<cmd>"),
        "Execute command and quit.")
    (cmdline("-f", "--file=file"), "Process file.")
    (cmdline("--uri=value"), "Connect to Uniform Resource Identifier. "
        "Format: [user[:pass]@]host[:port][/db]")
    (&storage.host, "", cmdline("-h", "--host=name"),
        "Connect to host.")
    (&storage.port, 0, cmdline("-P", "--port=#"),
        "Port number to use for connection.")
    (&storage.sock, "", cmdline("-S", "--socket=sock"),
        "Socket name to use in UNIX, "
        "pipe name to use in Windows (only classic sessions).")
    (&storage.user, "", cmdline("-u", "--dbuser=name", "--user=name"),
        "User for the connection to the server.")
    (cmdline("-p", "--password[=name] ", "--dbpassword[=name]"),
        "Password to use when connecting to server.")
    (cmdline("-p"), "Request password prompt to set the password")
    (&storage.schema, "", "schema",
        cmdline("-D", "--schema=name", "--database=name"), "Schema to use.")
    (&storage.recreate_database, false, "recreateDatabase",
        cmdline("--recreate-schema"), "Drop and recreate the specified schema."
        "Schema will be deleted if it exists!")
    (cmdline("-mx", "--mysqlx"),
        "Uses connection data to create Creating an X protocol session.",
        std::bind(
            &Shell_options::override_session_type, this, _1, _2))
    (cmdline("-mc", "--mysql"),
        "Uses connection data to create a Classic Session.",
        std::bind(
            &Shell_options::override_session_type, this, _1, _2))
    (cmdline("-ma"), "Uses the connection data to create the session with"
        "automatic protocol detection.",
        std::bind(
            &Shell_options::override_session_type, this, _1, _2))
    (cmdline("--sql"), "Start in SQL mode.", assign_value(
        &storage.initial_mode, shcore::IShell_core::Mode::SQL))
    (cmdline("--sqlc"), "Start in SQL mode using a classic session.",
        std::bind(
            &Shell_options::override_session_type, this, _1, _2))
    (cmdline("--sqlx"),
        "Start in SQL mode using Creating an X protocol session.",
        std::bind(
            &Shell_options::override_session_type, this, _1, _2))
    (cmdline("--js", "--javascript"), "Start in JavaScript mode.",
        [this](const std::string&, const char*) {
#ifdef HAVE_V8
          storage.initial_mode = shcore::IShell_core::Mode::JavaScript;
#else
          throw std::invalid_argument("JavaScript is not supported.");
#endif
        })
    (cmdline("--py", "--python"), "Start in Python mode.",
        [this](const std::string&, const char*) {
#ifdef HAVE_PYTHON
      storage.initial_mode = shcore::IShell_core::Mode::Python;
#else
      throw std::invalid_argument("Python is not supported.");
#endif
      })
    (cmdline("--json[=format]"),
        "Produce output in JSON format, allowed values:"
        "raw, pretty. If no format is specified pretty format is produced.",
        [this](const std::string&, const char* value) {
          if (!value || strcmp(value, "pretty") == 0) {
            storage.output_format = "json";
          } else if (strcmp(value, "raw") == 0) {
            storage.output_format = "json/raw";
          } else {
            throw std::invalid_argument(
                "Value for --json must be either pretty or raw.");
          }
        })
    (cmdline("--table"),
        "Produce output in table format (default for interactive mode). This "
        "option can be used to, force that format when running in batch mode.",
        assign_value(&storage.output_format, "table"))
    (cmdline("-E", "--vertical"),
        "Print the output of a query (rows) vertically.",
        assign_value(&storage.output_format, "vertical"));

  add_named_options()
    (&storage.output_format, "", SHCORE_OUTPUT_FORMAT,
        "Determines output format",
        [this](const std::string &val, shcore::opts::Source) {
          if (val != "table" && val != "json" && val != "json/raw" &&
              val != "vertical" && val != "tabbed")
            throw shcore::Exception::value_error(
                "The option " SHCORE_OUTPUT_FORMAT
                " must be one of: tabbed, table, vertical, json or json/raw.");
          return val;
        })
    (&storage.interactive, false, SHCORE_INTERACTIVE,
        "Enables interactive mode", shcore::opts::Read_only<bool>())
    (&storage.db_name_cache, -1, SHCORE_DB_NAME_CACHE,
        "Enable database name caching for autocompletion.")
    (&storage.devapi_schema_object_handles, -1,
        SHCORE_DEVAPI_DB_OBJECT_HANDLES,
        "Enable table and collection name handles for the DevAPI db object.");

  add_startup_options()(
      cmdline("-i", "--interactive[=full]"),
      "To use in batch mode. "
      "It forces emulation of interactive mode processing. Each "
      "line on the batch is processed as if it were in interactive mode.",
      [this](const std::string&, const char* value) {
        if (!value) {
          storage.interactive = true;
          storage.full_interactive = false;
        } else if (strcmp(value, "full") == 0) {
          storage.interactive = true;
          storage.full_interactive = true;
        } else {
          throw std::invalid_argument(
                    "Value for --interactive if any, must be full\n");
        }
      })(
      cmdline("--name-cache"),
      "Enable database name caching for autocompletion and DevAPI (default).",
      [this](const std::string&, const char*) {
        storage.db_name_cache = true;
        storage.db_name_cache_set = true;
        storage.devapi_schema_object_handles = true;
      })(
      cmdline("-A", "--no-name-cache"),
      "Disable automatic database name caching for autocompletion and DevAPI. "
      "Use \\rehash to load DB names manually.",
      [this](const std::string&, const char*) {
        storage.db_name_cache = false;
        storage.db_name_cache_set = true;
        storage.devapi_schema_object_handles = false;
      });

  // make sure hack for accessing log_level via Value works
  static_assert(
      std::is_integral<
          std::underlying_type<ngcommon::Logger::LOG_LEVEL>::type>::value,
      "Invalid underlying type of ngcommon::Logger::LOG_LEVEL enum");
  add_named_options()
    (&storage.force, false, SHCORE_BATCH_CONTINUE_ON_ERROR, cmdline("--force"),
        "To use in SQL batch mode, forces processing to "
        "continue if an error is found.", shcore::opts::Read_only<bool>())
    (reinterpret_cast<int*>(&storage.log_level),
        ngcommon::Logger::LOG_INFO, "logLevel", cmdline("--log-level=value"),
        "The log level. Value must be an integer between 1 and 8 any of "
        "[none, internal, error, warning, info, debug, debug2, debug3].",
        [this](const std::string &val, shcore::opts::Source) {
          const char* value = val.c_str();
          if (*value == '@') {
            storage.log_to_stderr = true;
            value++;
          }
          ngcommon::Logger::LOG_LEVEL nlog_level
            = ngcommon::Logger::get_log_level(value);
          if (nlog_level == ngcommon::Logger::LOG_NONE &&
              !ngcommon::Logger::is_level_none(value))
            throw std::invalid_argument(
                ngcommon::Logger::get_level_range_info());
          return nlog_level;
        })
    (&storage.passwords_from_stdin, false, "passwordsFromStdin",
        cmdline("--passwords-from-stdin"),
        "Read passwords from stdin instead of the tty.")
    (&storage.show_warnings, true, SHCORE_SHOW_WARNINGS,
        cmdline("--show-warnings"),
        "Automatically display SQL warnings on SQL mode if available.")
    (&storage.history_max_size, 1000, SHCORE_HISTORY_MAX_SIZE,
        "Shell's history maximum size",
        shcore::opts::Range<int>(0, std::numeric_limits<int>::max()))
    (&storage.histignore, "*IDENTIFIED*:*PASSWORD*", SHCORE_HISTIGNORE,
        cmdline("--histignore=filters"), "Shell's history ignore list.")
    (&storage.history_autosave, false, SHCORE_HISTORY_AUTOSAVE,
        "Shell's history autosave.")
    (&storage.sandbox_directory, home, SHCORE_SANDBOX_DIR,
        "Default sandbox directory")
    (&storage.gadgets_path, "", SHCORE_GADGETS_PATH, "Gadgets path")
    (&storage.wizards, true, SHCORE_USE_WIZARDS, "Enables wizard mode.");

  add_startup_options()
    (cmdline("--nw", "--no-wizard"), "Disables wizard mode.",
        assign_value(&storage.wizards, false))
    (&storage.print_version, false, cmdline("-V", "--version"),
        "Prints the version of MySQL Shell.")
    (cmdline("--ssl-key=name"), "X509 key in PEM format.",
        std::bind(&Ssl_options::set_key, &storage.ssl_options, _2))
    (cmdline("--ssl-cert=name"), "X509 cert in PEM format.",
        std::bind(&Ssl_options::set_cert, &storage.ssl_options, _2))
    (cmdline("--ssl-ca=name"), "CA file in PEM format.",
        std::bind(&Ssl_options::set_ca, &storage.ssl_options, _2))
    (cmdline("--ssl-capath=dir"), "CA directory.",
        std::bind(&Ssl_options::set_capath, &storage.ssl_options, _2))
    (cmdline("--ssl-cipher=name"), "SSL Cipher to use.",
        std::bind(&Ssl_options::set_ciphers, &storage.ssl_options, _2))
    (cmdline("--ssl-crl=name"), "Certificate revocation list.",
        std::bind(&Ssl_options::set_crl, &storage.ssl_options, _2))
    (cmdline("--ssl-crlpath=dir"), "Certificate revocation list path.",
        std::bind(&Ssl_options::set_crlpath, &storage.ssl_options, _2))
    (cmdline("--ssl-mode=mode"), "SSL mode to use, allowed values: DISABLED,"
        "PREFERRED, REQUIRED, VERIFY_CA, VERIFY_IDENTITY.",
        [this](const std::string& opt, const char* value) {
          int mode = mysqlshdk::db::MapSslModeNameToValue::get_value(value);
          if (mode == 0)
            throw std::invalid_argument(opt +
                " must be any any of [DISABLED, PREFERRED, REQUIRED, "
                         "VERIFY_CA, VERIFY_IDENTITY]");
          storage.ssl_options.set_mode(
              static_cast<mysqlshdk::db::Ssl_mode>(mode));})
    (cmdline("--tls-version=version"),
        "TLS version to use, permitted values are: TLSv1, TLSv1.1.",
        std::bind(&Ssl_options::set_tls_version,
            &storage.ssl_options, _2))
    (&storage.auth_method, "", cmdline("--auth-method=method"),
        "Authentication method to use.")
    (&storage.execute_dba_statement, "",
        cmdline("--dba=enableXProtocol"), "Enable the X Protocol "
        "in the server connected to. Must be used with --mysql.")
    (cmdline("--trace-proto"),
        assign_value(&storage.trace_protocol, true))
    (cmdline("--ssl[=opt]"), deprecated("--ssl-mode"))
    (cmdline("--node"), deprecated("--mysqlx or -mx"))
    (cmdline("--classic"), deprecated("--mysql or -mc"))
    (cmdline("--sqln"), deprecated("--sqlx"));

  try {
    handle_environment_options();
    handle_cmdline_options(argc, argv);

    check_session_type_conflicts();
    check_user_conflicts();
    check_password_conflicts();
    check_host_conflicts();
    check_host_socket_conflicts();
    check_port_conflicts();
    check_socket_conflicts();
    check_port_socket_conflicts();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    storage.exit_code = 1;
  }
}

void Shell_options::set(const std::string& option, const shcore::Value value) {
  try {
    if (value.type == shcore::Value_type::String)
      shcore::Options::set(option, value.as_string());
    else
      shcore::Options::set(option, value.repr());
  } catch(const std::exception& e) {
    throw shcore::Exception::argument_error(e.what());
  }
}

shcore::Value Shell_options::get(const std::string& option) {
  using shcore::opts::Concrete_option;
  auto it = named_options.find(option);
  if (it == named_options.end())
    throw std::invalid_argument("No option registered under name: " + option);
  Concrete_option<bool>* opt_bool =
      dynamic_cast<Concrete_option<bool>*>(it->second);
  if (opt_bool != nullptr)
    return shcore::Value(opt_bool->get());

  Concrete_option<std::string>* opt_string =
      dynamic_cast<Concrete_option<std::string>*>(it->second);
  if (opt_string != nullptr)
    return shcore::Value(opt_string->get());

  Concrete_option<int>* opt_int =
      dynamic_cast<Concrete_option<int>*>(it->second);
  if (opt_int != nullptr)
    return shcore::Value(opt_int->get());

  throw std::runtime_error(option + ": unable to deduce option type");
  return shcore::Value();
}

std::vector<std::string> Shell_options::get_named_options() {
  std::vector<std::string> res;
  for (const auto& op : named_options)
    res.push_back(op.first);
  return res;
}

bool Shell_options::custom_cmdline_handler(char** argv, int* argi) {
  int arg_format = 0;
  char* value = nullptr;

  if (strcmp(argv[*argi], "-VV") == 0) {
    storage.print_version = true;
    storage.print_version_extra = true;
  } else if (cmdline_arg_with_value(argv, argi, "--file", "-f", &value)) {
    storage.run_file = value;
    // the rest of the cmdline options, starting from here are all passed
    // through to the script
    storage.script_argv.push_back(value);
    for (++(*argi); argv[*argi]; (*argi)++) {
      storage.script_argv.push_back(argv[*argi]);
    }
  } else if ((arg_format =
                  cmdline_arg_with_value(argv, argi, "--uri", NULL, &value)) ||
             (argv[*argi][0] != '-' && (value = argv[*argi]))) {
    storage.uri = value;

    uri_data = shcore::get_connection_options(storage.uri, false);

    if (uri_data.has_password()) {
      std::string nopwd_uri = hide_password_in_uri(value, uri_data.get_user());

      // Required replacement when --uri=<value>
      if (arg_format == 3)
        nopwd_uri = "--uri=" + nopwd_uri;

      strncpy(argv[*argi], nopwd_uri.c_str(), strlen(argv[*argi]) + 1);
    }
  } else if ((arg_format = cmdline_arg_with_value(argv, argi, "--dbpassword",
                                                  NULL, &value, true))) {
    // Note that in any connection attempt, password prompt will be done if the
    // password is missing.
    // The behavior of the password cmd line argument is as follows:

    // ARGUMENT           EFFECT
    // --password         forces password prompt no matter it was already
    // provided
    // --password value   forces password prompt no matter it was already
    // provided (value is not taken as password)
    // --password=        sets password to empty (password is available but
    // empty so it will not be prompted)
    // -p<value> sets the password to <value>
    // --password=<value> sets the password to <value>

    if (!value) {
      // --password=
      if (arg_format == 3)
        storage.password = storage.pwd.c_str();
      // --password
      else
        storage.prompt_password = true;
    } else if (arg_format != 1) {  // --password=value || --pvalue
      storage.pwd.assign(value);
      storage.password = storage.pwd.c_str();

      std::string stars(storage.pwd.length(), '*');
      std::string pwd = arg_format == 2 ? "-p" : "--dbpassword=";
      pwd.append(stars);

      strncpy(argv[*argi], pwd.c_str(), strlen(argv[*argi]) + 1);
    } else {  // --password value (value is ignored)
      storage.prompt_password = true;
      (*argi)--;
    }
  } else if ((arg_format = cmdline_arg_with_value(argv, argi, "--password",
                                                  "-p", &value, true))) {
    // Note that in any connection attempt, password prompt will be done if
    // the password is missing.
    // The behavior of the password cmd line argument is as follows:

    // ARGUMENT           EFFECT
    // --password         forces password prompt no matter it was already
    // provided
    // --password value   forces password prompt no matter it was already
    // provided (value is not taken as password)
    // --password=        sets password to empty (password is available but
    // empty so it will not be prompted)
    // -p<value> sets the password to <value>
    // --password=<value> sets the password to <value>

    if (!value) {
      // --password=
      if (arg_format == 3)
        storage.password = storage.pwd.c_str();
      // --password
      else
        storage.prompt_password = true;
    } else if (arg_format != 1) {  // --password=value || --pvalue
      storage.pwd.assign(value);
      storage.password = storage.pwd.c_str();

      std::string stars(storage.pwd.length(), '*');
      std::string pwd = arg_format == 2 ? "-p" : "--password=";
      pwd.append(stars);

      strncpy(argv[*argi], pwd.c_str(), strlen(argv[*argi]) + 1);
    } else {  // --password value (value is ignored)
      storage.prompt_password = true;
      (*argi)--;
    }
  } else {
    return false;
  }
  return true;
}

void Shell_options::override_session_type(
    const std::string& option, const char* value) {
  if (value != nullptr)
    throw std::runtime_error("Option " + option + " does not support value");

  if (option.find("--sql") != std::string::npos)
    storage.initial_mode = shcore::IShell_core::Mode::SQL;

  mysqlsh::SessionType new_type = mysqlsh::SessionType::Auto;
  if (option == "-mc" || option == "--mysql" || option == "--sqlc")
    new_type = mysqlsh::SessionType::Classic;
  else if (option == "-mx" || option == "--mysqlx" || option == "--sqlx")
    new_type = mysqlsh::SessionType::X;

  if (new_type != storage.session_type) {
    if (!storage.default_session_type) {
      std::stringstream ss;
      if (storage.session_type == mysqlsh::SessionType::Auto) {
        ss << "Automatic protocol detection is enabled, unable to change to "
           << get_session_type_name(new_type) << " with option " << option;
      } else if (new_type == mysqlsh::SessionType::Auto) {
        ss << "Session type already configured to "
           << get_session_type_name(storage.session_type)
           << ", automatic protocol detection (-ma) can't be enabled.";
      } else {
        ss << "Session type already configured to "
           << get_session_type_name(storage.session_type)
           << ", unable to change to " << get_session_type_name(new_type)
           << " with option " << option;
      }
      throw std::invalid_argument(ss.str());
    }

    storage.session_type = new_type;
    session_type_arg = option;
  }

  storage.default_session_type = false;
}

void Shell_options::check_session_type_conflicts() {
  if (storage.session_type != mysqlsh::SessionType::Auto &&
      uri_data.has_scheme()) {
    if ((storage.session_type == mysqlsh::SessionType::Classic &&
         uri_data.get_scheme() != "mysql") ||
        (storage.session_type == mysqlsh::SessionType::X &&
         uri_data.get_scheme() != "mysqlx")) {
      auto error = "The given URI conflicts with the " + session_type_arg +
                   " session type option.";
      throw std::runtime_error(error);
    }
  }
}

void Shell_options::check_user_conflicts() {
  if (!storage.user.empty() && uri_data.has_user()) {
    if (storage.user != uri_data.get_user()) {
      auto error =
          "Conflicting options: provided user name differs from the "
          "user in the URI.";
      throw std::runtime_error(error);
    }
  }
}

void Shell_options::check_password_conflicts() {
  if (storage.password != NULL && uri_data.has_password()) {
    if (storage.password != uri_data.get_password()) {
      auto error =
          "Conflicting options: provided password differs from the "
          "password in the URI.";
      throw std::runtime_error(error);
    }
  }
}

void Shell_options::check_host_conflicts() {
  if (uri_data.has_host() && !storage.host.empty()) {
    if (storage.host != uri_data.get_host()) {
      auto error =
          "Conflicting options: provided host differs from the "
          "host in the URI.";
      throw std::runtime_error(error);
    }
  }
}

void Shell_options::check_host_socket_conflicts() {
  if (!storage.sock.empty()) {
    if ((!storage.host.empty() && storage.host != "localhost") ||
        (uri_data.has_host() && uri_data.get_host() != "localhost")) {
      auto error =
          "Conflicting options: socket can not be used if host is "
          "not 'localhost'.";
      throw std::runtime_error(error);
    }
  }
}

void Shell_options::check_port_conflicts() {
  if (storage.port != 0 && uri_data.has_transport_type() &&
      uri_data.get_transport_type() == mysqlshdk::db::Transport_type::Tcp &&
      uri_data.has_port() && uri_data.get_port() != storage.port) {
    auto error =
        "Conflicting options: provided port differs from the "
        "port in the URI.";
    throw std::runtime_error(error);
  }
}

void Shell_options::check_socket_conflicts() {
  if (!storage.sock.empty() && uri_data.has_transport_type() &&
      uri_data.get_transport_type() == mysqlshdk::db::Transport_type::Socket &&
      uri_data.get_socket() != storage.sock) {
    auto error =
        "Conflicting options: provided socket differs from the "
        "socket in the URI.";
    throw std::runtime_error(error);
  }
}

void Shell_options::check_port_socket_conflicts() {
  if (storage.port != 0 && !storage.sock.empty()) {
    auto error =
        "Conflicting options: port and socket can not be used "
        "together.";
    throw std::runtime_error(error);
  } else if (storage.port != 0 && uri_data.has_transport_type() &&
             uri_data.get_transport_type() ==
                 mysqlshdk::db::Transport_type::Socket) {
    auto error =
        "Conflicting options: port can not be used if the URI "
        "contains a socket.";
    throw std::runtime_error(error);
  } else if (!storage.sock.empty() && uri_data.has_port()) {
    auto error =
        "Conflicting options: socket can not be used if the URI "
        "contains a port.";
    throw std::runtime_error(error);
  }
}
}  // namespace mysqlsh
