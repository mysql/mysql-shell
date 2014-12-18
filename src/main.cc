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

  void print_error(const std::string &error);
  void print_shell_help();

  void switch_shell_mode(Shell_core::Mode mode, const std::vector<std::string> &args);

private:
  boost::shared_ptr<Shell_core> _shell;

  std::string _line_buffer;
};



Interactive_shell::Interactive_shell()
{
  rl_initialize();
  using_history();

  _shell.reset(new Shell_core(Shell_core::Mode_JScript));
}


std::string Interactive_shell::prompt()
{
  std::string prefix;
  std::string suffix;
  switch (_shell->interactive_mode())
  {
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
  return prefix + suffix;
}


void Interactive_shell::switch_shell_mode(Shell_core::Mode mode, const std::vector<std::string> &args)
{
  _line_buffer.clear();

  switch (mode)
  {
  case Shell_core::Mode_SQL:
    if (_shell->switch_mode(mode))
      std::cout << "Switching to SQL mode... Commands end with ; \n";
    break;
  case Shell_core::Mode_JScript:
    if (_shell->switch_mode(mode))
      std::cout << "Switching to JavaScript mode...\n";
    break;
  case Shell_core::Mode_Python:
    if (_shell->switch_mode(mode))
      std::cout << "Switching to Python mode...\n";
    break;
  }
}


void Interactive_shell::print_error(const std::string &error)
{
  std::cerr << "ERROR: " << error << "\n";
}


void Interactive_shell::print_shell_help()
{
  std::cout << "MySQL Shell Help\n";
}


void Interactive_shell::process_line(const std::string &line)
{
  // check if the line is an escape/shell command
  if (_line_buffer.empty() && !line.empty() && line[0] == '\\')
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
    else
    {
      print_error("Invalid shell command "+tokens.front()+". Try \\help or \\?\n");
    }
  }
  else
  {
    if (_line_buffer.empty())
      _line_buffer = line;
    else
      _line_buffer.append("\n").append(line);
    std::string line = _line_buffer;
    if (_shell->handle_interactive_input_line(line))
    {
      add_history(_line_buffer.substr(0, _line_buffer.size()-line.size()).c_str());
    }
    else
    {
      // command is not finished yet, needs more lines
    }
  }
}


void Interactive_shell::command_loop()
{
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


static void print_banner()
{
  std::cout << "Welcome to MySQL Shell 0.0.0\n" <<
    "\n" <<
    "Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.\n" <<
    "\n" <<
    "Oracle is a registered trademark of Oracle Corporation and/or its\n" <<
    "affiliates. Other names may be trademarks of their respective\n" <<
    "owners.\n" <<
    "\n";
}


int main(int argc, char **argv)
{
  {
    Interactive_shell shell;

    print_banner();

    shell.command_loop();
  }

  return 0;
}
