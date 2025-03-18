/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/include/shellcore/shell_polyglot.h"

#include <stdexcept>

#include "mysqlshdk/include/shellcore/base_shell.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/scripting/polyglot/polyglot_context.h"
#include "shellcore/base_session.h"
#include "shellcore/interrupt_handler.h"

namespace shcore {

namespace {

const char *to_string(polyglot::Language language) {
  switch (language) {
    case polyglot::Language::JAVASCRIPT:
      return "JavaScript";
  }

  throw std::logic_error{"to_string(polyglot::Language): should not happen"};
}

}  // namespace

Shell_polyglot::Shell_polyglot(Shell_core *shcore, polyglot::Language type)
    : Shell_language(shcore),
      m_type{type},
      m_polyglot{std::make_shared<polyglot::Polyglot_context>(
          shcore->registry(), type)} {
  // Starts and sleeps the polyglot interruption handler thread
  m_interrupt_handler_thread = std::thread([this, type]() {
    std::unique_lock<std::mutex> lock(m_interrupt_handler_mutex);

    while (!m_tear_down) {
      m_interrupt_handler_condition.wait(
          lock, [this]() { return m_tear_down || m_interrupt; });

      if (m_interrupt) {
        lock.unlock();

        try {
          abort();
        } catch (const std::exception &e) {
          log_error("Exception while trying to interrupt the %s execution: %s",
                    to_string(type), e.what());
        }

        lock.lock();
        m_interrupt = false;
      }
    }
  });
}

Shell_polyglot::~Shell_polyglot() {
  // Shutdowns the polyglot interruption handler thread
  {
    std::lock_guard<std::mutex> lock(m_interrupt_handler_mutex);
    m_tear_down = true;
  }

  m_interrupt_handler_condition.notify_one();
  m_interrupt_handler_thread.join();
}

void Shell_polyglot::set_result_processor(
    std::function<void(shcore::Value, bool)> result_processor) {
  m_result_processor = std::move(result_processor);
}

void Shell_polyglot::handle_input(std::string &code, Input_state &state) {
  handle_input(code, false);
  state = m_input_state;
}

void Shell_polyglot::flush_input(const std::string &code) {
  std::string code_copy{code};
  handle_input(code_copy, true);
}

int Shell_polyglot::debug(const std::string &path) {
  Value result;
  bool got_error = true;

  std::tie(result, got_error) =
      m_polyglot->debug(_owner->registry(), m_type, path);

  m_result_processor(result, got_error);

  return 0;
}

void Shell_polyglot::handle_input(std::string &code, bool flush) {
  // Undefined to be returned in case of errors
  Value result;

  shcore::Interrupt_handler handler([this]() {
    // NOTE: this code is not signal-safe
    bool notify = false;

    {
      std::lock_guard<std::mutex> lock(m_interrupt_handler_mutex);

      if (!m_interrupt) {
        m_interrupt = notify = true;
      }
    }

    if (notify) {
      m_interrupt_handler_condition.notify_one();
    }

    return true;
  });

  bool got_error = true;
  if (_owner->interactive()) {
    std::tie(result, got_error) =
        m_polyglot->execute_interactive(code, &m_input_state, flush);
  } else {
    try {
      std::tie(result, got_error) =
          m_polyglot->execute(code, _owner->get_input_source());
    } catch (const std::exception &exc) {
      mysqlsh::current_console()->print_diag(exc.what());
    }
  }

  if (!code.empty()) {
    _last_handled = code;
  }

  m_result_processor(result, got_error);
}

void Shell_polyglot::set_global(const std::string &name, const Value &value) {
  m_polyglot->set_global(name, value);
}

void Shell_polyglot::set_argv(const std::vector<std::string> &argv) {
  m_polyglot->set_argv(argv);
}

void Shell_polyglot::abort() noexcept {
  // Abort execution of JS code
  // To abort execution of MySQL query called during JS code, a separate
  // handler should be pushed into the stack in the code that performs the query
  log_info("User aborted JavaScript execution (^C)");
  m_polyglot->terminate();
}

std::string Shell_polyglot::get_continued_input_context() {
  return m_input_state == Input_state::Ok ? "" : "-";
}

bool Shell_polyglot::load_plugin(const Plugin_definition &plugin) {
  return m_polyglot->load_plugin(plugin);
}

}  // namespace shcore
