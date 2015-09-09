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

#include <fstream>
#include "mysh.h"

#include "shellcore/types.h"
#include "utils/utils_file.h"

#ifndef WIN32
#  include "editline/readline.h"
#endif

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/pointer_cast.hpp>
#include <boost/scope_exit.hpp>
#include <iostream>

#include "shellcore/types_cpp.h"

#include "shellcore/lang_base.h"
#include "shellcore/shell_core.h"
#include "shellcore/ishell_core.h"
#include "shellcore/common.h"

#include "shell_cmdline_options.h"

#include "../modules/base_session.h"
//#include "../modules/mod_schema.h"
#include <sys/stat.h>

#ifdef WIN32
#  include <io.h>
#  define isatty _isatty
#endif

using namespace shcore;

// TODO: This should be ported from the server, not used from there (see comment bellow)
//const int MAX_READLINE_BUF = 65536;

extern char *mysh_get_tty_password(const char *opt_message);

class Interactive_shell
{
public:
  Interactive_shell(const Shell_command_line_options &options);
  void command_loop();
  int process_stream(std::istream & stream, const std::string& source);
  int process_file();

  void init_environment();
  void init_scripts(Shell_core::Mode mode);

  void cmd_process_file(const std::vector<std::string>& params);
  bool connect();

  void print(const std::string &str);
  void println(const std::string &str);
  void print_error(const std::string &error);
  void print_json_info(const std::string &info, const std::string& label = "info");

  void cmd_print_shell_help(const std::vector<std::string>& args);
  void cmd_start_multiline(const std::vector<std::string>& args);
  void cmd_connect(const std::vector<std::string>& args);
  void cmd_connect_node(const std::vector<std::string>& args);
  void cmd_connect_classic(const std::vector<std::string>& args);
  void cmd_quit(const std::vector<std::string>& args);
  void cmd_warnings(const std::vector<std::string>& args);
  void cmd_nowarnings(const std::vector<std::string>& args);

  void print_banner();
  void print_cmd_line_helper();

  void set_log_level(ngcommon::Logger::LOG_LEVEL level) { if (_logger) _logger->set_log_level(level); }

private:
  Shell_command_line_options _options;
  static char *readline(const char *prompt);
  void process_line(const std::string &line);
  void process_result(shcore::Value result);
  std::string prompt();
  ngcommon::Logger* _logger;

  void switch_shell_mode(Shell_core::Mode mode, const std::vector<std::string> &args);

private:
  shcore::Value connect_session(const shcore::Argument_list &args, mysh::SessionType session_type);

private:
  static void deleg_print(void *self, const char *text);
  static void deleg_print_error(void *self, const char *text);
  static bool deleg_input(void *self, const char *text, std::string &ret);
  static bool deleg_password(void *self, const char *text, std::string &ret);
  static void deleg_source(void *self, const char *module);

  bool do_shell_command(const std::string &command);
private:
  Interpreter_delegate _delegate;

  boost::shared_ptr<mysh::ShellBaseSession> _session;
  //boost::shared_ptr<mysh::mysqlx::Schema> _db;
  boost::shared_ptr<Shell_core> _shell;

  std::string _input_buffer;
  bool _multiline_mode;

  Shell_command_handler _shell_command_handler;
};

Interactive_shell::Interactive_shell(const Shell_command_line_options &options) :
_options(options)
{
  std::string log_path = get_user_config_path();
  log_path += "mysqlx.log";
  ngcommon::Logger::create_instance(log_path.c_str(), false, _options.log_level);
  _logger = ngcommon::Logger::singleton();

#ifndef WIN32
  rl_initialize();
#endif
  //  using_history();

  _multiline_mode = false;

  _delegate.user_data = this;
  _delegate.print = &Interactive_shell::deleg_print;
  _delegate.print_error = &Interactive_shell::deleg_print_error;
  _delegate.input = &Interactive_shell::deleg_input;
  _delegate.password = &Interactive_shell::deleg_password;
  _delegate.source = &Interactive_shell::deleg_source;

  // Sets the global options
  Value::Map_type_ref shcore_options = Shell_core_options::get();

  // Updates shell core options that changed upon initialization
  (*shcore_options)[SHCORE_BATCH_CONTINUE_ON_ERROR] = Value(_options.force);
  (*shcore_options)[SHCORE_INTERACTIVE] = Value(_options.interactive);

  if (!_options.output_format.empty())
    (*shcore_options)[SHCORE_OUTPUT_FORMAT] = Value(_options.output_format);

  _shell.reset(new Shell_core(&_delegate));

  std::string cmd_help =
    "SYNTAX:\n"
    "   \\connect%1% <URI>\n\n"
    "WHERE:\n"
    "   URI is in the format of: [user[:password]@]hostname[:port]\n"
    "EXAMPLE:\n"
    "   \\connect%1% root@localhost\n";

  std::string cmd_help_source =
    "SYNTAX:\n"
    "   \\source <sql_file_path>\n"
    "   \\. <sql_file_path>\n\n"
    "EXAMPLES:\n"
    "   \\source C:\\Users\\MySQL\\sakila.sql\n"
    "   \\. C:\\Users\\MySQL\\sakila.sql\n\n"
    "NOTE: Can execute files from the supported types: SQL, Javascript, or Python.\n"
    "Processing is done using the active language set for processing mode.\n";

  SET_SHELL_COMMAND("\\help|\\?|\\h", "Print this help.", "", Interactive_shell::cmd_print_shell_help);
  SET_CUSTOM_SHELL_COMMAND("\\sql", "Sets shell on SQL processing mode.", "", boost::bind(&Interactive_shell::switch_shell_mode, this, Shell_core::Mode_SQL, _1));
  SET_CUSTOM_SHELL_COMMAND("\\js", "Sets shell on JavaScript processing mode.", "", boost::bind(&Interactive_shell::switch_shell_mode, this, Shell_core::Mode_JScript, _1));
  SET_CUSTOM_SHELL_COMMAND("\\py", "Sets shell on Python processing mode.", "", boost::bind(&Interactive_shell::switch_shell_mode, this, Shell_core::Mode_Python, _1));
  SET_SHELL_COMMAND("\\source|\\.", "Execute a script file. Takes a file name as an argument.", cmd_help_source, Interactive_shell::cmd_process_file);
  SET_SHELL_COMMAND("\\", "Start multiline input. Finish and execute with an empty line.", "", Interactive_shell::cmd_start_multiline);
  SET_SHELL_COMMAND("\\quit|\\q|\\exit", "Quit mysh.", "", Interactive_shell::cmd_quit);
  SET_SHELL_COMMAND("\\connect", "Connect to server using an application mode session.", (boost::format(cmd_help) % "").str(), Interactive_shell::cmd_connect);
  SET_SHELL_COMMAND("\\connect_node", "Connect to server using a node session.", (boost::format(cmd_help) % "_node").str(), Interactive_shell::cmd_connect_node);
  SET_SHELL_COMMAND("\\connect_classic", "Connect to server using the MySQL protocol.", (boost::format(cmd_help) % "_classic").str(), Interactive_shell::cmd_connect_classic);
  SET_SHELL_COMMAND("\\warnings|\\W", "Show warnings after every statement..", "", Interactive_shell::cmd_warnings);
  SET_SHELL_COMMAND("\\nowarnings|\\w", "Don't show warnings after every statement..", "", Interactive_shell::cmd_nowarnings);

  bool lang_initialized;
  _shell->switch_mode(_options.initial_mode, lang_initialized);

  if (lang_initialized)
    init_scripts(_options.initial_mode);
}

void Interactive_shell::cmd_process_file(const std::vector<std::string>& params)
{
  _options.run_file = boost::join(params, " ");

  Interactive_shell::process_file();
}

bool Interactive_shell::connect()
{
  try
  {
    if (_session && _session->is_connected())
    {
      if (_options.interactive)
        shcore::print("Closing old connection...\n");

      _session->close(shcore::Argument_list());
    }

    // strip password from uri
    if (_options.interactive)
    {
      std::string stype;

      switch (_options.session_type)
      {
        case mysh::Application:
          stype = "Application";
          break;
        case mysh::Node:
          stype = "Node";
          break;
        case mysh::Classic:
          stype = "Classic";
          break;
      }

      std::string uri_stripped = mysh::strip_password(_options.uri);

      std::string message = "Creating " + stype + " Session to " + uri_stripped + "...";
      if ((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string().find("json") == 0)
        print_json_info(message);
      else
        shcore::print(message + "\n");
    }

    Argument_list args;
    args.push_back(Value(_options.uri));
    connect_session(args, _options.session_type);
  }
  catch (std::exception &exc)
  {
    print_error(exc.what());
    return false;
  }

  return true;
}

Value Interactive_shell::connect_session(const Argument_list &args, mysh::SessionType session_type)
{
  std::string protocol;
  std::string user;
  std::string pass;
  std::string host;
  int port = 3306;
  std::string sock;
  std::string db;
  int pwd_found;

  Argument_list connect_args(args);

  // Handles the case where the password needs to be prompted
  if (!mysh::parse_mysql_connstring(args[0].as_string(), protocol, user, pass, host, port, sock, db, pwd_found))
    throw shcore::Exception::argument_error("Could not parse URI for MySQL connection");
  else
  {
    // This implies the URI is defined as user:@
    // So the : indicating there should be a password is there but the actual password is empty
    // It means we need to prompt fopr the password
    if (pwd_found && pass.empty())
    {
      char *tmp = mysh_get_tty_password("Enter password: ");
      if (tmp)
      {
        pass.assign(tmp);
        free(tmp);
        connect_args.push_back(Value(pass));
      }
    }
  }

  // Performs the connection
  boost::shared_ptr<mysh::ShellBaseSession> new_session(mysh::connect_session(connect_args, session_type));

  _session.reset(new_session, new_session.get());

  _shell->set_global("session", Value(boost::static_pointer_cast<Object_bridge>(_session)));

  // The default schemas is retrieved it will return null if none is set
  Value default_schema = _session->get_member("defaultSchema");

  // Whatever default schema is returned is ok to be set on db
  _shell->set_global("db", default_schema);

  if (_options.interactive)
  {
    std::string message;
    if (default_schema)
      message = "Default schema `" + db + "` accessible through db.";
    else
      message = "No default schema selected.";

    if ((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string().find("json") == 0)
      print_json_info(message);
    else
      shcore::print(message + "\n");
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

  get_user_config_path();

  try
  {
    std::string path = shcore::get_user_config_path();
    path += std::string(".shellrc");
    user_file = path;

    user_file += extension;
    if (file_exists(user_file))
      scripts_paths.push_back(user_file);
#ifndef WIN32
    std::string global_file("/usr/share/mysqlx/js/shellrc");

    global_file += extension;

    if (file_exists(global_file))
      scripts_paths.push_back(global_file);
#endif

    for (std::vector<std::string>::iterator i = scripts_paths.begin(); i != scripts_paths.end(); ++i)
    {
      _options.run_file = *i;
      process_file();
    }
  }
  catch (std::exception &e)
  {
    print_error(e.what());
  }
}

std::string Interactive_shell::prompt()
{
  if (_multiline_mode)
    return "... ";
  else
    return _shell->prompt();
}

void Interactive_shell::switch_shell_mode(Shell_core::Mode mode, const std::vector<std::string> &UNUSED(args))
{
  Shell_core::Mode old_mode = _shell->interactive_mode();
  bool lang_initialized = false;

  if (old_mode != mode)
  {
    _multiline_mode = false;
    _input_buffer.clear();

    //XXX reset the history... history should be specific to each shell mode
    switch (mode)
    {
      case Shell_core::Mode_None:
        break;
      case Shell_core::Mode_SQL:
        if (_shell->switch_mode(mode, lang_initialized))
          println("Switching to SQL mode... Commands end with ;");
        break;
      case Shell_core::Mode_JScript:
#ifdef HAVE_V8
        if (_shell->switch_mode(mode, lang_initialized))
          println("Switching to JavaScript mode...");
#else
        println("JavaScript mode is not supported on this platform, command ignored.");
#endif
        break;
      case Shell_core::Mode_Python:
        // TODO: remove following #if 0 #endif as soon as Python mode is implemented
#ifdef HAVE_PYTHON
        if (_shell->switch_mode(mode, lang_initialized))
          println("Switching to Python mode...");
#else
        println("Python mode is not yet supported, command ignored.");
#endif
        break;
    }

    // load scripts for standard locations
    if (lang_initialized)
      init_scripts(mode);
  }
}

void Interactive_shell::print(const std::string &str)
{
  std::cout << str;
}

void Interactive_shell::println(const std::string &str)
{
  std::cout << str << "\n";
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
    }
    else
      message = error_val.descr();
  }

  std::cerr << message << "\n";
}

void Interactive_shell::print_json_info(const std::string &info, const std::string& label)
{
  shcore::JSON_dumper dumper((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string() == "json");
  dumper.start_object();
  dumper.append_string(label);
  dumper.append_string(info);
  dumper.end_object();

  shcore::print(dumper.str() + "\n");
}

void Interactive_shell::cmd_print_shell_help(const std::vector<std::string>& args)
{
  bool printed = false;

  // If help came with parameter attempts to print the
  // specific help on the active shell first and global commands
  if (!args.empty())
  {
    printed = _shell->print_help(args[0]);

    if (!printed)
      printed = _shell_command_handler.print_command_help(args[0]);
  }

  // If not specific help found, prints the generic help
  if (!printed)
  {
    _shell_command_handler.print_commands("===== Global Commands =====");

    std::cout << std::endl;
    std::cout << std::endl;

    // Prints the active shell specific help
    _shell->print_help("");

    std::cout << std::endl << "For help on a specific command use the command as \\? <command>" << std::endl;
  }
}

void Interactive_shell::cmd_start_multiline(const std::vector<std::string>& args)
{
  if (args.empty())
    _multiline_mode = true;
}

void Interactive_shell::cmd_connect(const std::vector<std::string>& args)
{
  if (args.size() == 1)
  {
    _options.uri = args[0];
    _options.session_type = mysh::Application;
    connect();
  }
  else
    print_error("\\connect <uri>");
}

void Interactive_shell::cmd_connect_node(const std::vector<std::string>& args)
{
  if (args.size() == 1)
  {
    _options.uri = args[0];
    _options.session_type = mysh::Node;
    connect();
  }
  else
    print_error("\\connect_node <uri>");
}

void Interactive_shell::cmd_connect_classic(const std::vector<std::string>& args)
{
  if (args.size() == 1)
  {
    _options.uri = args[0];
    _options.session_type = mysh::Classic;
    connect();
  }
  else
    print_error("\\connect_classic <uri>");
}

void Interactive_shell::cmd_quit(const std::vector<std::string>& UNUSED(args))
{
  _options.interactive = false;
}

void Interactive_shell::cmd_warnings(const std::vector<std::string>& UNUSED(args))
{
  (*Shell_core_options::get())[SHCORE_SHOW_WARNINGS] = Value::True();

  println("Show warnings enabled.");
}

void Interactive_shell::cmd_nowarnings(const std::vector<std::string>& UNUSED(args))
{
  (*Shell_core_options::get())[SHCORE_SHOW_WARNINGS] = Value::False();

  println("Show warnings disabled.");
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
  char *tmp;
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
  tmp = strdup(line.c_str());
#endif
  return tmp;
}

bool Interactive_shell::deleg_input(void *UNUSED(cdata), const char *prompt, std::string &ret)
{
  char *tmp = Interactive_shell::readline(prompt);
  if (!tmp)
    return false;

  ret = tmp;
  free(tmp);

  return true;
}

bool Interactive_shell::deleg_password(void *UNUSED(cdata), const char *prompt, std::string &ret)
{
  char *tmp = mysh_get_tty_password(prompt);
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
  Interactive_input_state state = Input_ok;

  // check if the line is an escape/shell command
  if (_input_buffer.empty() && !line.empty() && !_multiline_mode)
  {
    try
    {
      handled_as_command = do_shell_command(line);
    }
    catch (std::exception &exc)
    {
      print_error(exc.what());
    }
  }

  if (!handled_as_command)
  {
    if (_multiline_mode && line.empty())
      _multiline_mode = false;
    else
    {
      if (_input_buffer.empty())
        _input_buffer = line;
      else
        _input_buffer.append("\n").append(line);
    }

    if (!_multiline_mode)
    {
      try
      {
        _shell->handle_input(_input_buffer, state, boost::bind(&Interactive_shell::process_result, this, _1));

        std::string executed = _shell->get_handled_input();

        if (!executed.empty())
        {
#ifndef WIN32
          add_history(executed.c_str());
#endif
          println("");
        }
      }
      catch (std::exception &exc)
      {
        print_error(exc.what());
      }

      // Clears the buffer if OK, if continued, buffer will contain
      // the non executed code
      if (state == Input_ok)
        _input_buffer.clear();
    }
  }
}

void Interactive_shell::process_result(shcore::Value result)
{
  if ((*Shell_core_options::get())[SHCORE_INTERACTIVE].as_bool())
  {
    if (result)
    {
      Value shell_hook;
      if (result.type == shcore::Object)
      {
        boost::shared_ptr<Object_bridge> object = result.as_object();
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
        if ((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string().find("json") == 0)
        {
          shcore::JSON_dumper dumper((*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string() == "json");
          dumper.start_object();
          dumper.append_value("result", result);
          dumper.end_object();

          print(dumper.str());
        }
        else
          print(result.descr(true).c_str());
      }
    }
  }
}

int Interactive_shell::process_file()
{
  // Default return value will be 1 indicating there were errors
  bool ret_val = 1;

  if (_options.run_file.empty())
    _shell->print_error("Usage: \\. <filename> | \\source <filename>");
  else
    //TODO: do path expansion (in case ~ is used in linux)
  {
    std::ifstream s(_options.run_file.c_str());

    if (!s.fail())
    {
      // The return value now depends on the stream processing
      ret_val = _shell->process_stream(s, _options.run_file);

      // When force is used, we do not care of the processing
      // errors
      if (_options.force)
        ret_val = 0;

      s.close();
    }
    else
    {
      // TODO: add a log entry once logging is
      _shell->print_error((boost::format("Failed to open file '%s', error: %d") % _options.run_file % errno).str());
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
    while (!stream.eof())
    {
      std::string line;

      std::getline(stream, line);

      if (_options.full_interactive)
      {
        print(prompt());
        println(line);
      }

      process_line(line);
    }

    // Being interactive, we do not care about the return value
    return 0;
  }
  else
    return _shell->process_stream(stream, source);
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
        shcore::print(message + "\n");
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
  std::string welcome_msg("Welcome to MySQLx Shell ");
  welcome_msg += MYSH_VERSION;
  println(welcome_msg);
  println("");
  println("Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.");
  println("");
  println("Oracle is a registered trademark of Oracle Corporation and/or its");
  println("affiliates. Other names may be trademarks of their respective");
  println("owners.");
  println("");
  println("Type '\\help', '\\h' or '\\?' for help.");
  println("");
}

void Interactive_shell::print_cmd_line_helper()
{
  println("MySQLx Shell 0.0.1");
  println("");
  println("Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.");
  println("");
  println("Oracle is a registered trademark of Oracle Corporation and/or its");
  println("affiliates. Other names may be trademarks of their respective");
  println("owners.");
  println("");
  println("Usage: mysqlx [OPTIONS]");
  println("  --help                   Display this help and exit.");
  println("  -f, --file=file          Process file.");
  println("  --uri                    Connect to Uniform Resource Identifier.");
  println("                           Format: [user[:pass]]@host[:port][/db]");
  println("                           or user[:pass]@::socket[/db] .");
  println("  -h, --host=name          Connect to host.");
  println("  -P, --port=#             Port number to use for connection.");
  println("  -u, --dbuser=name        User for the connection to the server.");
  println("  --user=name              An alias for dbuser.");
  println("  --dbpassword=name        Password to use when connecting to server");
  println("  --password=name          An alias for dbpassword.");
  println("  -p                       Request password prompt to set the password");
  println("  -D --schema=name         Schema to use.");
  println("  --database=name          An alias for schema.");
  println("  --session-type=name      Type of session to be created. Either app, node or classic.");
  println("  --sql                    Start in SQL mode.");
  println("  --js                     Start in JavaScript mode.");
  println("  --py                     Start in Python mode.");
  println("  --json                   Produce output in JSON format.");
  println("  --table                  Produce output in table format (default for interactive mode).");
  println("                           This option can be used to force that format when running in batch mode.");
  println("  -i, --interactive[=full] To use in batch mode, it forces emulation of interactive mode processing.");
  println("                           Each line on the batch is processed as if it were in interactive mode.");
  println("  --force                  To use in SQL batch mode, forces processing to continue if an error is found.");
  println("  --log-level=value        The log level. Value is an int in the range [1,8], default (1).");
  println("  --version                Prints the version of MySQL X Shell.");

  println("");
}

// Detects whether the shell will be running in interactive mode or not
// Non interactive mode is used when:
// - A file is processed using the --file option
// - A file is processed through the OS redirection mechanism
//
// Interactive mode is used when:
// - A file is processed using the --interactive option
// - No file is processed
// - The STDIN is opened by caller process
//
// An error occurs when both --file and STDIN redirection are used
std::string detect_interactive(Shell_command_line_options &options, bool &from_stdin)
{
  bool is_interactive = true;
  bool from_file = false;
  std::string error;

  from_stdin = false;

  int __stdin_fileno;
  int __stdout_fileno;

#if defined(WIN32)
  __stdin_fileno = _fileno(stdin);
  __stdout_fileno = _fileno(stdout);
#else
  __stdin_fileno = STDIN_FILENO;
  __stdout_fileno = STDOUT_FILENO;
#endif
  if (!isatty(__stdin_fileno) || !isatty(__stdout_fileno))
  {
    // Here we know the input comes from stdin
    from_stdin = true;

    // Now we find out if it is a redirected file or not
    struct stat stats;
    int result = fstat(__stdin_fileno, &stats);

    if (result == 0)
      from_file = (stats.st_mode & S_IFREG) == S_IFREG;

    // Can't process both redirected file and file from parameter.
    if (from_file && !options.run_file.empty())
      error = "--file (-f) option is forbidden when redirecting a file to stdin.";
    else
      is_interactive = false;
  }
  else
    is_interactive = options.run_file.empty();

  // The --interactive option forces the shell to work emulating the
  // interactive mode no matter if:
  // - Input is being redirected from file
  // - Input is being redirected from STDIN
  // - It is not running on a terminal
  if (options.interactive)
    is_interactive = true;

  options.interactive = is_interactive;

  return error;
}

int main(int argc, char **argv)
{
  int ret_val = 0;

  Shell_command_line_options options(argc, argv);

  if (options.exit_code != 0)
    return options.exit_code;

#ifdef HAVE_V8
  extern void JScript_context_init();

  JScript_context_init();
#endif

  {
    bool from_stdin = false;
    std::string error = detect_interactive(options, from_stdin);

    Interactive_shell shell(options);

    if (!error.empty())
    {
      shell.print_error(error);
      ret_val = 1;
    }
    else if (options.print_version)
    {
      std::string version_msg("MySQL X Shell Version ");
      version_msg += MYSH_VERSION;
      shell.print(version_msg);
      ret_val = options.exit_code;
    }
    else if (options.print_cmd_line_helper)
    {
      shell.print_cmd_line_helper();
      ret_val = options.exit_code;
    }
    else
    {
      // Performs the connection
      if (!options.uri.empty())
      {
        try
        {
          if (!shell.connect())
            return 1;
        }
        catch (std::exception &e)
        {
          shell.print_error(e.what());
          return 1;
        }
      }

      if (from_stdin)
        ret_val = shell.process_stream(std::cin, "STDIN");
      else if (!options.run_file.empty())
        ret_val = shell.process_file();
      else if (options.interactive)
      {
        shell.print_banner();
        shell.command_loop();
        ret_val = 0;
      }
    }
  }

  return ret_val;
}