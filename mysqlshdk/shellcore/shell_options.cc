/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/include/shellcore/shell_options.h"
#include <stdlib.h>
#include <cstdio>
#include <iostream>
#include <limits>
#include "mysqlshdk/libs/db/uri_common.h"
#include "mysqlshdk/shellcore/credential_manager.h"
#include "shellcore/ishell_core.h"
#include "shellcore/shell_notifications.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

namespace mysqlsh {

using mysqlshdk::db::Transport_type;

bool Shell_options::Storage::has_connection_data() const {
  return !uri.empty() || !user.empty() || !host.empty() || !schema.empty() ||
         !sock.empty() || port != 0 || password != NULL || prompt_password ||
         !m_connect_timeout.empty() || ssl_options.has_data();
}

/**
 * Builds Connection_options from options given by user through command line
 */
mysqlshdk::db::Connection_options Shell_options::Storage::connection_options()
    const {
  mysqlshdk::db::Connection_options target_server;
  if (!uri.empty()) {
    target_server = shcore::get_connection_options(uri);
  }

  shcore::update_connection_data(&target_server, user, password, host, port,
                                 sock, schema, ssl_options, auth_method,
                                 get_server_public_key, server_public_key_path,
                                 m_connect_timeout);

  if (no_password && !target_server.has_password()) {
    target_server.set_password("");
  }

  // If a scheme is given on the URI the session type must match the URI
  // scheme
  if (target_server.has_scheme()) {
    mysqlsh::SessionType uri_session_type = target_server.get_session_type();
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
    if (!error.empty()) throw shcore::Exception::argument_error(error);
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

  return target_server;
}

static std::string hide_password_in_uri(std::string uri,
                                        const std::string &username) {
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

using mysqlshdk::db::Ssl_options;
using shcore::opts::assign_value;
using shcore::opts::cmdline;
using shcore::opts::deprecated;
using std::placeholders::_1;
using std::placeholders::_2;

Shell_options::Shell_options(int argc, char **argv,
                             const std::string &configuration_file)
    : Options(configuration_file) {
  std::string home = shcore::get_home_dir();
#ifdef WIN32
  home += ("MySQL\\mysql-sandboxes");
#else
  home += ("mysql-sandboxes");
#endif

  const auto create_output_format_handler = [this](const char *value) {
    return [this, value](const std::string &, const char *) {
      set_output_format(value);
    };
  };

  // clang-format off
  add_startup_options()
    (&print_cmd_line_helper, false,
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
    (cmdline("--connect-timeout=#"), "Connection timeout in milliseconds.",
    std::bind(&Shell_options::set_connection_timeout, this, _1, _2))
    (&storage.sock, "", cmdline("-S", "--socket=sock"),
        "Socket name to use in UNIX, "
        "pipe name to use in Windows (only classic sessions).")
    (&storage.user, "", cmdline("-u", "--dbuser=name", "--user=name"),
        "User for the connection to the server.")
    (cmdline("-p", "--password[=name] ", "--dbpassword[=name]"),
        "Password to use when connecting to server.")
    (cmdline("-p"), "Request password prompt to set the password")
    (cmdline("--import file collection", "--import file table [column]"),
        "Import JSON documents from file to collection or table in MySQL"
        " Server. Set file to - if you want to read the data from stdin."
        " Requires a default schema on the connection options.")
    (&storage.schema, "", "schema",
        cmdline("-D", "--schema=name", "--database=name"), "Schema to use.")
    (&storage.recreate_database, false, "recreateDatabase",
        cmdline("--recreate-schema"), "Drop and recreate the specified schema. "
        "Schema will be deleted if it exists!")
    (cmdline("--mx", "--mysqlx"),
        "Uses connection data to create Creating an X protocol session.",
        std::bind(
            &Shell_options::override_session_type, this, _1, _2))
    (cmdline("--mc", "--mysql"),
        "Uses connection data to create a Classic Session.",
        std::bind(
            &Shell_options::override_session_type, this, _1, _2))
    (cmdline("-ma"), deprecated(nullptr, std::bind(
            &Shell_options::override_session_type, this, _1, _2)))
    (cmdline("-mc"), deprecated("--mc", std::bind(
            &Shell_options::override_session_type, this, _1, _2)))
    (cmdline("-mx"), deprecated("--mx", std::bind(
            &Shell_options::override_session_type, this, _1, _2)))
    (cmdline("--redirect-primary"), "Connect to the primary of the group. "
        "For use with InnoDB clusters.",
        assign_value(&storage.redirect_session,
          Shell_options::Storage::Primary))
    (cmdline("--redirect-secondary"), "Connect to a secondary of the group. "
        "For use with InnoDB clusters.",
        assign_value(&storage.redirect_session,
          Shell_options::Storage::Secondary))
    (cmdline("--cluster"), "Enable cluster management, setting the cluster "
        "global variable.",
        [this](const std::string&, const char* value) {
          storage.default_cluster = value ? value : "";
          storage.default_cluster_set = true;
        })
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
            set_output_format("json");
          } else if (strcmp(value, "raw") == 0) {
            set_output_format("json/raw");
          } else {
            throw std::invalid_argument(
                "Value for --json must be either pretty or raw.");
          }
        })
    (cmdline("--table"),
        "Produce output in table format (default for interactive mode). This "
        "option can be used to force that format when running in batch mode.",
        create_output_format_handler("table"))
    (cmdline("--tabbed"),
        "Produce output in tab separated format (default for batch mode). This "
        "option can be used to force that format when running in interactive "
        "mode.",
        create_output_format_handler("tabbed"))
    (cmdline("-E", "--vertical"),
        "Print the output of a query (rows) vertically.",
        create_output_format_handler("vertical"))
    (cmdline("--get-server-public-key"), "Request public key from the server "
        "required for RSA key pair-based password exchange. Use when "
        "connecting to MySQL 8.0 servers with classic MySQL sessions with SSL "
        "mode DISABLED.",
        assign_value(&storage.get_server_public_key, true))
    (&storage.server_public_key_path, "",
        cmdline("--server-public-key-path=path"), "The path name to a file "
        "containing a client-side copy of the public key required by the "
        "server for RSA key pair-based password exchange. Use when connecting "
        "to MySQL 8.0 servers with classic MySQL sessions with SSL mode "
        "DISABLED.");

  add_named_options()
    (&storage.output_format, "table", SHCORE_OUTPUT_FORMAT,
        "Determines output format",
        [this](const std::string &val, shcore::opts::Source) {
          set_output_format(val);
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
        ngcommon::Logger::get_level_range_info(),
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
        cmdline("--show-warnings=<true|false>"),
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
    (&storage.dba_gtid_wait_timeout, 60, SHCORE_DBA_GTID_WAIT_TIMEOUT,
        "Timeout value in seconds to wait for GTIDs to be synchronized.",
        shcore::opts::Range<int>(0, std::numeric_limits<int>::max()))
    (&storage.wizards, true, SHCORE_USE_WIZARDS, "Enables wizard mode.")
    (&storage.initial_mode, shcore::IShell_core::Mode::None,
        "defaultMode", "Specifies the shell mode to use when shell is started "
        "- one of sql, js or py.", std::bind(&shcore::parse_mode, _1),
        [](shcore::IShell_core::Mode mode) {
          return shcore::to_string(mode);
        })
    (&storage.pager, "", SHCORE_PAGER, "PAGER", cmdline("--pager=value"),
        "Pager used to display text output of statements executed in SQL mode "
        "as well as some other selected commands. Pager can be manually "
        "enabled in scripting modes. If you don't supply an "
        "option, the default pager is taken from your ENV variable PAGER. "
        "This option only works in interactive mode. This option is disabled "
        "by default.");

  add_startup_options()
    (cmdline("--name-cache"),
      "Enable database name caching for autocompletion and DevAPI (default).",
      [this](const std::string &, const char *) {
        storage.db_name_cache = true;
        storage.db_name_cache_set = true;
        storage.devapi_schema_object_handles = true;
      })
    (cmdline("-A", "--no-name-cache"),
      "Disable automatic database name caching for autocompletion and DevAPI. "
      "Use \\rehash to load DB names manually.",
      [this](const std::string &, const char *) {
        storage.db_name_cache = false;
        storage.db_name_cache_set = true;
        storage.devapi_schema_object_handles = false;
      })
    (cmdline("--nw", "--no-wizard"), "Disables wizard mode.",
        assign_value(&storage.wizards, false))
    (&storage.no_password, false, cmdline("--no-password"),
        "Sets empty password and disables prompt for password.")
    (&print_cmd_line_version, false, cmdline("-V", "--version"),
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
        std::bind(&Ssl_options::set_cipher, &storage.ssl_options, _2))
    (cmdline("--ssl-crl=name"), "Certificate revocation list.",
        std::bind(&Ssl_options::set_crl, &storage.ssl_options, _2))
    (cmdline("--ssl-crlpath=dir"), "Certificate revocation list path.",
        std::bind(&Ssl_options::set_crlpath, &storage.ssl_options, _2))
    (cmdline("--ssl-mode=mode"), "SSL mode to use, allowed values: DISABLED,"
        "PREFERRED, REQUIRED, VERIFY_CA, VERIFY_IDENTITY.",
        std::bind(&Shell_options::set_ssl_mode, this, _1, _2))
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
    (cmdline("--ssl[=opt]"), deprecated("--ssl-mode",
      std::bind(&Shell_options::set_ssl_mode, this, _1, _2), "REQUIRED",
     {
       {"1", "REQUIRED"},
       {"YES", "REQUIRED"},
       {"0", "DISABLED"},
       {"NO", "DISABLED"}
     }))
    (cmdline("--node"), deprecated("--mysqlx", std::bind(
            &Shell_options::override_session_type, this, _1, _2)))
    (cmdline("--classic"), deprecated("--mysql", std::bind(
            &Shell_options::override_session_type, this, _1, _2)))
    (cmdline("--sqln"), deprecated("--sqlx", std::bind(
            &Shell_options::override_session_type, this, _1, _2)));
  // clang-format on

  shcore::Credential_manager::get().register_options(this);

  try {
    handle_environment_options();
    if (!configuration_file.empty()) handle_persisted_options();
    handle_cmdline_options(
        argc, argv, false,
        std::bind(&Shell_options::custom_cmdline_handler, this, _1, _2));

    check_session_type_conflicts();
    check_user_conflicts();
    check_password_conflicts();
    check_host_conflicts();
    check_host_socket_conflicts();
    check_port_conflicts();
    check_socket_conflicts();
    check_port_socket_conflicts();
    check_import_options();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    storage.exit_code = 1;
  }
}

static inline std::string value_to_string(const shcore::Value &value) {
  return value.type == shcore::Value_type::String ? value.as_string()
                                                  : value.repr();
}

void Shell_options::set(const std::string &option, const shcore::Value &value) {
  try {
    shcore::Options::set(option, value_to_string(value));
  } catch (const std::exception &e) {
    throw shcore::Exception::argument_error(e.what());
  }
}

void Shell_options::notify(const std::string &option) {
  shcore::Value::Map_type_ref info = shcore::Value::new_map().as_map();
  (*info)["option"] = shcore::Value(option);
  (*info)["value"] = get(option);
  shcore::ShellNotifications::get()->notify(SN_SHELL_OPTION_CHANGED, nullptr,
                                            info);
}

void Shell_options::set_and_notify(const std::string &option,
                                   const std::string &value,
                                   bool save_to_file) {
  try {
    Options::set(option, value);
    notify(option);
    if (save_to_file) save(option);
  } catch (const std::exception &e) {
    throw shcore::Exception::argument_error(e.what());
  }
}

void Shell_options::set_and_notify(const std::string &option,
                                   const shcore::Value &value,
                                   bool save_to_file) {
  set_and_notify(option, value_to_string(value), save_to_file);
}

void Shell_options::unset(const std::string &option, bool save_to_file) {
  if (save_to_file) unsave(option);
  find_option(option)->second->reset_to_default_value();
  notify(option);
}

shcore::Value Shell_options::get(const std::string &option) {
  using shcore::opts::Concrete_option;
  auto it = named_options.find(option);
  if (it == named_options.end())
    throw std::invalid_argument("No option registered under name: " + option);
  Concrete_option<bool> *opt_bool =
      dynamic_cast<Concrete_option<bool> *>(it->second);
  if (opt_bool != nullptr) return shcore::Value(opt_bool->get());

  Concrete_option<int> *opt_int =
      dynamic_cast<Concrete_option<int> *>(it->second);
  if (opt_int != nullptr) return shcore::Value(opt_int->get());

  auto opt_set_string =
      dynamic_cast<Concrete_option<std::vector<std::string>> *>(it->second);

  if (opt_set_string != nullptr) {
    auto value = shcore::Value::new_array();
    auto array = value.as_array();

    for (const auto &val : opt_set_string->get()) {
      array->emplace_back(val);
    }

    return value;
  }

  return shcore::Value(get_value_as_string(option));
}

std::vector<std::string> Shell_options::get_named_options() {
  std::vector<std::string> res;
  for (const auto &op : named_options) res.push_back(op.first);
  return res;
}

bool Shell_options::custom_cmdline_handler(char **argv, int *argi) {
  Format arg_format = Format::INVALID;
  char *value = nullptr;

  const std::string full_opt{argv[*argi]};
  const auto opt = full_opt.substr(0, full_opt.find("="));

  const auto handle_missing_value = [&argv, &arg_format, &opt]() {
    if (Format::MISSING_VALUE == arg_format) {
      throw std::invalid_argument(std::string(argv[0]) + ": option " + opt +
                                  " requires an argument");
    }
  };

  if (strcmp(argv[*argi], "-VV") == 0) {
    print_cmd_line_version = true;
    print_cmd_line_version_extra = true;
  } else if (Format::INVALID != (arg_format = cmdline_arg_with_value(
                                     argv, argi, "--file", "-f", &value))) {
    handle_missing_value();

    storage.run_file = value;
    if (shcore::str_endswith(value, ".js")) {
      storage.initial_mode = shcore::IShell_core::Mode::JavaScript;
    } else if (shcore::str_endswith(value, ".py")) {
      storage.initial_mode = shcore::IShell_core::Mode::Python;
    } else if (shcore::str_endswith(value, ".sql")) {
      storage.initial_mode = shcore::IShell_core::Mode::SQL;
    }
    // the rest of the cmdline options, starting from here are all passed
    // through to the script
    storage.script_argv.push_back(value);
    for (++(*argi); argv[*argi]; (*argi)++) {
      storage.script_argv.push_back(argv[*argi]);
    }
  } else if (Format::INVALID != (arg_format = cmdline_arg_with_value(
                                     argv, argi, "--uri", NULL, &value)) ||
             (argv[*argi][0] != '-' && (value = argv[*argi]))) {
    handle_missing_value();

    storage.uri = value;

    uri_data = shcore::get_connection_options(storage.uri, false);

    if (uri_data.has_password()) {
      std::string nopwd_uri = hide_password_in_uri(value, uri_data.get_user());

      // Required replacement when --uri=<value>
      if (arg_format == Format::LONG) nopwd_uri = "--uri=" + nopwd_uri;

      strncpy(argv[*argi], nopwd_uri.c_str(), strlen(argv[*argi]) + 1);
    }
  } else if (Format::INVALID !=
             (arg_format = cmdline_arg_with_value(argv, argi, "--dbpassword",
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
      if (arg_format == Format::LONG) storage.password = storage.pwd.c_str();
      // --password
      else
        storage.prompt_password = true;
    } else if (arg_format !=
               Format::SEPARATE_VALUE) {  // --password=value || --pvalue
      storage.pwd.assign(value);
      storage.password = storage.pwd.c_str();

      std::string stars(storage.pwd.length(), '*');
      std::string pwd = arg_format == Format::SHORT ? "-p" : "--dbpassword=";
      pwd.append(stars);

      strncpy(argv[*argi], pwd.c_str(), strlen(argv[*argi]) + 1);
    } else {  // --password value (value is ignored)
      storage.prompt_password = true;
      (*argi)--;
    }
  } else if (Format::INVALID !=
             (arg_format = cmdline_arg_with_value(argv, argi, "--password",
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
      if (arg_format == Format::LONG) storage.password = storage.pwd.c_str();
      // --password
      else
        storage.prompt_password = true;
    } else if (arg_format !=
               Format::SEPARATE_VALUE) {  // --password=value || -pvalue
      storage.pwd.assign(value);
      storage.password = storage.pwd.c_str();

      std::string stars(storage.pwd.length(), '*');
      std::string pwd = arg_format == Format::SHORT ? "-p" : "--password=";
      pwd.append(stars);

      strncpy(argv[*argi], pwd.c_str(), strlen(argv[*argi]) + 1);
    } else {  // --password value (value is ignored)
      storage.prompt_password = true;
      (*argi)--;
    }
  } else if (strcmp("--import", argv[*argi]) == 0) {
    storage.import_args.push_back(argv[*argi]);
    (*argi)++;  // omit --import

    while (argv[*argi] != nullptr) {
      // We append --import arguments until next shell option in program
      // argument list, i.e. -* or --*. Single char '-' is a --import argument.
      if (argv[*argi][0] == '-' && argv[*argi][1] != '\0') {
        break;
      }
      storage.import_args.push_back(argv[*argi]);
      (*argi)++;
    }

    (*argi)--;
  } else {
    return false;
  }
  return true;
}

void Shell_options::override_session_type(const std::string &option,
                                          const char *value) {
  if (value != nullptr)
    throw std::runtime_error("Option " + option + " does not support value");

  if (option.find("--sql") != std::string::npos)
    storage.initial_mode = shcore::IShell_core::Mode::SQL;

  mysqlsh::SessionType new_type = mysqlsh::SessionType::Auto;
  if (option == "-mc" || option == "--mc" || option == "--mysql" ||
      option == "--sqlc")
    new_type = mysqlsh::SessionType::Classic;
  else if (option == "-mx" || option == "--mx" || option == "--mysqlx" ||
           option == "--sqlx")
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

void Shell_options::set_ssl_mode(const std::string &option, const char *value) {
  int mode = mysqlshdk::db::MapSslModeNameToValue::get_value(value);

  if (mode == 0) {
    throw std::invalid_argument(
        option +
        " must be any any of [DISABLED, PREFERRED, REQUIRED, "
        "VERIFY_CA, VERIFY_IDENTITY]");
  }

  storage.ssl_options.set_mode(static_cast<mysqlshdk::db::Ssl_mode>(mode));
}

void Shell_options::set_connection_timeout(const std::string &option,
                                           const char *value) {
  // Creates a temporary connection options object so the established
  // validations are performed on the data
  mysqlshdk::db::Connection_options options;
  options.set(mysqlshdk::db::kConnectTimeout, {value});
  storage.m_connect_timeout.assign(value);
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
      const auto error =
          "Conflicting options: provided password differs from the "
          "password in the URI.";
      throw std::runtime_error(error);
    }
  }

  if (storage.no_password && storage.prompt_password) {
    const auto error =
        "Conflicting options: --password and --no-password option cannot be "
        "used together.";
    throw std::runtime_error(error);
  }

  if (storage.no_password && uri_data.has_password() &&
      !uri_data.get_password().empty()) {
    const auto error =
        "Conflicting options: --no-password cannot be used if password is "
        "provided in URI.";
    throw std::runtime_error(error);
  }

  if (storage.no_password && storage.password && strlen(storage.password) > 0) {
    const auto error =
        "Conflicting options: --no-password cannot be used if password is "
        "provided.";
    throw std::runtime_error(error);
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
          "Conflicting options: socket cannot be used if host is "
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
        "Conflicting options: port and socket cannot be used "
        "together.";
    throw std::runtime_error(error);
  } else if (storage.port != 0 && uri_data.has_transport_type() &&
             uri_data.get_transport_type() ==
                 mysqlshdk::db::Transport_type::Socket) {
    auto error =
        "Conflicting options: port cannot be used if the URI "
        "contains a socket.";
    throw std::runtime_error(error);
  } else if (!storage.sock.empty() && uri_data.has_port()) {
    auto error =
        "Conflicting options: socket cannot be used if the URI "
        "contains a port.";
    throw std::runtime_error(error);
  }
}

void Shell_options::set_output_format(const std::string &format) {
  if (format != "table" && format != "json" && format != "json/raw" &&
      format != "vertical" && format != "tabbed") {
    throw shcore::Exception::value_error(
        "The option " SHCORE_OUTPUT_FORMAT
        " must be one of: tabbed, table, vertical, json or json/raw.");
  }

  storage.output_format = format;
  ngcommon::Logger::set_stderr_output_format(format);
}

void Shell_options::check_import_options() {
  bool is_schema_set = !storage.schema.empty() || uri_data.has_schema();
  if (!storage.import_args.empty()) {
    if (!storage.has_connection_data()) {
      const auto error = "Error: --import requires an active session.";
      throw std::runtime_error(error);
    }
    if (!is_schema_set) {
      const auto error =
          "Error: --import requires a default schema on the active session.";
      throw std::runtime_error(error);
    }
  }
}

}  // namespace mysqlsh
