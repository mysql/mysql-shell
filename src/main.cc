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

#include "editline/readline.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>

#include "shellcore/types_cpp.h"

#include "shellcore/lang_base.h"
#include "shellcore/shell_core.h"

#include "../modules/mod_db.h"


using namespace shcore;


class Interactive_shell
{
public:
  Interactive_shell();
  void command_loop();
  int run_script(const std::string &file);

  void init_environment();

private:
  void process_line(const std::string &line);
  std::string prompt();

  void print(const std::string &str);
  void println(const std::string &str);
  void print_error(const std::string &error);
  void print_shell_help();

  void switch_shell_mode(Shell_core::Mode mode, const std::vector<std::string> &args);

  void print_banner();

private:
  shcore::Value connect_db(const shcore::Argument_list &args);

private:
  static void deleg_print(void *self, const char *text);
  static std::string deleg_input(void *self, const char *text);
  static std::string deleg_password(void *self, const char *text);

  void do_shell_command(const std::string &command);
private:
  Interpreter_delegate _delegate;

  boost::shared_ptr<mysh::Db> _db;
  boost::shared_ptr<Shell_core> _shell;

  std::string _input_buffer;
  bool _multiline_mode;
};



Interactive_shell::Interactive_shell()
{
  rl_initialize();
//  using_history();

  _multiline_mode = false;

  _delegate.user_data = this;
  _delegate.print = &Interactive_shell::deleg_print;
  _delegate.input = &Interactive_shell::deleg_input;
  _delegate.password = &Interactive_shell::deleg_password;

  _shell.reset(new Shell_core(&_delegate));

  _db.reset(new mysh::Db(_shell.get()));
  _shell->set_global("db", Value(_db));

  switch_shell_mode(Shell_core::Mode_JScript, std::vector<std::string>());
}


Value Interactive_shell::connect_db(const Argument_list &args)
{
  _db->connect(args);

  return Value::Null();
}


void Interactive_shell::init_environment()
{
  _shell->set_global("connect",
                     Value(Cpp_function::create("connect",
                                                boost::bind(&Interactive_shell::connect_db, this, _1),
                                                "connection_string", String,
                                                NULL)));
}


std::string Interactive_shell::prompt()
{
  std::string prefix;
  std::string suffix;
  switch (_shell->interactive_mode())
  {
  case Shell_core::Mode_None:
    break;
  case Shell_core::Mode_SQL:
    prefix = "mysql";
    suffix = "> ";
    break;
  case Shell_core::Mode_JScript:
    prefix = "myjs";
    suffix = "> ";
    break;
  case Shell_core::Mode_Python:
    prefix = "mypy";
    suffix = ">>> ";
    break;
  }

  if (_multiline_mode)
    suffix = "... ";

  return prefix + suffix;
}


void Interactive_shell::switch_shell_mode(Shell_core::Mode mode, const std::vector<std::string> &args)
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


std::string Interactive_shell::deleg_input(void *cdata, const char *prompt)
{
  std::string s;
  char *tmp = readline(prompt);
  if (!tmp)
    return "";
  s = tmp;
  free(tmp);
  return s;
}


std::string Interactive_shell::deleg_password(void *cdata, const char *prompt)
{
  std::string s;
  //XXX disable echoing
  char *tmp = readline(prompt);
  if (!tmp)
    return "";
  s = tmp;
  free(tmp);
  return s;
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
  else if (tokens.front().compare("\\help") == 0 || tokens.front().compare("\\?") == 0)
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
    do_shell_command(line);
  }
  else
  {
    if (_multiline_mode)
    {
      if (line.empty())
        _multiline_mode = false;
      else
      {
        if (_input_buffer.empty())
          _input_buffer = line;
        else
          _input_buffer.append("\n").append(line);
      }
    }
    else
      _input_buffer = line;
    
    if (!_multiline_mode)
    {
      if (_shell->handle_interactive_input(_input_buffer) == Input_ok)
      {
        add_history(_input_buffer.c_str());
        println("");
      }
      _input_buffer.clear();
    }
  }
}


void Interactive_shell::command_loop()
{
  print_banner();

  for (;;)
  {
    char *cmd = readline(prompt().c_str());
    if (!cmd || !cmd[0] /* workaround for Eclipse CDT not picking EOFs */ )
      break;

    try
    {
      process_line(cmd);
    }
    catch (std::exception &exc)
    {
      print_error(exc.what());
    }
    free(cmd);
  }
  std::cout << "Bye!\n";
}


void Interactive_shell::print_banner()
{
  println("Welcome to MySQL Shell 0.0.1");
  println("");
  println("Copyright (c) 2014, 2015 Oracle and/or its affiliates. All rights reserved.");
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

  std::string host;
  std::string user;
  int port;

  Command_line_options(int argc, char **argv)
  {
    exit_code = 0;

    initial_mode = Shell_core::Mode_JScript;

    for (int i = 1; i < argc && exit_code == 0; i++)
    {
      char *value;
      if (check_arg_with_value(argv, i, "--file", "-f", value))
        run_file = value;
      else if (check_arg_with_value(argv, i, "--host", "-h", value))
        host = value;
      else if (check_arg_with_value(argv, i, "--user", "-u", value))
        user = value;
      else if (check_arg_with_value(argv, i, "--port", "-p", value))
        port = atoi(value);
      else if (exit_code == 0)
      {
        std::cerr << argv[0] << ": unknown option " << argv[i] <<"\n";
        exit_code = 1;
        break;
      }
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
    if (strcmp(argv[argi], arg) == 0 || strcmp(argv[argi], larg) == 0)
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
    else if (strncmp(argv[argi], larg, strlen(larg)) == 0 && strlen(argv[argi]) > strlen(larg))
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
    Interactive_shell shell;

    shell.init_environment();

    if (!options.run_file.empty())
      shell.run_script(options.run_file);
    else
      shell.command_loop();
  }

  return 0;
}
