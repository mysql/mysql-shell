/*
 * Copyright (c) 2014, 2016 Oracle and/or its affiliates. All rights reserved.
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
#ifndef _CMDLINE_SHELL_
#define _CMDLINE_SHELL_
#include "shell/base_shell.h"
#ifndef WIN32
#  include "editline/readline.h"
#endif

#include "shell/shell_options.h"
#include "shellcore/types.h"
#include "shellcore/shell_core.h"

namespace mysh {
class Command_line_shell :public mysh::Base_shell {
public:
  Command_line_shell(const Shell_options &options);
  void command_loop();

  void print_cmd_line_helper();
  void print_banner();

private:
  shcore::Interpreter_delegate _delegate;
  static char *readline(const char *prompt);

  static void deleg_print(void *self, const char *text);
  static void deleg_print_error(void *self, const char *text);
  static bool deleg_prompt(void *self, const char *text, std::string &ret);
  static bool deleg_password(void *self, const char *text, std::string &ret);
  static void deleg_source(void *self, const char *module);
};
}
#endif
