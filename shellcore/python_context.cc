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

#include <Python.h>

#include "shellcore/python_context.h"
#include "shellcore/python_utils.h"
#include "shellcore/shell_core.h"
#include "shellcore/common.h"

#include "shellcore/object_factory.h"
#include "shellcore/python_type_conversion.h"

#ifdef _WINDOWS
#  include <windows.h>
#endif

// used to identify a proper SHELL context object as a PyCObject
static const char *SHELLTypeSignature = "SHELLCONTEXT";

namespace shcore
{
  Python_context::Python_context(Interpreter_delegate *deleg) throw (Exception)
    : _types(this), _local_initialization(false)
  {
    if (!Py_IsInitialized())
    {
#ifdef _WINDOWS
      Py_NoSiteFlag = 1;
#endif
      Py_InitializeEx(0);

      _local_initialization = true;
    }

    _delegate = deleg;

    // register shell module
    register_shell_module();
    register_shell_stderr_module();
    register_mysqlx_module();
    register_mysql_module();

    PyObject *main = PyImport_AddModule("__main__");
    _globals = PyModule_GetDict(main);

    if (!main || !_globals)
    {
      throw Exception::runtime_error("Error initializing python context.");
      PyErr_Print();
    }

    PyDict_SetItemString(PyModule_GetDict(main),
    "shell", PyImport_ImportModule("shell"));

    PySys_SetObject((char*)"real_stdout", PySys_GetObject((char*)"stdout"));
    PySys_SetObject((char*)"real_stderr", PySys_GetObject((char*)"stderr"));
    PySys_SetObject((char*)"real_stdin", PySys_GetObject((char*)"stdin"));

    // make sys.stdout and sys.stderr send stuff to SHELL
    PySys_SetObject((char*)"stdout", get_shell_module());
    PySys_SetObject((char*)"stderr", get_shell_stderr_module());

    // set stdin to the Sh shell console
    PySys_SetObject((char*)"stdin", get_shell_module());

    // Stores the main thread state
    _main_thread_state = PyThreadState_Get();

    PyEval_InitThreads();
    PyEval_SaveThread();

    _types.init();
  }

  Python_context::~Python_context()
  {
    PyEval_RestoreThread(_main_thread_state);
    _main_thread_state = NULL;
    // Restores and finish python if was initialized locally
    if (_local_initialization)
    {
      Py_Finalize();
    }
  }

  /*
   * Gets the Python_context from the Python interpreter.
   */
  Python_context *Python_context::get()
  {
    PyObject *ctx;
    PyObject *module;
    PyObject *dict;

    module = PyDict_GetItemString(PyImport_GetModuleDict(), "shell");
    if (!module)
      throw std::runtime_error("SHELL module not found in Python runtime");

    dict = PyModule_GetDict(module);
    if (!dict)
      throw std::runtime_error("SHELL module is invalid in Python runtime");

    ctx = PyDict_GetItemString(dict, "__SHELL__");
    if (!ctx)
      throw std::runtime_error("SHELL context not found in Python runtime");

    return static_cast<Python_context*>(PyCObject_AsVoidPtr(ctx));
  }

  Python_context *Python_context::get_and_check()
  {
    try
    {
      return Python_context::get();
    }
    catch (std::exception &exc)
    {
      Python_context::set_python_error(PyExc_SystemError, "Could not get SHELL context: "); // TODO: add exc content
    }
    return NULL;
  }

  PyObject *Python_context::get_shell_module() {
    return _shell_module;
  }

  PyObject *Python_context::get_shell_stderr_module() {
    return _shell_stderr_module;
  }

  Value Python_context::execute(const std::string &code, boost::system::error_code &UNUSED(ret_error), const std::string& UNUSED(source)) throw (Exception)
  {
    PyObject *py_result;
    Value retvalue;

    // PyRun_SimpleString(code.c_str());
    py_result = PyRun_String(code.c_str(), Py_single_input /* with Py_eval_input print won't work */, _globals, _globals);

    // TODO/WARNING: Py_single_input always returns Py_None

    if (!py_result)
    {
      // TODO: use log_debug() as soon as logging is integrated
      PyErr_Print();
      return Value();
    }

    return _types.pyobj_to_shcore_value(py_result);
  }

  Value Python_context::execute_interactive(const std::string &code) BOOST_NOEXCEPT_OR_NOTHROW
  {
    Value retvalue;

    /*
     PyRun_String() works as follows:
     If the 2nd param is Py_single_input, the function will take any code as input
     and execute it. But the return value will always be None.

     If it is Py_eval_input, then the function will evaluate the code as an expression
     and return the result. But that will produce a syntax error if the code is NOT
     an expression.

     What we want is to be able to execute any arbitrary code and get it to return
     the expression result if it's an expression or None (or whatever) if it's not.
     That doesn't seem to be possible.

     OTOH sys.displayhook is called whenever the interpreter is outputting interactive
     evaluation results, with the original object (not string). So we workaround by
     installing a temporary displayhook to capture the evaluation results...
     */

    PyObject *orig_hook = PySys_GetObject((char*)"displayhook");
    Py_INCREF(orig_hook);

    PySys_SetObject((char*)"displayhook", PyDict_GetItemString(PyModule_GetDict(_shell_module), (char*)"interactivehook"));

    PyObject *py_result = PyRun_String(code.c_str(), Py_single_input, _globals, _globals);

    PySys_SetObject((char*)"displayhook", orig_hook);
    Py_DECREF(orig_hook);
    if (!py_result)
    {
      // TODO: use log_debug() as soon as logging is integrated
      PyErr_Print();
      return Value();
    }

    if (!_captured_eval_result.empty())
    {
      Value tmp(_types.pyobj_to_shcore_value(_captured_eval_result.back()));
      _captured_eval_result.pop_back();
      return tmp;
    }
    return Value::Null();
  }

  Value Python_context::get_global(const std::string &value)
  {
    PyObject *py_result;

    py_result = PyDict_GetItemString(_globals, value.c_str());
    if (!py_result)
    {
      // TODO: log error
      PyErr_Print();
    }

    return _types.pyobj_to_shcore_value(py_result);
  }

  void Python_context::set_global(const std::string &name, const Value &value)
  {
    PyDict_SetItemString(_globals, name.c_str(), _types.shcore_value_to_pyobj(value));
  }

  Value Python_context::pyobj_to_shcore_value(PyObject *value)
  {
    return _types.pyobj_to_shcore_value(value);
  }

  PyObject *Python_context::shcore_value_to_pyobj(const Value &value)
  {
    return _types.shcore_value_to_pyobj(value);
  }

  void Python_context::set_python_error(const std::exception &exc, const std::string &location)
  {
    PyErr_SetString(PyExc_SystemError, (location.empty() ? exc.what() : location + ": " + exc.what()).c_str());
  }

  void Python_context::set_python_error(PyObject *obj, const std::string &location)
  {
    log_error("%s", location.c_str());
    PyErr_SetString(obj, location.c_str());
  }

  bool Python_context::pystring_to_string(PyObject *strobject, std::string &ret_string, bool convert)
  {
    if (PyUnicode_Check(strobject))
    {
      PyObject *ref = PyUnicode_AsUTF8String(strobject);
      if (ref)
      {
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

    if (PyString_Check(strobject))
    {
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
     * If ret_string is not a string, we can choose if we want to convert it to string with PyObject_str()
     * or return an error
     */
    if (convert)
    {
      PyObject *str = PyObject_Str(strobject);
      if (str)
      {
        bool ret = pystring_to_string(str, ret_string, false);
        Py_DECREF(str);
        return ret;
      }
    }

    return false;
  }

  AutoPyObject Python_context::get_shell_list_class()
  {
    return _shell_list_class;
  }

  AutoPyObject Python_context::get_shell_dict_class()
  {
    return _shell_dict_class;
  }

  AutoPyObject Python_context::get_shell_object_class()
  {
    return _shell_object_class;
  }

  AutoPyObject Python_context::get_shell_function_class()
  {
    return _shell_function_class;
  }

  PyObject *Python_context::shell_print(PyObject *UNUSED(self), PyObject *args, const std::string& stream)
  {
    Python_context *ctx;
    std::string text;

    if (!(ctx = Python_context::get_and_check()))
      return NULL;

    PyObject *o;
    if (!PyArg_ParseTuple(args, "O", &o))
    {
      if (PyTuple_Size(args) == 1 && PyTuple_GetItem(args, 0) == Py_None)
      {
        PyErr_Clear();
        text = "None";
      }
      else
        return NULL;
    }
    else
    if (!ctx->pystring_to_string(o, text, true))
      return NULL;

#ifdef _WIN32
    OutputDebugStringA(text.c_str());
#else
    // g_print("%s", text.c_str());
    // TODO: logging
#endif
    if (stream == "error")
      ctx->_delegate->print_error(ctx->_delegate->user_data, text.c_str());
    else
      ctx->_delegate->print(ctx->_delegate->user_data, text.c_str());

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyObject *Python_context::shell_stdout(PyObject *self, PyObject *args)
  {
    return shell_print(self, args, "out");
  }

  PyObject *Python_context::shell_stderr(PyObject *self, PyObject *args)
  {
    return shell_print(self, args, "error");
  }

  PyObject *Python_context::shell_interactive_eval_hook(PyObject *UNUSED(self), PyObject *args)
  {
    Python_context *ctx;
    std::string text;

    if (!(ctx = Python_context::get_and_check()))
      return NULL;

    if (PyTuple_Size(args) == 1)
      ctx->_captured_eval_result.push_back(PyTuple_GetItem(args, 0));
    else
    {
      char buff[100];
      snprintf(buff, sizeof(buff), "interactivehook() takes exactly 1 argument (%i given)", (int)PyTuple_Size(args));
      PyErr_SetString(PyExc_TypeError, buff);
      return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
  }

  /* Register shell related functionality as a module */
  static PyMethodDef ShellModuleMethods[] = {
    { "write", &Python_context::shell_stdout, METH_VARARGS,
    "Write a string in the SHELL shell." },
    { "interactivehook", &Python_context::shell_interactive_eval_hook, METH_VARARGS,
      "Custom displayhook to capture interactive expr evaluation results." },
      { NULL, NULL, 0, NULL }        /* Sentinel */
  };

  static PyMethodDef ShellStdErrMethods[] = {
    { "write", &Python_context::shell_stderr, METH_VARARGS,
    "Write an error string in the SHELL shell." },
    { NULL, NULL, 0, NULL }        /* Sentinel */
  };

  void Python_context::register_shell_module() {
    PyObject *module = Py_InitModule("shell", ShellModuleMethods);
    if (module == NULL)
      throw std::runtime_error("Error initializing SHELL module in Python support");

    _shell_module = module;

    // add the context ptr
    PyObject* context_object = PyCObject_FromVoidPtrAndDesc(this, &SHELLTypeSignature, NULL);
    if (context_object != NULL)
      PyModule_AddObject(module, "__SHELL__", context_object);

    PyModule_AddStringConstant(module, "INT", (char*)shcore::type_name(Integer).c_str());
    PyModule_AddStringConstant(module, "DOUBLE", (char*)shcore::type_name(Float).c_str());
    PyModule_AddStringConstant(module, "STRING", (char*)shcore::type_name(String).c_str());
    PyModule_AddStringConstant(module, "LIST", (char*)shcore::type_name(Array).c_str());
    PyModule_AddStringConstant(module, "DICT", (char*)shcore::type_name(Map).c_str());
    PyModule_AddStringConstant(module, "OBJECT", (char*)shcore::type_name(Object).c_str());
    PyModule_AddStringConstant(module, "FUNCTION", (char*)shcore::type_name(Function).c_str());

    init_shell_list_type();
    init_shell_dict_type();
    init_shell_object_type();
    init_shell_function_type();
  }

  void Python_context::register_shell_stderr_module() {
    PyObject *module = Py_InitModule("shell_stderr", ShellStdErrMethods);
    if (module == NULL)
      throw std::runtime_error("Error initializing SHELL module in Python support");

    _shell_stderr_module = module;
  }

  PyObject *Python_context::get_session(PyObject *UNUSED(self), PyObject *args, const std::string &module, const std::string &type)
  {
    Python_context *ctx;
    std::string text;

    if (!(ctx = Python_context::get_and_check()))
      return NULL;

    shcore::Argument_list shell_args;
    int size = (int)PyTuple_Size(args);
    for (int index = 0; index < size; index++)
      shell_args.push_back(ctx->pyobj_to_shcore_value(PyTuple_GetItem(args, index)));

    PyObject *py_session = NULL;

    try
    {
      shcore::Value session(Object_factory::call_constructor(module, type, shell_args));
      py_session = ctx->shcore_value_to_pyobj(session);
    }
    catch (shcore::Exception &e)
    {
      PyErr_SetString(PyExc_RuntimeError, e.what());
    }

    if (py_session)
    {
      Py_INCREF(py_session);
      return py_session;
    }
    else
    {
      Py_INCREF(Py_None);
      return Py_None;
    }
  }

  PyObject *Python_context::mysqlx_get_session(PyObject *self, PyObject *args)
  {
    return get_session(self, args, "mysqlx", "XSession");
  }

  PyObject *Python_context::mysqlx_get_node_session(PyObject *self, PyObject *args)
  {
    return get_session(self, args, "mysqlx", "NodeSession");
  }

  PyObject *Python_context::mysql_get_classic_session(PyObject *self, PyObject *args)
  {
    return get_session(self, args, "mysql", "ClassicSession");
  }

  static PyMethodDef MysqlxModuleMethods[] =
  {
    { "getSession", &Python_context::mysqlx_get_session, METH_VARARGS,
    "Creates an XSession object." },
    { "getNodeSession", &Python_context::mysqlx_get_node_session, METH_VARARGS,
    "Creates a NodeSession object." },
    { NULL, NULL, 0, NULL }        /* Sentinel */
  };

  static PyMethodDef MysqlModuleMethods[] =
  {
    { "getClassicSession", &Python_context::mysql_get_classic_session, METH_VARARGS,
    "Creates an ClassicSession object." },
    { NULL, NULL, 0, NULL }        /* Sentinel */
  };

  void Python_context::register_mysqlx_module()
  {
    PyObject *module = Py_InitModule("mysqlx", MysqlxModuleMethods);

    if (module == NULL)
      throw std::runtime_error("Error initializing mysqlx module in Python support");

    _mysqlx_module = module;
  }

  void Python_context::register_mysql_module()
  {
    PyObject *module = Py_InitModule("mysql", MysqlModuleMethods);

    if (module == NULL)
      throw std::runtime_error("Error initializing mysql module in Python support");

    _mysql_module = module;
  }
}