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

#ifndef _PYTHON_MAP_WRAPPER_H_
#define _PYTHON_MAP_WRAPPER_H_

#include "shellcore/python_context.h"
#include "shellcore/types.h"

namespace shcore
{
class Python_context;

/*
 * Wraps an map object as a Python sequence object
 */
struct PyShDictObject
{
  PyObject_HEAD
  shcore::Value::Map_type_ref *map;
};

class Python_map_wrapper
{
public:
  Python_map_wrapper(Python_context *context);
  ~Python_map_wrapper();

  PyObject *wrap(boost::shared_ptr<Value::Map_type> map);
  static bool unwrap(PyObject *value, boost::shared_ptr<Value::Map_type> &ret_object);

private:
  Python_context *_context;
  PyShDictObject *_map_wrapper;
};

};

#endif
