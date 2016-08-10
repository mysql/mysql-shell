/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "mod_mysqlx_mysql_instance.h"

#include "utils/utils_general.h"
#include "common/process_launcher/process_launcher.h"
#include "shellcore/shell_core_options.h"

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "modules/mysqlxtest_utils.h"
#include "shellcore/interactive_object_wrapper.h"

#include <boost/lexical_cast.hpp>

#include <iostream>

using namespace std::placeholders;
using namespace mysh::mysqlx;

REGISTER_MODULE(MysqlInstance, mysql_instance)
{
  REGISTER_VARARGS_FUNCTION(MysqlInstance, validate_instance, validateInstance);
  REGISTER_VARARGS_FUNCTION(MysqlInstance, deploy_local_instance, deployLocalInstance);

  // Set the python environmental variable and the gadgets path

  std::string current_python_path = "";
  std::string gadgets_path;

  if (getenv("MYSQLPROVISION") != NULL)
    gadgets_path = std::string(getenv("MYSQLPROVISION")); // should be set to the mysqlprovision root dir

  if (!gadgets_path.empty())
  {
    if (getenv("PYTHONPATH") != NULL)
      current_python_path = std::string(getenv("PYTHONPATH"));

    std::string python_path = current_python_path + ":" + gadgets_path + "/gadgets/python";

#ifdef WIN32
    _putenv_s("PYTHONPATH", python_path.c_str());
#else
    setenv("PYTHONPATH", python_path.c_str(), true);
#endif
  }
  // Dont complain, just validate later and disable functions... or warn the user when attempts to use them
  //throw shcore::Exception::logic_error("Please set the mysqlprovision path using the environmental variable: MYSQLPROVISION.");
}

DEFINE_FUNCTION(MysqlInstance, validate_instance)
{
  args.ensure_count(1, "validateInstance");

  shcore::Value ret_val;

  std::string uri;
  shcore::Value::Map_type_ref options; // Map with the connection data

  std::string protocol;
  std::string user = "root"; // TODO: get the user from the session
  std::string host;
  int port = 0;
  std::string sock;
  std::string schema;
  std::string ssl_ca;
  std::string ssl_cert;
  std::string ssl_key;

  std::vector<std::string> valid_options = { "host", "port", "socket", "ssl_ca", "ssl_cert", "ssl_key", "ssl_key" };

  try
  {
    // Identify the type of connection data (String or Document)
    if (args[0].type == shcore::String)
    {
      uri = args.string_at(0);
      options = shcore::get_connection_data(uri, false);
    }

    // Connection data comes in a dictionary
    else if (args[0].type == shcore::Map)
      options = args.map_at(0);

    else
      throw shcore::Exception::argument_error("Unexpected argument on connection data.");

    if (options->size() == 0)
      throw shcore::Exception::argument_error("Connection data empty.");

    for (shcore::Value::Map_type::iterator i = options->begin(); i != options->end(); ++i)
    {
      if ((std::find(valid_options.begin(), valid_options.end(), i->first) == valid_options.end()))
        throw shcore::Exception::argument_error("Unexpected argument " + i->first + " on connection data.");
    }

    if (options->has_key("host"))
      host = (*options)["host"].as_string();

    if (options->has_key("port"))
      port = (*options)["port"].as_int();

    if (options->has_key("socket"))
      sock = (*options)["socket"].as_string();

    if (options->has_key("ssl_ca"))
      ssl_ca = (*options)["ssl_ca"].as_string();

    if (options->has_key("ssl_cert"))
      ssl_cert = (*options)["ssl_cert"].as_string();

    if (options->has_key("ssl_key"))
      ssl_key = (*options)["ssl_key"].as_string();

    if (port == 0 && sock.empty())
      port = get_default_instance_port();

    // TODO: validate additional data.

    std::string sock_port = (port == 0) ? sock : boost::lexical_cast<std::string>(port);

    // Handle empty required values
    if (!options->has_key("host"))
      throw shcore::Exception::argument_error("Missing required value for hostname.");

    std::string gadgets_path = (*shcore::Shell_core_options::get())[SHCORE_GADGETS_PATH].as_string();

    std::string instance_cmd = "--instance=" + user + "@" + host + ":" + std::to_string(port);
    const char *args_script[] = { "python", gadgets_path.c_str(), "check", instance_cmd.c_str(), "--stdin", NULL };

    ngcommon::Process_launcher p("python", args_script);

    std::string buf;
    char c;
    std::string success("Operation completed with success.");
    std::string password = "root\n";
    std::string error;

#ifdef WIN32
    success += "\r\n";
#else
    success += "\n";
#endif

    try
    {
      p.write(password.c_str(), password.length());
    }
    catch (shcore::Exception &e)
    {
      throw shcore::Exception::runtime_error(e.what());
    }

    bool read_error = false;

    while (p.read(&c, 1) > 0)
    {
      buf += c;
      if (c == '\n')
      {
        std::cout << "buf: " << buf << "\n";
        if ((buf.find("ERROR") != std::string::npos))
          read_error = true;

        if (read_error)
          error += buf;

        if (strcmp(success.c_str(), buf.c_str()) == 0)
        {
          std::string s_out = "The instance: " + host + ":" + std::to_string(port) + " is valid for Farm usage";
          ret_val = shcore::Value(s_out);
          break;
        }
        buf = "";
      }
    }

    if (read_error)
      throw shcore::Exception::logic_error(error);

    p.wait();
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION("validateInstance");

  return ret_val;
}

DEFINE_FUNCTION(MysqlInstance, deploy_local_instance)
{
  args.ensure_count(1, "deployLocalInstance");

  shcore::Value ret_val;

  shcore::Value::Map_type_ref options; // Map with the connection data

  int port = 0;
  int portx = 0;
  std::string sandbox_dir;

  std::vector<std::string> valid_options = { "port", "portx", "sandboxDir" };

  try
  {
    options = args.map_at(0);

    if (options->size() == 0)
      throw shcore::Exception::argument_error("Options empty.");

    for (shcore::Value::Map_type::iterator i = options->begin(); i != options->end(); ++i)
    {
      if ((std::find(valid_options.begin(), valid_options.end(), i->first) == valid_options.end()))
        throw shcore::Exception::argument_error("Unexpected argument " + i->first + " on connection data.");
    }

    if (options->has_key("port"))
      port = (*options)["port"].as_int();

    if (options->has_key("portx"))
      portx = (*options)["portx"].as_int();

    if (options->has_key("sandboxDir"))
      sandbox_dir = (*options)["sandboxDir"].as_string();

    std::string gadgets_path = (*shcore::Shell_core_options::get())[SHCORE_GADGETS_PATH].as_string();

    std::vector<std::string> sandbox_args;
    std::string arg;

    if (port != 0)
    {
      arg = "--port=" + std::to_string(port);
      sandbox_args.push_back(arg);
    }

    if (portx != 0)
    {
      arg = "--mysqlx-port=" + std::to_string(portx);
      sandbox_args.push_back(arg);
    }

    if (!sandbox_dir.empty())
    {
      arg = "--sandboxdir=" + sandbox_dir;
      sandbox_args.push_back(arg);
    }

    char **args_script = new char*[10];
    args_script[0] = const_cast<char*>("python");
    args_script[1] = const_cast<char*>(gadgets_path.c_str());
    args_script[2] = const_cast<char*>("sandbox");
    args_script[3] = const_cast<char*>("start");

    int i;

    for (i = 0; i < sandbox_args.size(); i++)
      args_script[i + 4] = const_cast<char*>(sandbox_args[i].c_str());

    args_script[i++ + 4] = const_cast<char*>("--stdin");
    args_script[i++ + 4] = NULL;

    ngcommon::Process_launcher p("python", const_cast<const char**>(args_script));

    std::string buf, passwd, error, answer;
    char c;
    std::string success("Operation completed with success.");

    // TODO: We're not a derived class of Interactive_object_wrapper, how to query for the password?
    /*
    if (password("Please enter a password for the root user of the MySQL Sandbox (root@localhost): ", answer))
    passwd = answer;

    if (passwd.empty())
    throw shcore::Exception::argument_error("The password cannot be empty.");
    */

    passwd = "root\n"; // TODO: see above

#ifdef WIN32
    success += "\r\n";
#else
    success += "\n";
#endif

    try
    {
      p.write(passwd.c_str(), passwd.length());
    }
    catch (shcore::Exception &e)
    {
      throw shcore::Exception::runtime_error(e.what());
    }

    bool read_error = false;

    while (p.read(&c, 1) > 0)
    {
      buf += c;
      if (c == '\n')
      {
        if ((buf.find("ERROR") != std::string::npos))
          read_error = true;

        if (read_error)
          error += buf;

        if (strcmp(success.c_str(), buf.c_str()) == 0)
        {
          std::string s_out = "Instance: localhost:" + std::to_string(port) + " successfully deployed.";
          ret_val = shcore::Value(s_out);
          break;
        }
        buf = "";
      }
    }

    if (read_error)
      throw shcore::Exception::logic_error(error);

    p.wait();
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION("deployLocalInstance");

  return ret_val;
}