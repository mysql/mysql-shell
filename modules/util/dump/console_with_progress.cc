/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#include "modules/util/dump/console_with_progress.h"

namespace mysqlsh {
namespace dump {

namespace {

class Hide_progress final {
 public:
  Hide_progress() = delete;

  Hide_progress(mysqlshdk::textui::IProgress *progress,
                std::recursive_mutex *mutex)
      : m_progress(dynamic_cast<mysqlshdk::textui::Text_progress *>(progress)),
        m_mutex(mutex) {
    hide_progress();
  }

  Hide_progress(const Hide_progress &) = delete;
  Hide_progress(Hide_progress &&) = delete;

  Hide_progress &operator=(const Hide_progress &) = delete;
  Hide_progress &operator=(Hide_progress &&) = delete;

  ~Hide_progress() { show_progress(); }

 private:
  void hide_progress() const {
    if (m_progress) {
      std::lock_guard<std::recursive_mutex> lock(*m_mutex);
      m_progress->hide(true);
    }
  }

  void show_progress() const {
    if (m_progress) {
      std::lock_guard<std::recursive_mutex> lock(*m_mutex);
      m_progress->hide(false);
    }
  }

  mysqlshdk::textui::Text_progress *m_progress;
  std::recursive_mutex *m_mutex;
};

}  // namespace

Console_with_progress::Console_with_progress(
    const std::unique_ptr<mysqlshdk::textui::IProgress> &progress,
    std::recursive_mutex *mutex)
    : m_progress(progress), m_mutex(mutex), m_console(current_console()) {}

void Console_with_progress::raw_print(const std::string &text,
                                      Output_stream stream,
                                      bool format_json) const {
  Hide_progress hp(m_progress.get(), m_mutex);
  m_console->raw_print(text, stream, format_json);
}

void Console_with_progress::print(const std::string &text) const {
  Hide_progress hp(m_progress.get(), m_mutex);
  m_console->print(text);
}

void Console_with_progress::println(const std::string &text) const {
  Hide_progress hp(m_progress.get(), m_mutex);
  m_console->println(text);
}

void Console_with_progress::print_error(const std::string &text) const {
  Hide_progress hp(m_progress.get(), m_mutex);
  m_console->print_error(text);
}

void Console_with_progress::print_warning(const std::string &text) const {
  Hide_progress hp(m_progress.get(), m_mutex);
  m_console->print_warning(text);
}

void Console_with_progress::print_note(const std::string &text) const {
  Hide_progress hp(m_progress.get(), m_mutex);
  m_console->print_note(text);
}

void Console_with_progress::print_status(const std::string &text) const {
  Hide_progress hp(m_progress.get(), m_mutex);
  m_console->print_status(text);
}

void Console_with_progress::print_info(const std::string &text) const {
  Hide_progress hp(m_progress.get(), m_mutex);
  m_console->print_info(text);
}

void Console_with_progress::print_para(const std::string &text) const {
  Hide_progress hp(m_progress.get(), m_mutex);
  m_console->print_para(text);
}

void Console_with_progress::print_value(const shcore::Value &value,
                                        const std::string &tag) const {
  Hide_progress hp(m_progress.get(), m_mutex);
  m_console->print_value(value, tag);
}

void Console_with_progress::print_diag(const std::string &text) const {
  Hide_progress hp(m_progress.get(), m_mutex);
  m_console->print_diag(text);
}

shcore::Prompt_result Console_with_progress::prompt(const std::string &prompt,
                                                    std::string *out_val,
                                                    Validator validator) const {
  Hide_progress(m_progress.get(), m_mutex);
  return m_console->prompt(prompt, out_val, validator);
}

Prompt_answer Console_with_progress::confirm(
    const std::string &prompt, Prompt_answer def, const std::string &yes_label,
    const std::string &no_label, const std::string &alt_label) const {
  Hide_progress hp(m_progress.get(), m_mutex);
  return m_console->confirm(prompt, def, yes_label, no_label, alt_label);
}

shcore::Prompt_result Console_with_progress::prompt_password(
    const std::string &prompt, std::string *out_val,
    Validator validator) const {
  Hide_progress hp(m_progress.get(), m_mutex);
  return m_console->prompt_password(prompt, out_val, validator);
}

bool Console_with_progress::select(const std::string &prompt_text,
                                   std::string *result,
                                   const std::vector<std::string> &items,
                                   size_t default_option, bool allow_custom,
                                   Validator validator) const {
  Hide_progress hp(m_progress.get(), m_mutex);
  return m_console->select(prompt_text, result, items, default_option,
                           allow_custom, validator);
}

std::shared_ptr<IPager> Console_with_progress::enable_pager() {
  return m_console->enable_pager();
}

void Console_with_progress::enable_global_pager() {
  m_console->enable_global_pager();
}

void Console_with_progress::disable_global_pager() {
  m_console->disable_global_pager();
}

bool Console_with_progress::is_global_pager_enabled() const {
  return m_console->is_global_pager_enabled();
}

void Console_with_progress::add_print_handler(
    shcore::Interpreter_print_handler *handler) {
  m_console->add_print_handler(handler);
}

void Console_with_progress::remove_print_handler(
    shcore::Interpreter_print_handler *handler) {
  m_console->remove_print_handler(handler);
}

}  // namespace dump
}  // namespace mysqlsh
