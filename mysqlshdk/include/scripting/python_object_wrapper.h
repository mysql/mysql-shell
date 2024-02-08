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

#ifndef _PYTHON_OBJECT_WRAPPER_H_
#define _PYTHON_OBJECT_WRAPPER_H_

#include <memory>

#include "scripting/python_context.h"
#include "scripting/types.h"

namespace shcore {

class Cpp_object_bridge;

py::Release wrap(const std::shared_ptr<Object_bridge> &object);

py::Release wrap(const std::shared_ptr<Cpp_object_bridge> &object,
                 const std::string &method);

bool unwrap(PyObject *value, std::shared_ptr<Object_bridge> &ret_object);

bool unwrap_method(PyObject *value, std::shared_ptr<Function_base> *method);

}  // namespace shcore

#endif  // _PYTHON_OBJECT_WRAPPER_H_
