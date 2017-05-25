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
#include "shellcore/shell_options.h"
#include "utils/uri_data.h"

class Shell_command_line_options : public Command_line_options {
public:
  // Takes the URI and the individual connection parameters and overrides
  Shell_command_line_options(int argc, char **argv);

  mysqlsh::Shell_options& get_options() { return _options; }
  static std::vector<std::string> get_details();
private:
  void override_session_type(mysqlsh::SessionType new_type, const std::string& option, char* value = NULL);
  std::string get_session_type_name(mysqlsh::SessionType type);
  void check_session_type_conflicts();
  void check_user_conflicts();
  void check_password_conflicts();
  void check_host_conflicts();
  void check_host_socket_conflicts();
  void check_port_conflicts();
  void check_socket_conflicts();
  void check_port_socket_conflicts();

  shcore::uri::Uri_data _uri_data;
  mysqlsh::Shell_options _options;
  std::string _session_type_arg;
};
#endif
