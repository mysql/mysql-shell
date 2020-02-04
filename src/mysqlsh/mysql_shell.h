/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
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
class Util;
class Os;

class Mysql_shell : public mysqlsh::Base_shell {
 public:
  Mysql_shell(const std::shared_ptr<Shell_options> &cmdline_options,
              shcore::Interpreter_delegate *custom_delegate);
  ~Mysql_shell() override;

  virtual std::shared_ptr<mysqlsh::ShellBaseSession> connect(
      const mysqlshdk::db::Connection_options &args,
      bool recreate_schema = false, bool shell_global_session = true);

  bool redirect_session_if_needed(
      bool secondary, const Connection_options &opts = Connection_options());

  bool cmd_print_shell_help(const std::vector<std::string> &args);
  bool cmd_start_multiline(const std::vector<std::string> &args);
  bool cmd_connect(const std::vector<std::string> &args);
  bool cmd_reconnect(const std::vector<std::string> &args);
  bool cmd_quit(const std::vector<std::string> &args);
  bool cmd_warnings(const std::vector<std::string> &args);
  bool cmd_nowarnings(const std::vector<std::string> &args);
  bool cmd_status(const std::vector<std::string> &args);
  bool cmd_use(const std::vector<std::string> &args);
  bool cmd_rehash(const std::vector<std::string> &args);
  bool cmd_option(const std::vector<std::string> &args);
  virtual bool cmd_process_file(const std::vector<std::string> &params);
  bool cmd_show(const std::vector<std::string> &args);
  bool cmd_watch(const std::vector<std::string> &args);

  void process_line(const std::string &line) override;
  bool reconnect_if_needed(bool force = false);

  static bool sql_safe_for_logging(const std::string &sql);

  shcore::Shell_command_handler *command_handler() {
    return shell_context()->command_handler();
  }

  using File_list = std::map<shcore::IShell_core::Mode,
                             std::vector<shcore::Plugin_definition>>;
  void load_files(const File_list &file_list, const std::string &context);

  /**
   * Gets all the startup files for the supported scripting languages at:
   *
   * - ${MYSQLSH_USER_CONFIG_HOME}/init.d
   */
  void get_startup_scripts(File_list *list);
  /**
   * Gets the plugin initialization files  for the supported languages.
   *
   * Iterates through known plugin directories and loads plugins based on their
   * init file extension. Checks following directories:
   *
   * - <INSTALL_ROOT>/share/plugins
   * - ${MYSQLSH_USER_CONFIG_HOME}/plugins
   */
  void get_plugins(File_list *list);
  bool get_plugins(File_list *list, const std::string &dir,
                   bool allow_recursive);
  void finish_init() override;

  void init_extra_globals();

 protected:
  static void set_sql_safe_for_logging(const std::string &patterns);

  void set_active_session(std::shared_ptr<mysqlsh::ShellBaseSession> session);

  virtual bool do_shell_command(const std::string &command);

  void refresh_completion(bool force = false);
  void refresh_schema_completion(bool force = false);
  void add_devapi_completions();

  void print_connection_message(mysqlsh::SessionType type,
                                const std::string &uri,
                                const std::string &sessionid);

  void process_sql_result(const std::shared_ptr<mysqlshdk::db::IResult> &result,
                          const shcore::Sql_result_info &info) override;

  Scoped_console m_console_handler;
  std::shared_ptr<mysqlsh::Shell> _global_shell;
  std::shared_ptr<mysqlsh::Sys> _global_js_sys;
  std::shared_ptr<mysqlsh::dba::Dba> _global_dba;
  std::shared_ptr<mysqlsh::Util> _global_util;
  std::shared_ptr<mysqlsh::Os> m_global_js_os;

  std::shared_ptr<mysqlsh::dba::Cluster> m_default_cluster;
  std::shared_ptr<mysqlsh::dba::ReplicaSet> m_default_replicaset;

  /// Last schema set by the user via \use command.
  std::string _last_active_schema;

 private:
  std::shared_ptr<mysqlsh::dba::Cluster> create_default_cluster_object();
  std::shared_ptr<mysqlsh::dba::ReplicaSet> create_default_replicaset_object();

#ifdef FRIEND_TEST
  FRIEND_TEST(Cmdline_shell, check_password_history_linenoise);
  FRIEND_TEST(Cmdline_shell, check_history_overflow_del);
  FRIEND_TEST(Cmdline_shell, check_history_source);
  FRIEND_TEST(Cmdline_shell, history_autosave_int);
  FRIEND_TEST(Cmdline_shell, check_help_shows_history);
  FRIEND_TEST(Interactive_dba_create_cluster, read_only_no_prompts);
#endif
};
}  // namespace mysqlsh
#endif
