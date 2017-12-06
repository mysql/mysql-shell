/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _PYTHON_CONTEXT_H_
#define _PYTHON_CONTEXT_H_

#include "python_utils.h"

#include "scripting/types_common.h"
#include "scripting/types.h"
#include "scripting/lang_base.h"
#include "utils/utils_file.h"

#include "scripting/python_type_conversion.h"
#include <list>
#include <string>
#include <utility>


namespace shcore {
class AutoPyObject {
private:
  PyObject *object;
  bool autorelease;
public:
  AutoPyObject()
    : object(0), autorelease(false) {}

  // Assigning another auto object always makes this one ref-counting as they share
  // now the same object. Same for the assignment operator.
  AutoPyObject(const AutoPyObject &other)
    : object(other.object), autorelease(true) {
    Py_XINCREF(object);
  }

  AutoPyObject(PyObject *py, bool retain = true)
  : object(py) {
    autorelease = retain;
    if (autorelease) {
      // Leave the braces in place, even though this is a one liner. They will silence LLVM.
      Py_XINCREF(object);
    }
  }

  ~AutoPyObject() {
    if (autorelease) {
      Py_XDECREF(object);
    }
  }

  AutoPyObject &operator= (PyObject *other) {
    if (object == other) // Ignore assignments of the same object.
      return *this;

    // Auto release only if we actually have increased its ref count.
    // Always make this auto object auto-releasing after that as we get an object that might
    // be shared by another instance.
    if (autorelease) {
      Py_XDECREF(object);
    }

    object = other;
    autorelease = true;
    Py_XINCREF(object);

    return *this;
  }

  operator bool() {
    return object != 0;
  }

  operator PyObject*() {
    return object;
  }

  PyObject* operator->() {
    return object;
  }
};

struct Interpreter_delegate;

class TYPES_COMMON_PUBLIC Python_context {
public:
  Python_context(Interpreter_delegate *deleg, bool redirect_stdio);
  ~Python_context();

  static Python_context *get();
  static Python_context *get_and_check();
  PyObject *get_shell_stderr_module();
  PyObject *get_shell_stdout_module();
  PyObject *get_shell_stdin_module();
  PyObject *get_shell_python_support_module();

  Value execute(const std::string &code, const std::string& source = "",
      const std::vector<std::string> &argv = {});
  Value execute_interactive(const std::string &code,
                            Input_state &r_state) noexcept;

  std::vector<std::pair<bool, std::string>> list_globals();
  static void get_members_of(
      PyObject *object, std::vector<std::pair<bool, std::string>> *out_keys);

  bool is_module(const std::string& file_name);
  Value execute_module(const std::string& file_name, const std::vector<std::string> &argv);

  Value pyobj_to_shcore_value(PyObject *value);
  PyObject *shcore_value_to_pyobj(const Value &value);

  Value get_global(const std::string &value);
  void set_global(const std::string &name, const Value &value);
  void set_global_item(const std::string &global_name, const std::string &item_name, const Value &value);

  PyObject *get_global_py(const std::string &value);

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

  PyObject *db_error() { return _db_error; }

private:
  static PyObject *shell_print(PyObject *self, PyObject *args, const std::string& stream);
  static PyObject *shell_flush(PyObject *self, PyObject *args);
  static PyObject *shell_flush_stderr(PyObject *self, PyObject *args);
  static PyObject *shell_stdout(PyObject *self, PyObject *args);
  static PyObject *shell_stderr(PyObject *self, PyObject *args);
  static PyObject *shell_raw_input(PyObject *self, PyObject *args);
  static PyObject *shell_stdin_read(PyObject *self, PyObject *args);
  static PyObject *shell_stdin_readline(PyObject *self, PyObject *args);
  static PyObject *shell_interactive_eval_hook(PyObject *self, PyObject *args);

  std::pair<shcore::Prompt_result, std::string> read_line(
      const std::string &prompt);

  static bool exit_error;
  static bool module_processing;
  static PyMethodDef ShellStdErrMethods[];
  static PyMethodDef ShellStdOutMethods[];
  static PyMethodDef ShellStdInMethods[];
  static PyMethodDef ShellPythonSupportMethods[];
private:
  PyObject *_global_namespace;
  PyObject *_globals;
  PyObject *_locals;
  PyThreadState *_main_thread_state;
  std::string _stdin_buffer;

  PyObject *_db_error;

  Python_type_bridger _types;

  PyObject *_shell_mysqlsh_module;

  PyObject *_shell_stderr_module;
  PyObject *_shell_stdout_module;
  PyObject *_shell_stdin_module;
  PyObject *_shell_python_support_module;

  std::map<PyObject*, std::shared_ptr<shcore::Object_bridge> > _modules;

  void register_mysqlsh_module();
  PyObject *call_module_function(PyObject *self, PyObject *args, PyObject *keywords, const std::string& name);

  void register_shell_stderr_module();
  void register_shell_stdout_module();
  void register_shell_stdin_module();
  void register_shell_python_support_module();

  void init_shell_list_type();
  void init_shell_dict_type();
  void init_shell_object_type();
  void init_shell_function_type();

  void set_argv(const std::vector<std::string> &argv);

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
