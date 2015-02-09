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

extern char *get_tty_password(const char *opt_message);

class Interactive_shell
{
public:
  Interactive_shell(Shell_core::Mode initial_mode);
  void command_loop();
  int run_script(const std::string &file);

  void init_environment();

  bool connect(const std::string &uri, const std::string &password);

  void print(const std::string &str);
  void println(const std::string &str);
  void print_error(const std::string &error);
  void print_shell_help();

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

  void do_shell_command(const std::string &command);
private:
  Interpreter_delegate _delegate;

  boost::shared_ptr<mysh::Session> _session;
  boost::shared_ptr<mysh::Db> _db;
  boost::shared_ptr<Shell_core> _shell;

  std::string _input_buffer;
  bool _multiline_mode;
};



Interactive_shell::Interactive_shell(Shell_core::Mode initial_mode)
{
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

  _shell.reset(new Shell_core(&_delegate));

  _session.reset(new mysh::Session(_shell.get()));
  _shell->set_global("_S", Value(boost::static_pointer_cast<Object_bridge>(_session)));

//  _db.reset(new mysh::Db(_shell.get()));
//  _shell->set_global("db", Value( boost::static_pointer_cast<Object_bridge, mysh::Db >(_db) ));

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
        if (_shell->switch_mode(mode))
          println("Switching to JavaScript mode...");
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


void Interactive_shell::print_shell_help()
{
  println("MySQL Shell Help");
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
  char *tmp = get_tty_password(prompt);
  if (!tmp)
    return false;
  ret = tmp;
  free(tmp);
  return true;
}


void Interactive_shell::do_shell_command(const std::string &line)
{
  std::vector<std::string> tokens;

  boost::algorithm::split(tokens, line, boost::is_any_of(" "), boost::token_compress_on);

  if (tokens.front().compare("\\sql") == 0)
  {
    switch_shell_mode(Shell_core::Mode_SQL, tokens);
  }
  else if (tokens.front().compare("\\js") == 0)
  {
    switch_shell_mode(Shell_core::Mode_JScript, tokens);
  }
  else if (tokens.front().compare("\\py") == 0)
  {
    switch_shell_mode(Shell_core::Mode_Python, tokens);
  }
  else if (tokens.front().compare("\\connect") == 0)
  {
    if (tokens.size() == 2)
    {
      connect(tokens[1], "");
    }
    else
      print_error("\\connect <uri>");
  }
  else if (tokens.front().compare("\\help") == 0 || tokens.front().compare("\\?") == 0 || tokens.front().compare("\\h") == 0)
  {
    print_shell_help();
  }
  else if (line == "\\")
  {
    println("Multi-line input. Finish and execute with an empty line.");
    _multiline_mode = true;
  }
  else
  {
    print_error("Invalid shell command "+tokens.front()+". Try \\help or \\?\n");
  }
}


void Interactive_shell::process_line(const std::string &line)
{
  // check if the line is an escape/shell command
  if (_input_buffer.empty() && !line.empty() && line[0] == '\\' && !_multiline_mode)
  {
    try
    {
      do_shell_command(line);
    }
    catch (std::exception &exc)
    {
      print_error(exc.what());
    }
  }
  else
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
      Interactive_input_state state;
      try
      {
        state = _shell->handle_interactive_input(_input_buffer);

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
  if (isatty(0)) // check if interactive
  {
    switch (_shell->interactive_mode())
    {
      case Shell_core::Mode_SQL:
        _shell->print("Currently in SQL mode. Use \\js or \\py to switch the shell to a scripting language.\n");
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

  for (;;)
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
}


int Interactive_shell::run_script(const std::string &file)
{
  boost::system::error_code err;

  int rc = _shell->run_script(file, err);
  if (err)
  {
    std::cerr << err << "\n";
    return 1;
  }
  return rc;
}


struct Command_line_options
{
  int exit_code;
  Shell_core::Mode initial_mode;
  std::string run_file;

  std::string uri;
  std::string password;

  Command_line_options(int argc, char **argv)
  {
    std::string host;
    std::string user;
    int port = 0;
    bool needs_password;

    exit_code = 0;
    needs_password = false;

    initial_mode = Shell_core::Mode_JScript;

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
      else if (exit_code == 0)
      {
        std::cerr << argv[0] << ": unknown option " << argv[i] <<"\n";
        exit_code = 1;
        break;
      }
    }

    if (needs_password)
    {
      char *tmp = get_tty_password("Enter password: ");
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

  bool check_arg(char **argv, int &argi, const char *arg, const char *larg)
  {
    if (strcmp(argv[argi], arg) == 0 || strcmp(argv[argi], larg) == 0)
      return true;
    return false;
  }

  bool check_arg_with_value(char **argv, int &argi, const char *arg, const char *larg, char *&value)
  {
    // --option value or -o value
    if (strcmp(argv[argi], arg) == 0 || (larg && strcmp(argv[argi], larg) == 0))
    {
      // value must be in next arg
      if (argv[argi+1] != NULL)
      {
        ++argi;
        value = argv[argi];
      }
      else
      {
        std::cerr << argv[0] << ": option " << argv[argi] << " requires an argument\n";
        exit_code = 1;
        return false;
      }
      return true;
    }
    // -ovalue
    else if (larg && strncmp(argv[argi], larg, strlen(larg)) == 0 && strlen(argv[argi]) > strlen(larg))
    {
      value = argv[argi] + strlen(larg);
      return true;
    }
    // --option=value
    else if (strncmp(argv[argi], arg, strlen(arg)) == 0 && argv[argi][strlen(arg)] == '=')
    {
      // value must be after =
      value = argv[argi] + strlen(arg)+1;
      return true;
    }
    return false;
  }
};


int main(int argc, char **argv)
{
  extern void JScript_context_init();

  Command_line_options options(argc, argv);

  if (options.exit_code != 0)
    return options.exit_code;

  JScript_context_init();

  {
    Interactive_shell shell(Shell_core::Mode_SQL);


    // if interactive, print the copyright info
    if (options.run_file.empty())
#if defined(WIN32)
      if(isatty(_fileno(stdin)))
#else
      if (isatty(STDIN_FILENO))
#endif
      shell.print_banner();

    shell.init_environment();

    if (!options.uri.empty())
    {
      if (!shell.connect(options.uri, options.password))
        return 1;
      shell.println("");
    }

    if (!options.run_file.empty())
      shell.run_script(options.run_file);
    else
      shell.command_loop();
  }

  return 0;
}
