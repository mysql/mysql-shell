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

#ifndef _SHELL_CMDLINE_OPTIONS_H_
#define _SHELL_CMDLINE_OPTIONS_H_

#include <stdlib.h>
#include <iostream>
#include "cmdline_options.h"
#include "shellcore/ishell_core.h"
#include "modules/base_session.h"

class Shell_command_line_options : public Command_line_options
{
public:
  shcore::IShell_core::Mode initial_mode;
  std::string run_file;

  int port;
  std::string ssl_ca;
  std::string ssl_cert;
  std::string ssl_key;
  int ssl;
  std::string uri;
  std::string app;
  std::string password;
  std::string output_format;
  mysh::SessionType session_type;
  bool print_cmd_line_helper;
  bool print_version;
  bool force;
  bool interactive;
  bool full_interactive;
  bool passwords_from_stdin;
  bool recreate_database;
  ngcommon::Logger::LOG_LEVEL log_level;

  // Takes the URI and the individual connection parameters and overrides
  Shell_command_line_options(int argc, char **argv);

  // On the URI as specified on the parameters
  void configure_connection_string(const std::string &connstring,
    std::string &user, std::string &password,
    std::string &host, int &port,
    std::string &database, bool prompt_pwd, std::string &ssl_ca,
    std::string &ssl_cert, std::string &ssl_key);
};
#endif
