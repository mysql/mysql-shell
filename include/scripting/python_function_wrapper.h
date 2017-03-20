/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _PYTHON_FUNCTION_WRAPPER_H_
#define _PYTHON_FUNCTION_WRAPPER_H_

#include "scripting/python_context.h"
#include "scripting/types.h"

namespace shcore {
class Python_context;

/*
 * Wraps a native/bridged C++ function reference as a Python sequence object
 */
struct PyShFuncObject {
  PyObject_HEAD
  shcore::Function_base_ref *func;
};

PyObject *wrap(std::shared_ptr<Function_base> func);
bool unwrap(PyObject *value, std::shared_ptr<Function_base> &ret_func);
};

#endif  // _PYTHON_FUNCTION_WRAPPER_H_
