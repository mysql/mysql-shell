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

#include "mysqlshdk/shellcore/shell_console.h"

#include <cstdio>
#include <memory>
#include <string>
#include <utility>

#ifdef __sun
#include <sys/wait.h>
#endif

#include "mysqlshdk/libs/textui/term_vt100.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/strformat.h"
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
      mysqlsh::current_shell_options()->get().wrap_json == "json");
  dumper.start_object();
  dumper.append_string(key);
  dumper.append_string(value);
  dumper.end_object();

  return dumper.str() + "\n";
}

std::string json_obj(const char *key, const shcore::Value &info) {
  shcore::JSON_dumper dumper(
      mysqlsh::current_shell_options()->get().wrap_json == "json");
  dumper.start_object();
  dumper.append_value(key, info);
  dumper.end_object();

  return dumper.str() + "\n";
}

inline bool use_json() {
  return mysqlsh::current_shell_options()->get().wrap_json != "off";
}
}  // namespace

#ifdef _WIN32

#define popen _popen
#define pclose _pclose

#endif  // _WIN32

class Shell_pager : public IPager {
 public:
  Shell_pager() : m_handler(this, &Shell_pager::print, nullptr, nullptr) {
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
      current_console()->add_print_handler(&m_handler);
    }
  }

  ~Shell_pager() {
    if (m_pager) {
      // disable pager and restore original delegate before printing anything
      const auto status = pclose(m_pager);
      m_pager = nullptr;

      current_console()->remove_print_handler(&m_handler);

      // inform of any errors
      std::string error;

      if (!shcore::verify_status_code(status, &error)) {
        current_console()->print_error("Pager \"" +
                                       current_shell_options()->get().pager +
                                       "\" " + error + ".");
      }
    }
  }

 private:
  static bool print(void *user_data, const char *text) {
    const auto self = static_cast<Shell_pager *>(user_data);
    fprintf(self->m_pager, "%s", text);
    fflush(self->m_pager);
    return true;
  }

  shcore::Interpreter_print_handler m_handler;

  FILE *m_pager = nullptr;
};

namespace {
int g_dont_log = 0;

void log_hook(const shcore::Logger::Log_entry &log, void *data) {
  Shell_console *self = reinterpret_cast<Shell_console *>(data);

  if (self->get_verbose() > 0) {
    const char *show_prefix = nullptr;

    // verbose=1, everything up to INFO
    // verbose=2, DEBUG
    // verbose=3, DEBUG2
    // verbose=4, DEBUG3
    switch (self->get_verbose()) {
      case 4:
        if (log.level == shcore::Logger::LOG_DEBUG3) show_prefix = "";
        // fallthrough
      case 3:
        if (log.level == shcore::Logger::LOG_DEBUG2) show_prefix = "";
        // fallthrough
      case 2:
        if (log.level == shcore::Logger::LOG_DEBUG) show_prefix = "";
        // fallthrough
      case 1:
        switch (log.level) {
          case shcore::Logger::LOG_INFO:
            show_prefix = "";
            break;
          case shcore::Logger::LOG_WARNING:
            show_prefix = "warning: ";
            break;
          case shcore::Logger::LOG_ERROR:
          case shcore::Logger::LOG_INTERNAL_ERROR:
            show_prefix = "error: ";
            break;
          default:
            break;
        }
        break;
    }

    if (show_prefix && g_dont_log == 0) {
      static char color_on[16] = {0};
      static char color_off[16] = {0};
      std::string ts = mysqlshdk::utils::fmttime(
          "%Y-%m-%dT%H:%M:%SZ", mysqlshdk::utils::Time_type::LOCAL,
          &log.timestamp);

      if (self->use_colors() && color_on[0] == '\0') {
        strcpy(color_on,
               mysqlshdk::vt100::attr(-1, -1, mysqlshdk::vt100::Dim).c_str());
        strcpy(color_off, mysqlshdk::vt100::attr().c_str());
      } else if (!self->use_colors() && color_on[0] != '\0') {
        strcpy(color_on, "");
        strcpy(color_off, "");
      }

      if (log.domain && *log.domain)
        self->raw_print(shcore::str_format("%sverbose: %s: %s%s: %s%s\n",
                                           color_on, ts.c_str(), show_prefix,
                                           log.domain, log.message, color_off),
                        Output_stream::STDERR);
      else
        self->raw_print(
            shcore::str_format("%sverbose: %s: %s%s%s\n", color_on, ts.c_str(),
                               show_prefix, log.message, color_off),
            Output_stream::STDERR);
    }
  }
}
}  // namespace

Shell_console::Shell_console(shcore::Interpreter_delegate *deleg)
    : m_ideleg(deleg) {
  m_print_handlers.emplace_back(deleg);

#ifndef _WIN32
  if (isatty(2)) {
    m_use_colors = true;
  }
#endif
}

Shell_console::~Shell_console() { detach_log_hook(); }

void Shell_console::dump_json(const char *tag, const std::string &s) const {
  delegate_print(json_obj(tag, s).c_str());
}

void Shell_console::raw_print(const std::string &text, Output_stream stream,
                              bool format_json) const {
  using Print_func = void (Shell_console::*)(const char *) const;
  Print_func func = stream == Output_stream::STDOUT
                        ? &Shell_console::delegate_print
                        : &Shell_console::delegate_print_diag;

  // format_json=false means bypass JSON wrapping
  if (use_json() && format_json) {
    const char *tag = stream == Output_stream::STDOUT ? "info" : "error";
    dump_json(tag, text);
  } else {
    (this->*func)(text.c_str());
  }
}

void Shell_console::print(const std::string &text) const {
  if (use_json()) {
    if (!text.empty()) dump_json("info", text);
  } else {
    raw_print(text, Output_stream::STDOUT);
  }
}

void Shell_console::println(const std::string &text) const {
  if (use_json()) {
    if (!text.empty()) dump_json("info", text);
  } else {
    raw_print(text + "\n", Output_stream::STDOUT);
  }
}

namespace {
std::string format_vars(const std::string &text) {
  return shcore::str_subvars(
      text,
      [](const std::string &var) {
        return shcore::get_member_name(var, shcore::current_naming_style());
      },
      "<<<", ">>>");
}

std::string fit_screen(const std::string &text) {
  // int r, c;
  // if (!mysqlshdk::vt100::get_screen_size(&r, &c)) c = 80;

  // if (c >= 120)
  //   c = 120;
  // else if (c >= 80)
  //   c = 80;
  // else
  //   return text;
  constexpr const int c = 80;

  return mysqlshdk::textui::format_markup_text(shcore::str_split(text, "\n"), c,
                                               0, false);
}

}  // namespace

void Shell_console::print_diag(const std::string &text) const {
  std::string ftext = format_vars(text);
  if (use_json()) {
    dump_json("error", ftext);
  } else {
    delegate_print_diag(ftext.c_str());
  }
  if (g_dont_log == 0) {
    g_dont_log++;
    log_debug("%s", ftext.c_str());
    g_dont_log--;
  }
}

void Shell_console::print_error(const std::string &text) const {
  std::string ftext = format_vars(text);
  if (use_json()) {
    dump_json("error", ftext);
  } else {
    delegate_print_error(
        (mysqlshdk::textui::error("ERROR: ") + ftext + "\n").c_str());
  }
  if (g_dont_log == 0) {
    g_dont_log++;
    log_error("%s", ftext.c_str());
    g_dont_log--;
  }
}

void Shell_console::print_warning(const std::string &text) const {
  std::string ftext = format_vars(text);
  if (use_json()) {
    dump_json("warning", ftext);
  } else {
    delegate_print_error(
        (mysqlshdk::textui::warning("WARNING: ") + ftext + "\n").c_str());
  }
  if (g_dont_log == 0) {
    g_dont_log++;
    log_warning("%s", ftext.c_str());
    g_dont_log--;
  }
}

void Shell_console::print_note(const std::string &text) const {
  std::string ftext = format_vars(text);
  if (use_json()) {
    dump_json("note", ftext);
  } else {
    delegate_print_error(
        (mysqlshdk::textui::notice("NOTE: ") + ftext + "\n").c_str());
  }
  if (g_dont_log == 0) {
    g_dont_log++;
    log_info("%s", text.c_str());
    g_dont_log--;
  }
}

void Shell_console::print_info(const std::string &text) const {
  std::string ftext = format_vars(text);
  if (use_json()) {
    dump_json("info", ftext);
  } else {
    delegate_print_error((ftext + "\n").c_str());
  }
  if (g_dont_log == 0 && !text.empty()) {
    g_dont_log++;
    log_debug("%s", ftext.c_str());
    g_dont_log--;
  }
}

void Shell_console::print_status(const std::string &text) const {
  std::string ftext = format_vars(text);
  if (use_json()) {
    dump_json("status", ftext);
  } else {
    delegate_print_error((ftext + "\n").c_str());
  }
  if (g_dont_log == 0 && !text.empty()) {
    g_dont_log++;
    log_debug("%s", ftext.c_str());
    g_dont_log--;
  }
}

void Shell_console::print_para(const std::string &text) const {
  std::string ftext = format_vars(text);
  if (use_json()) {
    dump_json("info", ftext);
  } else {
    delegate_print_error((fit_screen(ftext) + "\n\n").c_str());
  }
  if (g_dont_log == 0 && !text.empty()) {
    g_dont_log++;
    log_debug2("%s", ftext.c_str());
    g_dont_log--;
  }
}

bool Shell_console::prompt(const std::string &prompt, std::string *ret_val,
                           Validator validator) const {
  std::string text;
  if (use_json()) {
    text = json_obj("prompt", prompt);
  } else {
    text = mysqlshdk::textui::bold(prompt);
  }

  while (1) {
    shcore::Prompt_result result = m_ideleg->prompt(text.c_str(), ret_val);
    if (result == shcore::Prompt_result::Cancel)
      throw shcore::cancelled("Cancelled");
    if (result == shcore::Prompt_result::Ok) {
      if (validator) {
        std::string msg = validator(*ret_val);

        if (msg.empty()) return true;

        print_warning(msg);
      } else {
        return true;
      }
    }
  }
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

bool Shell_console::select(const std::string &prompt_text, std::string *result,
                           const std::vector<std::string> &options,
                           size_t default_option, bool allow_custom,
                           Validator validator) const {
  std::string answer;
  std::string default_str;
  std::string text(prompt_text);
  result->clear();

  if (default_option != 0)
    text += " [" + std::to_string(default_option) + "]: ";

  int index = 1;
  for (const auto &option : options)
    println(shcore::str_format("  %d) %s", index++, option.c_str()));

  println();

  bool valid = false;

  mysqlshdk::utils::nullable<std::string> good_answer;

  while (!valid && good_answer.is_null()) {
    if (prompt(text, &answer)) {
      int option = static_cast<int>(default_option);

      try {
        if (!answer.empty())
          option = std::stoi(answer);
        else
          valid = allow_custom;

        // The selection is a number from the list
        if (option > 0 && option <= static_cast<int>(options.size())) {
          answer = options[option - 1];
          valid = true;
        }
      } catch (const std::exception &err) {
        // User typed something else and it is allowed
        valid = allow_custom;
      }

      // If there's a validator, the answer should be validated
      std::string warning;
      if (valid && validator) {
        warning = validator(answer);
        valid = warning.empty();
      } else if (!valid) {
        warning = "Invalid option selected.";
      }

      if (valid)
        good_answer = answer;
      else
        print_warning(warning);
    } else {
      break;
    }
  }

  if (!good_answer.is_null()) *result = *good_answer;

  return valid;
}

shcore::Prompt_result Shell_console::prompt_password(
    const std::string &prompt, std::string *out_val,
    Validator validator) const {
  std::string text;
  if (use_json()) {
    text = json_obj("password", prompt);
  } else {
    text = mysqlshdk::textui::bold(prompt);
  }

  shcore::Prompt_result result;
  bool valid = true;
  do {
    result = m_ideleg->password(text.c_str(), out_val);

    if (result == shcore::Prompt_result::Ok) {
      if (validator) {
        std::string msg = validator(*out_val);

        valid = msg.empty();

        if (!valid) print_warning(msg);
      }
    }
  } while (result == shcore::Prompt_result::Ok && !valid);

  return result;
}

void Shell_console::print_value(const shcore::Value &value,
                                const std::string &tag) const {
  std::string output;
  // When using JSON output ALL must be JSON
  if (use_json()) {
    // If no tag is provided, prints the JSON representation of the Value
    if (tag.empty()) {
      output = value.json(mysqlsh::current_shell_options()->get().wrap_json ==
                          "json") +
               "\n";
    } else {
      if (value.type == shcore::String)
        output = json_obj(tag.c_str(), value.get_string());
      else
        output = json_obj(tag.c_str(), value);
    }

    // JSON output is always printed to stdout
    delegate_print(output.c_str());
  } else {
    if (tag == "error" && value.type == shcore::Map) {
      output = "ERROR";
      shcore::Value::Map_type_ref error_map = value.as_map();

      if (error_map->has_key("code")) {
        output.append(": ");
        // message.append(" ");
        output.append(((*error_map)["code"].repr()));

        if (error_map->has_key("state") && (*error_map)["state"])
          output.append(" (" + (*error_map)["state"].get_string() + ")");
      }

      if (error_map->has_key("line")) {
        output.append(" at line " + std::to_string(error_map->get_int("line")));
      }
      output.append(": ");
      if (error_map->has_key("message"))
        output.append((*error_map)["message"].get_string());
      else
        output.append("?");
    } else {
      output = value.descr(true);
    }

    output += "\n";

    if (tag == "error")
      delegate_print_diag(output.c_str());
    else
      delegate_print(output.c_str());
  }
}

std::shared_ptr<IPager> Shell_console::enable_pager() {
  std::shared_ptr<IPager> pager = m_current_pager.lock();

  if (!pager) {
    pager = std::make_shared<Shell_pager>();
    m_current_pager = pager;
  }

  return pager;
}

void Shell_console::enable_global_pager() { m_global_pager = enable_pager(); }

void Shell_console::disable_global_pager() { m_global_pager.reset(); }

bool Shell_console::is_global_pager_enabled() const {
  return nullptr != m_global_pager;
}

void Shell_console::add_print_handler(shcore::Interpreter_print_handler *h) {
  m_print_handlers.emplace_back(h);
}

void Shell_console::remove_print_handler(shcore::Interpreter_print_handler *h) {
  m_print_handlers.remove(h);
}

void Shell_console::delegate_print(const char *msg) const {
  delegate(msg, &shcore::Interpreter_print_handler::print);
}

void Shell_console::delegate_print_error(const char *msg) const {
  delegate(msg, &shcore::Interpreter_print_handler::print_error);
}

void Shell_console::delegate_print_diag(const char *msg) const {
  delegate(msg, &shcore::Interpreter_print_handler::print_diag);
}

void Shell_console::delegate(
    const char *msg, shcore::Interpreter_print_handler::Print func) const {
  for (auto it = m_print_handlers.crbegin(); it != m_print_handlers.crend();
       ++it) {
    if ((*it->*func)(msg)) {
      return;
    }
  }
}

void Shell_console::on_set_verbose() {
  if (get_verbose() > 0) {
    if (!m_hook_registered) {
      // if verbose is enabled, capture logging output and show them in the
      // console
      shcore::Logger::singleton()->attach_log_hook(log_hook, this, true);
      m_hook_registered = true;
    }
  } else {
    detach_log_hook();
  }
}

void Shell_console::detach_log_hook() {
  if (m_hook_registered) {
    shcore::Logger::singleton()->detach_log_hook(log_hook);
    m_hook_registered = false;
  }
}

}  // namespace mysqlsh
