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

#ifndef _PYTHON_ARRAY_WRAPPER_H_
#define _PYTHON_ARRAY_WRAPPER_H_

#include "shellcore/types.h"
#include "shellcore/python_context.h"

namespace shcore
{
class Python_context;

/*
 * Wraps an array object as a Python sequence object
 */
struct PyShListObject
{
  PyObject_HEAD
  shcore::Value::Array_type_ref *array;
};

class Python_array_wrapper
{
public:
  Python_array_wrapper(Python_context *context);
  ~Python_array_wrapper();

  PyObject *wrap(boost::shared_ptr<Value::Array_type> array);
  static bool unwrap(PyObject *value, boost::shared_ptr<Value::Array_type> &ret_array);

private:
  Python_context *_context;
  PyShListObject *_array_wrapper;
};

};

#endif
