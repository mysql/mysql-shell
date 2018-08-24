/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/devapi/mod_mysqlx_session.h"
#include "mysqlshdk/include/shellcore/base_shell.h"
#include "scripting/jscript_context.h"
#include "shellcore/base_session.h"
#include "shellcore/interrupt_handler.h"

using namespace shcore;

Shell_javascript::Shell_javascript(Shell_core *shcore)
    : Shell_language(shcore), _js(new JScript_context(shcore->registry())) {}

void Shell_javascript::set_result_processor(
    std::function<void(shcore::Value)> result_processor) {
  _result_processor = result_processor;
}

void Shell_javascript::handle_input(std::string &code, Input_state &state) {
  // Undefined to be returned in case of errors
  Value result;

  shcore::Interrupt_handler inth([this]() {
    abort();
    return true;
  });

  if (mysqlsh::current_shell_options()->get().interactive)
    result = _js->execute_interactive(code, state);
  else {
    try {
      result = _js->execute(code, _owner->get_input_source(),
                            _owner->get_input_args());
    } catch (std::exception &exc) {
      mysqlsh::current_console()->raw_print(exc.what(),
                                            mysqlsh::Output_stream::STDERR);
    }
  }

  _last_handled = code;

  _result_processor(result);
  m_last_input_state = state;
}

void Shell_javascript::set_global(const std::string &name, const Value &value) {
  _js->set_global(name, value);
}

void Shell_javascript::abort() noexcept {
  // Abort execution of JS code
  // To abort execution of MySQL query called during JS code, a separate
  // handler should be pushed into the stack in the code that performs the query
  log_info("User aborted JavaScript execution (^C)");
  _js->terminate();
}

void Shell_javascript::clear_input() {
  Shell_language::clear_input();
  m_last_input_state = Input_state::Ok;
}

std::string Shell_javascript::get_continued_input_context() {
  return m_last_input_state == Input_state::Ok ? "" : "-";
}
