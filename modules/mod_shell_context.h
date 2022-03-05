/*
 * Copyright (c) 2020, 2022 Oracle and/or its affiliates.
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
#ifndef MODULES_MOD_SHELLCONTEXT_H_
#define MODULES_MOD_SHELLCONTEXT_H_

#include <memory>
#include <string>

#include "mysqlshdk/include/scripting/common.h"
#include "mysqlshdk/include/scripting/lang_base.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/shellcore/shell_console.h"
#include "src/mysqlsh/mysql_shell.h"

#include "mysqlshdk/shellcore/shell_prompt_options.h"

namespace mysqlsh {
class Shell;

class Shell_context_wrapper_options {
 public:
  Shell_context_wrapper_options() {}

  Shell_context_wrapper_options(const Shell_context_wrapper_options &) =
      default;
  Shell_context_wrapper_options(Shell_context_wrapper_options &&) = default;
  Shell_context_wrapper_options &operator=(
      const Shell_context_wrapper_options &) = default;
  Shell_context_wrapper_options &operator=(Shell_context_wrapper_options &&) =
      default;

  virtual ~Shell_context_wrapper_options() = default;

  static const shcore::Option_pack_def<Shell_context_wrapper_options>
      &options();

  const std::string &log_file() const { return m_log_file; }

  shcore::Function_base_ref deleg_print_func() const {
    return m_deleg_print_func;
  }
  shcore::Function_base_ref deleg_prompt_func() const {
    return m_deleg_prompt_func;
  }
  shcore::Function_base_ref deleg_error_func() const {
    return m_deleg_error_func;
  }
  shcore::Function_base_ref deleg_diag_func() const {
    return m_deleg_diag_func;
  }

 private:
  void on_start_unpack(const shcore::Dictionary_t &options);

  shcore::Dictionary_t m_options;

  std::string m_log_file =
      shcore::path::join_path(shcore::get_user_config_path(), "mysqlsh.log");
  shcore::Function_base_ref m_deleg_print_func;
  shcore::Function_base_ref m_deleg_prompt_func;
  shcore::Function_base_ref m_deleg_error_func;
  shcore::Function_base_ref m_deleg_diag_func;
};

class Delegate_wrapper : public shcore::Interpreter_delegate {
 public:
  explicit Delegate_wrapper(
      const shcore::Option_pack_ref<Shell_context_wrapper_options> &callbacks);

 private:
  static bool deleg_print(void *self, const char *text);
  static shcore::Prompt_result deleg_prompt(
      void *self, const char *text,
      const shcore::prompt::Prompt_options &options, std::string *ret);
  static bool deleg_print_error(void *self, const char *text);
  static bool deleg_print_diag(void *self, const char *text);
  static shcore::Value call_delegate(
      void *cdata, const char *text,
      shcore::Function_base_ref Delegate_wrapper::*func);
  static shcore::Value call_prompt_delegate(
      void *cdata, const char *text,
      const shcore::prompt::Prompt_options &options,
      shcore::Function_base_ref Delegate_wrapper::*func);
  static shcore::Prompt_result get_prompt_value(
      const shcore::Value &prompt_value, std::string *ret);

  shcore::Function_base_ref m_deleg_print_func;
  shcore::Function_base_ref m_deleg_prompt_func;
  shcore::Function_base_ref m_deleg_error_func;
  shcore::Function_base_ref m_deleg_diag_func;
};

#if DOXYGEN_PY
/**
 * \ingroup ShellAPI
 * $(SHELLCONTEXTWRAPPER_BRIEF)
 */
class ShellContextWrapper {
 public:
  dict globals;
  Shell get_shell();
};
#endif

class SHCORE_PUBLIC Shell_context_wrapper
    : public shcore::Cpp_object_bridge,
      public std::enable_shared_from_this<Shell_context_wrapper> {
 public:
  Shell_context_wrapper(
      const shcore::Option_pack_ref<Shell_context_wrapper_options> &callbacks,
      const std::shared_ptr<Shell_options> &opts);

  shcore::Value get_member(const std::string &prop) const override;
  std::shared_ptr<Shell> get_shell() const;

  std::string class_name() const override { return "ShellContextWrapper"; }

 private:
  shcore::Dictionary_t get_globals() const;

  std::unique_ptr<Delegate_wrapper> m_delegate_wrapper;
  std::shared_ptr<Mysql_shell> m_mysql_shell;
  std::shared_ptr<Scoped_logger> m_logger;
  mysqlsh::Mysql_thread m_mysql_thread;
  std::shared_ptr<Scoped_interrupt> m_interrupt;
};

}  // namespace mysqlsh
#endif  // MODULES_MOD_SHELLCONTEXT_H_
