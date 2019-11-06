/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include "scripting/python_context.h"

#include <signal.h>
#include <exception>

#include "mysqlshdk/include/shellcore/console.h"
#include "scripting/common.h"
#include "scripting/module_registry.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_path.h"
#include "utils/utils_string.h"

#include "scripting/object_factory.h"
#include "scripting/python_type_conversion.h"

#include "modules/mod_utils.h"

#ifdef _WINDOWS
#include <windows.h>
#endif

// used to identify a proper SHELL context object as a PyCObject
static const char *SHELLTypeSignature = "SHELLCONTEXT";

extern const char *g_mysqlsh_path;

namespace shcore {

namespace {

#ifdef IS_PY3K

using PY_CHAR_T = wchar_t;
#define PY_CHAR_T_LITERAL(x) L##x
#define COPY_PY_CHAR_T(dst, src, n) mbstowcs(dst, src, n)

std::vector<std::wstring> to_py_char_t(const std::vector<std::string> &in) {
  std::vector<std::wstring> out;

  for (const auto &s : in) {
    out.emplace_back(s.length(), PY_CHAR_T_LITERAL('#'));
    COPY_PY_CHAR_T(&out.back()[0], s.c_str(), out.back().length());
  }

  return out;
}

#else  // !IS_PY3K

using PY_CHAR_T = char;
#define PY_CHAR_T_LITERAL(x) x
#define COPY_PY_CHAR_T(dst, src, n) strncpy(dst, src, n)

const std::vector<std::string> &to_py_char_t(
    const std::vector<std::string> &in) {
  return in;
}

#endif  // !IS_PY3K

void set_python_home(const std::string &home) {
  if (!home.empty()) {
    static PY_CHAR_T path[1000];

    if (home.length() > array_size(path) - 1) {
      throw std::runtime_error("Python home path too long");
    }

    COPY_PY_CHAR_T(path, home.c_str(), array_size(path) - 1);
    Py_SetPythonHome(path);
  }
}

void set_program_name() {
  static PY_CHAR_T name[1000];
  COPY_PY_CHAR_T(name, g_mysqlsh_path, array_size(name) - 1);
  Py_SetProgramName(name);
}

PyObject *py_register_module(const std::string &name, PyMethodDef *members) {
#ifdef IS_PY3K
  PyModuleDef *moduledef = new PyModuleDef{
      PyModuleDef_HEAD_INIT,
      nullptr,
      nullptr,
      -1,  // module does not support sub-interpreters, it has global state
      members,
      nullptr,
      nullptr,
      nullptr,
      [](void *module) {
        auto definition = PyModule_GetDef(static_cast<PyObject *>(module));

        PyDict_DelItemString(PyImport_GetModuleDict(), definition->m_name);
        delete[] definition->m_name;
        delete definition;
      }};

  const auto length = name.length();
  auto copied_name = new char[length + 1];
  strncpy(copied_name, name.c_str(), length);
  copied_name[length] = '\0';
  moduledef->m_name = copied_name;

  const auto module = PyModule_Create(moduledef);

  // create ModuleSpec to indicate it's a built-in module
  {
    const auto machinery = PyImport_ImportModule("importlib.machinery");

    if (machinery) {
      const auto modulespec = PyObject_GetAttrString(machinery, "ModuleSpec");

      if (modulespec) {
        const auto args = Py_BuildValue("(ss)", name.c_str(), nullptr);
        const auto kwargs = Py_BuildValue("{s:s}", "origin", "built-in");

        const auto spec = PyObject_Call(modulespec, args, kwargs);
        PyDict_SetItemString(PyModule_GetDict(module), "__spec__", spec);

        Py_XDECREF(spec);
        Py_XDECREF(kwargs);
        Py_XDECREF(args);
        Py_XDECREF(modulespec);
      }

      Py_XDECREF(machinery);
    }
  }

  // module needs to be added to the sys.modules
  PyDict_SetItemString(PyImport_GetModuleDict(), moduledef->m_name, module);
  return module;
#else   // !IS_PY3K
  return Py_InitModule(name.c_str(), members);
#endif  // !IS_PY3K
}

};  // namespace

bool Python_context::exit_error = false;
bool Python_context::module_processing = false;

// The static member _instance needs to be in a class not exported (no
// TYPES_COMMON_PUBLIC), otherwise MSVC complains with C2491.
class Python_init_singleton final {
 public:
  Python_init_singleton(const Python_init_singleton &py) = delete;
  Python_init_singleton(Python_init_singleton &&py) = delete;

  ~Python_init_singleton() {
    if (m_local_initialization) Py_Finalize();
  }

  Python_init_singleton &operator=(const Python_init_singleton &py) = delete;
  Python_init_singleton &operator=(Python_init_singleton &&py) = delete;

  static void init_python() {
    if (!s_instance) s_instance.reset(new Python_init_singleton());
  }

  // Initializing and finalizing the Python context multiple times when tests
  // are executed doesn't work well on macOS, hence there's no method to
  // destroy the singleton.

  static std::string get_new_scope_name() {
    return shcore::str_format("__main__%u", ++s_cnt);
  }

 private:
  Python_init_singleton() : m_local_initialization(false) {
    if (!Py_IsInitialized()) {
      std::string home;
#ifdef _WIN32
#define MAJOR_MINOR STRINGIFY(PY_MAJOR_VERSION) "." STRINGIFY(PY_MINOR_VERSION)
      const auto env_value = getenv("PYTHONHOME");

      // If PYTHONHOME is available, honors it
      if (env_value) {
        log_info("Setting Python home to '%s' from PYTHONHOME", env_value);
        home = env_value;
      } else {
        // If not will associate what should be the right path in
        // a standard distribution
        std::string python_path = shcore::get_mysqlx_home_path();

        if (!python_path.empty()) {
          python_path =
              shcore::path::join_path(python_path, "lib", "Python" MAJOR_MINOR);
        } else {
          // Not a standard distribution
          python_path = shcore::get_binary_folder();
          python_path =
              shcore::path::join_path(python_path, "Python" MAJOR_MINOR);
        }

        log_info("Setting Python home to '%s'", python_path.c_str());
        home = python_path;
      }
#undef MAJOR_MINOR
#else  // !_WIN32
      const auto env_value = getenv("PYTHONHOME");

      // If PYTHONHOME is available, honors it
      if (env_value) {
        log_info("Setting Python home to '%s' from PYTHONHOME", env_value);
        home = env_value;
      } else {
#if defined(HAVE_PYTHON) && HAVE_PYTHON == 2
        // If not will associate what should be the right path in
        // a standard distribution
        std::string python_path = shcore::path::join_path(
            shcore::get_mysqlx_home_path(), "lib", "mysqlsh");
        if (shcore::is_folder(python_path)) {
          // Override the system Python install with the bundled one
          log_info("Setting Python home to '%s'", python_path.c_str());
          home = python_path;
        }
#endif
      }
#endif  // !_WIN32
      set_python_home(home);
      set_program_name();
      Py_InitializeEx(0);

      // Initialize the signal module, so we can use PyErr_SetInterrupt().
      //
      // Python 3.5+ uses PyErr_CheckSignals() to double check if time.sleep()
      // was interrupted and continues to sleep if that function returns 0.
      // PyErr_CheckSignals() returns non-zero if a signal was raised (either
      // via PyErr_SetInterrupt() or Python's signal handler) AND if the call to
      // the Python callback set for the raised signal results in an exception.
      //
      // The initialization of signal module installs its own handler for
      // a signal only if it's currently set to SIG_DFL. In case of SIGINT it
      // will then also set the Python callback to signal.default_int_handler,
      // which generates KeyboardInterrupt when called. Since on non-Windows
      // platforms we have our own SIGINT handler, the initialization of signal
      // module is not going to install a handler and Python callback will
      // remain to be set to None. If PyErr_CheckSignals() was called with such
      // setup (after a previous call to PyErr_SetInterrupt() to raise the
      // SIGINT signal, since Python's signal handler was not installed), it
      // would return non-zero, but the exception would be "'NoneType' object is
      // not callable".
      //
      // If we set the Python callback after the signal module is initialized
      // (i.e.: signal.signal(signal.SIGINT, signal.default_int_handler)),
      // we will lose our capability to detect SIGINT (i.e. in JS), as Python's
      // implementation which sets that callback overwrites current (our) signal
      // handler with its own.
      //
      // In order to overcome these issues, we proceed as follows:
      //  1. Set the SIGINT handler to SIG_DFL, store the previous handler.
      //  2. Initialize the signal module, it will install its own SIGINT
      //     handler and set the callback to signal.default_int_handler.
      //  3. Restore the previous handler, overwriting Python's signal handler.
      // This will give us an initialized signal module with Python callback for
      // SIGINT set, but with no Python signal handler. We can continue to use
      // our own signal handler, and deliver the CTRL-C notification to Python
      // via PyErr_SetInterrupt().
      //
      // Python 2 @ Windows:
      // Initializing the signal module early and in a controlled way will also
      // prevent an issue when user imports this module and calls time.sleep().
      // In that case sleep cannot be interrupted by CTRL-C, because the
      // initialization of signal module will overwrite the default signal
      // handler and the mechanism used by Python 2.7 to wake up will not work.
      {
        const auto prev_signal = signal(SIGINT, SIG_DFL);
        PyOS_InitInterrupts();
        signal(SIGINT, prev_signal);
      }

      PY_CHAR_T *argv[] = {(PY_CHAR_T *)PY_CHAR_T_LITERAL("")};
      PySys_SetArgvEx(1, argv, 0);

      m_local_initialization = true;
    }
  }

  static std::unique_ptr<Python_init_singleton> s_instance;
  static unsigned int s_cnt;
  bool m_local_initialization;
};

std::unique_ptr<Python_init_singleton> Python_init_singleton::s_instance;
unsigned int Python_init_singleton::s_cnt = 0;

Python_context::Python_context(bool redirect_stdio) : _types(this) {
  m_compiler_flags.cf_flags = 0;

  Python_init_singleton::init_python();

  _global_namespace = PyImport_AddModule("__main__");
  _globals = PyModule_GetDict(_global_namespace);

  _locals = _globals;

  if (!_global_namespace || !_globals) {
    throw Exception::runtime_error("Error initializing python context.");
  }

  // register shell module
  register_shell_stderr_module();
  register_shell_stdout_module();
  register_shell_python_support_module();
  register_mysqlsh_module();

  PySys_SetObject(const_cast<char *>("real_stdout"),
                  PySys_GetObject(const_cast<char *>("stdout")));
  PySys_SetObject(const_cast<char *>("real_stderr"),
                  PySys_GetObject(const_cast<char *>("stderr")));
  PySys_SetObject(const_cast<char *>("real_stdin"),
                  PySys_GetObject(const_cast<char *>("stdin")));

  // make sys.stdout and sys.stderr send stuff to SHELL
  PySys_SetObject(const_cast<char *>("stdout"), get_shell_stdout_module());
  PySys_SetObject(const_cast<char *>("stderr"), get_shell_stderr_module());

  // set stdin to the shell console when on interactive mode
  if (redirect_stdio) {
    register_shell_stdin_module();
    PySys_SetObject(const_cast<char *>("stdin"), get_shell_stdin_module());

    // also override raw_input() to use the prompt function
    PyObject *dict = PyModule_GetDict(get_shell_python_support_module());
    assert(dict != nullptr);

    PyObject *raw_input = PyDict_GetItemString(dict, "raw_input");
    PyDict_SetItemString(_globals, "raw_input", raw_input);
    PyDict_SetItemString(
        PyModule_GetDict(PyDict_GetItemString(_globals, "__builtins__")),
        "raw_input", raw_input);
  }

  // Stores the main thread state
  _main_thread_state = PyThreadState_Get();

  PyEval_InitThreads();
  PyEval_SaveThread();

  _types.init();
}

bool Python_context::raw_execute_helper(const std::string &statement,
                                        std::string * /* error */) {
  bool ret_val = false;
  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);
  try {
    // we're not using the compiler flags here, as this is an internal method
    // and we don't want to interfere with the user context
    PyObject *py_result =
        PyRun_String(statement.c_str(), Py_single_input, _globals, _locals);

    if (py_result) {
      ret_val = true;
    } else {
      // Even it is internal processing, the information provided
      // By python is better as it is clear, descriptive and visible
      // So we print it right away
      PyErr_Print();
    }
  } catch (const std::exception &e) {
    mysqlsh::current_console()->print_error(e.what());
    log_error("Python error: %s", e.what());

    assert(0);
  }

  return ret_val;
}

bool Python_context::raw_execute(const std::string &statement,
                                 std::string *error) {
  WillEnterPython lock;
  return raw_execute_helper(statement, error);
}

bool Python_context::raw_execute(const std::vector<std::string> &statements,
                                 std::string *error) {
  WillEnterPython lock;
  bool ret_val = true;

  size_t index = 0;
  while (ret_val && index < statements.size())
    ret_val = raw_execute_helper(statements[index++], error);

  return ret_val;
}

Python_context::~Python_context() {
  {
    WillEnterPython lock;

    m_stored_objects.clear();

    // Release all internal module references
    const auto py_mysqlsh_dict = PyModule_GetDict(_shell_mysqlsh_module);
    const auto keys = PyDict_Keys(py_mysqlsh_dict);

    for (Py_ssize_t i = 0, c = PyList_GET_SIZE(keys); i < c; ++i) {
      const auto module_name = PyList_GetItem(keys, i);
      const auto module = PyDict_GetItem(py_mysqlsh_dict, module_name);

      if (PyModule_Check(module) && _modules.find(module) != _modules.end()) {
        PyDict_DelItem(py_mysqlsh_dict, module_name);
      }
    }

    Py_XDECREF(keys);
  }

  PyEval_RestoreThread(_main_thread_state);
  _main_thread_state = nullptr;
}

/*
 * Gets the Python_context from the Python interpreter.
 */
Python_context *Python_context::get() {
  PyObject *ctx;
  PyObject *module;
  PyObject *dict;

  module =
      PyDict_GetItemString(PyImport_GetModuleDict(), "shell_python_support");
  if (!module)
    throw std::runtime_error("SHELL module not found in Python runtime");

  dict = PyModule_GetDict(module);
  if (!dict)
    throw std::runtime_error("SHELL module is invalid in Python runtime");

  ctx = PyDict_GetItemString(dict, "__SHELL__");
  if (!ctx)
    throw std::runtime_error("SHELL context not found in Python runtime");

#ifdef IS_PY3K
  return static_cast<Python_context *>(PyCapsule_GetPointer(ctx, nullptr));
#else
  return static_cast<Python_context *>(PyCObject_AsVoidPtr(ctx));
#endif
}

Python_context *Python_context::get_and_check() {
  try {
    return Python_context::get();
  } catch (const std::exception &exc) {
    Python_context::set_python_error(
        PyExc_SystemError,
        std::string("Could not get SHELL context: ") + exc.what());
  }
  return NULL;
}

PyObject *Python_context::get_shell_stderr_module() {
  return _shell_stderr_module;
}

PyObject *Python_context::get_shell_stdout_module() {
  return _shell_stdout_module;
}

PyObject *Python_context::get_shell_stdin_module() {
  return _shell_stdin_module;
}

PyObject *Python_context::get_shell_python_support_module() {
  return _shell_python_support_module;
}

void Python_context::set_argv(const std::vector<std::string> &argv) {
  if (!argv.empty()) {
    const auto &input = to_py_char_t(argv);
    std::vector<const PY_CHAR_T *> argvv;

    for (const auto &s : input) argvv.push_back(s.c_str());

    argvv.push_back(nullptr);

    PySys_SetArgv(argv.size(), const_cast<PY_CHAR_T **>(argvv.data()));
  }
}

Value Python_context::execute(const std::string &code,
                              const std::string &UNUSED(source),
                              const std::vector<std::string> &argv) {
  PyObject *py_result;
  Value retvalue;
  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);

  set_argv(argv);

  py_result = PyRun_StringFlags(code.c_str(), Py_file_input, _globals, _locals,
                                &m_compiler_flags);

  if (!py_result) {
    PyErr_Print();
    return Value();
  }

  Value tmp(_types.pyobj_to_shcore_value(py_result));
  Py_XDECREF(py_result);
  return tmp;
}

Value Python_context::execute_interactive(const std::string &code,
                                          Input_state &r_state) noexcept {
  Value retvalue;
  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);
  r_state = shcore::Input_state::Ok;

  /*
   PyRun_String() works as follows:
   If the 2nd param is Py_single_input, the function will take any code as input
   and execute it. But the return value will always be None.

   If it is Py_eval_input, then the function will evaluate the code as an
   expression and return the result. But that will produce a syntax error if the
   code is NOT an expression.

   What we want is to be able to execute any arbitrary code and get it to return
   the expression result if it's an expression or None (or whatever) if it's
   not. That doesn't seem to be possible.

   OTOH sys.displayhook is called whenever the interpreter is outputting
   interactive evaluation results, with the original object (not string). So we
   workaround by installing a temporary displayhook to capture the evaluation
   results...

   Continued Lines:
   ----------------

   Because of how the Python interpreter works, it's not possible to
   use the same multiline handling as the official interpreter. We emulate it
   by catching EOF parse errors and accumulate lines until it succeeds.
   */
  PyObject *orig_hook = PySys_GetObject(const_cast<char *>("displayhook"));
  Py_INCREF(orig_hook);

  PySys_SetObject(
      const_cast<char *>("displayhook"),
      PyDict_GetItemString(PyModule_GetDict(_shell_python_support_module),
                           const_cast<char *>("interactivehook")));

  PyObject *py_result = nullptr;
  try {
    py_result = PyRun_StringFlags(code.c_str(), Py_single_input, _globals,
                                  _locals, &m_compiler_flags);
  } catch (const std::exception &e) {
    set_python_error(PyExc_RuntimeError, e.what());

    // C++ exceptions should be caught before they cross to the Python runtime.
    // Assert let us to catch these cases during development.
    assert(0);
  }

  PySys_SetObject(const_cast<char *>("displayhook"), orig_hook);
  Py_DECREF(orig_hook);
  if (!py_result) {
    PyObject *exc, *value, *tb;
    // check whether this is a SyntaxError: unexpected EOF while parsing
    // that would indicate that the command is not complete and needs more
    // input til it's completed
    PyErr_Fetch(&exc, &value, &tb);
    if (PyErr_GivenExceptionMatches(exc, PyExc_SyntaxError)) {
      const char *msg;
      PyObject *obj;
      if (PyArg_ParseTuple(value, "sO", &msg, &obj)) {
        if (strncmp(msg,
                    "unexpected character after line continuation character",
                    strlen("unexpected character after line continuation "
                           "character")) == 0) {
          // NOTE: These two characters will come if explicit line
          // continuation is specified
          if (code.length() >= 2 && code[code.length() - 2] == '\\' &&
              code[code.length() - 1] == '\n')
            r_state = Input_state::ContinuedSingle;
        } else if (strncmp(msg,
                           "EOF while scanning triple-quoted string literal",
                           strlen("EOF while scanning triple-quoted string "
                                  "literal")) == 0) {
          r_state = Input_state::ContinuedSingle;
        } else if (strncmp(msg, "unexpected EOF while parsing",
                           strlen("unexpected EOF while parsing")) == 0) {
          r_state = Input_state::ContinuedBlock;
        }
      }
    }
    PyErr_Restore(exc, value, tb);
    if (r_state != Input_state::Ok)
      PyErr_Clear();
    else
      PyErr_Print();
    return Value();
    // If no error was found butthe line has the implicit line continuation
    // we need to indicate so
  } else if (code.length() >= 2 && code[code.length() - 2] == '\\' &&
             code[code.length() - 1] == '\n') {
    r_state = Input_state::ContinuedSingle;
  }

  if (m_captured_eval_result) {
    const auto tmp = _types.pyobj_to_shcore_value(m_captured_eval_result);
    m_captured_eval_result = nullptr;
    return tmp;
  }

  return Value::Null();
}

void Python_context::get_members_of(
    PyObject *object, std::vector<std::pair<bool, std::string>> *out_keys) {
  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);
  PyObject *members = PyObject_Dir(object);

  if (members && PySequence_Check(members)) {
    for (int i = 0, c = PySequence_Size(members); i < c; i++) {
      PyObject *m = PySequence_ITEM(members, i);
      std::string s;

      if (m && pystring_to_string(m, &s) && s[0] != '_') {
        PyObject *mem = PyObject_GetAttrString(object, s.c_str());
        out_keys->push_back(std::make_pair(PyCallable_Check(mem), s));
        Py_XDECREF(mem);
      }
      Py_XDECREF(m);
    }
  }
  Py_XDECREF(members);
}

std::vector<std::pair<bool, std::string>> Python_context::list_globals() {
  std::vector<std::pair<bool, std::string>> keys;
  WillEnterPython lock;
  Py_ssize_t pos = 0;
  PyObject *key;
  PyObject *obj;

  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);

  while (PyDict_Next(_globals, &pos, &key, &obj)) {
    std::string key_string;
    pystring_to_string(key, &key_string);
    keys.push_back(std::make_pair(PyCallable_Check(obj), key_string));
  }

  // get __builtins__
  PyObject *builtins = PyDict_GetItemString(_globals, "__builtins__");
  if (builtins) {
    get_members_of(builtins, &keys);
  }
  return keys;
}

Value Python_context::get_global(const std::string &value) {
  PyObject *py_result;

  py_result = PyDict_GetItemString(_globals, value.c_str());

  return _types.pyobj_to_shcore_value(py_result);
}

PyObject *Python_context::get_global_py(const std::string &value) {
  PyObject *tmp = PyDict_GetItemString(_globals, value.c_str());
  Py_XINCREF(tmp);
  return tmp;
}

void Python_context::set_global(const std::string &name, const Value &value) {
  WillEnterPython lock;
  PyObject *p = _types.shcore_value_to_pyobj(value);
  assert(p != nullptr);
  // incs ref
  PyDict_SetItemString(_globals, name.c_str(), p);
  Py_XDECREF(p);
}

void Python_context::set_global_item(const std::string &global_name,
                                     const std::string &item_name,
                                     const Value &value) {
  PyObject *py_global;

  py_global = PyDict_GetItemString(_globals, global_name.c_str());
  if (!py_global)
    throw shcore::Exception::logic_error(
        "Python_context: Error setting global item on " + global_name + ".");
  else  // steals the ref
    PyModule_AddObject(py_global, item_name.c_str(),
                       _types.shcore_value_to_pyobj(value));
}

Value Python_context::pyobj_to_shcore_value(PyObject *value) {
  return _types.pyobj_to_shcore_value(value);
}

PyObject *Python_context::shcore_value_to_pyobj(const Value &value) {
  return _types.shcore_value_to_pyobj(value);
}

void Python_context::set_python_error(const std::exception &exc,
                                      const std::string &location) {
  PyErr_SetString(
      PyExc_SystemError,
      (location.empty() ? exc.what() : location + ": " + exc.what()).c_str());
}

void Python_context::set_python_error(const shcore::Exception &exc,
                                      const std::string &location) {
  std::string error_message;

  std::string type = exc.error()->get_string("type", "");
  std::string message = exc.error()->get_string("message", "");
  int64_t code = exc.error()->get_int("code", -1);
  std::string error_location = exc.error()->get_string("location", "");

  if (exc.is_mysql()) {
    PyObject *args = PyTuple_New(2);
    if (args) {
      PyTuple_SET_ITEM(args, 0, PyInt_FromLong(code));
      PyTuple_SET_ITEM(args, 1, PyString_FromString(message.c_str()));
      PyErr_SetObject(get()->_db_error, args);
      Py_DECREF(args);
      return;
    }
  }

  if (error_location.empty()) error_location = location;

  if (!message.empty()) {
    if (!type.empty()) error_message += type;

    if (code != -1)
      error_message += shcore::str_format(" (%lld)", (long long int)code);

    if (!error_message.empty()) error_message += ": ";

    error_message += message;

    if (!error_location.empty()) error_message += " at " + error_location;

    error_message += "\n";
  }

  PyErr_SetString(
      PyExc_SystemError,
      (error_location.empty() ? error_message
                              : error_location + ": " + error_message)
          .c_str());
}

void Python_context::set_python_error(PyObject *obj,
                                      const std::string &location) {
  PyErr_SetString(obj, location.c_str());
}

bool Python_context::pystring_to_string(PyObject *strobject,
                                        std::string *result, bool convert) {
  if (PyUnicode_Check(strobject)) {
    PyObject *ref = PyUnicode_AsUTF8String(strobject);

    if (ref) {
      bool ret = pystring_to_string(ref, result, false);
      Py_DECREF(ref);
      return ret;
    }

    return false;
  }

#ifdef IS_PY3K
  if (PyBytes_Check(strobject)) {
#else   // !IS_PY3K
  if (PyString_Check(strobject)) {
#endif  // !IS_PY3K
    char *s;
    Py_ssize_t len;
#ifdef IS_PY3K
    PyBytes_AsStringAndSize(strobject, &s, &len);
#else   // !IS_PY3K
    PyString_AsStringAndSize(strobject, &s, &len);
#endif  // !IS_PY3K
    if (s)
      *result = std::string(s, len);
    else
      *result = "";
    return true;
  }

  // If strobject is not a string, we can choose if we want to convert it to
  // string with PyObject_str() or return an error
  if (convert) {
    PyObject *str = PyObject_Str(strobject);

    if (str) {
      bool ret = pystring_to_string(str, result, false);
      Py_DECREF(str);
      return ret;
    }
  }

  return false;
}

AutoPyObject Python_context::get_shell_list_class() {
  return _shell_list_class;
}

AutoPyObject Python_context::get_shell_dict_class() {
  return _shell_dict_class;
}

AutoPyObject Python_context::get_shell_object_class() {
  return _shell_object_class;
}

AutoPyObject Python_context::get_shell_indexed_object_class() {
  return _shell_indexed_object_class;
}

AutoPyObject Python_context::get_shell_function_class() {
  return _shell_function_class;
}

PyObject *Python_context::shell_print(PyObject *UNUSED(self), PyObject *args,
                                      const std::string &stream) {
  Python_context *ctx;
  std::string text;

  if (!(ctx = Python_context::get_and_check())) return NULL;

  if (stream != "error.flush") {
    PyObject *o;
    if (!PyArg_ParseTuple(args, "O", &o)) {
      if (PyTuple_Size(args) == 1 && PyTuple_GetItem(args, 0) == Py_None) {
        PyErr_Clear();
        text = "None";
      } else {
        return NULL;
      }
    } else if (!pystring_to_string(o, &text, true)) {
      return NULL;
    }
  }

  if (stream == "error")
    mysqlsh::current_console()->print_diag(text);
  else
    mysqlsh::current_console()->print(text);

  Py_INCREF(Py_None);
  return Py_None;
}

PyObject *Python_context::shell_raw_input(PyObject *, PyObject *args) {
  const char *prompt = nullptr;
  if (!PyArg_ParseTuple(args, "|s", &prompt)) return nullptr;

  Python_context *ctx;
  if (!(ctx = Python_context::get_and_check())) return nullptr;

  shcore::Prompt_result result;
  std::string line;
  std::tie(result, line) = ctx->read_line(prompt ? prompt : "");
  if (result == shcore::Prompt_result::Cancel) {
    PyErr_SetNone(PyExc_KeyboardInterrupt);
    return nullptr;
  } else if (result == shcore::Prompt_result::CTRL_D) {
    // ^D - normally this would close the fd, but that isn't suitable here
    PyErr_SetNone(PyExc_EOFError);
    return nullptr;
  }
  if (!line.empty()) line = line.substr(0, line.size() - 1);  // strip \n
  return PyString_FromString(line.c_str());
}

std::pair<shcore::Prompt_result, std::string> Python_context::read_line(
    const std::string &prompt) {
  // Check if there's some leftover in the buffer
  if (!_stdin_buffer.empty()) {
    size_t n = _stdin_buffer.find('\n');
    if (n != std::string::npos) {
      std::string tmp = _stdin_buffer.substr(0, n + 1);
      _stdin_buffer = _stdin_buffer.substr(n + 1);
      return {shcore::Prompt_result::Ok, tmp};
    }
  }
  std::string ret;
  if (!mysqlsh::current_console()->prompt(prompt, &ret)) {
    return {shcore::Prompt_result::Cancel, ""};
  }
  _stdin_buffer.append(ret).append("\n");
  std::string tmp;
  std::swap(tmp, _stdin_buffer);
  return {shcore::Prompt_result::Ok, tmp};
}

PyObject *Python_context::shell_stdout(PyObject *self, PyObject *args) {
  return shell_print(self, args, "out");
}

PyObject *Python_context::shell_stderr(PyObject *self, PyObject *args) {
  return shell_print(self, args, "error");
}

PyObject *Python_context::shell_flush_stderr(PyObject *, PyObject *) {
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject *Python_context::shell_flush(PyObject *, PyObject *) {
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject *Python_context::shell_stdin_read(PyObject *, PyObject *args) {
  Python_context *ctx;

  if (!(ctx = Python_context::get_and_check())) return nullptr;

  // stdin.read(n) requires n number of chars to be read,
  // but we can only read one line at a time. Thus, we read one line at a time
  // from user, buffer it and return the requested number of chars
  int n = 0;
  if (!PyArg_ParseTuple(args, "I", &n)) return nullptr;

  std::string read_buffer;
  for (;;) {
    shcore::Prompt_result result;
    std::string line;
    std::tie(result, line) = ctx->read_line("");
    if (result == shcore::Prompt_result::Cancel) {
      PyErr_SetNone(PyExc_KeyboardInterrupt);
      return nullptr;
    } else if (result == shcore::Prompt_result::CTRL_D) {
      // ^D - normally this would close the fd, but that isn't suitable here
      PyErr_SetNone(PyExc_EOFError);
      return nullptr;
    }

    read_buffer += line;

    if (static_cast<int>(read_buffer.size()) >= n) {
      PyObject *str = PyString_FromString(read_buffer.substr(0, n).c_str());

      // Prepend to the buffer what was not consumed
      if (static_cast<int>(read_buffer.size()) > n)
        ctx->_stdin_buffer = read_buffer.substr(n) + ctx->_stdin_buffer;

      return str;
    }
  }
}

PyObject *Python_context::shell_stdin_readline(PyObject *, PyObject *) {
  Python_context *ctx;

  if (!(ctx = Python_context::get_and_check())) return nullptr;

  shcore::Prompt_result result;
  std::string line;
  std::tie(result, line) = ctx->read_line("");
  if (result == shcore::Prompt_result::Cancel) {
    PyErr_SetNone(PyExc_KeyboardInterrupt);
    return nullptr;
  } else if (result == shcore::Prompt_result::CTRL_D) {
    // ^D - normally this would close the fd, but that isn't suitable here
    PyErr_SetNone(PyExc_EOFError);
    return nullptr;
  }
  return PyString_FromString(line.c_str());
}

PyObject *Python_context::shell_interactive_eval_hook(PyObject *,
                                                      PyObject *args) {
  Python_context *ctx;
  std::string text;

  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);

  if (!(ctx = Python_context::get_and_check())) return NULL;

  if (PyTuple_Size(args) == 1) {
    ctx->m_captured_eval_result = PyTuple_GetItem(args, 0);
  } else {
    char buff[100];
    snprintf(buff, sizeof(buff),
             "interactivehook() takes exactly 1 argument (%i given)",
             static_cast<int>(PyTuple_Size(args)));
    PyErr_SetString(PyExc_TypeError, buff);
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

PyMethodDef Python_context::ShellStdErrMethods[] = {
    {"write", &Python_context::shell_stderr, METH_VARARGS,
     "Write an error string in the SHELL shell."},
    {"flush", &Python_context::shell_flush_stderr, METH_NOARGS, ""},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

PyMethodDef Python_context::ShellStdOutMethods[] = {
    {"write", &Python_context::shell_stdout, METH_VARARGS,
     "Write a string in the SHELL shell."},
    {"flush", &Python_context::shell_flush, METH_NOARGS, ""},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

PyMethodDef Python_context::ShellStdInMethods[] = {
    {"read", &Python_context::shell_stdin_read, METH_VARARGS,
     "Read a string through the shell input mechanism."},
    {"readline", &Python_context::shell_stdin_readline, METH_NOARGS,
     "Read a line of string through the shell input mechanism."},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

PyMethodDef Python_context::ShellPythonSupportMethods[] = {
    {"interactivehook", &Python_context::shell_interactive_eval_hook,
     METH_VARARGS,
     "Custom displayhook to capture interactive expr evaluation results."},
    {"raw_input", &Python_context::shell_raw_input, METH_VARARGS,
     "raw_input([prompt]) -> string\n"
     "\n"
     "Read a string from standard input.  The trailing newline is stripped.\n"
     "If the user hits EOF (Unix: Ctl-D, Windows: Ctl-Z+Return), raise "
     "EOFError.\n"},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

void Python_context::register_mysqlsh_module() {
  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);

  // Registers the mysqlsh module/package, at least for now this exists
  // only on the python side of things, this module encloses the inner
  // modules: mysql and mysqlx to prevent class names i.e. using connector/py
  _shell_mysqlsh_module = py_register_module("mysqlsh", nullptr);

  if (_shell_mysqlsh_module == NULL)
    throw std::runtime_error(
        "Error initializing the 'mysqlsh' module in Python support");

  PyObject *py_mysqlsh_dict = PyModule_GetDict(_shell_mysqlsh_module);

  // Now registers each available module as part of the mysqlsh package
  auto modules = Object_factory::package_contents("__modules__");

  shcore::Scoped_naming_style lower(NamingStyle::LowerCaseUnderscores);

  for (auto name : modules) {
    auto module_obj = Object_factory::call_constructor("__modules__", name,
                                                       shcore::Argument_list());

    std::shared_ptr<shcore::Module_base> module =
        std::dynamic_pointer_cast<shcore::Module_base>(module_obj);

    // The modules will be registered in python as __<name>__
    // But exposed in the mysqlsh module as the original name
    std::string module_name = "__" + name + "__";

    PyObject *py_module_ref = py_register_module(module_name, nullptr);

    if (py_module_ref == NULL)
      throw std::runtime_error("Error initializing the '" + name +
                               "' module in Python support");

    _modules[py_module_ref] = module;

    // Now inserts every element on the module
    PyObject *py_dict = PyModule_GetDict(py_module_ref);

    for (auto &name : module->get_members()) {
      shcore::Value member = module->get_member_advanced(name);
      if (member.type == shcore::Function || member.type == shcore::Object) {
        PyObject *p = _types.shcore_value_to_pyobj(member);
        // incrs ref
        PyDict_SetItemString(py_dict, name.c_str(), p);
        Py_XDECREF(p);
      }
    }

    // Now makes the module available through the mysqlsh module, using the
    // original name
    PyDict_SetItemString(py_mysqlsh_dict, name.c_str(), py_module_ref);
  }

  PyObject *r = Py_CompileString(
      "class DBError(Exception):\n"
      "  def __init__(self, code, msg, sqlstate=None):\n"
      "    self.code = code\n"
      "    self.msg = msg\n"
      "    self.args = (code, msg)\n"
      "\n"
      "  def __str__(self):\n"
      "    return 'MySQL Error (%s): %s' % (self.code, self.msg)\n"
      "\n",
      "", Py_file_input);

  if (!r) {
    PyErr_Print();
  } else {
    PyImport_ExecCodeModule(const_cast<char *>("mysqlsh"), r);
    Py_DECREF(r);
  }
  _db_error = PyDict_GetItemString(py_mysqlsh_dict, "DBError");
  if (!_db_error) PyErr_Print();

  // Finally inserts the globals
  PyDict_SetItemString(py_mysqlsh_dict, "globals", _global_namespace);
}

void Python_context::register_shell_stderr_module() {
  PyObject *module = py_register_module("shell_stderr", ShellStdErrMethods);
  if (module == NULL)
    throw std::runtime_error(
        "Error initializing SHELL module in Python support");

  _shell_stderr_module = module;
}

void Python_context::register_shell_stdout_module() {
  PyObject *module = py_register_module("shell_stdout", ShellStdOutMethods);
  if (module == NULL)
    throw std::runtime_error(
        "Error initializing SHELL module in Python support");

  _shell_stdout_module = module;
}

void Python_context::register_shell_stdin_module() {
  PyObject *module = py_register_module("shell_stdin", ShellStdInMethods);
  if (module == NULL)
    throw std::runtime_error(
        "Error initializing SHELL module in Python support");

  _shell_stdin_module = module;
}

void Python_context::register_shell_python_support_module() {
  PyObject *module =
      py_register_module("shell_python_support", ShellPythonSupportMethods);
  if (module == NULL)
    throw std::runtime_error(
        "Error initializing SHELL module in Python support");

  _shell_python_support_module = module;

  // add the context ptr
  PyObject *context_object;
#ifdef IS_PY3K
  context_object = PyCapsule_New(this, nullptr, nullptr);
  PyCapsule_SetContext(context_object, &SHELLTypeSignature);
#else
  context_object =
      PyCObject_FromVoidPtrAndDesc(this, &SHELLTypeSignature, NULL);
#endif

  if (context_object != NULL)
    PyModule_AddObject(module, "__SHELL__", context_object);

  PyModule_AddStringConstant(module, "INT", shcore::type_name(Integer).c_str());
  PyModule_AddStringConstant(module, "DOUBLE",
                             shcore::type_name(Float).c_str());
  PyModule_AddStringConstant(module, "STRING",
                             shcore::type_name(String).c_str());
  PyModule_AddStringConstant(module, "LIST", shcore::type_name(Array).c_str());
  PyModule_AddStringConstant(module, "DICT", shcore::type_name(Map).c_str());
  PyModule_AddStringConstant(module, "OBJECT",
                             shcore::type_name(Object).c_str());
  PyModule_AddStringConstant(module, "FUNCTION",
                             shcore::type_name(Function).c_str());

  init_shell_dict_type();
  init_shell_list_type();
  init_shell_object_type();
  init_shell_function_type();
}

bool Python_context::is_module(const std::string &file_name) {
  bool ret_val = false;

  PyObject *argv0 = PyString_FromString(file_name.c_str());
  PyObject *importer = PyImport_GetImporter(argv0);

  if (importer && (importer != Py_None)) {
#ifdef IS_PY3K
    // Importers are returned for any existing directory, even if it's not
    // a module. Call method on an importer to double check if it really is
    // module.
    static constexpr auto s_find_spec = "find_spec";
    PyObject *find_spec = nullptr;

    if (PyObject_HasAttrString(importer, s_find_spec)) {
      find_spec = PyObject_GetAttrString(importer, s_find_spec);
    } else {
      // old style importers don't have find_spec() method
      find_spec = PyObject_GetAttrString(importer, "find_module");
    }

    // use the same name as in execute_module() method (__main__)
    PyObject *runargs = Py_BuildValue("(s)", "__main__");
    PyObject *result = PyObject_Call(find_spec, runargs, nullptr);

    ret_val = result && (result != Py_None);

    Py_XDECREF(result);
    Py_XDECREF(runargs);
    Py_XDECREF(find_spec);

#else   // !IS_PY3K
    ret_val = Py_TYPE(importer) != &PyNullImporter_Type;
#endif  // !IS_PY3K
  }

  Py_XDECREF(argv0);
  Py_XDECREF(importer);

  if (PyErr_Occurred()) {
    PyErr_Print();
  }

  return ret_val;
}

Value Python_context::execute_module(const std::string &file_name,
                                     const std::vector<std::string> &argv) {
  shcore::Value ret_val;
  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);

  PyObject *argv0 = PyString_FromString(file_name.c_str());
  PyObject *sys_path = PySys_GetObject(const_cast<char *>("path"));

  set_argv(argv);

  // Register the module name as an input source
  if (sys_path) {
    if (!PyList_Insert(sys_path, 0, argv0)) {
      Py_INCREF(argv0);
      sys_path = NULL;

      // Now executes the __main__ module
      PyObject *runpy, *runmodule, *runargs, *py_result;
      runpy = PyImport_ImportModule("runpy");
      if (runpy == NULL) {
        PyErr_Print();
        throw shcore::Exception::runtime_error("Could not import runpy module");
      }

      runmodule = PyObject_GetAttrString(runpy, "_run_module_as_main");
      if (runmodule == NULL) {
        Py_DECREF(runpy);
        throw shcore::Exception::runtime_error(
            "Could not access runpy._run_module_as_main");
      }

      runargs = Py_BuildValue("(si)", "__main__", 0);
      if (runargs == NULL) {
        Py_DECREF(runpy);
        Py_DECREF(runmodule);
        throw shcore::Exception::runtime_error(
            "Could not create arguments for runpy._run_module_as_main");
      }

      module_processing = true;
      py_result = PyObject_Call(runmodule, runargs, NULL);
      module_processing = false;

      if (!py_result) {
        if (PyErr_ExceptionMatches(PyExc_SystemExit)) exit_error = true;

        PyErr_Print();
      }

      Py_DECREF(runpy);
      Py_DECREF(runmodule);
      Py_DECREF(runargs);
      Py_XDECREF(argv0);

      if (py_result) {
        ret_val = _types.pyobj_to_shcore_value(py_result);
        Py_XDECREF(py_result);
      }
    } else {
      throw shcore::Exception::runtime_error(
          "Unable register the module on the system path");
    }
  } else {
    throw shcore::Exception::runtime_error(
        "Unable to retrieve the system path to register the module");
  }

  return ret_val;
}

bool Python_context::load_plugin(const Plugin_definition &plugin) {
  WillEnterPython lock;
  bool ret_val = false;
  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);

  PyObject *plugin_file = nullptr;
  PyObject *plugin_path = nullptr;
  PyObject *result = nullptr;
  PyObject *globals = nullptr;
  PyObject *sys_path_backup = nullptr;
  FILE *file = nullptr;
  bool executed = false;
  std::string file_name = plugin.file;

  // The path to the plugins folder needs to be in sys.path while the plugin is
  // being loaded
  std::string plugin_dir = shcore::path::dirname(file_name);
  if (!plugin.main) plugin_dir = shcore::path::dirname(plugin_dir);
  plugin_dir = shcore::path::dirname(plugin_dir);

  try {
    file = fopen(file_name.c_str(), "r");
    if (nullptr == file) {
      throw std::runtime_error(shcore::str_format(
          "Failed opening the file: %s", errno_to_string(errno).c_str()));
    }

    plugin_file = PyString_FromString(file_name.c_str());
    if (nullptr == plugin_file) {
      throw std::runtime_error("Unable to get plugin file");
    }

    plugin_path = PyString_FromString(plugin_dir.c_str());
    if (nullptr == plugin_path) {
      throw std::runtime_error("Unable to get plugin path");
    }

    PyObject *sys_path = PySys_GetObject(const_cast<char *>("path"));
    if (nullptr == sys_path) {
      throw std::runtime_error("Failed getting sys.path");
    }

    // copy globals, so executed scripts does not pollute global
    // namespace
    globals = PyDict_Copy(_globals);
    if (PyDict_SetItemString(globals, "__file__", plugin_file)) {
      throw std::runtime_error("Failed setting __file__");
    }

    // Backups the original sys.path, in case the plugin load fails sys.path
    // will be set back to the original content before the plugin load
    sys_path_backup = PyList_GetSlice(sys_path, 0, PyList_Size(sys_path));
    if (nullptr == sys_path_backup) {
      std::string error = fetch_and_clear_exception();

      throw std::runtime_error(
          shcore::str_format("Failed backing up sys.path: %s", error.c_str()));
    }

    if (PyList_Insert(sys_path, 0, plugin_path)) {
      throw std::runtime_error("Failed adding plugin path to sys.path");
    }

    // PyRun_FileEx() will close the file
    // plugins are loaded in sandboxed environment, we're not using
    // the compiler flags here
    result = PyRun_FileEx(file, shcore::path::basename(file_name).c_str(),
                          Py_file_input, globals, nullptr, true);

    executed = true;

    // In case of an error, gets the description and clears the error
    // condition in python to be able to restore the sys.path
    std::string error;
    if (nullptr == result) {
      error = fetch_and_clear_exception();
    }

    // The plugin paths are required on sys.path at the moment the plugin is
    // loaded, after that they are not required so the original sys.path is
    // restored
    PySys_SetObject(const_cast<char *>("path"), sys_path_backup);

    // If there was an error, raises the corresponding exception
    if (nullptr == result) {
      throw std::runtime_error(
          shcore::str_format("Execution failed: %s", error.c_str()));
    } else {
      ret_val = true;
    }
  } catch (const std::runtime_error &error) {
    log_error("Error loading Python file '%s':\n\t%s", file_name.c_str(),
              error.what());

    if (file && !executed) {
      fclose(file);
    }
  }
  Py_XDECREF(plugin_file);
  Py_XDECREF(result);
  Py_XDECREF(globals);
  Py_XDECREF(plugin_path);
  Py_XDECREF(sys_path_backup);

  return ret_val;
}  // namespace shcore

std::string Python_context::fetch_and_clear_exception() {
  std::string exception;
  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);

  if (nullptr != PyErr_Occurred()) {
    PyObject *exc = nullptr;
    PyObject *value = nullptr;
    PyObject *tb = nullptr;

    PyErr_Fetch(&exc, &value, &tb);
    PyErr_NormalizeException(&exc, &value, &tb);

    {
      PyObject *str = PyObject_Str(value);
      pystring_to_string(str, &exception);
      Py_XDECREF(str);
    }

    if (nullptr != tb) {
      PyObject *traceback_module = PyImport_ImportModule("traceback");

      if (nullptr != traceback_module) {
        PyObject *format_exception =
            PyObject_GetAttrString(traceback_module, "format_exception");

        if (nullptr != format_exception && PyCallable_Check(format_exception)) {
          PyObject *backtrace = PyObject_CallFunctionObjArgs(
              format_exception, exc, value, tb, nullptr);

          if (nullptr != backtrace && PyList_Check(backtrace)) {
            exception = "\n";

            for (Py_ssize_t c = PyList_Size(backtrace), i = 0; i < c; ++i) {
              std::string item;
              pystring_to_string(PyList_GetItem(backtrace, i), &item);
              exception += item;
            }
          }

          Py_XDECREF(backtrace);
        }

        Py_XDECREF(format_exception);
      }

      Py_XDECREF(traceback_module);
    }

    Py_XDECREF(exc);
    Py_XDECREF(value);
    Py_XDECREF(tb);
  }

  return exception;
}

std::weak_ptr<AutoPyObject> Python_context::store(PyObject *object) {
  m_stored_objects.emplace_back(std::make_shared<AutoPyObject>(object));
  return m_stored_objects.back();
}

void Python_context::erase(const std::shared_ptr<AutoPyObject> &object) {
  m_stored_objects.remove(object);
}

}  // namespace shcore
