/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "shell/shell_options.h"
namespace mysqlsh {
Shell_options::Shell_options() {
#ifdef HAVE_V8
  initial_mode = shcore::IShell_core::Mode::JScript;
#else
#ifdef HAVE_PYTHON
  initial_mode = shcore::IShell_core::Mode::Python;
#else
  initial_mode = shcore::IShell_core::Mode::SQL;
#endif
#endif

  log_level = ngcommon::Logger::LOG_INFO;
  password = nullptr;
  session_type = mysqlsh::SessionType::Auto;

  default_session_type = true;
  print_cmd_line_helper = false;
  print_version = false;
  force = false;
  interactive = false;
  full_interactive = false;
  passwords_from_stdin = false;
  prompt_password = false;
  recreate_database = false;
  trace_protocol = false;
  wizards = true;
  admin_mode = false;

  port = 0;
  ssl = 0;
  exit_code = 0;
}

bool Shell_options::has_connection_data() {
  return !uri.empty() ||
         !user.empty() ||
         !host.empty() ||
         !schema.empty() ||
         port != 0 ||
         password != NULL ||
         prompt_password ||
         ssl == 1 ||
         !ssl_ca.empty() ||
         !ssl_cert.empty() ||
         !ssl_key.empty();
}
}
