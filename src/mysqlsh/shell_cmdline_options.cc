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

#include <stdlib.h>
#include <iostream>
#include <cstdio>
#include "shellcore/ishell_core.h"
#include "shell_cmdline_options.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"
#include "mysqlshdk/libs/db/uri_common.h"

using mysqlshdk::db::Transport_type;

Shell_command_line_options::Shell_command_line_options(int argc,
                                                       const char** argv)
    : Command_line_options(argc, argv) {
  int arg_format = 0;
  for (int i = 1; i < argc && exit_code == 0; i++) {
    const char *value;
    if (check_arg_with_value(argv, i, "--file", "-f", value)) {
      _options.run_file = value;
      // the rest of the cmdline options, starting from here are all passed
      // through to the script
      _options.script_argv.push_back(value);
      for (++i; i < argc; i++) {
        _options.script_argv.push_back(argv[i]);
      }
      break;
    } else if ((arg_format = check_arg_with_value(argv, i, "--uri", NULL, value))) {
      try {
        _options.uri = value;

        _uri_data = shcore::get_connection_options(_options.uri,
                                                   false);

        if (_uri_data.has_password()) {
          std::string pwd(_uri_data.get_password().length(), '*');

          std::string old_uri(value);
          auto nopwd_uri = old_uri.replace(_uri_data.get_user().length()+1,
                                           pwd.length(), pwd);

          // Required replacement when --uri=<value>
          if (arg_format == 3)
            nopwd_uri = "--uri=" + nopwd_uri;

          strcpy(const_cast<char*>(argv[i]), nopwd_uri.substr(0, nopwd_uri.length()).c_str());
        }
      } catch (const std::invalid_argument &error) {
        std::cerr << "Invalid value specified in --uri parameter.\n";
        exit_code = 1;
        break;
      }
    }
    else if (check_arg_with_value(argv, i, "--host", "-h", value))
      _options.host = value;
    else if (check_arg_with_value(argv, i, "--dbuser", "-u", value))
      _options.user = value;
    else if (check_arg_with_value(argv, i, "--user", NULL, value))
      _options.user = value;
    else if (check_arg_with_value(argv, i, "--port", "-P", value))
      _options.port = atoi(value);
    else if (check_arg_with_value(argv, i, "--socket", "-S", value))
      _options.sock = value;
    else if (check_arg_with_value(argv, i, "--schema", "-D", value))
      _options.schema = value;
    else if (check_arg_with_value(argv, i, "--database", NULL, value))
      _options.schema = value;
    else if (check_arg(argv, i, "--recreate-schema", NULL))
      _options.recreate_database = true;
    else if (check_arg_with_value(argv, i, "--execute", "-e", value))
      _options.execute_statement = value;
    else if (check_arg_with_value(argv, i, "--dba", NULL, value))
      _options.execute_dba_statement = value;
    else if ((arg_format = check_arg_with_value(argv, i, "--dbpassword", NULL, value, true))) {
      // Note that in any connection attempt, password prompt will be done if the password is missing.
      // The behavior of the password cmd line argument is as follows:

      // ARGUMENT           EFFECT
      // --password         forces password prompt no matter it was already provided
      // --password value   forces password prompt no matter it was already provided (value is not taken as password)
      // --password=        sets password to empty (password is available but empty so it will not be prompted)
      // -p<value> sets the password to <value>
      // --password=<value> sets the password to <value>

      if (!value) {
        // --password=
        if (arg_format == 3)
          _options.password = _options.pwd.c_str();

        // --password
        else
          _options.prompt_password = true;
      }
      // --password=value || --pvalue
      else if (arg_format != 1) {
        _options.pwd.assign(value);
        _options.password = _options.pwd.c_str();

        std::string stars(_options.pwd.length(), '*');
        std::string pwd = arg_format == 2 ? "-p" : "--dbpassword=";
        pwd.append(stars);

        strcpy(const_cast<char*>(argv[i]), pwd.c_str());
      }

      // --password value (value is ignored)
      else {
        _options.prompt_password = true;
        i--;
      }
    } else if ((arg_format = check_arg_with_value(argv, i, "--password", "-p", value, true))) {
      // Note that in any connection attempt, password prompt will be done if the password is missing.
      // The behavior of the password cmd line argument is as follows:

      // ARGUMENT           EFFECT
      // --password         forces password prompt no matter it was already provided
      // --password value   forces password prompt no matter it was already provided (value is not taken as password)
      // --password=        sets password to empty (password is available but empty so it will not be prompted)
      // -p<value> sets the password to <value>
      // --password=<value> sets the password to <value>

      if (!value) {
        // --password=
        if (arg_format == 3)
          _options.password = _options.pwd.c_str();

        // --password
        else
          _options.prompt_password = true;
      }
      // --password=value || --pvalue
      else if (arg_format != 1) {
        _options.pwd.assign(value);
        _options.password = _options.pwd.c_str();

        std::string stars(_options.pwd.length(), '*');
        std::string pwd = arg_format == 2 ? "-p" : "--password=";
        pwd.append(stars);

        strcpy(const_cast<char*>(argv[i]), pwd.c_str());
      }

      // --password value (value is ignored)
      else {
        _options.prompt_password = true;
        i--;
      }
    } else if (check_arg_with_value(argv, i, "--auth-method", NULL, value))
      _options.auth_method = value;
    else if (check_arg_with_value(argv, i, "--ssl-ca", NULL, value)) {
      _options.ssl_options.set_ca(value);
    } else if (check_arg_with_value(argv, i, "--ssl-cert", NULL, value)) {
      _options.ssl_options.set_cert(value);
    } else if (check_arg_with_value(argv, i, "--ssl-key", NULL, value)) {
      _options.ssl_options.set_key(value);
    } else if (check_arg_with_value(argv, i, "--ssl-capath", NULL, value)) {
      _options.ssl_options.set_capath(value);
    } else if (check_arg_with_value(argv, i, "--ssl-crl", NULL, value)) {
      _options.ssl_options.set_crl(value);
    } else if (check_arg_with_value(argv, i, "--ssl-crlpath", NULL, value)) {
      _options.ssl_options.set_crlpath(value);
    } else if (check_arg_with_value(argv, i, "--ssl-cipher", NULL, value)) {
      _options.ssl_options.set_ciphers(value);
    } else if (check_arg_with_value(argv, i, "--tls-version", NULL, value)) {
      _options.ssl_options.set_tls_version(value);
    } else if (check_arg_with_value(argv, i, "--ssl-mode", NULL, value)) {
      int mode = mysqlshdk::db::MapSslModeNameToValue::get_value(value);
      if (mode == 0) {
        std::cerr << "must be any any of [DISABLED, PREFERRED, REQUIRED, "
                     "VERIFY_CA, VERIFY_IDENTITY]";
        exit_code = 1;
        break;
      }
      _options.ssl_options.set_mode(mode);
    } else if (check_arg_with_value(argv, i, "--ssl", NULL, value, true)) {
      std::cerr << "The --ssl option is deprecated, use --ssl-mode instead";
      exit_code = 1;
      break;
    } else if (check_arg(argv, i, "--node", "--node")) {
      std::cerr << "The --node option has been deprecated, "
                   "please use --mysqlx or -mx instead.\n";
      exit_code = 1;
      break;
    } else if (check_arg(argv, i, "--classic", "--classic")) {
      std::cerr << "The --classic option has been deprecated, "
                   "please use --mysql or -mc instead.\n";
      exit_code = 1;
      break;
    } else if (check_arg(argv, i, "-mc", "--mysql"))
      override_session_type(mysqlsh::SessionType::Classic, "--mysql");
    else if (check_arg(argv, i, "-mx", "--mysqlx"))
      override_session_type(mysqlsh::SessionType::X, "--mysqlx");
    else if (check_arg(argv, i, "-ma", "-ma"))
      override_session_type(mysqlsh::SessionType::Auto, "-ma");
    else if (check_arg(argv, i, "--sql", "--sql")) {
      _options.initial_mode = shcore::IShell_core::Mode::SQL;
    } else if (check_arg(argv, i, "--js", "--javascript")) {
#ifdef HAVE_V8
      _options.initial_mode = shcore::IShell_core::Mode::JavaScript;
#else
      std::cerr << "JavaScript is not supported.\n";
      exit_code = 1;
      break;
#endif
    } else if (check_arg(argv, i, "--py", "--python")) {
#ifdef HAVE_PYTHON
      _options.initial_mode = shcore::IShell_core::Mode::Python;
#else
      std::cerr << "Python is not supported.\n";
      exit_code = 1;
      break;
#endif
    } else if (check_arg(argv, i, NULL, "--sqlc")) {
      _options.initial_mode = shcore::IShell_core::Mode::SQL;
      override_session_type(mysqlsh::SessionType::Classic, "--sqlc");
    } else if (check_arg(argv, i, NULL, "--sqln")) {
      std::cerr << "The --sqln option has been deprecated, \
                    please use --sqlx instead.";
      exit_code = 1;
      break;
    } else if (check_arg(argv, i, NULL, "--sqlx")) {
      _options.initial_mode = shcore::IShell_core::Mode::SQL;
      override_session_type(mysqlsh::SessionType::X, "--sqlx");
    } else if (check_arg_with_value(argv, i, "--json", NULL, value, true)) {
      if (!value || strcmp(value, "pretty") == 0) {
        _options.output_format = "json";
      } else if (strcmp(value, "raw") == 0) {
        _options.output_format = "json/raw";
      } else {
        std::cerr << "Value for --json must be either pretty or raw.\n";
        exit_code = 1;
        break;
      }
    } else if (check_arg(argv, i, "--table", "--table")) {
      _options.output_format = "table";
    } else if (check_arg(argv, i, "--trace-proto", NULL)) {
      _options.trace_protocol = true;
    } else if (check_arg(argv, i, "--help", "-?")) {
      _options.print_cmd_line_helper = true;
      exit_code = 0;
    } else if (check_arg(argv, i, nullptr, "-VV")) {
      _options.print_version = true;
      _options.print_version_extra = true;
      exit_code = 0;
    } else if (check_arg(argv, i, "--version", "-V")) {
      _options.print_version = true;
      exit_code = 0;
    } else if (check_arg(argv, i, "--force", "--force"))
      _options.force = true;
    else if (check_arg(argv, i, "--no-wizard", "--nw"))
      _options.wizards = false;
    else if (check_arg_with_value(argv, i, "--interactive", "-i", value, true)) {
      if (!value) {
        _options.interactive = true;
        _options.full_interactive = false;
      } else if (strcmp(value, "full") == 0) {
        _options.interactive = true;
        _options.full_interactive = true;
      } else {
        std::cerr << "Value for --interactive if any, must be full\n";
        exit_code = 1;
        break;
      }
    } else if (check_arg(argv, i, NULL, "--passwords-from-stdin"))
      _options.passwords_from_stdin = true;
    else if (check_arg_with_value(argv, i, "--log-level", NULL, value)) {
      ngcommon::Logger::LOG_LEVEL nlog_level;
      if (*value == '@') {
        _options.log_to_stderr = true;
        value++;
      }
      nlog_level = ngcommon::Logger::get_log_level(value);
      if (nlog_level == ngcommon::Logger::LOG_NONE && !ngcommon::Logger::is_level_none(value)) {
        std::cerr << ngcommon::Logger::get_level_range_info() << std::endl;
        exit_code = 1;
        break;
      } else
        _options.log_level = nlog_level;
    } else if (check_arg(argv, i, "--vertical", "-E")) {
      _options.output_format = "vertical";
    } else if (check_arg_with_value(argv, i, "--histignore", nullptr, value)) {
      _options.histignore = value;
    } else if (exit_code == 0) {
      if (argv[i][0] != '-') {
          value = argv[i];
        try {
          _options.uri = value;
          auto data = shcore::get_connection_options(value, false);
          if (data.has_password()) {
            std::string pwd(data.get_password().length(), '*');
            data.clear_password();
            data.set_password(pwd);

            // Hide password being used.
            auto nopwd_uri =
                data.as_uri(mysqlshdk::db::uri::formats::full());
            snprintf(const_cast<char*>(argv[i]), nopwd_uri.length() + 1, "%s",
                   nopwd_uri.c_str());
          }
        } catch (const std::invalid_argument& error) {
          std::cerr << "Invalid uri parameter.\n";
          exit_code = 1;
          break;
        }
      }
      else {
        std::cerr << argv[0] << ": unknown option " << argv[i] << "\n";
        exit_code = 1;
        break;
      }
    }
  }

  try {
    check_session_type_conflicts();
    check_user_conflicts();
    check_password_conflicts();
    check_host_conflicts();
    check_host_socket_conflicts();
    check_port_conflicts();
    check_socket_conflicts();
    check_port_socket_conflicts();
  } catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    exit_code = 1;
  }

  _options.exit_code = exit_code;
}

void Shell_command_line_options::check_session_type_conflicts() {
  if (_options.session_type != mysqlsh::SessionType::Auto &&
     _uri_data.has_scheme()) {
    if ((_options.session_type == mysqlsh::SessionType::Classic &&
      _uri_data.get_scheme() != "mysql") ||
      (_options.session_type == mysqlsh::SessionType::X &&
      _uri_data.get_scheme() != "mysqlx")) {
      auto error = "Provided URI is not compatible with " +\
                   get_session_type_name(_options.session_type) + " session "
                   "configured with " + _session_type_arg + ".";
      throw std::runtime_error(error);
    }
  }
}

void Shell_command_line_options::check_user_conflicts() {
  if (!_options.user.empty() && _uri_data.has_user()) {
    if (_options.user != _uri_data.get_user()) {
      auto error = "Conflicting options: provided user name differs from the "
                   "user in the URI.";
      throw std::runtime_error(error);
    }
  }
}

void Shell_command_line_options::check_password_conflicts() {
  if (_options.password != NULL && _uri_data.has_password()) {
    if (_options.password != _uri_data.get_password()) {
      auto error = "Conflicting options: provided password differs from the "
                   "password in the URI.";
      throw std::runtime_error(error);
    }
  }
}

void Shell_command_line_options::check_host_conflicts() {
  if (_uri_data.has_host() && !_options.host.empty()) {
    if (_options.host != _uri_data.get_host()) {
      auto error = "Conflicting options: provided host differs from the "
                   "host in the URI.";
      throw std::runtime_error(error);
    }
  }
}

void Shell_command_line_options::check_host_socket_conflicts() {
  if (!_options.sock.empty()) {
    if ((!_options.host.empty() && _options.host != "localhost") ||
        (_uri_data.has_host() &&
         _uri_data.get_host() != "localhost")) {
      auto error =  "Conflicting options: socket can not be used if host is "
                    "not 'localhost'.";
      throw std::runtime_error(error);
    }
  }
}

void Shell_command_line_options::check_port_conflicts() {
  if (_options.port != 0 &&
      _uri_data.has_transport_type() &&
      _uri_data.get_transport_type() == mysqlshdk::db::Transport_type::Tcp &&
      _uri_data.has_port() &&
      _uri_data.get_port() != _options.port) {
    auto error = "Conflicting options: provided port differs from the "
                 "port in the URI.";
    throw std::runtime_error(error);
  }
}

void Shell_command_line_options::check_socket_conflicts() {
  if (!_options.sock.empty() &&
      _uri_data.has_transport_type() &&
      _uri_data.get_transport_type() == mysqlshdk::db::Transport_type::Socket &&
      _uri_data.get_socket() != _options.sock ) {
    auto error = "Conflicting options: provided socket differs from the "
                 "socket in the URI.";
    throw std::runtime_error(error);
  }
}

void Shell_command_line_options::check_port_socket_conflicts() {
  if (_options.port != 0 && !_options.sock.empty()) {
    auto error =
        "Conflicting options: port and socket can not be used "
        "together.";
    throw std::runtime_error(error);
  } else if (_options.port != 0 && _uri_data.has_transport_type() &&
             _uri_data.get_transport_type() ==
                 mysqlshdk::db::Transport_type::Socket) {
    auto error =
        "Conflicting options: port can not be used if the URI "
        "contains a socket.";
    throw std::runtime_error(error);
  } else if (!_options.sock.empty() && _uri_data.has_port()) {
    auto error =
        "Conflicting options: socket can not be used if the URI "
        "contains a port.";
    throw std::runtime_error(error);
  }
}

std::string Shell_command_line_options::get_session_type_name(
    mysqlsh::SessionType type) {
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

void Shell_command_line_options::override_session_type(mysqlsh::SessionType new_type, const std::string& option, char* value) {
  if (new_type != _options.session_type) {
    if (!_options.default_session_type) {
      std::string msg = "";
      if (_options.session_type == mysqlsh::SessionType::Auto) {
        msg.append("Automatic protocol detection is enabled, unable to change to ");
        msg.append(get_session_type_name(new_type));
        msg.append(" with option ");
        msg.append(option);
      } else if (new_type == mysqlsh::SessionType::Auto) {
        msg.append("Session type already configured to ");
        msg.append(get_session_type_name(_options.session_type));
        msg.append(", automatic protocol detection (-ma) can't be enabled.");
      } else {
        msg.append("Session type already configured to ");
        msg.append(get_session_type_name(_options.session_type));
        msg.append(", unable to change to ");
        msg.append(get_session_type_name(new_type));
        msg.append(" with option ");
        msg.append(option);
      }

      if (value) {
        msg.append("=");
        msg.append(value);
      }

      std::cerr << msg << std::endl;
      exit_code = 1;
    }

    _options.session_type = new_type;
    _session_type_arg = option;
  }

  _options.default_session_type = false;
}

std::vector<std::string> Shell_command_line_options::get_details() {
  const std::vector<std::string> details = {
  "  -?, --help               Display this help and exit.",
  "  -f, --file=file          Process file.",
  "  -e, --execute=<cmd>      Execute command and quit.",
  "  --uri                    Connect to Uniform Resource Identifier.",
  "                           Format: [user[:pass]@]host[:port][/db]",
  "  -h, --host=name          Connect to host.",
  "  -P, --port=#             Port number to use for connection.",
  "  -S, --socket=sock        Socket name to use in UNIX, pipe name to use in",
  "                           Windows (only classic sessions).",
  "  -u, --dbuser=name        User for the connection to the server.",
  "  --user=name              An alias for dbuser.",
  "  --dbpassword=name        Password to use when connecting to server",
  "  -p, --password=name      An alias for dbpassword.",
  "  -p                       Request password prompt to set the password",
  "  -D, --schema=name        Schema to use.",
  "  --recreate-schema        Drop and recreate the specified schema.",
  "                           Schema will be deleted if it exists!",
  "  --database=name          An alias for --schema.",
  "  -mx, --mysqlx            Uses connection data to create Creating an X protocol session.",
  "  -mc, --mysql             Uses connection data to create a Classic Session.",
  "  -ma                      Uses the connection data to create the session with",
  "                           automatic protocol detection.",
  "  --sql                    Start in SQL mode.",
  "  --sqlc                   Start in SQL mode using a classic session.",
  "  --sqlx                   Start in SQL mode using Creating an X protocol session.",
  "  --js, --javascript       Start in JavaScript mode.",
  "  --py, --python           Start in Python mode.",
  "  --json[=format]          Produce output in JSON format, allowed values:",
  "                           raw, pretty. If no format is specified",
  "                           pretty format is produced.",
  "  --table                  Produce output in table format (default for",
  "                           interactive mode). This option can be used to",
  "                           force that format when running in batch mode.",
  "  -E, --vertical           Print the output of a query (rows) vertically.",
  "  -i, --interactive[=full] To use in batch mode, it forces emulation of",
  "                           interactive mode processing. Each line on the ",
  "                           batch is processed as if it were in ",
  "                           interactive mode.",
  "  --force                  To use in SQL batch mode, forces processing to",
  "                           continue if an error is found.",
  "  --log-level=value        The log level.",
  ngcommon::Logger::get_level_range_info(),
  "  -V, --version            Prints the version of MySQL Shell.",
  "  --ssl                    Deprecated, use --ssl-mode instead",
  "  --ssl-key=name           X509 key in PEM format.",
  "  --ssl-cert=name          X509 cert in PEM format.",
  "  --ssl-ca=name            CA file in PEM format.",
  "  --ssl-capath=dir         CA directory.",
  "  --ssl-cipher=name        SSL Cipher to use.",
  "  --ssl-crl=name           Certificate revocation list.",
  "  --ssl-crlpath=dir        Certificate revocation list path.",
  "  --ssl-mode=mode          SSL mode to use, allowed values: DISABLED,",
  "                           PREFERRED, REQUIRED, VERIFY_CA, VERIFY_IDENTITY.",
  "  --tls-version=version    TLS version to use, permitted values are :",
  "                           TLSv1, TLSv1.1.",
  "  --passwords-from-stdin   Read passwords from stdin instead of the tty.",
  "  --auth-method=method     Authentication method to use.",
  "  --show-warnings          Automatically display SQL warnings on SQL mode",
  "                           if available.",
  "  --dba enableXProtocol    Enable the X Protocol in the server connected to.",
  "                           Must be used with --mysql.",
  "  --nw, --no-wizard        Disables wizard mode.",
  "  --histignore=patterns    A colon-separated list of patterns to keep SQL",
  "                           input from getting logged into mysqlsh history."
  };
  return details;
}
