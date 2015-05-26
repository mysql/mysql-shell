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

#ifndef _PYTHON_CONTEXT_H_
#define _PYTHON_CONTEXT_H_

#include <Python.h>

#include "shellcore/shell_python.h"

#include "shellcore/python_type_conversion.h"
#include "shellcore/lang_base.h"
#include <string>

namespace shcore {

class AutoPyObject
  {
  private:
    PyObject *object;
    bool autorelease;
  public:
    AutoPyObject()
      : object(0), autorelease(false)
    {
    }

    // Assigning another auto object always makes this one ref-counting as they share
    // now the same object. Same for the assignment operator.
    AutoPyObject(const AutoPyObject &other)
      : object(other.object), autorelease(true)
    {
      Py_XINCREF(object);
    }

    AutoPyObject(PyObject *py, bool retain = true)
    : object(py)
    {
      autorelease = retain;
      if (autorelease)
      {
        // Leave the braces in place, even though this is a one liner. They will silence LLVM.
        Py_XINCREF(object);
      }
    }

    ~AutoPyObject()
    {
      if (autorelease)
      {
        Py_XDECREF(object);
      }
    }

    AutoPyObject &operator= (PyObject *other)
    {
      if (object == other) // Ignore assignments of the same object.
        return *this;

      // Auto release only if we actually have increased its ref count.
      // Always make this auto object auto-releasing after that as we get an object that might
      // be shared by another instance.
      if (autorelease)
      {
        Py_XDECREF(object);
      }

      object = other;
      autorelease = true;
      Py_XINCREF(object);

      return *this;
    }

    operator bool()
    {
      return object != 0;
    }

    operator PyObject*()
    {
      return object;
    }

    PyObject* operator->()
    {
      return object;
    }
  };

struct Interpreter_delegate;

class Python_context
{
public:
  Python_context(Interpreter_delegate *deleg) throw (Exception);
  ~Python_context();

  static Python_context *get();
  static Python_context *get_and_check();
  PyObject *get_shell_module();

  Value execute(const std::string &code, boost::system::error_code &ret_error, const std::string& source = "") throw (Exception);
  Value execute_interactive(const std::string &code) BOOST_NOEXCEPT_OR_NOTHROW;

  Value pyobj_to_shcore_value(PyObject *value);
  PyObject *shcore_value_to_pyobj(const Value &value);

  Value get_global(const std::string &value);
  void set_global(const std::string &name, const Value &value);

  static void set_python_error(const std::exception &exc, const std::string &location="");
  bool pystring_to_string(PyObject *strobject, std::string &ret_string, bool convert=false);

  AutoPyObject get_shell_list_class();
  AutoPyObject get_shell_dict_class();

  Interpreter_delegate *_delegate;

private:
  PyObject *_globals;
  PyThreadState *_main_thread_state;

  Python_type_bridger _types;

  PyObject *_shell_module;

  PyObject *shell_print(PyObject *self, PyObject *args);

  void register_shell_module();
  void init_shell_list_type();
  void init_shell_dict_type();

protected:
  AutoPyObject _shell_list_class;
  AutoPyObject _shell_dict_class;
};

}

#endif
