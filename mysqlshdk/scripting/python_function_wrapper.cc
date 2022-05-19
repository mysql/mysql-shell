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

#include "scripting/python_function_wrapper.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <sstream>
#include <string>
#include "scripting/common.h"
#include "scripting/python_utils.h"
#include "scripting/types_cpp.h"
#include "scripting/types_python.h"

using namespace shcore;

void translate_python_exception(const std::string &context = "");

namespace {
void method_dealloc(PyShFuncObject *self) {
  delete self->func;

  Py_TYPE(self)->tp_free(self);
}

PyObject *method_call(PyShFuncObject *self, PyObject *args, PyObject *) {
  const auto func = *self->func;

  Argument_list r;

  if (args) {
    Python_context *ctx = nullptr;

    for (size_t c = (size_t)PyTuple_Size(args), i = 0; i < c; i++) {
      PyObject *argval = PyTuple_GetItem(args, i);

      try {
        r.push_back(py::convert(argval, &ctx));
      } catch (...) {
        translate_python_exception();
        return nullptr;
      }
    }
  }

  try {
    Value result;

    if (std::dynamic_pointer_cast<shcore::Python_function>(func)) {
      result = func->invoke(r);
    } else {
      WillLeavePython lock;
      result = func->invoke(r);
    }

    return py::convert(result).release();
  } catch (...) {
    translate_python_exception();
  }

  return nullptr;
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

PyTypeObject PyShFuncObjectType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)  // PyObject_VAR_HEAD
    "builtin_function_or_method",  // char *tp_name; /* For printing, in format
                                   // "<module>.<name>" */
    sizeof(PyShFuncObject),
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

}  // namespace

void Python_context::init_shell_function_type() {
  if (PyType_Ready(&PyShFuncObjectType) < 0) {
    throw std::runtime_error(
        "Could not initialize Shcore Function type in python");
  }

  Py_INCREF(&PyShFuncObjectType);

  auto module = get_shell_python_support_module();

  PyModule_AddObject(module.get(), "Function",
                     reinterpret_cast<PyObject *>(&PyShFuncObjectType));

  _shell_function_class = py::Store{
      PyDict_GetItemString(PyModule_GetDict(module.get()), "Function")};
}

py::Release shcore::wrap(std::shared_ptr<Function_base> func) {
  PyShFuncObject *wrapper = PyObject_New(PyShFuncObject, &PyShFuncObjectType);
  wrapper->func = new Function_base_ref(func);
  return py::Release{reinterpret_cast<PyObject *>(wrapper)};
}

bool shcore::unwrap(PyObject *value, std::shared_ptr<Function_base> &ret_func) {
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return false;

  auto fclass = ctx->get_shell_function_class();
  if (PyObject_IsInstance(value, fclass.get())) {
    ret_func = *((PyShFuncObject *)value)->func;
    return true;
  }

  return false;
}
