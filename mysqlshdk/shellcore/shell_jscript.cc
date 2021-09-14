/*
 * Copyright (c) 2014, 2021, Oracle and/or its affiliates.
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

#include "shellcore/shell_jscript.h"

#include "mysqlshdk/include/shellcore/base_shell.h"
#include "scripting/jscript_context.h"
#include "shellcore/base_session.h"
#include "shellcore/interrupt_handler.h"

using namespace shcore;

Shell_javascript::Shell_javascript(Shell_core *shcore)
    : Shell_language(shcore), _js(new JScript_context(shcore->registry())) {}

void Shell_javascript::set_result_processor(
    std::function<void(shcore::Value, bool)> result_processor) {
  _result_processor = result_processor;
}

void Shell_javascript::handle_input(std::string &code, Input_state &state) {
  handle_input(code, false);
  state = m_input_state;
}

void Shell_javascript::flush_input(const std::string &code) {
  std::string code_copy{code};
  handle_input(code_copy, true);
}

void Shell_javascript::handle_input(std::string &code, bool flush) {
  // Undefined to be returned in case of errors
  Value result;

  shcore::Interrupt_handler inth([this]() {
    abort();
    return true;
  });

  bool got_error = true;
  if (_owner->interactive())
    std::tie(result, got_error) =
        _js->execute_interactive(code, &m_input_state, flush);
  else {
    try {
      std::tie(result, got_error) =
          _js->execute(code, _owner->get_input_source());
    } catch (const std::exception &exc) {
      mysqlsh::current_console()->print_diag(exc.what());
    }
  }

  if (!code.empty()) {
    _last_handled = code;
  }

  _result_processor(result, got_error);
}

void Shell_javascript::set_global(const std::string &name, const Value &value) {
  _js->set_global(name, value);
}

void Shell_javascript::set_argv(const std::vector<std::string> &argv) {
  _js->set_argv(argv);
}

void Shell_javascript::abort() noexcept {
  // Abort execution of JS code
  // To abort execution of MySQL query called during JS code, a separate
  // handler should be pushed into the stack in the code that performs the query
  log_info("User aborted JavaScript execution (^C)");
  _js->terminate();
}

std::string Shell_javascript::get_continued_input_context() {
  return m_input_state == Input_state::Ok ? "" : "-";
}

bool Shell_javascript::load_plugin(const Plugin_definition &plugin) {
  return _js->load_plugin(plugin);
}
