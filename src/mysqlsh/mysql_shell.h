/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <memory>
#include <string>
#include <vector>
#include "scripting/types.h"
#include "shellcore/base_shell.h"
#include "shellcore/shell_core.h"
#include "shellcore/shell_options.h"

#include "modules/adminapi/mod_dba.h"
#include "modules/mod_sys.h"
#include "mysqlshdk/libs/db/connection_options.h"

namespace mysqlsh {
class Shell;  // from modules

class Mysql_shell : public mysqlsh::Base_shell {
 public:
  Mysql_shell(std::shared_ptr<Shell_options> cmdline_options,
              shcore::Interpreter_delegate* custom_delegate);
  ~Mysql_shell();

  virtual void connect(
      const mysqlshdk::db::Connection_options& args,
      bool recreate_schema = false);

  const mysqlshdk::db::Connection_options& target_server() const {
    return _target_server;
  }

  bool cmd_print_shell_help(const std::vector<std::string>& args);
  bool cmd_start_multiline(const std::vector<std::string>& args);
  bool cmd_connect(const std::vector<std::string>& args);
  bool cmd_quit(const std::vector<std::string>& args);
  bool cmd_warnings(const std::vector<std::string>& args);
  bool cmd_nowarnings(const std::vector<std::string>& args);
  bool cmd_status(const std::vector<std::string>& args);
  bool cmd_use(const std::vector<std::string>& args);
  bool cmd_rehash(const std::vector<std::string>& args);
  virtual bool cmd_process_file(const std::vector<std::string>& params);

  virtual void process_line(const std::string& line);

  static bool sql_safe_for_logging(const std::string &sql);

  std::shared_ptr<mysqlsh::ShellBaseSession> create_session(
      const mysqlshdk::db::Connection_options &connection_options);

  shcore::Shell_command_handler* command_handler() {
    return &_shell_command_handler;
  }

 protected:
  mysqlshdk::db::Connection_options _target_server;
  shcore::Shell_command_handler _shell_command_handler;
  static void set_sql_safe_for_logging(const std::string &patterns);

  virtual bool do_shell_command(const std::string& command);

  void refresh_completion(bool force = false);
  void refresh_schema_completion(bool force = false);
  void add_devapi_completions();

  void print_connection_message(mysqlsh::SessionType type,
                                const std::string& uri,
                                const std::string& sessionid);

  std::shared_ptr<mysqlsh::Shell> _global_shell;
  std::shared_ptr<mysqlsh::Sys> _global_js_sys;
  std::shared_ptr<mysqlsh::dba::Dba> _global_dba;
#ifdef FRIEND_TEST
  FRIEND_TEST(Cmdline_shell, check_password_history_linenoise);
  FRIEND_TEST(Cmdline_shell, check_history_overflow_del);
  FRIEND_TEST(Cmdline_shell, check_history_source);
  FRIEND_TEST(Cmdline_shell, history_autosave_int);
  FRIEND_TEST(Cmdline_shell, check_help_shows_history);
#endif
};
}  // namespace mysqlsh
#endif
