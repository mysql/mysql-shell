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

#ifndef _PYTHON_CONTEXT_H_
#define _PYTHON_CONTEXT_H_

// keep the include order as is, otherwise we hit the Python issue 10910 (macOS)

#include "scripting/python_utils.h"

#include "scripting/lang_base.h"
#include "scripting/types.h"
#include "scripting/types_common.h"
#include "utils/utils_file.h"

#include <cassert>
#include <list>
#include <memory>
#include <string>
#include <utility>

#include "scripting/python_type_conversion.h"

namespace shcore {
namespace py {

/**
 * Used to store borrowed references, references passed from Python (i.e. as
 * arguments), or new references if such reference is returned to Python.
 *
 * Increases the reference count when acquiring the Python object, decreases it
 * when releasing the object.
 */
class Store {
 public:
  Store() = default;

  explicit Store(PyObject *py) : m_object(py) { Py_XINCREF(m_object); }

  Store(const Store &other) : Store(other.m_object) {}

  Store(Store &&other) { operator=(std::move(other)); }

  Store &operator=(PyObject *other) {
    // increasing the reference count of the other object then decreasing
    // the reference count of our object protects from self-assignment
    Py_XINCREF(other);
    Py_XDECREF(m_object);

    m_object = other;

    return *this;
  }

  Store &operator=(const Store &other) { return operator=(other.m_object); }

  Store &operator=(Store &&other) {
    // the other instance already increased the reference count, just move the
    // pointer
    assert(this != &other);

    Py_XDECREF(m_object);

    m_object = other.m_object;
    other.m_object = nullptr;

    return *this;
  }

  ~Store() { Py_XDECREF(m_object); }

  explicit operator bool() const { return nullptr != m_object; }

  operator PyObject *() const { return m_object; }

 private:
  PyObject *m_object = nullptr;
};

/**
 * Used to automatically release new references.
 *
 * Does not increase the reference count, decreases it when releasing the Python
 * object.
 */
class Release {
 public:
  Release() = default;

  explicit Release(PyObject *py) : m_object(py) {}

  Release(const Release &other) = delete;

  Release(Release &&other) { operator=(std::move(other)); }

  Release &operator=(const Release &other) = delete;

  Release &operator=(Release &&other) {
    assert(this != &other);

    m_object = other.m_object;
    other.m_object = nullptr;

    return *this;
  }

  ~Release() { Py_XDECREF(m_object); }

  explicit operator bool() const { return nullptr != m_object; }

  operator PyObject *() const { return m_object; }

 private:
  PyObject *m_object = nullptr;
};

}  // namespace py

class TYPES_COMMON_PUBLIC Python_context {
 public:
  Python_context(bool redirect_stdio);
  ~Python_context();

  static Python_context *get();
  static Python_context *get_and_check();
  PyObject *get_shell_stderr_module();
  PyObject *get_shell_stdout_module();
  PyObject *get_shell_stdin_module();
  PyObject *get_shell_python_support_module();

  Value execute(const std::string &code, const std::string &source = "");
  Value execute_interactive(const std::string &code, Input_state &r_state,
                            bool flush = true) noexcept;

  std::vector<std::pair<bool, std::string>> list_globals();
  static void get_members_of(
      PyObject *object, std::vector<std::pair<bool, std::string>> *out_keys);

  Value execute_module(const std::string &module,
                       const std::vector<std::string> &argv);
  bool load_plugin(const Plugin_definition &plugin);

  Value convert(PyObject *value);
  PyObject *convert(const Value &value);

  Value get_global(const std::string &value);
  void set_global(const std::string &name, const Value &value);
  void set_argv(const std::vector<std::string> &argv);

  PyObject *get_global_py(const std::string &value);

  static void set_python_error(const shcore::Exception &exc,
                               const std::string &location = "");
  static void set_python_error(const std::exception &exc,
                               const std::string &location = "");
  static void set_shell_error(const shcore::Error &e);
  static void set_python_error(PyObject *obj, const std::string &location);
  static bool pystring_to_string(PyObject *strobject, std::string *result,
                                 bool convert = false);

  py::Store get_shell_list_class() const;
  py::Store get_shell_dict_class() const;
  py::Store get_shell_object_class() const;
  py::Store get_shell_indexed_object_class() const;
  py::Store get_shell_function_class() const;

  PyObject *db_error() { return _db_error; }
  PyObject *error() { return _error; }

  std::string fetch_and_clear_exception();
  void throw_if_mysqlsh_error();

  bool raw_execute(const std::string &statement, std::string *error = nullptr);
  bool raw_execute(const std::vector<std::string> &statements,
                   std::string *error = nullptr);

  std::weak_ptr<py::Store> store(PyObject *object);
  void erase(const std::shared_ptr<py::Store> &object);

  PyObject *create_datetime_object(int year, int month, int day, int hour,
                                   int minute, int second, int useconds);
  PyTypeObject *get_datetime_type() const { return _datetime_type; }

 private:
  static PyObject *shell_print(PyObject *self, PyObject *args,
                               const std::string &stream);
  static PyObject *shell_flush(PyObject *self, PyObject *args);
  static PyObject *shell_flush_stderr(PyObject *self, PyObject *args);
  static PyObject *shell_stdout(PyObject *self, PyObject *args);
  static PyObject *shell_stdout_isatty(PyObject *self, PyObject *args);
  static PyObject *shell_stderr(PyObject *self, PyObject *args);
  static PyObject *shell_stderr_isatty(PyObject *self, PyObject *args);
  static PyObject *shell_raw_input(PyObject *self, PyObject *args);
  static PyObject *shell_stdin_read(PyObject *self, PyObject *args);
  static PyObject *shell_stdin_readline(PyObject *self, PyObject *args);
  static PyObject *shell_stdin_isatty(PyObject *self, PyObject *args);
  static PyObject *shell_interactive_eval_hook(PyObject *self, PyObject *args);

  std::pair<shcore::Prompt_result, std::string> read_line(
      const std::string &prompt);

  static bool exit_error;
  static bool module_processing;
  static PyMethodDef ShellStdErrMethods[];
  static PyMethodDef ShellStdOutMethods[];
  static PyMethodDef ShellStdInMethods[];
  static PyMethodDef ShellPythonSupportMethods[];

  PyObject *_globals;
  PyObject *_locals;
  PyThreadState *_main_thread_state;
  std::string _stdin_buffer;

  PyObject *_db_error;
  PyObject *_error;

  PyObject *_datetime = nullptr;
  PyTypeObject *_datetime_type = nullptr;

  PyObject *_mysqlsh_module = nullptr;
  PyObject *_mysqlsh_globals = nullptr;

  PyObject *_shell_stderr_module = nullptr;
  PyObject *_shell_stdout_module = nullptr;
  PyObject *_shell_stdin_module = nullptr;
  PyObject *_shell_python_support_module = nullptr;

  // compiler flags are needed to detect imports from __future__, so they are
  // available for subsequent executions
  PyCompilerFlags m_compiler_flags;

  void register_mysqlsh_builtins();
  void register_mysqlsh_module();

  void get_datetime_constructor();

  bool raw_execute_helper(const std::string &statement, std::string *error);

  void register_shell_stderr_module();
  void register_shell_stdout_module();
  void register_shell_stdin_module();
  void register_shell_python_support_module();

  void init_shell_list_type();
  void init_shell_dict_type();
  void init_shell_object_type();
  void init_shell_function_type();

  py::Store m_captured_eval_result;

  std::list<std::shared_ptr<py::Store>> m_stored_objects;

 protected:
  py::Store _shell_list_class;
  py::Store _shell_dict_class;
  py::Store _shell_object_class;
  py::Store _shell_indexed_object_class;
  py::Store _shell_function_class;
};

// The static member _instance needs to be in a class not exported (no
// TYPES_COMMON_PUBLIC), otherwise MSVC complains with C2491.
class Python_init_singleton final {
 public:
  Python_init_singleton(const Python_init_singleton &py) = delete;
  Python_init_singleton(Python_init_singleton &&py) = delete;
  ~Python_init_singleton();
  Python_init_singleton &operator=(const Python_init_singleton &py) = delete;
  Python_init_singleton &operator=(Python_init_singleton &&py) = delete;

  static void init_python();
  static void destroy_python();

  // Initializing and finalizing the Python context multiple times when tests
  // are executed doesn't work well on macOS, so on that platform we use this
  // function instead of destroy_python which is needed to be able to explicitly
  // destroy it before the main logger will be removed.

  static std::string get_new_scope_name();

 private:
  Python_init_singleton();
  static std::unique_ptr<Python_init_singleton> s_instance;
  static unsigned int s_cnt;
  bool m_local_initialization;
};
}  // namespace shcore

#endif
