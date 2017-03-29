/*
 * Copyright (c) 2017 Oracle and/or its affiliates. All rights reserved.
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
#ifndef _MYSQL_SHELL_
#define _MYSQL_SHELL_
#include "shellcore/base_shell.h"
#include "shellcore/shell_options.h"
#include "scripting/types.h"
#include "shellcore/shell_core.h"

#include "modules/mod_shell.h"
#include "modules/mod_sys.h"
#include "modules/adminapi/mod_dba.h"

namespace mysqlsh {
class Mysql_shell :public mysqlsh::Base_shell {
public:
  Mysql_shell(const Shell_options &options, shcore::Interpreter_delegate *custom_delegate);

  bool connect(bool primary_session = false);
  shcore::Value connect_session(const shcore::Argument_list &args, mysqlsh::SessionType session_type, bool recreate_schema);

  bool cmd_print_shell_help(const std::vector<std::string>& args);
  bool cmd_start_multiline(const std::vector<std::string>& args);
  bool cmd_connect(const std::vector<std::string>& args);
  bool cmd_quit(const std::vector<std::string>& args);
  bool cmd_warnings(const std::vector<std::string>& args);
  bool cmd_nowarnings(const std::vector<std::string>& args);
  bool cmd_status(const std::vector<std::string>& args);
  bool cmd_use(const std::vector<std::string>& args);
  bool cmd_process_file(const std::vector<std::string>& params);

  virtual void process_line(const std::string &line);

private:
  shcore::Shell_command_handler _shell_command_handler;

  bool do_shell_command(const std::string &command);

  std::shared_ptr<mysqlsh::Shell> _global_shell;
  std::shared_ptr<mysqlsh::Sys> _global_js_sys;
  std::shared_ptr<mysqlsh::dba::Dba> _global_dba;


};
}
#endif
