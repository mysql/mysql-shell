/*
 * Copyright (c) 2015, 2021, Oracle and/or its affiliates.
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

#include "scripting/python_type_conversion.h"

#include <cassert>

#include "scripting/obj_date.h"
#include "scripting/python_array_wrapper.h"
#include "scripting/python_function_wrapper.h"
#include "scripting/python_map_wrapper.h"
#include "scripting/python_object_wrapper.h"
#include "scripting/types_python.h"

namespace shcore {
namespace py {

Value convert(PyObject *py, Python_context *context) {
  return convert(py, &context);
}

Value convert(PyObject *py, Python_context **context) {
  assert(context);

  // Some numeric conversions yield errors, in that case the string
  // representation of the python object is returned
  Value retval;

  if (!py || py == Py_None) {
    return Value::Null();
  } else if (py == Py_False) {
    return Value(false);
  } else if (py == Py_True) {
    return Value(true);
  } else if (PyInt_Check(py) || PyLong_Check(py)) {
    int64_t value = PyLong_AsLongLong(py);

    if (value == -1 && PyErr_Occurred()) {
      // If reading LongLong failed then it attempts
      // reading UnsignedLongLong
      PyErr_Clear();
      uint64_t uint_value = PyLong_AsUnsignedLongLong(py);
      if (static_cast<int64_t>(uint_value) == -1 && PyErr_Occurred()) {
        // If reading UnsignedLongLong failed then retrieves
        // the string representation
        PyErr_Clear();
        Release obj_repr{PyObject_Repr(py)};
        if (obj_repr) {
          std::string s;

          if (Python_context::pystring_to_string(obj_repr, &s)) {
            retval = Value(s);
          }
        }
      } else {
        retval = Value(uint_value);
      }
    } else {
      retval = Value(value);
    }
  } else if (PyFloat_Check(py)) {
    double value = PyFloat_AsDouble(py);
    if (value == -1.0 && PyErr_Occurred()) {
      // At this point it means an error was generated above
      // It is needed to clear it
      PyErr_Clear();
      Release obj_repr{PyObject_Repr(py)};
      if (obj_repr) {
        std::string s;

        if (Python_context::pystring_to_string(obj_repr, &s)) {
          retval = Value(s);
        }
      }
    } else {
      retval = Value(value);
    }
  } else if (PyByteArray_Check(py)) {
    return Value(PyByteArray_AsString(py), PyByteArray_Size(py));
  } else if (PyBytes_Check(py)) {
    return Value(PyBytes_AsString(py), PyBytes_Size(py));
  } else if (PyUnicode_Check(py)) {
    // TODO: In case of error calls ourself using NULL, either handle here
    // before calling recursively or always allow NULL
    Release ref{PyUnicode_AsUTF8String(py)};
    retval = convert(ref, context);
  } else if (array_check(py)) {
    std::shared_ptr<Value::Array_type> array;

    if (!unwrap(py, &array)) {
      throw std::runtime_error("Could not unwrap shell array.");
    }

    return Value(array);
  } else if (PyList_Check(py)) {
    auto array = std::make_shared<Value::Array_type>();

    for (Py_ssize_t c = PyList_Size(py), i = 0; i < c; i++) {
      array->emplace_back(convert(PyList_GetItem(py, i), context));
    }

    return Value(array);
  } else if (PyTuple_Check(py)) {
    auto array = std::make_shared<Value::Array_type>();

    for (Py_ssize_t c = PyTuple_Size(py), i = 0; i < c; i++) {
      array->emplace_back(convert(PyTuple_GetItem(py, i), context));
    }

    return Value(array);
  } else if (PyDict_Check(py)) {
    std::shared_ptr<Value::Map_type> map(new Value::Map_type);

    PyObject *key, *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(py, &pos, &key, &value)) {
      // The key may be anything (not necessarily a string)
      // so we get the string representation of whatever it is
      PyObject *key_repr = PyObject_Str(key);
      std::string key_repr_string;
      Python_context::pystring_to_string(key_repr, &key_repr_string);
      (*map)[key_repr_string] = convert(value, context);

      Py_DECREF(key_repr);
    }

    return Value(map);
  } else if (PyFunction_Check(py)) {
    if (!*context) {
      try {
        *context = Python_context::get();
      } catch (const std::exception &e) {
        throw std::runtime_error(std::string{"Could not get SHELL context: "} +
                                 e.what());
      }
    }

    return Value(std::make_shared<Python_function>(*context, py));
  } else {
    std::shared_ptr<Value::Map_type> map;
    std::shared_ptr<Object_bridge> object;
    std::shared_ptr<Function_base> function;

    if (unwrap(py, &map)) {
      return Value(map);
    } else if (unwrap(py, object)) {
      return Value(object);
    } else if (unwrap_method(py, &function)) {
      return Value(function);
    } else if (unwrap(py, function)) {
      return Value(function);
    } else {
      if (!*context) {
        try {
          *context = Python_context::get();
        } catch (const std::exception &e) {
          throw std::runtime_error(
              std::string{"Could not get SHELL context: "} + e.what());
        }
      }

      auto get_int64 = [](PyObject *obj, const char *name) {
        auto att = PyObject_GetAttrString(obj, name);
        int64_t ret_val = PyLong_AsLongLong(att);
        Py_XDECREF(att);
        return ret_val;
      };

      if (PyObject_TypeCheck(py, (*context)->get_datetime_type())) {
        auto year = get_int64(py, "year");
        auto month = get_int64(py, "month");
        auto day = get_int64(py, "day");
        auto hour = get_int64(py, "hour");
        auto min = get_int64(py, "minute");
        auto sec = get_int64(py, "second");
        auto usec = get_int64(py, "microsecond");

        return shcore::Value(Object_bridge_ref(
            new Date(year, month, day, hour, min, sec, usec)));
      } else if (PyObject_TypeCheck(py, (*context)->get_date_type())) {
        auto year = get_int64(py, "year");
        auto month = get_int64(py, "month");
        auto day = get_int64(py, "day");

        return shcore::Value(Object_bridge_ref(new Date(year, month, day)));
      } else if (PyObject_TypeCheck(py, (*context)->get_time_type())) {
        auto hour = get_int64(py, "hour");
        auto min = get_int64(py, "minute");
        auto sec = get_int64(py, "second");
        auto usec = get_int64(py, "microsecond");

        return shcore::Value(Object_bridge_ref(new Date(hour, min, sec, usec)));
      } else {
        PyObject *obj_repr = PyObject_Repr(py);
        std::string s;
        Value ret_val;

        if (Python_context::pystring_to_string(obj_repr, &s)) {
          ret_val = Value(s);
        }

        Py_DECREF(obj_repr);

        if (ret_val)
          return ret_val;
        else
          throw std::invalid_argument(
              "Cannot convert Python value to internal value");
      }
    }
  }
  // } TODO: else if (Buffer/MemoryView || Tuple || DateTime || generic_object

  return retval;
}

PyObject *convert(const Value &value, Python_context * /*context*/) {
  PyObject *r = nullptr;
  switch (value.type) {
    case Undefined:
      r = Py_None;
      Py_INCREF(r);
      break;
    case Null:
      r = Py_None;
      Py_INCREF(r);
      break;
    case Bool:
      r = PyBool_FromLong(value.value.b);
      break;
    case String:
      r = PyString_FromString(value.value.s->c_str());
      break;
    case Integer:
      r = PyLong_FromLongLong(value.value.i);
      break;
    case UInteger:
      r = PyLong_FromUnsignedLongLong(value.value.ui);
      break;
    case Float:
      r = PyFloat_FromDouble(value.value.d);
      break;
    case Object: {
      if (value.as_object()->class_name() == "Date") {
        auto ctx = Python_context::get();
        std::shared_ptr<Date> date = value.as_object<Date>();

        // 0 dates are invalid in Python, but OK in MySQL, so we convert these
        // to None, which is what they probably really mean
        if (date->has_date() && date->get_year() == 0 &&
            date->get_month() == 0 && date->get_day() == 0) {
          r = Py_None;
          Py_INCREF(r);
        } else if (date->has_time()) {
          if (!date->has_date()) {
            r = ctx->create_time_object(date->get_hour(), date->get_min(),
                                        date->get_sec(), date->get_usec());
          } else {
            r = ctx->create_datetime_object(date->get_year(), date->get_month(),
                                            date->get_day(), date->get_hour(),
                                            date->get_min(), date->get_sec(),
                                            date->get_usec());
          }
        } else {
          r = ctx->create_date_object(date->get_year(), date->get_month(),
                                      date->get_day());
        }

      } else {
        r = wrap(*value.value.o);
      }
      break;
    }

    case Array:
      r = wrap(*value.value.array);
      break;
    case Map:
      r = wrap(*value.value.map);
      break;
    case MapRef:
      /*
      {
      std::shared_ptr<Value::Map_type> map(value.value.mapref->lock());
      if (map)
      {
      std::cout << "wrapmapref not implemented\n";
      }
      }
      */
      r = Py_None;
      break;
    case shcore::Function:
      r = wrap(*value.value.func);
      break;
  }
  return r;
}

}  // namespace py
}  // namespace shcore
