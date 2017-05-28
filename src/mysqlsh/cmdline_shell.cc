/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "cmdline_shell.h"
#include "shellcore/base_session.h"
#include "shell_cmdline_options.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "shellcore/shell_core_options.h" // <---
#include "modules/devapi/base_resultset.h"
#include "utils/utils_time.h"
#include "shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/logger.h"

// TODO: This should be ported from the server, not used from there (see comment below)
//const int MAX_READLINE_BUF = 65536;
extern char *mysh_get_tty_password(const char *opt_message);

namespace mysqlsh {
Command_line_shell::Command_line_shell(const Shell_options &options) : mysqlsh::Mysql_shell(options, &_delegate) {
#ifndef WIN32
  rl_initialize();
#endif

  _delegate.user_data = this;
  _delegate.print = &Command_line_shell::deleg_print;
  _delegate.print_error = &Command_line_shell::deleg_print_error;
  _delegate.prompt = &Command_line_shell::deleg_prompt;
  _delegate.password = &Command_line_shell::deleg_password;
  _delegate.source = &Command_line_shell::deleg_source;
  _delegate.print_value = nullptr;

  observe_notification("SN_STATEMENT_EXECUTED");

  finish_init();
}

void Command_line_shell::deleg_print(void *cdata, const char *text) {
  std::cout << text << std::flush;
}

void Command_line_shell::deleg_print_error(void *cdata, const char *text) {
  std::cerr << text << std::flush;
}

char *Command_line_shell::readline(const char *prompt) {
  char *tmp = NULL;
  // TODO: This should be ported from the server, not used from there
  /*
  tmp = (char *)malloc(MAX_READLINE_BUF);
  if (!tmp)
  throw Exception::runtime_error("Cannot allocate memory for Command_line_shell::readline buffer.");
  my_win_console_fputs(&my_charset_latin1, prompt);
  tmp = my_win_console_readline(&my_charset_latin1, tmp, MAX_READLINE_BUF);
  */

#ifndef WIN32
  std::string prompt_line(prompt);

  size_t pos = prompt_line.rfind("\n");
  if (pos != std::string::npos) {
    auto all_lines = prompt_line.substr(0, pos+1);
    prompt_line = prompt_line.substr(pos+1);

    if (!all_lines.empty())
      std::cout << all_lines << std::flush;
  }

  tmp = ::readline(prompt_line.c_str());
#else
  std::string line;
  std::cout << prompt << std::flush;

  std::getline(std::cin, line);

  if (!std::cin.fail())
    tmp = strdup(line.c_str());
#endif

  return tmp;
}

bool Command_line_shell::deleg_prompt(void *UNUSED(cdata), const char *prompt, std::string &ret) {
  char *tmp = Command_line_shell::readline(prompt);
  if (!tmp)
    return false;

  ret = tmp;
  free(tmp);

  return true;
}

bool Command_line_shell::deleg_password(void *cdata, const char *prompt, std::string &ret) {
  Command_line_shell *self = (Command_line_shell*)cdata;
  char *tmp = self->_options.passwords_from_stdin ? shcore::mysh_get_stdin_password(prompt) : mysh_get_tty_password(prompt);
  if (!tmp)
    return false;
  ret = tmp;
  free(tmp);
  return true;
}

void Command_line_shell::deleg_source(void *cdata, const char *module) {
  Command_line_shell *self = (Command_line_shell*)cdata;
  self->process_file(module, {});
}

void Command_line_shell::command_loop() {
  if (_options.interactive) // check if interactive
  {
    std::string message;
    auto session = _shell->get_dev_session();

    if (!session || (session && session->class_name() != "XSession"))
      message = " Use \\sql to switch to SQL mode and execute queries.";

    switch (_shell->interactive_mode()) {
      case shcore::Shell_core::Mode::SQL:
#ifdef HAVE_V8
        message = "Currently in SQL mode. Use \\js or \\py to switch the shell to a scripting language.";
#else
        message = "Currently in SQL mode. Use \\py to switch the shell to python scripting.";
#endif
        break;
      case shcore::Shell_core::Mode::JavaScript:
        message = "Currently in JavaScript mode." + message;
        break;
      case shcore::Shell_core::Mode::Python:
        message = "Currently in Python mode." + message;
        break;
      default:
        break;
    }

    println(message);
  }

  while (_options.interactive) {
    char *cmd = Command_line_shell::readline(prompt().c_str());
    if (!cmd)
      break;

    process_line(cmd);
    free(cmd);
  }

  std::cout << "Bye!\n";
}

void Command_line_shell::print_banner() {
  std::string welcome_msg("MySQL Shell ");
  welcome_msg += MYSH_VERSION;
  welcome_msg += "\n\n";
  welcome_msg += "Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.\n\n"\
                 "Oracle is a registered trademark of Oracle Corporation and/or its\n"\
                 "affiliates. Other names may be trademarks of their respective\n"\
                 "owners.";
  println(welcome_msg);
  println();
  println("Type '\\help' or '\\?' for help; '\\quit' to exit.");
  println();
}

void Command_line_shell::print_cmd_line_helper() {
  std::string help_msg("MySQL Shell ");
  help_msg += MYSH_VERSION;
  println(help_msg);
  println("");
  println("Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.");
  println("");
  println("Oracle is a registered trademark of Oracle Corporation and/or its");
  println("affiliates. Other names may be trademarks of their respective");
  println("owners.");
  println("");
  println("Usage: mysqlsh [OPTIONS] [URI]");
  println("Usage: mysqlsh [OPTIONS] [URI] -f <path> [script args...]");
  std::vector<std::string> details = Shell_command_line_options::get_details();
  for (std::string line : details)
    println(line);
  println("");
  println("Usage examples:");
  println("$ mysqlsh root@localhost/schema");
  println("$ mysqlsh mysqlx://root@some.server:3307/world_x");
  println("$ mysqlsh --uri root@localhost --py -f sample.py sample param");
  println("$ mysqlsh root@targethost:33070 -s world_x -f sample.js");
  println("");
}

void Command_line_shell::handle_notification(const std::string &name, const shcore::Object_bridge_ref& sender, shcore::Value::Map_type_ref data){
#ifndef WIN32
  if(name=="SN_STATEMENT_EXECUTED"){
    std::string executed = data->get_string("statement");
    add_history(executed.c_str());
  }
#endif
}

}
