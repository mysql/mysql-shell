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

#ifndef _SHELL_CMDLINE_OPTIONS_H_
#define _SHELL_CMDLINE_OPTIONS_H_

#include <stdlib.h>
#include <iostream>
#include "cmdline_options.h"
#include "shell/shell_options.h"

class Shell_command_line_options : public Command_line_options {
public:
  // Takes the URI and the individual connection parameters and overrides
  Shell_command_line_options(int argc, char **argv);

  mysqlsh::Shell_options& get_options() { return _options; }
private:
  void override_session_type(mysqlsh::SessionType new_type, const std::string& option, char* value = NULL);

  mysqlsh::Shell_options _options;
};
#endif
