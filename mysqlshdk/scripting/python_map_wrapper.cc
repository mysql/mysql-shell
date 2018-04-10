/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "scripting/python_map_wrapper.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <string>

using namespace shcore;

static PyObject *dict_keys(PyShDictObject *self, PyObject *args) {
  if (args) {
    Python_context::set_python_error(PyExc_ValueError,
                                     "method takes no arguments");
    return NULL;
  }

  PyObject *list = PyList_New(self->map->get()->size());

  Py_ssize_t i = 0;
  for (Value::Map_type::const_iterator iter = self->map->get()->begin();
       iter != self->map->get()->end(); ++iter)
    PyList_SetItem(list, i++, PyString_FromString(iter->first.c_str()));

  return list;
}

static PyObject *dict_items(PyShDictObject *self, PyObject *args) {
  if (args) {
    Python_context::set_python_error(PyExc_ValueError,
                                     "method takes no arguments");
    return NULL;
  }

  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return NULL;

  PyObject *list = PyList_New(self->map->get()->size());

  Py_ssize_t i = 0;
  for (Value::Map_type::const_iterator iter = self->map->get()->begin();
       iter != self->map->get()->end(); ++iter) {
    PyObject *tuple = PyTuple_New(2);
    PyTuple_SetItem(tuple, 0, PyString_FromString(iter->first.c_str()));
    PyTuple_SetItem(tuple, 1, ctx->shcore_value_to_pyobj(iter->second));
    PyList_SetItem(list, i++, tuple);
  }
  return list;
}

static PyObject *dict_values(PyShDictObject *self, PyObject *args) {
  if (args) {
    Python_context::set_python_error(PyExc_ValueError,
                                     "method takes no arguments");
    return NULL;
  }
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return NULL;

  PyObject *list = PyList_New(self->map->get()->size());

  Py_ssize_t i = 0;
  for (Value::Map_type::const_iterator iter = self->map->get()->begin();
       iter != self->map->get()->end(); ++iter)
    PyList_SetItem(list, i++, ctx->shcore_value_to_pyobj(iter->second));

  return list;
}

static PyObject *dict_has_key(PyShDictObject *self, PyObject *arg) {
  if (!arg) {
    Python_context::set_python_error(PyExc_ValueError,
                                     "missing required argument");
    return NULL;
  }

  const char *key_to_find = PyString_AsString(arg);
  bool found = false;

  if (key_to_find) found = self->map->get()->has_key(key_to_find);

  return PyBool_FromLong(found);
}

static PyObject *dict_update(PyShDictObject *self, PyObject *arg) {
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return NULL;

  if (!arg) {
    Python_context::set_python_error(PyExc_ValueError,
                                     "dict argument required for update()");
    return NULL;
  }

  Value value;

  try {
    value = ctx->pyobj_to_shcore_value(arg);
  } catch (std::exception &exc) {
    Python_context::set_python_error(exc);
    return NULL;
  }

  if (value.type != Map) {
    Python_context::set_python_error(PyExc_ValueError,
                                     "dict argument is not a dictionary");
    return NULL;
  }

  std::shared_ptr<Value::Map_type> map = value.as_map();

  self->map->get()->merge_contents(map, true);

  Py_RETURN_NONE;
}

static PyObject *dict_get(PyShDictObject *self, PyObject *arg) {
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return NULL;

  PyObject *def = NULL;
  char *key;

  if (!arg) {
    Python_context::set_python_error(PyExc_ValueError,
                                     "dict argument required for get()");
    return NULL;
  }

  if (!PyArg_ParseTuple(arg, "s|O", &key, &def)) return NULL;

  if (key) {
    if (self->map->get()->has_key(key))
      return ctx->shcore_value_to_pyobj((self->map->get()->find(key))->second);
    else {
      if (def) {
        Py_INCREF(def);
        return def;
      } else {
        std::string err = std::string("invalid key: ") + key;
        Python_context::set_python_error(PyExc_IndexError, err.c_str());
      }
    }
  }

  Py_RETURN_NONE;
}

static PyObject *dict_setdefault(PyShDictObject *self, PyObject *arg) {
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return NULL;

  PyObject *def = Py_None;
  char *key;

  if (!arg) {
    Python_context::set_python_error(PyExc_ValueError,
                                     "dict argument required for setdefault()");
    return NULL;
  }

  if (!PyArg_ParseTuple(arg, "s|O", &key, &def)) return NULL;

  if (key) {
    if (self->map->get()->has_key(key))
      return ctx->shcore_value_to_pyobj((self->map->get()->find(key))->second);
    else {
      if (def != Py_None) Py_INCREF(def);
      try {
        shcore::Value::Map_type *map = self->map->get();
        (*map)[key] = ctx->pyobj_to_shcore_value(def);
        return def;
      } catch (std::exception &exc) {
        Python_context::set_python_error(exc);
      }
    }
  }
  Py_RETURN_NONE;
}

static PyObject *dict_printable(PyShDictObject *self) {
  return PyString_FromString(Value(*self->map).repr().c_str());
}

static int dict_init(PyShDictObject *self, PyObject *args,
                     PyObject *UNUSED(kwds)) {
  PyObject *valueptr = NULL;

  if (!PyArg_ParseTuple(args, "")) return -1;

  delete self->map;

  if (valueptr) {
    try {
      self->map->reset(
          static_cast<Value::Map_type *>(PyCObject_AsVoidPtr(valueptr)));
    } catch (std::exception &exc) {
      Python_context::set_python_error(exc);
      return -1;
    }
  } else {
    try {
      self->map = new Value::Map_type_ref();
    } catch (std::exception &exc) {
      Python_context::set_python_error(exc);
      return -1;
    }
  }
  return 0;
}

static void dict_dealloc(PyShDictObject *self) {
  delete self->map;

  self->ob_type->tp_free(self);
}

static Py_ssize_t dict_length(PyShDictObject *self) {
  return self->map->get()->size();
}

static PyObject *dict_subscript(PyShDictObject *self, PyObject *key) {
  AutoPyObject tmp;
  if (PyUnicode_Check(key)) key = tmp = PyUnicode_AsUTF8String(key);

  if (!PyString_Check(key)) {
    Python_context::set_python_error(PyExc_KeyError,
                                     "shell.Dict key must be a string");
    return NULL;
  }
  const char *k = PyString_AsString(key);

  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return NULL;

  try {
    return ctx->shcore_value_to_pyobj((self->map->get()->find(k))->second);
  } catch (std::exception &exc) {
    Python_context::set_python_error(exc);
  }
  return NULL;
}

static int dict_as_subscript(PyShDictObject *self, PyObject *key,
                             PyObject *value) {
  AutoPyObject tmp;
  if (PyUnicode_Check(key)) key = tmp = PyUnicode_AsUTF8String(key);

  if (!PyString_Check(key)) {
    Python_context::set_python_error(PyExc_KeyError,
                                     "shell.Dict key must be a string");
    return -1;
  }
  const char *k = PyString_AsString(key);

  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return -1;

  try {
    if (value == NULL)
      self->map->get()->erase(k);
    else {
      try {
        Value v = ctx->pyobj_to_shcore_value(value);
        shcore::Value::Map_type *map = self->map->get();
        (*map)[k] = v;
      } catch (std::exception &exc) {
        Python_context::set_python_error(exc);
      }
    }
    return 0;
  } catch (std::exception &exc) {
    Python_context::set_python_error(exc);
  }
  return -1;
}

static PyObject *dict_getattro(PyShDictObject *self, PyObject *attr_name) {
  AutoPyObject tmp;
  if (PyUnicode_Check(attr_name))
    attr_name = tmp = PyUnicode_AsUTF8String(attr_name);

  if (PyString_Check(attr_name)) {
    const char *attrname = PyString_AsString(attr_name);

    PyObject *object;
    if ((object = PyObject_GenericGetAttr((PyObject *)self, attr_name)))
      return object;
    PyErr_Clear();

    if (strcmp(attrname, "__members__") == 0) {
      PyObject *members = PyList_New(self->map->get()->size());

      int i = 0;
      for (Value::Map_type::const_iterator iter = self->map->get()->begin();
           iter != self->map->get()->end(); ++iter) {
        PyObject *tmp_str = PyString_FromString(iter->first.c_str());
        PyList_SET_ITEM(members, i++, tmp_str);
      }
      return members;
    } else if (strcmp(attrname, "__methods__") == 0) {
      PyObject *methods = Py_BuildValue("[sssss]", "keys", "items", "values",
                                        "has_key", "update", "setdefault");
      return methods;
    } else {
      if (self->map->get()->has_key(attrname)) {
        Python_context *ctx = Python_context::get_and_check();
        if (!ctx) return NULL;

        return ctx->shcore_value_to_pyobj((**self->map)[attrname]);
      } else {
        std::string err = std::string("unknown attribute: ") + attrname;
        Python_context::set_python_error(PyExc_IndexError, err.c_str());
        return NULL;
      }
    }
  }
  Python_context::set_python_error(PyExc_KeyError,
                                   "shell.Dict key must be a string");
  return NULL;
}

PyDoc_STRVAR(PyShDictDoc,
             "Dict() -> shcore Map\n\
                                                                              \n\
                                                                                                                                                                                                                                                                                 Creates a new instance of a shcore Map object.");

static PyMethodDef PyShDictMethods[] = {
    //{"__getitem__", (PyCFunction)dict_subscript, METH_O|METH_COEXIST,
    // getitem_doc},
    {"keys", (PyCFunction)dict_keys, 0, NULL},
    {"items", (PyCFunction)dict_items, 0, NULL},
    {"values", (PyCFunction)dict_values, 0, NULL},
    {"has_key", (PyCFunction)dict_has_key, 0, NULL},
    {"update", (PyCFunction)dict_update, 0, NULL},
    {"get", (PyCFunction)dict_get, METH_VARARGS, NULL},
    {"setdefault", (PyCFunction)dict_setdefault, 0, NULL},
    {NULL, NULL, 0, NULL}};

static PyMappingMethods PyShDictObject_as_mapping = {
    (lenfunc)dict_length,             // inquiry mp_length;
    (binaryfunc)dict_subscript,       // binaryfunc mp_subscript;
    (objobjargproc)dict_as_subscript  // objobjargproc mp_ass_subscript;
};

static PyTypeObject PyShDictObjectType = {
    PyObject_HEAD_INIT(&PyType_Type)  // PyObject_VAR_HEAD
    0,
    "shell.Dict",  // char *tp_name; /* For printing, in format
                   // "<module>.<name>" */
    sizeof(PyShDictObject),
    0,  // int tp_basicsize, tp_itemsize; /* For allocation */

    /* Methods to implement standard operations */

    (destructor)dict_dealloc,  // destructor tp_dealloc;
    0,                         // printfunc tp_print;
    0,                         // getattrfunc tp_getattr;
    0,                         // setattrfunc tp_setattr;
    0,                         // (cmpfunc)dict_compare, // cmpfunc tp_compare;
    0,                         // (reprfunc)dict_repr, // reprfunc tp_repr;

    /* Method suites for standard classes */

    0,                           // PyNumberMethods *tp_as_number;
    0,                           // PySequenceMethods *tp_as_sequence;
    &PyShDictObject_as_mapping,  // PyMappingMethods *tp_as_mapping;

    /* More standard operations (here for binary compatibility) */

    0,                            // hashfunc tp_hash;
    0,                            // ternaryfunc tp_call;
    (reprfunc)dict_printable,     // reprfunc tp_str;
    (getattrofunc)dict_getattro,  // getattrofunc tp_getattro;
    0,                            // setattrofunc tp_setattro;

    /* Functions to access object as input/output buffer */
    0,  // PyBufferProcs *tp_as_buffer;

    /* Flags to define presence of optional/expanded features */
    Py_TPFLAGS_DEFAULT,  // long tp_flags;

    PyShDictDoc,  // char *tp_doc; /* Documentation string */

    /* Assigned meaning in release 2.0 */
    /* call function for all accessible objects */
    0,  // traverseproc tp_traverse;

    /* delete references to contained objects */
    0,  // inquiry tp_clear;

    /* Assigned meaning in release 2.1 */
    /* rich comparisons */
    0,  // richcmpfunc tp_richcompare;

    /* weak reference enabler */
    0,  // long tp_weakdictoffset;

    /* Added in release 2.2 */
    /* Iterators */
    0,  // getiterfunc tp_iter;
    0,  // iternextfunc tp_iternext;

    /* Attribute descriptor and subclassing stuff */
    PyShDictMethods,      // struct PyMethodDef *tp_methods;
    0,                    // struct PyMemberDef *tp_members;
    0,                    // struct PyGetSetDef *tp_getset;
    0,                    // struct _typeobject *tp_base;
    0,                    // PyObject *tp_dict;
    0,                    // descrgetfunc tp_descr_get;
    0,                    // descrsetfunc tp_descr_set;
    0,                    // long tp_dictoffset;
    (initproc)dict_init,  // initproc tp_init;
    PyType_GenericAlloc,  // allocfunc tp_alloc;
    PyType_GenericNew,    // newfunc tp_new;
    0,  // freefunc tp_free; /* Low-level free-memory routine */
    0,  // inquiry tp_is_gc; /* For PyObject_IS_GC */
    0,  // PyObject *tp_bases;
    0,  // PyObject *tp_mro; /* method resolution order */
    0,  // PyObject *tp_cache;
    0,  // PyObject *tp_subclasses;
    0,  // PyObject *tp_weakdict;
    0,  // tp_del
#if (PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION > 5)
    0  // tp_version_tag
#endif
};

void Python_context::init_shell_dict_type() {
  PyShDictObjectType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&PyShDictObjectType) < 0) {
    throw std::runtime_error("Could not initialize Shcore Map type in python");
  }

  Py_INCREF(&PyShDictObjectType);
  PyModule_AddObject(get_shell_python_support_module(), "Dict",
                     reinterpret_cast<PyObject *>(&PyShDictObjectType));

  _shell_dict_class = PyDict_GetItemString(
      PyModule_GetDict(get_shell_python_support_module()), "Dict");
}

PyObject *shcore::wrap(std::shared_ptr<Value::Map_type> map) {
  PyShDictObject *map_wrapper =
      PyObject_New(PyShDictObject, &PyShDictObjectType);
  map_wrapper->map = new Value::Map_type_ref(map);
  return reinterpret_cast<PyObject *>(map_wrapper);
}

bool shcore::unwrap(PyObject *value,
                    std::shared_ptr<Value::Map_type> &ret_object) {
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return false;

  if (PyObject_IsInstance(value, ctx->get_shell_dict_class())) {
    ret_object = *((PyShDictObject *)value)->map;
    return true;
  }
  return false;
}
