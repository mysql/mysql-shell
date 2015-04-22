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

#include <Python.h>

namespace shcore {

Python_context::Python_context()
: types(this)
{
  PyObject *main = PyImport_AddModule("__main__");
  globals = PyModule_GetDict(main);
}

Python_context::~Python_context()
{
}

Value Python_context::execute(const std::string &code)
{
  PyObject *py_result;
  // PyRun_SimpleString(code.c_str());
  py_result = PyRun_String(code.c_str(), Py_single_input /* TODO: use Py_file_input instead? */, globals, globals);
  if (!py_result)
  {
    // TODO: use log_debug() as soon as logging is integrated
    PyErr_Print();
    return Value::Null();
  }

  if (py_result != Py_None) {
    // TODO: use log_debug() as soon as logging is integrated
    printf("py_result != Py_None in %s:%d!\n", __FILE__, __LINE__);
    PyObject_Print(py_result, stdout, 0);
  }
  return types.pyobj_to_shcore_value(py_result);
}

void Python_context::set_global(const std::string &name, const Value &value)
{
  PyDict_SetItemString(globals, name.c_str(), types.shcore_value_to_pyobj(value));
}

}
