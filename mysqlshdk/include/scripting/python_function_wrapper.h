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
  // clang-format off
  PyObject_HEAD
  shcore::Function_base_ref *func;
  // clang-format on
};

py::Release wrap(std::shared_ptr<Function_base> func);
bool unwrap(PyObject *value, std::shared_ptr<Function_base> &ret_func);
}  // namespace shcore

#endif  // _PYTHON_FUNCTION_WRAPPER_H_
