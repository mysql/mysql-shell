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

#ifndef MYSQLSHDK_SHELLCORE_SHELL_CONSOLE_H_
#define MYSQLSHDK_SHELLCORE_SHELL_CONSOLE_H_

#include <string>
#include <vector>

#include "mysqlshdk/include/shellcore/console.h"
#include "scripting/lang_base.h"

namespace mysqlsh {

class Shell_console : public IConsole {
 public:
  // Interface methods
  explicit Shell_console(shcore::Interpreter_delegate *deleg);

  /**
   * Sends the provided text to the indicated stream.
   *
   * The produced output will depend on the active output format and provided
   * stream.
   *
   * If the active output format is not JSON or format_json is false the raw
   * text will be sent to the indicated stream.
   *
   * If format_json is true and the active output is JSON, a JSON object will
   * be created depending on the target stream:
   *
   * - STDOUT: {"info", <text> }
   * - STDERR: {"error", <text> }
   *
   * @param text   data to the indicated stream.
   * @param stream where the text will be sent.
   * @param format_json indicates the text should be automatically
   * formatted as JSON when shell output format is json or json/raw.
   */
  void raw_print(const std::string &text, Output_stream stream,
                 bool format_json = true) const override;

  /**
   * Sends the provided text to the STDOUT.
   *
   * The produced output will depend on the active output format:
   * - If JSON is active it will produce a JSON object as {"info": <text>}
   * - Otherwise the raw text will be sent to STDOUT.
   */
  void print(const std::string &text) const override;

  /**
   * Sends the provided text to the STDOUT.
   *
   * The produced output will depend on the active output format:
   * - If JSON is active it will produce a JSON object as {"info": <text>}
   * - Otherwise the raw text will be sent to STDOUT followed by a new line.
   */
  void println(const std::string &text = "") const override;

  /**
   * Sends the provided text to the STDOUT with an error tag.
   *
   * The produced output will depend on the active output format:
   * - For JSON:  {"error": <text>}
   * - Otherwise: the raw text will be sent to STDOUT prepended with an ERROR:
   *   tag.
   */
  void print_error(const std::string &text) const override;

  /**
   * Sends the provided text to the STDOUT with a warning tag.
   *
   * The produced output will depend on the active output format:
   * - For JSON:  {"warning": <text>}
   * - Otherwise: the raw text will be sent to STDOUT prepended with an WARNING:
   *   tag.
   */
  void print_warning(const std::string &text) const override;

  /**
   * Sends the provided text to the STDOUT with notice formatting.
   *
   * The produced output will depend on the active output format:
   * - For JSON:  {"note": <text>}
   * - Otherwise: the raw text will be sent to STDOUT with special color if
   *   supported.
   */
  void print_note(const std::string &text) const override;
  void print_info(const std::string &text) const override;
  bool prompt(const std::string &prompt, std::string *out_val) const override;
  Prompt_answer confirm(const std::string &prompt,
                        Prompt_answer def = Prompt_answer::NO,
                        const std::string &yes_label = "&Yes",
                        const std::string &no_label = "&No",
                        const std::string &alt_label = "") const override;
  shcore::Prompt_result prompt_password(const std::string &prompt,
                                        std::string *out_val) const override;
  void print_value(const shcore::Value &value,
                   const std::string &tag) const override;

  std::shared_ptr<IPager> enable_pager() override;
  void enable_global_pager() override;
  void disable_global_pager() override;
  bool is_global_pager_enabled() const override;

 private:
  shcore::Interpreter_delegate *m_ideleg;
  std::weak_ptr<IPager> m_current_pager;
  std::shared_ptr<IPager> m_global_pager;
};

}  // namespace mysqlsh

#endif  // MYSQLSHDK_SHELLCORE_SHELL_CONSOLE_H_
