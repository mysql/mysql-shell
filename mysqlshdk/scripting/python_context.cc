/*
 * Copyright (c) 2015, 2025, Oracle and/or its affiliates.
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
#include "scripting/python_context.h"

#include <cwchar>

#include <signal.h>
#include <exception>
#include <type_traits>

#include "mysqlshdk/include/scripting/python_object_wrapper.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
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
#else
#include <unistd.h>
#endif

// used to identify a proper SHELL context object as a PyCObject
static const char *SHELLTypeSignature = "SHELLCONTEXT";

extern const char *g_mysqlsh_path;

namespace shcore {

namespace {

void check_status(PyStatus status, const char *context) {
  assert(context);

  if (PyStatus_Exception(status)) {
    mysqlsh::current_console()->print_error(
        "Unable to configure Python, failed to " + std::string{context});
    Py_ExitStatusException(status);
  }
}

std::wstring python_home() {
  std::string home;
#ifdef _WIN32
#define MAJOR_MINOR STRINGIFY(PY_MAJOR_VERSION) "." STRINGIFY(PY_MINOR_VERSION)
  {
    // If not will associate what should be the right path in a standard
    // distribution
    std::string python_path = shcore::get_mysqlx_home_path();
    assert(!python_path.empty());

    python_path =
        shcore::path::join_path(python_path, "lib", "Python" MAJOR_MINOR);

    log_info("Setting Python home to '%s'", python_path.c_str());
    home = python_path;
  }
#undef MAJOR_MINOR
#else  // !_WIN32
#if HAVE_PYTHON == 2
  {
    // Set path to the bundled python version.
    std::string python_path = shcore::path::join_path(
        shcore::get_mysqlx_home_path(), "lib", "mysqlsh");
    if (shcore::is_folder(python_path)) {
      // Override the system Python install with the bundled one
      log_info("Setting Python home to '%s'", python_path.c_str());
      home = python_path;
    }
  }
#else   // HAVE_PYTHON != 2
  const auto env_value = getenv("PYTHONHOME");

  // If PYTHONHOME is available, honors it
  if (env_value) {
    log_info("Setting Python home to '%s' from PYTHONHOME", env_value);
    home = env_value;
  }
#endif  // HAVE_PYTHON != 2
#endif  // !_WIN32
  return utf8_to_wide(home);
}

std::wstring python_path(const std::wstring &home) {
#ifdef _WIN32
  return utf8_to_wide(shcore::path::join_path(
      wide_to_utf8(home), shcore::path::basename(PYTHON_EXECUTABLE)));
#else
  return utf8_to_wide(shcore::path::join_path(
      wide_to_utf8(home), "bin", shcore::path::basename(PYTHON_EXECUTABLE)));
#endif
}

void pre_initialize_python() {
  PyPreConfig pre_config;
  PyPreConfig_InitPythonConfig(&pre_config);

#ifdef _WIN32
  // Do not load user preferred locale on Windows (it is not UTF-8)
  pre_config.configure_locale = 0;
  pre_config.coerce_c_locale = 0;
  pre_config.coerce_c_locale_warn = 0;
#endif  // _WIN32

  check_status(Py_PreInitialize(&pre_config), "pre-initialize");
}

auto remove_int_signal() {
#ifdef _WIN32
  // on Windows there's no sigaction() and no siginterrupt()
  return signal(SIGINT, SIG_DFL);
#else   // !_WIN32
  // we need to use sigaction() here to make sure sa_flags are properly
  // restored, signal() would explicitly set them to 0 or SA_RESTART, depending
  // on whether siginterrupt() was called
  struct sigaction default_action;
  memset(&default_action, 0, sizeof(default_action));
  default_action.sa_handler = SIG_DFL;
  sigemptyset(&default_action.sa_mask);
  default_action.sa_flags = 0;

  struct sigaction prev_action;
  memset(&prev_action, 0, sizeof(prev_action));

  sigaction(SIGINT, &default_action, &prev_action);

  return prev_action;
#endif  // !_WIN32
}

template <typename T>
void restore_int_signal(T prev) {
#ifdef _WIN32
  signal(SIGINT, prev);
#else   // !_WIN32
  sigaction(SIGINT, &prev, nullptr);
#endif  // !_WIN32
}

void initialize_python() {
  PyConfig config;
  PyConfig_InitPythonConfig(&config);
  shcore::on_leave_scope clear_config([&config]() { PyConfig_Clear(&config); });

  config.install_signal_handlers = 1;

  if (const auto home = python_home(); !home.empty()) {
#ifdef _WIN32
    // binary folder holds all bundled dlls, including the ones needed by Python
    SetDllDirectoryW(utf8_to_wide(shcore::get_binary_folder()).c_str());
#endif
    check_status(PyConfig_SetString(&config, &config.home, home.c_str()),
                 "set Python home");

    check_status(PyConfig_SetString(&config, &config.program_name,
                                    python_path(home).c_str()),
                 "set program name");
  } else {
    check_status(PyConfig_SetString(&config, &config.program_name,
                                    utf8_to_wide(PYTHON_EXECUTABLE).c_str()),
                 "set program name");
  }
  // we want config.argv to be set to an array holding an empty string, this
  // is done by default by Python

  check_status(Py_InitializeFromConfig(&config), "initialize");
}

py::Release py_register_module(const std::string &name, PyMethodDef *members) {
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
        delete[] definition->m_name;
        delete definition;
      }};

  {
    const auto length = name.length();
    auto copied_name = new char[length + 1];
    strncpy(copied_name, name.c_str(), length);
    copied_name[length] = '\0';
    moduledef->m_name = copied_name;
  }

  py::Release module{PyModule_Create(moduledef)};

  // create ModuleSpec to indicate it's a built-in module
  {
    py::Release machinery{PyImport_ImportModule("importlib.machinery")};

    if (machinery) {
      py::Release modulespec{
          PyObject_GetAttrString(machinery.get(), "ModuleSpec")};

      if (modulespec) {
        py::Release args{Py_BuildValue("(ss)", name.c_str(), nullptr)};
        py::Release kwargs{Py_BuildValue("{s:s}", "origin", "built-in")};

        py::Release spec{
            PyObject_Call(modulespec.get(), args.get(), kwargs.get())};
        PyDict_SetItemString(PyModule_GetDict(module.get()), "__spec__",
                             spec.get());
      }
    }
  }

  // module needs to be added to the sys.modules
  PyDict_SetItemString(PyImport_GetModuleDict(), moduledef->m_name,
                       module.get());
  return module;
}

py::Release py_run_string_interactive(const std::string &str, PyObject *globals,
                                      PyObject *locals,
                                      PyCompilerFlags *flags) {
  // Previously, we were using PyRun_StringFlags() with the Py_single_input
  // flag to run the code string and capture its output using a custom
  // sys.displayhook, however Python's grammar allows only for a single
  // statement (simple_stmt or compound_stmt) to be used in such case. Since
  // we want to be able to run multiple statements at once, Py_file_input flag
  // is the alternative which allows the code string to be a sequence of
  // statements, but unfortunately it does not allow to obtain the result of
  // the execution.
  //
  // Below, the code string is parsed as a series of statements using the
  // Py_file_input flag, producing the AST representation. We use this
  // representation to split the code into separate statements, which can be
  // safely executed by PyRun_StringFlags() and results can be captured using
  // the custom sys.displayhook.

  assert(flags);

  // set the flag, so that we get the AST representation
  flags->cf_flags |= PyCF_ONLY_AST;

  // compile the input string using Py_file_input, so that multiple statements
  // are allowed
  py::Release ast{
      Py_CompileStringFlags(str.c_str(), "<string>", Py_file_input, flags)};

  // remove the AST flag
  flags->cf_flags &= ~PyCF_ONLY_AST;

  if (!ast) return {};

  // if there was no syntax error, execute the statements one by one
  py::Release ret;

  try {
    py::Release body{PyObject_GetAttrString(ast.get(), "body")};

    if (!body) {
      throw std::runtime_error("AST is missing a body attribute");
    }

    if (!PyList_Check(body.get())) {
      throw std::runtime_error("AST.body is not a list");
    }

    auto code = str;

    const auto get_int = [](PyObject *obj, const char *name) {
      py::Release item{PyObject_GetAttrString(obj, name)};

      if (!item) {
        throw std::runtime_error("Object has no attribute called: " +
                                 std::string{name});
      }

      if (!PyLong_Check(item.get())) {
        throw std::runtime_error("Attribute is not an integer: " +
                                 std::string{name});
      }

      return PyLong_AsLong(item.get());
    };

    const auto size = PyList_GET_SIZE(body.get());

    for (auto i = decltype(size){0}; i < size; ++i) {
      const auto statement = PyList_GET_ITEM(body.get(), i);
      // subtract 1 to get zero-based line number
      const auto lineno = get_int(statement, "lineno") - 1;
      const auto end_lineno = get_int(statement, "end_lineno") - 1;
      // offset is zero-based
      const auto col_offset = get_int(statement, "col_offset");
      const auto end_col_offset = get_int(statement, "end_col_offset");

      // find the code block to be executed
      std::size_t begin = 0;

      // find the first line
      for (auto line = decltype(lineno){0}; line < lineno; ++line) {
        begin = code.find('\n', begin) + 1;
      }

      std::size_t end = begin;

      // find the last line
      for (auto line = decltype(lineno){0}; line < end_lineno - lineno;
           ++line) {
        end = code.find('\n', end) + 1;
      }

      // adjust for the column offset
      begin += col_offset;
      end += end_col_offset;

      // select the code block
      const auto code_char = code[end];
      code[end] = '\0';
      // get the pointer after string has been modified, some compilers
      // optimize string copy operations and use copy-on-write
      const auto code_ptr = code.c_str() + begin;

      // run the code block
      ret = py::Release{
          PyRun_StringFlags(code_ptr, Py_single_input, globals, locals, flags)};

      // stop the execution if there was an error
      if (!ret) break;

      code[end] = code_char;
    }
  } catch (const std::runtime_error &e) {
    log_error("Error processing Python code: %s, while executing:\n%s",
              e.what(), str.c_str());
    throw std::runtime_error(
        "Internal error processing Python code, see log for more details");
  }

  return ret;
}

}  // namespace

bool Python_context::exit_error = false;
bool Python_context::module_processing = false;

Python_init_singleton::~Python_init_singleton() {
  if (m_local_initialization) Py_Finalize();
}

void Python_init_singleton::init_python() {
  if (!s_instance) {
    // python-cryptography requires this definition to allow working with
    // OpenSSL versions lower than 1.1.0
    if (OPENSSL_VERSION_ID < 10100) {
      shcore::setenv("CRYPTOGRAPHY_ALLOW_OPENSSL_102", "1");
    }
    s_instance.reset(new Python_init_singleton());
  }
}

void Python_init_singleton::destroy_python() { s_instance.reset(); }

std::string Python_init_singleton::get_new_scope_name() {
  return shcore::str_format("__main__%u", ++s_cnt);
}

Python_init_singleton::Python_init_singleton() : m_local_initialization(false) {
  if (!Py_IsInitialized()) {
    pre_initialize_python();
    // Initialize the signals, so we can use PyErr_SetInterrupt().
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
    auto prev_signal = remove_int_signal();
    initialize_python();
    restore_int_signal(std::move(prev_signal));

    m_local_initialization = true;
  }
}

std::unique_ptr<Python_init_singleton> Python_init_singleton::s_instance;
unsigned int Python_init_singleton::s_cnt = 0;

Python_context::Python_context(bool redirect_stdio) {
  m_compiler_flags.cf_flags = 0;
  m_compiler_flags.cf_feature_version = PY_MINOR_VERSION;

  Python_init_singleton::init_python();

  PyObject *global_namespace = PyImport_AddModule("__main__");
  if (!global_namespace || !(_globals = PyModule_GetDict(global_namespace))) {
    throw Exception::runtime_error("Error initializing python context.");
  }

  _locals = _globals;

  // add our bundled packages to sys.path
  {
    PyObject *sys_path = PySys_GetObject("path");
    auto add_path = [sys_path](const std::string &path) {
      PyObject *tmp = PyString_FromString(path.c_str());
      PyList_Append(sys_path, tmp);
      Py_DECREF(tmp);
    };

    auto python_packages_path = shcore::path::join_path(
        shcore::get_library_folder(), "python-packages");

    add_path(python_packages_path);

    // also add any .zip modules
    for (const auto &path : shcore::listdir(python_packages_path)) {
      if (shcore::str_endswith(path, ".zip")) {
        add_path(shcore::path::join_path(python_packages_path, path));
      }
    }

#ifdef PYTHON_DEPS_PATH
    // When python is bundled with the shell, the 3rd party dependencies are
    // included on the site-packages folder and so automatically added to the
    // PYTHONPATH.
    // When python is not bundled, PYTHON_DEPS_PATH contains the folder where
    // they are included on the shell package, adding to PYTHONPATH has to be
    // done here.
    python_packages_path =
        shcore::path::join_path(shcore::get_library_folder(), PYTHON_DEPS_PATH);

    // we're using wheels for the python deps, so just the root path is needed
    add_path(python_packages_path);
#endif
  }

  // register shell module

  register_shell_stderr_module();
  register_shell_stdout_module();

  register_mysqlsh_module();
  register_shell_python_support_module();
  register_mysqlsh_builtins();
  get_datetime_constructor();

  PySys_SetObject(const_cast<char *>("real_stdout"),
                  PySys_GetObject(const_cast<char *>("stdout")));
  PySys_SetObject(const_cast<char *>("real_stderr"),
                  PySys_GetObject(const_cast<char *>("stderr")));
  PySys_SetObject(const_cast<char *>("real_stdin"),
                  PySys_GetObject(const_cast<char *>("stdin")));

  // make sys.stdout and sys.stderr send stuff to SHELL
  PySys_SetObject(const_cast<char *>("stdout"), _shell_stdout_module.get());
  PySys_SetObject(const_cast<char *>("stderr"), _shell_stderr_module.get());

  // set stdin to the shell console when on interactive mode
  if (redirect_stdio) {
    register_shell_stdin_module();
    PySys_SetObject(const_cast<char *>("stdin"), _shell_stdin_module.get());

    // also override raw_input() to use the prompt function
    PyObject *dict = PyModule_GetDict(_shell_python_support_module.get());
    assert(dict != nullptr);

    PyObject *raw_input = PyDict_GetItemString(dict, "raw_input");
    PyDict_SetItemString(_globals, "raw_input", raw_input);
    PyDict_SetItemString(
        PyModule_GetDict(PyDict_GetItemString(_globals, "__builtins__")),
        "raw_input", raw_input);
  }
  // Stores the main thread state
  _main_thread_state = PyThreadState_Get();

#if PY_VERSION_HEX < 0x03070000
  PyEval_InitThreads();
#endif
  PyEval_SaveThread();
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

    PySys_SetObject(const_cast<char *>("stdout"),
                    PySys_GetObject(const_cast<char *>("real_stdout")));
    PySys_SetObject(const_cast<char *>("stderr"),
                    PySys_GetObject(const_cast<char *>("real_stderr")));

    _shell_stderr_module.reset();
    _shell_stdout_module.reset();
    _shell_stdin_module.reset();
    _shell_python_support_module.reset();
    _datetime.reset();
    _datetime_type.reset();
    _date.reset();
    _date_type.reset();
    _time.reset();
    _time_type.reset();

    _mysqlsh_globals.reset();
    _mysqlsh_module.reset();

    PyGC_Collect();
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

  module = PyImport_AddModule("mysqlsh.__shell_python_support__");
  if (!module)
    throw std::runtime_error("SHELL module not found in Python runtime");

  dict = PyModule_GetDict(module);
  if (!dict)
    throw std::runtime_error("SHELL module is invalid in Python runtime");

  ctx = PyDict_GetItemString(dict, "__SHELL__");
  if (!ctx)
    throw std::runtime_error("SHELL context not found in Python runtime");

  return static_cast<Python_context *>(PyCapsule_GetPointer(ctx, nullptr));
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

void Python_context::set_argv(const std::vector<std::string> &argv) {
  if (argv.empty()) {
    return;
  }

  // convert arguments to wide char
  std::vector<std::wstring> wargv;
  std::vector<wchar_t *> pargv;

  wargv.reserve(argv.size());
  pargv.reserve(argv.size());

  for (const auto &s : argv) {
    wargv.emplace_back(utf8_to_wide(s));
    pargv.emplace_back(wargv.back().data());
  }

  WillEnterPython lock;

#if PY_VERSION_HEX >= 0x030b0000
  PyConfig config;
  PyConfig_InitPythonConfig(&config);
  shcore::on_leave_scope clear_config([&config]() { PyConfig_Clear(&config); });

  // don't parse argv looking for Python's command line arguments
  config.parse_argv = 0;
  check_status(PyConfig_SetArgv(&config, pargv.size(), pargv.data()),
               "set argv");
  check_status(Py_InitializeFromConfig(&config), "initialize argv");

  {
    // prepend parent directory of argv[0] to sys.path
    auto path = get_absolute_path(argv[0]);

#ifndef _WIN32
    {
      // resolve a potential symlink
      std::string link;
      link.resize(PATH_MAX);

      if (const auto len = ::readlink(path.c_str(), link.data(), link.length());
          len > 0) {
        link.resize(len);

        if (!path::is_absolute(link)) {
          link = get_absolute_path(link, path::dirname(path));
        }

        path = std::move(link);
      }
    }
#endif  // !_WIN32

    const auto parent = path::dirname(path);
    const auto sys_path = PySys_GetObject("path");
    const py::Release python_path{PyString_FromString(parent.c_str())};
    PyList_Insert(sys_path, 0, python_path.get());
  }
#else   // PY_VERSION_HEX < 0x030b0000
  // the above code also works for Python < 3.11, but for some reason the call
  // to Py_InitializeFromConfig() clears the sys.path
  PySys_SetArgv(pargv.size(), pargv.data());
#endif  // PY_VERSION_HEX < 0x030b0000
}

Value Python_context::execute(const std::string &code,
                              const std::string &source) {
  PyObject *py_result;
  Value retvalue;
  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);

  PyObject *m = PyImport_AddModule("__main__");
  if (!m) return Value();

  Py_INCREF(m);

  PyObject *d = PyModule_GetDict(m);

  if (!source.empty()) {
    if (!PyDict_GetItemString(d, "__file__")) {
      {
        PyObject *f = PyUnicode_DecodeFSDefault(source.c_str());
        if (f && PyDict_SetItemString(d, "__file__", f) < 0) {
          Py_DECREF(f);
          Py_DECREF(m);

          PyErr_Print();
          return Value();
        }
        Py_XDECREF(f);
      }
      if (PyDict_SetItemString(d, "__cached__", Py_None) < 0) {
        Py_DECREF(m);

        PyErr_Print();
        return Value();
      }
    }
  }

  py_result = PyRun_StringFlags(code.c_str(), Py_file_input, _globals, d,
                                &m_compiler_flags);

  Py_DECREF(m);

  if (!py_result) {
    PyErr_Print();
    return Value();
  }

  auto tmp = convert(py_result);
  Py_XDECREF(py_result);
  return tmp;
}

Value Python_context::execute_interactive(const std::string &code,
                                          Input_state &r_state,
                                          bool flush) noexcept {
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
  py::Store orig_hook{PySys_GetObject(const_cast<char *>("displayhook"))};

  PySys_SetObject(
      const_cast<char *>("displayhook"),
      PyDict_GetItemString(PyModule_GetDict(_shell_python_support_module.get()),
                           const_cast<char *>("interactivehook")));

  py::Release py_result;
  try {
    py_result =
        py_run_string_interactive(code, _globals, _locals, &m_compiler_flags);
  } catch (const std::exception &e) {
    set_python_error(PyExc_RuntimeError, e.what());

    // C++ exceptions should be caught before they cross to the Python runtime.
    // Assert let us to catch these cases during development.
    assert(0);
  }

  PySys_SetObject(const_cast<char *>("displayhook"), orig_hook.get());
  orig_hook.reset();

  if (!py_result) {
#if PY_VERSION_HEX >= 0x030c0000
    py::Release exc_value{PyErr_GetRaisedException()};
#else
    PyObject *type, *value, *tb;

    PyErr_Fetch(&type, &value, &tb);
    PyErr_NormalizeException(&type, &value, &tb);

    py::Release exc_type{type}, exc_value{value}, exc_tb{tb};
#endif

    // check whether this is a SyntaxError: unexpected EOF while parsing
    // that would indicate that the command is not complete and needs more
    // input til it's completed
    if (PyErr_GivenExceptionMatches(exc_value.get(), PyExc_SyntaxError) ||
        PyErr_GivenExceptionMatches(exc_value.get(), PyExc_IndentationError)) {
      // If there's no more input then the interpreter should not continue in
      // continued mode so any error should bubble up
      if (!flush) {
        const char *msg;
        PyObject *obj;
        const py::Release args{PyObject_GetAttrString(exc_value.get(), "args")};

        if (args && PyTuple_Check(args.get()) &&
            PyArg_ParseTuple(args.get(), "sO", &msg, &obj)) {
          using namespace std::literals;

          constexpr auto unexpected_character =
              "unexpected character after line continuation character"sv;
          constexpr auto eof_while_scanning =
              "EOF while scanning triple-quoted string literal"sv;
          // unterminated triple-quoted string literal, or
          // unterminated triple-quoted f-string literal
          constexpr auto unterminated_triple_quoted =
              "unterminated triple-quoted "sv;
          // "'%c' was never closed"
          constexpr auto was_never_closed = "was never closed"sv;
          constexpr auto unexpected_eof = "unexpected EOF while parsing"sv;
          constexpr auto expected_indented_block =
              "expected an indented block"sv;

          std::string_view msg_view{msg};
          const auto starts_with = [](std::string_view view,
                                      std::string_view prefix) {
            return view.substr(0, prefix.length()) == prefix;
          };
          const auto msg_starts_with = [&starts_with,
                                        msg_view](std::string_view prefix) {
            return starts_with(msg_view, prefix);
          };

          if (msg_starts_with(unexpected_character)) {
            // NOTE: These two characters will come if explicit line
            // continuation is specified
            if (code.length() >= 2 && code[code.length() - 2] == '\\' &&
                code[code.length() - 1] == '\n') {
              r_state = Input_state::ContinuedSingle;
            }
          } else if (msg_starts_with(eof_while_scanning)) {
            r_state = Input_state::ContinuedSingle;
          } else if (msg_starts_with(unterminated_triple_quoted)) {
            r_state = Input_state::ContinuedSingle;
          } else if (starts_with(msg_view.substr(4), was_never_closed)) {
            r_state = Input_state::ContinuedBlock;
          } else if (msg_starts_with(unexpected_eof)) {
            r_state = Input_state::ContinuedBlock;
          } else if (msg_starts_with(expected_indented_block)) {
            r_state = Input_state::ContinuedBlock;
          }
        }
      }
    }

#if PY_VERSION_HEX >= 0x030c0000
    PyErr_SetRaisedException(exc_value.release());
#else
    PyErr_Restore(exc_type.release(), exc_value.release(), exc_tb.release());
#endif

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
    auto tmp = convert(m_captured_eval_result.get());
    m_captured_eval_result = nullptr;
    return tmp;
  }

  return Value::Null();
}

void Python_context::get_members_of(
    PyObject *object, std::vector<std::pair<bool, std::string>> *out_keys) {
  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);
  py::Release members{PyObject_Dir(object)};

  if (!members || !PySequence_Check(members.get())) return;

  for (Py_ssize_t i = 0, c = PySequence_Size(members.get()); i < c; i++) {
    py::Release m{PySequence_ITEM(members.get(), i)};
    std::string s;

    if (m && pystring_to_string(m, &s) && !shcore::str_beginswith(s, "__")) {
      py::Release mem{PyObject_GetAttrString(object, s.c_str())};
      out_keys->push_back(std::make_pair(PyCallable_Check(mem.get()), s));
    }
  }
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
  return convert(PyDict_GetItemString(_globals, value.c_str()));
}

py::Store Python_context::get_global_py(const std::string &value) {
  return py::Store{PyDict_GetItemString(_globals, value.c_str())};
}

void Python_context::set_global(const std::string &name, const Value &value) {
  WillEnterPython lock;
  auto p = convert(value);
  assert(p);
  // doesn't steal
  PyDict_SetItemString(_globals, name.c_str(), p.get());
  PyObject_SetAttrString(_mysqlsh_globals.get(), name.c_str(), p.get());
}

Value Python_context::convert(PyObject *value) {
  return py::convert(value, this);
}

py::Release Python_context::convert(const Value &value) {
  return py::convert(value);
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

  int64_t code = exc.error()->get_int("code", -1);
  if (code > 0) {
    set_shell_error(exc);
    return;
  }

  auto type = exc.error()->get_string("type", "");
  auto message = exc.error()->get_string("message", "");
  auto error_location = exc.error()->get_string("location", "");

  if (error_location.empty()) error_location = location;

  PyObject *exctype = PyExc_RuntimeError;

  if (!message.empty()) {
    if (!type.empty()) {
      if (exc.is_argument())
        exctype = PyExc_ValueError;
      else if (exc.is_type())
        exctype = PyExc_TypeError;
      else if (exc.is_runtime())
        exctype = PyExc_RuntimeError;
      else
        error_message += type;
    }

    if (code != -1)
      error_message += shcore::str_format(" (%lld)", (long long int)code);

    if (!error_message.empty()) error_message += ": ";

    error_message += message;

    if (!error_location.empty()) error_message += " at " + error_location;

    error_message += "\n";
  }

  PyErr_SetString(
      exctype, (error_location.empty() ? error_message
                                       : error_location + ": " + error_message)
                   .c_str());
}

void Python_context::set_shell_error(const shcore::Error &e) {
  py::Release args{PyTuple_New(2)};
  if (!args) return;

  PyTuple_SET_ITEM(args.get(), 0, PyInt_FromLong(e.code()));
  PyTuple_SET_ITEM(args.get(), 1, PyString_FromString(e.what()));

  auto target = (e.code() < 50000) ? get()->db_error() : get()->error();
  PyErr_SetObject(target.get(), args.get());  // doesn't steal
}

void Python_context::set_python_error(PyObject *obj,
                                      const std::string &location) {
  PyErr_SetString(obj, location.c_str());
}

void Python_context::set_python_error(const std::string &module,
                                      const std::string &exception,
                                      const std::string &location) {
  py::Release py_module{PyImport_ImportModule(module.c_str())};
  if (!py_module) {
    PyErr_Clear();
    throw std::runtime_error(shcore::str_format(
        "Could not import Python '%s' module", module.c_str()));
  }

  PyObject *py_module_dict = PyModule_GetDict(py_module.get());

  PyObject *except_item =
      PyDict_GetItemString(py_module_dict, exception.c_str());
  if (!except_item) {
    throw std::runtime_error(
        shcore::str_format("Could not find type '%s' in '%s' module",
                           exception.c_str(), module.c_str()));
  }

  set_python_error(except_item, location);
}

bool Python_context::pystring_to_string(PyObject *strobject,
                                        std::string *result, bool convert) {
  if (PyUnicode_Check(strobject)) {
    py::Release ref{PyUnicode_AsUTF8String(strobject)};

    if (ref) return pystring_to_string(ref.get(), result, false);

    return false;
  }

  if (PyBytes_Check(strobject)) {
    char *s;
    Py_ssize_t len;
    if (PyBytes_AsStringAndSize(strobject, &s, &len) == -1) return false;

    if (s && (len > 0))
      *result = std::string(s, len);
    else
      *result = "";
    return true;
  }

  // If strobject is not a string, we can choose if we want to convert it to
  // string with PyObject_str() or return an error
  if (convert) {
    py::Release str{PyObject_Str(strobject)};

    if (str) return pystring_to_string(str.get(), result, false);
  }

  return false;
}

bool Python_context::pystring_to_string(const py::Release &strobject,
                                        std::string *result, bool convert) {
  return pystring_to_string(strobject.get(), result, convert);
}

py::Store Python_context::get_shell_list_class() const {
  return _shell_list_class;
}

py::Store Python_context::get_shell_dict_class() const {
  return _shell_dict_class;
}

py::Store Python_context::get_shell_object_class() const {
  return _shell_object_class;
}

py::Store Python_context::get_shell_indexed_object_class() const {
  return _shell_indexed_object_class;
}

py::Store Python_context::get_shell_function_class() const {
  return _shell_function_class;
}

PyObject *Python_context::shell_print(PyObject *UNUSED(self), PyObject *args,
                                      const std::string &stream) {
  if (!Python_context::get_and_check()) return nullptr;

  std::string text;
  if (stream != "error.flush") {
    PyObject *o;
    if (!PyArg_ParseTuple(args, "O", &o)) {
      if (PyTuple_Size(args) == 1 && PyTuple_GetItem(args, 0) == Py_None) {
        PyErr_Clear();
        text = "None";
      } else {
        return nullptr;
      }
    } else if (!pystring_to_string(o, &text, true)) {
      return nullptr;
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
  if (mysqlsh::current_console()->prompt(prompt, &ret) !=
      shcore::Prompt_result::Ok) {
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

PyObject *Python_context::shell_stdout_isatty(PyObject *, PyObject *) {
  Py_INCREF(Py_False);
  return Py_False;
}

PyObject *Python_context::shell_stdout_fileno(PyObject *, PyObject *) {
  try {
    Python_context::set_python_error("io", "UnsupportedOperation",
                                     "Method not supported.");
  } catch (...) {
    set_python_error(PyExc_IOError, "Method not supported.");
  }

  return nullptr;
}

PyObject *Python_context::shell_stderr(PyObject *self, PyObject *args) {
  return shell_print(self, args, "error");
}

PyObject *Python_context::shell_stderr_isatty(PyObject *, PyObject *) {
  Py_INCREF(Py_False);
  return Py_False;
}

PyObject *Python_context::shell_stderr_fileno(PyObject *, PyObject *) {
  try {
    Python_context::set_python_error("io", "UnsupportedOperation",
                                     "Method not supported.");
  } catch (...) {
    set_python_error(PyExc_IOError, "Method not supported.");
  }

  return nullptr;
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

PyObject *Python_context::shell_stdin_isatty(PyObject *, PyObject *) {
  Py_INCREF(Py_False);
  return Py_False;
}

PyObject *Python_context::shell_interactive_eval_hook(PyObject *,
                                                      PyObject *args) {
  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);

  Python_context *ctx;
  if (!(ctx = Python_context::get_and_check())) return nullptr;

  if (PyTuple_Size(args) == 1) {
    ctx->m_captured_eval_result = py::Store{PyTuple_GetItem(args, 0)};
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
    {"isatty", &Python_context::shell_stderr_isatty, METH_NOARGS, ""},
    {"fileno", &Python_context::shell_stderr_fileno, METH_NOARGS, ""},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

PyMethodDef Python_context::ShellStdOutMethods[] = {
    {"write", &Python_context::shell_stdout, METH_VARARGS,
     "Write a string in the SHELL shell."},
    {"flush", &Python_context::shell_flush, METH_NOARGS, ""},
    {"isatty", &Python_context::shell_stdout_isatty, METH_NOARGS, ""},
    {"fileno", &Python_context::shell_stdout_fileno, METH_NOARGS, ""},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

PyMethodDef Python_context::ShellStdInMethods[] = {
    {"read", &Python_context::shell_stdin_read, METH_VARARGS,
     "Read a string through the shell input mechanism."},
    {"readline", &Python_context::shell_stdin_readline, METH_NOARGS,
     "Read a line of string through the shell input mechanism."},
    {"isatty", &Python_context::shell_stdin_isatty, METH_NOARGS, ""},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

PyMethodDef Python_context::ShellPythonSupportMethods[] = {
    {"interactivehook", &Python_context::shell_interactive_eval_hook,
     METH_VARARGS,
     "Custom displayhook to capture interactive expr evaluation results."},
    {"raw_input", &Python_context::shell_raw_input, METH_VARARGS,
     "raw_input([prompt]) -> string\n\nRead a string from standard input.  The "
     "trailing newline is stripped.\nIf the user hits EOF (Unix: Ctl-D, "
     "Windows: Ctl-Z+Return), raise EOFError.\n"},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static void add_module_members(
    Python_context *ctx, PyObject *py_module_ref,
    const std::shared_ptr<shcore::Cpp_object_bridge> &module) {
  // Now inserts every element on the module
  PyObject *py_dict = PyModule_GetDict(py_module_ref);

  for (const auto &member_name : module->get_members()) {
    shcore::Value member = module->get_member_advanced(member_name);
    py::Release value;
    if (member.get_type() == shcore::Function)
      value = wrap(module, member_name);
    else
      value = ctx->convert(member);

    // doesn't steal
    PyDict_SetItemString(py_dict, member_name.c_str(), value.get());
  }
}

void Python_context::register_mysqlsh_builtins() {
  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);

  // Now add built-in shell modules/objects in the mysqlsh module
  auto modules = Object_factory::package_contents("__modules__");

  for (const auto &name : modules) {
    auto module_obj = Object_factory::call_constructor("__modules__", name,
                                                       shcore::Argument_list());

    const auto module =
        std::dynamic_pointer_cast<shcore::Module_base>(module_obj);

    std::string module_name = "mysqlsh." + name;

    py::Release py_module_ref;

    // add mysqlsh module methods directly into the top-level mysqlsh module
    if (name == "mysqlsh") {
      py_module_ref = py::Release::incref(_mysqlsh_module.get());
    } else {
      py_module_ref = py_register_module(module_name, nullptr);
    }

    if (!py_module_ref)
      throw std::runtime_error("Error initializing the '" + name +
                               "' module in Python support");

    // add to the mysqlsh module namespace so that
    // import mysqlsh; mysqlsh.mysql.whatever() still works for backwards
    // compatibility
    if (name == "mysql" || name == "mysqlx")
      PyDict_SetItemString(PyModule_GetDict(_mysqlsh_module.get()),
                           name.c_str(), py_module_ref.get());

    add_module_members(this, py_module_ref.get(), module);
  }
}

void Python_context::register_mysqlsh_module() {
  // Registers the mysqlsh module/package, at least for now this exists
  // only on the python side of things, this module encloses the inner
  // modules: mysql and mysqlx to prevent class names i.e. using connector/py
  // It also exposes other mysqlsh internal machinery.
  // Actual code for this module is in python/packages/mysqlsh
  _mysqlsh_module = py::Store{PyImport_ImportModule("mysqlsh")};
  if (!_mysqlsh_module) {
    PyErr_Print();

    throw std::runtime_error(
        "Error initializing the 'mysqlsh' module in Python support");
  }

  PyObject *py_mysqlsh_dict = PyModule_GetDict(_mysqlsh_module.get());

  if (!PyDict_GetItemString(py_mysqlsh_dict, "DBError")) {
    PyErr_Print();
    assert(0);
  }

  if (!PyDict_GetItemString(py_mysqlsh_dict, "Error")) {
    PyErr_Print();
    assert(0);
  }

  {
    const py::Release path{PyString_FromString(g_mysqlsh_path)};
    PyDict_SetItemString(py_mysqlsh_dict, "executable", path.get());
  }

  _mysqlsh_globals =
      py::Store{PyDict_GetItemString(py_mysqlsh_dict, "globals")};
  assert(_mysqlsh_globals);
}

py::Store Python_context::db_error() const {
  PyObject *py_mysqlsh_dict = PyModule_GetDict(_mysqlsh_module.get());
  return py::Store{PyDict_GetItemString(py_mysqlsh_dict, "DBError")};
}

py::Store Python_context::error() const {
  PyObject *py_mysqlsh_dict = PyModule_GetDict(_mysqlsh_module.get());
  return py::Store{PyDict_GetItemString(py_mysqlsh_dict, "Error")};
}

void Python_context::get_datetime_constructor() {
  py::Release py_datetime_module{PyImport_ImportModule("datetime")};
  if (!py_datetime_module) {
    PyErr_Print();

    throw std::runtime_error("Could not import Python datetime module");
  }

  PyObject *py_datetime_dict = PyModule_GetDict(py_datetime_module.get());

  _datetime = py::Store{PyDict_GetItemString(py_datetime_dict, "datetime")};
  if (!_datetime) PyErr_Print();
  assert(_datetime);

  _date = py::Store{PyDict_GetItemString(py_datetime_dict, "date")};
  if (!_date) PyErr_Print();
  assert(_date);

  _time = py::Store{PyDict_GetItemString(py_datetime_dict, "time")};
  if (!_time) PyErr_Print();
  assert(_time);

  // Creates a temporary datetime object to cache the python type
  {
    auto tmp = create_datetime_object(2000, 1, 1, 1, 1, 1, 0);
    _datetime_type =
        py::Store{reinterpret_cast<PyObject *>(Py_TYPE(tmp.get()))};
  }
  {
    auto tmp = create_date_object(2000, 1, 1);
    _date_type = py::Store{reinterpret_cast<PyObject *>(Py_TYPE(tmp.get()))};
  }
  {
    auto tmp = create_time_object(0, 0, 0, 0);
    _time_type = py::Store{reinterpret_cast<PyObject *>(Py_TYPE(tmp.get()))};
  }
}

py::Release Python_context::create_datetime_object(int year, int month, int day,
                                                   int hour, int minute,
                                                   int second, int useconds) {
  py::Release args{Py_BuildValue("(iiiiiii)", year, month, day, hour, minute,
                                 second, useconds)};

  return py::Release{PyObject_Call(_datetime.get(), args.get(), nullptr)};
}

py::Release Python_context::create_date_object(int year, int month, int day) {
  py::Release args{Py_BuildValue("(iii)", year, month, day)};

  return py::Release{PyObject_Call(_date.get(), args.get(), nullptr)};
}

py::Release Python_context::create_time_object(int hour, int minute, int second,
                                               int useconds) {
  py::Release args{Py_BuildValue("(iiii)", hour, minute, second, useconds)};

  return py::Release{PyObject_Call(_time.get(), args.get(), nullptr)};
}

void Python_context::register_shell_stderr_module() {
  auto module = py_register_module("mysqlsh.shell_stderr", ShellStdErrMethods);
  if (!module)
    throw std::runtime_error(
        "Error initializing SHELL module in Python support");

  _shell_stderr_module = std::move(module);
}

void Python_context::register_shell_stdout_module() {
  auto module = py_register_module("mysqlsh.shell_stdout", ShellStdOutMethods);
  if (!module)
    throw std::runtime_error(
        "Error initializing SHELL module in Python support");

  _shell_stdout_module = std::move(module);
}

void Python_context::register_shell_stdin_module() {
  auto module = py_register_module("mysqlsh.shell_stdin", ShellStdInMethods);
  if (!module)
    throw std::runtime_error(
        "Error initializing SHELL module in Python support");

  _shell_stdin_module = std::move(module);
}

void Python_context::register_shell_python_support_module() {
  {
    auto module = py_register_module("mysqlsh.__shell_python_support__",
                                     ShellPythonSupportMethods);
    if (!module)
      throw std::runtime_error(
          "Error initializing SHELL module in Python support");

    _shell_python_support_module = std::move(module);
  }

  // helper
  auto module_ptr = _shell_python_support_module.get();

  // add the context ptr
  {
    py::Release context_object{PyCapsule_New(this, nullptr, nullptr)};
    PyCapsule_SetContext(context_object.get(), &SHELLTypeSignature);

    if (context_object)
      PyModule_AddObject(module_ptr, "__SHELL__", context_object.release());
  }

  PyModule_AddStringConstant(module_ptr, "INT",
                             shcore::type_name(Integer).c_str());
  PyModule_AddStringConstant(module_ptr, "DOUBLE",
                             shcore::type_name(Float).c_str());
  PyModule_AddStringConstant(module_ptr, "STRING",
                             shcore::type_name(String).c_str());
  PyModule_AddStringConstant(module_ptr, "LIST",
                             shcore::type_name(Array).c_str());
  PyModule_AddStringConstant(module_ptr, "DICT",
                             shcore::type_name(Map).c_str());
  PyModule_AddStringConstant(module_ptr, "OBJECT",
                             shcore::type_name(Object).c_str());
  PyModule_AddStringConstant(module_ptr, "FUNCTION",
                             shcore::type_name(Function).c_str());

  init_shell_dict_type();
  init_shell_list_type();
  init_shell_object_type();
  init_shell_function_type();
}

Value Python_context::execute_module(const std::string &module_name,
                                     const std::vector<std::string> &argv) {
  shcore::Value ret_val;
  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);

  const py::Release modname{PyString_FromString(module_name.c_str())};

  set_argv(argv);

  const py::Release runpy{PyImport_ImportModule("runpy")};

  if (!runpy) {
    PyErr_Print();
    throw shcore::Exception::runtime_error("Could not import runpy module");
  }

  const py::Release runmodule{
      PyObject_GetAttrString(runpy.get(), "_run_module_as_main")};

  if (!runmodule) {
    PyErr_Print();
    throw shcore::Exception::runtime_error(
        "Could not access runpy._run_module_as_main");
  }

  const py::Release runargs{PyTuple_Pack(2, modname.get(), Py_False)};

  if (!runargs) {
    PyErr_Print();
    throw shcore::Exception::runtime_error(
        "Could not create arguments for runpy._run_module_as_main");
  }

  module_processing = true;
  const py::Release py_result{
      PyObject_Call(runmodule.get(), runargs.get(), NULL)};
  module_processing = false;

  if (!py_result) {
    if (PyErr_ExceptionMatches(PyExc_SystemExit)) exit_error = true;

    PyErr_Print();
  }

  if (py_result) {
    ret_val = convert(py_result.get());
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
  FILE *file = nullptr;
  bool executed = false;
  std::string file_name = plugin.file;

  // The path to the plugins folder needs to be in sys.path while the plugin is
  // being loaded
  std::string plugin_dir = shcore::path::dirname(file_name);
  if (!plugin.main) plugin_dir = shcore::path::dirname(plugin_dir);
  plugin_dir = shcore::path::dirname(plugin_dir);

  try {
#ifdef _WIN32
    file = _wfopen(utf8_to_wide(file_name).c_str(), L"r");
#else   // !_WIN32
    file = fopen(file_name.c_str(), "r");
#endif  // !_WIN32
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
    py::Release sys_path_backup{
        PyList_GetSlice(sys_path, 0, PyList_Size(sys_path))};
    if (!sys_path_backup) {
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
    PySys_SetObject(const_cast<char *>("path"), sys_path_backup.get());

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

  return ret_val;
}

namespace {
bool get_code_and_msg(int *out_code, std::string *out_msg) {
  bool ret = false;

#if PY_VERSION_HEX >= 0x030c0000
  py::Release exc_value{PyErr_GetRaisedException()};
#else
  PyObject *type, *value, *tb;

  PyErr_Fetch(&type, &value, &tb);
  PyErr_NormalizeException(&type, &value, &tb);

  py::Release exc_type{type}, exc_value{value}, exc_tb{tb};
#endif

  {
    py::Release args{PyObject_GetAttrString(exc_value.get(), "args")};

    if (args && PyTuple_Check(args.get()) &&
        PyTuple_GET_SIZE(args.get()) == 2) {
      const char *msg;

      if (PyTuple_GetItem(args.get(), 0) == Py_None) {
        PyObject *dummy;
        *out_code = 0;
        if (PyArg_ParseTuple(args.get(), "Os", &dummy, &msg) && msg) {
          ret = true;
          *out_msg = msg;
        }
      } else {
        if (PyArg_ParseTuple(args.get(), "is", out_code, &msg) && msg) {
          ret = true;
          *out_msg = msg;
        }
      }
    }
  }

  if (!ret) {
#if PY_VERSION_HEX >= 0x030c0000
    PyErr_SetRaisedException(exc_value.release());
#else
    PyErr_Restore(exc_type.release(), exc_value.release(), exc_tb.release());
#endif
  }

  return ret;
}
}  // namespace

void Python_context::throw_if_mysqlsh_error() {
  assert(PyErr_Occurred() != nullptr);

  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);

  auto db_error_ref = db_error();
  auto error_ref = error();

  if (PyErr_ExceptionMatches(db_error_ref.get())) {
    int code = 0;
    std::string msg;
    if (get_code_and_msg(&code, &msg))
      throw shcore::Exception::mysql_error_with_code(msg, code);

  } else if (PyErr_ExceptionMatches(error_ref.get())) {
    int code = 0;
    std::string msg;
    if (get_code_and_msg(&code, &msg)) {
      if (code <= 0)
        throw shcore::Exception::runtime_error(msg);
      else
        throw shcore::Exception(msg, code);
    }
  }
}

void Python_context::clear_exception() { PyErr_Clear(); }

std::string Python_context::fetch_and_clear_exception(
    std::string *out_traceback, std::string *out_type) {
  // If out_traceback is NULL, then return the whole traceback (legacy behavior)
  // If out_traceback is NOT NULL, then only the exception text is returned and
  // out_traceback contains the full traceback

  // if out_type is set, then out_traceback must also be set
  assert(!out_type || (out_type && out_traceback));

  if (!PyErr_Occurred()) return {};

  std::string exception;
  shcore::Scoped_naming_style ns(shcore::NamingStyle::LowerCaseUnderscores);

#if PY_VERSION_HEX >= 0x030c0000
  py::Release exc_value{PyErr_GetRaisedException()};
  py::Release exc_tb{PyException_GetTraceback(exc_value.get())};
#else
  PyObject *type, *value, *tb;

  PyErr_Fetch(&type, &value, &tb);
  PyErr_NormalizeException(&type, &value, &tb);

  py::Release exc_type{type}, exc_value{value}, exc_tb{tb};
#endif

  {
    py::Release str{PyObject_Str(exc_value.get())};
    pystring_to_string(str, &exception);
  }

  if (!exc_tb) {
    return exception;
  }

#if PY_VERSION_HEX >= 0x030c0000
  // docs for traceback.print_exception()/format_exception() say:
  // Since Python 3.10, instead of passing value and tb, an exception object can
  // be passed as the first argument.
  // in our case, the first argument is exc_type
  py::Release exc_type = std::move(exc_value);
  exc_tb.reset();
#endif

  py::Release traceback_module{PyImport_ImportModule("traceback")};

  if (!traceback_module) {
    return exception;
  }

  std::string traceback;
  py::Release format_exception{
      PyObject_GetAttrString(traceback_module.get(), "format_exception")};

  if (format_exception && PyCallable_Check(format_exception.get())) {
    py::Release backtrace{
        PyObject_CallFunctionObjArgs(format_exception.get(), exc_type.get(),
                                     exc_value.get(), exc_tb.get(), nullptr)};

    if (backtrace && PyList_Check(backtrace.get())) {
      traceback = "\n";

      for (Py_ssize_t c = PyList_Size(backtrace.get()), i = 0; i < c; ++i) {
        std::string item;
        pystring_to_string(PyList_GetItem(backtrace.get(), i), &item);
        traceback += item;
      }
    }
  }

  if (!out_traceback) {
    return traceback;
  } else {
    *out_traceback = traceback;
    if (out_type) {
      *out_type = Py_TYPE(exc_type.get())->tp_name;
    }
    return exception;
  }
}

}  // namespace shcore
