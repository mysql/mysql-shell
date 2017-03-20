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

#ifndef _PYTHON_MAP_WRAPPER_H_
#define _PYTHON_MAP_WRAPPER_H_

#include "scripting/python_context.h"
#include "scripting/types.h"

namespace shcore {
/*
 * Wraps an map object as a Python sequence object
 */
struct PyShDictObject {
  PyObject_HEAD
  shcore::Value::Map_type_ref *map;
};

PyObject *wrap(std::shared_ptr<Value::Map_type> map);
bool unwrap(PyObject *value, std::shared_ptr<Value::Map_type> &ret_object);
};

#endif
