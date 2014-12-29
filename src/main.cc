/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

#include <boost/algorithm/string.hpp>
#include <iostream>

#include "shellcore/lang_base.h"
#include "shellcore/shell_core.h"

using namespace shcore;


class Interactive_shell
{
public:
  Interactive_shell();
  void command_loop();

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
  static void deleg_print(void *self, const char *text);
  static std::string deleg_input(void *self, const char *text);
  static std::string deleg_password(void *self, const char *text);

  void do_shell_command(const std::string &command);
private:
  Interpreter_delegate _delegate;

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

  switch_shell_mode(Shell_core::Mode_JScript, std::vector<std::string>());
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
  char *tmp = readline(prompt);//XXX
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
    if (!cmd)
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
  println("Welcome to MySQL Shell 0.0.0");
  println("");
  println("Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.");
  println("");
  println("Oracle is a registered trademark of Oracle Corporation and/or its");
  println("affiliates. Other names may be trademarks of their respective");
  println("owners.");
}


int main(int argc, char **argv)
{
  extern void JScript_context_init();

  JScript_context_init();

  {
    Interactive_shell shell;
    shell.command_loop();
  }

  return 0;
}
