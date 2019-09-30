/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _PYTHON_OBJECT_WRAPPER_H_
#define _PYTHON_OBJECT_WRAPPER_H_

#include <memory>

#include "scripting/python_context.h"
#include "scripting/types.h"

namespace shcore {
class Python_context;

struct PyMemberCache {
  std::map<std::string, AutoPyObject> members;
};

/*
 * Wraps a native/bridged C++ object reference as a Python sequence object
 */
struct PyShObjObject {
  PyObject_HEAD shcore::Object_bridge_ref *object;
  PyMemberCache *cache;
};

struct PyShObjIndexedObject {
  PyObject_HEAD shcore::Object_bridge_ref *object;
  PyMemberCache *cache;
};

PyObject *wrap(std::shared_ptr<Object_bridge> object);
bool unwrap(PyObject *value, std::shared_ptr<Object_bridge> &ret_object);

bool unwrap_method(PyObject *value, std::shared_ptr<Function_base> *method);

}  // namespace shcore

#endif  // _PYTHON_OBJECT_WRAPPER_H_
