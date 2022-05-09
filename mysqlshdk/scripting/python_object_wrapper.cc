/*
 * Copyright (c) 2015, 2022, Oracle and/or its affiliates.
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

void translate_python_exception(const std::string &context = "") {
  try {
    throw;
  } catch (const mysqlshdk::db::Error &e) {
    Python_context::set_shell_error(e);
  } catch (const shcore::Exception &e) {
    Python_context::set_python_error(e, context);
  } catch (const shcore::Error &e) {
    if (context.empty())
      Python_context::set_shell_error(e);
    else
      Python_context::set_shell_error(
          shcore::Error((context + ": " + e.what()).c_str(), e.code()));
  } catch (const std::exception &exc) {
    Python_context::set_python_error(exc);
  }
}

namespace {
/** Wraps a GRT method as a Python object
 */
struct PyShMethodObject {
  // clang-format off
  PyObject_HEAD
  std::shared_ptr<Cpp_object_bridge> *object{nullptr};
  // clang-format on
  std::string *method{nullptr};
};

struct PyMemberCache {
  std::map<std::string, py::Store> members;
};

/*
 * Wraps a native/bridged C++ object reference as a Python sequence object
 */
struct PyShObjObject {
  // clang-format off
  PyObject_HEAD
  shcore::Object_bridge_ref *object{nullptr};
  // clang-format on
  PyMemberCache *cache{nullptr};
};

struct PyShObjIndexedObject {
  // clang-format off
  PyObject_HEAD
  shcore::Object_bridge_ref *object{nullptr};
  // clang-format on
};

py::Release call_object_method(std::shared_ptr<Cpp_object_bridge> object,
                               const char *method, PyObject *args,
                               PyObject *kwargs) {
  Argument_list arglist;
  Python_context *ctx = nullptr;

  for (Py_ssize_t a = 0; a < PyTuple_Size(args); a++) {
    PyObject *argval = PyTuple_GetItem(args, a);

    try {
      arglist.push_back(py::convert(argval, &ctx));
    } catch (...) {
      char buffer[100];
      snprintf(buffer, sizeof(buffer), "argument #" PY_SIZE_T_FMT, a);
      translate_python_exception(buffer);
      return {};
    }
  }

  auto keyword_args = py::convert(kwargs, &ctx);

  try {
    Value result;
    {
      WillLeavePython lock;
      shcore::Scoped_naming_style lower(shcore::LowerCaseUnderscores);
      result = object->call_advanced(method, arglist, keyword_args.as_map());
    }
    return py::convert(result);
  } catch (...) {
    translate_python_exception();
  }
  return {};
}

void method_dealloc(PyShMethodObject *self) {
  delete std::exchange(self->object, nullptr);
  delete std::exchange(self->method, nullptr);

  PyObject_FREE(self);
}

PyObject *method_call(PyShMethodObject *self, PyObject *args, PyObject *kw) {
  if (!self->method || !self->object) return nullptr;
  return call_object_method(*self->object, self->method->c_str(), args, kw)
      .release();
}

PyObject *method_getattro(PyShMethodObject *self, PyObject *attr_name) {
  if (!self->method || !self->object) return nullptr;

  std::string attrname;
  if (!Python_context::pystring_to_string(attr_name, &attrname)) return nullptr;

  if (auto object = PyObject_GenericGetAttr((PyObject *)self, attr_name);
      object)
    return object;

  if (attrname == "__name__") {
    PyErr_Clear();
    return PyString_FromString(self->method->c_str());
  } else if (attrname == "__qualname__") {
    PyErr_Clear();
    std::string qname = (*self->object)->class_name() + "." + *self->method;
    return PyString_FromString(qname.c_str());
  }

  return nullptr;
}

int method_init(PyShMethodObject *, PyObject *, PyObject *) {
  // it shouldn't be possible for this to be called from Python
  Python_context::set_python_error(
      std::runtime_error{"Type isn't meant to be instantiated directly, in the "
                         "interpreter layer."});
  return -1;
}

#if PY_VERSION_HEX >= 0x03080000 && PY_VERSION_HEX < 0x03090000
#ifdef __clang__
// The tp_print is marked as deprecated, which makes clang unhappy, 'cause it's
// initialized below. Skipping initialization also makes clang unhappy, so we're
// disabling the deprecated declarations warning.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif  // __clang__
#endif  // PY_VERSION_HEX

PyTypeObject PyShMethodObjectType = {
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

    0,                              //  hashfunc tp_hash;
    (ternaryfunc)method_call,       //  ternaryfunc tp_call;
    0,                              //  reprfunc tp_str;
    (getattrofunc)method_getattro,  //  getattrofunc tp_getattro;
    PyObject_GenericSetAttr,        //  setattrofunc tp_setattro;

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
    0,                      //  struct PyMethodDef *tp_methods;
    0,                      //  struct PyMemberDef *tp_members;
    0,                      //  struct PyGetSetDef *tp_getset;
    0,                      //  struct _typeobject *tp_base;
    0,                      //  PyObject *tp_dict;
    0,                      //  descrgetfunc tp_descr_get;
    0,                      //  descrsetfunc tp_descr_set;
    0,                      //  long tp_dictoffset;
    (initproc)method_init,  //  initproc tp_init;
    PyType_GenericAlloc,    //  allocfunc tp_alloc;
    PyType_GenericNew,      //  newfunc tp_new;
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
#if PY_VERSION_HEX < 0x03090000
    0  // tp_print
#endif
#endif
};

#if PY_VERSION_HEX >= 0x03080000 && PY_VERSION_HEX < 0x03090000
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__
#endif  // PY_VERSION_HEX

py::Release wrap_method(const std::shared_ptr<Cpp_object_bridge> &object,
                        const char *method_name) {
  // create a method call object and return it
  PyShMethodObject *method =
      PyObject_New(PyShMethodObject, &PyShMethodObjectType);
  if (!method) return {};
  method->object = new std::shared_ptr<Cpp_object_bridge>(object);
  method->method = new std::string(method_name);
  return py::Release{(PyObject *)method};
}

//----------------------------------------------------------------

int object_init(PyShObjObject *self, PyObject *args, PyObject *) {
  Python_context *ctx = Python_context::get_and_check();
  if (ctx) {
    if (!PyArg_ParseTuple(args, "")) return -1;

    delete std::exchange(self->object, nullptr);
    delete std::exchange(self->cache, nullptr);

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
void object_dealloc(PyShObjObject *self) {
  DEBUG_OBJ_FREE(PythonObjectWrapper, self);

  delete std::exchange(self->object, nullptr);
  delete std::exchange(self->cache, nullptr);

  Py_TYPE(self)->tp_free(self);
}

int object_compare(PyShObjObject *self, PyShObjObject *other) {
  if (self->object->get() == other->object->get()) return 0;

  return 1;
}

PyObject *object_rich_compare(PyShObjObject *self, PyObject *other, int op);

PyObject *object_printable(PyShObjObject *self) {
  Value object((*self->object));
  std::string format = mysqlsh::current_shell_options()->get().wrap_json;

  if (format.find("json") == 0)
    return PyString_FromString(object.json(format == "json").c_str());
  return PyString_FromString(object.descr(true).c_str());
}

PyObject *object_getattro(PyShObjObject *self, PyObject *attr_name) {
  if (auto object = PyObject_GenericGetAttr(reinterpret_cast<PyObject *>(self),
                                            attr_name))
    return object;

  if (!self->object || !self->cache) return nullptr;

  // At this point Python exception is set and we either return it to the
  // user if conversion fails or need to clear it
  std::string attrname;
  if (!Python_context::pystring_to_string(attr_name, &attrname)) return nullptr;

  PyErr_Clear();

  std::shared_ptr<Cpp_object_bridge> cobj(
      std::static_pointer_cast<Cpp_object_bridge>(*self->object));

  shcore::Scoped_naming_style lower(shcore::LowerCaseUnderscores);

  if (cobj->has_method_advanced(attrname))
    return wrap_method(cobj, attrname.c_str()).release();

  shcore::Value member;
  bool error_handled = false;
  try {
    member = cobj->get_member_advanced(attrname);
  } catch (const Exception &exc) {
    if (!exc.is_attribute()) {
      Python_context::set_python_error(exc, "");
      error_handled = true;
    }
  } catch (...) {
    translate_python_exception();
    error_handled = true;
  }

  if (member.type != shcore::Undefined) {
    auto object = py::convert(member);
    self->cache->members[attrname] = py::Store{object.get()};
    return object.release();
  } else if (!error_handled) {
    std::string err = std::string("unknown attribute: ") + attrname;
    Python_context::set_python_error(PyExc_AttributeError, err.c_str());
  }

  return nullptr;
}

int object_setattro(PyShObjObject *self, PyObject *attr_name,
                    PyObject *attr_value) {
  std::string attrname;

  if (Python_context::pystring_to_string(attr_name, &attrname)) {
    std::shared_ptr<Cpp_object_bridge> cobj(
        std::static_pointer_cast<Cpp_object_bridge>(*self->object));
    shcore::Scoped_naming_style lower(shcore::LowerCaseUnderscores);

    if (cobj->has_member_advanced(attrname)) {
      Value value;

      try {
        value = py::convert(attr_value);
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

py::Release call_object_method(std::shared_ptr<shcore::Object_bridge> object,
                               Value method, PyObject *args) {
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
    return {};
  }

  Argument_list r;
  Python_context *ctx = nullptr;

  for (int c = func->signature().size(), i = 0; i < c; i++) {
    PyObject *argval = PyTuple_GetItem(args, i);

    try {
      Value v = py::convert(argval, &ctx);
      r.push_back(v);
    } catch (...) {
      translate_python_exception();
      return {};
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
    return py::convert(result);
  } catch (...) {
    translate_python_exception();
  }

  return {};
}

PyObject *object_callmethod(PyShObjObject *self, PyObject *args) {
  PyObject *method_name;
  std::string method_name_string;
  shcore::Scoped_naming_style lower(shcore::LowerCaseUnderscores);

  if (PyTuple_Size(args) < 1 || !(method_name = PyTuple_GetItem(args, 0)) ||
      !Python_context::pystring_to_string(method_name, &method_name_string)) {
    Python_context::set_python_error(
        PyExc_TypeError, "1st argument must be name of method to call");
    return nullptr;
  }
  std::shared_ptr<Cpp_object_bridge> cobj(
      std::static_pointer_cast<Cpp_object_bridge>(*self->object));

  const Value method = cobj->get_member_advanced(method_name_string);
  if (!method) {
    Python_context::set_python_error(PyExc_TypeError, "invalid method");
    return nullptr;
  }

  /*
   * get_member() can return not only function but also properties which are
   * not functions. We need to validate that it actually returned a function
   */
  try {
    std::shared_ptr<shcore::Function_base> func = method.as_function();
  } catch (const std::exception &exc) {
    Python_context::set_python_error(exc, "member is not a function");
    return nullptr;
  }

  return call_object_method(*self->object, method,
                            PyTuple_GetSlice(args, 1, PyTuple_Size(args)))
      .release();
}

PyObject *object_dir_method(PyShObjObject *self, PyObject *) {
  shcore::Scoped_naming_style lower(shcore::LowerCaseUnderscores);
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
  Py_ssize_t length = object_length(self);

  if (index < 0 || index >= length) {
    Python_context::set_python_error(PyExc_IndexError,
                                     "object index out of range");
    return nullptr;
  }

  try {
    return py::convert(self->object->get()->get_member(index)).release();
  } catch (...) {
    translate_python_exception();
  }
  return nullptr;
}

int object_assign(PyShObjObject *self, Py_ssize_t index, PyObject *value) {
  Py_ssize_t length = object_length(self);

  if (index < 0 || index >= length) {
    Python_context::set_python_error(PyExc_IndexError,
                                     "object index out of range");
    return -1;
  }

  try {
    self->object->get()->set_member(index, py::convert(value));

    return 0;
  } catch (...) {
    translate_python_exception();
  }

  return -1;
}

PyMethodDef PyShObjMethods[] = {
    {"__callmethod__", (PyCFunction)object_callmethod, METH_VARARGS, call_doc},
    {"__dir__", (PyCFunction)object_dir_method, METH_NOARGS, nullptr},
    {NULL, NULL, 0, NULL}};

PySequenceMethods PyShObject_as_sequence = {
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

PyMappingMethods PyShObjMappingMethods = {
    (lenfunc)NULL,                  // PyMappingMethods.mp_length
    (getattrofunc)object_getattro,  // PyMappingMethods.mp_subscript
    (setattrofunc)object_setattro,  // PyMappingMethods.mp_ass_subscript
};

#if PY_VERSION_HEX >= 0x03080000 && PY_VERSION_HEX < 0x03090000
#ifdef __clang__
// The tp_print is marked as deprecated, which makes clang unhappy, 'cause it's
// initialized below. Skipping initialization also makes clang unhappy, so we're
// disabling the deprecated declarations warning.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif  // __clang__
#endif  // PY_VERSION_HEX

PyTypeObject PyShObjObjectType = {
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
    0,  //  PyAsyncMethods *tp_as_async; // void *tp_reserved;
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
#if PY_VERSION_HEX < 0x03090000
    0  // tp_print
#endif
#endif
};

PyTypeObject PyShObjIndexedObjectType = {
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
    0,  //  PyAsyncMethods *tp_as_async; // void *tp_reserved;
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
#if PY_VERSION_HEX < 0x03090000
    0  // tp_print
#endif
#endif
};

#if PY_VERSION_HEX >= 0x03080000 && PY_VERSION_HEX < 0x03090000
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__
#endif  // PY_VERSION_HEX

PyObject *object_rich_compare(PyShObjObject *self, PyObject *other, int op) {
  if (Py_EQ == op) {
    const auto type = Py_TYPE(other);

    if (type == &PyShObjObjectType || type == &PyShObjIndexedObjectType) {
      if (object_compare(self, (PyShObjObject *)other) == 0) {
        Py_RETURN_TRUE;
      }
    }
    Py_RETURN_FALSE;
  } else {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
}

}  // namespace

void Python_context::init_shell_object_type() {
  // Initializes the normal object
  if (PyType_Ready(&PyShObjObjectType) < 0) {
    throw std::runtime_error(
        "Could not initialize Shcore Object type in python");
  }

  Py_INCREF(&PyShObjObjectType);

  auto module = get_shell_python_support_module();

  PyModule_AddObject(module.get(), "Object",
                     reinterpret_cast<PyObject *>(&PyShObjObjectType));

  _shell_object_class =
      py::Store{PyDict_GetItemString(PyModule_GetDict(module.get()), "Object")};

  // Initializes the indexed object
  if (PyType_Ready(&PyShObjIndexedObjectType) < 0) {
    throw std::runtime_error(
        "Could not initialize Shcore Indexed Object type in python");
  }

  Py_INCREF(&PyShObjIndexedObjectType);

  PyModule_AddObject(module.get(), "IndexedObject",
                     reinterpret_cast<PyObject *>(&PyShObjIndexedObjectType));

  _shell_indexed_object_class = py::Store{
      PyDict_GetItemString(PyModule_GetDict(module.get()), "IndexedObject")};
}

py::Release shcore::wrap(const std::shared_ptr<Object_bridge> &object) {
  PyShObjObject *wrapper;

  if (object->is_indexed())
    wrapper = PyObject_New(PyShObjObject, &PyShObjIndexedObjectType);
  else
    wrapper = PyObject_New(PyShObjObject, &PyShObjObjectType);

  DEBUG_OBJ_MALLOC_N(PythonObjectWrapper, wrapper, object->class_name());

  wrapper->object = new Object_bridge_ref(object);
  wrapper->cache = new PyMemberCache();
  return py::Release{reinterpret_cast<PyObject *>(wrapper)};
}

py::Release shcore::wrap(const std::shared_ptr<Cpp_object_bridge> &object,
                         const std::string &method) {
  return wrap_method(object, method.c_str());
}

bool shcore::unwrap(PyObject *value,
                    std::shared_ptr<Object_bridge> &ret_object) {
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return false;

  auto oclass = ctx->get_shell_object_class();
  auto ioclass = ctx->get_shell_indexed_object_class();

  if (PyObject_IsInstance(value, oclass.get()) ||
      PyObject_IsInstance(value, ioclass.get())) {
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
