/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/python_context.h"
#include "shellcore/python_utils.h"

#include "shellcore/python_type_conversion.h"

#include <Python.h>

using namespace shcore;

void Python_context_init() {
  Py_InitializeEx(0);
}

Python_context::Python_context() throw (Exception)
: types(this)
{
  PyObject *main = PyImport_AddModule("__main__");
  _globals = PyModule_GetDict(main);

  if (!main || !globals)
  {
    throw Exception::runtime_error("Error initializing python context.");
    PyErr_Print();
  }

  // Stores the main thread state
  _main_thread_state = PyThreadState_Get();

  PyEval_InitThreads();
  PyEval_SaveThread();
}

Python_context::~Python_context()
{
  PyEval_RestoreThread(_main_thread_state);
  _main_thread_state = NULL;
  Py_Finalize();
}

Value Python_context::execute(const std::string &code)
{
  PyObject *py_result;
  Value retvalue;

  // PyRun_SimpleString(code.c_str());
  py_result = PyRun_String(code.c_str(), Py_eval_input /* TODO: use Py_file_input instead? */, _globals, _globals);
  if (!py_result)
  {
    // TODO: use log_debug() as soon as logging is integrated
    // PyErr_Print();
    return Value();
  }

  return types.pyobj_to_shcore_value(py_result);
}

PyObject *Python_context::get_global(const std::string &value)
{
  PyObject *py_result;

  py_result= PyDict_GetItemString(_globals, value.c_str());
  if (!py_result)
  {
    //TODO: log error
  }
  return py_result;
}

void Python_context::set_global(const std::string &name, const Value &value)
{
  PyDict_SetItemString(_globals, name.c_str(), types.shcore_value_to_pyobj(value));
}

Value Python_context::pyobj_to_shcore_value(PyObject *value) 
{
  return types.pyobj_to_shcore_value(value);
}

PyObject *Python_context::shcore_value_to_pyobj(const Value &value)
{
  return types.shcore_value_to_pyobj(value);
}
