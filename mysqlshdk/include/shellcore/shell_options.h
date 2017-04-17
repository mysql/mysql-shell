/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _SHELL_OPTIONS_H_
#define _SHELL_OPTIONS_H_

#include <stdlib.h>
#include <iostream>
#include "shellcore/ishell_core.h"
#include "mysqlshdk/libs/db/ssl_info.h"

namespace mysqlsh {
struct SHCORE_PUBLIC Shell_options {
public:
  Shell_options();

  shcore::IShell_core::Mode initial_mode;
  std::string run_file;

  // Individual connection parameters
  std::string user;
  std::string pwd;
  const char *password;
  std::string host;
  int port;
  std::string schema;
  std::string sock;
  std::string auth_method;

  std::string protocol;

  // SSL connection parameters
  mysqlshdk::utils::Ssl_info ssl_info;

  std::string uri;

  std::string output_format;
  mysqlsh::SessionType session_type;
  bool default_session_type;
  bool print_cmd_line_helper;
  bool print_version;
  bool print_version_extra;
  bool force;
  bool interactive;
  bool full_interactive;
  bool passwords_from_stdin;
  bool prompt_password;
  bool recreate_database;
  bool trace_protocol;
  bool log_to_stderr;
  std::string execute_statement;
  std::string execute_dba_statement;
  ngcommon::Logger::LOG_LEVEL log_level;
  bool wizards;
  bool admin_mode;

  // cmdline params to be passed to script
  std::vector<std::string> script_argv;

  int exit_code;

  bool has_connection_data();
};
}
#endif
