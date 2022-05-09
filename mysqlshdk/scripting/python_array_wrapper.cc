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

#include "scripting/python_array_wrapper.h"

#include <algorithm>
#include <iterator>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {

namespace {

const std::size_t k_magic_state = 0x12345678;

////////////////////////////////////////////////////////////////////////////////
// Array_object
////////////////////////////////////////////////////////////////////////////////
struct Array_object {
  PyListObject base;
  Array_t *array = nullptr;
};

////////////////////////////////////////////////////////////////////////////////
// Array_object_type: sequence protocol
////////////////////////////////////////////////////////////////////////////////

Py_ssize_t ao_length(Array_object *self);
PyObject *ao_concat(Array_object *self, PyObject *right);
PyObject *ao_repeat(Array_object *self, Py_ssize_t n);
PyObject *ao_item(Array_object *self, Py_ssize_t index);
int ao_assign_item(Array_object *self, Py_ssize_t index, PyObject *value);
int ao_contains(Array_object *self, PyObject *value);
PyObject *ao_inplace_concat(Array_object *self, PyObject *other);
PyObject *ao_inplace_repeat(Array_object *self, Py_ssize_t n);

static PySequenceMethods array_object_as_sequence = {
    (lenfunc)ao_length,               // lenfunc sq_length;
    (binaryfunc)ao_concat,            // binaryfunc sq_concat;
    (ssizeargfunc)ao_repeat,          // ssizeargfunc sq_repeat;
    (ssizeargfunc)ao_item,            // ssizeargfunc sq_item;
    0,                                // void *was_sq_slice;
    (ssizeobjargproc)ao_assign_item,  // ssizeobjargproc sq_ass_item;
    0,                                // void *was_sq_ass_slice;
    (objobjproc)ao_contains,          // objobjproc sq_contains;
    (binaryfunc)ao_inplace_concat,    // binaryfunc sq_inplace_concat;
    (ssizeargfunc)ao_inplace_repeat   // ssizeargfunc sq_inplace_repeat;
};

////////////////////////////////////////////////////////////////////////////////
// Array_object_type: mapping protocol
////////////////////////////////////////////////////////////////////////////////

PyObject *ao_subscript(Array_object *self, PyObject *item);
int ao_assign_subscript(Array_object *self, PyObject *item, PyObject *value);

PyMappingMethods array_object_as_mapping = {
    (lenfunc)ao_length,                 // lenfunc mp_length;
    (binaryfunc)ao_subscript,           // binaryfunc mp_subscript;
    (objobjargproc)ao_assign_subscript  // objobjargproc mp_ass_subscript;
};

////////////////////////////////////////////////////////////////////////////////
// Array_object_type: methods
////////////////////////////////////////////////////////////////////////////////

PyDoc_STRVAR(getitem_doc, "L.__getitem__(y) <==> L[y]");
PyDoc_STRVAR(reversed_doc,
             "L.__reversed__() -- return a reverse iterator over the list");
PyDoc_STRVAR(sizeof_doc, "L.__sizeof__() -- size of L in memory, in bytes");
PyDoc_STRVAR(append_doc, "L.append(object) -> None -- append object to end");
PyDoc_STRVAR(clear_doc,
             "L.clear() -> None -- remove all elements from the list");
PyDoc_STRVAR(copy_doc, "L.copy() -> list -- a shallow copy of L");
PyDoc_STRVAR(
    count_doc,
    "L.count(value) -> integer -- return number of occurrences of value");
PyDoc_STRVAR(extend_doc,
             "L.extend(iterable) -> None -- extend list by appending elements "
             "from the iterable");
PyDoc_STRVAR(index_doc,
             "L.index(value, [start, [stop]]) -> integer -- return first index "
             "of value.\nRaises ValueError if the value is not present.");
PyDoc_STRVAR(insert_doc,
             "L.insert(index, object) -> None -- insert object at index");
PyDoc_STRVAR(
    pop_doc,
    "L.pop([index]) -> item -- remove and return item at index (default "
    "last).\nRaises IndexError if list is empty or index is out of range.");
PyDoc_STRVAR(remove_doc,
             "L.remove(value) -> None -- remove first occurrence of value");
PyDoc_STRVAR(remove_all_doc,
             "L.remove_all() -> None -- remove all elements from the list");
PyDoc_STRVAR(reverse_doc, "L.reverse() -> None -- reverse *IN PLACE*");
PyDoc_STRVAR(
    sort_doc,
    "L.sort(key=None, reverse=False) -> None -- stable sort *IN PLACE*");

PyObject *ao_getstate(Array_object *self);
PyObject *ao_riter(Array_object *self);
PyObject *ao_setstate(Array_object *self, PyObject *state);
PyObject *ao_sizeof(Array_object *self);
PyObject *ao_append(Array_object *self, PyObject *v);
PyObject *ao_copy(Array_object *self);
PyObject *ao_count(Array_object *self, PyObject *v);
PyObject *ao_extend(Array_object *self, PyObject *other);
PyObject *ao_index(Array_object *self, PyObject *args);
PyObject *ao_insert(Array_object *self, PyObject *args);
PyObject *ao_pop(Array_object *self, PyObject *args);
PyObject *ao_remove(Array_object *self, PyObject *v);
PyObject *ao_remove_all(Array_object *self);
PyObject *ao_reverse(Array_object *self);
PyObject *ao_sort(Array_object *self, PyObject *args, PyObject *kwds);

#if __GNUC__ > 7 && !defined(__clang__)
// -Wcast-function-type was added in GCC 8.0, needs to be suppressed here,
// because we're casting functions with different number of parameters to
// PyCFunction
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif  // __GNUC__ > 7 && !defined(__clang__)

static PyMethodDef array_object_methods[] = {
    {"__getitem__", (PyCFunction)ao_subscript, METH_O | METH_COEXIST,
     getitem_doc},
    {"__getstate__", (PyCFunction)ao_getstate, METH_NOARGS, nullptr},
    {"__reversed__", (PyCFunction)ao_riter, METH_NOARGS, reversed_doc},
    {"__setstate__", (PyCFunction)ao_setstate, METH_O, nullptr},
    {"__sizeof__", (PyCFunction)ao_sizeof, METH_NOARGS, sizeof_doc},
    {"append", (PyCFunction)ao_append, METH_O, append_doc},
    {"clear", (PyCFunction)ao_remove_all, METH_NOARGS, clear_doc},
    {"copy", (PyCFunction)ao_copy, METH_NOARGS, copy_doc},
    {"count", (PyCFunction)ao_count, METH_O, count_doc},
    {"extend", (PyCFunction)ao_extend, METH_O, extend_doc},
    {"index", (PyCFunction)ao_index, METH_VARARGS, index_doc},
    {"insert", (PyCFunction)ao_insert, METH_VARARGS, insert_doc},
    {"pop", (PyCFunction)ao_pop, METH_VARARGS, pop_doc},
    {"remove", (PyCFunction)ao_remove, METH_O, remove_doc},
    {"remove_all", (PyCFunction)ao_remove_all, METH_NOARGS, remove_all_doc},
    {"reverse", (PyCFunction)ao_reverse, METH_NOARGS, reverse_doc},
    {"sort", (PyCFunction)ao_sort, METH_VARARGS | METH_KEYWORDS, sort_doc},
    {nullptr, nullptr, 0, nullptr},
};

#if __GNUC__ > 7 && !defined(__clang__)
#pragma GCC diagnostic pop
#endif  // __GNUC__ > 7 && !defined(__clang__)

////////////////////////////////////////////////////////////////////////////////
// Array_object_type
////////////////////////////////////////////////////////////////////////////////

PyDoc_STRVAR(array_object_doc,
             "List() -> new empty shcore array\n"
             "\n"
             "Creates a new instance of a shcore array object.");

void ao_dealloc(Array_object *self);
PyObject *ao_repr(Array_object *self);
PyObject *ao_str(Array_object *self);
int ao_gc_traverse(PyObject *, visitproc, void *);
int ao_gc_clear(Array_object *);
PyObject *ao_richcompare(Array_object *self, PyObject *right, int op);
PyObject *ao_iter(Array_object *self);
int ao_init(Array_object *self, PyObject *args, PyObject *kw);
PyObject *ao_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

#if PY_VERSION_HEX >= 0x03080000 && PY_VERSION_HEX < 0x03090000
#ifdef __clang__
// The tp_print is marked as deprecated, which makes clang unhappy, 'cause it's
// initialized below. Skipping initialization also makes clang unhappy, so we're
// disabling the deprecated declarations warning.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif  // __clang__
#endif  // PY_VERSION_HEX

static PyTypeObject Array_object_type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)  // PyObject_VAR_HEAD
    // full name including the module is required for pickle support
    "mysqlsh.__shell_python_support__.List",  // const char *tp_name;

    /* For allocation */

    sizeof(Array_object),  // Py_ssize_t tp_basicsize;
    0,                     // Py_ssize_t tp_itemsize;

    /* Methods to implement standard operations */

    (destructor)ao_dealloc,  // destructor tp_dealloc;
    0,                       // printfunc tp_print;
    0,                       // getattrfunc tp_getattr;
    0,                       // setattrfunc tp_setattr;
    0,                       // cmpfunc tp_compare;
    (reprfunc)ao_repr,       // reprfunc tp_repr;

    /* Method suites for standard classes */

    0,                          //  PyNumberMethods *tp_as_number;
    &array_object_as_sequence,  //  PySequenceMethods *tp_as_sequence;
    &array_object_as_mapping,   //  PyMappingMethods *tp_as_mapping;

    /* More standard operations (here for binary compatibility) */

    0,                        // hashfunc tp_hash;
    0,                        // ternaryfunc tp_call;
    (reprfunc)ao_str,         // reprfunc tp_str;
    PyObject_GenericGetAttr,  // getattrofunc tp_getattro;
    0,                        // setattrofunc tp_setattro;

    /* Functions to access object as input/output buffer */
    0,  // PyBufferProcs *tp_as_buffer;

    /* Flags to define presence of optional/expanded features */
    Py_TPFLAGS_DEFAULT,  // long tp_flags;

    array_object_doc,  // char *tp_doc; /* Documentation string */

    /* Assigned meaning in release 2.0 */
    /* call function for all accessible objects */
    ao_gc_traverse,  // traverseproc tp_traverse;

    /* delete references to contained objects */
    (inquiry)ao_gc_clear,  // inquiry tp_clear;

    /* Assigned meaning in release 2.1 */
    /* rich comparisons */
    (richcmpfunc)ao_richcompare,  // richcmpfunc tp_richcompare;

    /* weak reference enabler */
    0,  // long tp_weaklistoffset;

    /* Added in release 2.2 */
    /* Iterators */
    (getiterfunc)ao_iter,  // getiterfunc tp_iter;
    0,                     // iternextfunc tp_iternext;

    /* Attribute descriptor and subclassing stuff */
    array_object_methods,  // struct PyMethodDef *tp_methods;
    0,                     // struct PyMemberDef *tp_members;
    0,                     // struct PyGetSetDef *tp_getset;
    0,                     // struct _typeobject *tp_base;
    0,                     // PyObject *tp_dict;
    0,                     // descrgetfunc tp_descr_get;
    0,                     // descrsetfunc tp_descr_set;
    0,                     // long tp_dictoffset;
    (initproc)ao_init,     // initproc tp_init;
    PyType_GenericAlloc,   // allocfunc tp_alloc;
    ao_new,                // newfunc tp_new;
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

////////////////////////////////////////////////////////////////////////////////
// Array_object_iterator
////////////////////////////////////////////////////////////////////////////////

struct Array_object_iterator {
  // clang-format off
  PyObject_HEAD
  Py_ssize_t index;
  Array_object *o;
  // clang-format on
};

////////////////////////////////////////////////////////////////////////////////
// Array_object_iterator_type: methods
////////////////////////////////////////////////////////////////////////////////

PyDoc_STRVAR(length_hint_doc,
             "Private method returning an estimate of len(list(it)).");
PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");
PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

PyObject *ao_iter_length(Array_object_iterator *self);
PyObject *ao_iter_reduce(Array_object_iterator *self);
PyObject *ao_iter_setstate(Array_object_iterator *self, PyObject *state);

#if __GNUC__ > 7 && !defined(__clang__)
// -Wcast-function-type was added in GCC 8.0, needs to be suppressed here,
// because we're casting functions with different number of parameters to
// PyCFunction
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif  // __GNUC__ > 7 && !defined(__clang__)

static PyMethodDef ao_iter_methods[] = {
    {"__length_hint__", (PyCFunction)ao_iter_length, METH_NOARGS,
     length_hint_doc},
    {"__reduce__", (PyCFunction)ao_iter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", (PyCFunction)ao_iter_setstate, METH_O, setstate_doc},
    {nullptr, nullptr, 0, nullptr},
};

#if __GNUC__ > 7 && !defined(__clang__)
#pragma GCC diagnostic pop
#endif  // __GNUC__ > 7 && !defined(__clang__)

////////////////////////////////////////////////////////////////////////////////
// Array_object_iterator_type
////////////////////////////////////////////////////////////////////////////////

void ao_iter_dealloc(Array_object_iterator *self);
PyObject *ao_iter_next(Array_object_iterator *self);

#if PY_VERSION_HEX >= 0x03080000 && PY_VERSION_HEX < 0x03090000
#ifdef __clang__
// The tp_print is marked as deprecated, which makes clang unhappy, 'cause it's
// initialized below. Skipping initialization also makes clang unhappy, so we're
// disabling the deprecated declarations warning.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif  // __clang__
#endif  // PY_VERSION_HEX

static PyTypeObject Array_object_iterator_type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)  // PyObject_VAR_HEAD
    "List_iterator",                        // const char *tp_name;

    /* For allocation */

    sizeof(Array_object_iterator),  // Py_ssize_t tp_basicsize;
    0,                              // Py_ssize_t tp_itemsize;

    /* Methods to implement standard operations */

    (destructor)ao_iter_dealloc,  // destructor tp_dealloc;
    0,                            // printfunc tp_print;
    0,                            // getattrfunc tp_getattr;
    0,                            // setattrfunc tp_setattr;
    0,                            // cmpfunc tp_compare;
    0,                            // reprfunc tp_repr;

    /* Method suites for standard classes */

    0,  //  PyNumberMethods *tp_as_number;
    0,  //  PySequenceMethods *tp_as_sequence;
    0,  //  PyMappingMethods *tp_as_mapping;

    /* More standard operations (here for binary compatibility) */

    0,                        // hashfunc tp_hash;
    0,                        // ternaryfunc tp_call;
    0,                        // reprfunc tp_str;
    PyObject_GenericGetAttr,  // getattrofunc tp_getattro;
    0,                        // setattrofunc tp_setattro;

    /* Functions to access object as input/output buffer */
    0,  // PyBufferProcs *tp_as_buffer;

    /* Flags to define presence of optional/expanded features */
    Py_TPFLAGS_DEFAULT,  // long tp_flags;

    0,  // char *tp_doc; /* Documentation string */

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
    PyObject_SelfIter,           // getiterfunc tp_iter;
    (iternextfunc)ao_iter_next,  // iternextfunc tp_iternext;

    /* Attribute descriptor and subclassing stuff */
    ao_iter_methods,  // struct PyMethodDef *tp_methods;
    0,                // struct PyMemberDef *tp_members;
    0,                // struct PyGetSetDef *tp_getset;
    0,                // struct _typeobject *tp_base;
    0,                // PyObject *tp_dict;
    0,                // descrgetfunc tp_descr_get;
    0,                // descrsetfunc tp_descr_set;
    0,                // long tp_dictoffset;
    0,                // initproc tp_init;
    0,                // allocfunc tp_alloc;
    0,                // newfunc tp_new;
    0,                // freefunc tp_free; /* Low-level free-memory routine */
    0,                // inquiry tp_is_gc; /* For PyObject_IS_GC */
    0,                // PyObject *tp_bases;
    0,                // PyObject *tp_mro; /* method resolution order */
    0,                // PyObject *tp_cache;
    0,                // PyObject *tp_subclasses;
    0,                // PyObject *tp_weaklist;
    0                 // tp_del
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

////////////////////////////////////////////////////////////////////////////////
// Array_object_riterator
////////////////////////////////////////////////////////////////////////////////

struct Array_object_riterator {
  // clang-format off
  PyObject_HEAD
  Py_ssize_t index;
  Array_object *o;
  // clang-format on
};

////////////////////////////////////////////////////////////////////////////////
// Array_object_riterator_type: methods
////////////////////////////////////////////////////////////////////////////////

PyObject *ao_riter_length(Array_object_riterator *self);
PyObject *ao_riter_reduce(Array_object_riterator *self);
PyObject *ao_riter_setstate(Array_object_riterator *self, PyObject *state);

#if __GNUC__ > 7 && !defined(__clang__)
// -Wcast-function-type was added in GCC 8.0, needs to be suppressed here,
// because we're casting functions with different number of parameters to
// PyCFunction
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif  // __GNUC__ > 7 && !defined(__clang__)

static PyMethodDef ao_riter_methods[] = {
    {"__length_hint__", (PyCFunction)ao_riter_length, METH_NOARGS,
     length_hint_doc},
    {"__reduce__", (PyCFunction)ao_riter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", (PyCFunction)ao_riter_setstate, METH_O, setstate_doc},
    {nullptr, nullptr, 0, nullptr},
};

#if __GNUC__ > 7 && !defined(__clang__)
#pragma GCC diagnostic pop
#endif  // __GNUC__ > 7 && !defined(__clang__)

////////////////////////////////////////////////////////////////////////////////
// Array_object_riterator_type
////////////////////////////////////////////////////////////////////////////////

void ao_riter_dealloc(Array_object_riterator *self);
PyObject *ao_riter_next(Array_object_riterator *self);

#if PY_VERSION_HEX >= 0x03080000 && PY_VERSION_HEX < 0x03090000
#ifdef __clang__
// The tp_print is marked as deprecated, which makes clang unhappy, 'cause it's
// initialized below. Skipping initialization also makes clang unhappy, so we're
// disabling the deprecated declarations warning.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif  // __clang__
#endif  // PY_VERSION_HEX

static PyTypeObject Array_object_riterator_type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)  // PyObject_VAR_HEAD
    "List_riterator",                       // const char *tp_name;

    /* For allocation */

    sizeof(Array_object_riterator),  // Py_ssize_t tp_basicsize;
    0,                               // Py_ssize_t tp_itemsize;

    /* Methods to implement standard operations */

    (destructor)ao_riter_dealloc,  // destructor tp_dealloc;
    0,                             // printfunc tp_print;
    0,                             // getattrfunc tp_getattr;
    0,                             // setattrfunc tp_setattr;
    0,                             // cmpfunc tp_compare;
    0,                             // reprfunc tp_repr;

    /* Method suites for standard classes */

    0,  //  PyNumberMethods *tp_as_number;
    0,  //  PySequenceMethods *tp_as_sequence;
    0,  //  PyMappingMethods *tp_as_mapping;

    /* More standard operations (here for binary compatibility) */

    0,                        // hashfunc tp_hash;
    0,                        // ternaryfunc tp_call;
    0,                        // reprfunc tp_str;
    PyObject_GenericGetAttr,  // getattrofunc tp_getattro;
    0,                        // setattrofunc tp_setattro;

    /* Functions to access object as input/output buffer */
    0,  // PyBufferProcs *tp_as_buffer;

    /* Flags to define presence of optional/expanded features */
    Py_TPFLAGS_DEFAULT,  // long tp_flags;

    0,  // char *tp_doc; /* Documentation string */

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
    PyObject_SelfIter,            // getiterfunc tp_iter;
    (iternextfunc)ao_riter_next,  // iternextfunc tp_iternext;

    /* Attribute descriptor and subclassing stuff */
    ao_riter_methods,  // struct PyMethodDef *tp_methods;
    0,                 // struct PyMemberDef *tp_members;
    0,                 // struct PyGetSetDef *tp_getset;
    0,                 // struct _typeobject *tp_base;
    0,                 // PyObject *tp_dict;
    0,                 // descrgetfunc tp_descr_get;
    0,                 // descrsetfunc tp_descr_set;
    0,                 // long tp_dictoffset;
    0,                 // initproc tp_init;
    0,                 // allocfunc tp_alloc;
    0,                 // newfunc tp_new;
    0,                 // freefunc tp_free; /* Low-level free-memory routine */
    0,                 // inquiry tp_is_gc; /* For PyObject_IS_GC */
    0,                 // PyObject *tp_bases;
    0,                 // PyObject *tp_mro; /* method resolution order */
    0,                 // PyObject *tp_cache;
    0,                 // PyObject *tp_subclasses;
    0,                 // PyObject *tp_weaklist;
    0                  // tp_del
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

////////////////////////////////////////////////////////////////////////////////
// helpers
////////////////////////////////////////////////////////////////////////////////

py::Release get_builtin(const char *name) {
  return py::Release::incref(PyDict_GetItemString(PyEval_GetBuiltins(), name));
}

Array_t to_array(PyObject *value) {
  Array_t ret;

  if (array_check(value)) {
    ret = *((Array_object *)value)->array;
  } else {
    ret = py::convert(value).as_array();
  }

  return ret;
}

Array_t to_sequence(Array_object *self, PyObject *value, const char *error) {
  Array_t ret;

  if (self == (Array_object *)value) {
    // protect from self assignment, copy the value
    const auto array = self->array->get();

    ret = make_array();

    ret->reserve(array->size());
    ret->insert(ret->begin(), array->begin(), array->end());
  } else if (array_check(value)) {
    ret = *((Array_object *)value)->array;
  } else {
    py::Release sequence{PySequence_Fast(value, error)};
    if (sequence) ret = py::convert(sequence.get()).as_array();
  }

  return ret;
}

py::Release ao_slice(Array_object *self, Py_ssize_t ilow, Py_ssize_t ihigh) {
  const auto size = ao_length(self);

  if (ilow < 0) {
    ilow = 0;
  } else if (ilow > size) {
    ilow = size;
  }

  if (ihigh < ilow) {
    ihigh = ilow;
  } else if (ihigh > size) {
    ihigh = size;
  }

  const auto src = self->array->get();
  const auto dst = make_array();

  dst->reserve(ihigh - ilow);
  dst->insert(dst->begin(), src->begin() + ilow, src->begin() + ihigh);

  return wrap(dst);
}

/**
 * self[ilow:ihigh] = v if v != NULL.
 * del self[ilow:ihigh] if v == NULL.
 */
int ao_assign_slice(Array_object *self, Py_ssize_t ilow, Py_ssize_t ihigh,
                    PyObject *v) {
  Array_t value;

  if (v) {
    value = to_sequence(self, v, "can only assign an iterable");

    if (!value) {
      return -1;
    }
  } else {
    value = make_array();
  }

  const Py_ssize_t array_size = ao_length(self);

  if (ilow < 0) {
    ilow = 0;
  } else if (ilow > array_size) {
    ilow = array_size;
  }

  if (ihigh < ilow) {
    ihigh = ilow;
  } else if (ihigh > array_size) {
    ihigh = array_size;
  }

  const Py_ssize_t items_to_replace = ihigh - ilow;
  assert(items_to_replace >= 0);
  const Py_ssize_t value_size = value->size();
  const Py_ssize_t diff = value_size - items_to_replace;
  const auto array = self->array->get();

  if (0 == array_size + diff) {
    array->clear();
    return 0;
  }

  if (diff < 0) {
    array->erase(array->begin() + ilow + value_size, array->begin() + ihigh);
  } else if (diff > 0) {
    array->reserve(array_size + diff);
    array->insert(array->begin() + ihigh, value->begin() + items_to_replace,
                  value->end());
  }

  std::copy(value->begin(),
            value->begin() + std::min(items_to_replace, value_size),
            array->begin() + ilow);

  return 0;
}

binaryfunc old_list_concat = nullptr;

PyObject *new_list_concat(PyObject *self, PyObject *other) {
  assert(old_list_concat);

  py::Release list;

  if (array_check(other)) {
    const auto array = ((Array_object *)other)->array->get();

    list = py::Release{PyList_New(array->size())};
    other = list.get();

    Py_ssize_t i = 0;

    try {
      for (const auto &v : *array)
        PyList_SET_ITEM(other, i++, py::convert(v).release());

    } catch (const std::exception &exc) {
      Python_context::set_python_error(PyExc_RuntimeError, exc.what());
      return nullptr;
    }
  }

  return old_list_concat(self, other);
}

void replace_list_concat() {
  // Python's list_concat() is used in expressions `list() + x`, requires `x` to
  // be an instance of a list class. We're fulfilling this requirement, however
  // we're not storing data in the list itself, and from the PyList_*() API
  // point of view, Array_object list is always empty. In order for this to
  // work, we're using our own implementation of list_concat(), if it's argument
  // is an Array_object, we're converting it to a Python's list.
  if (!old_list_concat) {
    // replace `sq_concat`, which is called in case of `list() + x`
    old_list_concat = Array_object_type.tp_base->tp_as_sequence->sq_concat;
    Array_object_type.tp_base->tp_as_sequence->sq_concat = new_list_concat;

    // replace list.__add__ as well
    const auto add = (PyWrapperDescrObject *)PyDict_GetItemString(
        Array_object_type.tp_base->tp_dict, "__add__");

    assert(add);
    assert((binaryfunc)add->d_wrapped == old_list_concat);

    add->d_wrapped = (void *)new_list_concat;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Array_object_type: sequence protocol
////////////////////////////////////////////////////////////////////////////////

/**
 * Returns the number of objects in sequence o on success, and -1 on failure.
 * This is equivalent to the Python expression len(o).
 */
Py_ssize_t ao_length(Array_object *self) { return self->array->get()->size(); }

/**
 * Return the concatenation of o1 and o2 on success, and NULL on failure.
 * This is the equivalent of the Python expression o1 + o2.
 */
PyObject *ao_concat(Array_object *self, PyObject *right) {
  if (!PyList_Check(right)) {
    Python_context::set_python_error(
        PyExc_TypeError,
        str_format("can only concatenate list (not \"%.200s\") to list",
                   right->ob_type->tp_name));

    return nullptr;
  }

  const auto l = self->array->get();
  const auto r = to_array(right);
  const auto result = make_array();

  result->reserve(l->size() + r->size());

  result->insert(result->end(), l->begin(), l->end());
  result->insert(result->end(), r->begin(), r->end());

  return wrap(result).release();
}

/**
 * Return the result of repeating sequence object o count times, or NULL on
 * failure. This is the equivalent of the Python expression o * count.
 */
PyObject *ao_repeat(Array_object *self, Py_ssize_t n) {
  if (n < 0) {
    n = 0;
  }

  const auto array = self->array->get();
  const auto size = array->size();
  const auto result = make_array();

  result->reserve(size * n);

  for (Py_ssize_t i = 0; i < n; ++i) {
    result->insert(result->end(), array->begin(), array->end());
  }

  return wrap(result).release();
}

/**
 * Return the ith element of o, or NULL on failure. This is the equivalent of
 * the Python expression o[i].
 *
 * Negative indexes are handled as follows: if the sq_length slot is filled, it
 * is called and the sequence length is used to compute a positive index which
 * is passed to sq_item. If sq_length is NULL, the index is passed as is to the
 * function.
 */
PyObject *ao_item(Array_object *self, Py_ssize_t index) {
  if (index < 0 || static_cast<Value::Array_type::size_type>(index) >=
                       self->array->get()->size()) {
    Python_context::set_python_error(PyExc_IndexError,
                                     "list index out of range");
    return nullptr;
  }

  try {
    return py::convert(self->array->get()->at(index)).release();
  } catch (const std::exception &exc) {
    Python_context::set_python_error(PyExc_RuntimeError, exc.what());
  }

  return nullptr;
}

/**
 * Assign object v to the ith element of o. Raise an exception and return -1 on
 * failure; return 0 on success. This is the equivalent of the Python statement
 * o[i] = v.
 *
 * If v is NULL, the element is deleted, however this feature is deprecated.
 */
int ao_assign_item(Array_object *self, Py_ssize_t index, PyObject *value) {
  if (index < 0 || static_cast<Value::Array_type::size_type>(index) >=
                       self->array->get()->size()) {
    Python_context::set_python_error(PyExc_IndexError,
                                     "list assignment index out of range");
    return -1;
  }

  try {
    const auto array = self->array->get();

    if (nullptr == value) {
      array->erase(array->begin() + index);
    } else {
      (*array)[index] = py::convert(value);
    }

    return 0;
  } catch (const std::exception &exc) {
    Python_context::set_python_error(PyExc_RuntimeError, exc.what());
  }

  return -1;
}

/**
 * Determine if o contains value. If an item in o is equal to value, return 1,
 * otherwise return 0. On error, return -1. This is equivalent to the Python
 * expression value in o.
 */
int ao_contains(Array_object *self, PyObject *value) {
  try {
    const auto array = self->array->get();

    return array->end() !=
           std::find(array->begin(), array->end(), py::convert(value));
  } catch (const std::exception &exc) {
    Python_context::set_python_error(PyExc_RuntimeError, exc.what());
  }

  return -1;
}

/**
 * Return the concatenation of o1 and o2 on success, and NULL on failure. The
 * operation is done in-place when o1 supports it. This is the equivalent of the
 * Python expression o1 += o2.
 *
 * Return value: New reference.
 */
PyObject *ao_inplace_concat(Array_object *self, PyObject *other) {
  // we're not using the result, decrease the reference count
  py::Release result{ao_extend(self, other)};
  if (!result) return nullptr;

  // we're returning a new reference, increase the reference count
  Py_INCREF(self);

  return reinterpret_cast<PyObject *>(self);
}

/**
 * Return the result of repeating sequence object o count times, or NULL on
 * failure. The operation is done in-place when o supports it. This is the
 * equivalent of the Python expression o *= count.
 *
 * Return value: New reference.
 */
PyObject *ao_inplace_repeat(Array_object *self, Py_ssize_t n) {
  const auto array = self->array->get();
  const auto size = array->size();

  if (0 == size || 1 == n) {
    // NOP
  } else if (n < 1) {
    array->clear();
  } else {
    array->reserve(n * size);

    const auto last = array->end();

    // loop starting at 1, we need to repeat items n - 1 times
    for (Py_ssize_t i = 1; i < n; ++i) {
      array->insert(array->end(), array->begin(), last);
    }
  }

  // we're returning a new reference, increase the reference count
  Py_INCREF(self);

  return reinterpret_cast<PyObject *>(self);
}

////////////////////////////////////////////////////////////////////////////////
// Array_object_type: mapping protocol
////////////////////////////////////////////////////////////////////////////////

/**
 * Return element of o corresponding to the object key or NULL on failure. This
 * is the equivalent of the Python expression o[key].
 */
PyObject *ao_subscript(Array_object *self, PyObject *item) {
  if (PyIndex_Check(item)) {
    Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);

    if (-1 == i && PyErr_Occurred()) return nullptr;

    if (i < 0) i += ao_length(self);

    return ao_item(self, i);
  }

  if (PySlice_Check(item)) {
    Py_ssize_t start = 0;
    Py_ssize_t stop = 0;
    Py_ssize_t step = 0;

    if (PySlice_Unpack(item, &start, &stop, &step) < 0) return nullptr;

    const auto length =
        PySlice_AdjustIndices(ao_length(self), &start, &stop, step);

    if (length <= 0) return wrap(make_array()).release();

    if (1 == step) return ao_slice(self, start, stop).release();

    const auto src = self->array->get();
    const auto dst = make_array();

    dst->resize(length);

    for (Py_ssize_t cur = start, i = 0; i < length; cur += (size_t)step, ++i)
      (*dst)[i] = (*src)[cur];

    return wrap(dst).release();
  }

  Python_context::set_python_error(
      PyExc_TypeError,
      str_format("list indices must be integers or slices, not %.200s",
                 item->ob_type->tp_name));

  return nullptr;
}

/**
 * Map the object key to the value v. Raise an exception and return -1 on
 * failure; return 0 on success. This is the equivalent of the Python statement
 * o[key] = v.
 *
 * v can also be set to NULL to delete an item.
 */
int ao_assign_subscript(Array_object *self, PyObject *item, PyObject *value) {
  if (PyIndex_Check(item)) {
    Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);

    if (-1 == i && PyErr_Occurred()) {
      return -1;
    }

    if (i < 0) {
      i += ao_length(self);
    }

    return ao_assign_item(self, i, value);
  } else if (PySlice_Check(item)) {
    Py_ssize_t start = 0;
    Py_ssize_t stop = 0;
    Py_ssize_t step = 0;

    if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
      return -1;
    }

    const auto length =
        PySlice_AdjustIndices(ao_length(self), &start, &stop, step);

    if (1 == step) {
      return ao_assign_slice(self, start, stop, value);
    }

    // Make sure s[5:2] = [..] inserts at the right place: before 5, not before
    // 2.
    if ((step < 0 && start < stop) || (step > 0 && start > stop)) {
      stop = start;
    }

    if (value) {
      const auto src =
          to_sequence(self, value, "must assign iterable to extended slice");

      if (!src) {
        return -1;
      }

      const Py_ssize_t src_size = src->size();

      if (src_size != length) {
        Python_context::set_python_error(
            PyExc_ValueError, str_format("attempt to assign sequence of size "
                                         "%zd to extended slice of size %zd",
                                         src_size, length));

        return -1;
      }

      if (!length) {
        return 0;
      }

      const auto dst = self->array->get();

      for (Py_ssize_t cur = start, i = 0; i < length;
           cur += (size_t)step, ++i) {
        (*dst)[cur] = (*src)[i];
      }
    } else {
      if (length <= 0) {
        return 0;
      }

      if (step < 0) {
        stop = start + 1;
        start = stop + step * (length - 1) - 1;
        step = -step;
      }

      const auto array = self->array->get();
      Py_ssize_t idx = 0;

      // in range [start, stop), move 0, step, 2*step, ... element to the end of
      // that range, erase moved elements
      assert(step > 0);
      array->erase(std::remove_if(array->begin() + start, array->begin() + stop,
                                  [&idx, step](const auto &) {
                                    return 0 == (idx++ % step);
                                  }),
                   array->begin() + stop);
    }

    return 0;
  } else {
    Python_context::set_python_error(
        PyExc_TypeError,
        str_format("list indices must be integers or slices, not %.200s",
                   item->ob_type->tp_name));

    return -1;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Array_object_type: methods
////////////////////////////////////////////////////////////////////////////////

PyObject *ao_getstate(Array_object *) {
  return PyLong_FromSize_t(k_magic_state);
}

PyObject *ao_setstate(Array_object *self, PyObject *state) {
  // The pickling/unpickling is handled automatically by Python, it saves all
  // the data (including the contents of our list), however when the contents is
  // restored, it's appended to the underlying list object. Since we're not
  // using it, we need to restore our data. This is done by adding
  // __getstate__() and __setstate__() methods, first one returns a magic
  // number, second - upon receiving that magic number - moves data from the
  // base class back to us.
  const auto magic = PyLong_AsSize_t(state);

  if (PyErr_Occurred()) {
    return nullptr;
  }

  if (k_magic_state != magic) {
    Python_context::set_python_error(PyExc_RuntimeError,
                                     "Wrong magic number used");
    return nullptr;
  }

  // our base class holds all the items, copy it back to us
  const auto array = self->array->get();
  const auto list = (PyObject *)self;
  const auto size = PyList_Size(list);

  array->reserve(size);

  for (Py_ssize_t i = 0; i < size; ++i) {
    py::Release result{ao_append(self, PyList_GetItem(list, i))};
    if (!result) return nullptr;
  }

  // delete the contents of the base class
  PyList_SetSlice(list, 0, size, nullptr);

  Py_RETURN_NONE;
}

PyObject *ao_sizeof(Array_object *self) {
  return PyLong_FromSsize_t(_PyObject_SIZE(Py_TYPE(self)));
}

/**
 * Add an item to the end of the list. Equivalent to a[len(a):] = [x].
 */
PyObject *ao_append(Array_object *self, PyObject *v) {
  try {
    self->array->get()->push_back(py::convert(v));
    Py_RETURN_NONE;
  } catch (const std::exception &exc) {
    Python_context::set_python_error(PyExc_RuntimeError, exc.what());
  }

  return nullptr;
}

/**
 * Return a shallow copy of the list. Equivalent to a[:].
 */
PyObject *ao_copy(Array_object *self) {
  return ao_slice(self, 0, ao_length(self)).release();
}

/**
 * Return the number of times x appears in the list.
 */
PyObject *ao_count(Array_object *self, PyObject *v) {
  try {
    const auto array = self->array->get();

    return PyLong_FromSsize_t(
        std::count(array->begin(), array->end(), py::convert(v)));
  } catch (const std::exception &exc) {
    Python_context::set_python_error(PyExc_RuntimeError, exc.what());
  }

  return nullptr;
}

/**
 * Extend the list by appending all the items from the iterable. Equivalent to
 * a[len(a):] = iterable.
 */
PyObject *ao_extend(Array_object *self, PyObject *other) {
  {
    // fail if other is not iterable
    py::Release it{PyObject_GetIter(other)};
    if (!it) return nullptr;
  }

  try {
    const auto right = to_sequence(self, other, "argument must be iterable");

    if (right) {
      const auto array = self->array->get();

      array->reserve(array->size() + right->size());
      array->insert(array->end(), right->begin(), right->end());

      Py_RETURN_NONE;
    }
  } catch (const std::exception &exc) {
    Python_context::set_python_error(PyExc_RuntimeError, exc.what());
  }

  return nullptr;
}

/**
 * Return zero-based index in the list of the first item whose value is equal to
 * x. Raises a ValueError if there is no such item.
 *
 * The optional arguments start and end are interpreted as in the slice notation
 * and are used to limit the search to a particular subsequence of the list. The
 * returned index is computed relative to the beginning of the full sequence
 * rather than the start argument.
 */
PyObject *ao_index(Array_object *self, PyObject *args) {
  const auto size = ao_length(self);

  PyObject *v = nullptr;
  Py_ssize_t start = 0;
  Py_ssize_t stop = size;

  if (!PyArg_ParseTuple(args, "O|O&O&:index", &v, _PyEval_SliceIndexNotNone,
                        &start, _PyEval_SliceIndexNotNone, &stop)) {
    return nullptr;
  }

  if (start < 0) {
    start += size;

    if (start < 0) {
      start = 0;
    }
  }

  if (stop < 0) {
    stop += size;

    if (stop < 0) {
      stop = 0;
    }
  }

  if (stop > size) {
    stop = size;
  }

  try {
    if (start < stop) {
      const auto array = self->array->get();
      const auto end = array->begin() + stop;
      const auto idx = std::find(array->begin() + start, end, py::convert(v));

      if (end != idx) {
        return PyLong_FromSsize_t(std::distance(array->begin(), idx));
      }
    }

    py::Release repr{PyObject_Repr(v)};
    std::string repr_str;

    Python_context::pystring_to_string(repr, &repr_str);
    Python_context::set_python_error(PyExc_ValueError,
                                     repr_str + " is not in list");
  } catch (const std::exception &exc) {
    Python_context::set_python_error(PyExc_RuntimeError, exc.what());
  }

  return nullptr;
}

/**
 * Insert an item at a given position. The first argument is the index of the
 * element before which to insert, so a.insert(0, x) inserts at the front of the
 * list, and a.insert(len(a), x) is equivalent to a.append(x).
 */
PyObject *ao_insert(Array_object *self, PyObject *args) {
  Py_ssize_t where = 0;
  PyObject *value = nullptr;

  if (PyArg_ParseTuple(args, "nO:insert", &where, &value)) {
    try {
      const auto array = self->array->get();
      const auto size = ao_length(self);

      if (where < 0) {
        where += size;

        if (where < 0) {
          where = 0;
        }
      }

      if (where > size) {
        where = size;
      }

      array->insert(array->begin() + where, py::convert(value));

      Py_RETURN_NONE;
    } catch (const std::exception &exc) {
      Python_context::set_python_error(PyExc_RuntimeError, exc.what());
    }
  }

  return nullptr;
}

/**
 * Remove the item at the given position in the list, and return it. If no index
 * is specified, a.pop() removes and returns the last item in the list.
 *
 * Raises IndexError if list is empty or index is out of range.
 */
PyObject *ao_pop(Array_object *self, PyObject *args) {
  Py_ssize_t i = -1;

  if (!PyArg_ParseTuple(args, "|n:pop", &i)) {
    return nullptr;
  }

  const auto size = ao_length(self);

  if (0 == size) {
    // Special-case most common failure cause
    Python_context::set_python_error(PyExc_IndexError, "pop from empty list");
    return nullptr;
  }

  if (i < 0) {
    i += size;
  }

  if (i < 0 || i >= size) {
    Python_context::set_python_error(PyExc_IndexError,
                                     "pop index out of range");
    return nullptr;
  }

  try {
    const auto array = self->array->get();
    const auto value = (*array)[i];

    array->erase(array->begin() + i);

    return py::convert(value).release();
  } catch (const std::exception &exc) {
    Python_context::set_python_error(PyExc_RuntimeError, exc.what());
  }

  return nullptr;
}

/**
 * Remove the first item from the list whose value is equal to x. It raises a
 * ValueError if there is no such item.
 */
PyObject *ao_remove(Array_object *self, PyObject *v) {
  try {
    const auto array = self->array->get();
    const auto size = array->size();

    array->erase(std::remove(array->begin(), array->end(), py::convert(v)),
                 array->end());

    if (size != array->size()) {
      Py_RETURN_NONE;
    } else {
      Python_context::set_python_error(PyExc_ValueError,
                                       "list.remove(x): x not in list");
    }
  } catch (const std::exception &exc) {
    Python_context::set_python_error(PyExc_RuntimeError, exc.what());
  }

  return nullptr;
}

PyObject *ao_remove_all(Array_object *self) {
  self->array->get()->clear();
  Py_RETURN_NONE;
}

/**
 * Reverse the elements of the list in place.
 */
PyObject *ao_reverse(Array_object *self) {
  const auto array = self->array->get();

  if (array->size() > 1) {
    std::reverse(array->begin(), array->end());
  }

  Py_RETURN_NONE;
}

/**
 * Sort the items of the list in place. Stable sort.
 *
 * key specifies a function of one argument that is used to extract a comparison
 * key from each element in iterable (for example, key=str.lower). The default
 * value is None (compare the elements directly).
 *
 * reverse is a boolean value. If set to True, then the list elements are sorted
 * as if each comparison were reversed.
 */
PyObject *ao_sort(Array_object *self, PyObject *args, PyObject *kwds) {
  static char *kwlist[] = {const_cast<char *>("key"),
                           const_cast<char *>("reverse"), 0};

  int reverse = 0;
  PyObject *keyfunc = nullptr;

  if (args) {
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Oi:sort", kwlist, &keyfunc,
                                     &reverse)) {
      return nullptr;
    }

    if (Py_SIZE(args) > 0) {
      Python_context::set_python_error(
          PyExc_TypeError, "must use keyword argument for key function");
      return nullptr;
    }
  }

  if (Py_None == keyfunc) {
    keyfunc = nullptr;
  }

  struct Item {
    Value key;
    Value value;
  };

  const auto array = self->array->get();
  std::vector<Item> items{array->size()};

  if (keyfunc) {
    // key function was given, invoke it for all elements, items will be sorted
    // using results of the invocations
    try {
      Python_context *ctx = nullptr;
      decltype(items)::size_type idx = 0;

      for (const auto &v : (*array)) {
        const auto conv = py::convert(v);
        py::Release key{
            PyObject_CallFunctionObjArgs(keyfunc, conv.get(), nullptr)};

        if (!key) return nullptr;

        items[idx++].key = py::convert(key.get(), &ctx);
      }
    } catch (const std::exception &exc) {
      Python_context::set_python_error(PyExc_RuntimeError, exc.what());
      return nullptr;
    }

    // move all elements to the vector which is going to be sorted
    //
    // this is done after all elements are converted, so we don't end up with
    // partially modified array in case of any errors
    decltype(items)::size_type idx = 0;

    for (auto &v : (*array)) {
      items[idx++].value = std::move(v);
    }
  } else {
    // no key function - items will be sorted using actual values
    decltype(items)::size_type idx = 0;

    for (auto &v : (*array)) {
      items[idx++].key = std::move(v);
    }
  }

  // Reverse sort stability achieved by initially reversing the list, applying a
  // stable forward sort, then reversing the final result.
  if (reverse) {
    std::reverse(items.begin(), items.end());
  }

  std::stable_sort(items.begin(), items.end(),
                   [](const auto &l, const auto &r) { return l.key < r.key; });

  // reverse the results
  if (reverse) {
    std::reverse(items.begin(), items.end());
  }

  // move items back to the original array
  decltype(items)::size_type idx = 0;

  for (auto &i : items) {
    (*array)[idx++] = std::move(keyfunc ? i.value : i.key);
  }

  Py_RETURN_NONE;
}

////////////////////////////////////////////////////////////////////////////////
// Array_object_type
////////////////////////////////////////////////////////////////////////////////

PyObject *ao_new(PyTypeObject *type, PyObject *args, PyObject *kw) {
  Array_object *self = (Array_object *)PyType_GenericNew(type, args, kw);

  self->array = new Array_t(make_array());

  return reinterpret_cast<PyObject *>(self);
}

int ao_init(Array_object *self, PyObject *args, PyObject *kw) {
  {
    // we don't want the underlying list to store anything, so we're
    // using arguments to initialize an empty list
    py::Release list_args{PyTuple_New(0)};

    if (PyList_Type.tp_init((PyObject *)self, list_args.get(), nullptr) < 0) {
      return -1;
    }
  }

  PyObject *arg = nullptr;

  if (!PyArg_ParseTuple(args, "|O:List", &arg)) {
    return -1;
  }

  if (!_PyArg_NoKeywords("List", kw)) {
    return -1;
  }

  delete self->array;
  self->array = new Array_t(make_array());

  if (arg) {
    py::Release result{ao_extend(self, arg)};
    if (!result) return -1;
  }

  return 0;
}

void ao_dealloc(Array_object *self) {
  delete std::exchange(self->array, nullptr);

  Py_TYPE(self)->tp_free(self);
}

PyObject *ao_repr(Array_object *self) {
  return PyString_FromString(Value(*self->array).repr().c_str());
}

PyObject *ao_str(Array_object *self) {
  return PyString_FromString(Value(*self->array).descr().c_str());
}

int ao_gc_traverse(PyObject *, visitproc, void *) {
  // used to override traverse function from Python's list
  // empty, as we don't hold any Python objects
  return 0;
}

int ao_gc_clear(Array_object *) {
  // used to override clear function from Python's list
  // empty, as we don't hold any Python objects
  return 0;
}

PyObject *ao_richcompare(Array_object *self, PyObject *right, int op) {
  if (!PyList_Check(right)) {
    Py_RETURN_NOTIMPLEMENTED;
  }

  const auto r = to_array(right);
  const auto sr = r->size();
  const auto l = self->array->get();
  const auto sl = l->size();

  const auto convert = [](bool b) {
    auto res = b ? Py_True : Py_False;
    Py_INCREF(res);
    return res;
  };

  if (sr != sl && (Py_EQ == op || Py_NE == op)) {
    // lengths differ, lists differ
    return convert(Py_NE == op);
  }

  Value::Array_type::size_type i = 0;

  for (i = 0; i < sl && i < sr; ++i) {
    // find index where items are different
    if ((*l)[i] != (*r)[i]) {
      break;
    }
  }

  bool result = false;

  if (i >= sl || i >= sr) {
    // no more items - compare sizes
    switch (op) {
      case Py_LT:
        result = sl < sr;
        break;

      case Py_LE:
        result = sl <= sr;
        break;

      case Py_EQ:
        result = sl == sr;
        break;

      case Py_NE:
        result = sl != sr;
        break;

      case Py_GT:
        result = sl > sr;
        break;

      case Py_GE:
        result = sl >= sr;
        break;

      default:
        return NULL;  // cannot happen
    }
  } else {
    // we have an item that differs
    const auto &vl = (*l)[i];
    const auto &vr = (*r)[i];

    switch (op) {
      case Py_LT:
        result = vl < vr;
        break;

      case Py_LE:
        result = vl <= vr;
        break;

      case Py_EQ:
        Py_RETURN_FALSE;

      case Py_NE:
        Py_RETURN_TRUE;

      case Py_GT:
        result = vl > vr;
        break;

      case Py_GE:
        result = vl >= vr;
        break;

      default:
        return NULL;  // cannot happen
    }
  }

  return convert(result);
}

////////////////////////////////////////////////////////////////////////////////
// Array_object_iterator
////////////////////////////////////////////////////////////////////////////////

PyObject *ao_iter(Array_object *self) {
  Array_object_iterator *it =
      PyObject_New(Array_object_iterator, &Array_object_iterator_type);

  it->index = 0;
  Py_INCREF(self);
  it->o = self;

  return reinterpret_cast<PyObject *>(it);
}

void ao_iter_dealloc(Array_object_iterator *self) {
  Py_XDECREF(self->o);
  PyObject_Del(self);
}

PyObject *ao_iter_next(Array_object_iterator *self) {
  if (!self->o) return nullptr;

  if (self->index < ao_length(self->o)) {
    try {
      auto item = py::convert(self->o->array->get()->at(self->index));
      ++self->index;
      return item.release();
    } catch (const std::exception &exc) {
      Python_context::set_python_error(PyExc_RuntimeError, exc.what());
    }
  } else {
    Py_DECREF(self->o);
    self->o = nullptr;
  }

  return nullptr;
}

PyObject *ao_iter_length(Array_object_iterator *self) {
  Py_ssize_t length = 0;

  if (self->o) {
    const auto len = ao_length(self->o) - self->index;

    if (len > 0) {
      length = len;
    }
  }

  return PyLong_FromSsize_t(length);
}

PyObject *ao_iter_reduce(Array_object_iterator *self) {
  if (self->o)
    return Py_BuildValue("N(O)n", get_builtin("iter").release(), self->o,
                         self->index);

  // empty iterator, create empty list
  return Py_BuildValue("N(N)", get_builtin("iter").release(),
                       wrap(make_array()).release());
}

PyObject *ao_iter_setstate(Array_object_iterator *self, PyObject *state) {
  auto index = PyLong_AsSsize_t(state);

  if (-1 == index && PyErr_Occurred()) {
    return nullptr;
  }

  if (self->o) {
    if (index < 0) {
      index = 0;
    } else if (index > ao_length(self->o)) {
      index = ao_length(self->o);  // iterator exhausted
    }

    self->index = index;
  }

  Py_RETURN_NONE;
}

////////////////////////////////////////////////////////////////////////////////
// Array_object_iterator
////////////////////////////////////////////////////////////////////////////////

PyObject *ao_riter(Array_object *self) {
  Array_object_riterator *it =
      PyObject_New(Array_object_riterator, &Array_object_riterator_type);

  it->index = ao_length(self) - 1;
  Py_INCREF(self);
  it->o = self;

  return reinterpret_cast<PyObject *>(it);
}

void ao_riter_dealloc(Array_object_riterator *self) {
  Py_XDECREF(self->o);
  PyObject_Del(self);
}

PyObject *ao_riter_next(Array_object_riterator *self) {
  if (!self->o) {
    return nullptr;
  }

  if (self->index >= 0 && self->index < ao_length(self->o)) {
    try {
      auto item = py::convert(self->o->array->get()->at(self->index));
      --self->index;
      return item.release();
    } catch (const std::exception &exc) {
      Python_context::set_python_error(PyExc_RuntimeError, exc.what());
    }
  } else {
    Py_DECREF(self->o);
    self->o = nullptr;
    self->index = -1;
  }

  return nullptr;
}

PyObject *ao_riter_length(Array_object_riterator *self) {
  Py_ssize_t length = self->index + 1;

  if (!self->o || ao_length(self->o) < length) {
    length = 0;
  }

  return PyLong_FromSsize_t(length);
}

PyObject *ao_riter_reduce(Array_object_riterator *self) {
  if (self->o)
    return Py_BuildValue("N(O)n", get_builtin("reversed").release(), self->o,
                         self->index);

  // empty iterator, create empty list, iter() is used on purpose
  return Py_BuildValue("N(N)", get_builtin("iter").release(),
                       wrap(make_array()).release());
}

PyObject *ao_riter_setstate(Array_object_riterator *self, PyObject *state) {
  auto index = PyLong_AsSsize_t(state);

  if (-1 == index && PyErr_Occurred()) {
    return nullptr;
  }

  if (self->o) {
    if (index < -1) {
      index = -1;
    } else if (index > ao_length(self->o) - 1) {
      index = ao_length(self->o) - 1;
    }

    self->index = index;
  }

  Py_RETURN_NONE;
}

}  // namespace

void Python_context::init_shell_list_type() {
  // this has to be done at runtime
  Array_object_type.tp_base = &PyList_Type;

  if (PyType_Ready(&Array_object_type) < 0) {
    throw Exception::runtime_error(
        "Could not initialize Shcore Array type in python");
  }

  replace_list_concat();

  Py_INCREF(&Array_object_type);

  const auto module = get_shell_python_support_module();

  PyModule_AddObject(module.get(), "List",
                     reinterpret_cast<PyObject *>(&Array_object_type));

  _shell_list_class =
      py::Store{PyDict_GetItemString(PyModule_GetDict(module.get()), "List")};
}

bool array_check(PyObject *value) {
  return Py_TYPE(value) == &Array_object_type;
}

py::Release wrap(const std::shared_ptr<Value::Array_type> &array) {
  Array_object *wrapper = PyObject_New(Array_object, &Array_object_type);

  // tp_new is not used by PyObject_New, array needs to be allocated
  wrapper->array = new Array_t(array);

  // PyObject_New only initializes Python's object fields, zero-initialize base
  // list object
  wrapper->base.ob_item = nullptr;
  Py_SIZE(&wrapper->base) = 0;
  wrapper->base.allocated = 0;

  return py::Release{reinterpret_cast<PyObject *>(wrapper)};
}

bool unwrap(PyObject *value, std::shared_ptr<Value::Array_type> *ret_object) {
  if (const auto ctx = Python_context::get_and_check()) {
    const auto lclass = ctx->get_shell_list_class();

    if (PyObject_IsInstance(value, lclass.get())) {
      const auto array = ((Array_object *)value)->array;

      assert(array);

      *ret_object = *array;

      return true;
    }
  }

  return false;
}

}  // namespace shcore
