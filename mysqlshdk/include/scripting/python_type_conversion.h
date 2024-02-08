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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_PYTHON_TYPE_CONVERSION_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_PYTHON_TYPE_CONVERSION_H_

#include "scripting/python_utils.h"
#include "scripting/types.h"

namespace shcore {

class Python_context;

namespace py {

Value convert(PyObject *value, Python_context *context = nullptr);

Value convert(PyObject *value, Python_context **context, bool is_binary = true);

void set_item(const Dictionary_t &map, PyObject *key, PyObject *value,
              Python_context *context = nullptr);

void set_item(const Dictionary_t &map, PyObject *key, PyObject *value,
              Python_context **context);

py::Release convert(const Value &value, Python_context *context = nullptr);

}  // namespace py
}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_PYTHON_TYPE_CONVERSION_H_
