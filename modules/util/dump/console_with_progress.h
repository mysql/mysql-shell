/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_CONSOLE_WITH_PROGRESS_H_
#define MODULES_UTIL_DUMP_CONSOLE_WITH_PROGRESS_H_

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/textui/text_progress.h"

namespace mysqlsh {
namespace dump {

class Console_with_progress final : public IConsole {
 public:
  Console_with_progress() = delete;

  Console_with_progress(mysqlshdk::textui::IProgress *progress,
                        std::recursive_mutex *mutex);

  Console_with_progress(const Console_with_progress &) = delete;
  Console_with_progress(Console_with_progress &&) = delete;

  Console_with_progress &operator=(const Console_with_progress &) = delete;
  Console_with_progress &operator=(Console_with_progress &&) = delete;

  ~Console_with_progress() override = default;

  void raw_print(const std::string &text, Output_stream stream,
                 bool format_json = true) const override;

  void print(const std::string &text) const override;

  void println(const std::string &text = "") const override;

  void print_error(const std::string &text) const override;

  void print_warning(const std::string &text) const override;

  void print_note(const std::string &text) const override;

  void print_status(const std::string &text) const override;

  void print_info(const std::string &text = "") const override;

  void print_para(const std::string &text) const override;

  void print_value(const shcore::Value &value,
                   const std::string &tag) const override;

  void print_diag(const std::string &text) const override;

  shcore::Prompt_result prompt(const std::string &prompt, std::string *out_val,
                               Validator validator = nullptr) const override;

  Prompt_answer confirm(const std::string &prompt,
                        Prompt_answer def = Prompt_answer::NO,
                        const std::string &yes_label = "&Yes",
                        const std::string &no_label = "&No",
                        const std::string &alt_label = "") const override;

  shcore::Prompt_result prompt_password(
      const std::string &prompt, std::string *out_val,
      Validator validator = nullptr) const override;

  bool select(const std::string &prompt_text, std::string *result,
              const std::vector<std::string> &items, size_t default_option = 0,
              bool allow_custom = false,
              Validator validator = nullptr) const override;

  std::shared_ptr<IPager> enable_pager() override;

  void enable_global_pager() override;

  void disable_global_pager() override;

  bool is_global_pager_enabled() const override;

  void add_print_handler(shcore::Interpreter_print_handler *handler) override;

  void remove_print_handler(
      shcore::Interpreter_print_handler *handler) override;

 private:
  mysqlshdk::textui::IProgress *m_progress;
  std::recursive_mutex *m_mutex;
  std::shared_ptr<IConsole> m_console;
};

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_CONSOLE_WITH_PROGRESS_H_
