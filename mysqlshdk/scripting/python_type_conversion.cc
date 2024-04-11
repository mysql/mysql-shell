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

#include "scripting/python_type_conversion.h"

#include <cassert>

#include "scripting/obj_date.h"
#include "scripting/python_array_wrapper.h"
#include "scripting/python_function_wrapper.h"
#include "scripting/python_map_wrapper.h"
#include "scripting/python_object_wrapper.h"
#include "scripting/python_utils.h"
#include "scripting/types_python.h"

namespace shcore {
namespace py {

Value convert(PyObject *py, Python_context *context) {
  return convert(py, &context);
}

Value convert(PyObject *py, Python_context **context, bool is_binary) {
  assert(context);

  // Some numeric conversions yield errors, in that case the string
  // representation of the python object is returned

  if (!py || py == Py_None) return Value::Null();
  if (py == Py_False) return Value(false);
  if (py == Py_True) return Value(true);

  auto as_str_repr = [](PyObject *py_obj) -> Value {
    PyErr_Clear();
    if (Release obj_repr{PyObject_Repr(py_obj)}; obj_repr) {
      if (std::string s; Python_context::pystring_to_string(obj_repr, &s))
        return Value(s);
    }

    return {};
  };

  if (PyInt_Check(py) || PyLong_Check(py)) {
    int64_t value = PyLong_AsLongLong(py);
    if (value != -1 || !PyErr_Occurred()) return Value(value);

    // If reading LongLong failed then it attempts
    // reading UnsignedLongLong
    PyErr_Clear();
    uint64_t uint_value = PyLong_AsUnsignedLongLong(py);
    if (static_cast<int64_t>(uint_value) != -1 || !PyErr_Occurred())
      return Value(uint_value);

    // If reading UnsignedLongLong failed then retrieves
    // the string representation
    return as_str_repr(py);
  }

  if (PyFloat_Check(py)) {
    double value = PyFloat_AsDouble(py);
    if (value != -1.0 || !PyErr_Occurred()) return Value(value);

    // At this point it means an error was generated above
    // It is needed to clear it
    return as_str_repr(py);
  }

  if (PyByteArray_Check(py))
    return Value(PyByteArray_AsString(py), PyByteArray_Size(py), true);

  if (PyBytes_Check(py))
    return Value(PyBytes_AsString(py), PyBytes_Size(py), is_binary);

  if (PyUnicode_Check(py)) {
    // TODO: In case of error calls ourself using NULL, either handle here
    // before calling recursively or always allow NULL
    Release ref{PyUnicode_AsUTF8String(py)};

    // After unicode check, the value will match the Bytes check but the content
    // should be treated as string
    return convert(ref.get(), context, false);
  }

  if (array_check(py)) {
    Array_t array;

    if (!unwrap(py, &array))
      throw std::runtime_error("Could not unwrap shell array.");

    return Value(array);
  }

  if (PyList_Check(py)) {
    Py_ssize_t size = PyList_Size(py);

    auto array = std::make_shared<Value::Array_type>();
    array->reserve(size);

    for (Py_ssize_t i = 0; i < size; i++)
      array->emplace_back(convert(PyList_GetItem(py, i), context));

    return Value(array);
  }

  if (PyTuple_Check(py)) {
    Py_ssize_t size = PyTuple_Size(py);

    auto array = std::make_shared<Value::Array_type>();
    array->reserve(size);

    for (Py_ssize_t i = 0; i < size; i++)
      array->emplace_back(convert(PyTuple_GetItem(py, i), context));

    return Value(array);
  }

  if (dict_check(py)) {
    Dictionary_t dict;

    if (!unwrap(py, &dict)) {
      throw std::runtime_error("Could not unwrap shell dictionary.");
    }

    return Value(std::move(dict));
  }

  if (PyDict_Check(py)) {
    auto map = std::make_shared<Value::Map_type>();

    PyObject *key = nullptr, *value = nullptr;
    Py_ssize_t pos = 0;

    while (PyDict_Next(py, &pos, &key, &value)) {
      std::string key_string;

      // The key may be anything (not necessarily a string) so we get the string
      // representation of whatever it is.
      Python_context::pystring_to_string(key, &key_string, true);
      (*map)[key_string] = convert(value, context);
    }

    return Value(map);
  }

  if (PyFunction_Check(py)) {
    if (!*context) {
      try {
        *context = Python_context::get();
      } catch (const std::exception &e) {
        throw std::runtime_error(std::string{"Could not get SHELL context: "} +
                                 e.what());
      }
    }

    return Value(std::make_shared<Python_function>(*context, py));
  }
  // TODO: else if (Buffer/MemoryView || Tuple || DateTime || generic_object

  if (std::shared_ptr<Object_bridge> object; unwrap(py, object))
    return Value(object);

  if (std::shared_ptr<Function_base> function;
      unwrap_method(py, &function) || unwrap(py, function))
    return Value(function);

  if (!*context) {
    try {
      *context = Python_context::get();
    } catch (const std::exception &e) {
      throw std::runtime_error(std::string{"Could not get SHELL context: "} +
                               e.what());
    }
  }

  auto get_int64 = [](PyObject *obj, const char *name) {
    py::Release att{PyObject_GetAttrString(obj, name)};
    return PyLong_AsLongLong(att.get());
  };

  if (PyObject_TypeCheck(py, (*context)->get_datetime_type())) {
    auto year = get_int64(py, "year");
    auto month = get_int64(py, "month");
    auto day = get_int64(py, "day");
    auto hour = get_int64(py, "hour");
    auto min = get_int64(py, "minute");
    auto sec = get_int64(py, "second");
    auto usec = get_int64(py, "microsecond");

    return shcore::Value(
        Object_bridge_ref(new Date(year, month, day, hour, min, sec, usec)));
  }
  if (PyObject_TypeCheck(py, (*context)->get_date_type())) {
    auto year = get_int64(py, "year");
    auto month = get_int64(py, "month");
    auto day = get_int64(py, "day");

    return shcore::Value(Object_bridge_ref(new Date(year, month, day)));
  }
  if (PyObject_TypeCheck(py, (*context)->get_time_type())) {
    auto hour = get_int64(py, "hour");
    auto min = get_int64(py, "minute");
    auto sec = get_int64(py, "second");
    auto usec = get_int64(py, "microsecond");

    return shcore::Value(Object_bridge_ref(new Date(hour, min, sec, usec)));
  }

  // Any other object gets simply wrapped
  return Value(std::make_shared<Python_object>(py));
}

void set_item(const Dictionary_t &map, PyObject *key, PyObject *value,
              Python_context *context) {
  set_item(map, key, value, &context);
}

void set_item(const Dictionary_t &map, PyObject *key, PyObject *value,
              Python_context **context) {
  assert(context);

  std::string key_as_string;

  // The key may be anything (not necessarily a string) so we get the string
  // representation of whatever it is.
  Python_context::pystring_to_string(key, &key_as_string, true);
  map.get()->set(key_as_string, convert(value, context));
}

py::Release convert(const Value &value, Python_context * /*context*/) {
  switch (value.get_type()) {
    case Undefined:
    case Null:
      return py::Release::incref(Py_None);
    case Bool:
      return py::Release{PyBool_FromLong(value.as_bool())};
    case String:
      return py::Release{PyString_FromString(value.get_string().c_str())};
    case Integer:
      return py::Release{PyLong_FromLongLong(value.as_int())};
      break;
    case UInteger:
      return py::Release{PyLong_FromUnsignedLongLong(value.as_uint())};
      break;
    case Float:
      return py::Release{PyFloat_FromDouble(value.as_double())};
      break;
    case Object: {
      if (auto object = value.as_object<Python_object>())
        return py::Release::incref(object->object());

      if (value.as_object()->class_name() != "Date")
        return wrap(value.as_object());

      std::shared_ptr<Date> date = value.as_object<Date>();

      // 0 dates are invalid in Python, but OK in MySQL, so we convert these
      // to None, which is what they probably really mean
      if (date->has_date() && date->get_year() == 0 && date->get_month() == 0 &&
          date->get_day() == 0) {
        return py::Release::incref(Py_None);
      }

      auto ctx = Python_context::get();

      py::Release r;
      if (date->has_time()) {
        if (!date->has_date())
          r = ctx->create_time_object(date->get_hour(), date->get_min(),
                                      date->get_sec(), date->get_usec());
        else
          r = ctx->create_datetime_object(date->get_year(), date->get_month(),
                                          date->get_day(), date->get_hour(),
                                          date->get_min(), date->get_sec(),
                                          date->get_usec());

      } else {
        r = ctx->create_date_object(date->get_year(), date->get_month(),
                                    date->get_day());
      }

      if (r) return r;

      // The conversion failed, so we take the string representation of the
      // object

      // Cleanup the error condition
      ctx->clear_exception();

      // Take the object string representation
      return py::Release{PyString_FromString(value.descr().c_str())};
    }
    case Array:
      return wrap(value.as_array());
    case Map:
      return wrap(value.as_map());
    case shcore::Function:
      return wrap(value.as_function());
    case shcore::Binary:
      return py::Release{PyBytes_FromStringAndSize(value.get_string().c_str(),
                                                   value.get_string().size())};
  }

  return {};
}

}  // namespace py
}  // namespace shcore
