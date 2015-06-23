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

#include "shellcore/python_function_wrapper.h"
#include <string>
#include <sstream>
#include "shellcore/python_utils.h"
#include "shellcore/common.h"

using namespace shcore;


static void method_dealloc(PyShFuncObject *self)
{
  delete self->func;

  self->ob_type->tp_free(self);
}


static PyObject *method_call(PyShFuncObject *self, PyObject *args, PyObject *UNUSED(kw))
{
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx)
    return NULL;

  if ((int)self->func->get()->signature().size() != PyTuple_Size(args))
  {
    std::stringstream err;
    err << self->func->get()->name().c_str() << "()" <<
    " takes " << (int)self->func->get()->signature().size() <<
    " arguments (" << (int)PyTuple_Size(args) <<
    " given)";

    PyErr_SetString(PyExc_TypeError, err.str().c_str());
    return NULL;
  }

  Argument_list r;

  for (int c = self->func->get()->signature().size(), i = 0; i < c; i++)
  {
    PyObject *argval= PyTuple_GetItem(args, i);

    try
    {
      Value v = ctx->pyobj_to_shcore_value(argval);
      r.push_back(v);
    }
    catch (std::exception &exc)
    {
      Python_context::set_python_error(exc);
      return NULL;
    }
  }

  try
  {
    Value result;
    {
      WillLeavePython lock;

      result = self->func->get()->invoke(r);
    }
    return ctx->shcore_value_to_pyobj(result);
  }
  catch (std::exception &exc)
  {
    Python_context::set_python_error(exc);
    return NULL;
  }

  return NULL;
}


static PyTypeObject PyShFuncObjectType =
{
PyObject_HEAD_INIT(&PyType_Type) //PyObject_VAR_HEAD
0,
"grt.Object", //char *tp_name; /* For printing, in format "<module>.<name>" */
sizeof(PyShFuncObject), 0, //int tp_basicsize, tp_itemsize; /* For allocation */

/* Methods to implement standard operations */

(destructor)method_dealloc, //  destructor tp_dealloc;
0, //  printfunc tp_print;
0, //  getattrfunc tp_getattr;
0, //  setattrfunc tp_setattr;
0, //(cmpfunc)object_compare, //  cmpfunc tp_compare;
0, //(reprfunc)object_repr,//  reprfunc tp_repr;

/* Method suites for standard classes */

0, //  PyNumberMethods *tp_as_number;
0,//  PySequenceMethods *tp_as_sequence;
0, //  PyMappingMethods *tp_as_mapping;

/* More standard operations (here for binary compatibility) */

0, //  hashfunc tp_hash;
(ternaryfunc)method_call, //  ternaryfunc tp_call;
0, //  reprfunc tp_str;
PyObject_GenericGetAttr, //  getattrofunc tp_getattro;
PyObject_GenericSetAttr, //  setattrofunc tp_setattro;

/* Functions to access object as input/output buffer */
0, //  PyBufferProcs *tp_as_buffer;

/* Flags to define presence of optional/expanded features */
Py_TPFLAGS_DEFAULT, //  long tp_flags;

0, //  char *tp_doc; /* Documentation string */

/* Assigned meaning in release 2.0 */
/* call function for all accessible objects */
0, //  traverseproc tp_traverse;

/* delete references to contained objects */
0, //  inquiry tp_clear;

/* Assigned meaning in release 2.1 */
/* rich comparisons */
0, //  richcmpfunc tp_richcompare;

/* weak reference enabler */
0, //  long tp_weaklistoffset;

/* Added in release 2.2 */
/* Iterators */
0, //  getiterfunc tp_iter;
0, //  iternextfunc tp_iternext;

/* Attribute descriptor and subclassing stuff */
0, //  struct PyMethodDef *tp_methods;
0, //  struct PyMemberDef *tp_members;
0, //  struct PyGetSetDef *tp_getset;
0, //  struct _typeobject *tp_base;
0, //  PyObject *tp_dict;
0, //  descrgetfunc tp_descr_get;
0, //  descrsetfunc tp_descr_set;
0, //  long tp_dictoffset;
0, //  initproc tp_init;
PyType_GenericAlloc, //  allocfunc tp_alloc;
PyType_GenericNew, //  newfunc tp_new;
0, //  freefunc tp_free; /* Low-level free-memory routine */
0, //  inquiry tp_is_gc; /* For PyObject_IS_GC */
0, //  PyObject *tp_bases;
0, //  PyObject *tp_mro; /* method resolution order */
0, //  PyObject *tp_cache;
0, //  PyObject *tp_subclasses;
0, //  PyObject *tp_weaklist;
0, // tp_del
#if (PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION > 5)
0  // tp_version_tag
#endif
};

void Python_context::init_shell_function_type()
{
  PyShFuncObjectType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&PyShFuncObjectType) < 0)
  {
    throw std::runtime_error("Could not initialize Shcore Function type in python");
  }

  Py_INCREF(&PyShFuncObjectType);
  PyModule_AddObject(get_shell_module(), "Function", reinterpret_cast<PyObject *>(&PyShFuncObjectType));

  _shell_function_class = PyDict_GetItemString(PyModule_GetDict(get_shell_module()), "Function");
}


Python_function_wrapper::Python_function_wrapper(Python_context *context)
: _context(context)
{
  _function_wrapper = PyObject_New(PyShFuncObject, &PyShFuncObjectType);

  Py_INCREF(&PyShFuncObjectType);
}


Python_function_wrapper::~Python_function_wrapper()
{
  PyObject_Del(_function_wrapper);
  Py_DECREF(&PyShFuncObjectType);
}


PyObject *Python_function_wrapper::wrap(boost::shared_ptr<Function_base> func)
{
  _function_wrapper->func = new Function_base_ref(func);
  return reinterpret_cast<PyObject*>(_function_wrapper);
}


bool Python_function_wrapper::unwrap(PyObject *value, boost::shared_ptr<Function_base> &ret_func)
{
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return false;

  if (PyObject_IsInstance(value, ctx->get_shell_function_class())) {
    ret_func = *((PyShFuncObject*)value)->func;
    return true;
  }
  return false;
}
