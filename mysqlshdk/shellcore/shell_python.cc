/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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
          mysqlsh::current_shell_options()->get().interactive)),
      m_last_input_state(Input_state::Ok) {}

std::string Shell_python::preprocess_input_line(const std::string &s) {
  const char *p = s.c_str();
  while (*p && isblank(*p)) ++p;
  if (*p == '#') return std::string();
  return s;
}

void Shell_python::set_result_processor(
    std::function<void(shcore::Value, bool)> result_processor) {
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
  if (state == Input_state::Ok)
    _result_processor(result, result.type == shcore::Undefined);

  m_last_input_state = state;
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
#ifdef _WIN32
  // On Windows, signal is always delivered asynchronously, from another thread.
  // If the main python thread is executing a long-lasting call which is also
  // interrupted by CTRL-C (i.e. time.sleep()), it will generate a python
  // exception, unwind the stack, and move the instruction pointer to the
  // beginning of the "catch" block. If we use check_signals() to inject the
  // KeyboardInterrupt exception, it will be raised from that "catch" block
  // interfering with the program flow.
  // Python code is relying on PyErr_CheckSignals() to detect keyboard
  // interrupts and to act accordingly (i.e. changing the reported exception to
  // KeyboardInterrupt), so we're using PyErr_SetInterrupt() to trigger the
  // signal which is going to be picked up by PyErr_CheckSignals().
  log_info("User aborted Python execution (^C)");
  PyErr_SetInterrupt();
#else
  if (Py_AddPendingCall(&Shell_python::check_signals,
                        static_cast<void *>(this)) < 0)
    log_warning("Could not interrupt Python");
  else
    log_info("User aborted Python execution (^C)");
#endif
}

bool Shell_python::is_module(const std::string &file_name) {
  bool ret_val = false;

  try {
    WillEnterPython lock;

    ret_val = _py->is_module(file_name);
  } catch (std::exception &exc) {
    mysqlsh::current_console()->print_diag(
        std::string("Exception while loading ")
            .append(file_name)
            .append(": ")
            .append(exc.what()));
  }

  return ret_val;
}

void Shell_python::execute_module(const std::string &file_name) {
  shcore::Value ret_val;

  auto tid = PyThread_get_thread_ident();
  shcore::Interrupt_handler inth([this, tid]() {
    _aborted = true;
    abort(tid);
    return true;
  });

  try {
    WillEnterPython lock;

    ret_val = _py->execute_module(file_name, _owner->get_input_args());

    _result_processor(ret_val, ret_val.type == shcore::Undefined);
  } catch (std::exception &exc) {
    mysqlsh::current_console()->print_diag(
        std::string("Exception while loading ")
            .append(file_name)
            .append(": ")
            .append(exc.what()));
    // Should shcore::Exceptions bubble up??
  }
}

void Shell_python::load_plugin(const std::string &file_name) {
  _py->load_plugin(file_name);
}

void Shell_python::clear_input() {
  Shell_language::clear_input();
  m_last_input_state = Input_state::Ok;
}

std::string Shell_python::get_continued_input_context() {
  return m_last_input_state == Input_state::Ok ? "" : "-";
}
