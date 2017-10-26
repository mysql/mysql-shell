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

#include "src/mysqlsh/shell_cmdline_options.h"
#include <stdlib.h>
#include <cstdio>
#include <iostream>
#include "mysqlshdk/libs/db/uri_common.h"
#include "shellcore/ishell_core.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

#define SHCORE_OUTPUT_FORMAT "outputFormat"
#define SHCORE_INTERACTIVE "interactive"
#define SHCORE_SHOW_WARNINGS "showWarnings"
#define SHCORE_BATCH_CONTINUE_ON_ERROR "batchContinueOnError"
#define SHCORE_USE_WIZARDS "useWizards"
#define SHCORE_SANDBOX_DIR "sandboxDir"

#define SHCORE_HISTORY_MAX_SIZE "history.maxSize"
#define SHCORE_HISTIGNORE "history.sql.ignorePattern"
#define SHCORE_HISTORY_AUTOSAVE "history.autoSave"

using mysqlshdk::db::Transport_type;

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

Shell_command_line_options::Shell_command_line_options(int argc,
                                                       const char** argv)
    : Options(false,
              std::bind(&Shell_command_line_options::custom_cmdline_handler,
                        this, _1, _2)) {
  std::string home = shcore::get_home_dir();
#ifdef WIN32
  home += ("MySQL\\mysql-sandboxes");
#else
  home += ("mysql-sandboxes");
#endif
  add_startup_options()
    (&shell_options.print_cmd_line_helper, false,
        cmdline("-?", "--help"), "Display this help and exit.")
    (&shell_options.execute_statement, "", cmdline("-e", "--execute=<cmd>"),
        "Execute command and quit.")
    (cmdline("-f", "--file=file"), "Process file.")
    (cmdline("--uri=value"), "Connect to Uniform Resource Identifier. "
        "Format: [user[:pass]@]host[:port][/db]")
    (&shell_options.host, "", cmdline("-h", "--host=name"),
        "Connect to host.")
    (&shell_options.port, 0, cmdline("-P", "--port=#"),
        "Port number to use for connection.")
    (&shell_options.sock, "", cmdline("-S", "--socket=sock"),
        "Socket name to use in UNIX, "
        "pipe name to use in Windows (only classic sessions).")
    (&shell_options.user, "", cmdline("-u", "--dbuser=name", "--user=name"),
        "User for the connection to the server.")
    (cmdline("-p", "--password[=name] ", "--dbpassword[=name]"),
        "Password to use when connecting to server.")
    (cmdline("-p"), "Request password prompt to set the password")
    (&shell_options.schema, "", "schema",
        cmdline("-D", "--schema=name", "--database=name"), "Schema to use.")
    (&shell_options.recreate_database, false, "recreateDatabase",
        cmdline("--recreate-schema"), "Drop and recreate the specified schema."
        "Schema will be deleted if it exists!")
    (cmdline("-mx", "--mysqlx"),
        "Uses connection data to create Creating an X protocol session.",
        std::bind(
            &Shell_command_line_options::override_session_type, this, _1, _2))
    (cmdline("-mc", "--mysql"),
        "Uses connection data to create a Classic Session.",
        std::bind(
            &Shell_command_line_options::override_session_type, this, _1, _2))
    (cmdline("-ma"), "Uses the connection data to create the session with"
        "automatic protocol detection.",
        std::bind(
            &Shell_command_line_options::override_session_type, this, _1, _2))
    (cmdline("--sql"), "Start in SQL mode.", assign_value(
        &shell_options.initial_mode, shcore::IShell_core::Mode::SQL))
    (cmdline("--sqlc"), "Start in SQL mode using a classic session.",
        std::bind(
            &Shell_command_line_options::override_session_type, this, _1, _2))
    (cmdline("--sqlx"),
        "Start in SQL mode using Creating an X protocol session.",
        std::bind(
            &Shell_command_line_options::override_session_type, this, _1, _2))
    (cmdline("--js", "--javascript"), "Start in JavaScript mode.",
        [this](const std::string&, const char*) {
#ifdef HAVE_V8
          shell_options.initial_mode = shcore::IShell_core::Mode::JavaScript;
#else
          throw std::invalid_argument("JavaScript is not supported.");
#endif
        })
    (cmdline("--py", "--python"), "Start in Python mode.",
        [this](const std::string&, const char*) {
#ifdef HAVE_PYTHON
      shell_options.initial_mode = shcore::IShell_core::Mode::Python;
#else
      throw std::invalid_argument("Python is not supported.");
#endif
      })
    (cmdline("--json[=format]"),
        "Produce output in JSON format, allowed values:"
        "raw, pretty. If no format is specified pretty format is produced.",
        [this](const std::string&, const char* value) {
          if (!value || strcmp(value, "pretty") == 0) {
            shell_options.output_format = "json";
          } else if (strcmp(value, "raw") == 0) {
            shell_options.output_format = "json/raw";
          } else {
            throw std::invalid_argument(
                "Value for --json must be either pretty or raw.");
          }
        })
    (cmdline("--table"),
        "Produce output in table format (default for interactive mode). This "
        "option can be used to, force that format when running in batch mode.",
        assign_value(&shell_options.output_format, "table"))
    (cmdline("-E", "--vertical"),
        "Print the output of a query (rows) vertically.",
        assign_value(&shell_options.output_format, "vertical"));

  add_named_options()
    (&shell_options.output_format, "", SHCORE_OUTPUT_FORMAT,
        "Determines output format")
    (&shell_options.interactive, false, SHCORE_INTERACTIVE,
        "Enables interactive mode", shcore::opts::Read_only<bool>());

  add_startup_options()(
      cmdline("-i", "--interactive[=full]"),
      "To use in batch mode. "
      "It forces emulation of interactive mode processing. Each "
      "line on the batch is processed as if it were in interactive mode.",
      [this](const std::string&, const char* value) {
        if (!value) {
          shell_options.interactive = true;
          shell_options.full_interactive = false;
        } else if (strcmp(value, "full") == 0) {
          shell_options.interactive = true;
          shell_options.full_interactive = true;
        } else {
          throw std::invalid_argument(
                    "Value for --interactive if any, must be full\n");
        }
      });

  add_named_options()
    (&shell_options.force, false, "force", cmdline("--force"),
        "To use in SQL batch mode, forces processing to "
        "continue if an error is found.")
    (&shell_options.log_level, ngcommon::Logger::LOG_INFO, "logLevel",
        cmdline("--log-level=value"),
        "The log level. Value must be an integer between 1 and 8 any of "
        "[none, internal, error, warning, info, debug, debug2, debug3].",
        [this](const std::string &val, shcore::opts::Source){
          const char* value = val.c_str();
          if (*value == '@') {
            shell_options.log_to_stderr = true;
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
    (&shell_options.passwords_from_stdin, false, "passwordsFromStdin",
        cmdline("--passwords-from-stdin"),
        "Read passwords from stdin instead of the tty.")
    (&shell_options.show_warnings, true, SHCORE_SHOW_WARNINGS,
        cmdline("--show-warnings"),
        "Automatically display SQL warnings on SQL mode if available.",
        shcore::opts::Read_only<bool>())
    (&shell_options.histignore, "*IDENTIFIED*:*PASSWORD*", SHCORE_HISTIGNORE,
        cmdline("--histignore=filters"), "History ignore.")
    (&shell_options.sandbox_directory, home, SHCORE_SANDBOX_DIR,
        "Default sandbox directory")
    (&shell_options.wizards, true, SHCORE_USE_WIZARDS, "Enables wizard mode.");

  add_startup_options()
    (cmdline("--nw", "--no-wizard"), "Disables wizard mode.",
        assign_value(&shell_options.wizards, false))
    (&shell_options.print_version, false, cmdline("-V", "--version"),
        "Prints the version of MySQL Shell.")
    (cmdline("--ssl-key=name"), "X509 key in PEM format.",
        std::bind(&Ssl_options::set_key, &shell_options.ssl_options, _2))
    (cmdline("--ssl-cert=name"), "X509 cert in PEM format.",
        std::bind(&Ssl_options::set_cert, &shell_options.ssl_options, _2))
    (cmdline("--ssl-ca=name"), "CA file in PEM format.",
        std::bind(&Ssl_options::set_ca, &shell_options.ssl_options, _2))
    (cmdline("--ssl-capath=dir"), "CA directory.",
        std::bind(&Ssl_options::set_capath, &shell_options.ssl_options, _2))
    (cmdline("--ssl-cipher=name"), "SSL Cipher to use.",
        std::bind(&Ssl_options::set_ciphers, &shell_options.ssl_options, _2))
    (cmdline("--ssl-crl=name"), "Certificate revocation list.",
        std::bind(&Ssl_options::set_crl, &shell_options.ssl_options, _2))
    (cmdline("--ssl-crlpath=dir"), "Certificate revocation list path.",
        std::bind(&Ssl_options::set_crlpath, &shell_options.ssl_options, _2))
    (cmdline("--ssl-mode=mode"), "SSL mode to use, allowed values: DISABLED,"
        "PREFERRED, REQUIRED, VERIFY_CA, VERIFY_IDENTITY.",
        [this](const std::string& opt, const char* value) {
          int mode = mysqlshdk::db::MapSslModeNameToValue::get_value(value);
          if (mode == 0)
            throw std::invalid_argument(opt +
                " must be any any of [DISABLED, PREFERRED, REQUIRED, "
                         "VERIFY_CA, VERIFY_IDENTITY]");
          shell_options.ssl_options.set_mode(
              static_cast<mysqlshdk::db::Ssl_mode>(mode));})
    (cmdline("--tls-version=version"),
        "TLS version to use, permitted values are: TLSv1, TLSv1.1.",
        std::bind(&Ssl_options::set_tls_version,
            &shell_options.ssl_options, _2))
    (&shell_options.auth_method, "", cmdline("--auth-method=method"),
        "Authentication method to use.")
    (&shell_options.execute_dba_statement, "",
        cmdline("--dba=enableXProtocol"), "Enable the X Protocol "
        "in the server connected to. Must be used with --mysql.")
    (cmdline("--trace-proto"),
        assign_value(&shell_options.trace_protocol, true))
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
    shell_options.exit_code = 1;
  }
}

bool Shell_command_line_options::custom_cmdline_handler(const char** argv,
                                                        int* argi) {
  int arg_format = 0;
  const char* value = nullptr;

  if (strcmp(argv[*argi], "-VV") == 0) {
    shell_options.print_version = true;
    shell_options.print_version_extra = true;
  } else if (cmdline_arg_with_value(argv, argi, "--file", "-f", &value)) {
    shell_options.run_file = value;
    // the rest of the cmdline options, starting from here are all passed
    // through to the script
    shell_options.script_argv.push_back(value);
    for (++(*argi); argv[*argi]; (*argi)++) {
      shell_options.script_argv.push_back(argv[*argi]);
    }
  } else if ((arg_format =
                  cmdline_arg_with_value(argv, argi, "--uri", NULL, &value)) ||
             (argv[*argi][0] != '-' && (value = argv[*argi]))) {
    shell_options.uri = value;

    uri_data = shcore::get_connection_options(shell_options.uri, false);

    if (uri_data.has_password()) {
      std::string nopwd_uri = hide_password_in_uri(value, uri_data.get_user());

      // Required replacement when --uri=<value>
      if (arg_format == 3)
        nopwd_uri = "--uri=" + nopwd_uri;

      snprintf(const_cast<char*>(argv[*argi]), nopwd_uri.length() + 1, "%s",
               nopwd_uri.c_str());
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
        shell_options.password = shell_options.pwd.c_str();
      // --password
      else
        shell_options.prompt_password = true;
    } else if (arg_format != 1) {  // --password=value || --pvalue
      shell_options.pwd.assign(value);
      shell_options.password = shell_options.pwd.c_str();

      std::string stars(shell_options.pwd.length(), '*');
      std::string pwd = arg_format == 2 ? "-p" : "--dbpassword=";
      pwd.append(stars);

      snprintf(const_cast<char*>(argv[*argi]), pwd.length(), "%s", pwd.c_str());
    } else {  // --password value (value is ignored)
      shell_options.prompt_password = true;
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
        shell_options.password = shell_options.pwd.c_str();
      // --password
      else
        shell_options.prompt_password = true;
    } else if (arg_format != 1) {  // --password=value || --pvalue
      shell_options.pwd.assign(value);
      shell_options.password = shell_options.pwd.c_str();

      std::string stars(shell_options.pwd.length(), '*');
      std::string pwd = arg_format == 2 ? "-p" : "--password=";
      pwd.append(stars);

      snprintf(const_cast<char*>(argv[*argi]), pwd.length(), "%s", pwd.c_str());
    } else {  // --password value (value is ignored)
      shell_options.prompt_password = true;
      (*argi)--;
    }
  } else {
    return false;
  }
  return true;
}

void Shell_command_line_options::override_session_type(
    const std::string& option, const char* value) {
  if (value != nullptr)
    throw std::runtime_error("Option " + option + " does not support value");

  if (option.find("--sql") != std::string::npos)
    shell_options.initial_mode = shcore::IShell_core::Mode::SQL;

  mysqlsh::SessionType new_type = mysqlsh::SessionType::Auto;
  if (option == "-mc" || option == "--mysql" || option == "--sqlc")
    new_type = mysqlsh::SessionType::Classic;
  else if (option == "-mx" || option == "--mysqlx" || option == "--sqlx")
    new_type = mysqlsh::SessionType::X;

  if (new_type != shell_options.session_type) {
    if (!shell_options.default_session_type) {
      std::stringstream ss;
      if (shell_options.session_type == mysqlsh::SessionType::Auto) {
        ss << "Automatic protocol detection is enabled, unable to change to "
           << get_session_type_name(new_type) << " with option " << option;
      } else if (new_type == mysqlsh::SessionType::Auto) {
        ss << "Session type already configured to "
           << get_session_type_name(shell_options.session_type)
           << ", automatic protocol detection (-ma) can't be enabled.";
      } else {
        ss << "Session type already configured to "
           << get_session_type_name(shell_options.session_type)
           << ", unable to change to " << get_session_type_name(new_type)
           << " with option " << option;
      }
      throw std::invalid_argument(ss.str());
    }

    shell_options.session_type = new_type;
    session_type_arg = option;
  }

  shell_options.default_session_type = false;
}

void Shell_command_line_options::check_session_type_conflicts() {
  if (shell_options.session_type != mysqlsh::SessionType::Auto &&
      uri_data.has_scheme()) {
    if ((shell_options.session_type == mysqlsh::SessionType::Classic &&
         uri_data.get_scheme() != "mysql") ||
        (shell_options.session_type == mysqlsh::SessionType::X &&
         uri_data.get_scheme() != "mysqlx")) {
      auto error = "The given URI conflicts with the " + session_type_arg +
                   " session type option.";
      throw std::runtime_error(error);
    }
  }
}

void Shell_command_line_options::check_user_conflicts() {
  if (!shell_options.user.empty() && uri_data.has_user()) {
    if (shell_options.user != uri_data.get_user()) {
      auto error =
          "Conflicting options: provided user name differs from the "
          "user in the URI.";
      throw std::runtime_error(error);
    }
  }
}

void Shell_command_line_options::check_password_conflicts() {
  if (shell_options.password != NULL && uri_data.has_password()) {
    if (shell_options.password != uri_data.get_password()) {
      auto error =
          "Conflicting options: provided password differs from the "
          "password in the URI.";
      throw std::runtime_error(error);
    }
  }
}

void Shell_command_line_options::check_host_conflicts() {
  if (uri_data.has_host() && !shell_options.host.empty()) {
    if (shell_options.host != uri_data.get_host()) {
      auto error =
          "Conflicting options: provided host differs from the "
          "host in the URI.";
      throw std::runtime_error(error);
    }
  }
}

void Shell_command_line_options::check_host_socket_conflicts() {
  if (!shell_options.sock.empty()) {
    if ((!shell_options.host.empty() && shell_options.host != "localhost") ||
        (uri_data.has_host() && uri_data.get_host() != "localhost")) {
      auto error =
          "Conflicting options: socket can not be used if host is "
          "not 'localhost'.";
      throw std::runtime_error(error);
    }
  }
}

void Shell_command_line_options::check_port_conflicts() {
  if (shell_options.port != 0 && uri_data.has_transport_type() &&
      uri_data.get_transport_type() == mysqlshdk::db::Transport_type::Tcp &&
      uri_data.has_port() && uri_data.get_port() != shell_options.port) {
    auto error =
        "Conflicting options: provided port differs from the "
        "port in the URI.";
    throw std::runtime_error(error);
  }
}

void Shell_command_line_options::check_socket_conflicts() {
  if (!shell_options.sock.empty() && uri_data.has_transport_type() &&
      uri_data.get_transport_type() == mysqlshdk::db::Transport_type::Socket &&
      uri_data.get_socket() != shell_options.sock) {
    auto error =
        "Conflicting options: provided socket differs from the "
        "socket in the URI.";
    throw std::runtime_error(error);
  }
}

void Shell_command_line_options::check_port_socket_conflicts() {
  if (shell_options.port != 0 && !shell_options.sock.empty()) {
    auto error =
        "Conflicting options: port and socket can not be used "
        "together.";
    throw std::runtime_error(error);
  } else if (shell_options.port != 0 && uri_data.has_transport_type() &&
             uri_data.get_transport_type() ==
                 mysqlshdk::db::Transport_type::Socket) {
    auto error =
        "Conflicting options: port can not be used if the URI "
        "contains a socket.";
    throw std::runtime_error(error);
  } else if (!shell_options.sock.empty() && uri_data.has_port()) {
    auto error =
        "Conflicting options: socket can not be used if the URI "
        "contains a port.";
    throw std::runtime_error(error);
  }
}
