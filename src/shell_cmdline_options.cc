/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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
#include "shellcore/ishell_core.h"
#include "shell_cmdline_options.h"
#include "utils/utils_general.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

Shell_command_line_options::Shell_command_line_options(int argc, char **argv)
  : Command_line_options(argc, argv) {
  int arg_format = 0;
  for (int i = 1; i < argc && exit_code == 0; i++) {
    char *value;
    if (check_arg_with_value(argv, i, "--file", "-f", value))
      _options.run_file = value;
    else if ((arg_format = check_arg_with_value(argv, i, "--uri", NULL, value))) {
      if (shcore::validate_uri(value)) {
        _options.uri = value;

        shcore::Value::Map_type_ref data = shcore::get_connection_data(value, false);

        if (data->has_key("dbPassword")) {
          std::string pwd(data->get_string("dbPassword").length(), '*');
          (*data)["dbPassword"] = shcore::Value(pwd);

          // Required replacement when --uri <value>
          auto nopwd_uri = shcore::build_connection_string(data, true);

          // Required replacement when --uri=<value>
          if (arg_format == 3)
            nopwd_uri = "--uri=" + nopwd_uri;

           strcpy(argv[i], nopwd_uri.substr(0, _options.uri.length()).c_str());
        }
      } else {
        std::cerr << "Invalid value specified in --uri parameter.\n";
        exit_code = 1;
        break;
      }
    } else if (check_arg_with_value(argv, i, "--app", NULL, value))
      _options.app = value;
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

        strcpy(argv[i], pwd.c_str());
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

        strcpy(argv[i], pwd.c_str());
      }

      // --password value (value is ignored)
      else {
        _options.prompt_password = true;
        i--;
      }
    } else if (check_arg_with_value(argv, i, "--auth-method", NULL, value))
      _options.auth_method = value;
    else if (check_arg_with_value(argv, i, "--ssl-ca", NULL, value)) {
      _options.ssl_ca = value;
      _options.ssl = 1;
    } else if (check_arg_with_value(argv, i, "--ssl-cert", NULL, value)) {
      _options.ssl_cert = value;
      _options.ssl = 1;
    } else if (check_arg_with_value(argv, i, "--ssl-key", NULL, value)) {
      _options.ssl_key = value;
      _options.ssl = 1;
    } else if (check_arg_with_value(argv, i, "--ssl", NULL, value, true)) {
      if (!value)
        _options.ssl = 1;
      else {
        if (boost::iequals(value, "yes") || boost::iequals(value, "1"))
          _options.ssl = 1;
        else if (boost::iequals(value, "no") || boost::iequals(value, "0"))
          _options.ssl = 0;
        else {
          std::cerr << "Value for --ssl must be any of 1|0|yes|no";
          exit_code = 1;
          break;
        }
      }
    } else if (check_arg(argv, i, "--x", "--x"))
      override_session_type(mysh::SessionType::X, "--x");
    else if (check_arg(argv, i, "--node", "--node"))
      override_session_type(mysh::SessionType::Node, "--node");
    else if (check_arg(argv, i, "--classic", "--classic"))
      override_session_type(mysh::SessionType::Classic, "--classic");
    else if (check_arg(argv, i, "--sql", "--sql")) {
      _options.initial_mode = shcore::IShell_core::Mode::SQL;
    } else if (check_arg(argv, i, "--js", "--javascript")) {
#ifdef HAVE_V8
      _options.initial_mode = shcore::IShell_core::Mode::JScript;
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
      override_session_type(mysh::SessionType::Classic, "--sqlc");
    } else if (check_arg(argv, i, NULL, "--sqln")) {
      _options.initial_mode = shcore::IShell_core::Mode::SQL;
      override_session_type(mysh::SessionType::Node, "--sqln");
    } else if (check_arg_with_value(argv, i, "--json", NULL, value, true)) {
      if (!value || strcmp(value, "pretty") == 0)
        _options.output_format = "json";
      else if (strcmp(value, "raw") == 0)
        _options.output_format = "json/raw";
      else {
        std::cerr << "Value for --json must be either pretty or raw.\n";
        exit_code = 1;
        break;
      }
    } else if (check_arg(argv, i, "--table", "--table"))
      _options.output_format = "table";
    else if (check_arg(argv, i, "--trace-proto", NULL))
      _options.trace_protocol = true;
    else if (check_arg(argv, i, "--help", "--help")) {
      _options.print_cmd_line_helper = true;
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
      nlog_level = ngcommon::Logger::get_log_level(value);
      if (nlog_level == ngcommon::Logger::LOG_NONE && !ngcommon::Logger::is_level_none(value)) {
        std::cerr << ngcommon::Logger::get_level_range_info() << std::endl;
        exit_code = 1;
        break;
      } else
        _options.log_level = nlog_level;
    } else if (exit_code == 0) {
      if (argv[i][0] != '-')
        _options.schema = argv[i];
      else {
        std::cerr << argv[0] << ": unknown option " << argv[i] << "\n";
        exit_code = 1;
        break;
      }
    }
  }

  _options.exit_code = exit_code;
}

void Shell_command_line_options::override_session_type(mysh::SessionType new_type, const std::string& option, char* value) {
  auto get_session_type = [](mysh::SessionType type) {
    std::string label;
    switch (type) {
      case mysh::SessionType::X:
        label = "X";
        break;
      case mysh::SessionType::Node:
        label = "Node";
        break;
      case mysh::SessionType::Classic:
        label = "Classic";
        break;
      case mysh::SessionType::Auto:
        break;
    }

    return label;
  };

  if (new_type != _options.session_type) {
    if (!_options.default_session_type) {
      std::string msg = "Session type already configured to ";
      msg.append(get_session_type(_options.session_type));
      msg.append(", unable to change to ");
      msg.append(get_session_type(new_type));
      msg.append(" with option ");
      msg.append(option);

      if (value) {
        msg.append("=");
        msg.append(value);
      }

      std::cerr << msg << std::endl;
      exit_code = 1;
    }

    _options.session_type = new_type;
  }

  _options.default_session_type = false;
}
