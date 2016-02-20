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

#include "shellcore/types_common.h"
#include "shellcore/shell_python.h"
#include "utils/utils_file.h"

#include "shellcore/python_type_conversion.h"
#include "shellcore/lang_base.h"
#include <string>
#include <list>

namespace shcore
{
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

  // The static member _instance needs to be in a class not exported (no TYPES_COMMON_PUBLIC), otherwise MSVC complains with C2491.
  class Python_init_singleton
  {
  public:

    ~Python_init_singleton()
    {
      if (_local_initialization)
        Py_Finalize();
    }

    static std::string get_new_scope_name();

    static void init_python();
    
  private:
    static int cnt;
    bool _local_initialization;
    static std::unique_ptr<Python_init_singleton> _instance;
    
    Python_init_singleton(const Python_init_singleton& py) { }

    Python_init_singleton() : _local_initialization(false)
    {
      if (!Py_IsInitialized())
      {
#ifdef _WINDOWS
        Py_NoSiteFlag = 1;

        char path[1000];
        char *env_value;

        // If PYTHONHOME is available, honors it
        env_value = getenv("PYTHONHOME");
        if (env_value)
          strcpy(path, env_value);
        else
        {
          // If not will associate what should be the right path in
          // a standard distribution
          std::string python_path;
          python_path = shcore::get_mysqlx_home_path();
          if (!python_path.empty())
            python_path.append("\\lib\\Python2.7");
          else
          {
            // Not a standard distribution
            python_path = shcore::get_binary_folder();
            python_path.append("\\Python2.7");
          }

          strcpy(path, python_path.c_str());
        }

        Py_SetPythonHome(path);
#endif
        Py_InitializeEx(0);

        _local_initialization = true;
      }
    }
  };

  struct Interpreter_delegate;

  class TYPES_COMMON_PUBLIC Python_context
  {
  public:
    Python_context(Interpreter_delegate *deleg) throw (Exception);
    ~Python_context();

    static Python_context *get();
    static Python_context *get_and_check();
    PyObject *get_shell_module();
    PyObject *get_shell_stderr_module();

    Value execute(const std::string &code, boost::system::error_code &ret_error, const std::string& source = "") throw (Exception);
    Value execute_interactive(const std::string &code, Interactive_input_state &r_state) BOOST_NOEXCEPT_OR_NOTHROW;

    Value pyobj_to_shcore_value(PyObject *value);
    PyObject *shcore_value_to_pyobj(const Value &value);

    Value get_global(const std::string &value);
    void set_global(const std::string &name, const Value &value);
    void set_global_item(const std::string &global_name, const std::string &item_name, const Value &value);

    static void set_python_error(const shcore::Exception &exc, const std::string &location = "");
    static void set_python_error(const std::exception &exc, const std::string &location = "");
    static void set_python_error(PyObject *obj, const std::string &location);
    bool pystring_to_string(PyObject *strobject, std::string &ret_string, bool convert = false);

    AutoPyObject get_shell_list_class();
    AutoPyObject get_shell_dict_class();
    AutoPyObject get_shell_object_class();
    AutoPyObject get_shell_indexed_object_class();
    AutoPyObject get_shell_function_class();

    Interpreter_delegate *_delegate;

  public:
    static PyObject *shell_print(PyObject *self, PyObject *args, const std::string& stream);
    static PyObject *shell_prompt(PyObject *self, PyObject *args);
    static PyObject *shell_stdout(PyObject *self, PyObject *args);
    static PyObject *shell_stderr(PyObject *self, PyObject *args);
    static PyObject *shell_interactive_eval_hook(PyObject *self, PyObject *args);

    static PyObject *get_constant(PyObject *self, PyObject *args, const std::string &group, const std::string& id);
    static PyObject *get_object(PyObject *self, PyObject *args, const std::string &module, const std::string &type);
    static PyObject *mysqlx_get_session(PyObject *self, PyObject *args);
    static PyObject *mysqlx_get_node_session(PyObject *self, PyObject *args);
    static PyObject *mysqlx_expr(PyObject *self, PyObject *args);
    static PyObject *mysqlx_text(PyObject *self, PyObject *args);
    static PyObject *mysqlx_blob(PyObject *self, PyObject *args);
    static PyObject *mysqlx_decimal(PyObject *self, PyObject *args);
    static PyObject *mysqlx_numeric(PyObject *self, PyObject *args);
    static PyObject *mysql_get_classic_session(PyObject *self, PyObject *args);

  private:
    PyObject *_globals;
    PyObject *_locals;
    PyThreadState *_main_thread_state;

    Python_type_bridger _types;

    PyObject *_shell_module;
    PyObject *_shell_stderr_module;
    PyObject *_mysqlx_module;
    PyObject *_mysql_module;

    void register_shell_module();
    void register_shell_stderr_module();
    void register_mysqlx_module();
    void register_mysql_module();

    void init_shell_list_type();
    void init_shell_dict_type();
    void init_shell_object_type();
    void init_shell_function_type();

    std::list<AutoPyObject> _captured_eval_result;

  protected:
    AutoPyObject _shell_list_class;
    AutoPyObject _shell_dict_class;
    AutoPyObject _shell_object_class;
    AutoPyObject _shell_indexed_object_class;
    AutoPyObject _shell_function_class;
  };
}

#endif
