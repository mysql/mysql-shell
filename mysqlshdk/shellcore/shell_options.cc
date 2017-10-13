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

#include "shellcore/shell_options.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
Shell_options::Shell_options() {
#ifdef HAVE_V8
  initial_mode = shcore::IShell_core::Mode::JavaScript;
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
  print_version_extra = false;
  force = false;
  interactive = false;
  full_interactive = false;
  passwords_from_stdin = false;
  prompt_password = false;
  recreate_database = false;
  trace_protocol = false;
  wizards = true;
  admin_mode = false;
  log_to_stderr = false;

  port = 0;

  exit_code = 0;
}

bool Shell_options::has_connection_data() {
  return !uri.empty() ||
    !user.empty() ||
    !host.empty() ||
    !schema.empty() ||
    !sock.empty() ||
    port != 0 ||
    password != NULL ||
    prompt_password ||
    ssl_options.has_data();
}

static mysqlsh::SessionType get_session_type(
    const mysqlshdk::db::Connection_options &opt) {
  if (!opt.has_scheme()) {
    return mysqlsh::SessionType::Auto;
  } else {
    std::string scheme = opt.get_scheme();
    if (scheme == "mysqlx")
      return mysqlsh::SessionType::X;
    else if (scheme == "mysql")
      return mysqlsh::SessionType::Classic;
    else
      throw std::invalid_argument("Unknown MySQL URI type "+scheme);
  }
}

/**
 * Builds Connection_options from options given by user through command line
 */
mysqlshdk::db::Connection_options Shell_options::connection_options() {
  mysqlshdk::db::Connection_options target_server;
  if (!uri.empty()) {
    target_server = shcore::get_connection_options(uri);
  }

  shcore::update_connection_data(&target_server, user, password, host, port,
                                 sock, schema, ssl_options, auth_method);

  // If a scheme is given on the URI the session type must match the URI
  // scheme
  if (target_server.has_scheme()) {
    mysqlsh::SessionType uri_session_type = get_session_type(target_server);
    std::string error;

    if (session_type != mysqlsh::SessionType::Auto &&
        session_type != uri_session_type) {
      if (session_type == mysqlsh::SessionType::Classic)
        error = "The given URI conflicts with the --mysql session type option.";
      else if (session_type == mysqlsh::SessionType::X)
        error =
            "The given URI conflicts with the --mysqlx session type "
            "option.";
    }
    if (!error.empty())
      throw shcore::Exception::argument_error(error);
  } else {
    switch (session_type) {
      case mysqlsh::SessionType::Auto:
        target_server.clear_scheme();
        break;
      case mysqlsh::SessionType::X:
        target_server.set_scheme("mysqlx");
        break;
      case mysqlsh::SessionType::Classic:
        target_server.set_scheme("mysql");
        break;
    }
  }

  // TODO(rennox): Analize if this should actually exist... or if it
  // should be done right before connection for the missing data
  shcore::set_default_connection_data(&target_server);
  return target_server;
}

}
