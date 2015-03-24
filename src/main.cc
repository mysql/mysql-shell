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

#ifndef WIN32
#  include "editline/readline.h"
#endif

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/pointer_cast.hpp>
#include <iostream>

#include "shellcore/types_cpp.h"

#include "shellcore/lang_base.h"
#include "shellcore/shell_core.h"

#include "cmdline_options.h"

#include "../modules/mod_session.h"
#include "../modules/mod_db.h"

#ifdef WIN32
#  include <m_ctype.h>
#  include <my_sys.h>
#  include <io.h>
#  define isatty _isatty
#endif


using namespace shcore;

const int MAX_READLINE_BUF = 65536;

extern char *mysh_get_tty_password(const char *opt_message);

class Interactive_shell
{
public:
  Interactive_shell(Shell_core::Mode initial_mode);
  void command_loop();
  int process_stream(std::istream & stream, const std::string& source) { return _shell->process_stream(stream, source);  }

  void init_environment();

  bool connect(const std::string &uri, const std::string &password);

  void print(const std::string &str);
  void println(const std::string &str);
  void print_error(const std::string &error);

  void cmd_print_shell_help(const std::vector<std::string>& args);
  void cmd_start_multiline(const std::vector<std::string>& args);
  void cmd_connect(const std::vector<std::string>& args);
  void cmd_quit(const std::vector<std::string>& args);

  void print_banner();

private:
  static char *readline(const char *prompt);
  void process_line(const std::string &line);
  std::string prompt();

  void switch_shell_mode(Shell_core::Mode mode, const std::vector<std::string> &args);

private:
  shcore::Value connect_session(const shcore::Argument_list &args);

private:
  static void deleg_print(void *self, const char *text);
  static void deleg_print_error(void *self, const char *text);
  static bool deleg_input(void *self, const char *text, std::string &ret);
  static bool deleg_password(void *self, const char *text, std::string &ret);

  bool do_shell_command(const std::string &command);
private:
  Interpreter_delegate _delegate;

  boost::shared_ptr<mysh::Session> _session;
  boost::shared_ptr<mysh::Db> _db;
  boost::shared_ptr<Shell_core> _shell;

  std::string _input_buffer;
  bool _multiline_mode;
  bool _interactive;

  Shell_command_handler _shell_command_handler;
};



Interactive_shell::Interactive_shell(Shell_core::Mode initial_mode)
{
#ifndef WIN32
  rl_initialize();
#endif  
//  using_history();

  _multiline_mode = false;
  _interactive - false;

  _delegate.user_data = this;
  _delegate.print = &Interactive_shell::deleg_print;
  _delegate.print_error = &Interactive_shell::deleg_print_error;
  _delegate.input = &Interactive_shell::deleg_input;
  _delegate.password = &Interactive_shell::deleg_password;

  _shell.reset(new Shell_core(&_delegate));

  _session.reset(new mysh::Session(_shell.get()));
  _shell->set_global("_S", Value(boost::static_pointer_cast<Object_bridge>(_session)));

//  _db.reset(new mysh::Db(_shell.get()));
//  _shell->set_global("db", Value( boost::static_pointer_cast<Object_bridge, mysh::Db >(_db) ));

  
  SET_SHELL_COMMAND("\\help|\\?|\\h", "Print this help.", "", Interactive_shell::cmd_print_shell_help);
  SET_CUSTOM_SHELL_COMMAND("\\sql", "Sets shell on SQL processing mode.", "", boost::bind(&Interactive_shell::switch_shell_mode, this, Shell_core::Mode_SQL, _1));
  SET_CUSTOM_SHELL_COMMAND("\\js", "Sets shell on JavaScript processing mode.", "", boost::bind(&Interactive_shell::switch_shell_mode, this, Shell_core::Mode_JScript, _1));
  SET_CUSTOM_SHELL_COMMAND("\\py", "Sets shell on Python processing mode.", "", boost::bind(&Interactive_shell::switch_shell_mode, this, Shell_core::Mode_Python, _1));
  SET_SHELL_COMMAND("\\", "Start multiline input. Finish and execute with an empty line.", "", Interactive_shell::cmd_start_multiline);
  SET_SHELL_COMMAND("\\quit|\\q|\\exit", "Quit mysh.", "", Interactive_shell::cmd_quit);
  std::string cmd_help =
    "SYNTAX:\n"
    "   \\connect <uri>\n\n"
    "EXAMPLE:\n"
    "   \\connect root@localhost:3306\n";
  SET_SHELL_COMMAND("\\connect", "Connect to server.", cmd_help, Interactive_shell::cmd_connect);

  _shell->switch_mode(initial_mode);
}


bool Interactive_shell::connect(const std::string &uri, const std::string &password)
{
  Argument_list args;

  args.push_back(Value(uri));
  if (!password.empty())
    args.push_back(Value(password));

  try
  {
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
  _session->connect(args);

  boost::shared_ptr<mysh::Db> db(_session->default_schema());
  if (db)
  {
    if (_shell->interactive_mode() != Shell_core::Mode_SQL)
      _shell->print("Default schema `"+db->schema()+"` accessible through db.\n");
    _shell->set_global("db", Value(boost::static_pointer_cast<Object_bridge>(db)));
  }
  else
  {
    // XXX assign a dummy placeholder to db
    _shell->print("No default schema selected.\n");
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


std::string Interactive_shell::prompt()
{
  if (_multiline_mode)
    return "... ";
  else
    return _shell->prompt();
}


void Interactive_shell::switch_shell_mode(Shell_core::Mode mode, const std::vector<std::string> &args)
{
  Shell_core::Mode old_mode = _shell->interactive_mode();

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
        if (_shell->switch_mode(mode))
          println("Switching to SQL mode... Commands end with ;");
        break;
      case Shell_core::Mode_JScript:
#ifdef HAVE_V8
        if (_shell->switch_mode(mode))
          println("Switching to JavaScript mode...");
#else
        println("JavaScript mode is not supported on this platform, command ignored.");
#endif
        break;
      case Shell_core::Mode_Python:
        if (_shell->switch_mode(mode))
          println("Switching to Python mode...");
        break;
    }
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
  std::cerr << "ERROR: " << error << "\n";
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
    _shell_command_handler.print_commands("Global Commands.");

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
    connect(args[0], "");
  }
  else
    print_error("\\connect <uri>");
}

void Interactive_shell::cmd_quit(const std::vector<std::string>& args)
{
  _interactive = false;
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
  tmp = (char *)malloc(MAX_READLINE_BUF);
  if (!tmp)
    throw Exception::runtime_error("Cannot allocate memory for Interactive_shell::readline buffer.");
  my_win_console_fputs(&my_charset_latin1, prompt);
  tmp = my_win_console_readline(&my_charset_latin1, tmp, MAX_READLINE_BUF);
#endif
  return tmp;
}


bool Interactive_shell::deleg_input(void *cdata, const char *prompt, std::string &ret)
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
  char *tmp = mysh_get_tty_password(prompt);
  if (!tmp)
    return false;
  ret = tmp;
  free(tmp);
  return true;
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
        Value result = _shell->handle_input(_input_buffer, state);

        if (result)
        {
          boost::shared_ptr<Object_bridge> object = result.as_object();
          Value dump_function;
          if (object)
            dump_function = object->get_member("__paged_output__");

          if (dump_function)
            object->call("__paged_output__", Argument_list());
          else
            this->print(result.descr(true).c_str());
        }

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


void Interactive_shell::command_loop()
{
  _interactive = true;

  if (isatty(0)) // check if interactive
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
  println("Welcome to MySQL Shell 0.0.1");
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


class Shell_command_line_options : public Command_line_options
{
public:
  Shell_core::Mode initial_mode;
  std::string run_file;

  std::string uri;
  std::string password;

  Shell_command_line_options(int argc, char **argv)
          : Command_line_options(argc, argv)
  {
    std::string host;
    std::string user;
    int port = 0;
    bool needs_password;

    needs_password = false;

    initial_mode = Shell_core::Mode_SQL;

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
      else if (check_arg(argv, i, "--password", "-p"))
        needs_password = true;
      else if (check_arg(argv, i, "--sql", "--sql"))
        initial_mode = Shell_core::Mode_SQL;
      else if (check_arg(argv, i, "--js", "--js"))
        initial_mode = Shell_core::Mode_JScript;
      else if (check_arg(argv, i, "--py", "--py"))
        initial_mode = Shell_core::Mode_Python;
      else if (exit_code == 0)
      {
        std::cerr << argv[0] << ": unknown option " << argv[i] <<"\n";
        exit_code = 1;
        break;
      }
    }

    if (needs_password)
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
      if (!uri.empty())
        uri.append("@");
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

  {
    Interactive_shell shell(options.initial_mode);

    bool is_interactive = true;

#if defined(WIN32)
    if(!isatty(_fileno(stdin) || !isatty(_fileno(stdout))))
#else
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
#endif
    {
      if (!options.run_file.empty())
      {
        shell.print_error("--file (-f) option is forbidden when redirecting input to stdin.");
        return 1;
      }
      else
        is_interactive = false;
    }
    else
      is_interactive = options.run_file.empty();

    shell.init_environment();

    if (!options.uri.empty())
    {
      if (!shell.connect(options.uri, options.password))
        return 1;
      shell.println("");
    }

    // Three processing modes are available at this point
    // Interactive, file processing and STDIN processing
    if (is_interactive)
    {
      shell.print_banner();
      shell.command_loop();
    }
    else if (!options.run_file.empty())
    {
      std::ifstream s(options.run_file.c_str());
      if (!s.fail())
      {
        ret_val = shell.process_stream(s, options.run_file);
        s.close();
      }
      else
      {
        ret_val = 1;
        shell.print_error((boost::format("Failed to open file '%s', error: %d") % options.run_file.c_str() % errno).str());
      }
    }
    else
      ret_val = shell.process_stream(std::cin, "STDIN");
  }

  return ret_val;
}
