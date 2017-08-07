/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "scripting/python_context.h"
#include "shellcore/shell_python.h"
#include "scripting/python_utils.h"
#include "shellcore/base_session.h"
#include "shellcore/interrupt_handler.h"

#include "pythread.h"

using namespace shcore;

Shell_python::Shell_python(Shell_core *shcore)
  : Shell_language(shcore) {
  _py = std::shared_ptr<Python_context>(new Python_context(shcore->get_delegate()));
}

Shell_python::~Shell_python() {
  _py.reset();
}

std::string Shell_python::preprocess_input_line(const std::string &s) {
  const char *p = s.c_str();
  while (*p && isblank(*p))
    ++p;
  if (*p == '#')
    return std::string();
  return s;
}

/*
* Helper function to ensure the exceptions generated on the mysqlx_connector
* are properly translated to the corresponding shcore::Exception type
*/

/*
 * Handle shell input on Python mode
 */
void Shell_python::handle_input(std::string &code, Input_state &state,
    std::function<void(shcore::Value)> result_processor) {
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

  if ((*Shell_core_options::get())[SHCORE_INTERACTIVE].as_bool()) {
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
    result_processor(result);
}

/*
 * Set global variable
 */
void Shell_python::set_global(const std::string &name, const Value &value) {
  _py->set_global(name, value);
}

int Shell_python::check_signals(void *thread_id) {
  Shell_python *self = static_cast<Shell_python*>(thread_id);
  if (self->_aborted) {
    PyThreadState_SetAsyncExc(self->_pending_interrupt_thread,
                              PyExc_KeyboardInterrupt);
  }
  return PyErr_CheckSignals();
}

void Shell_python::abort(long thread_id) noexcept {
  _pending_interrupt_thread = thread_id;
  if (Py_AddPendingCall(&Shell_python::check_signals,
                        static_cast<void*>(this)) < 0)
    log_warning("Could not interrupt Python");
  else
    log_info("User aborted Python execution (^C)");
}

bool Shell_python::is_module(const std::string& file_name) {
  bool ret_val = false;

  try {
    WillEnterPython lock;

    ret_val = _py->is_module(file_name);
  } catch (std::exception &exc) {
    _owner->print_error(std::string("Exception in PS ps function: ") + exc.what());
  }

  return ret_val;
}

void Shell_python::execute_module(const std::string& file_name, std::function<void(shcore::Value)> result_processor) {
  shcore::Value ret_val;
  try {
    WillEnterPython lock;

    ret_val = _py->execute_module(file_name, _owner->get_input_args());

    result_processor(ret_val);
  } catch (std::exception &exc) {
    _owner->print_error(std::string("Exception in PS ps function: ") + exc.what());

    // Should shcore::Exceptions bubble up??
  }
}
