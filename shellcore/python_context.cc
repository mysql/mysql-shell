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
#include "shellcore/python_context.h"
#include "shellcore/python_utils.h"
#include "shellcore/shell_core.h"
#include "shellcore/common.h"
#include "modules/mod_mysqlx_constant.h"
#include "utils/utils_file.h"

#include "shellcore/object_factory.h"
#include "shellcore/python_type_conversion.h"
#include "shellcore/shell_registry.h"
#include <boost/format.hpp>
#include <exception>

#ifdef _WINDOWS
#  include <windows.h>
#endif

// used to identify a proper SHELL context object as a PyCObject
static const char *SHELLTypeSignature = "SHELLCONTEXT";

namespace shcore
{
  Python_context::Python_context(Interpreter_delegate *deleg) throw (Exception)
    : _local_initialization(false), _types(this)
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

    set_global_item("shell", "options", Value(boost::static_pointer_cast<Object_bridge>(Shell_core_options::get_instance())));
    set_global_item("shell", "storedSessions", StoredSessions::get());
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

    py_result = PyRun_String(code.c_str(), Py_file_input, _globals, _globals);

    if (!py_result)
    {
      PyErr_Print();
      return Value();
    }

    Value tmp(_types.pyobj_to_shcore_value(py_result));
    Py_XDECREF(py_result);
    return tmp;
  }

  Value Python_context::execute_interactive(const std::string &code, bool &r_continued) BOOST_NOEXCEPT_OR_NOTHROW
  {
    Value retvalue;

    r_continued = false;

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

     Continued Lines:
     ----------------

     Because of how the Python interpreter works, it's not possible to
     use the same multiline handling as the official interpreter. We emulate it
     by catching EOF parse errors and accumulate lines until it succeeds.
     */

    PyObject *orig_hook = PySys_GetObject((char*)"displayhook");
    Py_INCREF(orig_hook);

    PySys_SetObject((char*)"displayhook", PyDict_GetItemString(PyModule_GetDict(_shell_module), (char*)"interactivehook"));

    PyObject *py_result = PyRun_String(code.c_str(), Py_single_input, _globals, _globals);

    r_continued = false;

    PySys_SetObject((char*)"displayhook", orig_hook);
    Py_DECREF(orig_hook);
    if (!py_result)
    {
      PyObject *exc, *value, *tb;
      // check whether this is a SyntaxError: unexpected EOF while parsing
      // that would indicate that the command is not complete and needs more input
      // til it's completed
      PyErr_Fetch(&exc, &value, &tb);
      if (PyErr_GivenExceptionMatches(exc, PyExc_SyntaxError))
      {
        const char *msg;
        PyObject *obj;
        if (PyArg_ParseTuple(value, "sO", &msg, &obj)
           && (strncmp(msg, "unexpected EOF while parsing", strlen("unexpected EOF while parsing")) == 0 ||
              strncmp(msg, "EOF while scanning triple-quoted string literal", strlen("EOF while scanning triple-quoted string literal")) == 0))
          r_continued = true;
      }
      PyErr_Restore(exc, value, tb);
      if (r_continued)
        PyErr_Clear();
      else
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

  void Python_context::set_global_item(const std::string &global_name, const std::string &item_name, const Value &value)
  {
    PyObject *py_global;

    py_global = PyDict_GetItemString(_globals, global_name.c_str());
    if (!py_global)
    {
      // TODO: log error
      PyErr_Print();
    }
    else
      PyModule_AddObject(py_global, item_name.c_str(), _types.shcore_value_to_pyobj(value));
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

  void Python_context::set_python_error(const shcore::Exception &exc, const std::string &location)
  {
    std::string error_message;

    std::string type = exc.error()->get_string("type", "");
    std::string message = exc.error()->get_string("message", "");
    int64_t code = exc.error()->get_int("code", -1);
    std::string error_location = exc.error()->get_string("location", "");

    if (error_location.empty())
      error_location = location;

    if (!message.empty())
    {
      if (!type.empty())
        error_message += type;

      if (code != -1)
        error_message += (boost::format(" (%1%)") % code).str();

      if (!error_message.empty())
        error_message += ": ";

      error_message += message;

      if (!error_location.empty())
        error_message += " at " + error_location;

      error_message += "\n";
    }

    PyErr_SetString(PyExc_SystemError, (error_location.empty() ? error_message : error_location + ": " + error_message).c_str());
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

  AutoPyObject Python_context::get_shell_indexed_object_class()
  {
    return _shell_indexed_object_class;
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

  PyObject *Python_context::shell_prompt(PyObject *UNUSED(self), PyObject *args)
  {
    PyObject *ret_val = NULL;
    Python_context *ctx;

    if (!(ctx = Python_context::get_and_check()))
      return NULL;

    PyObject *pyPrompt;
    PyObject *pyOptions;
    try
    {
      if (PyArg_ParseTuple(args, "S|O", &pyPrompt, &pyOptions))
      {
        Py_ssize_t count = PyTuple_Size(args);
        if (count < 1 || count > 2)
          throw std::runtime_error("Invalid number of parameters");
        else
        {
          // First parameters is a string object
          std::string prompt = ctx->pyobj_to_shcore_value(pyPrompt).as_string();

          shcore::Value::Map_type_ref options_map;
          if (count == 2)
          {
            Value options = ctx->pyobj_to_shcore_value(pyOptions);
            if (options.type != shcore::Map)
              throw std::runtime_error("Second parameter must be a dictionary");
            else
              options_map = options.as_map();
          }

          // If there are options, reads them to determine how to proceed
          std::string default_value;
          bool password = false;
          bool succeeded = false;

          // Identifies a default value in case en empty string is returned
          // or hte prompt fails
          if (options_map)
          {
            if (options_map->has_key("defaultValue"))
              default_value = options_map->get_string("defaultValue");

            // Identifies if it is normal prompt or password prompt
            if (options_map->has_key("type"))
              password = (options_map->get_string("type") == "password");
          }

          // Performs the actual prompt
          std::string r;
          if (password)
            succeeded = ctx->_delegate->password(ctx->_delegate->user_data, prompt.c_str(), r);
          else
            succeeded = ctx->_delegate->prompt(ctx->_delegate->user_data, prompt.c_str(), r);

          // Uses the default value if needed
          if (!default_value.empty() && (!succeeded || r.empty()))
            r = default_value;

          ret_val = ctx->shcore_value_to_pyobj(Value(r));
        }
      }
    }
    catch (std::exception &e)
    {
      Python_context::set_python_error(PyExc_SystemError, e.what());
    }

    return ret_val;
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
    { "prompt", &Python_context::shell_prompt, METH_VARARGS,
    "Prompts input to the user." },
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

  PyObject *Python_context::get_constant(PyObject *UNUSED(self), PyObject *args, const std::string &group, const std::string& id)
  {
    Python_context *ctx;
    std::string text;

    if (!(ctx = Python_context::get_and_check()))
      return NULL;

    shcore::Argument_list shell_args;

    shell_args.push_back(Value(group));
    shell_args.push_back(Value(id));

    if (args)
    {
      int size = (int)PyTuple_Size(args);
      for (int index = 0; index < size; index++)
        shell_args.push_back(ctx->pyobj_to_shcore_value(PyTuple_GetItem(args, index)));
    }

    PyObject *py_session = NULL;

    try
    {
      shcore::Value session(Object_factory::call_constructor("mysqlx", "Constant", shell_args));
      py_session = ctx->shcore_value_to_pyobj(session);
    }
    catch (shcore::Exception &e)
    {
      set_python_error(e);
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

  PyObject *Python_context::get_object(PyObject *UNUSED(self), PyObject *args, const std::string &module, const std::string &type)
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
      set_python_error(e);
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
    return get_object(self, args, "mysqlx", "XSession");
  }

  PyObject *Python_context::mysqlx_get_node_session(PyObject *self, PyObject *args)
  {
    return get_object(self, args, "mysqlx", "NodeSession");
  }

  PyObject *Python_context::mysqlx_expr(PyObject *self, PyObject *args)
  {
    return get_object(self, args, "mysqlx", "Expression");
  }

  PyObject *Python_context::mysqlx_text(PyObject *self, PyObject *args)
  {
    return get_constant(self, args, "DataTypes", "Text");
  }

  PyObject *Python_context::mysqlx_blob(PyObject *self, PyObject *args)
  {
    return get_constant(self, args, "DataTypes", "Blob");
  }

  PyObject *Python_context::mysqlx_decimal(PyObject *self, PyObject *args)
  {
    return get_constant(self, args, "DataTypes", "Decimal");
  }

  PyObject *Python_context::mysqlx_numeric(PyObject *self, PyObject *args)
  {
    return get_constant(self, args, "DataTypes", "Numeric");
  }

  PyObject *Python_context::mysql_get_classic_session(PyObject *self, PyObject *args)
  {
    return get_object(self, args, "mysql", "ClassicSession");
  }

  static PyMethodDef MysqlxModuleMethods[] =
  {
    { "getSession", &Python_context::mysqlx_get_session, METH_VARARGS,
    "Creates an XSession object." },
    { "getNodeSession", &Python_context::mysqlx_get_node_session, METH_VARARGS,
    "Creates a NodeSession object." },
    { "expr", &Python_context::mysqlx_expr, METH_VARARGS,
    "Creates a Expression object." },
    { "Decimal", &Python_context::mysqlx_decimal, METH_VARARGS,
    "Creates a Numeric data type definition." },
    { "Numeric", &Python_context::mysqlx_numeric, METH_VARARGS,
    "Creates a Numeric data type definition." },
    { "Text", &Python_context::mysqlx_text, METH_VARARGS,
    "Creates a Text data type definition." },
    { "Blob", &Python_context::mysqlx_blob, METH_VARARGS,
    "Creates a Blob data type definition." },
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
    else
    {
      // Data Type Constants
      PyModule_AddObject(module, "TinyInt", get_constant(NULL, NULL, "DataTypes", "TinyInt"));
      PyModule_AddObject(module, "SmallInt", get_constant(NULL, NULL, "DataTypes", "SmallInt"));
      PyModule_AddObject(module, "MediumInt", get_constant(NULL, NULL, "DataTypes", "MediumInt"));
      PyModule_AddObject(module, "Int", get_constant(NULL, NULL, "DataTypes", "Int"));
      PyModule_AddObject(module, "Integer", get_constant(NULL, NULL, "DataTypes", "Integer"));
      PyModule_AddObject(module, "BigInt", get_constant(NULL, NULL, "DataTypes", "BigInt"));
      PyModule_AddObject(module, "Real", get_constant(NULL, NULL, "DataTypes", "Real"));
      PyModule_AddObject(module, "Float", get_constant(NULL, NULL, "DataTypes", "Float"));
      PyModule_AddObject(module, "Double", get_constant(NULL, NULL, "DataTypes", "Double"));
      PyModule_AddObject(module, "Date", get_constant(NULL, NULL, "DataTypes", "Date"));
      PyModule_AddObject(module, "Time", get_constant(NULL, NULL, "DataTypes", "Time"));
      PyModule_AddObject(module, "Timestamp", get_constant(NULL, NULL, "DataTypes", "Timestamp"));
      PyModule_AddObject(module, "DateTime", get_constant(NULL, NULL, "DataTypes", "DateTime"));
      PyModule_AddObject(module, "Year", get_constant(NULL, NULL, "DataTypes", "Year"));
      PyModule_AddObject(module, "Bit", get_constant(NULL, NULL, "DataTypes", "Bit"));

      // Index Type Constants
      PyModule_AddObject(module, "IndexUnique", get_constant(NULL, NULL, "IndexTypes", "IndexUnique"));
    }

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