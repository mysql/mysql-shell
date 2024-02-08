/*
 * Copyright (c) 2014, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_BASE_SHELL_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_BASE_SHELL_H_

#include "mysqlshdk/shellcore/provider_script.h"
#include "mysqlshdk/shellcore/provider_sql.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/completer.h"
#include "shellcore/scoped_contexts.h"
#include "shellcore/shell_core.h"
#include "shellcore/shell_options.h"
#include "shellcore/shell_sql.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mysqlsh {
class SHCORE_PUBLIC Base_shell {
 public:
  explicit Base_shell(const std::shared_ptr<Shell_options> &cmdline_options);

  virtual ~Base_shell();

  int process_stream(std::istream &stream, const std::string &source,
                     const std::vector<std::string> &argv,
                     bool force_batch = false);
  int process_file(const std::string &path,
                   const std::vector<std::string> &argv);

  int run_module(const std::string &module,
                 const std::vector<std::string> &argv);

  /** Finalize initialization steps after basic init of the shell is already
     done Does things like loading init scripts.
   */
  virtual void finish_init();

  void init_scripts(shcore::Shell_core::Mode mode);
  void load_default_modules(shcore::Shell_core::Mode mode);

  shcore::IShell_core::Mode interactive_mode() const {
    return _shell->interactive_mode();
  }

  virtual void process_line(const std::string &line);
  void flush_input();
  shcore::Input_state input_state() const { return _input_mode; }
  void clear_input();

  void notify_executed_statement(const std::string &line);
  virtual std::string prompt();
  std::map<std::string, std::string> *prompt_variables();

  std::shared_ptr<shcore::Shell_core> shell_context() const { return _shell; }

  void set_global_object(
      const std::string &name,
      const std::shared_ptr<shcore::Cpp_object_bridge> &object,
      shcore::IShell_core::Mode_mask modes =
          shcore::IShell_core::Mode_mask::any());
  virtual bool switch_shell_mode(shcore::Shell_core::Mode mode,
                                 const std::vector<std::string> &args,
                                 bool initializing = false,
                                 bool prompt_variables_update = true);

  shcore::completer::Completer *completer() { return &_completer; }

  std::shared_ptr<shcore::completer::Object_registry>
  completer_object_registry() {
    return _completer_object_registry;
  }

  std::shared_ptr<shcore::completer::Provider_sql> provider_sql() {
    return _provider_sql;
  }

  std::shared_ptr<Shell_options> get_options() const {
    return m_shell_options.get();
  }

  const Shell_options::Storage &options() const { return get_options()->get(); }

  virtual void print_result(const shcore::Value &result);
  virtual void process_result(const shcore::Value &result, bool got_error);

 protected:
  void request_prompt_variables_update(bool clear_cache = false);

  Scoped_shell_options m_shell_options;
  std::unique_ptr<std::string> _deferred_output;
  std::shared_ptr<shcore::Shell_core> _shell;
  std::map<std::string, std::string> _prompt_variables;
  shcore::Input_state _input_mode;
  std::string _input_buffer;
  shcore::completer::Completer _completer;
  std::shared_ptr<shcore::completer::Object_registry>
      _completer_object_registry;
  std::shared_ptr<shcore::completer::Provider_sql> _provider_sql;

  virtual void process_sql_result(
      const std::shared_ptr<mysqlshdk::db::IResult> &result,
      const shcore::Sql_result_info &info);

  void println_deferred(const std::string &str);

  std::string get_shell_hook(const std::string &class_name);

 private:
  enum class Prompt_variables_update_type { NO_UPDATE, UPDATE, CLEAR_CACHE };

  void update_prompt_variables();
  void execute_buffered_code(bool flush);

  Prompt_variables_update_type m_pending_update =
      Prompt_variables_update_type::UPDATE;

 protected:
#ifdef FRIEND_TEST
  friend class Completion_cache_refresh;
  FRIEND_TEST(Interactive_shell_test, ssl_status);
#endif
};
}  // namespace mysqlsh
#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_BASE_SHELL_H_
