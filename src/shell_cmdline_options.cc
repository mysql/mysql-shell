/*
 * Copyright (c) 2014, 2015 Oracle and/or its affiliates. All rights reserved.
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
  char* log_level_value;
  bool needs_password = false;

  output_format = "";
  print_cmd_line_helper = false;
  print_version = false;

  session_type = mysh::Application;

  char default_json[7] = "pretty";
  char default_interactive[1] = "";
  char default_ssl[2] = "1";

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
  prompt_password = false;
  trace_protocol = false;

  port = 0;
  ssl = 0;
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
    else if (check_arg_with_value(argv, i, "--schema", "-D", value))
      schema = value;
    else if (check_arg_with_value(argv, i, "--database", NULL, value))
      schema = value;
    else if (check_arg(argv, i, "--recreate-schema", NULL))
      recreate_database = true;
    else if (check_arg(argv, i, "-p", "--password") || check_arg(argv, i, NULL, "--dbpassword"))
      prompt_password = true;
    else if (check_arg_with_value(argv, i, "--dbpassword", NULL, value, (char*)""))
    {
      password = value;
      if (password.empty())
        prompt_password = true;
    }
    else if (check_arg_with_value(argv, i, "--password", "-p", value, (char*)""))
    {
      password = value;
      if (password.empty() && !strchr(argv[i], '=')) // --password requests password, --password= means blank password
        prompt_password = true;
    }
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
    else if (check_arg_with_value(argv, i, "--ssl", NULL, value, default_ssl))
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
    else if (check_arg_with_value(argv, i, "--session-type", NULL, value))
    {
      if (strcmp(value, "classic") == 0)
        session_type = mysh::Classic;
      else if (strcmp(value, "node") == 0)
        session_type = mysh::Node;
      else if (strcmp(value, "app") == 0)
        session_type = mysh::Application;
      else
      {
        std::cerr << "Value for --session-type must be either app, node or classic.\n";
        exit_code = 1;
        break;
      }
    }
    else if (check_arg(argv, i, "--stx", "--stx"))
      session_type = mysh::Application;
    else if (check_arg(argv, i, "--stn", "--stn"))
      session_type = mysh::Node;
    else if (check_arg(argv, i, "--stc", "--stc"))
      session_type = mysh::Classic;
    else if (check_arg(argv, i, "--sql", "--sql"))
    {
      initial_mode = IShell_core::Mode_SQL;
      session_type = mysh::Node;
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
      session_type = mysh::Classic;
    }
    else if (check_arg_with_value(argv, i, "--json", NULL, value, default_json))
    {
      if (strcmp(value, "raw") != 0 && strcmp(value, "pretty") != 0)
      {
        std::cerr << "Value for --json must be either pretty or raw.\n";
        exit_code = 1;
        break;
      }

      std::string json(value);

      if (json == "pretty")
        output_format = "json";
      else
        output_format = "json/raw";
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
    else if (check_arg_with_value(argv, i, "--interactive", "-i", value, default_interactive))
    {
      if (strcmp(value, "") != 0 && strcmp(value, "full") != 0)
      {
        std::cerr << "Value for --interactive if any, must be full\n";
        exit_code = 1;
        break;
      }

      interactive = true;
      full_interactive = (strcmp(value, "full") == 0);
    }
    else if (check_arg(argv, i, NULL, "--passwords-from-stdin"))
      passwords_from_stdin = true;
    else if (check_arg_with_value(argv, i, "--log-level", NULL, log_level_value))
    {
      try
      {
        int nlog_level = boost::lexical_cast<int>(log_level_value);
        if (nlog_level < 1 || nlog_level > 8)
          throw 1;
        log_level = static_cast<ngcommon::Logger::LOG_LEVEL>(nlog_level);
      }
      catch (...)
      {
        std::cerr << "Value for --log-level must be an integer between 1 and 8.\n";
        exit_code = 1;
      }
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