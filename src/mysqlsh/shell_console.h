/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SRC_MYSQLSH_SHELL_CONSOLE_H_
#define SRC_MYSQLSH_SHELL_CONSOLE_H_

#include <string>
#include <vector>

#include "mysqlshdk/include/shellcore/console.h"
#include "scripting/lang_base.h"

namespace mysqlsh {

class Shell_console : public IConsole {
 public:
  // Interface methods
  explicit Shell_console(shcore::Interpreter_delegate *deleg);

  void print(const std::string &text) override;
  void println(const std::string &text = "") override;
  void print_error(const std::string &text) override;
  void print_warning(const std::string &text) override;
  void print_note(const std::string &text) override;
  void print_info(const std::string &text) override;
  bool prompt(const std::string &prompt, std::string *out_val) override;
  Prompt_answer confirm(const std::string &prompt,
                        Prompt_answer def = Prompt_answer::NO,
                        const std::string &yes_label = "&Yes",
                        const std::string &no_label = "&No",
                        const std::string &alt_label = "") override;
  shcore::Prompt_result prompt_password(const std::string &prompt,
                                        std::string *out_val) override;

 private:
  shcore::Interpreter_delegate *m_ideleg;
};

}  // namespace mysqlsh

#endif  // SRC_MYSQLSH_SHELL_CONSOLE_H_
