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

using namespace shcore;

Python_type_bridger::Python_type_bridger(Python_context *context)
: owner(context)
{
}


Python_type_bridger::~Python_type_bridger()
{
/*
  delete object_wrapper;
  delete function_wrapper;
  delete map_wrapper;
  delete array_wrapper;
*/
}


void Python_type_bridger::init()
{
/*
  object_wrapper = new JScript_object_wrapper(owner);
  map_wrapper = new JScript_map_wrapper(owner);
  array_wrapper = new JScript_array_wrapper(owner);
  function_wrapper = new JScript_function_wrapper(owner);
*/
}


PyObject *Python_type_bridger::native_object_to_py(Object_bridge_ref object)
{
/*
  if (object->class_name() == "Date")
  {

    boost::shared_ptr<Date> date = boost::static_pointer_cast<Date>(object);
    return v8::Date::New(owner->isolate(), date->as_ms());
  }

  return object_wrapper->wrap(object);
*/
}


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
    retval = Value(value);
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
    retval = Value(value);
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
  else
  {
    PyObject *obj_repr = PyObject_Repr(py);
    const char *s = PyString_AsString(obj_repr);
    throw std::invalid_argument("Cannot convert Python value to internal value: "+std::string(s));
    Py_DECREF(obj_repr);
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
    r = native_object_to_py(*value.value.o);
    break;
  case Array:
    // TODO: maybe convert fully
    //r = array_wrapper->wrap(*value.value.array);
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
