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

#include "shellcore/python_type_conversion.h"
#include "shellcore/python_array_wrapper.h"

using namespace shcore;

Python_type_bridger::Python_type_bridger(Python_context *context)
: _owner(context)
{
}


Python_type_bridger::~Python_type_bridger()
{
  delete _array_wrapper;
}


void Python_type_bridger::init()
{
  _array_wrapper = new Python_array_wrapper(_owner);
}

/* TODO:
PyObject *Python_type_bridger::native_object_to_py(Object_bridge_ref object)
{
}
*/


Value Python_type_bridger::pyobj_to_shcore_value(PyObject *py) const
{
  // Some conversions yield errors, in that case a temporary result is assigned to retval and check_err is set
  bool check_err = false;
  Value retval;

  if (py == Py_None)
    return Value::Null();
  else if (PyInt_Check(py))
  {
    long value = PyInt_AsLong(py);
    if (value == -1)
      check_err = true;
    retval = Value((int64_t)value);
  }
  else if (py == Py_False)
    return Value(false);
  else if (py == Py_True)
    return Value(true);
  else if (PyLong_Check(py))
  {
    long value = PyLong_AsLong(py);
    if (value == -1)
      check_err = true;
    retval = Value((int64_t)value);
  }
  else if (PyFloat_Check(py))
  {
    double value = PyFloat_AsDouble(py);
    if (value == -1.0)
      check_err = true;
    retval = Value(value);
#if PY_VERSION_HEX >= 0x2060000
  }
  else if (PyByteArray_Check(py))
    return Value(PyByteArray_AsString(py), PyByteArray_Size(py));
#endif
  else if (PyString_Check(py))
    return Value(PyString_AsString(py), PyString_Size(py));
  else if (PyUnicode_Check(py))
  {
    // TODO: In case of error calls ourself using NULL, either handle here before calling
    // recursively or always allow NULL
    PyObject *ref = PyUnicode_AsUTF8String(py);
    retval = pyobj_to_shcore_value(ref);
    Py_DECREF(ref);
  }  // } TODO: else if (Buffer/MemoryView || Tuple || List || Dictionary || DateTime || generic_object) {
  else if (PyList_Check(py))
  {
    boost::shared_ptr<Value::Array_type> array(new Value::Array_type);

    for (Py_ssize_t c = PyList_Size(py), i = 0; i < c; i++)
    {
      PyObject *item = PyList_GetItem(py, i);
      array->push_back(pyobj_to_shcore_value(item));
    }
    return Value(array);
  }
  else
  {
    boost::shared_ptr<Value::Array_type> array;

    if (Python_array_wrapper::unwrap(py, array))
    {
      return Value(array);
    }
    else
    {
      PyObject *obj_repr = PyObject_Repr(py);
      const char *s = PyString_AsString(obj_repr);
      throw std::invalid_argument("Cannot convert Python value to internal value: "+std::string(s));
      Py_DECREF(obj_repr);
    }
  }

  if (check_err)
  {
    PyObject *pyerror = PyErr_Occurred();
    if (pyerror)
    {
     // TODO: PyErr_Clean();
      throw Exception::argument_error("Error converting return value");
    }
  }
  return retval;
}


PyObject *Python_type_bridger::shcore_value_to_pyobj(const Value &value)
{
  PyObject *r;
  switch (value.type)
  {
  case Undefined:
    Py_INCREF(Py_None);
    r = Py_None;
    break;
  case Null:
    Py_INCREF(Py_None);
    r = Py_None;
    break;
  case Bool:
    r = PyBool_FromLong(value.value.b);
    break;
  case String:
    r = PyString_FromString(value.value.s->c_str());
    break;
  case Integer:
    r = PyInt_FromSsize_t(value.value.i);
    break;
  case Float:
    r = PyFloat_FromDouble(value.value.d);
    break;
  case Object:
    // TODO:
    // r = native_object_to_py(*value.value.o);
    break;
  case Array:
    r = _array_wrapper->wrap(*value.value.array);
    break;
  case Map:
    // TODO: maybe convert fully
    //r = map_wrapper->wrap(*value.value.map);
    break;
  case MapRef:
    /*
    {
      boost::shared_ptr<Value::Map_type> map(value.value.mapref->lock());
      if (map)
      {
        std::cout << "wrapmapref not implemented\n";
      }
    }
    */
    break;
  case shcore::Function:
    // TODO: r= function_wrapper->wrap(*value.value.func);
    break;
  }
  return r;
}
