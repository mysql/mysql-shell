/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_SHELLCORE_SHELL_CONSOLE_H_
#define MYSQLSHDK_SHELLCORE_SHELL_CONSOLE_H_

#include <string>
#include <vector>

#include "mysqlshdk/include/shellcore/console.h"
#include "scripting/lang_base.h"

// How to use different print methods from console:
//
// - print(), println(), print_value()
//   Use for the primary output of a command. The primary output is usually
//   what the user would want to capture from the stdout, pipe to other commands
//   etc. Errors and status messages meant for the user shouldn't use this.
//
// - print_error()
//   Use for errors that prevent the shell from doing something. Not all errors
//   should use this. Errors that are recovered from or that can be ignored can
//   use print_warning() or print_note() instead.
//   If the shell was a GUI, this would open a popup dialog.
//   Should be short, to the point and potentially parseable.
//
// - print_warning()
//   Use for non-fatal errors that should be receive attention from the user
//   and ideally be corrected.
//   If the shell was a GUI, this would open a popup dialog.
//   Should be short, to the point and potentially parseable.
//
// - print_note()
//   Use for error-like messages that should receive attention (making sure the
//   user is aware of something) from the user but there's nothing that needs to
//   be corrected.
//   If the shell was a GUI, this would open a popup notification.
//   Should be short, to the point and potentially parseable.
//
// - print_status()
//   Use for printing informational messages that tell what the shell is doing,
//   progress text etc.
//   If the shell was a GUI, this would be shown as a statusbar or progress
//   dialog message.
//
// - print_para()
//   Use for printing paragraphs of text explaining a previous message or some
//   general help-like text meant for a human user. An experienced user should
//   be fine completely ignoring these messages.
//
// - print_info()
//   Use for other informational text meant for users.

namespace mysqlsh {

class Shell_console : public IConsole {
 public:
  // Interface methods
  explicit Shell_console(shcore::Interpreter_delegate *deleg);
  ~Shell_console() override;

  virtual bool use_json() const;

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
   *
   * NOTE: Use only for primary output, not side-messages meant for the user.
   */
  void print(const std::string &text) const override;

  /**
   * Sends the provided text to the STDOUT.
   *
   * The produced output will depend on the active output format:
   * - If JSON is active it will produce a JSON object as {"info": <text>}
   * - Otherwise the raw text will be sent to STDOUT followed by a new line.
   *
   * NOTE: Use only for primary output, not side-messages meant for the user.
   */
  void println(const std::string &text = "") const override;

  /**
   * Sends the provided text to the STDERR with an error tag.
   *
   * The produced output will depend on the active output format:
   * - For JSON:  {"error": <text>}
   * - Otherwise: the raw text will be sent to STDERR prepended with an ERROR:
   *   tag.
   */
  void print_error(const std::string &text) const override;

  /**
   * Sends the provided text to the STDERR with an error tag.
   *
   * The produced output will depend on the active output format:
   * - For JSON:  {"error": <text>}
   * - Otherwise: the raw text will be sent to STDERR prepended with an ERROR:
   *   tag.
   */
  void print_diag(const std::string &text) const override;
  /**
   * Sends the provided text to the STDERR with a warning tag.
   *
   * The produced output will depend on the active output format:
   * - For JSON:  {"warning": <text>}
   * - Otherwise: the raw text will be sent to STDERR prepended with an WARNING:
   *   tag.
   */
  void print_warning(const std::string &text) const override;

  /**
   * Sends the provided text to the STDERR with notice formatting.
   *
   * The produced output will depend on the active output format:
   * - For JSON:  {"note": <text>}
   * - Otherwise: the raw text will be sent to STDERR with special color if
   *   supported.
   */
  void print_note(const std::string &text) const override;

  /**
   * Sends the provided text to the STDERR.
   */
  void print_info(const std::string &text = "") const override;

  /**
   * Sends the provided text to the STDERR.
   */
  void print_status(const std::string &text) const override;

  /**
   * Formats and sends the given text to STDERR.
   *
   * This will:
   * - Break lines at 80 char boundaries
   * - Insert a newline after the paragraph
   * - Substitute <<<vars>>>
   */
  virtual void print_para(const std::string &text) const override;

  /**
   * @brief Prompt interface for prompts coming from the user (shell.prompt)
   *
   * @param prompt The main prompt message
   * @param options The options defining the type and structure of the prompt
   * @param out_val The value to be returned as answer to the prompt
   * @return shcore::Prompt_result The prompt resolution state
   */
  shcore::Prompt_result prompt(const std::string &prompt,
                               const shcore::prompt::Prompt_options &options,
                               std::string *out_val) const override;

  /**
   * @brief Prompt interface for prompts coming from Shell code
   *
   * @param prompt The main prompt message
   * @param out_val The value to be returned as answer to the prompt
   * @param validator Optional validation callback to be used during the prompt
   * @param type Optional prompt type: supports all types except Confirm and
   * Select
   * @param title Optional title for the prompt (ignored in the Shell
   * implementation)
   * @param description List of paragraphs to be printed as context for the
   * prompt
   * @param default_value Optional value to be returned in case user provides
   * empty reply
   * @return shcore::Prompt_result The prompt resolution state
   */
  shcore::Prompt_result prompt(
      const std::string &prompt, std::string *out_val,
      Validator validator = nullptr,
      shcore::prompt::Prompt_type type = shcore::prompt::Prompt_type::TEXT,
      const std::string &title = "",
      const std::vector<std::string> &description = {},
      const std::string &default_value = "") const override;
  /**
   * @brief Prompt interface for confirmation prompts coming from Shell code
   *
   * @param prompt The main prompt message
   * @param def Optional identifier of the value to be used as return value if
   * user provides empty reply
   * @param yes_label Optional label to customize the default &Yes label
   * @param no_label Optional label to customize the default &No label
   * @param alt_label Optional label to add a third valid reply
   * @param title Optional title for the prompt (ignored in the Shell
   * implementation)
   * @param description List of paragraphs to be printed as context for the
   * prompt
   * @return Prompt_answer The identifier of the selected answer
   */
  Prompt_answer confirm(
      const std::string &prompt, Prompt_answer def = Prompt_answer::NO,
      const std::string &yes_label = "&Yes",
      const std::string &no_label = "&No", const std::string &alt_label = "",
      const std::string &title = "",
      const std::vector<std::string> &description = {}) const override;
  /**
   * @brief Prompt interface for password prompts coming from Shell code
   *
   * @param prompt The main prompt message
   * @param out_val The value to be returned as answer to the prompt
   * @param validator Optional validation callback to be used during the prompt
   * @param title Optional title for the prompt (ignored in the Shell
   * implementation)
   * @param description List of paragraphs to be printed as context for the
   * prompt
   * @return shcore::Prompt_result The prompt resolution state
   */
  shcore::Prompt_result prompt_password(
      const std::string &prompt, std::string *out_val,
      Validator validator = nullptr, const std::string &title = "",
      const std::vector<std::string> &description = {}) const override;
  /**
   * @brief Prompt interface for select prompts coming from Shell code
   *
   * @param prompt_text The main prompt message
   * @param out_val The value to be returned as answer to the prompt
   * @param items List of eligible options
   * @param default_option 1 based index of the option to be used as default
   * answer if user provides empty reply
   * @param allow_custom Option to enable the user providing a custom option
   * (not from the list)
   * @param validator Optional callback to validate the user reply
   * @param title Optional title for the prompt (ignored in the Shell
   * implementation)
   * @param description List of paragraphs to be printed as context for the
   * prompt
   * @return true
   */
  bool select(const std::string &prompt_text, std::string *out_val,
              const std::vector<std::string> &items, size_t default_option = 0,
              bool allow_custom = false, Validator validator = nullptr,
              const std::string &title = "",
              const std::vector<std::string> &description = {}) const override;

  void print_value(const shcore::Value &value,
                   const std::string &tag) const override;

  std::shared_ptr<IPager> enable_pager() override;
  void enable_global_pager() override;
  void disable_global_pager() override;
  bool is_global_pager_enabled() const override;

  void add_print_handler(shcore::Interpreter_print_handler *) override;

  void remove_print_handler(shcore::Interpreter_print_handler *) override;

  void set_use_colors(bool flag) { m_use_colors = flag; }
  bool use_colors() const { return m_use_colors; }

 private:
  void dump_json(const char *tag, const std::string &s) const;

  void delegate_print(const char *msg) const;

  void delegate_print_error(const char *msg) const;

  void delegate_print_diag(const char *msg) const;

  void delegate(const char *msg,
                shcore::Interpreter_print_handler::Print print) const;

  void on_set_verbose() override;

  void detach_log_hook();

  shcore::Prompt_result call_prompt(
      const std::string &text, const shcore::prompt::Prompt_options &options,
      std::string *ret_val, Validator validator,
      shcore::Interpreter_delegate::Prompt func) const;

  shcore::Interpreter_delegate *m_ideleg;
  std::weak_ptr<IPager> m_current_pager;
  std::shared_ptr<IPager> m_global_pager;
  std::list<shcore::Interpreter_print_handler *> m_print_handlers;
  bool m_use_colors = false;
  bool m_hook_registered = false;
};

/**
 * @brief Custom console for GUI integration
 * NOTE: For now it just reuses the current Shell_console, in the future the
 * output format will change based on GUI needs.
 */
class Gui_shell_console : public Shell_console {
 public:
  explicit Gui_shell_console(shcore::Interpreter_delegate *deleg);
  ~Gui_shell_console() override = default;

  bool use_json() const override { return true; }
};

}  // namespace mysqlsh

#endif  // MYSQLSHDK_SHELLCORE_SHELL_CONSOLE_H_
