/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "scripting/python_context.h"

#include "shellcore/shell_python.h"

#include "mysqlshdk/include/shellcore/base_shell.h"
#include "shellcore/base_session.h"
#include "shellcore/interrupt_handler.h"

#include "pythread.h"

using namespace shcore;

Shell_python::Shell_python(Shell_core *shcore)
    : Shell_language(shcore),
      _py(new Python_context(
          shcore->get_delegate(),
          mysqlsh::current_shell_options()->get().interactive)) {}

std::string Shell_python::preprocess_input_line(const std::string &s) {
  const char *p = s.c_str();
  while (*p && isblank(*p)) ++p;
  if (*p == '#') return std::string();
  return s;
}

void Shell_python::set_result_processor(
    std::function<void(shcore::Value)> result_processor) {
  _result_processor = result_processor;
}

/*
 * Handle shell input on Python mode
 */
void Shell_python::handle_input(std::string &code, Input_state &state) {
  Value result;

  auto tid = PyThread_get_thread_ident();
  shcore::Interrupt_handler inth([this, tid]() {
    _aborted = true;
    abort(tid);
    return true;
  });
  if (_aborted) {
    WillEnterPython lock;
    PyThreadState_SetAsyncExc(tid, NULL);
    _aborted = false;
  }

  if (mysqlsh::current_shell_options()->get().interactive) {
    WillEnterPython lock;
    result = _py->execute_interactive(code, state);
  } else {
    try {
      WillEnterPython lock;
      result = _py->execute(code, _owner->get_input_source(),
                            _owner->get_input_args());
    } catch (Exception &exc) {
      // This exception was already printed in PY
      // and the correct return_value of undefined is set
    }
  }
  _last_handled = code;

  // Only processes the result when full statements are executed
  if (state == Input_state::Ok) _result_processor(result);
}

/*
 * Set global variable
 */
void Shell_python::set_global(const std::string &name, const Value &value) {
  _py->set_global(name, value);
}

int Shell_python::check_signals(void *thread_id) {
  Shell_python *self = static_cast<Shell_python *>(thread_id);
  if (self->_aborted) {
    PyThreadState_SetAsyncExc(self->_pending_interrupt_thread,
                              PyExc_KeyboardInterrupt);
  }
  return PyErr_CheckSignals();
}

void Shell_python::abort(long thread_id) noexcept {
  _pending_interrupt_thread = thread_id;
  if (Py_AddPendingCall(&Shell_python::check_signals,
                        static_cast<void *>(this)) < 0)
    log_warning("Could not interrupt Python");
  else
    log_info("User aborted Python execution (^C)");
}

bool Shell_python::is_module(const std::string &file_name) {
  bool ret_val = false;

  try {
    WillEnterPython lock;

    ret_val = _py->is_module(file_name);
  } catch (std::exception &exc) {
    _owner->print_error(std::string("Exception while loading ")
                            .append(file_name)
                            .append(": ")
                            .append(exc.what()));
  }

  return ret_val;
}

void Shell_python::execute_module(const std::string &file_name) {
  shcore::Value ret_val;
  try {
    WillEnterPython lock;

    ret_val = _py->execute_module(file_name, _owner->get_input_args());

    _result_processor(ret_val);
  } catch (std::exception &exc) {
    _owner->print_error(std::string("Exception while loading ")
                            .append(file_name)
                            .append(": ")
                            .append(exc.what()));
    // Should shcore::Exceptions bubble up??
  }
}
