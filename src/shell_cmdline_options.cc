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
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "modules/base_session.h"

using namespace shcore;

Shell_command_line_options::Shell_command_line_options(int argc, char **argv)
: Command_line_options(argc, argv), log_level(ngcommon::Logger::LOG_ERROR)
{
  std::string connection_string;
  std::string host;
  std::string user;
  std::string protocol;
  std::string database;
  
  char* log_level_value;
  bool needs_password = false;

  output_format = "";
  print_cmd_line_helper = false;
  print_version = false;

  session_type = mysh::Application;

  char default_json[7] = "pretty";
  char default_interactive[1] = "";
  char default_ssl[2] = "1";

  initial_mode = IShell_core::Mode_JScript;
  force = false;
  interactive = false;
  full_interactive = false;

  port = 0;
  ssl = 0;
  for (int i = 1; i < argc && exit_code == 0; i++)
  {
    char *value;
    if (check_arg_with_value(argv, i, "--file", "-f", value))
      run_file = value;
    else if (check_arg_with_value(argv, i, "--uri", NULL, value))
      connection_string = value;
    else if (check_arg_with_value(argv, i, "--host", "-h", value))
      host = value;
    else if (check_arg_with_value(argv, i, "--dbuser", "-u", value))
      user = value;
    else if (check_arg_with_value(argv, i, "--user", NULL, value))
      user = value;
    else if (check_arg_with_value(argv, i, "--port", "-P", value))
      port = atoi(value);
    else if (check_arg_with_value(argv, i, "--schema", "-D", value))
      database = value;
    else if (check_arg_with_value(argv, i, "--database", NULL, value))
      database = value;
    else if (check_arg(argv, i, "-p", "-p"))
      needs_password = true;
    else if (check_arg_with_value(argv, i, "--dbpassword", NULL, value))
      password = value;
    else if (check_arg_with_value(argv, i, "--password", NULL, value))
      password = value;
    else if (check_arg_with_value(argv, i, "--ssl-ca", NULL, value))
      ssl_ca = value;
    else if (check_arg_with_value(argv, i, "--ssl-cert", NULL, value))
      ssl_cert = value;
    else if (check_arg_with_value(argv, i, "--ssl-key", NULL, value))
      ssl_key = value;
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
    else if (check_arg(argv, i, "--sql", "--sql"))
      initial_mode = IShell_core::Mode_SQL;
    else if (check_arg(argv, i, "--js", "--js"))
      initial_mode = IShell_core::Mode_JScript;
    else if (check_arg(argv, i, "--py", "--py"))
      initial_mode = IShell_core::Mode_Python;
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
      std::cerr << argv[0] << ": unknown option " << argv[i] << "\n";
      exit_code = 1;
      break;
    }
  }

  // Configures the URI using all hte associated parameters
  configure_connection_string(connection_string, user, password, host, port, database, needs_password, ssl_ca, ssl_cert, ssl_key);
}

Shell_command_line_options::Shell_command_line_options(const Shell_command_line_options& other) :
Command_line_options(0, NULL)
{
  initial_mode = other.initial_mode;
  run_file = other.run_file;
  uri = other.uri;
  password = other.password;
  output_format = other.output_format;
  session_type = other.session_type;
  print_cmd_line_helper = other.print_cmd_line_helper;
  print_version = other.print_version;
  force = other.force;
  interactive = other.interactive;
  full_interactive = other.full_interactive;
  log_level = other.log_level;
  ssl_ca = other.ssl_ca;
  ssl_cert = other.ssl_cert;
  ssl_key = other.ssl_key;
  ssl = other.ssl;
  port = other.port;
}

// Takes the URI and the individual connection parameters and overrides
// On the URI as specified on the parameters
void Shell_command_line_options::configure_connection_string(const std::string &connstring,
                                   std::string &user, std::string &password,
                                   std::string &host, int &port,
                                   std::string &database, bool prompt_pwd, std::string &ssl_ca,
                                   std::string &ssl_cert, std::string &ssl_key)
{
  std::string uri_protocol;
  std::string uri_user;
  std::string uri_password;
  std::string uri_host;
  int uri_port = 0;
  std::string uri_sock;
  std::string uri_database;
  std::string uri_ssl_ca;
  std::string uri_ssl_cert;
  std::string uri_ssl_key;
  int pwd_found;

  bool conn_params_defined = false;

  // First validates the URI if specified
  if (!connstring.empty())
  {
    if (!mysh::parse_mysql_connstring(connstring, uri_protocol, uri_user, uri_password, uri_host, uri_port, uri_sock, uri_database, pwd_found, 
        uri_ssl_ca, uri_ssl_cert, uri_ssl_key))
    {
      std::cerr << "Invalid value specified in --uri parameter.\n";
      exit_code = 1;
      return;
    }
  }

  // URI was either empty or valid, in any case we need to override whatever was configured on the uri_* variables
  // With what was received on the individual parameters.
  if (!user.empty() || !password.empty() || !host.empty() || !database.empty() || port || !ssl_ca.empty() || !ssl_cert.empty() || !ssl_key.empty())
  {
    // This implies URI recreation process should be done to either
    // - Create an URI if none was specified.
    // - Update the URI with the parameters overriding it's values.
    conn_params_defined = true;

    if (!user.empty())
      uri_user = user;

    if (!password.empty())
      uri_password = password;

    if (!host.empty())
      uri_host = host;

    if (!database.empty())
      uri_database = database;

    if (port)
      uri_port = port;

    if (!ssl_ca.empty())
      uri_ssl_ca = ssl_ca;

    if (!ssl_cert.empty())
      uri_ssl_cert = ssl_cert;

    if (!ssl_key.empty())
      uri_ssl_key = ssl_key;
  }

  // If needed we construct the URi from the individual parameters
  if (conn_params_defined)
  {
    // Configures the URI string
    if (!uri_protocol.empty())
    {
      uri.append(uri_protocol);
      uri.append("://");
    }

    // Sets the user and password
    if (!uri_user.empty())
    {
      uri.append(uri_user);

      // If password needs to be prompted appends the : but not the password
      if (prompt_pwd)
        uri.append(":");

      // If the password will not be prompted and is defined appends both :password
      else if (!uri_password.empty())
      {
        uri.append(":").append(uri_password);
      }

      uri.append("@");
    }

    // Sets the host
    if (!uri_host.empty())
      uri.append(uri_host);

    // Sets the port
    if (!uri_host.empty() && port > 0)
      uri.append((boost::format(":%i") % port).str());

    // Sets the database
    if (!uri_database.empty())
    {
      uri.append("/");
      uri.append(uri_database);
    }

    conn_str_cat_ssl_data(uri, uri_ssl_ca, uri_ssl_cert, uri_ssl_key);
  }

  // Or we take the URI as defined since no overrides were done
  else
    uri = connstring;
}

void Shell_command_line_options::conn_str_cat_ssl_data(std::string& uri, const std::string& ssl_ca, const std::string& ssl_cert, const std::string& ssl_key)
{
  if (!uri.empty() && (!ssl_ca.empty() || !ssl_cert.empty() || !ssl_key.empty()))
  {
    bool first = false;
    int cnt = 0;
    if (!ssl_ca.empty()) ++cnt;
    if (!ssl_cert.empty()) ++cnt;
    if (!ssl_key.empty()) ++cnt;
    
    if (!ssl_ca.empty() && uri.rfind("ssl_ca=") == std::string::npos)
    {
      if (!first)
      {
        first = true;
        uri.append("?");
      }
      uri.append("ssl_ca=").append(ssl_ca);
      if (--cnt) uri.append("&");
    }
    if (!ssl_cert.empty() && uri.rfind("ssl_cert=") == std::string::npos)
    {
      if (!first)
      {
        first = true;
        uri.append("?");
      }
      uri.append("ssl_cert=").append(ssl_cert);
      if (--cnt) uri.append("&");
    }
    if (!ssl_key.empty() && uri.rfind("ssl_key=") == std::string::npos)
    {
      if (!first)
      {
        first = true;
        uri.append("?");
      }
      uri.append("ssl_key=").append(ssl_key);
    }
  }
}