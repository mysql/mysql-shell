/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_CONSOLE_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_CONSOLE_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "mysqlshdk/include/scripting/lang_base.h"

namespace mysqlsh {

enum class Prompt_answer { NONE = 0, YES = 1, NO = 2, ALT = 3 };
enum class Output_stream { STDOUT = 0, STDERR = 1 };

class IPager {
 public:
  IPager() = default;
  virtual ~IPager() = default;

  IPager(const IPager &) = delete;
  IPager(IPager &&) = delete;

  IPager &operator=(const IPager &) = delete;
  IPager &operator=(IPager &&) = delete;
};

class IConsole {
 public:
  virtual ~IConsole() {}

  virtual void raw_print(const std::string &text, Output_stream stream,
                         bool format_json = true) const = 0;
  virtual void print(const std::string &text) const = 0;
  virtual void println(const std::string &text = "") const = 0;
  virtual void print_error(const std::string &text) const = 0;
  virtual void print_warning(const std::string &text) const = 0;
  virtual void print_note(const std::string &text) const = 0;
  virtual void print_status(const std::string &text) const = 0;
  virtual void print_info(const std::string &text = "") const = 0;
  virtual void print_para(const std::string &text) const = 0;

  virtual void print_value(const shcore::Value &value,
                           const std::string &tag) const = 0;
  virtual void print_diag(const std::string &text) const = 0;

  // Throws shcore::cancelled() on ^C
  using Validator = std::function<std::string(const std::string &)>;

  virtual shcore::Prompt_result prompt(const std::string &prompt,
                                       std::string *out_val,
                                       Validator validator = nullptr) const = 0;

  /**
   * Show confirmation prompt with the displayed options.
   * @param  prompt    prompt text to display
   * @param  def       default option
   * @param  yes_label label to override Yes option
   * @param  no_label  label to override No option
   * @param  alt_label label to override third option (e.g. Cancel)
   * @return           answer entered by user
   *
   * no_label and alt_label can be "", in which case they will be skipped.
   * label text can precede the shortcut letter with &.
   *
   * Throws shcore::cancelled() on ^C
   */
  virtual Prompt_answer confirm(const std::string &prompt,
                                Prompt_answer def = Prompt_answer::NO,
                                const std::string &yes_label = "&Yes",
                                const std::string &no_label = "&No",
                                const std::string &alt_label = "") const = 0;

  virtual shcore::Prompt_result prompt_password(
      const std::string &prompt, std::string *out_val,
      Validator validator = nullptr) const = 0;

  virtual bool select(const std::string &prompt_text, std::string *result,
                      const std::vector<std::string> &items,
                      size_t default_option = 0, bool allow_custom = false,
                      Validator validator = nullptr) const = 0;

  /**
   * Enables the pager and returns its handle. As long IPager exists, all output
   * is going to be handled by the pager. If this method is called while
   * the previous handle still exists, it returns the same handle. Pager is
   * automatically configured using the current shell options.
   *
   * @returns handle to the current pager.
   */
  virtual std::shared_ptr<IPager> enable_pager() = 0;

  /**
   * Enables the global pager, which exists till disable_global_pager(). This
   * has the same effect as calling enable_pager() and storing the returned
   * handle.
   */
  virtual void enable_global_pager() = 0;

  /**
   * Disables the global pager. This has the same effect as releasing the
   * handle returned by the enable_pager().
   */
  virtual void disable_global_pager() = 0;

  /**
   * Checks if the global pager is enabled.
   *
   * @returns true if the global pager is enabled.
   */
  virtual bool is_global_pager_enabled() const = 0;

  virtual void add_print_handler(shcore::Interpreter_print_handler *) = 0;

  virtual void remove_print_handler(shcore::Interpreter_print_handler *) = 0;

  void set_verbose(int level) {
    m_verbose = level;
    on_set_verbose();
  }

  int get_verbose() const { return m_verbose; }

 protected:
  int m_verbose = 0;

 private:
  virtual void on_set_verbose() {}
};

std::shared_ptr<IConsole> current_console(bool allow_empty = false);

}  // namespace mysqlsh

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_CONSOLE_H_
