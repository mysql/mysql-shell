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

#ifndef SRC_MYSQLSH_SHELL_CMDLINE_OPTIONS_H_
#define SRC_MYSQLSH_SHELL_CMDLINE_OPTIONS_H_

#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/utils/options.h"
#include "shellcore/shell_options.h"

class Shell_command_line_options : protected shcore::Options {
 public:
  Shell_command_line_options(int argc, char **argv);

  const mysqlsh::Shell_options* operator->() {
    return &shell_options;
  }
  const mysqlsh::Shell_options* operator*() {
    return &shell_options;
  }

  mysqlsh::Shell_options& get_options() {
    return shell_options;
  }

  std::vector<std::string> get_details() {
    return get_cmdline_help(28, 50);
  }

 protected:
  bool custom_cmdline_handler(char **argv, int* argi);

  void override_session_type(const std::string& option, const char* value);

  void check_session_type_conflicts();
  void check_user_conflicts();
  void check_password_conflicts();
  void check_host_conflicts();
  void check_host_socket_conflicts();
  void check_port_conflicts();
  void check_socket_conflicts();
  void check_port_socket_conflicts();

  mysqlsh::Shell_options shell_options;
  mysqlshdk::db::Connection_options uri_data;
  std::string session_type_arg;
};

#endif  // SRC_MYSQLSH_SHELL_CMDLINE_OPTIONS_H_
