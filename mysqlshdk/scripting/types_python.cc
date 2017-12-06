/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "scripting/types_python.h"
#include "scripting/object_factory.h"
#include "scripting/common.h"

using namespace shcore;

Python_function::Python_function(Python_context* context, PyObject *function)
: _py(context), _function(function){

  Py_INCREF(_function);
}

const std::string &Python_function::name() const {
  // TODO:
  static std::string tmp;
  return tmp;
}

const std::vector<std::pair<std::string, Value_type> > &Python_function::signature() const {
  // TODO:
  static std::vector<std::pair<std::string, Value_type> > tmp;
  return tmp;
}

Value_type Python_function::return_type() const {
  // TODO:
  return Undefined;
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
