/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "scripting/python_context.h"

#include <exception>

#include "scripting/common.h"
#include "scripting/module_registry.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

#include "scripting/object_factory.h"
#include "scripting/python_type_conversion.h"

#include "shellcore/shell_core_options.h"
#include "modules/mod_utils.h"

#ifdef _WINDOWS
#include <windows.h>
#endif

// used to identify a proper SHELL context object as a PyCObject
static const char *SHELLTypeSignature = "SHELLCONTEXT";

namespace shcore {
bool Python_context::exit_error = false;
bool Python_context::module_processing = false;
std::unique_ptr<Python_init_singleton> Python_init_singleton::_instance;
int Python_init_singleton::cnt = 0;

std::string Python_init_singleton::get_new_scope_name() {
  return shcore::str_format("__main__%d", ++cnt);
}

void Python_init_singleton::init_python() {
  if (_instance.get() == NULL)
    _instance.reset(new Python_init_singleton());
}

Python_context::Python_context(Interpreter_delegate *deleg) throw(Exception)
    : _types(this) {
  _delegate = deleg;

  Python_init_singleton::init_python();

  _global_namespace = PyImport_AddModule("__main__");
  _globals = PyModule_GetDict(_global_namespace);

  _locals = _globals;

  if (!_global_namespace || !_globals) {
    throw Exception::runtime_error("Error initializing python context.");
    PyErr_Print();
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
  if ((*shcore::Shell_core_options::get())[SHCORE_INTERACTIVE] ==
      shcore::Value::True()) {
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

Python_context::~Python_context() {
  PyObject *key, *value;
  Py_ssize_t pos = 0;

  PyObject *py_mysqlsh_dict = PyModule_GetDict(_shell_mysqlsh_module);
  // Release all internal module references
  while (PyDict_Next(py_mysqlsh_dict, &pos, &key, &value)) {
    if (PyModule_Check(value)) {
      PyObject *keys;
      PyObject *dict = PyModule_GetDict(value);
      keys = PyDict_Keys(dict);
      for (Py_ssize_t i = 0, c = PyList_GET_SIZE(keys); i < c; i++) {
        std::string name = PyString_AsString(PyList_GetItem(keys, i));
        if (_modules.find(value) != _modules.end()) {
          PyDict_DelItem(dict, PyList_GetItem(keys, i));
          break;
        }
      }
      Py_XDECREF(keys);
    }
  }

  PyEval_RestoreThread(_main_thread_state);
  _main_thread_state = NULL;
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

  return static_cast<Python_context *>(PyCObject_AsVoidPtr(ctx));
}

Python_context *Python_context::get_and_check() {
  try {
    return Python_context::get();
  } catch (std::exception &exc) {
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
  std::vector<const char *> argvv;

  for (const std::string &s : argv)
    argvv.push_back(s.c_str());

  argvv.push_back(nullptr);

  PySys_SetArgv(argv.size(), const_cast<char **>(argvv.data()));
}

Value Python_context::execute(
    const std::string &code, const std::string &UNUSED(source),
    const std::vector<std::string> &argv) throw(Exception) {
  PyObject *py_result;
  Value retvalue;

  set_argv(argv);

  py_result = PyRun_String(code.c_str(), Py_file_input, _globals, _locals);

  if (!py_result) {
    PyErr_Print();
    return Value();
  }

  Value tmp(_types.pyobj_to_shcore_value(py_result));
  Py_XDECREF(py_result);
  return tmp;
}

Value Python_context::execute_interactive(
    const std::string &code, Input_state &r_state) noexcept {
  Value retvalue;

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

  PyObject *py_result =
      PyRun_String(code.c_str(), Py_single_input, _globals, _locals);

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
          if (code[code.length() - 2] == '\\' &&
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
  } else if (code[code.length() - 2] == '\\' &&
             code[code.length() - 1] == '\n') {
    r_state = Input_state::ContinuedSingle;
  }

  if (!_captured_eval_result.empty()) {
    Value tmp(_types.pyobj_to_shcore_value(_captured_eval_result.back()));
    _captured_eval_result.pop_back();
    return tmp;
  }
  return Value::Null();
}

Value Python_context::get_global(const std::string &value) {
  PyObject *py_result;

  py_result = PyDict_GetItemString(_globals, value.c_str());

  return _types.pyobj_to_shcore_value(py_result);
}

void Python_context::set_global(const std::string &name, const Value &value) {
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

  if (error_location.empty())
    error_location = location;

  if (!message.empty()) {
    if (!type.empty())
      error_message += type;

    if (code != -1)
      error_message += shcore::str_format(" (%lld)", (long long int)code);

    if (!error_message.empty())
      error_message += ": ";

    error_message += message;

    if (!error_location.empty())
      error_message += " at " + error_location;

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
  log_error("%s", location.c_str());
  PyErr_SetString(obj, location.c_str());
}

bool Python_context::pystring_to_string(PyObject *strobject,
                                        std::string &ret_string, bool convert) {
  if (PyUnicode_Check(strobject)) {
    PyObject *ref = PyUnicode_AsUTF8String(strobject);
    if (ref) {
      char *s;
      Py_ssize_t len;
      PyString_AsStringAndSize(ref, &s, &len);
      if (s)
        ret_string = std::string(s, len);
      else
        ret_string = "";
      Py_DECREF(ref);
      return true;
    }
    return false;
  }

  if (PyString_Check(strobject)) {
    char *s;
    Py_ssize_t len;
    PyString_AsStringAndSize(strobject, &s, &len);
    if (s)
      ret_string = std::string(s, len);
    else
      ret_string = "";
    return true;
  }

  /*
   * If ret_string is not a string, we can choose if we want to convert it to
   * string with PyObject_str() or return an error
   */
  if (convert) {
    PyObject *str = PyObject_Str(strobject);
    if (str) {
      bool ret = pystring_to_string(str, ret_string, false);
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

  if (!(ctx = Python_context::get_and_check()))
    return NULL;

  if (stream != "error.flush") {
    PyObject *o;
    if (!PyArg_ParseTuple(args, "O", &o)) {
      if (PyTuple_Size(args) == 1 && PyTuple_GetItem(args, 0) == Py_None) {
        PyErr_Clear();
        text = "None";
      } else {
        return NULL;
      }
    } else if (!ctx->pystring_to_string(o, text, true)) {
      return NULL;
    }
  }

  if (stream == "error") {
    ctx->_delegate->print_error(ctx->_delegate->user_data, text.c_str());
  } else {
    ctx->_delegate->print(ctx->_delegate->user_data, text.c_str());
  }

  Py_INCREF(Py_None);
  return Py_None;
}

PyObject *Python_context::shell_raw_input(PyObject *, PyObject *args) {
  const char *prompt = nullptr;
  if (!PyArg_ParseTuple(args, "|s", &prompt))
    return nullptr;

  Python_context *ctx;
  if (!(ctx = Python_context::get_and_check()))
    return nullptr;

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
  if (!line.empty())
    line = line.substr(0, line.size() - 1);  // strip \n
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
  shcore::Prompt_result result =
      _delegate->prompt(_delegate->user_data, prompt.c_str(), &ret);
  if (result != shcore::Prompt_result::Ok) {
    return {result, ""};
  }
  _stdin_buffer.append(ret).append("\n");
  std::string tmp;
  std::swap(tmp, _stdin_buffer);
  return {result, tmp};
}

PyObject *Python_context::shell_stdout(PyObject *self, PyObject *args) {
  return shell_print(self, args, "out");
}

PyObject *Python_context::shell_stderr(PyObject *self, PyObject *args) {
  return shell_print(self, args, "error");
}

PyObject *Python_context::shell_flush_stderr(PyObject *self, PyObject *args) {
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject *Python_context::shell_flush(PyObject *self, PyObject *args) {
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject *Python_context::shell_stdin_read(PyObject *self, PyObject *args) {
  Python_context *ctx;

  if (!(ctx = Python_context::get_and_check()))
    return nullptr;

  // stdin.read(n) requires n number of chars to be read,
  // but we can only read one line at a time. Thus, we read one line at a time
  // from user, buffer it and return the requested number of chars
  int n;
  if (!PyArg_ParseTuple(args, "i", &n))
    return nullptr;

  for (;;) {
    if (ctx->_stdin_buffer.size() >= n) {
      PyObject *str =
          PyString_FromString(ctx->_stdin_buffer.substr(0, n).c_str());
      ctx->_stdin_buffer = ctx->_stdin_buffer.substr(n);
      return str;
    }

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
    ctx->_stdin_buffer.append(line);
  }
}

PyObject *Python_context::shell_stdin_readline(PyObject *self, PyObject *args) {
  Python_context *ctx;

  if (!(ctx = Python_context::get_and_check()))
    return nullptr;

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


PyObject *Python_context::shell_interactive_eval_hook(PyObject *UNUSED(self),
                                                      PyObject *args) {
  Python_context *ctx;
  std::string text;

  if (!(ctx = Python_context::get_and_check()))
    return NULL;

  if (PyTuple_Size(args) == 1) {
    ctx->_captured_eval_result.push_back(PyTuple_GetItem(args, 0));
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

PyObject *Python_context::call_module_function(PyObject *self, PyObject *args,
                                               PyObject *keywords,
                                               const std::string &name) {
  Python_context *ctx;
  std::string text;

  if (!(ctx = Python_context::get_and_check()))
    return NULL;

  shcore::Argument_list shell_args;

  if (keywords) {
    shell_args.push_back(ctx->pyobj_to_shcore_value(keywords));
  } else if (args) {
    int size = static_cast<int>(PyTuple_Size(args));

    for (int index = 0; index < size; index++)
      shell_args.push_back(
          ctx->pyobj_to_shcore_value(PyTuple_GetItem(args, index)));
  }

  PyObject *py_ret_val = nullptr;
  try {
    auto module = _modules[self];
    shcore::Value ret_val = module->call(name, shell_args);
    py_ret_val = ctx->shcore_value_to_pyobj(ret_val);
  } catch (shcore::Exception &e) {
    set_python_error(e);
  }

  if (py_ret_val) {
    return py_ret_val;
  } else {
    Py_INCREF(Py_None);
    return Py_None;
  }

  return NULL;
}

void Python_context::register_mysqlsh_module() {
  // Registers the mysqlsh module/package, at least for now this exists
  // only on the python side of things, this module encloses the inner
  // modules: mysql and mysqlx to prevent class names i.e. using connector/py
  PyMethodDef py_mysqlsh_members[] = {{NULL, NULL, 0, NULL}};
  _shell_mysqlsh_module = Py_InitModule("mysqlsh", py_mysqlsh_members);

  if (_shell_mysqlsh_module == NULL)
    throw std::runtime_error(
        "Error initializing the 'mysqlsh' module in Python support");

  PyObject *py_mysqlsh_dict = PyModule_GetDict(_shell_mysqlsh_module);

  // Now registers each available module as part of the mysqlsh package
  auto modules = Object_factory::package_contents("__modules__");

  for (auto name : modules) {
    auto module_obj = Object_factory::call_constructor("__modules__", name,
                                                       shcore::Argument_list());

    std::shared_ptr<shcore::Module_base> module =
        std::dynamic_pointer_cast<shcore::Module_base>(module_obj);

    PyMethodDef py_members[] = {{NULL, NULL, 0, NULL}};

    // The modules will be registered in python as __<name>__
    // But exposed in the mysqlsh module as the original name
    std::string module_name = "__" + name + "__";
    PyObject *py_module_ref = Py_InitModule(module_name.c_str(), py_members);

    if (py_module_ref == NULL)
      throw std::runtime_error("Error initializing the '" + name +
                               "' module in Python support");

    _modules[py_module_ref] = module;

    // Now inserts every element on the module
    PyObject *py_dict = PyModule_GetDict(py_module_ref);

    for (auto &name :
         module->get_members_advanced(shcore::LowerCaseUnderscores)) {
      shcore::Value member =
          module->get_member_advanced(name, shcore::LowerCaseUnderscores);
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

  // Create custom exception objects
  _db_error = PyErr_NewException((char*)"mysqlsh.DBError", NULL, NULL);

  // Finally inserts the globals
  PyDict_SetItemString(py_mysqlsh_dict, "globals", _global_namespace);
}

void Python_context::register_shell_stderr_module() {
  PyObject *module = Py_InitModule("shell_stderr", ShellStdErrMethods);
  if (module == NULL)
    throw std::runtime_error(
        "Error initializing SHELL module in Python support");

  _shell_stderr_module = module;
}

void Python_context::register_shell_stdout_module() {
  PyObject *module = Py_InitModule("shell_stdout", ShellStdOutMethods);
  if (module == NULL)
    throw std::runtime_error(
        "Error initializing SHELL module in Python support");

  _shell_stdout_module = module;
}

void Python_context::register_shell_stdin_module() {
  PyObject *module = Py_InitModule("shell_stdin", ShellStdInMethods);
  if (module == NULL)
    throw std::runtime_error(
        "Error initializing SHELL module in Python support");

  _shell_stdin_module = module;
}

void Python_context::register_shell_python_support_module() {
  PyObject *module =
      Py_InitModule("shell_python_support", ShellPythonSupportMethods);
  if (module == NULL)
    throw std::runtime_error(
        "Error initializing SHELL module in Python support");

  _shell_python_support_module = module;

  // add the context ptr
  PyObject *context_object =
      PyCObject_FromVoidPtrAndDesc(this, &SHELLTypeSignature, NULL);
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

  init_shell_list_type();
  init_shell_dict_type();
  init_shell_object_type();
  init_shell_function_type();
}

bool Python_context::is_module(const std::string &file_name) {
  bool ret_val = false;

  PyObject *argv0 = NULL, *importer = NULL;

  ret_val = ((argv0 = PyString_FromString(file_name.c_str())) &&
              (importer = PyImport_GetImporter(argv0)) &&
              (importer != Py_None) &&
              (importer->ob_type != &PyNullImporter_Type));

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
      if (runpy == NULL)
        throw shcore::Exception::runtime_error("Could not import runpy module");

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
        if (PyErr_ExceptionMatches(PyExc_SystemExit))
          exit_error = true;

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
}  // namespace shcore
