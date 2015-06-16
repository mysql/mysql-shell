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

#include "shellcore/python_object_wrapper.h"
#include <string>
#include <sstream>
#include "shellcore/python_utils.h"

using namespace shcore;


static int object_init(PyShObjObject *self, PyObject *args, PyObject *kwds)
{
  Python_context *ctx = Python_context::get_and_check();
  if (ctx)
  {
    PyObject *valueptr = NULL;
    static const char *kwlist[] = {"__valueptr__", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", (char**)kwlist, &valueptr))
      return -1;

    delete self->object;

    if (valueptr && valueptr != Py_None)
    {
      try
      {
        self->object->reset(static_cast<Object_bridge*>(PyCObject_AsVoidPtr(valueptr)));
      }
      catch (std::exception &exc)
      {
        Python_context::set_python_error(exc);
        return -1;
      }
    }
    else
    {
      try
      {
        self->object = new Object_bridge_ref();
      }
      catch (std::exception &exc)
      {
        Python_context::set_python_error(exc);
        return -1;
      }
    }
    return 0;
  }
  return -1;
}


static void object_dealloc(PyShObjObject *self)
{
  delete self->object;

  self->ob_type->tp_free(self);
}


static int object_compare(PyShObjObject *self, PyShObjObject *other)
{
  if (self->object->get() == other->object->get())
    return 0;

  return 1;
}


static PyObject *object_printable(PyShObjObject *self)
{
  std::string s;
  return PyString_FromString(self->object->get()->append_repr(s).c_str());
}


static PyObject *object_getattro(PyShObjObject *self, PyObject *attr_name)
{
  if (PyString_Check(attr_name))
  {
    const char *attrname = PyString_AsString(attr_name);

    PyObject *object;
    if ((object= PyObject_GenericGetAttr((PyObject*)self, attr_name)))
      return object;
    PyErr_Clear();

    if (self->object->get()->has_member(attrname))
    {
      Python_context *ctx = Python_context::get_and_check();
      if (!ctx) return NULL;
      return ctx->shcore_value_to_pyobj(self->object->get()->get_member(attrname));
    }
    // TODO: can the object attribute be a method?
    //else if (self->object->get()->has_method(attrname))
    //{
      // create a method call object and return it
      // TODO
    //  return (PyObject*)method;
    else
    {
      std::string err = std::string("unknown attribute : ") + attrname;
      PyErr_SetString(PyExc_IndexError, err.c_str());
    }
  }
  return NULL;
}


static int object_setattro(PyShObjObject *self, PyObject *attr_name, PyObject *attr_value)
{
  if (PyString_Check(attr_name))
  {
    const char *attrname = PyString_AsString(attr_name);

    if (self->object->get()->has_member(attrname))
    {
      Python_context *ctx = Python_context::get_and_check();
      if (!ctx) return -1;

      Value value;

      try
      {
        value = ctx->pyobj_to_shcore_value(attr_value);
      }
      catch (const std::exception &exc)
      {
        Python_context::set_python_error(exc);
        return -1;
      }
      try
      {
        self->object->get()->set_member(attrname, value);
      }
      catch (const std::exception &exc)
      {
        Python_context::set_python_error(exc);
        return -1;
      }
      return 0;
    }

    PyErr_Format(PyExc_AttributeError, "unknown attribute '%s'", attrname);
  }
  return -1;
}


static PyObject *call_object_method(shcore::Object_bridge *object, Value method, PyObject *args)
{
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx)
    return NULL;

  boost::shared_ptr<shcore::Function_base> func = method.as_function();

  if ((int)func->signature().size() != PyTuple_Size(args))
  {
    std::stringstream err;
    err << func->name().c_str() << "()" <<
    " takes " << (int)func->signature().size() <<
    " arguments (" << (int)PyTuple_Size(args) <<
    " given)";

    PyErr_SetString(PyExc_TypeError, err.str().c_str());
    return NULL;
  }

  Argument_list r;

  for (int c = func->signature().size(), i = 0; i < c; i++)
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

      result = object->call(func->name(), r);
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


static PyObject *
object_callmethod(PyShObjObject *self, PyObject *args)
{
  PyObject *method_name;

  if (PyTuple_Size(args) < 1 || !(method_name = PyTuple_GetItem(args, 0)) || !PyString_Check(method_name))
  {
    PyErr_SetString(PyExc_TypeError, "1st argument must be name of method to call");
    return NULL;
  }

  const Value method = self->object->get()->get_member(PyString_AsString(method_name));
  if (!method)
  {
    PyErr_SetString(PyExc_TypeError, "invalid method");
    return NULL;
  }

  /*
   * get_member() can return not only function but also properties which are not functions.
   * We need to validate that it actually returned a function
   */
  try
  {
    boost::shared_ptr<shcore::Function_base> func = method.as_function();
  }
  catch (std::exception &exc)
  {
    Python_context::set_python_error(exc, "member is not a function");
    return NULL;
  }

  return call_object_method(self->object->get(), method, PyTuple_GetSlice(args, 1, PyTuple_Size(args)));
}


PyDoc_STRVAR(PyShObjDoc,
"Object(shcoreclass) -> Shcore Object\n\
\n\
Creates a new instance of a Shcore object. shcoreclass specifies the name of\n\
the shcore class of the object.");


PyDoc_STRVAR(call_doc,
"callmethod(method_name, ...) -> value");


static PyMethodDef PyShObjMethods[] = {
{"__callmethod__",  (PyCFunction)object_callmethod,  METH_VARARGS, call_doc},
{NULL, NULL, 0, NULL}
};


static PyTypeObject PyShObjObjectType =
{
PyObject_HEAD_INIT(&PyType_Type)  // PyObject_VAR_HEAD
0,
"shell.Object",  // char *tp_name; /* For printing, in format "<module>.<name>" */
sizeof(PyShObjObject), 0,  // int tp_basicsize, tp_itemsize; /* For allocation */

/* Methods to implement standard operations */

(destructor)object_dealloc,  // destructor tp_dealloc;
0,  // printfunc tp_print;
0,  // getattrfunc tp_getattr;
0,  // setattrfunc tp_setattr;
(cmpfunc)object_compare,  //  cmpfunc tp_compare;
0,  // (reprfunc)dict_repr, // reprfunc tp_repr;

/* Method suites for standard classes */

0,  // PyNumberMethods *tp_as_number;
0,  // PySequenceMethods *tp_as_sequence;
0,  //  PyMappingMethods *tp_as_mapping;

/* More standard operations (here for binary compatibility) */

0, //  hashfunc tp_hash;
0,  // ternaryfunc tp_call;
(reprfunc)object_printable,  // reprfunc tp_str;
(getattrofunc)object_getattro,  // getattrofunc tp_getattro;
(setattrofunc)object_setattro,  //  setattrofunc tp_setattro;

/* Functions to access object as input/output buffer */
0,  // PyBufferProcs *tp_as_buffer;

/* Flags to define presence of optional/expanded features */
Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, //  long tp_flags;

PyShObjDoc,  // char *tp_doc; /* Documentation string */

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
PyShObjMethods,  // struct PyMethodDef *tp_methods;
0,  // struct PyMemberDef *tp_members;
0,  //  struct PyGetSetDef *tp_getset;
0,  // struct _typeobject *tp_base;
0,  // PyObject *tp_dict;
0,  // descrgetfunc tp_descr_get;
0,  // descrsetfunc tp_descr_set;
0,  // long tp_dictoffset;
(initproc)object_init,  // initproc tp_init;
PyType_GenericAlloc,  // allocfunc tp_alloc;
PyType_GenericNew,  // newfunc tp_new;
0,  // freefunc tp_free; /* Low-level free-memory routine */
0,  // inquiry tp_is_gc; /* For PyObject_IS_GC */
0,  // PyObject *tp_bases;
0,  // PyObject *tp_mro; /* method resolution order */
0,  // PyObject *tp_cache;
0,  // PyObject *tp_subclasses;
0,  // PyObject *tp_weakdict;
0,  // tp_del
#if (PY_MAJOR_VERSION == 2) && (PY_MINOR_VERSION > 5)
0   // tp_version_tag
#endif
};


void Python_context::init_shell_object_type()
{
  PyShObjObjectType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&PyShObjObjectType) < 0)
  {
    throw std::runtime_error("Could not initialize Shcore Object type in python");
  }

  Py_INCREF(&PyShObjObjectType);
  PyModule_AddObject(get_shell_module(), "Object", reinterpret_cast<PyObject *>(&PyShObjObjectType));

  _shell_object_class = PyDict_GetItemString(PyModule_GetDict(get_shell_module()), "Object");
}


Python_object_wrapper::Python_object_wrapper(Python_context *context)
: _context(context)
{
  _object_wrapper = PyObject_New(PyShObjObject, &PyShObjObjectType);

  Py_INCREF(&PyShObjObjectType);
}


Python_object_wrapper::~Python_object_wrapper()
{
  PyObject_Del(_object_wrapper);
  Py_DECREF(&PyShObjObjectType);
}


PyObject *Python_object_wrapper::wrap(boost::shared_ptr<Object_bridge> object)
{
  _object_wrapper->object = new Object_bridge_ref(object);
  return reinterpret_cast<PyObject*>(_object_wrapper);
}


bool Python_object_wrapper::unwrap(PyObject *value, boost::shared_ptr<Object_bridge> &ret_object)
{
  Python_context *ctx = Python_context::get_and_check();
  if (!ctx) return false;

  if (PyObject_IsInstance(value, ctx->get_shell_object_class())) {
    ret_object = *((PyShObjObject*)value)->object;
    return true;
  }
  return false;
}
