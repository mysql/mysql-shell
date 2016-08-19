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

#include "interactive_shell.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "shellcore/shell_core_options.h" // <---
#include "shellcore/shell_registry.h"
#include "modules/base_resultset.h"
#include "shell_resultset_dumper.h"
#include "utils/utils_time.h"
#include "logger/logger.h"

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

// TODO: This should be ported from the server, not used from there (see comment bellow)
//const int MAX_READLINE_BUF = 65536;
extern char *mysh_get_tty_password(const char *opt_message);

Interactive_shell::Interactive_shell(const Shell_command_line_options &options, Interpreter_delegate *custom_delegate) :
_options(options)
{
  std::string log_path = get_user_config_path();
  log_path += "mysqlsh.log";
  ngcommon::Logger::create_instance(log_path.c_str(), false, _options.log_level);
  _logger = ngcommon::Logger::singleton();

#ifndef WIN32
  rl_initialize();
#endif
  //  using_history();

  _input_mode = Input_ok;

  if (custom_delegate)
  {
    _delegate.user_data = custom_delegate->user_data;
    _delegate.print = custom_delegate->print;
    _delegate.print_error = custom_delegate->print_error;
    _delegate.prompt = custom_delegate->prompt;
    _delegate.password = custom_delegate->password;
    _delegate.source = custom_delegate->source;
  }
  else
  {
    _delegate.user_data = this;
    _delegate.print = &Interactive_shell::deleg_print;
    _delegate.print_error = &Interactive_shell::deleg_print_error;
    _delegate.prompt = &Interactive_shell::deleg_prompt;
    _delegate.password = &Interactive_shell::deleg_password;
    _delegate.source = &Interactive_shell::deleg_source;
  }

  // Sets the global options
  Value::Map_type_ref shcore_options = Shell_core_options::get();

  // Updates shell core options that changed upon initialization
  (*shcore_options)[SHCORE_BATCH_CONTINUE_ON_ERROR] = Value(_options.force);
  (*shcore_options)[SHCORE_INTERACTIVE] = Value(_options.interactive);
  (*shcore_options)[SHCORE_USE_WIZARDS] = Value(_options.wizards);
  if (!_options.output_format.empty())
    (*shcore_options)[SHCORE_OUTPUT_FORMAT] = Value(_options.output_format);

  _shell.reset(new Shell_core(&_delegate));

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

  SET_SHELL_COMMAND("\\help|\\?|\\h", "Print this help.", "", Interactive_shell::cmd_print_shell_help);
  SET_CUSTOM_SHELL_COMMAND("\\sql", "Switch to SQL processing mode.", "", std::bind(&Interactive_shell::switch_shell_mode, this, Shell_core::Mode_SQL, _1));
  SET_CUSTOM_SHELL_COMMAND("\\js", "Switch to JavaScript processing mode.", "", std::bind(&Interactive_shell::switch_shell_mode, this, Shell_core::Mode_JScript, _1));
  SET_CUSTOM_SHELL_COMMAND("\\py", "Switch to Python processing mode.", "", std::bind(&Interactive_shell::switch_shell_mode, this, Shell_core::Mode_Python, _1));
  SET_SHELL_COMMAND("\\source|\\.", "Execute a script file. Takes a file name as an argument.", cmd_help_source, Interactive_shell::cmd_process_file);
  SET_SHELL_COMMAND("\\", "Start multi-line input when in SQL mode.", "", Interactive_shell::cmd_start_multiline);
  SET_SHELL_COMMAND("\\quit|\\q|\\exit", "Quit MySQL Shell.", "", Interactive_shell::cmd_quit);
  SET_SHELL_COMMAND("\\connect|\\c", "Connect to a server.", cmd_help_connect, Interactive_shell::cmd_connect);
  SET_SHELL_COMMAND("\\warnings|\\W", "Show warnings after every statement.", "", Interactive_shell::cmd_warnings);
  SET_SHELL_COMMAND("\\nowarnings|\\w", "Don't show warnings after every statement.", "", Interactive_shell::cmd_nowarnings);
  SET_SHELL_COMMAND("\\status|\\s", "Print information about the current global connection.", "", Interactive_shell::cmd_status);
  SET_SHELL_COMMAND("\\use|\\u", "Set the current schema for the global session.", cmd_help_use, Interactive_shell::cmd_use);

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
  SET_SHELL_COMMAND("\\saveconn|\\savec", "Store a session configuration.", cmd_help_store_connection, Interactive_shell::cmd_store_connection);
  SET_SHELL_COMMAND("\\rmconn|\\rmc", "Remove the stored session configuration.", cmd_help_delete_connection, Interactive_shell::cmd_delete_connection);
  SET_SHELL_COMMAND("\\lsconn|\\lsc", "List stored session configurations.", "", Interactive_shell::cmd_list_connections);

  bool lang_initialized;
  _shell->switch_mode(_options.initial_mode, lang_initialized);

  _result_processor = std::bind(&Interactive_shell::process_result, this, _1);

  if (lang_initialized)
    init_scripts(_options.initial_mode);
}

bool Interactive_shell::cmd_process_file(const std::vector<std::string>& params)
{
  std::string file;

  if (params[0].find("\\source") != std::string::npos)
    _options.run_file = params[0].substr(8);
  else
    _options.run_file = params[0].substr(3);

  boost::trim(_options.run_file);

  Interactive_shell::process_file();

  return true;
}

void Interactive_shell::print_connection_message(mysh::SessionType type, const std::string& uri, const std::string& sessionid)
{
  std::string stype;

  switch (type)
  {
    case mysh::Application:
      stype = "an X";
      break;
    case mysh::Node:
      stype = "a Node";
      break;
    case mysh::Classic:
      stype = "a Classic";
      break;
  }

  std::string message;
  if (!sessionid.empty())
    message = "Using '" + sessionid + "' stored connection\n";

  message += "Creating " + stype + " Session to '" + uri + "'";

  if ((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string().find("json") == 0)
    print_json_info(message);
  else
  {
    message += "\n";
    _delegate.print(_delegate.user_data, message.c_str());
  }
}

bool Interactive_shell::connect(bool primary_session)
{
  try
  {
    Argument_list args;
    Value::Map_type_ref connection_data;
    bool secure_password = true;
    if (!_options.app.empty())
    {
      if (StoredSessions::get_instance()->connections()->has_key(_options.app))
        connection_data = (*StoredSessions::get_instance()->connections())[_options.app].as_map();
      else
        throw shcore::Exception::argument_error((boost::format("The stored connection %1% was not found") % _options.app).str());
    }
    else if (!_options.uri.empty())
    {
      connection_data = get_connection_data(_options.uri);
      if (connection_data->has_key("dbPassword") && !connection_data->get_string("dbPassword").empty())
        secure_password = false;
    }
    else
      connection_data.reset(new shcore::Value::Map_type);

    // If the session is being created from command line
    // Individual parameters will override whatever was defined on the URI/stored connection
    if (primary_session)
    {
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
        _delegate.print(_delegate.user_data, "mysqlx: [Warning] PLAIN authentication method is NOT secure!\n");

      if (!secure_password)
        _delegate.print(_delegate.user_data, "mysqlx: [Warning] Using a password on the command line interface can be insecure.\n");
    }

    // Sets any missing parameter to default values
    shcore::set_default_connection_data(connection_data, _options.session_type == mysh::Classic ? 3306 : 33060);

    if (_options.interactive)
      print_connection_message(_options.session_type, shcore::build_connection_string(connection_data, false), _options.app);

    args.push_back(Value(connection_data));

    connect_session(args, _options.session_type, primary_session ? _options.recreate_database : false);
  }
  catch (Exception &exc)
  {
    _delegate.print_error(_delegate.user_data, exc.format().c_str());
    return false;
  }
  catch (std::exception &exc)
  {
    std::string error(exc.what());
    error += "\n";
    _delegate.print_error(_delegate.user_data, error.c_str());
    return false;
  }

  return true;
}

Value Interactive_shell::connect_session(const Argument_list &args, mysh::SessionType session_type, bool recreate_schema)
{
  std::string pass;
  std::string schema_name;

  if (recreate_schema && session_type != mysh::Node && session_type != mysh::Classic)
    throw shcore::Exception::argument_error("Recreate schema option can only be used in classic or node sessions");

  shcore::Value::Map_type_ref connection_data = args.map_at(0);

  // Retrieves the schema on which the session will work on
  Argument_list schema_arg;
  if (connection_data->has_key("schema"))
  {
    schema_name = (*connection_data)["schema"].as_string();
    schema_arg.push_back(Value(schema_name));
  }

  if (recreate_schema && schema_name.empty())
      throw shcore::Exception::runtime_error("Recreate schema requested, but no schema specified");

  // Creates the argument list for the real connection call
  Argument_list connect_args;
  connect_args.push_back(shcore::Value(connection_data));

  // Prompts for the password if needed
  if (!connection_data->has_key("dbPassword") || _options.prompt_password)
  {
    if (_delegate.password(_delegate.user_data, "Enter password:", pass))
      connect_args.push_back(Value(pass));
  }

  std::shared_ptr<mysh::ShellDevelopmentSession> old_session(_shell->get_dev_session()),
                                                   new_session(_shell->connect_dev_session(connect_args, session_type));

  new_session->set_option("trace_protocol", _options.trace_protocol);

  if (recreate_schema)
  {
    std::string message = "Recreating schema " + schema_name + "...\n";
    _delegate.print(_delegate.user_data, message.c_str());
    try
    {
      new_session->drop_schema(schema_arg);
    }
    catch (shcore::Exception &e)
    {
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

  if (_options.interactive)
  {
    if (old_session && old_session.unique() && old_session->is_connected())
    {
      if (_options.interactive)
        _delegate.print(_delegate.user_data, "Closing old connection...\n");

      old_session->close(shcore::Argument_list());
    }

    _delegate.print(_delegate.user_data, "Session successfully established. ");

    std::string message;
    shcore::Value default_schema;
    std::string session_type = new_session->class_name();
    if (!session_type.compare("XSession"))
       default_schema = new_session->get_member("defaultSchema");
    else
       default_schema = new_session->get_member("currentSchema");

    if (default_schema)
    {
      if (session_type == "ClassicSession")
        message = "Default schema set to `" + default_schema.as_object()->get_member("name").as_string() + "`.";
      else
        message = "Default schema `" + default_schema.as_object()->get_member("name").as_string() + "` accessible through db.";
    }
    else
      message = "No default schema selected.";

    if ((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string().find("json") == 0)
      print_json_info(message);
    else
    {
      message += "\n";
      _delegate.print(_delegate.user_data, message.c_str());
    }

    // extra empty line to separate connect msgs from the stdheader nobody reads
    _delegate.print(_delegate.user_data, "\n");
  }

  return Value::Null();
}

// load scripts for standard locations in order to be able to implement standard routines
void Interactive_shell::init_scripts(Shell_core::Mode mode)
{
  std::string extension;

  if (mode == Shell_core::Mode_JScript)
    extension.append(".js");
  else if (mode == Shell_core::Mode_Python)
    extension.append(".py");
  else
    return;

  std::vector<std::string> scripts_paths;

  std::string user_file = "";

  try
  {
    // Checks existence of gobal startup script
    std::string path = shcore::get_global_config_path();
    path.append("shellrc");
    path.append(extension);
    if (file_exists(path))
      scripts_paths.push_back(path);

    // Checks existence of startup script at MYSQLSH_HOME
    // Or the binary location if not a standard installation
    path = shcore::get_mysqlx_home_path();
    if (!path.empty())
      path.append("/share/mysqlsh/mysqlshrc");
    else
    {
      path = shcore::get_binary_folder();
      path.append("/mysqlshrc");
    }
    path.append(extension);
    if (file_exists(path))
      scripts_paths.push_back(path);

    // Checks existence of user startup script
    path = shcore::get_user_config_path();
    path.append("mysqlshrc");
    path.append(extension);
    if (file_exists(path))
      scripts_paths.push_back(path);

    for (std::vector<std::string>::iterator i = scripts_paths.begin(); i != scripts_paths.end(); ++i)
    {
      _options.run_file = *i;
      process_file();
    }
  }
  catch (std::exception &e)
  {
    std::string error(e.what());
    error += "\n";
    _delegate.print_error(_delegate.user_data, error.c_str());
  }
}

std::string Interactive_shell::prompt()
{
  if (_input_mode != Input_ok)
  {
    return std::string(_shell->prompt().length() - 4, ' ').append("... ");
  }
  else
    return _shell->prompt();
}

bool Interactive_shell::switch_shell_mode(Shell_core::Mode mode, const std::vector<std::string> &UNUSED(args))
{
  Shell_core::Mode old_mode = _shell->interactive_mode();
  bool lang_initialized = false;

  if (old_mode != mode)
  {
    _input_mode = Input_ok;
    _input_buffer.clear();

    //XXX reset the history... history should be specific to each shell mode
    switch (mode)
    {
      case Shell_core::Mode_None:
        break;
      case Shell_core::Mode_SQL:
      {
        Value session = _shell->active_session();
        if (session && (session.as_object()->class_name() == "XSession"))
        {
          println("The active session is an " + session.as_object()->class_name());
          println("SQL mode is not supported on this session type: command ignored.");
          println("To switch to SQL mode reconnect with a Node Session by either:");
          println("* Using the \\connect -n shell command.");
          println("* Using --session-type=node when calling the MySQL Shell on the command line.");
        }
        else
        {
          if (_shell->switch_mode(mode, lang_initialized))
            println("Switching to SQL mode... Commands end with ;");
        }
        break;
      }
      case Shell_core::Mode_JScript:
#ifdef HAVE_V8
        if (_shell->switch_mode(mode, lang_initialized))
          println("Switching to JavaScript mode...");
#else
        println("JavaScript mode is not supported, command ignored.");
#endif
        break;
      case Shell_core::Mode_Python:
#ifdef HAVE_PYTHON
        if (_shell->switch_mode(mode, lang_initialized))
          println("Switching to Python mode...");
#else
        println("Python mode is not supported, command ignored.");
#endif
        break;
    }

    // load scripts for standard locations
    if (lang_initialized)
      init_scripts(mode);
  }

  return true;
}

void Interactive_shell::print(const std::string &str)
{
  std::cout << str;
}

void Interactive_shell::println(const std::string &str)
{
  std::string line(str);
  line += "\n";
  _delegate.print(_delegate.user_data, line.c_str());
}

void Interactive_shell::print_error(const std::string &error)
{
  Value error_val;
  try
  {
    error_val = Value::parse(error);
  }
  catch (shcore::Exception &e)
  {
    error_val = Value(error);
  }

  log_error("%s", error.c_str());
  std::string message;

  if ((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string().find("json") == 0)
  {
    Value::Map_type_ref error_map(new Value::Map_type());

    Value error_obj(error_map);

    (*error_map)["error"] = error_val;

    message = error_obj.json((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string() == "json");
  }
  else
  {
    message = "ERROR: ";
    if (error_val.type == shcore::Map)
    {
      Value::Map_type_ref error_map = error_val.as_map();

      if (error_map->has_key("code"))
      {
        //message.append(" ");
        message.append(((*error_map)["code"].repr()));

        if (error_map->has_key("state") && (*error_map)["state"])
          message.append(" (" + (*error_map)["state"].as_string() + ")");

        message.append(": ");
      }

      if (error_map->has_key("message"))
        message.append((*error_map)["message"].as_string());
      else
        message.append("?");
      message.append("\n");
    }
    else
      message = error_val.descr();
  }

  std::cerr << message << std::flush;
}

void Interactive_shell::print_json_info(const std::string &info, const std::string& label)
{
  shcore::JSON_dumper dumper((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string() == "json");
  dumper.start_object();
  dumper.append_string(label);
  dumper.append_string(info);
  dumper.end_object();

  _delegate.print(_delegate.user_data, dumper.str().c_str());
  _delegate.print(_delegate.user_data, "\n");
}

bool Interactive_shell::cmd_print_shell_help(const std::vector<std::string>& args)
{
  bool printed = false;

  // If help came with parameter attempts to print the
  // specific help on the active shell first and global commands
  if (args.size() > 1)
  {
    printed = _shell->print_help(args[1]);

    if (!printed)
      printed = _shell_command_handler.print_command_help(args[1]);
  }

  // If not specific help found, prints the generic help
  if (!printed)
  {
    _shell_command_handler.print_commands("===== Global Commands =====");

    // Prints the active shell specific help
    _shell->print_help("");

    println("");
    println("For help on a specific command use the command as \\? <command>");
    println("");
    auto globals = _shell->get_global_objects();

    if (globals.size())
    {
      println("===== Global Variables =====");

      for (auto name : globals)
      {
        auto object_val = _shell->get_global(name);
        auto object = std::dynamic_pointer_cast<Cpp_object_bridge>(object_val.as_object());
        auto brief = object->get_help_text("__brief__", false);

        if (!brief.empty())
          println((boost::format("%-10s %s") % name % brief).str());
      }
      println("");
      println("For more help on a global variable use <var>.help(), e.g. dba.help()");
      println("");
    }
  }

  return true;
}

bool Interactive_shell::cmd_start_multiline(const std::vector<std::string>& args)
{
  // This command is only available for SQL Mode
  if (args.size() == 1 && _shell->interactive_mode() == Shell_core::Mode_SQL)
  {
    _input_mode = Input_continued_block;

    return true;
  }

  return false;
}

bool Interactive_shell::cmd_connect(const std::vector<std::string>& args)
{
  bool error = false;
  _options.session_type = mysh::Application;

  // Holds the argument index for the target to which the session will be established
  size_t target_index = 1;

  if (args.size() > 1 && args.size() < 4)
  {
    if (args.size() == 3)
    {
      target_index++;

      std::string type = args[1];

      if (!type.compare("-x") || !type.compare("-X"))
        _options.session_type = mysh::Application;
      else if (!type.compare("-n") || !type.compare("-N"))
        _options.session_type = mysh::Node;
      else if (!type.compare("-c") || !type.compare("-C"))
        _options.session_type = mysh::Classic;
      else
        error = true;
    }

    if (!error)
    {
      if (args[target_index].find("$") == 0)
        _options.app = args[target_index].substr(1);
      else
      {
        _options.app = "";
        _options.uri = args[target_index];
      }

      connect();

      if (_shell->interactive_mode() == IShell_core::Mode_SQL && _options.session_type == mysh::Application)
        println("WARNING: An X Session has been established and SQL execution is not allowed.");
    }
  }

  if (error)
    _delegate.print_error(_delegate.user_data, "\\connect [-<type>] <uri or $name>\n");

  return true;
}

bool Interactive_shell::cmd_quit(const std::vector<std::string>& UNUSED(args))
{
  _options.interactive = false;

  return true;
}

bool Interactive_shell::cmd_warnings(const std::vector<std::string>& UNUSED(args))
{
  (*Shell_core_options::get())[SHCORE_SHOW_WARNINGS] = Value::True();

  println("Show warnings enabled.");

  return true;
}

bool Interactive_shell::cmd_nowarnings(const std::vector<std::string>& UNUSED(args))
{
  (*Shell_core_options::get())[SHCORE_SHOW_WARNINGS] = Value::False();

  println("Show warnings disabled.");

  return true;
}

bool Interactive_shell::cmd_store_connection(const std::vector<std::string>& args)
{
  std::string error;
  std::string name;
  std::string uri;

  bool overwrite = false;

  // Reads the parameters
  switch (args.size())
  {
    case 2:
      if (args[1] == "-f")
        error = "usage";
      else
        name = args[1];
      break;
    case 3:
      if (args[1] == "-f")
      {
        overwrite = true;
        name = args[2];
      }
      else
      {
        name = args[1];
        uri = args[2];
      }
      break;
    case 4:
      if (args[1] != "-f")
        error = "usage";
      else
      {
        overwrite = true;
        name = args[2];
        uri = args[3];
      }

      break;
      break;
    default:
      error = "usage";
  }

  // Performs additional validations
  if (error.empty())
  {
    if (!shcore::is_valid_identifier(name))
      error = (boost::format("The session configuration name '%s' is not a valid identifier") % name).str();
    else
    {
      if (uri.empty())
      {
        if (_shell->get_dev_session())
          uri = _shell->get_dev_session()->uri();
        else
          error = "Unable to save session information, no active session available";
      }
    }
  }

  // Attempsts the store
  if (error.empty())
  {
    try
    {
      StoredSessions::get_instance()->add_connection(name, uri, overwrite);

      std::string uri = shcore::build_connection_string((*StoredSessions::get_instance()->connections())[name].as_map(), false);

      _delegate.print(_delegate.user_data, (boost::format("Successfully stored %s as %s.\n") % uri % name).str().c_str());
    }
    catch (std::exception& err)
    {
      error = err.what();
    }
  }
  else if (error == "usage")
    error = "\\saveconn [-f] <session_cfg_name> [<uri>]";

  if (!error.empty())
  {
    error += "\n";
    _delegate.print_error(_delegate.user_data, error.c_str());
  }

  return true;
}

bool Interactive_shell::cmd_delete_connection(const std::vector<std::string>& args)
{
  std::string error;

  if (args.size() == 2)
  {
    try
    {
      StoredSessions::get_instance()->remove_connection(args[1]);

      _delegate.print(_delegate.user_data, (boost::format("Successfully deleted session configuration named %s.\n") % args[1]).str().c_str());
    }
    catch (std::exception& err)
    {
      error = err.what();
    }
  }
  else
    error = "\\rmconn <session_cfg_name>";

  if (!error.empty())
  {
    error += "\n";
    _delegate.print_error(_delegate.user_data, error.c_str());
  }

  return true;
}

bool Interactive_shell::cmd_list_connections(const std::vector<std::string>& args)
{
  if (args.size() == 1)
  {
    std::string format = (*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string();

    Value::Map_type_ref connections = StoredSessions::get_instance()->connections();
    if (format.find("json") != std::string::npos)
      _delegate.print(_delegate.user_data, shcore::Value(connections).json(format != "json/raw").c_str());
    else
    {
      for (auto connection : (*connections.get()))
      {
        std::string uri = shcore::build_connection_string(connection.second.as_map(), false);
        _delegate.print(_delegate.user_data, (boost::format("%1% : %2%\n") % connection.first % uri).str().c_str());
      }
    }

    _delegate.print(_delegate.user_data, "\n");
  }
  else
    _delegate.print_error(_delegate.user_data, "\\lsconn\n");

  return true;
}

bool Interactive_shell::cmd_status(const std::vector<std::string>& UNUSED(args))
{
  std::string version_msg("MySQL Shell Version ");
  version_msg += MYSH_VERSION;
  version_msg += " Development Preview\n";
  println(version_msg);

  if (_shell->get_dev_session() && _shell->get_dev_session()->is_connected())
  {
    shcore::Value raw_status = _shell->get_dev_session()->get_status(shcore::Argument_list());
    std::string output_format = (*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string();

    if (output_format.find("json") == 0)
      println(raw_status.json(output_format == "json"));
    else
    {
      shcore::Value::Map_type_ref status = raw_status.as_map();

      std::string format = "%-30s%s";

      if (status->has_key("STATUS_ERROR"))
        println((boost::format(format) % "Error Retrieving Status: " % (*status)["STATUS_ERROR"].descr(true)).str());
      else
      {
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

        if (status->has_key("SERVER_STATS"))
        {
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
  }
  else
    _delegate.print_error(_delegate.user_data, "Not Connected.\n");

  return true;
}

bool Interactive_shell::cmd_use(const std::vector<std::string>& args)
{
  std::string error;
  if (_shell->get_dev_session() && _shell->get_dev_session()->is_connected())
  {
    std::string real_param;

    // If quoted, takes as param what's inside of the quotes
    auto start = args[0].find_first_of("\"'`");
    if (start != std::string::npos)
    {
      std::string quote = args[0].substr(start, 1);

      if (args[0].size() >= start)
      {
        auto end = args[0].find(quote, start + 1);

        if (end != std::string::npos)
          real_param = args[0].substr(start + 1, end - start - 1);
        else
          error = "Mistmatched quote on command parameter: " + args[0].substr(start) + "\n";
      }
    }
    else if (args.size() == 2)
      real_param = args[1];
    else
      error = "\\use <schema_name>\n";

    if (error.empty())
    {
      try
      {
        shcore::Value schema = _shell->set_current_schema(real_param);
        std::string message = "Schema `" + schema.as_object()->get_member("name").as_string() + "` accessible through db.";

        if ((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string().find("json") == 0)
          print_json_info(message);
        else
        {
          message += "\n";
          _delegate.print(_delegate.user_data, message.c_str());
        }
      }
      catch (shcore::Exception &e)
      {
        error = e.format();
      }
    }
  }
  else
    error = "Not Connected.\n";

  if (!error.empty())
    _delegate.print_error(_delegate.user_data, error.c_str());

  return true;
}

void Interactive_shell::deleg_print(void *cdata, const char *text)
{
  Interactive_shell *self = (Interactive_shell*)cdata;
  self->print(text);
}

void Interactive_shell::deleg_print_error(void *cdata, const char *text)
{
  Interactive_shell *self = (Interactive_shell*)cdata;
  self->print_error(text);
}

char *Interactive_shell::readline(const char *prompt)
{
  char *tmp = NULL;
#ifndef WIN32
  tmp = ::readline(prompt);
#else
  // TODO: This should be ported from the server, not used from there
  /*
  tmp = (char *)malloc(MAX_READLINE_BUF);
  if (!tmp)
  throw Exception::runtime_error("Cannot allocate memory for Interactive_shell::readline buffer.");
  my_win_console_fputs(&my_charset_latin1, prompt);
  tmp = my_win_console_readline(&my_charset_latin1, tmp, MAX_READLINE_BUF);
  */
  std::string line;
  std::cout << prompt << std::flush;
  std::getline(std::cin, line);

  if (!std::cin.fail())
    tmp = strdup(line.c_str());

#endif
  return tmp;
}

bool Interactive_shell::deleg_prompt(void *UNUSED(cdata), const char *prompt, std::string &ret)
{
  char *tmp = Interactive_shell::readline(prompt);
  if (!tmp)
    return false;

  ret = tmp;
  free(tmp);

  return true;
}

bool Interactive_shell::deleg_password(void *cdata, const char *prompt, std::string &ret)
{
  Interactive_shell *self = (Interactive_shell*)cdata;
  char *tmp = self->_options.passwords_from_stdin ? mysh_get_stdin_password(prompt) : mysh_get_tty_password(prompt);
  if (!tmp)
    return false;
  ret = tmp;
  free(tmp);
  return true;
}

void Interactive_shell::deleg_source(void *cdata, const char *module)
{
  Interactive_shell *self = (Interactive_shell*)cdata;
  self->_options.run_file.assign(module);
  self->process_file();
}

bool Interactive_shell::do_shell_command(const std::string &line)
{
  // Verifies if the command can be handled by the active shell
  bool handled = _shell->handle_shell_command(line);

  // Global Command Processing (xShell specific)
  if (!handled)
    handled = _shell_command_handler.process(line);

  return handled;
}

void Interactive_shell::process_line(const std::string &line)
{
  bool handled_as_command = false;

  // check if the line is an escape/shell command
  if (_input_buffer.empty() && !line.empty() && _input_mode == Input_ok)
  {
    try
    {
      handled_as_command = do_shell_command(line);
    }
    catch (std::exception &exc)
    {
      std::string error(exc.what());
      error += "\n";
      _delegate.print_error(_delegate.user_data, error.c_str());
    }
  }

  if (!handled_as_command)
  {
    if (_input_mode == Input_continued_block && line.empty())
      _input_mode = Input_ok;

    // Appends the line, no matter it is an empty line
    _input_buffer.append(_shell->preprocess_input_line(line));

    // Appends the new line if anything has been added to the buffer
    if (!_input_buffer.empty())
      _input_buffer.append("\n");

    if (_input_mode != Input_continued_block && !_input_buffer.empty())
    {
      try
      {
        _shell->handle_input(_input_buffer, _input_mode, _result_processor);

        // Here we analyze the input mode as it was let after executing the code
        if (_input_mode == Input_ok)
        {
          std::string executed = _shell->get_handled_input();

          if (!executed.empty())
          {
#ifndef WIN32
            add_history(executed.c_str());
#endif
            println("");
          }
        }
          }
      catch (shcore::Exception &exc)
      {
        _delegate.print_error(_delegate.user_data, exc.format().c_str());
      }
      catch (std::exception &exc)
      {
        std::string error(exc.what());
        error += "\n";
        _delegate.print_error(_delegate.user_data, error.c_str());
      }

      // TODO: Do we need this cleanup? i.e. in case of exceptions above??
      // Clears the buffer if OK, if continued, buffer will contain
      // the non executed code
      if (_input_mode == Input_ok)
        _input_buffer.clear();
        }
      }

  _shell->reconnect_if_needed();
    }

void Interactive_shell::abort()
{
  if (!_shell) return;

  if (_shell->is_running_query())
  {
    try
    {
      _shell->abort();
    }
    catch (std::runtime_error& e)
    {
      log_exception("Error when killing connection ", e);
    }
  }
}

void Interactive_shell::process_result(shcore::Value result)
{
  if ((*Shell_core_options::get())[SHCORE_INTERACTIVE].as_bool()
      || _shell->interactive_mode() == Shell_core::Mode_SQL)
  {
    if (result)
    {
      Value shell_hook;
      std::shared_ptr<Object_bridge> object;
      if (result.type == shcore::Object)
      {
        object = result.as_object();
        if (object && object->has_member("__shell_hook__"))
          shell_hook = object->get_member("__shell_hook__");

        if (shell_hook)
        {
          Argument_list args;
          Value hook_result = object->call("__shell_hook__", args);

          // Recursive call to continue processing shell hooks if any
          process_result(hook_result);
        }
      }

      // If the function is not found the values still needs to be printed
      if (!shell_hook)
      {
        // Resultset objects get printed
        if (object && object->class_name().find("Result") != std::string::npos)
        {
          std::shared_ptr<mysh::ShellBaseResult> resultset = std::static_pointer_cast<mysh::ShellBaseResult> (object);

          // Result buffering will be done ONLY if on any of the scripting interfaces
          ResultsetDumper dumper(resultset, _shell->interactive_mode() != IShell_core::Mode_SQL);
          dumper.dump();
        }
        else
        {
          if ((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string().find("json") == 0)
          {
            shcore::JSON_dumper dumper((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string() == "json");
            dumper.start_object();
            dumper.append_value("result", result);
            dumper.end_object();

            _delegate.print(_delegate.user_data, dumper.str().c_str());
          }
          else
            _delegate.print(_delegate.user_data, result.descr(true).c_str());
        }
      }
    }
  }

  // Return value of undefined implies an error processing
  if (result.type == shcore::Undefined)
    _shell->set_error_processing();
}

int Interactive_shell::process_file()
{
  // Default return value will be 1 indicating there were errors
  int ret_val = 1;

  if (_options.run_file.empty())
    _delegate.print_error(_delegate.user_data, "Usage: \\. <filename> | \\source <filename>\n");
  else
    //TODO: do path expansion (in case ~ is used in linux)
  {
    std::ifstream s(_options.run_file.c_str());

    if (!s.fail())
    {
      // The return value now depends on the stream processing
      ret_val = process_stream(s, _options.run_file);

      // When force is used, we do not care of the processing
      // errors
      if (_options.force)
        ret_val = 0;

      s.close();
    }
    else
    {
      // TODO: add a log entry once logging is
      _delegate.print_error(_delegate.user_data, (boost::format("Failed to open file '%s', error: %d\n") % _options.run_file % errno).str().c_str());
    }
  }

  return ret_val;
}

int Interactive_shell::process_stream(std::istream & stream, const std::string& source)
{
  // If interactive is set, it means that the shell was started with the option to
  // Emulate interactive mode while processing the stream
  if (_options.interactive)
  {
    bool comment_first_js_line = _shell->interactive_mode() == IShell_core::Mode_JScript;
    while (!stream.eof())
    {
      std::string line;

      std::getline(stream, line);

      // When processing JavaScript files, validates the very first line to start with #!
      // If that's the case, it is replaced by a comment indicator //
      if (comment_first_js_line && line.size() > 1 && line[0] == '#' && line[1] == '!')
        line.replace(0, 2, "//");

      comment_first_js_line = false;

      if (_options.full_interactive)
      {
        _delegate.print(_delegate.user_data, prompt().c_str());
        println(line);
      }

      process_line(line);
    }

    // Being interactive, we do not care about the return value
    return 0;
  }
  else
  {
    return _shell->process_stream(stream, source, _result_processor);
  }
}

void Interactive_shell::command_loop()
{
  if (_options.interactive) // check if interactive
  {
    std::string message;
    switch (_shell->interactive_mode())
    {
      case Shell_core::Mode_SQL:
#ifdef HAVE_V8
        message = "Currently in SQL mode. Use \\js or \\py to switch the shell to a scripting language.";
#else
        message = "Currently in SQL mode. Use \\py to switch the shell to python scripting.";
#endif
        break;
      case Shell_core::Mode_JScript:
        message = "Currently in JavaScript mode. Use \\sql to switch to SQL mode and execute queries.";
        break;
      case Shell_core::Mode_Python:
        message = "Currently in Python mode. Use \\sql to switch to SQL mode and execute queries.";
        break;
      default:
        break;
    }

    if (!message.empty())
    {
      if ((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string().find("json") == 0)
        print_json_info(message);
      else
      {
        message += "\n";
        _delegate.print(_delegate.user_data, message.c_str());
      }
    }
  }

  while (_options.interactive)
  {
    char *cmd = Interactive_shell::readline(prompt().c_str());
    if (!cmd)
      break;

    process_line(cmd);
    free(cmd);
  }

  std::cout << "Bye!\n";
}

void Interactive_shell::print_banner()
{
  std::string welcome_msg("Welcome to MySQL Shell ");
  welcome_msg += MYSH_VERSION;
  welcome_msg += " Development Preview";
  println(welcome_msg);
  println("");
  println("Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.");
  println("");
  println("Oracle is a registered trademark of Oracle Corporation and/or its");
  println("affiliates. Other names may be trademarks of their respective");
  println("owners.");
  println("");
  println("Type '\\help', '\\h' or '\\?' for help, type '\\quit' or '\\q' to exit.");
  println("");
}

void Interactive_shell::print_cmd_line_helper()
{
  std::string help_msg("MySQL Shell ");
  help_msg += MYSH_VERSION;
  help_msg += " Development Preview";
  println(help_msg);
  println("");
  println("Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.");
  println("");
  println("Oracle is a registered trademark of Oracle Corporation and/or its");
  println("affiliates. Other names may be trademarks of their respective");
  println("owners.");
  println("");
  println("Usage: mysqlsh [OPTIONS] [db_name]");
  println("  --help                   Display this help and exit.");
  println("  -f, --file=file          Process file.");
  println("  -e, --execute=<cmd>      Execute command and quit.");
  println("  --uri                    Connect to Uniform Resource Identifier.");
  println("                           Format: [user[:pass]]@host[:port][/db]");
  println("                           or user[:pass]@::socket[/db] .");
  println("  --app                    Connect to using a Stored Session.");
  println("  -h, --host=name          Connect to host.");
  println("  -P, --port=#             Port number to use for connection.");
  println("  -S, --socket=sock        Socket name to use in UNIX, pipe name to use in Windows (only classic sessions).");
  println("  -u, --dbuser=name        User for the connection to the server.");
  println("  --user=name              An alias for dbuser.");
  println("  --dbpassword=name        Password to use when connecting to server");
  println("  --password=name          An alias for dbpassword.");
  println("  -p                       Request password prompt to set the password");
  println("  -D --schema=name         Schema to use.");
  println("  --recreate-schema        Drop and recreate the specified schema. Schema will be deleted if it exists!");
  println("  --database=name          An alias for --schema.");
  println("  --x                      Uses connection data to create an X Session.");
  println("  --node                   Uses connection data to create a Node Session.");
  println("  --classic                Uses connection data to create a Classic Session.");
  println("  --sql                    Start in SQL mode using a node session.");
  println("  --sqlc                   Start in SQL mode using a classic session.");
  println("  --js                     Start in JavaScript mode.");
  println("  --py                     Start in Python mode.");
  println("  --json                   Produce output in JSON format.");
  println("  --table                  Produce output in table format (default for interactive mode).");
  println("                           This option can be used to force that format when running in batch mode.");
  println("  -i, --interactive[=full] To use in batch mode, it forces emulation of interactive mode processing.");
  println("                           Each line on the batch is processed as if it were in interactive mode.");
  println("  --force                  To use in SQL batch mode, forces processing to continue if an error is found.");
  println("  --log-level=value        The log level." + ngcommon::Logger::get_level_range_info());
  println("  --version                Prints the version of MySQL Shell.");
  println("  --ssl                    Enable SSL for connection(automatically enabled with other flags)");
  println("  --ssl-key=name           X509 key in PEM format");
  println("  --ssl-cert=name          X509 cert in PEM format");
  println("  --ssl-ca=name            CA file in PEM format (check OpenSSL docs)");
  println("  --passwords-from-stdin   Read passwords from stdin instead of the tty");
  println("  --auth-method=method     Authentication method to use");
  println("  --show-warnings          Automatically display SQL warnings on SQL mode if available");
  println("  --dba enableXProtocol    Enable the X Protocol in the server connected to. Must be used with --classic");

  println("");
}