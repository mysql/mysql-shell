/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
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
#include <unordered_map>
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

std::string json_obj(
    const char *key, const std::string &value,
    const std::vector<std::string> &description = {},
    const std::unordered_map<std::string, std::string> &additional = {},
    const std::function<void(shcore::JSON_dumper *)> &callback = {}) {
  const auto &options = mysqlsh::current_shell_options()->get();
  shcore::JSON_dumper dumper(options.wrap_json == "json", options.binary_limit);
  dumper.start_object();
  dumper.append_string(key);
  dumper.append_string(value);

  // Inserts the additional entries into the object
  for (const auto &entry : additional) {
    dumper.append_string(entry.first);
    dumper.append_string(entry.second);
  }

  // Inserts the description into the object
  if (!description.empty()) {
    dumper.append_string("description");
    dumper.start_array();
    for (const auto &desc : description) {
      dumper.append_string(desc);
    }

    dumper.end_array();
  }

  // Allows defining yet additional entries into the object
  if (callback) {
    callback(&dumper);
  }

  dumper.end_object();

  return dumper.str() + "\n";
}

std::string json_obj(const char *key, const shcore::Value &info) {
  const auto &options = mysqlsh::current_shell_options()->get();
  shcore::JSON_dumper dumper(options.wrap_json == "json", options.binary_limit);
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

    if (show_prefix && shcore::current_logger()->log_allowed()) {
      static char color_on[16] = {0};
      static char color_off[16] = {0};
      std::string ts = mysqlshdk::utils::fmttime(
          "%Y-%m-%dT%H:%M:%SZ", mysqlshdk::utils::Time_type::LOCAL,
          &log.timestamp);

      if (self->use_colors() && color_on[0] == '\0') {
        snprintf(color_on, sizeof(color_on), "%s",
                 mysqlshdk::vt100::attr(-1, -1, mysqlshdk::vt100::Dim).c_str());
        snprintf(color_off, sizeof(color_off), "%s",
                 mysqlshdk::vt100::attr().c_str());
      } else if (!self->use_colors() && color_on[0] != '\0') {
        memset(color_on, 0, sizeof(color_on));
        memset(color_off, 0, sizeof(color_off));
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
    if (!text.empty()) dump_json("info", text + "\n");
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
  if (shcore::current_logger()->log_allowed()) {
    shcore::Log_reentrant_protector lock;
    log_debug("%s", ftext.c_str());
  }
}

void Shell_console::print_error(const std::string &text) const {
  std::string ftext = format_vars(text);
  if (use_json()) {
    dump_json("error", ftext + "\n");
  } else {
    delegate_print_error(
        (mysqlshdk::textui::error("ERROR: ") + ftext + "\n").c_str());
  }
  if (shcore::current_logger()->log_allowed()) {
    shcore::Log_reentrant_protector lock;
    log_error("%s", ftext.c_str());
  }
}

void Shell_console::print_warning(const std::string &text) const {
  std::string ftext = format_vars(text);
  if (use_json()) {
    dump_json("warning", ftext + "\n");
  } else {
    delegate_print_error(
        (mysqlshdk::textui::warning("WARNING: ") + ftext + "\n").c_str());
  }
  if (shcore::current_logger()->log_allowed()) {
    shcore::Log_reentrant_protector lock;
    log_warning("%s", ftext.c_str());
  }
}

void Shell_console::print_note(const std::string &text) const {
  std::string ftext = format_vars(text);
  if (use_json()) {
    dump_json("note", ftext + "\n");
  } else {
    delegate_print_error(
        (mysqlshdk::textui::notice("NOTE: ") + ftext + "\n").c_str());
  }
  if (shcore::current_logger()->log_allowed()) {
    shcore::Log_reentrant_protector lock;
    log_info("%s", text.c_str());
  }
}

void Shell_console::print_info(const std::string &text) const {
  std::string ftext = format_vars(text);
  if (use_json()) {
    dump_json("info", ftext + "\n");
  } else {
    delegate_print_error((ftext + "\n").c_str());
  }
  if (shcore::current_logger()->log_allowed() && !text.empty()) {
    shcore::Log_reentrant_protector lock;
    log_debug("%s", ftext.c_str());
  }
}

void Shell_console::print_status(const std::string &text) const {
  std::string ftext = format_vars(text);
  if (use_json()) {
    dump_json("status", ftext + "\n");
  } else {
    delegate_print_error((ftext + "\n").c_str());
  }
  if (shcore::current_logger()->log_allowed() && !text.empty()) {
    shcore::Log_reentrant_protector lock;
    log_debug("%s", ftext.c_str());
  }
}

void Shell_console::print_para(const std::string &text) const {
  std::string ftext = format_vars(text);
  if (use_json()) {
    dump_json("info", ftext + "\n\n");
  } else {
    delegate_print_error((fit_screen(ftext) + "\n\n").c_str());
  }
  if (shcore::current_logger()->log_allowed() && !text.empty()) {
    shcore::Log_reentrant_protector lock;
    log_debug2("%s", ftext.c_str());
  }
}

shcore::Prompt_result Shell_console::call_prompt(
    const std::string &text, const shcore::prompt::Prompt_options &options,
    std::string *out_val, Validator validator,
    shcore::Interpreter_delegate::Prompt func) const {
  shcore::Prompt_result result;
  bool valid = true;
  do {
    result = (m_ideleg->*func)(text.c_str(), options, out_val);

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

shcore::Prompt_result Shell_console::prompt(
    const std::string &prompt, const shcore::prompt::Prompt_options &options,
    std::string *out_val) const {
  return call_prompt(prompt, options, out_val, {},
                     &shcore::Interpreter_delegate::prompt);
}

shcore::Prompt_result Shell_console::prompt(
    const std::string &prompt, std::string *ret_val, Validator validator,
    shcore::prompt::Prompt_type type, const std::string &title,
    const std::vector<std::string> &description,
    const std::string &default_value) const {
  // Other prompt types mus use their specific Shell_console function
  assert(type == shcore::prompt::Prompt_type::TEXT ||
         type == shcore::prompt::Prompt_type::PASSWORD ||
         type == shcore::prompt::Prompt_type::FILEOPEN ||
         type == shcore::prompt::Prompt_type::FILESAVE ||
         type == shcore::prompt::Prompt_type::DIRECTORY);

  shcore::prompt::Prompt_options options;
  options.title = title;
  options.type = type;
  options.description = description;
  options.default_value = shcore::Value(default_value);

  options.done();

  return call_prompt(prompt, options, ret_val, validator,
                     &shcore::Interpreter_delegate::prompt);
}

Prompt_answer Shell_console::confirm(
    const std::string &prompt, Prompt_answer def, const std::string &yes_label,
    const std::string &no_label, const std::string &alt_label,
    const std::string &title,
    const std::vector<std::string> &description) const {
  assert(def != Prompt_answer::ALT || !alt_label.empty());

  shcore::prompt::Prompt_options options;
  options.title = title;
  options.type = shcore::prompt::Prompt_type::CONFIRM;
  options.description = description;
  options.yes_label = yes_label;
  options.no_label = no_label;
  options.alt_label = alt_label;

  if (def == Prompt_answer::YES) {
    options.default_value = shcore::Value(yes_label);
  } else if (def == Prompt_answer::NO) {
    options.default_value = shcore::Value(no_label);
  } else if (def == Prompt_answer::ALT) {
    options.default_value = shcore::Value(alt_label);
  }

  options.done();

  std::string ans;

  auto res = call_prompt(prompt, options, &ans, {},
                         &shcore::Interpreter_delegate::prompt);

  Prompt_answer final_ans = Prompt_answer::NONE;
  // TODO(rennox): Does it make sense to validate this returned value? iiuc
  // call_prompt will ALWAYS return OK for confirm prompts.
  if (res == shcore::Prompt_result::Ok) {
    if (ans == yes_label) {
      final_ans = Prompt_answer::YES;
    } else if (ans == no_label) {
      final_ans = Prompt_answer::NO;
    } else if (ans == alt_label) {
      final_ans = Prompt_answer::ALT;
    }
  }

  return final_ans;
}

bool Shell_console::select(const std::string &prompt_text, std::string *out_val,
                           const std::vector<std::string> &options,
                           size_t default_option, bool allow_custom,
                           Validator validator, const std::string &title,
                           const std::vector<std::string> &description) const {
  assert(default_option <= options.size());

  shcore::prompt::Prompt_options prompt_options;
  prompt_options.title = title;
  prompt_options.type = shcore::prompt::Prompt_type::SELECT;
  prompt_options.description = description;
  prompt_options.select_items = options;
  prompt_options.allow_custom = allow_custom;
  if (default_option != 0) {
    prompt_options.default_value =
        shcore::Value(static_cast<unsigned int>(default_option));
  }

  prompt_options.done();

  call_prompt(prompt_text, prompt_options, out_val, validator,
              &shcore::Interpreter_delegate::prompt);

  // TODO(rennox): Not needed to validate anything as SELECT prompts always
  // return true
  return true;
}

shcore::Prompt_result Shell_console::prompt_password(
    const std::string &prompt, std::string *out_val, Validator validator,
    const std::string &title,
    const std::vector<std::string> &description) const {
  return this->prompt(prompt, out_val, validator,
                      shcore::prompt::Prompt_type::PASSWORD, title,
                      description);
}

void Shell_console::print_value(const shcore::Value &value,
                                const std::string &tag) const {
  std::string output;
  // When using JSON output ALL must be JSON
  if (use_json()) {
    // If no tag is provided, prints the JSON representation of the Value
    if (tag.empty()) {
      output = value.json(mysqlsh::current_shell_options()->get().wrap_json ==
                          "json");
    } else {
      if (value.type == shcore::String)
        output = json_obj(tag.c_str(), value.get_string());
      else
        output = json_obj(tag.c_str(), value);
    }

    output += "\n";

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
      shcore::current_logger()->attach_log_hook(log_hook, this, true);
      m_hook_registered = true;
    }
  } else {
    detach_log_hook();
  }
}

void Shell_console::detach_log_hook() {
  if (m_hook_registered) {
    shcore::current_logger()->detach_log_hook(log_hook);
    m_hook_registered = false;
  }
}

}  // namespace mysqlsh
