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

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#include "scripting/python_object_wrapper.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <sstream>
#include <string>
#include "mysqlshdk/include/shellcore/base_shell.h"  // TODO(alfredo) doesn't belong here
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "scripting/python_utils.h"
#include "scripting/types_cpp.h"

#ifndef WIN32
#define PY_SIZE_T_FMT "%zi"
#else
#define PY_SIZE_T_FMT "%Ii"
#endif

using namespace shcore;

DEBUG_OBJ_ENABLE(PythonObjectWrapper);

/** Wraps a GRT method as a Python object
 */
struct PyShMethodObject {
  // clang-format off
  PyObject_HEAD
  std::shared_ptr<Cpp_object_bridge> *object;
  // clang-format on
  std::string *method;
};

void translate_python_exception(const std::string &context = "") {
  try {
    throw;
  } catch (const mysqlshdk::db::Error &e) {
    PyObject *err = PyTuple_New(2);
    PyTuple_SET_ITEM(err, 0, PyInt_FromLong(e.code()));
    PyTuple_SET_ITEM(err, 1, PyString_FromString(e.what()));
    PyErr_SetObject(Python_context::get()->db_error(), err);
    Py_DECREF(err);
  } catch (Exception &e) {
    Python_context::set_python_error(e, context);
  } catch (const std::exception &exc) {
    Python_context::set_python_error(exc);
  }
}

static PyObject *call_object_method(std::shared_ptr<Cpp_object_bridge> object,
                                    const char *method, PyObject *args) {
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return NULL;

  Argument_list arglist;

  for (Py_ssize_t a = 0; a < PyTuple_Size(args); a++) {
    PyObject *argval = PyTuple_GetItem(args, a);

    try {
      arglist.push_back(ctx->pyobj_to_shcore_value(argval));
    } catch (...) {
      char buffer[100];
      snprintf(buffer, sizeof(buffer), "argument #" PY_SIZE_T_FMT, a);
      translate_python_exception(buffer);
      return NULL;
    }
  }

  try {
    WillLeavePython lock;
    shcore::Scoped_naming_style lower(shcore::LowerCaseUnderscores);
    return ctx->shcore_value_to_pyobj(object->call_advanced(method, arglist));
  } catch (...) {
    translate_python_exception();
    return NULL;
  }
  return NULL;
}

static void method_dealloc(PyShMethodObject *self) {
  delete self->object;
  delete self->method;

  PyObject_FREE(self);
}

static PyObject *method_call(PyShMethodObject *self, PyObject *args,
                             PyObject *UNUSED(kw)) {
  return call_object_method(*self->object, self->method->c_str(), args);
}

static PyTypeObject PyShMethodObjectType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)  // PyObject_VAR_HEAD
    "builtin_function_or_method",  // char *tp_name; /* For printing, in format
                                   // "<module>.<name>" */
    sizeof(PyShMethodObject),
    0,  // int tp_basicsize, tp_itemsize; /* For allocation */

    /* Methods to implement standard operations */

    (destructor)method_dealloc,  //  destructor tp_dealloc;
    0,                           //  printfunc tp_print;
    0,                           //  getattrfunc tp_getattr;
    0,                           //  setattrfunc tp_setattr;
    0,  //(cmpfunc)object_compare, //  cmpfunc tp_compare;
    0,  //(reprfunc)object_repr,//  reprfunc tp_repr;

    /* Method suites for standard classes */

    0,  //  PyNumberMethods *tp_as_number;
    0,  //  PySequenceMethods *tp_as_sequence;
    0,  //  PyMappingMethods *tp_as_mapping;

    /* More standard operations (here for binary compatibility) */

    0,                         //  hashfunc tp_hash;
    (ternaryfunc)method_call,  //  ternaryfunc tp_call;
    0,                         //  reprfunc tp_str;
    PyObject_GenericGetAttr,   //  getattrofunc tp_getattro;
    PyObject_GenericSetAttr,   //  setattrofunc tp_setattro;

    /* Functions to access object as input/output buffer */
    0,  //  PyBufferProcs *tp_as_buffer;

    /* Flags to define presence of optional/expanded features */
    Py_TPFLAGS_DEFAULT,  //  long tp_flags;

    0,  //  char *tp_doc; /* Documentation string */

    /* Assigned meaning in release 2.0 */
    /* call function for all accessible objects */
    0,  //  traverseproc tp_traverse;

    /* delete references to contained objects */
    0,  //  inquiry tp_clear;

    /* Assigned meaning in release 2.1 */
    /* rich comparisons */
    0,  //  richcmpfunc tp_richcompare;

    /* weak reference enabler */
    0,  //  long tp_weaklistoffset;

    /* Added in release 2.2 */
    /* Iterators */
    0,  //  getiterfunc tp_iter;
    0,  //  iternextfunc tp_iternext;

    /* Attribute descriptor and subclassing stuff */
    0,                    //  struct PyMethodDef *tp_methods;
    0,                    //  struct PyMemberDef *tp_members;
    0,                    //  struct PyGetSetDef *tp_getset;
    0,                    //  struct _typeobject *tp_base;
    0,                    //  PyObject *tp_dict;
    0,                    //  descrgetfunc tp_descr_get;
    0,                    //  descrsetfunc tp_descr_set;
    0,                    //  long tp_dictoffset;
    0,                    //  initproc tp_init;
    PyType_GenericAlloc,  //  allocfunc tp_alloc;
    PyType_GenericNew,    //  newfunc tp_new;
    0,  //  freefunc tp_free; /* Low-level free-memory routine */
    0,  //  inquiry tp_is_gc; /* For PyObject_IS_GC */
    0,  //  PyObject *tp_bases;
    0,  //  PyObject *tp_mro; /* method resolution order */
    0,  //  PyObject *tp_cache;
    0,  //  PyObject *tp_subclasses;
    0,  //  PyObject *tp_weaklist;
    0   // tp_del
#if PY_VERSION_HEX >= 0x02060000
    ,
    0  // tp_version_tag
#endif
#if PY_VERSION_HEX >= 0x03040000
    ,
    0  // tp_finalize
#endif
#if PY_VERSION_HEX >= 0x03080000
    ,
    0,  // tp_vectorcall
    0   // tp_print
#endif
};

static PyObject *wrap_method(const std::shared_ptr<Cpp_object_bridge> &object,
                             const char *method_name) {
  // create a method call object and return it
  PyShMethodObject *method =
      PyObject_New(PyShMethodObject, &PyShMethodObjectType);
  if (!method) return NULL;
  method->object = new std::shared_ptr<Cpp_object_bridge>(object);
  method->method = new std::string(method_name);
  return (PyObject *)method;
}

//----------------------------------------------------------------

static int object_init(PyShObjObject *self, PyObject *args,
                       PyObject *UNUSED(kwds)) {
  Python_context *ctx = Python_context::get_and_check();
  if (ctx) {
    if (!PyArg_ParseTuple(args, "")) return -1;

    delete self->object;
    delete self->cache;

    DEBUG_OBJ_MALLOC(PythonObjectWrapper, self);

    try {
      self->object = new Object_bridge_ref();
      self->cache = new PyMemberCache();
    } catch (const std::exception &exc) {
      Python_context::set_python_error(exc);
      return -1;
    }
    return 0;
  }
  return -1;
}

// session.schemas.sakila.city.select().execute()
static void object_dealloc(PyShObjObject *self) {
  DEBUG_OBJ_FREE(PythonObjectWrapper, self);

  delete self->object;
  delete self->cache;

  Py_TYPE(self)->tp_free(self);
}

static int object_compare(PyShObjObject *self, PyShObjObject *other) {
  if (self->object->get() == other->object->get()) return 0;

  return 1;
}

static PyObject *object_rich_compare(PyShObjObject *self, PyObject *other,
                                     int op);

static PyObject *object_printable(PyShObjObject *self) {
  PyObject *ret_val;

  Value object((*self->object));
  std::string format = mysqlsh::current_shell_options()->get().wrap_json;

  if (format.find("json") == 0)
    ret_val = PyString_FromString(object.json(format == "json").c_str());
  else
    ret_val = PyString_FromString(object.descr(true).c_str());

  return ret_val;
}

static PyObject *object_getattro(PyShObjObject *self, PyObject *attr_name) {
  std::string attrname;

  if (Python_context::pystring_to_string(attr_name, &attrname)) {
    PyObject *object;
    if ((object = PyObject_GenericGetAttr((PyObject *)self, attr_name)))
      return object;
    PyErr_Clear();

    std::shared_ptr<Cpp_object_bridge> cobj(
        std::static_pointer_cast<Cpp_object_bridge>(*self->object));

    shcore::Scoped_naming_style lower(shcore::LowerCaseUnderscores);

    if (cobj->has_method_advanced(attrname)) {
      return wrap_method(cobj, attrname.c_str());
    }

    shcore::Value member;
    bool error_handled = false;
    try {
      member = cobj->get_member_advanced(attrname);
    } catch (Exception &exc) {
      if (!exc.is_attribute()) {
        Python_context::set_python_error(exc, "");
        error_handled = true;
      }
    } catch (...) {
      translate_python_exception();
      error_handled = true;
    }

    if (member.type != shcore::Undefined) {
      Python_context *ctx = Python_context::get_and_check();
      if (!ctx) return NULL;
      object = ctx->shcore_value_to_pyobj(member);
      self->cache->members[attrname] = object;
      return object;
    } else if (!error_handled) {
      std::string err = std::string("unknown attribute: ") + attrname;
      Python_context::set_python_error(PyExc_IndexError, err.c_str());
    }
  }
  return NULL;
}

static int object_setattro(PyShObjObject *self, PyObject *attr_name,
                           PyObject *attr_value) {
  std::string attrname;

  if (Python_context::pystring_to_string(attr_name, &attrname)) {
    std::shared_ptr<Cpp_object_bridge> cobj(
        std::static_pointer_cast<Cpp_object_bridge>(*self->object));
    shcore::Scoped_naming_style lower(shcore::LowerCaseUnderscores);

    if (cobj->has_member_advanced(attrname)) {
      Python_context *ctx = Python_context::get_and_check();
      if (!ctx) return -1;

      Value value;

      try {
        value = ctx->pyobj_to_shcore_value(attr_value);
      } catch (...) {
        translate_python_exception();
        return -1;
      }
      try {
        cobj->set_member_advanced(attrname, value);
      } catch (...) {
        translate_python_exception();
        return -1;
      }
      return 0;
    }

    PyErr_Format(PyExc_AttributeError, "unknown attribute '%s'",
                 attrname.c_str());
  }
  return -1;
}

static PyObject *call_object_method(
    std::shared_ptr<shcore::Object_bridge> object, Value method,
    PyObject *args) {
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return NULL;
  shcore::Scoped_naming_style lower(shcore::LowerCaseUnderscores);

  std::shared_ptr<shcore::Function_base> func = method.as_function();

  if ((int)func->signature().size() != PyTuple_Size(args)) {
    std::shared_ptr<Cpp_function> cfunc(
        std::static_pointer_cast<Cpp_function>(func));
    std::stringstream err;
    err << cfunc->name(shcore::LowerCaseUnderscores).c_str() << "()"
        << " takes " << (int)func->signature().size() << " arguments ("
        << (int)PyTuple_Size(args) << " given)";

    Python_context::set_python_error(PyExc_TypeError, err.str().c_str());
    return NULL;
  }

  Argument_list r;

  for (int c = func->signature().size(), i = 0; i < c; i++) {
    PyObject *argval = PyTuple_GetItem(args, i);

    try {
      Value v = ctx->pyobj_to_shcore_value(argval);
      r.push_back(v);
    } catch (...) {
      translate_python_exception();
      return NULL;
    }
  }

  try {
    Value result;
    {
      WillLeavePython lock;

      std::shared_ptr<Cpp_object_bridge> cobj(
          std::static_pointer_cast<Cpp_object_bridge>(object));
      std::shared_ptr<Cpp_function> cfunc(
          std::static_pointer_cast<Cpp_function>(func));

      result =
          cobj->call_advanced(cfunc->name(shcore::LowerCaseUnderscores), r);
    }
    return ctx->shcore_value_to_pyobj(result);
  } catch (...) {
    translate_python_exception();
    return NULL;
  }

  return NULL;
}

static PyObject *object_callmethod(PyShObjObject *self, PyObject *args) {
  PyObject *method_name;
  std::string method_name_string;
  shcore::Scoped_naming_style lower(shcore::LowerCaseUnderscores);

  if (PyTuple_Size(args) < 1 || !(method_name = PyTuple_GetItem(args, 0)) ||
      !Python_context::pystring_to_string(method_name, &method_name_string)) {
    Python_context::set_python_error(
        PyExc_TypeError, "1st argument must be name of method to call");
    return NULL;
  }
  std::shared_ptr<Cpp_object_bridge> cobj(
      std::static_pointer_cast<Cpp_object_bridge>(*self->object));

  const Value method = cobj->get_member_advanced(method_name_string);
  if (!method) {
    Python_context::set_python_error(PyExc_TypeError, "invalid method");
    return NULL;
  }

  /*
   * get_member() can return not only function but also properties which are not
   * functions. We need to validate that it actually returned a function
   */
  try {
    std::shared_ptr<shcore::Function_base> func = method.as_function();
  } catch (const std::exception &exc) {
    Python_context::set_python_error(exc, "member is not a function");
    return NULL;
  }

  return call_object_method(*self->object, method,
                            PyTuple_GetSlice(args, 1, PyTuple_Size(args)));
}

static PyObject *object_dir_method(PyShObjObject *self, PyObject *) {
  const auto cobj = std::static_pointer_cast<Cpp_object_bridge>(*self->object);
  const auto members = cobj->get_members();
  std::set<std::string> unique_names(members.begin(), members.end());

  PyObject *list = PyList_New(unique_names.size());

  int i = 0;
  for (const auto &m : unique_names) {
    PyList_SET_ITEM(list, i++, PyString_FromString(m.c_str()));
  }

  return list;
}

PyDoc_STRVAR(PyShObjDoc,
             "Object(shcoreclass) -> Shcore Object\n\
\n\
Creates a new instance of a Shcore object. shcoreclass specifies the name of\n\
the shcore class of the object.");

PyDoc_STRVAR(call_doc, "callmethod(method_name, ...) -> value");

Py_ssize_t object_length(PyShObjObject *self) {
  return self->object->get()->get_member("length").as_uint();
}

PyObject *object_item(PyShObjObject *self, Py_ssize_t index) {
  Python_context *ctx;

  if (!(ctx = Python_context::get_and_check())) return NULL;

  Py_ssize_t length = object_length(self);

  if (index < 0 || index >= length) {
    Python_context::set_python_error(PyExc_IndexError,
                                     "object index out of range");
    return NULL;
  }

  try {
    return ctx->shcore_value_to_pyobj(self->object->get()->get_member(index));
  } catch (...) {
    translate_python_exception();
    return NULL;
  }
}

int object_assign(PyShObjObject *self, Py_ssize_t index, PyObject *value) {
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return -1;

  Py_ssize_t length = object_length(self);

  if (index < 0 || index >= length) {
    Python_context::set_python_error(PyExc_IndexError,
                                     "object index out of range");
    return -1;
  }

  try {
    self->object->get()->set_member(index, ctx->pyobj_to_shcore_value(value));

    return 0;
  } catch (...) {
    translate_python_exception();
  }

  return -1;
}

static PyMethodDef PyShObjMethods[] = {
    {"__callmethod__", (PyCFunction)object_callmethod, METH_VARARGS, call_doc},
    {"__dir__", (PyCFunction)object_dir_method, METH_NOARGS, nullptr},
    {NULL, NULL, 0, NULL}};

static PySequenceMethods PyShObject_as_sequence = {
    (lenfunc)object_length,     // lenfunc sq_length;
    0,                          // binaryfunc sq_concat;
    0,                          // ssizeargfunc sq_repeat;
    (ssizeargfunc)object_item,  // ssizeargfunc sq_item;
    0,  // (ssizessizeargfunc)list_slice, // ssizessizeargfunc sq_slice;
    (ssizeobjargproc)object_assign,  // ssizeobjargproc sq_ass_item;
    0,  // (ssizessizeobjargproc)list_assign_slice,// ssizessizeobjargproc
        // sq_ass_slice;
    0,  // objobjproc sq_contains;
    0,  // binaryfunc sq_inplace_concat;
    0   // ssizeargfunc sq_inplace_repeat;
};

static PyMappingMethods PyShObjMappingMethods = {
    (lenfunc)NULL,                  // PyMappingMethods.mp_length
    (getattrofunc)object_getattro,  // PyMappingMethods.mp_subscript
    (setattrofunc)object_setattro,  // PyMappingMethods.mp_ass_subscript
};

static PyTypeObject PyShObjObjectType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)  // PyObject_VAR_HEAD
    "shell.Object",  // char *tp_name; /* For printing, in format
                     // "<module>.<name>" */
    sizeof(PyShObjObject),
    0,  // int tp_basicsize, tp_itemsize; /* For allocation */

    /* Methods to implement standard operations */

    (destructor)object_dealloc,  // destructor tp_dealloc;
    0,                           // printfunc tp_print;
    0,                           // getattrfunc tp_getattr;
    0,                           // setattrfunc tp_setattr;
#ifdef IS_PY3K
    0,  //  PyAsyncMethods *tp_as_async; // void *tp_reserved;
#else
    (cmpfunc)object_compare,  //  cmpfunc tp_compare;
#endif
    0,  // (reprfunc)dict_repr, // reprfunc tp_repr;

    /* Method suites for standard classes */

    0,                       // PyNumberMethods *tp_as_number;
    0,                       // PySequenceMethods *tp_as_sequence;
    &PyShObjMappingMethods,  //  PyMappingMethods *tp_as_mapping;

    /* More standard operations (here for binary compatibility) */

    0,                              //  hashfunc tp_hash;
    0,                              // ternaryfunc tp_call;
    (reprfunc)object_printable,     // reprfunc tp_str;
    (getattrofunc)object_getattro,  // getattrofunc tp_getattro;
    (setattrofunc)object_setattro,  //  setattrofunc tp_setattro;

    /* Functions to access object as input/output buffer */
    0,  // PyBufferProcs *tp_as_buffer;

    /* Flags to define presence of optional/expanded features */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  //  long tp_flags;

    PyShObjDoc,  // char *tp_doc; /* Documentation string */

    /* Assigned meaning in release 2.0 */
    /* call function for all accessible objects */
    0,  // traverseproc tp_traverse;

    /* delete references to contained objects */
    0,  // inquiry tp_clear;

    /* Assigned meaning in release 2.1 */
    /* rich comparisons */
    (richcmpfunc)object_rich_compare,  // richcmpfunc tp_richcompare;

    /* weak reference enabler */
    0,  // long tp_weakdictoffset;

    /* Added in release 2.2 */
    /* Iterators */
    0,  // getiterfunc tp_iter;
    0,  // iternextfunc tp_iternext;

    /* Attribute descriptor and subclassing stuff */
    PyShObjMethods,         // struct PyMethodDef *tp_methods;
    0,                      // struct PyMemberDef *tp_members;
    0,                      //  struct PyGetSetDef *tp_getset;
    0,                      // struct _typeobject *tp_base;
    0,                      // PyObject *tp_dict;
    0,                      // descrgetfunc tp_descr_get;
    0,                      // descrsetfunc tp_descr_set;
    0,                      // long tp_dictoffset;
    (initproc)object_init,  // initproc tp_init;
    PyType_GenericAlloc,    // allocfunc tp_alloc;
    PyType_GenericNew,      // newfunc tp_new;
    0,  // freefunc tp_free; /* Low-level free-memory routine */
    0,  // inquiry tp_is_gc; /* For PyObject_IS_GC */
    0,  // PyObject *tp_bases;
    0,  // PyObject *tp_mro; /* method resolution order */
    0,  // PyObject *tp_cache;
    0,  // PyObject *tp_subclasses;
    0,  // PyObject *tp_weakdict;
    0   // tp_del
#if PY_VERSION_HEX >= 0x02060000
    ,
    0  // tp_version_tag
#endif
#if PY_VERSION_HEX >= 0x03040000
    ,
    0  // tp_finalize
#endif
#if PY_VERSION_HEX >= 0x03080000
    ,
    0,  // tp_vectorcall
    0   // tp_print
#endif
};

static PyTypeObject PyShObjIndexedObjectType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)  // PyObject_VAR_HEAD
    "shell.Object",  // char *tp_name; /* For printing, in format
                     // "<module>.<name>" */
    sizeof(PyShObjObject),
    0,  // int tp_basicsize, tp_itemsize; /* For allocation */

    /* Methods to implement standard operations */

    (destructor)object_dealloc,  // destructor tp_dealloc;
    0,                           // printfunc tp_print;
    0,                           // getattrfunc tp_getattr;
    0,                           // setattrfunc tp_setattr;
#ifdef IS_PY3K
    0,  //  PyAsyncMethods *tp_as_async; // void *tp_reserved;
#else
    (cmpfunc)object_compare,  //  cmpfunc tp_compare;
#endif
    0,  // (reprfunc)dict_repr, // reprfunc tp_repr;

    /* Method suites for standard classes */

    0,                        // PyNumberMethods *tp_as_number;
    &PyShObject_as_sequence,  // PySequenceMethods *tp_as_sequence;
    0,                        //  PyMappingMethods *tp_as_mapping;

    /* More standard operations (here for binary compatibility) */

    0,                              //  hashfunc tp_hash;
    0,                              // ternaryfunc tp_call;
    (reprfunc)object_printable,     // reprfunc tp_str;
    (getattrofunc)object_getattro,  // getattrofunc tp_getattro;
    (setattrofunc)object_setattro,  //  setattrofunc tp_setattro;

    /* Functions to access object as input/output buffer */
    0,  // PyBufferProcs *tp_as_buffer;

    /* Flags to define presence of optional/expanded features */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  //  long tp_flags;

    PyShObjDoc,  // char *tp_doc; /* Documentation string */

    /* Assigned meaning in release 2.0 */
    /* call function for all accessible objects */
    0,  // traverseproc tp_traverse;

    /* delete references to contained objects */
    0,  // inquiry tp_clear;

    /* Assigned meaning in release 2.1 */
    /* rich comparisons */
    (richcmpfunc)object_rich_compare,  // richcmpfunc tp_richcompare;

    /* weak reference enabler */
    0,  // long tp_weakdictoffset;

    /* Added in release 2.2 */
    /* Iterators */
    0,  // getiterfunc tp_iter;
    0,  // iternextfunc tp_iternext;

    /* Attribute descriptor and subclassing stuff */
    PyShObjMethods,         // struct PyMethodDef *tp_methods;
    0,                      // struct PyMemberDef *tp_members;
    0,                      //  struct PyGetSetDef *tp_getset;
    0,                      // struct _typeobject *tp_base;
    0,                      // PyObject *tp_dict;
    0,                      // descrgetfunc tp_descr_get;
    0,                      // descrsetfunc tp_descr_set;
    0,                      // long tp_dictoffset;
    (initproc)object_init,  // initproc tp_init;
    PyType_GenericAlloc,    // allocfunc tp_alloc;
    PyType_GenericNew,      // newfunc tp_new;
    0,  // freefunc tp_free; /* Low-level free-memory routine */
    0,  // inquiry tp_is_gc; /* For PyObject_IS_GC */
    0,  // PyObject *tp_bases;
    0,  // PyObject *tp_mro; /* method resolution order */
    0,  // PyObject *tp_cache;
    0,  // PyObject *tp_subclasses;
    0,  // PyObject *tp_weakdict;
    0   // tp_del
#if PY_VERSION_HEX >= 0x02060000
    ,
    0  // tp_version_tag
#endif
#if PY_VERSION_HEX >= 0x03040000
    ,
    0  // tp_finalize
#endif
#if PY_VERSION_HEX >= 0x03080000
    ,
    0,  // tp_vectorcall
    0   // tp_print
#endif
};

static PyObject *object_rich_compare(PyShObjObject *self, PyObject *other,
                                     int op) {
  if (Py_EQ == op) {
    const auto type = Py_TYPE(other);

    if (type == &PyShObjObjectType || type == &PyShObjIndexedObjectType) {
      if (object_compare(self, (PyShObjObject *)other) == 0) {
        Py_RETURN_TRUE;
      }
    }
    Py_RETURN_FALSE;
  } else {
    return Py_INCREF(Py_NotImplemented), Py_NotImplemented;
  }
}

void Python_context::init_shell_object_type() {
  // Initializes the normal object
  if (PyType_Ready(&PyShObjObjectType) < 0) {
    throw std::runtime_error(
        "Could not initialize Shcore Object type in python");
  }

  Py_INCREF(&PyShObjObjectType);
  PyModule_AddObject(get_shell_python_support_module(), "Object",
                     reinterpret_cast<PyObject *>(&PyShObjObjectType));

  _shell_object_class = PyDict_GetItemString(
      PyModule_GetDict(get_shell_python_support_module()), "Object");

  // Initializes the indexed object
  if (PyType_Ready(&PyShObjIndexedObjectType) < 0) {
    throw std::runtime_error(
        "Could not initialize Shcore Indexed Object type in python");
  }

  Py_INCREF(&PyShObjIndexedObjectType);
  PyModule_AddObject(get_shell_python_support_module(), "IndexedObject",
                     reinterpret_cast<PyObject *>(&PyShObjIndexedObjectType));

  _shell_indexed_object_class = PyDict_GetItemString(
      PyModule_GetDict(get_shell_python_support_module()), "IndexedObject");
}

PyObject *shcore::wrap(const std::shared_ptr<Object_bridge> &object) {
  PyShObjObject *wrapper;

  if (object->is_indexed())
    wrapper = PyObject_New(PyShObjObject, &PyShObjIndexedObjectType);
  else
    wrapper = PyObject_New(PyShObjObject, &PyShObjObjectType);

  DEBUG_OBJ_MALLOC_N(PythonObjectWrapper, wrapper, object->class_name());

  wrapper->object = new Object_bridge_ref(object);
  wrapper->cache = new PyMemberCache();
  return reinterpret_cast<PyObject *>(wrapper);
}

PyObject *shcore::wrap(const std::shared_ptr<Cpp_object_bridge> &object,
                       const std::string &method) {
  return wrap_method(object, method.c_str());
}

bool shcore::unwrap(PyObject *value,
                    std::shared_ptr<Object_bridge> &ret_object) {
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return false;

  if (PyObject_IsInstance(value, ctx->get_shell_object_class()) ||
      PyObject_IsInstance(value, ctx->get_shell_indexed_object_class())) {
    ret_object = *((PyShObjObject *)value)->object;
    return true;
  }
  return false;
}

bool shcore::unwrap_method(PyObject *value,
                           std::shared_ptr<Function_base> *method) {
  if (PyObject_TypeCheck(value, &PyShMethodObjectType)) {
    const auto o = reinterpret_cast<PyShMethodObject *>(value);
    const auto m = o->object->get()->get_member_advanced(*o->method);

    if (shcore::Value_type::Function != m.type) {
      return false;
    }

    *method = m.as_function();

    return true;
  }

  return false;
}
