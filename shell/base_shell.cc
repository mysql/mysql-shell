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

#include "shell/base_shell.h"
#include "modules/base_session.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "shellcore/shell_core_options.h" // <---
#include "shellcore/shell_notifications.h"
#include "modules/base_resultset.h"
#include "shell_resultset_dumper.h"
#include "utils/utils_time.h"
#include "utils/utils_help.h"
#include "logger/logger.h"

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

// TODO: This should be ported from the server, not used from there (see comment bellow)
//const int MAX_READLINE_BUF = 65536;
extern char *mysh_get_tty_password(const char *opt_message);

namespace mysqlsh {
Base_shell::Base_shell(const Shell_options &options, shcore::Interpreter_delegate *custom_delegate) :
_options(options) {
  std::string log_path = shcore::get_user_config_path();
  log_path += "mysqlsh.log";
  ngcommon::Logger::create_instance(log_path.c_str(), false, _options.log_level);
  _logger = ngcommon::Logger::singleton();

  _input_mode = shcore::Input_state::Ok;

  // Sets the global options
  shcore::Value::Map_type_ref shcore_options = shcore::Shell_core_options::get();

  // Updates shell core options that changed upon initialization
  (*shcore_options)[SHCORE_BATCH_CONTINUE_ON_ERROR] = shcore::Value(_options.force);
  (*shcore_options)[SHCORE_INTERACTIVE] = shcore::Value(_options.interactive);
  (*shcore_options)[SHCORE_USE_WIZARDS] = shcore::Value(_options.wizards);
  if (!_options.output_format.empty())
    (*shcore_options)[SHCORE_OUTPUT_FORMAT] = shcore::Value(_options.output_format);

  _shell.reset(new shcore::Shell_core(custom_delegate));

  std::string cmd_help_connect =
    "SYNTAX:\n"
    "   \\connect [-<TYPE>] <URI>\n\n"
    "   \\connect [-<TYPE >] $<SESSION_CFG_NAME>\n\n"
    "WHERE:\n"
    "   TYPE is an optional parameter to specify the session type. Accepts the next values:\n"
    "        x: to establish an X session\n"
    "        n: to establish an Node session\n"
    "        c: to establish a Classic session\n"
    "        If the session type is not specified, an X session will be established.\n"
    "   URI is in the format of: [user[:password]@]hostname[:port]\n"
    "   SESSION_CFG_NAME is the name of a stored session configuration\n\n"
    "EXAMPLES:\n"
    "   \\connect root@localhost\n"
    "   \\connect -n $my_cfg_name";

  std::string cmd_help_source =
    "SYNTAX:\n"
    "   \\source <sql_file_path>\n"
    "   \\. <sql_file_path>\n\n"
    "EXAMPLES:\n"
    "   \\source C:\\Users\\MySQL\\sakila.sql\n"
    "   \\. C:\\Users\\MySQL\\sakila.sql\n\n"
    "NOTE: Can execute files from the supported types: SQL, Javascript, or Python.\n"
    "Processing is done using the active language set for processing mode.\n";

  std::string cmd_help_use =
    "SYNTAX:\n"
    "   \\use <schema>\n"
    "   \\u <schema>\n\n"
    "EXAMPLES:\n"
    "   \\use mysql"
    "   \\u 'my schema'"
    "NOTE: This command works with the global session.\n"
    "If it is either a Node or Classic session, the current schema will be updated (affects SQL mode).\n"
    "The global db variable will be updated to hold the requested schema.\n";

  SET_SHELL_COMMAND("\\help|\\?|\\h", "Print this help.", "", Base_shell::cmd_print_shell_help);
  SET_CUSTOM_SHELL_COMMAND("\\sql", "Switch to SQL processing mode.", "", std::bind(&Base_shell::switch_shell_mode, this, shcore::Shell_core::Mode::SQL, _1));
  SET_CUSTOM_SHELL_COMMAND("\\js", "Switch to JavaScript processing mode.", "", std::bind(&Base_shell::switch_shell_mode, this, shcore::Shell_core::Mode::JScript, _1));
  SET_CUSTOM_SHELL_COMMAND("\\py", "Switch to Python processing mode.", "", std::bind(&Base_shell::switch_shell_mode, this, shcore::Shell_core::Mode::Python, _1));
  SET_SHELL_COMMAND("\\source|\\.", "Execute a script file. Takes a file name as an argument.", cmd_help_source, Base_shell::cmd_process_file);
  SET_SHELL_COMMAND("\\", "Start multi-line input when in SQL mode.", "", Base_shell::cmd_start_multiline);
  SET_SHELL_COMMAND("\\quit|\\q|\\exit", "Quit MySQL Shell.", "", Base_shell::cmd_quit);
  SET_SHELL_COMMAND("\\connect|\\c", "Connect to a server.", cmd_help_connect, Base_shell::cmd_connect);
  SET_SHELL_COMMAND("\\warnings|\\W", "Show warnings after every statement.", "", Base_shell::cmd_warnings);
  SET_SHELL_COMMAND("\\nowarnings|\\w", "Don't show warnings after every statement.", "", Base_shell::cmd_nowarnings);
  SET_SHELL_COMMAND("\\status|\\s", "Print information about the current global connection.", "", Base_shell::cmd_status);
  SET_SHELL_COMMAND("\\use|\\u", "Set the current schema for the global session.", cmd_help_use, Base_shell::cmd_use);

  const std::string cmd_help_store_connection =
    "SYNTAX:\n"
    "   \\savecon [-f] <SESSION_CONFIG_NAME> <URI>\n\n"
    "   \\savecon <SESSION_CONFIG_NAME>\n\n"
    "WHERE:\n"
    "   SESSION_CONFIG_NAME is the name to be assigned to the session configuration. Must be a valid identifier\n"
    "   -f is an optional flag, when specified the store operation will override the configuration associated to SESSION_CONFIG_NAME\n"
    "   URI Optional. the connection string following the URI convention. If not provided, will use the URI of the current session.\n\n"
    "EXAMPLES:\n"
    "   \\saveconn my_config_name root:123@localhost:33060\n";
  const std::string cmd_help_delete_connection =
    "SYNTAX:\n"
    "   \\rmconn <SESSION_CONFIG_NAME>\n\n"
    "WHERE:\n"
    "   SESSION_CONFIG_NAME is the name of session configuration to be deleted.\n\n"
    "EXAMPLES:\n"
    "   \\rmconn my_config_name\n";

  bool lang_initialized;
  _shell->switch_mode(_options.initial_mode, lang_initialized);

  _result_processor = std::bind(&Base_shell::process_result, this, _1);

  if (lang_initialized) {
    load_default_modules(_options.initial_mode);
    init_scripts(_options.initial_mode);
  }
}

bool Base_shell::cmd_process_file(const std::vector<std::string>& params) {
  std::string file;

  if (params[0].find("\\source") != std::string::npos)
    file = params[0].substr(8);
  else
    file = params[0].substr(3);

  boost::trim(file);

  std::vector<std::string> args(params);
  args[0] = file;
  Base_shell::process_file(file, args);

  return true;
}

void Base_shell::print_connection_message(mysqlsh::SessionType type, const std::string& uri, const std::string& sessionid) {
  std::string stype;

  switch (type) {
    case mysqlsh::SessionType::X:
      stype = "an X";
      break;
    case mysqlsh::SessionType::Node:
      stype = "a Node";
      break;
    case mysqlsh::SessionType::Classic:
      stype = "a Classic";
      break;
    case mysqlsh::SessionType::Auto:
      stype = "a";
      break;
  }

  std::string message;
  if (!sessionid.empty())
    message = "Using '" + sessionid + "' stored connection\n";

  message += "Creating " + stype + " Session to '" + uri + "'";

  println(message);
}

bool Base_shell::connect(bool primary_session) {
  try {
    shcore::Argument_list args;
    shcore::Value::Map_type_ref connection_data;
    bool secure_password = true;
    if (!_options.uri.empty()) {
      connection_data = shcore::get_connection_data(_options.uri);
      if (connection_data->has_key("dbPassword") && !connection_data->get_string("dbPassword").empty())
        secure_password = false;
    } else
      connection_data.reset(new shcore::Value::Map_type);

    // If the session is being created from command line
    // Individual parameters will override whatever was defined on the URI/stored connection
    if (primary_session) {
      if (_options.password)
        secure_password = false;

      shcore::update_connection_data(connection_data,
                                     _options.user, _options.password,
                                     _options.host, _options.port,
                                     _options.sock, _options.schema,
                                     _options.ssl != 0,
                                     _options.ssl_ca, _options.ssl_cert, _options.ssl_key,
                                     _options.auth_method);
      if (_options.auth_method == "PLAIN")
        println("mysqlx: [Warning] PLAIN authentication method is NOT secure!");

      if (!secure_password)
        println("mysqlx: [Warning] Using a password on the command line interface can be insecure.");
    }

    // If a scheme is given on the URI the session type must match the URI scheme
    if (connection_data->has_key("scheme")) {
      std::string scheme = connection_data->get_string("scheme");
      std::string error;

      if (_options.session_type == mysqlsh::SessionType::Auto) {
        if (scheme == "mysqlx")
          _options.session_type = mysqlsh::SessionType::Node;
        else if (scheme == "mysql")
          _options.session_type = mysqlsh::SessionType::Classic;
      } else {
        if (scheme == "mysqlx") {
          if (_options.session_type == mysqlsh::SessionType::Classic)
            error = "Invalid URI for Classic session";
        } else if (scheme == "mysql") {
          if (_options.session_type == mysqlsh::SessionType::X)
            error = "Invalid URI for X session";
          else if (_options.session_type == mysqlsh::SessionType::Node)
            error = "Invalid URI for Node session";
        }
      }

      if (!error.empty())
        throw shcore::Exception::argument_error(error);
    }

    shcore::set_default_connection_data(connection_data);

    if (_options.interactive)
      print_connection_message(_options.session_type, shcore::build_connection_string(connection_data, false), /*_options.app*/"");

    args.push_back(shcore::Value(connection_data));

    connect_session(args, _options.session_type, primary_session ? _options.recreate_database : false);
  } catch (shcore::Exception &exc) {
    _shell->print_value(shcore::Value(exc.error()), "error");
    return false;
  } catch (std::exception &exc) {
    print_error(exc.what());
    return false;
  }

  return true;
}

shcore::Value Base_shell::connect_session(const shcore::Argument_list &args, mysqlsh::SessionType session_type, bool recreate_schema) {
  std::string pass;
  std::string schema_name;

  shcore::Value::Map_type_ref connection_data = args.map_at(0);

  // Retrieves the schema on which the session will work on
  shcore::Argument_list schema_arg;
  if (connection_data->has_key("schema")) {
    schema_name = (*connection_data)["schema"].as_string();
    schema_arg.push_back(shcore::Value(schema_name));
  }

  if (recreate_schema && schema_name.empty())
      throw shcore::Exception::runtime_error("Recreate schema requested, but no schema specified");

  // Creates the argument list for the real connection call
  shcore::Argument_list connect_args;
  connect_args.push_back(shcore::Value(connection_data));

  // Prompts for the password if needed
  if (!connection_data->has_key("dbPassword") || _options.prompt_password) {
    if (_shell->password("Enter password: ", pass))
      connect_args.push_back(shcore::Value(pass));
  }

  std::shared_ptr<mysqlsh::ShellDevelopmentSession> old_session(_shell->get_dev_session()),
                                                   new_session(_shell->connect_dev_session(connect_args, session_type));

  new_session->set_option("trace_protocol", _options.trace_protocol);

  if (recreate_schema) {
    println("Recreating schema " + schema_name + "...");
    try {
      new_session->drop_schema(schema_arg);
    } catch (shcore::Exception &e) {
      if (e.is_mysql() && e.code() == 1008)
        ; // ignore DB doesn't exist error
      else
        throw;
    }
    new_session->create_schema(schema_arg);

    if (new_session->class_name().compare("XSession"))
      new_session->call("setCurrentSchema", schema_arg);
  }

  _shell->set_dev_session(new_session);

  if (_options.interactive) {
    if (old_session && old_session.unique() && old_session->is_connected()) {
      if (_options.interactive)
        println("Closing old connection...");

      old_session->close(shcore::Argument_list());
    }

    std::string session_type = new_session->class_name();
    std::string message;

    if (_options.session_type == mysqlsh::SessionType::Auto) {
      if (session_type == "ClassicSession")
        message = "Classic ";
      else if (session_type == "NodeSession")
        message = "Node ";
    }

    message += "Session successfully established. ";

    shcore::Value default_schema;

    if (!session_type.compare("XSession"))
       default_schema = new_session->get_member("defaultSchema");
    else
       default_schema = new_session->get_member("currentSchema");

    if (default_schema) {
      if (session_type == "ClassicSession")
        message += "Default schema set to `" + default_schema.as_object()->get_member("name").as_string() + "`.";
      else
        message += "Default schema `" + default_schema.as_object()->get_member("name").as_string() + "` accessible through db.";
    } else
      message += "No default schema selected.";

    println(message);
  }

  return shcore::Value::Null();
}

// load scripts for standard locations in order to be able to implement standard routines
void Base_shell::init_scripts(shcore::Shell_core::Mode mode) {
  std::string extension;

  if (mode == shcore::Shell_core::Mode::JScript)
    extension.append(".js");
  else if (mode == shcore::Shell_core::Mode::Python)
    extension.append(".py");
  else
    return;

  std::vector<std::string> scripts_paths;

  std::string user_file = "";

  try {
    // Checks existence of gobal startup script
    std::string path = shcore::get_global_config_path();
    path.append("shellrc");
    path.append(extension);
    if (shcore::file_exists(path))
      scripts_paths.push_back(path);

    // Checks existence of startup script at MYSQLSH_HOME
    // Or the binary location if not a standard installation
    path = shcore::get_mysqlx_home_path();
    if (!path.empty())
      path.append("/share/mysqlsh/mysqlshrc");
    else {
      path = shcore::get_binary_folder();
      path.append("/mysqlshrc");
    }
    path.append(extension);
    if (shcore::file_exists(path))
      scripts_paths.push_back(path);

    // Checks existence of user startup script
    path = shcore::get_user_config_path();
    path.append("mysqlshrc");
    path.append(extension);
    if (shcore::file_exists(path))
      scripts_paths.push_back(path);

    for (std::vector<std::string>::iterator i = scripts_paths.begin(); i != scripts_paths.end(); ++i) {
      process_file(*i, {*i});
    }
  } catch (std::exception &e) {
    std::string error(e.what());
    error += "\n";
    print_error(error);
  }
}

void Base_shell::load_default_modules(shcore::Shell_core::Mode mode) {
  // Module preloading only occurs on interactive mode
  if (_options.interactive) {
    if (mode == shcore::Shell_core::Mode::JScript) {
      process_line("var mysqlx = require('mysqlx');");
      process_line("var mysql = require('mysql');");
    } else if (mode == shcore::Shell_core::Mode::Python) {
      process_line("from mysqlsh import mysqlx");
      process_line("from mysqlsh import mysql");
    }
  }
}

std::string Base_shell::prompt() {
  std::string ret_val = _shell->prompt();

  // The continuation prompt should be used if state != Ok
  if (_input_mode != shcore::Input_state::Ok) {
    if (ret_val.length() > 4)
      ret_val = std::string(ret_val.length() - 4, ' ');
    else
      ret_val.clear();

    ret_val.append("... ");
  }

  return ret_val;
}

bool Base_shell::switch_shell_mode(shcore::Shell_core::Mode mode, const std::vector<std::string> &UNUSED(args)) {
  shcore::Shell_core::Mode old_mode = _shell->interactive_mode();
  bool lang_initialized = false;

  if (old_mode != mode) {
    _input_mode = shcore::Input_state::Ok;
    _input_buffer.clear();

    //XXX reset the history... history should be specific to each shell mode
    switch (mode) {
      case shcore::Shell_core::Mode::None:
        break;
      case shcore::Shell_core::Mode::SQL:
      {
        auto session = _shell->get_dev_session();
        if (session && (session->class_name() == "XSession")) {
          println("The active session is an " + session->class_name());
          println("SQL mode is not supported on this session type: command ignored.");
          println("To switch to SQL mode reconnect with a Node Session by either:");
          println("* Using the \\connect -n shell command.");
          println("* Using --node when calling the MySQL Shell on the command line.");
        } else {
          if (_shell->switch_mode(mode, lang_initialized))
            println("Switching to SQL mode... Commands end with ;");
        }
        break;
      }
      case shcore::Shell_core::Mode::JScript:
#ifdef HAVE_V8
        if (_shell->switch_mode(mode, lang_initialized))
          println("Switching to JavaScript mode...");
#else
        println("JavaScript mode is not supported, command ignored.");
#endif
        break;
      case shcore::Shell_core::Mode::Python:
#ifdef HAVE_PYTHON
        if (_shell->switch_mode(mode, lang_initialized))
          println("Switching to Python mode...");
#else
        println("Python mode is not supported, command ignored.");
#endif
        break;
    }

    // load scripts for standard locations
    if (lang_initialized) {
      load_default_modules(mode);
      init_scripts(mode);
    }
  }

  return true;
}

void Base_shell::println(const std::string &str) {
  _shell->println(str);
}

void Base_shell::print_error(const std::string &error) {
  _shell->print_error(error);
}

bool Base_shell::cmd_print_shell_help(const std::vector<std::string>& args) {
  bool printed = false;

  // If help came with parameter attempts to print the
  // specific help on the active shell first and global commands
  if (args.size() > 1) {
    printed = _shell->print_help(args[1]);

    if (!printed) {
      std::string help;
      if (_shell_command_handler.get_command_help(args[1], help)) {
        _shell->println(help);
        printed = true;
      }
    }
  }

  // If not specific help found, prints the generic help
  if (!printed) {
    _shell->print(_shell_command_handler.get_commands("===== Global Commands ====="));

    // Prints the active shell specific help
    _shell->print_help("");

    println("");
    println("For help on a specific command use the command as \\? <command>");
    println("");
    auto globals = _shell->get_global_objects();
    std::vector<std::pair<std::string, std::string> > global_names;

    if (globals.size()) {
      for (auto name : globals) {
        auto object_val = _shell->get_global(name);
        auto object = std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(object_val.as_object());
        global_names.push_back({name, object->class_name()});
      }
    }

    // Inserts the default modules
    global_names.push_back({"mysqlx", "mysqlx"});
    global_names.push_back({"mysql", "mysql"});

    std::sort(global_names.begin(), global_names.end());

    if (!global_names.empty()) {
      println("===== Global Objects =====");
      for (auto name : global_names) {
        auto brief = shcore::get_help_text(name.second + "_INTERACTIVE_BRIEF");
        if (brief.empty())
          brief = shcore::get_help_text(name.second + "_BRIEF");

        if (!brief.empty())
          println((boost::format("%-10s %s") % name.first % brief[0]).str());
      }
    }

    println();
    println("Please note that MySQL Document Store APIs are subject to change in future");
    println("releases.");
    println("");
    println("For more help on a global variable use <var>.help(), e.g. dba.help()");
    println("");
  }

  return true;
}

bool Base_shell::cmd_start_multiline(const std::vector<std::string>& args) {
  // This command is only available for SQL Mode
  if (args.size() == 1 && _shell->interactive_mode() == shcore::Shell_core::Mode::SQL) {
    _input_mode = shcore::Input_state::ContinuedBlock;

    return true;
  }

  return false;
}

bool Base_shell::cmd_connect(const std::vector<std::string>& args) {
  bool error = false;
  bool uri = false;
  _options.session_type = mysqlsh::SessionType::Auto;

  // Holds the argument index for the target to which the session will be established
  size_t target_index = 1;

  if (args.size() > 1 && args.size() < 4) {
    if (args.size() == 3) {
      std::string arg_2 = args[2];
      boost::trim(arg_2);

      if (!arg_2.empty()) {
        target_index++;
        uri = true;
      }
    }

    std::string arg = args[1];
    boost::trim(arg);

    if (arg.empty())
      error = true;
    else if (!arg.compare("-n") || !arg.compare("-N"))
      _options.session_type = mysqlsh::SessionType::Node;
    else if (!arg.compare("-c") || !arg.compare("-C"))
      _options.session_type = mysqlsh::SessionType::Classic;
    else {
      if (args.size() == 3)
        error = true;
      else
        uri = true;
    }

    if (!error) {
      if (uri) {
        /*if (args[target_index].find("$") == 0)
          _options.app = args[target_index].substr(1);
          else {
          _options.app = "";*/
        _options.uri = args[target_index];
        //}
      }
      connect();

      if (_shell->interactive_mode() == shcore::IShell_core::Mode::SQL && _options.session_type == mysqlsh::SessionType::X)
        println("WARNING: An X Session has been established and SQL execution is not allowed.");
    }
  } else
    error = true;

  if (error)
    print_error("\\connect [-<type>] <uri or $name>\n");

  return true;
}

bool Base_shell::cmd_quit(const std::vector<std::string>& UNUSED(args)) {
  _options.interactive = false;

  return true;
}

bool Base_shell::cmd_warnings(const std::vector<std::string>& UNUSED(args)) {
  (*shcore::Shell_core_options::get())[SHCORE_SHOW_WARNINGS] = shcore::Value::True();

  println("Show warnings enabled.");

  return true;
}

bool Base_shell::cmd_nowarnings(const std::vector<std::string>& UNUSED(args)) {
  (*shcore::Shell_core_options::get())[SHCORE_SHOW_WARNINGS] = shcore::Value::False();

  println("Show warnings disabled.");

  return true;
}

bool Base_shell::cmd_status(const std::vector<std::string>& UNUSED(args)) {
  std::string version_msg("MySQL Shell Version ");
  version_msg += MYSH_VERSION;
  version_msg += "\n";
  println(version_msg);

  if (_shell->get_dev_session() && _shell->get_dev_session()->is_connected()) {
    shcore::Value raw_status = _shell->get_dev_session()->get_status(shcore::Argument_list());
    std::string output_format = (*shcore::Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string();

    if (output_format.find("json") == 0)
      println(raw_status.json(output_format == "json"));
    else {
      shcore::Value::Map_type_ref status = raw_status.as_map();

      std::string format = "%-30s%s";

      if (status->has_key("STATUS_ERROR"))
        println((boost::format(format) % "Error Retrieving Status: " % (*status)["STATUS_ERROR"].descr(true)).str());
      else {
        if (status->has_key("SESSION_TYPE"))
          println((boost::format(format) % "Session type: " % (*status)["SESSION_TYPE"].descr(true)).str());

        if (status->has_key("NODE_TYPE"))
          println((boost::format(format) % "Server type: " % (*status)["NODE_TYPE"].descr(true)).str());

        if (status->has_key("CONNECTION_ID"))
          println((boost::format(format) % "Connection Id: " % (*status)["CONNECTION_ID"].descr(true)).str());

        if (status->has_key("DEFAULT_SCHEMA"))
          println((boost::format(format) % "Default schema: " % (*status)["DEFAULT_SCHEMA"].descr(true)).str());

        if (status->has_key("CURRENT_SCHEMA"))
          println((boost::format(format) % "Current schema: " % (*status)["CURRENT_SCHEMA"].descr(true)).str());

        if (status->has_key("CURRENT_USER"))
          println((boost::format(format) % "Current user: " % (*status)["CURRENT_USER"].descr(true)).str());

        if (status->has_key("SSL_CIPHER"))
          println((boost::format(format) % "SSL: Cipher in use: " % (*status)["SSL_CIPHER"].descr(true)).str());
        else
          println((boost::format(format) % "SSL:" % "Not in use.").str());

        if (status->has_key("SERVER_VERSION"))
          println((boost::format(format) % "Server version: " % (*status)["SERVER_VERSION"].descr(true)).str());

        if (status->has_key("SERVER_INFO"))
          println((boost::format(format) % "Server info: " % (*status)["SERVER_INFO"].descr(true)).str());

        if (status->has_key("PROTOCOL_VERSION"))
          println((boost::format(format) % "Protocol version: " % (*status)["PROTOCOL_VERSION"].descr(true)).str());

        if (status->has_key("CONNECTION"))
          println((boost::format(format) % "Connection: " % (*status)["CONNECTION"].descr(true)).str());

        if (status->has_key("SERVER_CHARSET"))
          println((boost::format(format) % "Server characterset: " % (*status)["SERVER_CHARSET"].descr(true)).str());

        if (status->has_key("SCHEMA_CHARSET"))
          println((boost::format(format) % "Schema characterset: " % (*status)["SCHEMA_CHARSET"].descr(true)).str());

        if (status->has_key("CLIENT_CHARSET"))
          println((boost::format(format) % "Client characterset: " % (*status)["CLIENT_CHARSET"].descr(true)).str());

        if (status->has_key("CONNECTION_CHARSET"))
          println((boost::format(format) % "Conn. characterset: " % (*status)["CONNECTION_CHARSET"].descr(true)).str());

        if (status->has_key("SERVER_STATS")) {
          std::string stats = (*status)["SERVER_STATS"].descr(true);
          size_t start = stats.find(" ");
          start++;
          size_t end = stats.find(" ", start);

          std::string time = stats.substr(start, end - start);
          unsigned long ltime = boost::lexical_cast<unsigned long>(time);
          std::string str_time = MySQL_timer::format_legacy(ltime, false, true);

          println((boost::format(format) % "Up time: " % str_time).str());
          println("");
          println(stats.substr(end + 2));
        }
      }
    }
  } else
    print_error("Not Connected.\n");

  return true;
}

bool Base_shell::cmd_use(const std::vector<std::string>& args) {
  std::string error;
  if (_shell->get_dev_session() && _shell->get_dev_session()->is_connected()) {
    std::string real_param;

    // If quoted, takes as param what's inside of the quotes
    auto start = args[0].find_first_of("\"'`");
    if (start != std::string::npos) {
      std::string quote = args[0].substr(start, 1);

      if (args[0].size() >= start) {
        auto end = args[0].find(quote, start + 1);

        if (end != std::string::npos)
          real_param = args[0].substr(start + 1, end - start - 1);
        else
          error = "Mistmatched quote on command parameter: " + args[0].substr(start) + "\n";
      }
    } else if (args.size() == 2)
      real_param = args[1];
    else
      error = "\\use <schema_name>\n";

    if (error.empty()) {
      try {
        shcore::Value schema = _shell->set_current_schema(real_param);
        auto session = _shell->get_dev_session();

        if (session) {
          auto session_type = session->class_name();
          std::string message = "Schema `" + schema.as_object()->get_member("name").as_string() + "` accessible through db.";

          if (session_type == "ClassicSession")
            message = "Schema set to `" + schema.as_object()->get_member("name").as_string() + "`.";
          else
            message = "Schema `" + schema.as_object()->get_member("name").as_string() + "` accessible through db.";

          println(message);
        }
      } catch (shcore::Exception &e) {
        error = e.format();
      }
    }
  } else
    error = "Not Connected.\n";

  if (!error.empty())
    print_error(error);

  return true;
}

bool Base_shell::do_shell_command(const std::string &line) {
  // Verifies if the command can be handled by the active shell
  bool handled = _shell->handle_shell_command(line);

  // Global Command Processing (xShell specific)
  if (!handled)
    handled = _shell_command_handler.process(line);

  return handled;
}

void Base_shell::process_line(const std::string &line) {
  bool handled_as_command = false;
  std::string to_history;

  // check if the line is an escape/shell command
  if (_input_buffer.empty() && !line.empty() && _input_mode == shcore::Input_state::Ok) {
    try {
      handled_as_command = do_shell_command(line);
    } catch (std::exception &exc) {
      std::string error(exc.what());
      error += "\n";
      print_error(error);
    }
  }

  if (handled_as_command)
    to_history = line;
  else {
    if (_input_mode == shcore::Input_state::ContinuedBlock && line.empty())
      _input_mode = shcore::Input_state::Ok;

    // Appends the line, no matter it is an empty line
    _input_buffer.append(_shell->preprocess_input_line(line));

    // Appends the new line if anything has been added to the buffer
    if (!_input_buffer.empty())
      _input_buffer.append("\n");

    if (_input_mode != shcore::Input_state::ContinuedBlock && !_input_buffer.empty()) {
      try {
        _shell->handle_input(_input_buffer, _input_mode, _result_processor);

        // Here we analyze the input mode as it was let after executing the code
        if (_input_mode == shcore::Input_state::Ok) {
          to_history = _shell->get_handled_input();
        }
      } catch (shcore::Exception &exc) {
        _shell->print_value(shcore::Value(exc.error()), "error");
        to_history = _input_buffer;
      } catch (std::exception &exc) {
        std::string error(exc.what());
        error += "\n";
        print_error(error);
        to_history = _input_buffer;
      }

      // TODO: Do we need this cleanup? i.e. in case of exceptions above??
      // Clears the buffer if OK, if continued, buffer will contain
      // the non executed code
      if (_input_mode == shcore::Input_state::Ok)
        _input_buffer.clear();
    }
  }

  if (!to_history.empty()) {
    shcore::Value::Map_type_ref data(new shcore::Value::Map_type());
    (*data)["statement"] = shcore::Value(to_history);
    shcore::ShellNotifications::get()->notify("SN_STATEMENT_EXECUTED", nullptr, data);
  }

  _shell->reconnect_if_needed();
}

void Base_shell::abort() {
  if (!_shell) return;

  if (_shell->is_running_query()) {
    try {
      _shell->abort();
    } catch (std::runtime_error& e) {
      log_exception("Error when killing connection ", e);
    }
  }
}

void Base_shell::process_result(shcore::Value result) {
  if ((*shcore::Shell_core_options::get())[SHCORE_INTERACTIVE].as_bool()
      || _shell->interactive_mode() == shcore::Shell_core::Mode::SQL) {
    if (result) {
      shcore::Value shell_hook;
      std::shared_ptr<shcore::Object_bridge> object;
      if (result.type == shcore::Object) {
        object = result.as_object();
        if (object && object->has_member("__shell_hook__"))
          shell_hook = object->get_member("__shell_hook__");

        if (shell_hook) {
          shcore::Argument_list args;
          shcore::Value hook_result = object->call("__shell_hook__", args);

          // Recursive call to continue processing shell hooks if any
          process_result(hook_result);
        }
      }

      // If the function is not found the values still needs to be printed
      if (!shell_hook) {
        // Resultset objects get printed
        if (object && object->class_name().find("Result") != std::string::npos) {
          std::shared_ptr<mysqlsh::ShellBaseResult> resultset = std::static_pointer_cast<mysqlsh::ShellBaseResult> (object);

          // Result buffering will be done ONLY if on any of the scripting interfaces
          ResultsetDumper dumper(resultset, _shell->get_delegate(), _shell->interactive_mode() != shcore::IShell_core::Mode::SQL);
          dumper.dump();
        } else {
          // In JSON mode: the json representation is used for Object, Array and Map
          // For anything else a map is printed with the "value" key
          std::string tag;
          if (result.type != shcore::Object && result.type != shcore::Array && result.type != shcore::Map)
            tag = "value";

          _shell->print_value(result, tag);
        }
      }
    }
  }

  // Return value of undefined implies an error processing
  if (result.type == shcore::Undefined)
  _shell->set_error_processing();
}

int Base_shell::process_file(const std::string& file, const std::vector<std::string> &argv) {
  // Default return value will be 1 indicating there were errors
  int ret_val = 1;

  if (file.empty())
    print_error("Usage: \\. <filename> | \\source <filename>\n");
  else if (_shell->is_module(file))
    _shell->execute_module(file, _result_processor);
  else
    //TODO: do path expansion (in case ~ is used in linux)
  {
    std::ifstream s(file.c_str());

    if (!s.fail()) {
      // The return value now depends on the stream processing
      ret_val = process_stream(s, file, argv);

      // When force is used, we do not care of the processing
      // errors
      if (_options.force)
        ret_val = 0;

      s.close();
    } else {
      // TODO: add a log entry once logging is
      print_error((boost::format("Failed to open file '%s', error: %d\n") % file % errno).str());
    }
  }

  return ret_val;
}

int Base_shell::process_stream(std::istream & stream, const std::string& source,
    const std::vector<std::string> &argv) {
  // If interactive is set, it means that the shell was started with the option to
  // Emulate interactive mode while processing the stream
  if (_options.interactive) {
    if (_options.full_interactive)
      _shell->print(prompt());

    bool comment_first_js_line = _shell->interactive_mode() == shcore::IShell_core::Mode::JScript;
    while (!stream.eof()) {
      std::string line;

      std::getline(stream, line);

      // When processing JavaScript files, validates the very first line to start with #!
      // If that's the case, it is replaced by a comment indicator //
      if (comment_first_js_line && line.size() > 1 && line[0] == '#' && line[1] == '!')
        line.replace(0, 2, "//");

      comment_first_js_line = false;

      if (_options.full_interactive)
        println(line);

      process_line(line);

      if (_options.full_interactive)
        _shell->print(prompt());
    }

    // Being interactive, we do not care about the return value
    return 0;
  } else {
    return _shell->process_stream(stream, source, _result_processor, argv);
  }
}
}
