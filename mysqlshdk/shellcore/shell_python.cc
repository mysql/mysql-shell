/*
 * Copyright (c) 2015, 2024, Oracle and/or its affiliates.
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

#include "scripting/python_context.h"

#include "shellcore/shell_python.h"

#include "mysqlshdk/include/shellcore/base_shell.h"
#include "shellcore/base_session.h"
#include "shellcore/interrupt_handler.h"

#include "pythread.h"

#if defined(_WIN32)
#include <windows.h>
#endif

using namespace shcore;

Shell_python::Shell_python(Shell_core *shcore)
    : Shell_language(shcore),
      _py(new Python_context(
          mysqlsh::current_shell_options()->get().interactive)) {}

std::string Shell_python::preprocess_input_line(const std::string &s) {
  const char *p = s.c_str();
  while (*p && isblank(*p)) ++p;

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
  handle_input(code, false);
  state = m_input_state;
}

void Shell_python::flush_input(const std::string &code) {
  std::string code_copy{code};
  handle_input(code_copy, true);
}

void Shell_python::handle_input(std::string &code, bool flush) {
  shcore::Interrupt_handler inth([this]() {
    abort();
    return true;
  });
  if (m_aborted) {
    m_aborted = false;
  }

  Value result;
  if (_owner->interactive()) {
    WillEnterPython lock;
    result = _py->execute_interactive(code, m_input_state, flush);
  } else {
    try {
      WillEnterPython lock;
      result = _py->execute(code, _owner->get_input_source());
    } catch (Exception &) {
      // This exception was already printed in PY
      // and the correct return_value of undefined is set
    }
  }
  if (!code.empty()) {
    _last_handled = code;
  }

  // Only processes the result when full statements are executed
  if (m_input_state == Input_state::Ok)
    _result_processor(result, result.get_type() == shcore::Undefined);
}

/*
 * Set global variable
 */
void Shell_python::set_global(const std::string &name, const Value &value) {
  _py->set_global(name, value);
}

void Shell_python::set_argv(const std::vector<std::string> &argv) {
  _py->set_argv(argv);
}

void Shell_python::abort() noexcept {
  m_aborted = true;
  log_info("User aborted Python execution (^C)");
  // On Windows, signal is always delivered asynchronously, from another thread.
  // If the main python thread is executing a long-lasting call which is also
  // interrupted by CTRL-C (i.e. time.sleep()), it will generate a python
  // exception, unwind the stack, and move the instruction pointer to the
  // beginning of the "catch" block. If we use PyThreadState_SetAsyncExc() to
  // inject the KeyboardInterrupt exception, it will be raised from that "catch"
  // block interfering with the program flow.
  //
  // Python code is relying on PyErr_CheckSignals() to detect keyboard
  // interrupts and to act accordingly (i.e. changing the reported exception to
  // KeyboardInterrupt), so we're using PyErr_SetInterrupt() to trigger the
  // signal which is going to be picked up by PyErr_CheckSignals().
  PyErr_SetInterrupt();

#if defined(_WIN32)
  // Python's signal handler is not installed, we need to manually trigger the
  // event to make sure that time.sleep() is interrupted
  SetEvent(_PyOS_SigintEvent());
#endif
}

void Shell_python::execute_module(const std::string &module_name,
                                  const std::vector<std::string> &args) {
  shcore::Interrupt_handler inth([this]() {
    abort();
    return true;
  });

  shcore::Value ret_val;
  try {
    WillEnterPython lock;

    ret_val = _py->execute_module(module_name, args);

    _result_processor(ret_val, ret_val.get_type() == shcore::Undefined);
  } catch (const std::exception &exc) {
    mysqlsh::current_console()->print_diag(
        std::string("Exception while loading ")
            .append(module_name)
            .append(": ")
            .append(exc.what()));
    // Should shcore::Exceptions bubble up??
  }
}

bool Shell_python::load_plugin(const Plugin_definition &plugin) {
  return _py->load_plugin(plugin);
}

std::string Shell_python::get_continued_input_context() {
  return m_input_state == Input_state::Ok ? "" : "-";
}
