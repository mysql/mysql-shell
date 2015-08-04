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

#ifdef HAVE_PYTHON
extern "C" void Python_context_init();
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

#include "cmdline_options.h"

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
  Interactive_shell(Shell_core::Mode initial_mode, ngcommon::Logger::LOG_LEVEL log_level = ngcommon::Logger::LOG_NONE);
  void command_loop();
  int process_stream(std::istream & stream, const std::string& source);
  int process_file(const char *filename);

  void init_environment();
  void init_scripts(Shell_core::Mode mode);

  void cmd_process_file(const std::vector<std::string>& params);
  bool connect(const std::string &uri, bool needs_password, bool interactive);

  void print(const std::string &str);
  void println(const std::string &str);
  void print_error(const std::string &error);

  void cmd_print_shell_help(const std::vector<std::string>& args);
  void cmd_start_multiline(const std::vector<std::string>& args);
  void cmd_connect(const std::vector<std::string>& args);
  void cmd_quit(const std::vector<std::string>& args);
  void cmd_warnings(const std::vector<std::string>& args);
  void cmd_nowarnings(const std::vector<std::string>& args);

  void print_banner();
  void print_cmd_line_helper();

  /* Processing Options */
  void set_force(bool value) { _batch_continue_on_error = value; }
  void set_output_format(const std::string& format);
  void set_interactive(bool value);

  void set_log_level(ngcommon::Logger::LOG_LEVEL level) { if (_logger) _logger->set_log_level(level); }

private:
  static char *readline(const char *prompt);
  void process_line(const std::string &line);
  void process_result(shcore::Value result);
  std::string prompt();
  ngcommon::Logger* _logger;

  void switch_shell_mode(Shell_core::Mode mode, const std::vector<std::string> &args);

private:
  shcore::Value connect_session(const shcore::Argument_list &args);

private:
  static void deleg_print(void *self, const char *text);
  static void deleg_print_error(void *self, const char *text);
  static bool deleg_input(void *self, const char *text, std::string &ret);
  static bool deleg_password(void *self, const char *text, std::string &ret);
  static void deleg_source(void *self, const char *module);

  bool do_shell_command(const std::string &command);
private:
  Interpreter_delegate _delegate;

  boost::shared_ptr<mysh::BaseSession> _session;
  //boost::shared_ptr<mysh::mysqlx::Schema> _db;
  boost::shared_ptr<Shell_core> _shell;

  std::string _input_buffer;
  bool _multiline_mode;
  bool _show_warnings;
  bool _interactive;
  bool _batch_continue_on_error;
  std::string _output_format;

  Shell_command_handler _shell_command_handler;
};

Interactive_shell::Interactive_shell(Shell_core::Mode initial_mode, ngcommon::Logger::LOG_LEVEL log_level) :
_batch_continue_on_error(false), _show_warnings(false)
{
  std::string log_path = get_user_config_path();
  log_path += "mysqlx.log";
  ngcommon::Logger::create_instance(log_path.c_str(), false, log_level);
  _logger = ngcommon::Logger::singleton();

#ifndef WIN32
  rl_initialize();
#endif
  //  using_history();

  _multiline_mode = false;
  _interactive = false;
  _output_format = "normal";

  _delegate.user_data = this;
  _delegate.print = &Interactive_shell::deleg_print;
  _delegate.print_error = &Interactive_shell::deleg_print_error;
  _delegate.input = &Interactive_shell::deleg_input;
  _delegate.password = &Interactive_shell::deleg_password;
  _delegate.source = &Interactive_shell::deleg_source;

  _shell.reset(new Shell_core(&_delegate));

  //_session.reset(new mysh::mysqlx::Session(dynamic_cast<shcore::IShell_core*>(_shell.get())));
  //_shell->set_global("session", Value(boost::static_pointer_cast<Object_bridge>(_session)));

  //  _db.reset(new mysh::Schema(_shell.get()));
  //  _shell->set_global("db", Value( boost::static_pointer_cast<Object_bridge, mysh::Schema >(_db) ));

  std::string cmd_help =
    "SYNTAX:\n"
    "   \\connect <URI>\n\n"
    "WHERE:\n"
    "   URI is in the format of: [PROTOCOL://][user[:password]@]hostname[:port]\n"
    "   PROTOCOL can be:\n"
    "     mysql   for traditional MySQL connections.\n"
    "     mysqlx  for SQL and NoSQL access in MySQL X compliant servers (MySQL 5.7+).\n\n"
    "EXAMPLE:\n"
    "   \\connect mysqlx://root@localhost:3306\n\n"
    "NOTE: The mysql protocol will be used as default if mysqlx is not specified.";

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
  SET_SHELL_COMMAND("\\connect", "Connect to server.", cmd_help, Interactive_shell::cmd_connect);
  SET_SHELL_COMMAND("\\warnings|\\W", "Show warnings after every statement..", cmd_help, Interactive_shell::cmd_warnings);
  SET_SHELL_COMMAND("\\nowarnings|\\w", "Don't show warnings after every statement..", cmd_help, Interactive_shell::cmd_nowarnings);

  bool lang_initialized;
  _shell->switch_mode(initial_mode, lang_initialized);

  if (lang_initialized)
    init_scripts(initial_mode);
}

void Interactive_shell::cmd_process_file(const std::vector<std::string>& params)
{
  std::string filename = boost::join(params, " ");

  Interactive_shell::process_file(filename.c_str());
}

bool Interactive_shell::connect(const std::string &uri, bool needs_password, bool interactive)
{
  Argument_list args;

  std::string protocol;
  std::string user;
  std::string pass;
  std::string host;
  int port = 3306;
  std::string sock;
  std::string db;
  int pwd_found;

  if (!mysh::parse_mysql_connstring(uri, protocol, user, pass, host, port, sock, db, pwd_found))
    throw shcore::Exception::argument_error("Could not parse URI for MySQL connection");
  else
  {
    if (!pwd_found && needs_password) {
      char *tmp = mysh_get_tty_password("Enter password: ");
      if (tmp)
      {
        pass.append(tmp);
        free(tmp);
        args.push_back(Value(uri));
        args.push_back(Value(pass));
      }
    }
    else
      args.push_back(Value(uri));
  }

  try
  {
    if (_session && _session->is_connected())
    {
      if (interactive)
        shcore::print("Closing old connection...\n");

      _session->close(shcore::Argument_list());
    }

    // strip password from uri
    if (interactive)
    {
      std::string uri_stripped = mysh::strip_password(uri);
      shcore::print("Connecting to " + uri_stripped + "...\n");
    }

    connect_session(args);
  }
  catch (std::exception &exc)
  {
    print_error(exc.what());
    return false;
  }

  return true;
}

Value Interactive_shell::connect_session(const Argument_list &args)
{
  // TODO: Review if this replacement is correct
  boost::shared_ptr<mysh::BaseSession> new_session(mysh::connect_session(args));
  _session.reset(new_session, new_session.get());

  _shell->set_global("session", Value(boost::static_pointer_cast<Object_bridge>(_session)));

  Value default_schema = _session->get_member("defaultSchema");
  if (default_schema)
  {
    // TODO: Uncomment this...
    //if (_shell->interactive_mode() != Shell_core::Mode_SQL)
    //  _shell->print("Default schema `" + db->schema() + "` accessible through db.\n");
    _shell->set_global("db", default_schema);
  }
  else
  {
    // XXX assign a dummy placeholder to db
    if (_shell->is_interactive() && _shell->interactive_mode() != Shell_core::Mode_SQL)
    {
      _shell->print("No default schema selected.\n");
    }
  }

  return Value::Null();
}

void Interactive_shell::init_environment()
{
  _shell->set_global("connect",
    Value(Cpp_function::create("connect",
    boost::bind(&Interactive_shell::connect_session, this, _1),
    "connection_string", String,
    NULL)));
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
      process_file((*i).c_str());
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

  if (_output_format.find("json") == 0)
  {
    Value::Map_type_ref error_map(new Value::Map_type());

    Value error_obj(error_map);

    (*error_map)["error"] = error_val;

    message = error_obj.json(_output_format == "jsonpretty");
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
    connect(args[0], true, true);
  }
  else
    print_error("\\connect <uri>");
}

void Interactive_shell::cmd_quit(const std::vector<std::string>& UNUSED(args))
{
  _interactive = false;
}

void Interactive_shell::cmd_warnings(const std::vector<std::string>& UNUSED(args))
{
  _show_warnings = true;

  _shell->set_show_warnings(_show_warnings);

  println("Show warnings enabled.");
}

void Interactive_shell::cmd_nowarnings(const std::vector<std::string>& UNUSED(args))
{
  _show_warnings = false;

  _shell->set_show_warnings(_show_warnings);

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

  self->process_file(module);
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

void Interactive_shell::set_output_format(const std::string& format)
{
  _output_format = format;
  _shell->set_output_format(format);
}

void Interactive_shell::set_interactive(bool value)
{
  _interactive = value;
  _shell->set_interactive(value);
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
  if (result)
  {
    Value dump_function;
    if (result.type == shcore::Object)
    {
      boost::shared_ptr<Object_bridge> object = result.as_object();
      if (object && object->has_member("__paged_output__"))
        dump_function = object->get_member("__paged_output__");

      if (dump_function)
      {
        Argument_list args;
        args.push_back(Value(_interactive));
        args.push_back(Value(_output_format));
        args.push_back(Value(_show_warnings));
        object->call("__paged_output__", args);
      }
    }

    // If the function is not found the values still needs to be printed
    if (!dump_function)
    {
      if (_output_format == "jsonraw" || _output_format == "jsonpretty")
      {
        shcore::JSON_dumper dumper(_output_format == "jsonpretty");
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

int Interactive_shell::process_file(const char *filename)
{
  // Default return value will be 1 indicating there were errors
  bool ret_val = 1;

  if (!filename)
    _shell->print_error("Usage: \\. <filename> | \\source <filename>");
  else
    //TODO: do path expansion (in case ~ is used in linux)
  {
    std::ifstream s(filename);

    if (!s.fail())
    {
      // The return value now depends on the stream processing
      ret_val = _shell->process_stream(s, filename, _batch_continue_on_error);

      // When force is used, we do not care of the processing
      // errors
      if (_batch_continue_on_error)
        ret_val = 0;

      s.close();
    }
    else
    {
      // TODO: add a log entry once logging is
      _shell->print_error((boost::format("Failed to open file '%s', error: %d") % filename % errno).str());
    }
  }

  return ret_val;
}

int Interactive_shell::process_stream(std::istream & stream, const std::string& source)
{
  // If interactive is set, it means that the shell was started with the option to
  // Emulate interactive mode while processing the stream
  if (_interactive)
  {
    while (!stream.eof())
    {
      std::string line;

      std::getline(stream, line);

      print(prompt());

      println(line);

      process_line(line);
    }

    // Being interactive, we do not care about the return value
    return 0;
  }
  else
    return _shell->process_stream(stream, source, _batch_continue_on_error);
}

void Interactive_shell::command_loop()
{
  if (_interactive) // check if interactive
  {
    switch (_shell->interactive_mode())
    {
      case Shell_core::Mode_SQL:
#ifdef HAVE_V8
        _shell->print("Currently in SQL mode. Use \\js or \\py to switch the shell to a scripting language.\n");
#else
        _shell->print("Currently in SQL mode. Use \\py to switch the shell to python scripting.\n");
#endif
        break;
      case Shell_core::Mode_JScript:
        _shell->print("Currently in JavaScript mode. Use \\sql to switch to SQL mode and execute queries.\n");
        break;
      case Shell_core::Mode_Python:
        _shell->print("Currently in Python mode. Use \\sql to switch to SQL mode and execute queries.\n");
        break;
      default:
        break;
    }
  }

  while (_interactive)
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
  println("Welcome to MySQLx Shell 0.0.1");
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
  println("  --help                 Display this help and exit.");
  println("  -f, --file=file        Process file.");
  println("  --uri                  Connect to Uniform Resource Identifier.");
  println("                         Format: [protocol://][user[:pass]]@host[:port][/db]");
  println("                         or user[:pass]@::socket[/db] .");
  println("  -h, --host=name        Connect to host.");
  println("  -P, --port=#           Port number to use for connection.");
  println("  -u, --user=name        User for the connection to the server.");
  println("  -p, --password[=name]  Password to use when connecting to server");
  println("                         If password is not given it's asked from the tty.");
  println("  --sql                  Start in SQL mode.");
  println("  --js                   Start in JavaScript mode.");
  println("  --py                   Start in Python mode.");
  println("  --json                 Produce output in JSON format.");
  println("  --table                Produce output in table format (default for interactive mode).");
  println("                         This option can be used to force that format when running in batch mode.");
  println("  -i, --interactive      To use in batch mode, it forces emulation of interactive mode processing.");
  println("                         Each line on the batch is processed as if it were in interactive mode.");
  println("  --force                To use in SQL batch mode, forces processing to continue if an error is found.");
  println("  --log-level=value      The log level. Value is an int in the range [1,8], default (1).");

  println("");
}

class Shell_command_line_options : public Command_line_options
{
public:
  Shell_core::Mode initial_mode;
  std::string run_file;

  std::string uri;
  std::string password;
  std::string output_format;
  bool print_cmd_line_helper;
  bool force;
  bool interactive;
  ngcommon::Logger::LOG_LEVEL log_level;

  Shell_command_line_options(int argc, char **argv)
    : Command_line_options(argc, argv), log_level(ngcommon::Logger::LOG_ERROR)
  {
    std::string host;
    std::string user;
    int port = 0;
    bool needs_password_;
    char* log_level_value;

    needs_password_ = false;
    print_cmd_line_helper = false;

    initial_mode = Shell_core::Mode_SQL;
    force = false;
    interactive = false;

    for (int i = 1; i < argc && exit_code == 0; i++)
    {
      char *value;
      if (check_arg_with_value(argv, i, "--file", "-f", value))
        run_file = value;
      else if (check_arg_with_value(argv, i, "--uri", NULL, value))
        uri = value;
      else if (check_arg_with_value(argv, i, "--host", "-h", value))
        host = value;
      else if (check_arg_with_value(argv, i, "--user", "-u", value))
        user = value;
      else if (check_arg_with_value(argv, i, "--port", "-P", value))
        port = atoi(value);
      else if (check_arg(argv, i, "-p", "-p"))
        needs_password_ = true;
      else if (check_arg_with_value(argv, i, "--password", "-p", value))
        password = value;
      else if (check_arg(argv, i, "--sql", "--sql"))
        initial_mode = Shell_core::Mode_SQL;
      else if (check_arg(argv, i, "--js", "--js"))
        initial_mode = Shell_core::Mode_JScript;
      else if (check_arg_with_value(argv, i, "--json", NULL, value, "raw"))
      {
        if (strcmp(value, "raw") != 0 && strcmp(value, "pretty") != 0)
        {
          std::cerr << "Value for --json must be either pretty or raw.\n";
          exit_code = 1;
          break;
        }

        output_format = "json";
        output_format.append(value);
      }
      else if (check_arg(argv, i, "--table", "--table"))
        output_format = "table";
      else if (check_arg(argv, i, "--py", "--py"))
        initial_mode = Shell_core::Mode_Python;
      else if (check_arg(argv, i, "--help", "--help"))
      {
        print_cmd_line_helper = true;
        exit_code = 0;
      }
      else if (check_arg(argv, i, "--force", "--force"))
        force = true;
      else if (check_arg(argv, i, "--interactive", "-i"))
        interactive = true;
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

    if (needs_password_)
    {
      char *tmp = mysh_get_tty_password("Enter password: ");
      if (tmp)
      {
        password = tmp;
        free(tmp);
      }
    }

    if (uri.empty())
    {
      uri = user;
      if (!uri.empty()) {
        if (!password.empty()) {
          uri.append(":").append(password);
        }
        uri.append("@");
      }
      uri.append(host);
      if (port > 0)
        uri.append((boost::format(":%i") % port).str());
    }
  }
};

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

#ifdef HAVE_PYTHON
  Python_context_init();
#endif

  {
    Interactive_shell shell(options.initial_mode, options.log_level);

    shell.set_force(options.force);
    shell.set_output_format(options.output_format);

    if (options.print_cmd_line_helper)
    {
      shell.print_cmd_line_helper();
      return options.exit_code;
    }

    bool is_interactive = true;
    bool from_stdin = false;
    bool from_file = false;

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
      {
        shell.print_error("--file (-f) option is forbidden when redirecting a file to stdin.");
        return 1;
      }
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

    shell.set_interactive(is_interactive);

    shell.init_environment();

    if (!options.uri.empty())
    {
      try
      {
        if (!shell.connect(options.uri, options.needs_password, is_interactive))
          return 1;
      }
      catch (std::exception &e)
      {
        shell.print_error(e.what());
        return 1;
      }
    }

    // Three processing modes are available at this point
    // Interactive, file processing and STDIN processing
    if (from_stdin)
      ret_val = shell.process_stream(std::cin, "STDIN");
    else if (!options.run_file.empty())
      ret_val = shell.process_file(options.run_file.c_str());
    else if (is_interactive)
    {
      shell.print_banner();
      shell.command_loop();
    }
  }

  return ret_val;
}