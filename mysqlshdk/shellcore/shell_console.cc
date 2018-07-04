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

#include "mysqlshdk/shellcore/shell_console.h"

#include <memory>
#include <string>
#include <utility>

#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "scripting/shexcept.h"
#include "shellcore/base_shell.h"
#include "shellcore/shell_options.h"

namespace mysqlsh {
namespace {
std::string json_obj(const char *key, const std::string &value) {
  shcore::JSON_dumper dumper(
      mysqlsh::current_shell_options()->get().output_format == "json");
  dumper.start_object();
  dumper.append_string(key);
  dumper.append_string(value);
  dumper.end_object();

  return dumper.str() + "\n";
}

std::string json_obj(const char *key, const shcore::Value &info) {
  shcore::JSON_dumper dumper(
      mysqlsh::current_shell_options()->get().output_format == "json");
  dumper.start_object();
  dumper.append_value(key, info);
  dumper.end_object();

  return dumper.str() + "\n";
}

bool use_json() {
  std::string format = mysqlsh::current_shell_options()->get().output_format;
  return format.find("json") != std::string::npos;
}
}  // namespace

#ifdef _WIN32

#define popen _popen
#define pclose _pclose

#endif  // _WIN32

class Shell_pager : public IPager {
 private:
  using Delegate = shcore::Interpreter_delegate;

 public:
  explicit Shell_pager(Delegate *delegate)
      : m_delegate{delegate}, m_original_delegate{*delegate} {
    const auto &options = current_shell_options()->get();

    if (options.interactive && !options.pager.empty()) {
      m_pager = popen(options.pager.c_str(), "w");

      if (!m_pager) {
        current_console()->print_error(
            "Failed to open pager \"" + options.pager +
            "\", error: " + shcore::errno_to_string(errno) + ".");
      }
    }

    if (m_pager) {
      m_delegate->user_data = this;

      if (m_delegate->print) {
        m_delegate->print = print;
      }

      if (m_delegate->prompt) {
        m_delegate->prompt = prompt;
      }

      if (m_delegate->password) {
        m_delegate->password = password;
      }

      if (m_delegate->print_error) {
        m_delegate->print_error = print_error;
      }
    }
  }

  ~Shell_pager() {
    if (m_pager) {
      // disable pager and restore original delegate before printing anything
      const auto status = pclose(m_pager);
      m_pager = nullptr;
      *m_delegate = m_original_delegate;

      // inform of any errors
#ifdef _WIN32
      const auto exit_code = status;
      const bool error_occurred = 0 != exit_code;
#else   // !_WIN32
      const auto exit_code = WEXITSTATUS(status);
      const bool error_occurred = WIFEXITED(status) && 0 != exit_code;
#endif  // !_WIN32
      if (error_occurred) {
        current_console()->print_error(
            "Pager \"" + current_shell_options()->get().pager +
            "\" returned exit code: " + std::to_string(exit_code) + ".");
      }
    }
  }

 private:
  static void print(void *user_data, const char *text) {
    const auto self = static_cast<Shell_pager *>(user_data);
    fprintf(self->m_pager, "%s", text);
    fflush(self->m_pager);
  }

  static shcore::Prompt_result prompt(void *user_data, const char *prompt,
                                      std::string *ret_input) {
    const auto self = static_cast<Shell_pager *>(user_data);
    return self->call_delegate(&Delegate::prompt, prompt, ret_input);
  }

  static shcore::Prompt_result password(void *user_data, const char *prompt,
                                        std::string *ret_password) {
    const auto self = static_cast<Shell_pager *>(user_data);
    return self->call_delegate(&Delegate::password, prompt, ret_password);
  }

  static void print_error(void *user_data, const char *text) {
    const auto self = static_cast<Shell_pager *>(user_data);
    self->call_delegate(&Delegate::print_error, text);
  }

  template <typename F, typename... Args>
  auto call_delegate(F func, Args &&... args)
      -> decltype((std::declval<Delegate>().*
                   func)(nullptr, std::forward<Args>(args)...)) {
    return (m_original_delegate.*func)(m_original_delegate.user_data,
                                       std::forward<Args>(args)...);
  }

  Delegate *m_delegate = nullptr;

  Delegate m_original_delegate;

  FILE *m_pager = nullptr;
};

Shell_console::Shell_console(shcore::Interpreter_delegate *deleg)
    : m_ideleg(deleg) {}

void Shell_console::raw_print(const std::string &text,
                              Output_stream stream) const {
  std::string tag = stream == Output_stream::STDOUT ? "info" : "error";
  std::string output = use_json() ? json_obj(tag.c_str(), text) : text;

  using Print_func = void (*)(void *, const char *);
  Print_func print =
      stream == Output_stream::STDOUT ? m_ideleg->print : m_ideleg->print_error;

  print(m_ideleg->user_data, output.c_str());

  log_debug("%s", text.c_str());
}

void Shell_console::print(const std::string &text) const {
  raw_print(text, Output_stream::STDOUT);
}

void Shell_console::println(const std::string &text) const {
  if (use_json() && !text.empty()) {
    m_ideleg->print(m_ideleg->user_data, json_obj("info", text).c_str());
  } else {
    m_ideleg->print(m_ideleg->user_data, (text + "\n").c_str());
  }
  if (!text.empty()) log_debug("%s", text.c_str());
}

void Shell_console::print_error(const std::string &text) const {
  if (use_json()) {
    m_ideleg->print(m_ideleg->user_data, json_obj("error", text).c_str());
  } else {
    m_ideleg->print(
        m_ideleg->user_data,
        (mysqlshdk::textui::error("ERROR: ") + text + "\n").c_str());
  }
  log_error("%s", text.c_str());
}

void Shell_console::print_warning(const std::string &text) const {
  if (use_json()) {
    m_ideleg->print(m_ideleg->user_data, json_obj("warning", text).c_str());
  } else {
    m_ideleg->print(
        m_ideleg->user_data,
        (mysqlshdk::textui::warning("WARNING: ") + text + "\n").c_str());
  }
  log_warning("%s", text.c_str());
}

void Shell_console::print_note(const std::string &text) const {
  if (use_json()) {
    m_ideleg->print(m_ideleg->user_data, json_obj("note", text).c_str());
  } else {
    m_ideleg->print(m_ideleg->user_data,
                    mysqlshdk::textui::notice(text + "\n").c_str());
  }
  log_info("%s", text.c_str());
}

void Shell_console::print_info(const std::string &text) const {
  if (use_json()) {
    m_ideleg->print(m_ideleg->user_data, json_obj("info", text).c_str());
  } else {
    m_ideleg->print(m_ideleg->user_data, (text + "\n").c_str());
  }
  log_info("%s", text.c_str());
}

bool Shell_console::prompt(const std::string &prompt,
                           std::string *ret_val) const {
  std::string text;
  if (use_json()) {
    text = json_obj("prompt", prompt);
  } else {
    text = mysqlshdk::textui::bold(prompt);
  }
  shcore::Prompt_result result =
      m_ideleg->prompt(m_ideleg->user_data, text.c_str(), ret_val);
  if (result == shcore::Prompt_result::Cancel)
    throw shcore::cancelled("Cancelled");
  if (result == shcore::Prompt_result::Ok) return true;
  return false;
}

static char process_label(const std::string &s, std::string *out_display,
                          std::string *out_clean_text) {
  out_display->clear();
  if (s.empty()) return 0;

  char letter = 0;
  char prev = 0;
  for (char c : s) {
    if (prev == '&') letter = c;
    if (c != '&') {
      if (prev == '&') {
        out_display->push_back('[');
        out_display->push_back(c);
        out_display->push_back(']');
      } else {
        out_display->push_back(c);
      }
      out_clean_text->push_back(c);
    }
    prev = c;
  }
  return letter;
}

Prompt_answer Shell_console::confirm(const std::string &prompt,
                                     Prompt_answer def,
                                     const std::string &yes_label,
                                     const std::string &no_label,
                                     const std::string &alt_label) const {
  assert(def != Prompt_answer::ALT || !alt_label.empty());

  Prompt_answer final_ans = Prompt_answer::NONE;
  std::string ans;
  char yes_letter = 0;
  char no_letter = 0;
  char alt_letter = 0;
  std::string def_str;
  std::string clean_yes_text, clean_no_text, clean_alt_text;
  if (yes_label == "&Yes" && no_label == "&No" && alt_label.empty()) {
    std::string display_text;
    yes_letter = process_label(yes_label, &display_text, &clean_yes_text);
    no_letter = process_label(no_label, &display_text, &clean_no_text);

    if (def == Prompt_answer::YES)
      def_str = "[Y/n]: ";
    else if (def == Prompt_answer::NO)
      def_str = "[y/N]: ";
    else
      def_str = "[y/n]: ";
  } else {
    std::string display_text;
    yes_letter = process_label(yes_label, &display_text, &clean_yes_text);
    if (!display_text.empty()) def_str.append(display_text).append("/");

    no_letter = process_label(no_label, &display_text, &clean_no_text);
    if (!display_text.empty()) def_str.append(display_text).append("/");

    alt_letter = process_label(alt_label, &display_text, &clean_alt_text);
    if (!display_text.empty()) def_str.append(display_text).append("/");

    def_str.pop_back();  // erase trailing /

    switch (def) {
      case Prompt_answer::YES:
        def_str.append(" (default ").append(clean_yes_text).append("): ");
        break;

      case Prompt_answer::NO:
        def_str.append(" (default ").append(clean_no_text).append("): ");
        break;

      case Prompt_answer::ALT:
        def_str.append(" (default ").append(clean_alt_text).append("): ");
        break;

      default:
        break;
    }
  }

  while (final_ans == Prompt_answer::NONE) {
    if (this->prompt(prompt + " " + def_str, &ans)) {
      if (ans.empty()) {
        final_ans = def;
      } else {
        if (shcore::str_caseeq(ans, std::string{&yes_letter, 1}) ||
            shcore::str_caseeq(ans, clean_yes_text)) {
          final_ans = Prompt_answer::YES;
        } else if (!clean_no_text.empty() &&
                   (shcore::str_caseeq(ans, std::string{&no_letter, 1}) ||
                    shcore::str_caseeq(ans, clean_no_text))) {
          final_ans = Prompt_answer::NO;
        } else if (!clean_alt_text.empty() &&
                   (shcore::str_caseeq(ans, std::string{&alt_letter, 1}) ||
                    shcore::str_caseeq(ans, clean_alt_text))) {
          final_ans = Prompt_answer::ALT;
        } else {
          println("\nPlease pick an option out of " + def_str);
        }
      }
    } else {
      break;
    }
  }
  return final_ans;
}  // namespace mysqlsh

shcore::Prompt_result Shell_console::prompt_password(
    const std::string &prompt, std::string *out_val) const {
  std::string text;
  if (use_json()) {
    text = json_obj("password", prompt);
  } else {
    text = mysqlshdk::textui::bold(prompt);
  }
  return m_ideleg->password(m_ideleg->user_data, text.c_str(), out_val);
}

void Shell_console::print_value(const shcore::Value &value,
                                const std::string &tag) const {
  std::string output;
  bool add_new_line = true;
  // When using JSON output ALL must be JSON
  std::string format = mysqlsh::current_shell_options()->get().output_format;
  if (use_json()) {
    // If no tag is provided, prints the JSON representation of the Value
    if (tag.empty()) {
      output = value.json(format == "json");
    } else {
      if (value.type == shcore::String)
        output = json_obj(tag.c_str(), value.as_string());
      else
        output = json_obj(tag.c_str(), value);

      add_new_line = false;
    }
  } else {
    if (tag == "error" && value.type == shcore::Map) {
      output = "ERROR: ";
      shcore::Value::Map_type_ref error_map = value.as_map();

      if (error_map->has_key("code")) {
        // message.append(" ");
        output.append(((*error_map)["code"].repr()));

        if (error_map->has_key("state") && (*error_map)["state"])
          output.append(" (" + (*error_map)["state"].as_string() + ")");

        output.append(": ");
      }

      if (error_map->has_key("message"))
        output.append((*error_map)["message"].as_string());
      else
        output.append("?");
    } else {
      output = value.descr(true);
    }
  }

  if (add_new_line) output += "\n";

  if (tag == "error")
    m_ideleg->print_error(m_ideleg->user_data, output.c_str());
  else
    m_ideleg->print(m_ideleg->user_data, output.c_str());
}

std::shared_ptr<IPager> Shell_console::enable_pager() {
  std::shared_ptr<IPager> pager = m_current_pager.lock();

  if (!pager) {
    pager = std::make_shared<Shell_pager>(m_ideleg);
    m_current_pager = pager;
  }

  return pager;
}

void Shell_console::enable_global_pager() { m_global_pager = enable_pager(); }

void Shell_console::disable_global_pager() { m_global_pager.reset(); }

bool Shell_console::is_global_pager_enabled() const {
  return nullptr != m_global_pager;
}

}  // namespace mysqlsh
