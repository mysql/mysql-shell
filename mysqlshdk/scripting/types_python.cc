/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "scripting/types_python.h"
#include "scripting/object_factory.h"
#include "scripting/common.h"

using namespace shcore;

Python_function::Python_function(Python_context* context, PyObject *function)
: _py(context), _function(function){

  Py_INCREF(_function);
}

std::string Python_function::name() {
  // TODO:
  return "";
}

std::vector<std::pair<std::string, Value_type> > Python_function::signature() {
  // TODO:
  return std::vector<std::pair<std::string, Value_type> >();
}

std::pair<std::string, Value_type> Python_function::return_type() {
  // TODO:
  return std::pair<std::string, Value_type>();
}

bool Python_function::operator == (const Function_base &UNUSED(other)) const {
  // TODO:
  return false;
}

bool Python_function::operator != (const Function_base &UNUSED(other)) const {
  // TODO:
  return false;
}

Value Python_function::invoke(const Argument_list &args) {
  PyObject *argv = PyTuple_New(args.size());


  for(size_t index = 0; index < args.size(); index++) {
    PyTuple_SetItem(argv, index, _py->shcore_value_to_pyobj(args[index]));
  }

  PyObject* ret_val = PyObject_CallObject(_function, argv);

  return _py->pyobj_to_shcore_value(ret_val);
}

