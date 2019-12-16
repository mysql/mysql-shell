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

#include "scripting/python_array_wrapper.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <algorithm>
#include <iterator>

using namespace shcore;

static int list_init(PyShListObject *self, PyObject *args, PyObject *kwds) {
  PyObject *valueptr = NULL;
  static const char *kwlist[] = {"__valueptr__", 0};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", (char **)kwlist,
                                   &valueptr))
    return -1;

  delete self->array;

  if (valueptr) {
    try {
#ifdef IS_PY3K
      self->array->reset(static_cast<Value::Array_type *>(
          PyCapsule_GetPointer(valueptr, NULL)));
#else
      self->array->reset(
          static_cast<Value::Array_type *>(PyCObject_AsVoidPtr(valueptr)));
#endif
    } catch (const std::exception &exc) {
      Python_context::set_python_error(exc);
      return -1;
    }
  } else {
    try {
      self->array = new Value::Array_type_ref();
    } catch (const std::exception &exc) {
      Python_context::set_python_error(exc);
      return -1;
    }
  }
  return 0;
}

void list_dealloc(PyShListObject *self) {
  delete self->array;

  Py_TYPE(self)->tp_free(self);
}

Py_ssize_t list_length(PyShListObject *self) {
  return self->array->get()->size();
}

PyObject *list_item(PyShListObject *self, Py_ssize_t index) {
  Python_context *ctx;

  if (!(ctx = Python_context::get_and_check())) return NULL;

  if (index < 0 || index >= (int)self->array->get()->size()) {
    Python_context::set_python_error(PyExc_IndexError,
                                     "list index out of range");
    return NULL;
  }

  try {
    return ctx->shcore_value_to_pyobj(self->array->get()->at(index));
  } catch (const std::exception &exc) {
    Python_context::set_python_error(PyExc_RuntimeError, exc.what());
    return NULL;
  }
}

int list_assign(PyShListObject *self, Py_ssize_t index, PyObject *value) {
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return -1;

  if (index < 0 || index >= (int)self->array->get()->size()) {
    Python_context::set_python_error(PyExc_IndexError,
                                     "list index out of range");
    return -1;
  }

  try {
    shcore::Value::Array_type *array;
    array = self->array->get();

    if (value == NULL)
      array->erase(array->begin() + index);
    else
      array->insert(array->begin() + index, ctx->pyobj_to_shcore_value(value));
    return 0;
  } catch (const std::exception &exc) {
    Python_context::set_python_error(PyExc_RuntimeError, exc.what());
  }

  return -1;
}

int list_contains(PyShListObject *self, PyObject *value) {
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return -1;

  try {
    shcore::Value::Array_type *array;
    array = self->array->get();

    if (std::find(array->begin(), array->end(),
                  ctx->pyobj_to_shcore_value(value)) != array->end())
      return 1;
  } catch (...) {
  }
  return 0;
}

PyObject *list_inplace_concat(PyShListObject *self, PyObject *other) {
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return NULL;

  other = PySequence_Fast(other, "argument to += must be a sequence");
  if (!other) return NULL;

  shcore::Value::Array_type *array;
  array = self->array->get();

  shcore::Value::Array_type *array_other =
      new Value::Array_type(ctx->pyobj_to_shcore_value(other));

  std::copy(array_other->begin(), array_other->end(),
            std::back_inserter(*array));

  Py_INCREF(self);
  return (PyObject *)self;
}

PyObject *list_repr(PyShListObject *self) {
  return PyString_FromString(Value(*self->array).repr().c_str());
}

PyObject *list_str(PyShListObject *self) {
  return PyString_FromString(Value(*self->array).descr().c_str());
}

PyObject *list_append(PyShListObject *self, PyObject *v) {
  if (!v) {
    Python_context::set_python_error(PyExc_ValueError, "missing argument");
    return NULL;
  }

  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return NULL;

  try {
    self->array->get()->push_back(ctx->pyobj_to_shcore_value(v));
    Py_RETURN_NONE;
  } catch (const std::exception &exc) {
    Python_context::set_python_error(exc);
    return NULL;
  }
  return NULL;
}

PyObject *list_insert(PyShListObject *self, PyObject *args) {
  int i;
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return NULL;

  PyObject *value;
  if (!PyArg_ParseTuple(args, "iO:insert", &i, &value)) return NULL;

  try {
    shcore::Value::Array_type *array;
    array = self->array->get();

    array->insert(array->begin() + i, ctx->pyobj_to_shcore_value(value));
    Py_RETURN_NONE;
  } catch (const std::exception &exc) {
    Python_context::set_python_error(exc);
    return NULL;
  }
  return NULL;
}

PyObject *list_remove(PyShListObject *self, PyObject *v) {
  if (!v) {
    Python_context::set_python_error(PyExc_ValueError, "missing argument");
    return NULL;
  }

  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return NULL;

  try {
    shcore::Value::Array_type *array;
    array = self->array->get();

    std::remove(array->begin(), array->end(), ctx->pyobj_to_shcore_value(v));
    Py_RETURN_NONE;
  } catch (const std::exception &exc) {
    Python_context::set_python_error(exc);
    return NULL;
  }
  return NULL;
}

PyObject *list_remove_all(PyShListObject *self, PyObject *) {
  try {
    self->array->get()->clear();
    Py_RETURN_NONE;
  } catch (const std::exception &exc) {
    Python_context::set_python_error(exc);
    return NULL;
  }
  return NULL;
}

PyDoc_STRVAR(PyShListDoc,
             "List() -> shcore Array\n\
                                                    \n\
                                                                                                                                  Creates a new instance of a shcore array object.");

PyDoc_STRVAR(append_doc, "L.append(value) -- append object to end");
PyDoc_STRVAR(insert_doc, "L.insert(index, value) -- insert object at index");
PyDoc_STRVAR(remove_doc,
             "L.remove(value) -- remove first occurrence of object");
PyDoc_STRVAR(remove_all_doc,
             "L.remove_all() -- remove all elements from the list");
PyDoc_STRVAR(extend_doc, "L.extend(list) -- add all elements from the list");

static PyMethodDef PyShListMethods[] = {
    //{"__getitem__", (PyCFunction)list_subscript, METH_O|METH_COEXIST,
    // getitem_doc},
    {"append", (PyCFunction)list_append, METH_O, append_doc},
    {"extend", (PyCFunction)list_inplace_concat, METH_O, extend_doc},
    {"insert", (PyCFunction)list_insert, METH_VARARGS, insert_doc},
    {"remove", (PyCFunction)list_remove, METH_O, remove_doc},
    {"remove_all", (PyCFunction)list_remove_all, METH_NOARGS, remove_all_doc},
    {NULL, NULL, 0, NULL}};

static PySequenceMethods PyShListObject_as_sequence = {
    (lenfunc)list_length,     // lenfunc sq_length;
    0,                        // binaryfunc sq_concat;
    0,                        // ssizeargfunc sq_repeat;
    (ssizeargfunc)list_item,  // ssizeargfunc sq_item;
    0,  // (ssizessizeargfunc)list_slice, // ssizessizeargfunc sq_slice;
    (ssizeobjargproc)list_assign,  // ssizeobjargproc sq_ass_item;
    0,  // (ssizessizeobjargproc)list_assign_slice,// ssizessizeobjargproc
        // sq_ass_slice;
    (objobjproc)list_contains,        // objobjproc sq_contains;
    (binaryfunc)list_inplace_concat,  // binaryfunc sq_inplace_concat;
    0                                 // ssizeargfunc sq_inplace_repeat;
};

static PyTypeObject PyShListObjectType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)  // PyObject_VAR_HEAD
    "List",  // char *tp_name; /* For printing, in format "<module>.<name>" */
    sizeof(PyShListObject),
    0,  // int tp_basicsize, tp_itemsize; /* For allocation */

    /* Methods to implement standard operations */

    (destructor)list_dealloc,  //  destructor tp_dealloc;
    0,                         // printfunc tp_print;
    0,                         // getattrfunc tp_getattr;
    0,                         // setattrfunc tp_setattr;
    0,                         // (cmpfunc)list_compare, //  cmpfunc tp_compare;
    (reprfunc)list_repr,       // reprfunc tp_repr;

    /* Method suites for standard classes */

    0,                            //  PyNumberMethods *tp_as_number;
    &PyShListObject_as_sequence,  //  PySequenceMethods *tp_as_sequence;
    0,                            //  PyMappingMethods *tp_as_mapping;

    /* More standard operations (here for binary compatibility) */

    0,                        // hashfunc tp_hash;
    0,                        // ternaryfunc tp_call;
    (reprfunc)list_str,       // reprfunc tp_str;
    PyObject_GenericGetAttr,  // getattrofunc tp_getattro;
    0,                        // setattrofunc tp_setattro;

    /* Functions to access object as input/output buffer */
    0,  // PyBufferProcs *tp_as_buffer;

    /* Flags to define presence of optional/expanded features */
    Py_TPFLAGS_DEFAULT,  // long tp_flags;

    PyShListDoc,  // char *tp_doc; /* Documentation string */

    /* Assigned meaning in release 2.0 */
    /* call function for all accessible objects */
    0,  // traverseproc tp_traverse;

    /* delete references to contained objects */
    0,  // inquiry tp_clear;

    /* Assigned meaning in release 2.1 */
    /* rich comparisons */
    0,  // richcmpfunc tp_richcompare;

    /* weak reference enabler */
    0,  // long tp_weaklistoffset;

    /* Added in release 2.2 */
    /* Iterators */
    0,  // getiterfunc tp_iter;
    0,  // iternextfunc tp_iternext;

    /* Attribute descriptor and subclassing stuff */
    PyShListMethods,      // struct PyMethodDef *tp_methods;
    0,                    // struct PyMemberDef *tp_members;
    0,                    //  struct PyGetSetDef *tp_getset;
    0,                    // struct _typeobject *tp_base;
    0,                    // PyObject *tp_dict;
    0,                    // descrgetfunc tp_descr_get;
    0,                    // descrsetfunc tp_descr_set;
    0,                    // long tp_dictoffset;
    (initproc)list_init,  // initproc tp_init;
    PyType_GenericAlloc,  // allocfunc tp_alloc;
    PyType_GenericNew,    // newfunc tp_new;
    0,  // freefunc tp_free; /* Low-level free-memory routine */
    0,  // inquiry tp_is_gc; /* For PyObject_IS_GC */
    0,  // PyObject *tp_bases;
    0,  // PyObject *tp_mro; /* method resolution order */
    0,  // PyObject *tp_cache;
    0,  // PyObject *tp_subclasses;
    0,  // PyObject *tp_weaklist;
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

void Python_context::init_shell_list_type() {
  if (PyType_Ready(&PyShListObjectType) < 0) {
    throw Exception::runtime_error(
        "Could not initialize Shcore Array type in python");
  }

  Py_INCREF(&PyShListObjectType);
  PyModule_AddObject(get_shell_python_support_module(), "List",
                     reinterpret_cast<PyObject *>(&PyShListObjectType));

  _shell_list_class = PyDict_GetItemString(
      PyModule_GetDict(get_shell_python_support_module()), "List");
}

PyObject *shcore::wrap(std::shared_ptr<Value::Array_type> array) {
  PyShListObject *wrapper = PyObject_New(PyShListObject, &PyShListObjectType);
  wrapper->array = new Value::Array_type_ref(array);
  return reinterpret_cast<PyObject *>(wrapper);
}

bool shcore::unwrap(PyObject *value,
                    std::shared_ptr<Value::Array_type> &ret_object) {
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return false;

  if (PyObject_IsInstance(value, ctx->get_shell_list_class())) {
    ret_object = *((PyShListObject *)value)->array;
    return true;
  }
  return false;
}
