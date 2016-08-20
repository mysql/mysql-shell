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

using namespace shcore;

Shell_command_line_options::Shell_command_line_options(int argc, char **argv)
  : Command_line_options(argc, argv), log_level(ngcommon::Logger::LOG_ERROR)
{
  output_format = "";
  print_cmd_line_helper = false;
  print_version = false;
  execute_statement = "";

  session_type = mysh::Application;
  default_session_type = true;

#ifdef HAVE_V8
  initial_mode = IShell_core::Mode_JScript;
#else
#ifdef HAVE_PYTHON
  initial_mode = IShell_core::Mode_Python;
#else
  initial_mode = IShell_core::Mode_SQL;
#endif
#endif

  recreate_database = false;
  force = false;
  interactive = false;
  full_interactive = false;
  passwords_from_stdin = false;
  password = NULL;
  prompt_password = false;
  trace_protocol = false;
  wizards = true;
  admin_mode = false;

  sock = "";
  port = 0;
  ssl = 0;
  int arg_format = 0;
  for (int i = 1; i < argc && exit_code == 0; i++)
  {
    char *value;
    if (check_arg_with_value(argv, i, "--file", "-f", value))
      run_file = value;
    else if (check_arg_with_value(argv, i, "--uri", NULL, value))
    {
      if (shcore::validate_uri(value))
        uri = value;
      else
      {
        std::cerr << "Invalid value specified in --uri parameter.\n";
        exit_code = 1;
        break;
      }
    }
    else if (check_arg_with_value(argv, i, "--app", NULL, value))
      app = value;
    else if (check_arg_with_value(argv, i, "--host", "-h", value))
      host = value;
    else if (check_arg_with_value(argv, i, "--dbuser", "-u", value))
      user = value;
    else if (check_arg_with_value(argv, i, "--user", NULL, value))
      user = value;
    else if (check_arg_with_value(argv, i, "--port", "-P", value))
      port = atoi(value);
    else if (check_arg_with_value(argv, i, "--socket", "-S", value))
      sock = value;
    else if (check_arg_with_value(argv, i, "--schema", "-D", value))
      schema = value;
    else if (check_arg_with_value(argv, i, "--database", NULL, value))
      schema = value;
    else if (check_arg(argv, i, "--recreate-schema", NULL))
      recreate_database = true;
    else if (check_arg_with_value(argv, i, "--execute", "-e", value))
      execute_statement = value;
    else if (check_arg_with_value(argv, i, "--dba", NULL, value))
      execute_dba_statement = value;
    else if ((arg_format = check_arg_with_value(argv, i, "--dbpassword", NULL, value, true)) ||
             (arg_format = check_arg_with_value(argv, i, "--password", "-p", value, true)))
    {
      // Note that in any connection attempt, password prompt will be done if the password is missing.
      // The behavior of the password cmd line argument is as follows:

      // ARGUMENT           EFFECT
      // --password         forces password prompt no matter it was already provided
      // --password value   forces password prompt no matter it was already provided (value is not taken as password)
      // --password=        sets password to empty (password is available but empty so it will not be prompted)
      // -p<value> sets the password to <value>
      // --password=<value> sets the password to <value>

      if (!value)
      {
        // --password=
        if (arg_format == 3)
          password = const_cast<char *>("");

        // --password
        else
          prompt_password = true;
      }
      // --password=value || --pvalue
      else if (arg_format != 1)
        password = value;

      // --password value (value is ignored)
      else
      {
        prompt_password = true;
        i--;
      }
    }
    else if (check_arg_with_value(argv, i, "--auth-method", NULL, value))
      auth_method = value;
    else if (check_arg_with_value(argv, i, "--ssl-ca", NULL, value))
    {
      ssl_ca = value;
      ssl = 1;
    }
    else if (check_arg_with_value(argv, i, "--ssl-cert", NULL, value))
    {
      ssl_cert = value;
      ssl = 1;
    }
    else if (check_arg_with_value(argv, i, "--ssl-key", NULL, value))
    {
      ssl_key = value;
      ssl = 1;
    }
    else if (check_arg_with_value(argv, i, "--ssl", NULL, value, true))
    {
      if (!value)
        ssl = 1;
      else
      {
        if (boost::iequals(value, "yes") || boost::iequals(value, "1"))
          ssl = 1;
        else if (boost::iequals(value, "no") || boost::iequals(value, "0"))
          ssl = 0;
        else
        {
          std::cerr << "Value for --ssl must be any of 1|0|yes|no";
          exit_code = 1;
          break;
        }
      }
    }
    else if (check_arg(argv, i, "--x", "--x"))
      override_session_type(mysh::Application, "--x");
    else if (check_arg(argv, i, "--node", "--node"))
      override_session_type(mysh::Node, "--node");
    else if (check_arg(argv, i, "--classic", "--classic"))
      override_session_type(mysh::Classic, "--x");
    else if (check_arg(argv, i, "--sql", "--sql"))
    {
      initial_mode = IShell_core::Mode_SQL;
      override_session_type(mysh::Node, "--sql");
    }
    else if (check_arg(argv, i, "--js", "--javascript"))
    {
#ifdef HAVE_V8
      initial_mode = IShell_core::Mode_JScript;
#else
      std::cerr << "JavaScript is not supported.\n";
      exit_code = 1;
      break;
#endif
    }
    else if (check_arg(argv, i, "--py", "--python"))
    {
#ifdef HAVE_PYTHON
      initial_mode = IShell_core::Mode_Python;
#else
      std::cerr << "Python is not supported.\n";
      exit_code = 1;
      break;
#endif
    }
    else if (check_arg(argv, i, NULL, "--sqlc"))
    {
      initial_mode = IShell_core::Mode_SQL;
      override_session_type(mysh::Classic, "--sqlc");
    }
    else if (check_arg_with_value(argv, i, "--json", NULL, value, true))
    {
      if (!value || strcmp(value, "pretty") == 0)
        output_format = "json";
      else if (strcmp(value, "raw") == 0)
        output_format = "json/raw";
      else
      {
        std::cerr << "Value for --json must be either pretty or raw.\n";
        exit_code = 1;
        break;
      }
    }
    else if (check_arg(argv, i, "--table", "--table"))
      output_format = "table";
    else if (check_arg(argv, i, "--trace-proto", NULL))
      trace_protocol = true;
    else if (check_arg(argv, i, "--help", "--help"))
    {
      print_cmd_line_helper = true;
      exit_code = 0;
    }
    else if (check_arg(argv, i, "--version", "--version"))
    {
      print_version = true;
      exit_code = 0;
    }
    else if (check_arg(argv, i, "--force", "--force"))
      force = true;
    else if (check_arg(argv, i, "--no-wizard", "--nw"))
      wizards = false;
    else if (check_arg_with_value(argv, i, "--interactive", "-i", value, true))
    {
      if (!value)
      {
        interactive = true;
        full_interactive = false;
      }
      else if (strcmp(value, "full") == 0)
      {
        interactive = true;
        full_interactive = true;
      }
      else
      {
        std::cerr << "Value for --interactive if any, must be full\n";
        exit_code = 1;
        break;
      }
    }
    else if (check_arg(argv, i, NULL, "--passwords-from-stdin"))
      passwords_from_stdin = true;
    else if (check_arg_with_value(argv, i, "--log-level", NULL, value))
    {
      ngcommon::Logger::LOG_LEVEL nlog_level;
      nlog_level = ngcommon::Logger::get_log_level(value);
      if (nlog_level == ngcommon::Logger::LOG_NONE && !ngcommon::Logger::is_level_none(value))
      {
        std::cerr << ngcommon::Logger::get_level_range_info() << std::endl;
        exit_code = 1;
        break;
      }
      else
        log_level = nlog_level;
    }
    else if (exit_code == 0)
    {
      if (argv[i][0] != '-')
        schema = argv[i];
      else
      {
        std::cerr << argv[0] << ": unknown option " << argv[i] << "\n";
        exit_code = 1;
        break;
      }
    }
  }
}

void Shell_command_line_options::override_session_type(mysh::SessionType new_type, const std::string& option, char* value)
{
  auto get_session_type = [](mysh::SessionType type)
  {
    std::string label;
    switch (type)
    {
      case mysh::Application:
        label = "X";
        break;
      case mysh::Node:
        label = "Node";
        break;
      case mysh::Classic:
        label = "Classic";
        break;
    }

    return label;
  };

  if (new_type != session_type)
  {
    if (!default_session_type)
    {
      std::string msg = "Overriding Session Type from ";
      msg.append(get_session_type(session_type));
      msg.append(" to ");
      msg.append(get_session_type(new_type));
      msg.append(" from option ");
      msg.append(option);

      if (value)
      {
        msg.append("=");
        msg.append(value);
      }

      std::cout << msg.c_str() << std::endl;
    }

    session_type = new_type;
  }

  default_session_type = false;
}

bool Shell_command_line_options::has_connection_data()
{
  return !app.empty() ||
         !uri.empty() ||
         !user.empty() ||
         !host.empty() ||
         !schema.empty() ||
         port != 0 ||
         password != NULL ||
         prompt_password ||
         ssl == 1 ||
         !ssl_ca.empty() ||
         !ssl_cert.empty() ||
         !ssl_key.empty();
}