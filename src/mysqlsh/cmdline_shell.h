/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _CMDLINE_SHELL_
#define _CMDLINE_SHELL_

#include <memory>
#include <string>
#include <vector>

#include "shellcore/base_shell.h"

#include "mysqlsh/mysql_shell.h"
#include "scripting/types.h"
#include "shellcore/shell_core.h"
#include "shellcore/shell_notifications.h"
#include "shellcore/shell_options.h"

#include "mysqlsh/history.h"
#include "mysqlsh/prompt_manager.h"

namespace mysqlsh {

class Command_line_shell : public Mysql_shell,
                           public shcore::NotificationObserver {
 public:
  explicit Command_line_shell(std::shared_ptr<Shell_options> options);

  Command_line_shell(std::shared_ptr<Shell_options> options,
                     std::unique_ptr<shcore::Interpreter_delegate> delegate);
  ~Command_line_shell() override;

  void print_banner();
  void command_loop();

  void print_cmd_line_helper();

  std::string prompt() override;

  void load_state(const std::string &statedir);
  void save_state(const std::string &statedir);

  void load_prompt_theme(const std::string &path);

  void quiet_print();
  void restore_print();

 private:
  void handle_interrupt();
  bool _interrupted = false;

 protected:
  void pause_history(bool flag) { _history.pause(flag); }

 private:
  History _history;

  bool cmd_process_file(const std::vector<std::string> &params) override;

  bool cmd_history(const std::vector<std::string> &args);

  bool cmd_pager(const std::vector<std::string> &args);

  bool cmd_nopager(const std::vector<std::string> &args);

  void handle_notification(const std::string &name,
                           const shcore::Object_bridge_ref &sender,
                           shcore::Value::Map_type_ref data) override;

 private:
  static char *readline(const char *prompt);

  static void deleg_print(void *self, const char *text);
  static void deleg_disable_print(void *self, const char *text);
  static void deleg_print_error(void *self, const char *text);
  static void deleg_print_diag(void *self, const char *text);
  static shcore::Prompt_result deleg_prompt(void *self, const char *text,
                                            std::string *ret);
  static shcore::Prompt_result deleg_password(void *self, const char *text,
                                              std::string *ret);
  std::string query_variable(
      const std::string &var,
      mysqlsh::Prompt_manager::Dynamic_variable_type type);

  std::string get_current_session_uri() const;
  void detect_session_change();

  std::unique_ptr<shcore::Interpreter_delegate> _delegate;

  shcore::Interpreter_delegate _backup_delegate;
  std::stringstream _full_output;
  Prompt_manager _prompt;
  bool _output_printed;
  const std::string m_default_pager;
  std::string m_current_session_uri;

#ifdef FRIEND_TEST
  FRIEND_TEST(Cmdline_shell, query_variable_classic);
  FRIEND_TEST(Cmdline_shell, query_variable_x);
  FRIEND_TEST(Cmdline_shell, help);
  FRIEND_TEST(Cmdline_shell, prompt);
  FRIEND_TEST(Shell_history, check_password_history_linenoise);
  FRIEND_TEST(Shell_history, history_linenoise);
  FRIEND_TEST(Shell_history, history_management);
  FRIEND_TEST(Shell_history, history_sizes);
  FRIEND_TEST(Shell_history, check_history_overflow_del);
  FRIEND_TEST(Shell_history, check_history_source_js);
  FRIEND_TEST(Shell_history, check_history_source_py);
  FRIEND_TEST(Shell_history, history_autosave_int);
  FRIEND_TEST(Shell_history, check_help_shows_history);
  FRIEND_TEST(Shell_history, history_del_invisible_entry);
  FRIEND_TEST(Shell_history, history_del_lower_but_no_upper);
  FRIEND_TEST(Shell_history, shell_options_help_history);
  FRIEND_TEST(Shell_history, history_entry_number_reset);
  FRIEND_TEST(Shell_history, history_source_history);
  FRIEND_TEST(Shell_history, history_del_range);
  FRIEND_TEST(Shell_history, history_delete_range);
  FRIEND_TEST(Shell_history, history_numbering);
  FRIEND_TEST(Shell_history, never_filter_latest);
  FRIEND_TEST(Shell_error_printing, print_error);
  FRIEND_TEST(Shell_error_printing, print_diag);
  friend class Test_debugger;
#endif
};
}  // namespace mysqlsh
#endif
