/*
 * Copyright (c) 2015, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "scripting/python_map_wrapper.h"

#include <string>
#include <type_traits>

#include "mysqlshdk/libs/utils/utils_general.h"

namespace shcore {

namespace {

namespace dict {

////////////////////////////////////////////////////////////////////////////////
// dict::Object
////////////////////////////////////////////////////////////////////////////////

struct Object {
  PyDictObject base;
  Dictionary_t *dict = nullptr;
};

////////////////////////////////////////////////////////////////////////////////
// dict::Iterator
////////////////////////////////////////////////////////////////////////////////

template <typename It>
struct Iterator_base {
  // clang-format off
  PyObject_HEAD
  py::Store o;
  // clang-format on
  size_t initial_size = 0;
  It next;
};

using Map_value_type = shcore::Value::Map_type::value_type;

using Map_iterator = shcore::Value::Map_type::const_iterator;
using Map_reverse_iterator = shcore::Value::Map_type::const_reverse_iterator;

using Iterator = Iterator_base<Map_iterator>;
using Reverse_iterator = Iterator_base<Map_reverse_iterator>;

////////////////////////////////////////////////////////////////////////////////
// dict::View
////////////////////////////////////////////////////////////////////////////////

struct View {
  // clang-format off
  PyObject_HEAD
  py::Store o;
  // clang-format on
};

////////////////////////////////////////////////////////////////////////////////
// dict::Type - number protocol
////////////////////////////////////////////////////////////////////////////////

PyObject *nb_or(PyObject *self, PyObject *other);
PyObject *inplace_or(PyObject *self, PyObject *other);

PyNumberMethods as_number = {
    0,           // binaryfunc nb_add;
    0,           // binaryfunc nb_subtract;
    0,           // binaryfunc nb_multiply;
    0,           // binaryfunc nb_remainder;
    0,           // binaryfunc nb_divmod;
    0,           // ternaryfunc nb_power;
    0,           // unaryfunc nb_negative;
    0,           // unaryfunc nb_positive;
    0,           // unaryfunc nb_absolute;
    0,           // inquiry nb_bool;
    0,           // unaryfunc nb_invert;
    0,           // binaryfunc nb_lshift;
    0,           // binaryfunc nb_rshift;
    0,           // binaryfunc nb_and;
    0,           // binaryfunc nb_xor;
    nb_or,       // binaryfunc nb_or;
    0,           // unaryfunc nb_int;
    0,           // void *nb_reserved;
    0,           // unaryfunc nb_float;
    0,           // binaryfunc nb_inplace_add;
    0,           // binaryfunc nb_inplace_subtract;
    0,           // binaryfunc nb_inplace_multiply;
    0,           // binaryfunc nb_inplace_remainder;
    0,           // ternaryfunc nb_inplace_power;
    0,           // binaryfunc nb_inplace_lshift;
    0,           // binaryfunc nb_inplace_rshift;
    0,           // binaryfunc nb_inplace_and;
    0,           // binaryfunc nb_inplace_xor;
    inplace_or,  // binaryfunc nb_inplace_or;
    0,           // binaryfunc nb_floor_divide;
    0,           // binaryfunc nb_true_divide;
    0,           // binaryfunc nb_inplace_floor_divide;
    0,           // binaryfunc nb_inplace_true_divide;
    0,           // unaryfunc nb_index;
    0,           // binaryfunc nb_matrix_multiply;
    0,           // binaryfunc nb_inplace_matrix_multiply;
};

////////////////////////////////////////////////////////////////////////////////
// dict::Type - sequence protocol
////////////////////////////////////////////////////////////////////////////////

int contains(dict::Object *self, PyObject *key);

PySequenceMethods as_sequence = {
    0,                     // lenfunc sq_length;
    0,                     // binaryfunc sq_concat;
    0,                     // ssizeargfunc sq_repeat;
    0,                     // ssizeargfunc sq_item;
    0,                     // void *was_sq_slice;
    0,                     // ssizeobjargproc sq_ass_item;
    0,                     // void *was_sq_ass_slice;
    (objobjproc)contains,  // objobjproc sq_contains;
    0,                     // binaryfunc sq_inplace_concat;
    0                      // ssizeargfunc sq_inplace_repeat;
};

////////////////////////////////////////////////////////////////////////////////
// dict::Type - mapping protocol
////////////////////////////////////////////////////////////////////////////////

Py_ssize_t length(dict::Object *self);
PyObject *subscript(dict::Object *self, PyObject *key);
int assign_subscript(dict::Object *self, PyObject *key, PyObject *value);

PyMappingMethods as_mapping = {
    (lenfunc)length,                 // lenfunc mp_length;
    (binaryfunc)subscript,           // binaryfunc mp_subscript;
    (objobjargproc)assign_subscript  // objobjargproc mp_ass_subscript;
};

////////////////////////////////////////////////////////////////////////////////
// dict::Type - methods
////////////////////////////////////////////////////////////////////////////////

PyDoc_STRVAR(contains_doc,
             "D.__contains__(key) -- True if D has a key k, else False.");
PyDoc_STRVAR(dir_doc, "D.__dir__() -- returns list of valid attributes");
PyDoc_STRVAR(get_item_doc, "D.__getitem__(y) <==> D[y]");
PyDoc_STRVAR(
    reversed_doc,
    "D.__reversed__() -- return a reverse iterator over the dict keys.");
PyDoc_STRVAR(sizeof_doc, "D.__sizeof__() -> size of D in memory, in bytes");
PyDoc_STRVAR(clear_doc, "D.clear() -> None.  Remove all items from D.");
PyDoc_STRVAR(copy_doc, "D.copy() -> a shallow copy of D");
PyDoc_STRVAR(from_keys_doc,
             "fromkeys($type, iterable, value=None) -- Returns a new dict with "
             "keys from iterable and values equal to value.");
PyDoc_STRVAR(get_doc,
             "D.get(k[,d]) -> D[k] if k in D, else d.  d defaults to None.");
PyDoc_STRVAR(has_key_doc,
             "D.has_key(key) -- True if D has a key k, else False.");
PyDoc_STRVAR(items_doc,
             "D.items() -> a set-like object providing a view on D's items");
PyDoc_STRVAR(keys_doc,
             "D.keys() -> a set-like object providing a view on D's keys");
PyDoc_STRVAR(pop_doc,
             "D.pop(k[,d]) -> v, remove specified key and return the "
             "corresponding value.\nIf key is not found, d is returned if "
             "given, otherwise KeyError is raised");
PyDoc_STRVAR(pop_item_doc,
             "D.popitem() -> (k, v), remove and return some (key, value) pair "
             "as a\n2-tuple; but raise KeyError if D is empty.");
PyDoc_STRVAR(
    set_default_doc,
    "D.setdefault(k[,d]) -> D.get(k,d), also set D[k]=d if k not in D");
PyDoc_STRVAR(update_doc,
             "D.update([E, ]**F) -> None.  Update D from dict/iterable E and "
             "F.\nIf E is present and has a .keys() method, then does:  for k "
             "in E: D[k] = E[k]\nIf E is present and lacks a .keys() method, "
             "then does:  for k, v in E: D[k] = v\nIn either case, this is "
             "followed by: for k in F:  D[k] = F[k]");
PyDoc_STRVAR(values_doc,
             "D.values() -> an object providing a view on D's values");

PyObject *dir(dict::Object *self);
PyObject *reversed(dict::Object *self, PyObject *);
PyObject *size_of(dict::Object *self);
PyObject *clear(dict::Object *self);
PyObject *copy(dict::Object *self);
PyObject *from_keys(PyTypeObject *type, PyObject *args);
PyObject *get(dict::Object *self, PyObject *args);
PyObject *has_key(dict::Object *self, PyObject *key);
PyObject *items(dict::Object *self);
PyObject *keys(dict::Object *self);
PyObject *pop(dict::Object *self, PyObject *args);
PyObject *pop_item(dict::Object *self);
PyObject *set_default(dict::Object *self, PyObject *args);
PyObject *update(dict::Object *self, PyObject *args, PyObject *kwds);
PyObject *values(dict::Object *self);

#if __GNUC__ > 7 && !defined(__clang__)
// -Wcast-function-type was added in GCC 8.0, needs to be suppressed here,
// because we're casting functions with different number of parameters to
// PyCFunction
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif  // __GNUC__ > 7 && !defined(__clang__)

PyMethodDef methods[] = {
    {"__contains__", (PyCFunction)has_key, METH_O | METH_COEXIST, contains_doc},
    {"__dir__", (PyCFunction)dir, METH_NOARGS, dir_doc},
    {"__getitem__", (PyCFunction)subscript, METH_O | METH_COEXIST,
     get_item_doc},
    {"__reversed__", (PyCFunction)reversed, METH_NOARGS, reversed_doc},
    {"__sizeof__", (PyCFunction)size_of, METH_NOARGS, sizeof_doc},
    {"clear", (PyCFunction)clear, METH_NOARGS, clear_doc},
    {"copy", (PyCFunction)copy, METH_NOARGS, copy_doc},
    {"fromkeys", (PyCFunction)from_keys, METH_VARARGS | METH_CLASS,
     from_keys_doc},
    {"get", (PyCFunction)get, METH_VARARGS, get_doc},
    {"has_key", (PyCFunction)has_key, METH_O, has_key_doc},
    {"items", (PyCFunction)items, METH_NOARGS, items_doc},
    {"keys", (PyCFunction)keys, METH_NOARGS, keys_doc},
    {"pop", (PyCFunction)pop, METH_VARARGS, pop_doc},
    {"popitem", (PyCFunction)pop_item, METH_NOARGS, pop_item_doc},
    {"setdefault", (PyCFunction)set_default, METH_VARARGS, set_default_doc},
    {"update", (PyCFunction)update, METH_VARARGS | METH_KEYWORDS, update_doc},
    {"values", (PyCFunction)values, METH_NOARGS, values_doc},
    {nullptr, nullptr, 0, nullptr},
};

#if __GNUC__ > 7 && !defined(__clang__)
#pragma GCC diagnostic pop
#endif  // __GNUC__ > 7 && !defined(__clang__)

////////////////////////////////////////////////////////////////////////////////
// dict::Type
////////////////////////////////////////////////////////////////////////////////

PyDoc_STRVAR(dict_object_doc,
             "Dict() -> new empty shcore dictionary\n"
             "\n"
             "Creates a new instance of a shcore dictionary object.");

void dealloc(dict::Object *self);
PyObject *repr(dict::Object *self);
PyObject *str(dict::Object *self);
PyObject *get_attribute(dict::Object *self, PyObject *name);
int gc_traverse(PyObject *, visitproc, void *);
int gc_clear(PyObject *);
PyObject *richcompare(dict::Object *self, PyObject *right, int op);
PyObject *iterator(dict::Object *self);
int init(dict::Object *self, PyObject *args, PyObject *kwds);
PyObject *create(PyTypeObject *type, PyObject *args, PyObject *kwds);

#if PY_VERSION_HEX >= 0x03080000 && PY_VERSION_HEX < 0x03090000
#ifdef __clang__
// The tp_print is marked as deprecated, which makes clang unhappy, 'cause it's
// initialized below. Skipping initialization also makes clang unhappy, so we're
// disabling the deprecated declarations warning.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif  // __clang__
#endif  // PY_VERSION_HEX

PyTypeObject Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)  // PyObject_VAR_HEAD
    // full name including the module is required for pickle support
    "mysqlsh.__shell_python_support__.Dict",  // const char *tp_name;

    /* For allocation */

    sizeof(dict::Object),  // Py_ssize_t tp_basicsize;
    0,                     // Py_ssize_t tp_itemsize;

    /* Methods to implement standard operations */

    (destructor)dict::dealloc,  // destructor tp_dealloc;
    0,                          // printfunc tp_print;
    0,                          // getattrfunc tp_getattr;
    0,                          // setattrfunc tp_setattr;
    0,                          // cmpfunc tp_compare;
    (reprfunc)dict::repr,       // reprfunc tp_repr;

    /* Method suites for standard classes */

    &dict::as_number,    //  PyNumberMethods *tp_as_number;
    &dict::as_sequence,  //  PySequenceMethods *tp_as_sequence;
    &dict::as_mapping,   //  PyMappingMethods *tp_as_mapping;

    /* More standard operations (here for binary compatibility) */

    0,                                  // hashfunc tp_hash;
    0,                                  // ternaryfunc tp_call;
    (reprfunc)dict::str,                // reprfunc tp_str;
    (getattrofunc)dict::get_attribute,  // getattrofunc tp_getattro;
    0,                                  // setattrofunc tp_setattro;

    /* Functions to access object as input/output buffer */
    0,  // PyBufferProcs *tp_as_buffer;

    /* Flags to define presence of optional/expanded features */
    Py_TPFLAGS_DEFAULT,  // long tp_flags;

    dict_object_doc,  // char *tp_doc; /* Documentation string */

    /* Assigned meaning in release 2.0 */
    /* call function for all accessible objects */
    dict::gc_traverse,  // traverseproc tp_traverse;

    /* delete references to contained objects */
    dict::gc_clear,  // inquiry tp_clear;

    /* Assigned meaning in release 2.1 */
    /* rich comparisons */
    (richcmpfunc)dict::richcompare,  // richcmpfunc tp_richcompare;

    /* weak reference enabler */
    0,  // long tp_weaklistoffset;

    /* Added in release 2.2 */
    /* Iterators */
    (getiterfunc)dict::iterator,  // getiterfunc tp_iter;
    0,                            // iternextfunc tp_iternext;

    /* Attribute descriptor and subclassing stuff */
    dict::methods,         // struct PyMethodDef *tp_methods;
    0,                     // struct PyMemberDef *tp_members;
    0,                     // struct PyGetSetDef *tp_getset;
    0,                     // struct _typeobject *tp_base;
    0,                     // PyObject *tp_dict;
    0,                     // descrgetfunc tp_descr_get;
    0,                     // descrsetfunc tp_descr_set;
    0,                     // long tp_dictoffset;
    (initproc)dict::init,  // initproc tp_init;
    PyType_GenericAlloc,   // allocfunc tp_alloc;
    dict::create,          // newfunc tp_new;
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
    0  // tp_vectorcall
#if PY_VERSION_HEX < 0x03090000
    ,
    0  // tp_print
#endif
#endif
#if PY_VERSION_HEX >= 0x030C0000
    ,
    0  // tp_watched
#endif
#if PY_VERSION_HEX >= 0x030D0000
    ,
    0  // tp_versions_used
#endif
};

#if PY_VERSION_HEX >= 0x03080000 && PY_VERSION_HEX < 0x03090000
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__
#endif  // PY_VERSION_HEX

////////////////////////////////////////////////////////////////////////////////
// dict::Iterator - common helper functions
////////////////////////////////////////////////////////////////////////////////

namespace iter {

Map_iterator begin(dict::Iterator *self);
Map_reverse_iterator begin(dict::Reverse_iterator *self);

Map_iterator end(dict::Iterator *self);
Map_reverse_iterator end(dict::Reverse_iterator *self);

template <typename It>
PyObject *create(dict::Object *dict, PyTypeObject *type);

template <typename It>
void dealloc(dict::Iterator_base<It> *self);

using convert_t = PyObject *(*)(const Map_value_type &);

template <typename It>
PyObject *next(dict::Iterator_base<It> *self, convert_t convert);

#define DEFINE_ITERATOR(tp_name, it)                                        \
  PyTypeObject Type = {                                                     \
      PyVarObject_HEAD_INIT(&PyType_Type, 0) /* PyObject_VAR_HEAD */        \
      tp_name,                               /* const char *tp_name; */     \
      sizeof(dict::Iterator_base<it>),       /* Py_ssize_t tp_basicsize; */ \
      0,                                     /* Py_ssize_t tp_itemsize; */  \
      (destructor)dict::iter::dealloc<it>,   /* destructor tp_dealloc; */   \
      0,                                     /* printfunc tp_print; */      \
      0,                                     /* getattrfunc tp_getattr; */  \
      0,                                     /* setattrfunc tp_setattr; */  \
      0,                                     /* cmpfunc tp_compare; */      \
      0,                                     /* reprfunc tp_repr; */        \
      0,                       /*  PyNumberMethods *tp_as_number; */        \
      0,                       /*  PySequenceMethods *tp_as_sequence; */    \
      0,                       /*  PyMappingMethods *tp_as_mapping; */      \
      0,                       /* hashfunc tp_hash; */                      \
      0,                       /* ternaryfunc tp_call; */                   \
      0,                       /* reprfunc tp_str; */                       \
      PyObject_GenericGetAttr, /* getattrofunc tp_getattro; */              \
      0,                       /* setattrofunc tp_setattro; */              \
      0,                       /* PyBufferProcs *tp_as_buffer; */           \
      Py_TPFLAGS_DEFAULT,      /* long tp_flags; */                         \
      0,                       /* char *tp_doc; */                          \
      0,                       /* traverseproc tp_traverse; */              \
      0,                       /* inquiry tp_clear; */                      \
      0,                       /* richcmpfunc tp_richcompare; */            \
      0,                       /* long tp_weaklistoffset; */                \
      PyObject_SelfIter,       /* getiterfunc tp_iter; */                   \
      (iternextfunc)next<it>,  /* iternextfunc tp_iternext; */              \
      dict::iter::methods<it>, /* struct PyMethodDef *tp_methods; */        \
      0,                       /* struct PyMemberDef *tp_members; */        \
  }

}  // namespace iter

////////////////////////////////////////////////////////////////////////////////
// dict::Iterator - common type methods
////////////////////////////////////////////////////////////////////////////////

namespace iter {

PyDoc_STRVAR(length_hint_doc,
             "Private method returning an estimate of len(dict(it)).");
PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

template <typename It>
PyObject *length(dict::Iterator_base<It> *self);

template <typename It>
PyObject *reduce(dict::Iterator_base<It> *self);

#if __GNUC__ > 7 && !defined(__clang__)
// -Wcast-function-type was added in GCC 8.0, needs to be suppressed here,
// because we're casting functions with different number of parameters to
// PyCFunction
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif  // __GNUC__ > 7 && !defined(__clang__)

template <typename It>
PyMethodDef methods[] = {
    {"__length_hint__", (PyCFunction)length<It>, METH_NOARGS, length_hint_doc},
    {"__reduce__", (PyCFunction)reduce<It>, METH_NOARGS, reduce_doc},
    {nullptr, nullptr, 0, nullptr},
};

#if __GNUC__ > 7 && !defined(__clang__)
#pragma GCC diagnostic pop
#endif  // __GNUC__ > 7 && !defined(__clang__)

}  // namespace iter

////////////////////////////////////////////////////////////////////////////////
// dict::iter::keys::Type
////////////////////////////////////////////////////////////////////////////////

namespace iter::keys {

PyObject *convert(const Map_value_type &pair);

template <typename It>
PyObject *next(dict::Iterator_base<It> *self);

#ifdef __GNUC__
// We're using macro to initialize the PyTypeObject and skip initialization
// of some of the fields to avoid problems with tp_print, which was added,
// deprecated, and then removed from Python.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif  // __GNUC__

DEFINE_ITERATOR("Dict_keyiterator", Map_iterator);

namespace rev {

DEFINE_ITERATOR("Dict_reversekeyiterator", Map_reverse_iterator);

}  // namespace rev

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  // __GNUC__

}  // namespace iter::keys

////////////////////////////////////////////////////////////////////////////////
// dict::iter::values::Type
////////////////////////////////////////////////////////////////////////////////

namespace iter::values {

PyObject *convert(const Map_value_type &pair);

template <typename It>
PyObject *next(dict::Iterator_base<It> *self);

#ifdef __GNUC__
// We're using macro to initialize the PyTypeObject and skip initialization
// of some of the fields to avoid problems with tp_print, which was added,
// deprecated, and then removed from Python.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif  // __GNUC__

DEFINE_ITERATOR("Dict_valueiterator", Map_iterator);

namespace rev {

DEFINE_ITERATOR("Dict_reversevalueiterator", Map_reverse_iterator);

}  // namespace rev

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  // __GNUC__

}  // namespace iter::values

////////////////////////////////////////////////////////////////////////////////
// dict::iter::items::Type
////////////////////////////////////////////////////////////////////////////////

namespace iter::items {

PyObject *convert(const Map_value_type &pair);

template <typename It>
PyObject *next(dict::Iterator_base<It> *self);

#ifdef __GNUC__
// We're using macro to initialize the PyTypeObject and skip initialization
// of some of the fields to avoid problems with tp_print, which was added,
// deprecated, and then removed from Python.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif  // __GNUC__

DEFINE_ITERATOR("Dict_itemiterator", Map_iterator);

namespace rev {

DEFINE_ITERATOR("Dict_reverseitemiterator", Map_reverse_iterator);

}  // namespace rev

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  // __GNUC__

}  // namespace iter::items

////////////////////////////////////////////////////////////////////////////////
// dict::View - common functions
////////////////////////////////////////////////////////////////////////////////

namespace view {

PyObject *create(dict::Object *dict, PyTypeObject *type);
void dealloc(dict::View *self);

PyObject *subtract(dict::View *self, PyObject *other);
PyObject *nb_and(dict::View *self, PyObject *other);
PyObject *nb_or(dict::View *self, PyObject *other);
PyObject *nb_xor(dict::View *self, PyObject *other);

Py_ssize_t length(dict::View *self);

PyObject *subscript(dict::View *self, PyObject *item,
                    dict::iter::convert_t convert);

}  // namespace view

////////////////////////////////////////////////////////////////////////////////
// dict::view - common number protocol
////////////////////////////////////////////////////////////////////////////////

namespace view {

PyNumberMethods as_number = {
    0,                     // binaryfunc nb_add;
    (binaryfunc)subtract,  // binaryfunc nb_subtract;
    0,                     // binaryfunc nb_multiply;
    0,                     // binaryfunc nb_remainder;
    0,                     // binaryfunc nb_divmod;
    0,                     // ternaryfunc nb_power;
    0,                     // unaryfunc nb_negative;
    0,                     // unaryfunc nb_positive;
    0,                     // unaryfunc nb_absolute;
    0,                     // inquiry nb_bool;
    0,                     // unaryfunc nb_invert;
    0,                     // binaryfunc nb_lshift;
    0,                     // binaryfunc nb_rshift;
    (binaryfunc)nb_and,    // binaryfunc nb_and;
    (binaryfunc)nb_xor,    // binaryfunc nb_xor;
    (binaryfunc)nb_or,     // binaryfunc nb_or;
    0,                     // unaryfunc nb_int;
    0,                     // void *nb_reserved;
    0,                     // unaryfunc nb_float;
    0,                     // binaryfunc nb_inplace_add;
    0,                     // binaryfunc nb_inplace_subtract;
    0,                     // binaryfunc nb_inplace_multiply;
    0,                     // binaryfunc nb_inplace_remainder;
    0,                     // ternaryfunc nb_inplace_power;
    0,                     // binaryfunc nb_inplace_lshift;
    0,                     // binaryfunc nb_inplace_rshift;
    0,                     // binaryfunc nb_inplace_and;
    0,                     // binaryfunc nb_inplace_xor;
    0,                     // binaryfunc nb_inplace_or;
    0,                     // binaryfunc nb_floor_divide;
    0,                     // binaryfunc nb_true_divide;
    0,                     // binaryfunc nb_inplace_floor_divide;
    0,                     // binaryfunc nb_inplace_true_divide;
    0,                     // unaryfunc nb_index;
    0,                     // binaryfunc nb_matrix_multiply;
    0,                     // binaryfunc nb_inplace_matrix_multiply;
};

}  // namespace view

////////////////////////////////////////////////////////////////////////////////
// dict::view - common getters/setters
////////////////////////////////////////////////////////////////////////////////

namespace view {

PyObject *mapping(dict::View *self, void *);

PyGetSetDef getset[] = {{"mapping", (getter)mapping, nullptr,
                         "dictionary that this view refers to", nullptr},
                        {nullptr, nullptr, nullptr, nullptr, nullptr}};

}  // namespace view

////////////////////////////////////////////////////////////////////////////////
// dict::view - common type methods
////////////////////////////////////////////////////////////////////////////////

namespace view {

PyDoc_STRVAR(
    isdisjoint_doc,
    "Return True if the view and the given iterable have a null intersection.");

PyObject *richcompare(dict::View *self, PyObject *other, int op);
PyObject *repr(PyObject *self);
PyObject *isdisjoint(dict::View *self, PyObject *other);

}  // namespace view

////////////////////////////////////////////////////////////////////////////////
// dict::view::keys::Type - sequence protocol
////////////////////////////////////////////////////////////////////////////////

namespace view::keys {

int contains(dict::View *self, PyObject *key);

PySequenceMethods as_sequence = {
    (lenfunc)length,       // lenfunc sq_length;
    0,                     // binaryfunc sq_concat;
    0,                     // ssizeargfunc sq_repeat;
    0,                     // ssizeargfunc sq_item;
    0,                     // void *was_sq_slice;
    0,                     // ssizeobjargproc sq_ass_item;
    0,                     // void *was_sq_ass_slice;
    (objobjproc)contains,  // objobjproc sq_contains;
    0,                     // binaryfunc sq_inplace_concat;
    0                      // ssizeargfunc sq_inplace_repeat;
};

}  // namespace view::keys

////////////////////////////////////////////////////////////////////////////////
// dict::view::keys::Type - mapping protocol
////////////////////////////////////////////////////////////////////////////////

namespace view::keys {

PyObject *subscript(dict::View *self, PyObject *item);

PyMappingMethods as_mapping = {
    (lenfunc)length,        // lenfunc mp_length;
    (binaryfunc)subscript,  // binaryfunc mp_subscript;
    0                       // objobjargproc mp_ass_subscript;
};

}  // namespace view::keys

////////////////////////////////////////////////////////////////////////////////
// dict::view::keys::Type - methods
////////////////////////////////////////////////////////////////////////////////

namespace view::keys {

PyDoc_STRVAR(reversed_keys_doc,
             "Return a reverse iterator over the dict keys.");

PyObject *iterator(dict::View *self);
PyObject *reversed(dict::View *self, PyObject *);

#if __GNUC__ > 7 && !defined(__clang__)
// -Wcast-function-type was added in GCC 8.0, needs to be suppressed here,
// because we're casting functions with different number of parameters to
// PyCFunction
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif  // __GNUC__ > 7 && !defined(__clang__)

PyMethodDef methods[] = {
    {"isdisjoint", (PyCFunction)isdisjoint, METH_O, isdisjoint_doc},
    {"__reversed__", (PyCFunction)reversed, METH_NOARGS, reversed_keys_doc},
    {nullptr, nullptr, 0, nullptr},
};

#if __GNUC__ > 7 && !defined(__clang__)
#pragma GCC diagnostic pop
#endif  // __GNUC__ > 7 && !defined(__clang__)

}  // namespace view::keys

////////////////////////////////////////////////////////////////////////////////
// dict::view::keys::Type
////////////////////////////////////////////////////////////////////////////////

namespace view::keys {

#if PY_VERSION_HEX >= 0x03080000 && PY_VERSION_HEX < 0x03090000
#ifdef __clang__
// The tp_print is marked as deprecated, which makes clang unhappy, 'cause it's
// initialized below. Skipping initialization also makes clang unhappy, so we're
// disabling the deprecated declarations warning.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif  // __clang__
#endif  // PY_VERSION_HEX

PyTypeObject Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)  // PyObject_VAR_HEAD
    "Dict_keys",                            // const char *tp_name;

    /* For allocation */

    sizeof(dict::View),  // Py_ssize_t tp_basicsize;
    0,                   // Py_ssize_t tp_itemsize;

    /* Methods to implement standard operations */

    (destructor)dict::view::dealloc,  // destructor tp_dealloc;
    0,                                // printfunc tp_print;
    0,                                // getattrfunc tp_getattr;
    0,                                // setattrfunc tp_setattr;
    0,                                // cmpfunc tp_compare;
    dict::view::repr,                 // reprfunc tp_repr;

    /* Method suites for standard classes */

    &dict::view::as_number,          //  PyNumberMethods *tp_as_number;
    &dict::view::keys::as_sequence,  //  PySequenceMethods *tp_as_sequence;
    &dict::view::keys::as_mapping,   //  PyMappingMethods *tp_as_mapping;

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
    dict::gc_traverse,  // traverseproc tp_traverse;

    /* delete references to contained objects */
    dict::gc_clear,  // inquiry tp_clear;

    /* Assigned meaning in release 2.1 */
    /* rich comparisons */
    (richcmpfunc)dict::view::richcompare,  // richcmpfunc tp_richcompare;

    /* weak reference enabler */
    0,  // long tp_weaklistoffset;

    /* Added in release 2.2 */
    /* Iterators */
    (getiterfunc)dict::view::keys::iterator,  // getiterfunc tp_iter;
    0,                                        // iternextfunc tp_iternext;

    /* Attribute descriptor and subclassing stuff */
    dict::view::keys::methods,  // struct PyMethodDef *tp_methods;
    0,                          // struct PyMemberDef *tp_members;
    dict::view::getset,         // struct PyGetSetDef *tp_getset;
    0,                          // struct _typeobject *tp_base;
    0,                          // PyObject *tp_dict;
    0,                          // descrgetfunc tp_descr_get;
    0,                          // descrsetfunc tp_descr_set;
    0,                          // long tp_dictoffset;
    0,                          // initproc tp_init;
    0,                          // allocfunc tp_alloc;
    0,                          // newfunc tp_new;
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
    0  // tp_vectorcall
#if PY_VERSION_HEX < 0x03090000
    ,
    0  // tp_print
#endif
#endif
#if PY_VERSION_HEX >= 0x030C0000
    ,
    0  // tp_watched
#endif
#if PY_VERSION_HEX >= 0x030D0000
    ,
    0  // tp_versions_used
#endif
};

#if PY_VERSION_HEX >= 0x03080000 && PY_VERSION_HEX < 0x03090000
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__
#endif  // PY_VERSION_HEX

}  // namespace view::keys

////////////////////////////////////////////////////////////////////////////////
// dict::view::values::Type - sequence protocol
////////////////////////////////////////////////////////////////////////////////

namespace view::values {

PySequenceMethods as_sequence = {
    (lenfunc)length,  // lenfunc sq_length;
    0,                // binaryfunc sq_concat;
    0,                // ssizeargfunc sq_repeat;
    0,                // ssizeargfunc sq_item;
    0,                // void *was_sq_slice;
    0,                // ssizeobjargproc sq_ass_item;
    0,                // void *was_sq_ass_slice;
    0,                // objobjproc sq_contains;
    0,                // binaryfunc sq_inplace_concat;
    0                 // ssizeargfunc sq_inplace_repeat;
};

}  // namespace view::values

////////////////////////////////////////////////////////////////////////////////
// dict::view::values::Type - mapping protocol
////////////////////////////////////////////////////////////////////////////////

namespace view::values {

PyObject *subscript(dict::View *self, PyObject *item);

PyMappingMethods as_mapping = {
    (lenfunc)length,        // lenfunc mp_length;
    (binaryfunc)subscript,  // binaryfunc mp_subscript;
    0                       // objobjargproc mp_ass_subscript;
};

}  // namespace view::values

////////////////////////////////////////////////////////////////////////////////
// dict::view::values::Type - methods
////////////////////////////////////////////////////////////////////////////////

namespace view::values {

PyDoc_STRVAR(reversed_values_doc,
             "Return a reverse iterator over the dict values.");

PyObject *iterator(dict::View *self);
PyObject *reversed(dict::View *self, PyObject *);

#if __GNUC__ > 7 && !defined(__clang__)
// -Wcast-function-type was added in GCC 8.0, needs to be suppressed here,
// because we're casting functions with different number of parameters to
// PyCFunction
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif  // __GNUC__ > 7 && !defined(__clang__)

PyMethodDef methods[] = {
    {"__reversed__", (PyCFunction)reversed, METH_NOARGS, reversed_values_doc},
    {nullptr, nullptr, 0, nullptr},
};

#if __GNUC__ > 7 && !defined(__clang__)
#pragma GCC diagnostic pop
#endif  // __GNUC__ > 7 && !defined(__clang__)

}  // namespace view::values

////////////////////////////////////////////////////////////////////////////////
// dict::view::values::Type
////////////////////////////////////////////////////////////////////////////////

namespace view::values {

#if PY_VERSION_HEX >= 0x03080000 && PY_VERSION_HEX < 0x03090000
#ifdef __clang__
// The tp_print is marked as deprecated, which makes clang unhappy, 'cause it's
// initialized below. Skipping initialization also makes clang unhappy, so we're
// disabling the deprecated declarations warning.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif  // __clang__
#endif  // PY_VERSION_HEX

PyTypeObject Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)  // PyObject_VAR_HEAD
    "Dict_values",                          // const char *tp_name;

    /* For allocation */

    sizeof(dict::View),  // Py_ssize_t tp_basicsize;
    0,                   // Py_ssize_t tp_itemsize;

    /* Methods to implement standard operations */

    (destructor)dict::view::dealloc,  // destructor tp_dealloc;
    0,                                // printfunc tp_print;
    0,                                // getattrfunc tp_getattr;
    0,                                // setattrfunc tp_setattr;
    0,                                // cmpfunc tp_compare;
    dict::view::repr,                 // reprfunc tp_repr;

    /* Method suites for standard classes */

    0,                                 //  PyNumberMethods *tp_as_number;
    &dict::view::values::as_sequence,  //  PySequenceMethods *tp_as_sequence;
    &dict::view::values::as_mapping,   //  PyMappingMethods *tp_as_mapping;

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
    dict::gc_traverse,  // traverseproc tp_traverse;

    /* delete references to contained objects */
    dict::gc_clear,  // inquiry tp_clear;

    /* Assigned meaning in release 2.1 */
    /* rich comparisons */
    0,  // richcmpfunc tp_richcompare;

    /* weak reference enabler */
    0,  // long tp_weaklistoffset;

    /* Added in release 2.2 */
    /* Iterators */
    (getiterfunc)dict::view::values::iterator,  // getiterfunc tp_iter;
    0,                                          // iternextfunc tp_iternext;

    /* Attribute descriptor and subclassing stuff */
    dict::view::values::methods,  // struct PyMethodDef *tp_methods;
    0,                            // struct PyMemberDef *tp_members;
    dict::view::getset,           // struct PyGetSetDef *tp_getset;
    0,                            // struct _typeobject *tp_base;
    0,                            // PyObject *tp_dict;
    0,                            // descrgetfunc tp_descr_get;
    0,                            // descrsetfunc tp_descr_set;
    0,                            // long tp_dictoffset;
    0,                            // initproc tp_init;
    0,                            // allocfunc tp_alloc;
    0,                            // newfunc tp_new;
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
    0  // tp_vectorcall
#if PY_VERSION_HEX < 0x03090000
    ,
    0  // tp_print
#endif
#endif
#if PY_VERSION_HEX >= 0x030C0000
    ,
    0  // tp_watched
#endif
#if PY_VERSION_HEX >= 0x030D0000
    ,
    0  // tp_versions_used
#endif
};

#if PY_VERSION_HEX >= 0x03080000 && PY_VERSION_HEX < 0x03090000
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__
#endif  // PY_VERSION_HEX

}  // namespace view::values

////////////////////////////////////////////////////////////////////////////////
// dict::view::items::Type - sequence protocol
////////////////////////////////////////////////////////////////////////////////

namespace view::items {

int contains(dict::View *self, PyObject *tuple);

PySequenceMethods as_sequence = {
    (lenfunc)length,       // lenfunc sq_length;
    0,                     // binaryfunc sq_concat;
    0,                     // ssizeargfunc sq_repeat;
    0,                     // ssizeargfunc sq_item;
    0,                     // void *was_sq_slice;
    0,                     // ssizeobjargproc sq_ass_item;
    0,                     // void *was_sq_ass_slice;
    (objobjproc)contains,  // objobjproc sq_contains;
    0,                     // binaryfunc sq_inplace_concat;
    0                      // ssizeargfunc sq_inplace_repeat;
};

}  // namespace view::items

////////////////////////////////////////////////////////////////////////////////
// dict::view::items::Type - mapping protocol
////////////////////////////////////////////////////////////////////////////////

namespace view::items {

PyObject *subscript(dict::View *self, PyObject *item);

PyMappingMethods as_mapping = {
    (lenfunc)length,        // lenfunc mp_length;
    (binaryfunc)subscript,  // binaryfunc mp_subscript;
    0                       // objobjargproc mp_ass_subscript;
};

}  // namespace view::items

////////////////////////////////////////////////////////////////////////////////
// dict::view::items::Type - methods
////////////////////////////////////////////////////////////////////////////////

namespace view::items {

PyDoc_STRVAR(reversed_items_doc,
             "Return a reverse iterator over the dict items.");

PyObject *iterator(dict::View *self);
PyObject *reversed(dict::View *self, PyObject *);

#if __GNUC__ > 7 && !defined(__clang__)
// -Wcast-function-type was added in GCC 8.0, needs to be suppressed here,
// because we're casting functions with different number of parameters to
// PyCFunction
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif  // __GNUC__ > 7 && !defined(__clang__)

PyMethodDef methods[] = {
    {"isdisjoint", (PyCFunction)isdisjoint, METH_O, isdisjoint_doc},
    {"__reversed__", (PyCFunction)reversed, METH_NOARGS, reversed_items_doc},
    {nullptr, nullptr, 0, nullptr},
};

#if __GNUC__ > 7 && !defined(__clang__)
#pragma GCC diagnostic pop
#endif  // __GNUC__ > 7 && !defined(__clang__)

}  // namespace view::items

////////////////////////////////////////////////////////////////////////////////
// dict::view::items::Type
////////////////////////////////////////////////////////////////////////////////

namespace view::items {

#if PY_VERSION_HEX >= 0x03080000 && PY_VERSION_HEX < 0x03090000
#ifdef __clang__
// The tp_print is marked as deprecated, which makes clang unhappy, 'cause it's
// initialized below. Skipping initialization also makes clang unhappy, so we're
// disabling the deprecated declarations warning.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif  // __clang__
#endif  // PY_VERSION_HEX

PyTypeObject Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)  // PyObject_VAR_HEAD
    "Dict_items",                           // const char *tp_name;

    /* For allocation */

    sizeof(dict::View),  // Py_ssize_t tp_basicsize;
    0,                   // Py_ssize_t tp_itemsize;

    /* Methods to implement standard operations */

    (destructor)dict::view::dealloc,  // destructor tp_dealloc;
    0,                                // printfunc tp_print;
    0,                                // getattrfunc tp_getattr;
    0,                                // setattrfunc tp_setattr;
    0,                                // cmpfunc tp_compare;
    dict::view::repr,                 // reprfunc tp_repr;

    /* Method suites for standard classes */

    &dict::view::as_number,           //  PyNumberMethods *tp_as_number;
    &dict::view::items::as_sequence,  //  PySequenceMethods *tp_as_sequence;
    &dict::view::items::as_mapping,   //  PyMappingMethods *tp_as_mapping;

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
    dict::gc_traverse,  // traverseproc tp_traverse;

    /* delete references to contained objects */
    dict::gc_clear,  // inquiry tp_clear;

    /* Assigned meaning in release 2.1 */
    /* rich comparisons */
    (richcmpfunc)dict::view::richcompare,  // richcmpfunc tp_richcompare;

    /* weak reference enabler */
    0,  // long tp_weaklistoffset;

    /* Added in release 2.2 */
    /* Iterators */
    (getiterfunc)dict::view::items::iterator,  // getiterfunc tp_iter;
    0,                                         // iternextfunc tp_iternext;

    /* Attribute descriptor and subclassing stuff */
    dict::view::items::methods,  // struct PyMethodDef *tp_methods;
    0,                           // struct PyMemberDef *tp_members;
    dict::view::getset,          // struct PyGetSetDef *tp_getset;
    0,                           // struct _typeobject *tp_base;
    0,                           // PyObject *tp_dict;
    0,                           // descrgetfunc tp_descr_get;
    0,                           // descrsetfunc tp_descr_set;
    0,                           // long tp_dictoffset;
    0,                           // initproc tp_init;
    0,                           // allocfunc tp_alloc;
    0,                           // newfunc tp_new;
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
    0  // tp_vectorcall
#if PY_VERSION_HEX < 0x03090000
    ,
    0  // tp_print
#endif
#endif
#if PY_VERSION_HEX >= 0x030C0000
    ,
    0  // tp_watched
#endif
#if PY_VERSION_HEX >= 0x030D0000
    ,
    0  // tp_versions_used
#endif
};

#if PY_VERSION_HEX >= 0x03080000 && PY_VERSION_HEX < 0x03090000
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__
#endif  // PY_VERSION_HEX

}  // namespace view::items

////////////////////////////////////////////////////////////////////////////////
// helpers
////////////////////////////////////////////////////////////////////////////////

/**
 * Wraps the key into a tuple and sets it as an argument of the key error. This
 * protects from using a key which is already a tuple, as it would be unpacked
 * as the arguments of the exception.
 */
void set_key_error(PyObject *key) {
  const py::Release tuple{PyTuple_Pack(1, key)};

  if (tuple) {
    PyErr_SetObject(PyExc_KeyError, tuple.get());
  } else {
    std::string error;

    Python_context::pystring_to_string(key, &error, true);
    Python_context::set_python_error(PyExc_KeyError, error);
  }
}

/**
 * Sets Python RuntimeError with the message taken from the given exception.
 */
void set_exception(const std::exception &exc) {
  Python_context::set_python_error(PyExc_RuntimeError, exc.what());
}

/**
 * Converts the given value to dictionary, expects a Python dictionary.
 */
Dictionary_t to_dict(PyObject *value) {
  Dictionary_t ret;

  if (dict_check(value)) {
    ret = *((dict::Object *)value)->dict;
  } else {
    ret = py::convert(value).as_map();
  }

  return ret;
}

/**
 * Merges key-value pairs from other into self, overwriting existing keys.
 * Expects a dictionary or a mapping.
 */
bool merge(dict::Object *self, PyObject *other) {
  if (PyDict_Check(other)) {
    const auto dict = to_dict(other);

    for (const auto &item : *dict) {
      self->dict->get()->set(item.first, item.second);
    }
  } else {
    const py::Release keys{PyMapping_Keys(other)};

    if (!keys) {
      return false;
    }

    const py::Release iter{PyObject_GetIter(keys.get())};

    if (!iter) {
      return false;
    }

    Python_context *context = nullptr;

    while (const py::Release key{PyIter_Next(iter.get())}) {
      const py::Release value{PyObject_GetItem(other, key.get())};

      if (!value) {
        return false;
      }

      py::set_item(*self->dict, key.get(), value.get(), &context);
    }
  }

  return true;
}

/**
 * Merges key-value pairs from other into self, overwriting existing keys.
 * Expects an iterable producing iterable objects of length 2.
 */
bool merge_iterable(dict::Object *self, PyObject *other) {
  const py::Release iter{PyObject_GetIter(other)};

  if (!iter) {
    return false;
  }

  int index = 0;
  Python_context *context = nullptr;

  while (const py::Release item{PyIter_Next(iter.get())}) {
    const py::Release sequence{PySequence_Fast(item.get(), "")};

    if (!sequence) {
      if (PyErr_ExceptionMatches(PyExc_TypeError)) {
        PyErr_Format(PyExc_TypeError,
                     "cannot convert dictionary update sequence element #%zd "
                     "to a sequence",
                     index);
      }

      break;
    }

    const auto length = PySequence_Fast_GET_SIZE(sequence.get());

    if (2 != length) {
      PyErr_Format(PyExc_ValueError,
                   "dictionary update sequence element #%zd has length %zd; 2 "
                   "is required",
                   index, length);
      break;
    }

    // borrowed references
    const auto key = PySequence_Fast_GET_ITEM(sequence.get(), 0);
    const auto value = PySequence_Fast_GET_ITEM(sequence.get(), 1);

    py::set_item(*self->dict, key, value, &context);

    ++index;
  }

  return !PyErr_Occurred();
}

/**
 * Updates self with key-value pairs from other, overwriting existing keys.
 * Expects a dictionary, a mapping or an iterable producing iterable objects of
 * length 2.
 */
bool update(dict::Object *self, PyObject *other) {
  if (PyDict_Check(other)) {
    return merge(self, other);
  }

  if (PyObject_HasAttrString(other, "keys")) {
    return merge(self, other);
  }

  return merge_iterable(self, other);
}

/**
 * Updates self with an optional object held in args, overwriting existing keys.
 * If keyword arguments are specified, the dictionary is then updated with those
 * key/value pairs.
 *
 * The optional object must be a dictionary, a mapping or an iterable producing
 * iterable objects of length 2.
 */
bool update(dict::Object *self, PyObject *args, PyObject *kwds,
            const char *method) {
  PyObject *arg = nullptr;

  if (!PyArg_UnpackTuple(args, method, 0, 1, &arg)) {
    return false;
  }

  if (arg && !update(self, arg)) {
    return false;
  }

  if (kwds) {
    if (PyArg_ValidateKeywordArguments(kwds)) {
      return merge(self, kwds);
    } else {
      return false;
    }
  }

  return true;
}

/**
 * Converts a dictionary view to a set. Returns a new reference.
 */
PyObject *as_set(dict::View *self) {
  return PySet_New(reinterpret_cast<PyObject *>(self));
}

/**
 * Converts self to a set and calls the specified method using the other as an
 * argument. Returns a new reference holding self as a set.
 */
PyObject *call_set_method(dict::View *self, PyObject *other,
                          const char *method_name) {
  py::Release result{as_set(self)};

  if (!result) {
    return nullptr;
  }

  py::Release method{PyString_FromString(method_name)};
  py::Release unused{
      PyObject_CallMethodObjArgs(result.get(), method.get(), other, nullptr)};

  if (!unused) {
    return nullptr;
  }

  return result.release();
}

////////////////////////////////////////////////////////////////////////////////
// dict::Type - number protocol
////////////////////////////////////////////////////////////////////////////////

/**
 * Create a new dictionary with the merged keys and values of d and other, which
 * must both be dictionaries. The values of other take priority when d and other
 * share keys.
 */
PyObject *nb_or(PyObject *self, PyObject *other) {
  if (!PyDict_Check(self) || !PyDict_Check(other)) {
    Py_RETURN_NOTIMPLEMENTED;
  }

  PyObject *result = nullptr;

  try {
    py::Release dict =
        dict_check(self)
            ? py::Release{copy(reinterpret_cast<dict::Object *>(self))}
            : wrap(to_dict(self));

    if (update(reinterpret_cast<dict::Object *>(dict.get()), other)) {
      result = dict.release();
    }
  } catch (const std::exception &exc) {
    set_exception(exc);
  }

  return result;
}

/**
 * Update the dictionary d with keys and values from other, which may be either
 * a mapping or an iterable of key/value pairs. The values of other take
 * priority when d and other share keys.
 */
PyObject *inplace_or(PyObject *self, PyObject *other) {
  assert(dict_check(self));

  try {
    if (update(reinterpret_cast<dict::Object *>(self), other)) {
      Py_INCREF(self);
      return self;
    }
  } catch (const std::exception &exc) {
    set_exception(exc);
  }

  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// dict::Type - sequence protocol
////////////////////////////////////////////////////////////////////////////////

/**
 * Determine if o contains value. If an item in o is equal to value, return 1,
 * otherwise return 0. On error, return -1. This is equivalent to the Python
 * expression value in o.
 */
int contains(dict::Object *self, PyObject *key) {
  if (!key) {
    return -1;
  }

  std::string key_to_find;
  bool found = false;

  if (Python_context::pystring_to_string(key, &key_to_find)) {
    found = self->dict->get()->has_key(key_to_find);
  }

  return found;
}

////////////////////////////////////////////////////////////////////////////////
// dict::Type - mapping protocol
////////////////////////////////////////////////////////////////////////////////

/**
 * Returns the number of objects in sequence o on success, and -1 on failure.
 * This is equivalent to the Python expression len(o).
 */
Py_ssize_t length(dict::Object *self) { return self->dict->get()->size(); }

/**
 * Return element of o corresponding to the object key or NULL on failure. This
 * is the equivalent of the Python expression o[key].
 */
PyObject *subscript(dict::Object *self, PyObject *key) {
  std::string key_to_find;

  if (Python_context::pystring_to_string(key, &key_to_find)) {
    try {
      const auto result = self->dict->get()->find(key_to_find);

      if (result != self->dict->get()->end()) {
        return py::convert(result->second).release();
      }
    } catch (const std::exception &exc) {
      set_exception(exc);
    }
  }

  if (!PyErr_Occurred()) {
    // TODO(pawel): Python dict allows for subclasses to implement __missing__
    // method, which handles missing keys (either returns some value or raises
    // a specific exception). Should we support this?

    // if there was no other exception, it means that the key was not found
    set_key_error(key);
  }

  return nullptr;
}

/**
 * Map the object key to the value v. Raise an exception and return -1 on
 * failure; return 0 on success. This is the equivalent of the Python statement
 * o[key] = v.
 *
 * v can also be set to NULL to delete an item.
 */
int assign_subscript(dict::Object *self, PyObject *key, PyObject *value) {
  std::string key_to_find;

  if (Python_context::pystring_to_string(key, &key_to_find)) {
    if (value) {
      // set the value
      try {
        self->dict->get()->set(key_to_find, py::convert(value));
      } catch (const std::exception &exc) {
        set_exception(exc);
      }
    } else {
      // delete the value
      if (!self->dict->get()->erase(key_to_find)) {
        set_key_error(key);
      }
    }
  } else {
    if (value) {
      Python_context::set_python_error(PyExc_KeyError,
                                       "shell.Dict key must be a string");
    } else {
      // this is a delete operation, simply report a missing key
      set_key_error(key);
    }
  }

  return PyErr_Occurred() ? -1 : 0;
}

////////////////////////////////////////////////////////////////////////////////
// dict::Type - methods
////////////////////////////////////////////////////////////////////////////////

PyObject *dir(dict::Object *self) {
  py::Release class_methods{
      PyObject_Dir(reinterpret_cast<PyObject *>(&dict::Type))};
  const auto methods_size = PyObject_Size(class_methods.get());

  py::Release keys{dict::keys(self)};
  const auto keys_size = dict::length(self);

  const auto all_members = PyList_New(methods_size + keys_size);

  PyList_SetSlice(all_members, 0, methods_size, class_methods.get());
  PyList_SetSlice(all_members, methods_size, methods_size + keys_size,
                  keys.get());

  return all_members;
}

/**
 * Return a reverse iterator over the keys of the dictionary.
 */
PyObject *reversed(dict::Object *self, PyObject *) {
  return dict::iter::create<Map_reverse_iterator>(self,
                                                  &dict::iter::keys::rev::Type);
}

PyObject *size_of(dict::Object *self) {
  return PyLong_FromSsize_t(_PyObject_SIZE(Py_TYPE(self)));
}

/**
 * Remove all items from the dictionary.
 */
PyObject *clear(dict::Object *self) {
  self->dict->get()->clear();
  self->base.ma_used = 0;
  Py_RETURN_NONE;
}

/**
 * Return a shallow copy of the dictionary.
 */
PyObject *copy(dict::Object *self) {
  const auto copy =
      std::make_shared<shcore::Value::Map_type>(*self->dict->get());
  return wrap(copy).release();
}

/**
 * Create a new dictionary with keys from iterable and values set to value.
 */
PyObject *from_keys(PyTypeObject *type, PyObject *args) {
  PyObject *iterable = nullptr;
  PyObject *value = Py_None;

  if (!PyArg_UnpackTuple(args, "fromkeys", 1, 2, &iterable, &value)) {
    return nullptr;
  }

  py::Release dict{PyObject_CallObject((PyObject *)type, nullptr)};

  if (!dict) {
    return nullptr;
  }

  const py::Release iter{PyObject_GetIter(iterable)};

  if (!iter) {
    return nullptr;
  }

  const bool is_shell_dict = dict_check(dict.get());
  const bool is_dict = PyDict_CheckExact(dict.get());
  Python_context *context = nullptr;

  while (const py::Release key{PyIter_Next(iter.get())}) {
    int result = 0;

    if (is_shell_dict) {
      try {
        py::set_item(*reinterpret_cast<dict::Object *>(dict.get())->dict,
                     key.get(), value, &context);
      } catch (const std::exception &exc) {
        set_exception(exc);
        result = -1;
      }
    } else if (is_dict) {
      result = PyDict_SetItem(dict.get(), key.get(), value);
    } else {
      result = PyObject_SetItem(dict.get(), key.get(), value);
    }

    if (result < 0) {
      return nullptr;
    }
  }

  if (is_shell_dict) {
    const auto shdict = reinterpret_cast<dict::Object *>(dict.get());
    shdict->base.ma_used = length(shdict);
  }

  return dict.release();
}

/**
 * Return the value for key if key is in the dictionary, else default. If
 * default is not given, it defaults to None, so that this method never raises
 * a KeyError.
 */
PyObject *get(dict::Object *self, PyObject *args) {
  PyObject *key = nullptr;
  PyObject *default_value = Py_None;

  if (!PyArg_UnpackTuple(args, "get", 1, 2, &key, &default_value)) {
    return nullptr;
  }

  std::string key_to_find;

  if (Python_context::pystring_to_string(key, &key_to_find)) {
    try {
      const auto result = self->dict->get()->find(key_to_find);

      if (result != self->dict->get()->end()) {
        return py::convert(result->second).release();
      }
    } catch (const std::exception &exc) {
      set_exception(exc);
      return nullptr;
    }
  }

  Py_INCREF(default_value);
  return default_value;
}

/**
 * Return True if d has a key key, else False.
 */
PyObject *has_key(dict::Object *self, PyObject *key) {
  const auto result = contains(self, key);

  if (result < 0) {
    return nullptr;
  } else if (result > 0) {
    Py_RETURN_TRUE;
  } else {
    Py_RETURN_FALSE;
  }
}

/**
 * Return a new view of the dictionary’s items ((key, value) pairs).
 */
PyObject *items(dict::Object *self) {
  return dict::view::create(self, &dict::view::items::Type);
}

/**
 * Return a new view of the dictionary’s keys.
 */
PyObject *keys(dict::Object *self) {
  return dict::view::create(self, &dict::view::keys::Type);
}

/**
 * If key is in the dictionary, remove it and return its value, else return
 * default. If default is not given and key is not in the dictionary, a KeyError
 * is raised.
 */
PyObject *pop(dict::Object *self, PyObject *args) {
  PyObject *key = nullptr;
  PyObject *default_value = nullptr;

  if (!PyArg_UnpackTuple(args, "pop", 1, 2, &key, &default_value)) {
    return nullptr;
  }

  std::string key_to_find;

  if (Python_context::pystring_to_string(key, &key_to_find)) {
    try {
      const auto result = self->dict->get()->find(key_to_find);

      if (result != self->dict->get()->end()) {
        auto ret = py::convert(result->second);

        self->dict->get()->erase(result);
        --self->base.ma_used;

        return ret.release();
      }
    } catch (const std::exception &exc) {
      set_exception(exc);
      return nullptr;
    }
  }

  if (default_value) {
    Py_INCREF(default_value);
    return default_value;
  } else {
    set_key_error(key);
    return nullptr;
  }
}

/**
 * Remove and return a (key, value) pair from the dictionary. If the dictionary
 * is empty, calling popitem() raises a KeyError.
 */
PyObject *pop_item(dict::Object *self) {
  if (self->dict->get()->empty()) {
    Python_context::set_python_error(PyExc_KeyError,
                                     "popitem(): dictionary is empty");
    return nullptr;
  }

  py::Release result{PyTuple_New(2)};
  const auto item = std::prev(self->dict->get()->end());

  try {
    PyTuple_SET_ITEM(result.get(), 0, PyString_FromString(item->first.c_str()));
    PyTuple_SET_ITEM(result.get(), 1, py::convert(item->second).release());
  } catch (const std::exception &exc) {
    set_exception(exc);
    return nullptr;
  }

  self->dict->get()->erase(item);
  --self->base.ma_used;

  return result.release();
}

/**
 * If key is in the dictionary, return its value. If not, insert key with a
 * value of default and return default. default defaults to None.
 */
PyObject *set_default(dict::Object *self, PyObject *args) {
  PyObject *key = nullptr;
  PyObject *default_value = Py_None;

  if (!PyArg_UnpackTuple(args, "setdefault", 1, 2, &key, &default_value)) {
    return nullptr;
  }

  std::string key_to_find;

  if (Python_context::pystring_to_string(key, &key_to_find)) {
    try {
      const auto result = self->dict->get()->find(key_to_find);

      if (result != self->dict->get()->end()) {
        return py::convert(result->second).release();
      } else {
        self->dict->get()->set(key_to_find, py::convert(default_value));
        ++self->base.ma_used;

        Py_INCREF(default_value);
        return default_value;
      }
    } catch (const std::exception &exc) {
      set_exception(exc);
    }
  } else {
    Python_context::set_python_error(PyExc_KeyError,
                                     "shell.Dict key must be a string");
  }

  return nullptr;
}

/**
 * Update the dictionary with the key/value pairs from other, overwriting
 * existing keys. Return None.
 *
 * update() accepts either another dictionary object or an iterable of key/value
 * pairs (as tuples or other iterables of length two). If keyword arguments are
 * specified, the dictionary is then updated with those key/value pairs:
 * d.update(red=1, blue=2).
 */
PyObject *update(dict::Object *self, PyObject *args, PyObject *kwds) {
  try {
    if (update(self, args, kwds, "update")) {
      self->base.ma_used = length(self);
      Py_RETURN_NONE;
    }
  } catch (const std::exception &exc) {
    set_exception(exc);
  }

  return nullptr;
}

/**
 * Return a new view of the dictionary’s values.
 */
PyObject *values(dict::Object *self) {
  return dict::view::create(self, &dict::view::values::Type);
}

////////////////////////////////////////////////////////////////////////////////
// dict::Type
////////////////////////////////////////////////////////////////////////////////

void dealloc(dict::Object *self) {
  delete self->dict;

  Py_TYPE(self)->tp_free(self);
}

PyObject *repr(dict::Object *self) {
  return PyString_FromString(Value(*self->dict).repr().c_str());
}

PyObject *str(dict::Object *self) {
  return PyString_FromString(Value(*self->dict).descr().c_str());
}

PyObject *get_attribute(dict::Object *self, PyObject *name) {
  if (auto object =
          PyObject_GenericGetAttr(reinterpret_cast<PyObject *>(self), name)) {
    return object;
  }

  // PyObject_GenericGetAttr() has failed and set an exception, if we find the
  // key we need to clear the exception

  std::string key_to_find;

  if (Python_context::pystring_to_string(name, &key_to_find)) {
    try {
      const auto result = self->dict->get()->find(key_to_find);

      if (result != self->dict->get()->end()) {
        PyErr_Clear();
        return py::convert(result->second).release();
      }
    } catch (const std::exception &exc) {
      set_exception(exc);
    }
  }

  return nullptr;
}

int gc_traverse(PyObject *, visitproc, void *) {
  // used to override traverse function from Python's list
  // empty, as we don't hold any Python objects
  return 0;
}

int gc_clear(PyObject *) {
  // used to override clear function from Python's list
  // empty, as we don't hold any Python objects
  return 0;
}

/**
 * Dictionaries compare equal if and only if they have the same (key, value)
 * pairs (regardless of ordering). Order comparisons (‘<’, ‘<=’, ‘>=’, ‘>’)
 * raise TypeError.
 */
PyObject *richcompare(dict::Object *self, PyObject *right, int op) {
  if (!PyDict_Check(right) || !(Py_EQ == op || Py_NE == op)) {
    Py_RETURN_NOTIMPLEMENTED;
  }

  const auto r = to_dict(right);
  const auto sr = r->size();
  const auto l = self->dict->get();
  const auto sl = l->size();

  const auto result = [op](bool b) {
    auto res = b == (Py_EQ == op) ? Py_True : Py_False;
    Py_INCREF(res);
    return res;
  };

  if (sr != sl) {
    // lengths differ, dictionaries differ
    return result(false);
  } else {
    static_assert(
        std::is_same_v<std::map<std::string, Value>,
                       std::remove_pointer_t<decltype(l)>::container_type>,
        "This algorithm assumes that items in the map are ordered");

    for (auto li = l->begin(), ri = r->begin(); li != l->end(); ++li, ++ri) {
      if (li->first != ri->first || li->second != ri->second) {
        return result(false);
      }
    }
  }

  return result(true);
}

PyObject *iterator(dict::Object *self) {
  return dict::iter::create<Map_iterator>(self, &dict::iter::keys::Type);
}

int init(dict::Object *self, PyObject *args, PyObject *kwds) {
  {
    // we don't want the underlying dictionary to store anything, so we're
    // using arguments to initialize an empty dictionary
    py::Release dict_args{PyTuple_New(0)};

    if (PyDict_Type.tp_init((PyObject *)self, dict_args.get(), nullptr) < 0) {
      return -1;
    }
  }

  try {
    if (update(self, args, kwds, "Dict")) {
      self->base.ma_used = length(self);
      return 0;
    }
  } catch (const std::exception &exc) {
    set_exception(exc);
  }

  return -1;
}

PyObject *create(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  auto self = (dict::Object *)PyType_GenericNew(type, args, kwds);

  assert(!self->dict);
  self->dict = new Dictionary_t(make_dict());

  return reinterpret_cast<PyObject *>(self);
}

////////////////////////////////////////////////////////////////////////////////
// dict::Iterator - common helper functions
////////////////////////////////////////////////////////////////////////////////

namespace iter {

Map_iterator begin(dict::Iterator *self) {
  return self->o.get<dict::Object *>()->dict->get()->begin();
}

Map_reverse_iterator begin(dict::Reverse_iterator *self) {
  return self->o.get<dict::Object *>()->dict->get()->rbegin();
}

Map_iterator end(dict::Iterator *self) {
  return self->o.get<dict::Object *>()->dict->get()->end();
}

Map_reverse_iterator end(dict::Reverse_iterator *self) {
  return self->o.get<dict::Object *>()->dict->get()->rend();
}

template <typename It>
PyObject *create(dict::Object *dict, PyTypeObject *type) {
  auto it = PyObject_New(dict::Iterator_base<It>, type);

  // placement new into the memory allocated for the iterator, assignment is not
  // safe, as memory was allocated by malloc()
  new (&it->o) py::Store{reinterpret_cast<PyObject *>(dict)};
  new (&it->next) It{begin(it)};

  it->initial_size = dict->dict->get()->size();

  return reinterpret_cast<PyObject *>(it);
}

template <typename It>
void dealloc(dict::Iterator_base<It> *self) {
  self->next.~It();
  self->o.~Store();
  PyObject_Del(self);
}

template <typename It>
PyObject *next(dict::Iterator_base<It> *self, convert_t convert) {
  if (!self->o) {
    return nullptr;
  }

  const auto &dict = self->o.template get<dict::Object *>()->dict->get();

  if (!dict) {
    return nullptr;
  }

  if (dict->size() != self->initial_size) {
    Python_context::set_python_error(
        PyExc_RuntimeError, "dictionary changed size during iteration");
    // invalid size makes sure that this state is remembered
    self->initial_size = static_cast<size_t>(-1);
    return nullptr;
  }

  if (self->next == end(self)) {
    // we've reached the end, release the reference to the dictionary
    self->o.reset();
  } else {
    try {
      const auto key = convert(*self->next);
      ++self->next;
      return key;
    } catch (const std::exception &exc) {
      set_exception(exc);
    }
  }

  return nullptr;
}

}  // namespace iter

////////////////////////////////////////////////////////////////////////////////
// dict::Iterator - common type methods
////////////////////////////////////////////////////////////////////////////////

namespace iter {

template <typename It>
PyObject *length(dict::Iterator_base<It> *self) {
  Py_ssize_t length = 0;

  if (self->o) {
    length = std::distance(self->next, end(self));
    assert(length >= 0);
  }

  return PyLong_FromSsize_t(length);
}

template <typename It>
PyObject *reduce(dict::Iterator_base<It> *self) {
  // copy the state, this increases the reference count; when iterator reaches
  // the end, it will decrease it and release the ownership of the dictionary;
  // if there's an error during iteration, iterator will not release the
  // ownership and destructor of `copy` will do that
  auto copy = *self;

  // copy the values returned by the iterator to a Python list
  auto list = PySequence_List(reinterpret_cast<PyObject *>(&copy));

  if (!list) {
    return nullptr;
  }

  return Py_BuildValue("N(N)", py::get_builtin("iter").release(), list);
}

}  // namespace iter

////////////////////////////////////////////////////////////////////////////////
// dict::iter::keys::Type
////////////////////////////////////////////////////////////////////////////////

namespace iter::keys {

PyObject *convert(const Map_value_type &pair) {
  return PyString_FromString(pair.first.c_str());
}

template <typename It>
PyObject *next(dict::Iterator_base<It> *self) {
  return dict::iter::next(self, dict::iter::keys::convert);
}

}  // namespace iter::keys

////////////////////////////////////////////////////////////////////////////////
// dict::iter::values::Type
////////////////////////////////////////////////////////////////////////////////

namespace iter::values {

PyObject *convert(const Map_value_type &pair) {
  return py::convert(pair.second).release();
}

template <typename It>
PyObject *next(dict::Iterator_base<It> *self) {
  return dict::iter::next(self, dict::iter::values::convert);
}

}  // namespace iter::values

////////////////////////////////////////////////////////////////////////////////
// dict::iter::items::Type
////////////////////////////////////////////////////////////////////////////////

namespace iter::items {

PyObject *convert(const Map_value_type &pair) {
  py::Release result{PyTuple_New(2)};

  if (!result) {
    return nullptr;
  }

  PyTuple_SET_ITEM(result.get(), 0, dict::iter::keys::convert(pair));
  PyTuple_SET_ITEM(result.get(), 1, dict::iter::values::convert(pair));

  return result.release();
}

template <typename It>
PyObject *next(dict::Iterator_base<It> *self) {
  return dict::iter::next(self, dict::iter::items::convert);
}

}  // namespace iter::items

////////////////////////////////////////////////////////////////////////////////
// dict::View - common functions
////////////////////////////////////////////////////////////////////////////////

namespace view {

PyObject *create(dict::Object *dict, PyTypeObject *type) {
  auto view = (dict::View *)PyType_GenericAlloc(type, 0);

  // placement new into the memory allocated for the view, assignment is not
  // safe, as memory was allocated by malloc()
  new (&view->o) py::Store{reinterpret_cast<PyObject *>(dict)};

  return reinterpret_cast<PyObject *>(view);
}

void dealloc(dict::View *self) {
  self->o.~Store();
  PyObject_Del(self);
}

PyObject *subtract(dict::View *self, PyObject *other) {
  return call_set_method(self, other, "difference_update");
}

PyObject *nb_and(dict::View *self, PyObject *other) {
  return call_set_method(self, other, "intersection_update");
}

PyObject *nb_or(dict::View *self, PyObject *other) {
  return call_set_method(self, other, "update");
}

PyObject *nb_xor(dict::View *self, PyObject *other) {
  return call_set_method(self, other, "symmetric_difference_update");
}

Py_ssize_t length(dict::View *self) {
  Py_ssize_t length = 0;

  if (self->o) {
    length = dict::length(self->o.get<dict::Object *>());
  }

  return length;
}

PyObject *subscript(dict::View *self, PyObject *item,
                    dict::iter::convert_t convert) {
  if (!self->o) {
    return nullptr;
  }

  // emulate a list - backward compatibility
  if (PyIndex_Check(item)) {
    Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);

    if (-1 == i && PyErr_Occurred()) {
      return nullptr;
    }

    const auto length = dict::length(self->o.get<dict::Object *>());

    if (i < 0) {
      i += length;
    }

    if (i < 0 || i >= length) {
      Python_context::set_python_error(PyExc_IndexError,
                                       "list index out of range");
      return nullptr;
    }

    auto it = self->o.get<dict::Object *>()->dict->get()->begin();
    std::advance(it, i);

    return convert(*(it));
  } else {
    Python_context::set_python_error(
        PyExc_TypeError,
        str_format("list indices must be integers or slices, not %.200s",
                   item->ob_type->tp_name));

    return nullptr;
  }
}

}  // namespace view

////////////////////////////////////////////////////////////////////////////////
// dict::view - common getters/setters
////////////////////////////////////////////////////////////////////////////////

namespace view {

PyObject *mapping(dict::View *self, void *) {
  if (!self->o) {
    return nullptr;
  }

  return PyDictProxy_New(self->o.get());
}

}  // namespace view

////////////////////////////////////////////////////////////////////////////////
// dict::view - common type methods
////////////////////////////////////////////////////////////////////////////////

namespace view {

PyObject *richcompare(dict::View *self, PyObject *other, int op) {
  if (!PyAnySet_Check(other) && !PyDictViewSet_Check(other)) {
    Py_RETURN_NOTIMPLEMENTED;
  }

  py::Release left{as_set(self)};

  if (!left) {
    return nullptr;
  }

  py::Release right{PySet_New(other)};

  if (!right) {
    return nullptr;
  }

  return PyObject_RichCompare(left.get(), right.get(), op);
}

PyObject *repr(PyObject *self) {
  auto rc = Py_ReprEnter(self);

  if (rc != 0) {
    return rc > 0 ? PyUnicode_FromString("...") : nullptr;
  }

  shcore::on_leave_scope repr_leave{[self]() { Py_ReprLeave(self); }};

  py::Release sequence{PySequence_List(self)};

  if (!sequence) {
    return nullptr;
  }

  return PyUnicode_FromFormat("%s(%R)", Py_TYPE(self)->tp_name, sequence.get());
}

PyObject *isdisjoint(dict::View *self, PyObject *other) {
  py::Release set{as_set(self)};

  if (!set) {
    return nullptr;
  }

  py::Release method{PyString_FromString("isdisjoint")};

  return PyObject_CallMethodObjArgs(set.get(), method.get(), other, nullptr);
}

}  // namespace view

////////////////////////////////////////////////////////////////////////////////
// dict::view::keys::Type - sequence protocol
////////////////////////////////////////////////////////////////////////////////

namespace view::keys {

int contains(dict::View *self, PyObject *key) {
  if (!self->o) {
    return 0;
  }

  return dict::contains(self->o.get<dict::Object *>(), key);
}

}  // namespace view::keys

////////////////////////////////////////////////////////////////////////////////
// dict::view::keys::Type - mapping protocol
////////////////////////////////////////////////////////////////////////////////

namespace view::keys {

PyObject *subscript(dict::View *self, PyObject *item) {
  return dict::view::subscript(self, item, dict::iter::keys::convert);
}

}  // namespace view::keys

////////////////////////////////////////////////////////////////////////////////
// dict::view::keys::Type - methods
////////////////////////////////////////////////////////////////////////////////

namespace view::keys {

PyObject *iterator(dict::View *self) {
  if (!self->o) {
    Py_RETURN_NONE;
  }

  return dict::iter::create<Map_iterator>(self->o.get<dict::Object *>(),
                                          &dict::iter::keys::Type);
}

PyObject *reversed(dict::View *self, PyObject *) {
  if (!self->o) {
    Py_RETURN_NONE;
  }

  return dict::iter::create<Map_reverse_iterator>(self->o.get<dict::Object *>(),
                                                  &dict::iter::keys::rev::Type);
}

}  // namespace view::keys

////////////////////////////////////////////////////////////////////////////////
// dict::view::values::Type - mapping protocol
////////////////////////////////////////////////////////////////////////////////

namespace view::values {

PyObject *subscript(dict::View *self, PyObject *item) {
  return dict::view::subscript(self, item, dict::iter::values::convert);
}

}  // namespace view::values

////////////////////////////////////////////////////////////////////////////////
// dict::view::values::Type - methods
////////////////////////////////////////////////////////////////////////////////

namespace view::values {

PyObject *iterator(dict::View *self) {
  if (!self->o) {
    Py_RETURN_NONE;
  }

  return dict::iter::create<Map_iterator>(self->o.get<dict::Object *>(),
                                          &dict::iter::values::Type);
}

PyObject *reversed(dict::View *self, PyObject *) {
  if (!self->o) {
    Py_RETURN_NONE;
  }

  return dict::iter::create<Map_reverse_iterator>(
      self->o.get<dict::Object *>(), &dict::iter::values::rev::Type);
}

}  // namespace view::values

////////////////////////////////////////////////////////////////////////////////
// dict::view::items::Type - sequence protocol
////////////////////////////////////////////////////////////////////////////////

namespace view::items {

int contains(dict::View *self, PyObject *tuple) {
  if (!self->o || !PyTuple_Check(tuple) || 2 != PyTuple_GET_SIZE(tuple)) {
    return 0;
  }

  const auto key = PyTuple_GET_ITEM(tuple, 0);
  std::string key_to_find;

  if (Python_context::pystring_to_string(key, &key_to_find)) {
    try {
      const auto result =
          self->o.get<dict::Object *>()->dict->get()->find(key_to_find);

      if (result != self->o.get<dict::Object *>()->dict->get()->end()) {
        return py::convert(PyTuple_GET_ITEM(tuple, 1)) == result->second;
      }
    } catch (const std::exception &exc) {
      set_exception(exc);
      return -1;
    }
  }

  return 0;
}

}  // namespace view::items

////////////////////////////////////////////////////////////////////////////////
// dict::view::items::Type - mapping protocol
////////////////////////////////////////////////////////////////////////////////

namespace view::items {

PyObject *subscript(dict::View *self, PyObject *item) {
  return dict::view::subscript(self, item, dict::iter::items::convert);
}

}  // namespace view::items

////////////////////////////////////////////////////////////////////////////////
// dict::view::items::Type - methods
////////////////////////////////////////////////////////////////////////////////

namespace view::items {

PyObject *iterator(dict::View *self) {
  if (!self->o) {
    Py_RETURN_NONE;
  }

  return dict::iter::create<Map_iterator>(self->o.get<dict::Object *>(),
                                          &dict::iter::items::Type);
}

PyObject *reversed(dict::View *self, PyObject *) {
  if (!self->o) {
    Py_RETURN_NONE;
  }

  return dict::iter::create<Map_reverse_iterator>(
      self->o.get<dict::Object *>(), &dict::iter::items::rev::Type);
}

}  // namespace view::items

}  // namespace dict

}  // namespace

void Python_context::init_shell_dict_type() {
  // this has to be done at runtime
  dict::Type.tp_base = &PyDict_Type;
  dict::view::keys::Type.tp_base = &PyDictKeys_Type;
  dict::view::values::Type.tp_base = &PyDictValues_Type;
  dict::view::items::Type.tp_base = &PyDictItems_Type;

  if (PyType_Ready(&dict::Type) < 0) {
    throw std::runtime_error("Could not initialize Shcore Dict type in Python");
  }

  if (PyType_Ready(&dict::view::keys::Type) < 0) {
    throw std::runtime_error(
        "Could not initialize Shcore Dict keys type in Python");
  }

  if (PyType_Ready(&dict::view::values::Type) < 0) {
    throw std::runtime_error(
        "Could not initialize Shcore Dict values type in Python");
  }

  if (PyType_Ready(&dict::view::items::Type) < 0) {
    throw std::runtime_error(
        "Could not initialize Shcore Dict items type in Python");
  }

  Py_INCREF(&dict::Type);

  auto module = get_shell_python_support_module();

  PyModule_AddObject(module.get(), "Dict",
                     reinterpret_cast<PyObject *>(&dict::Type));

  _shell_dict_class =
      py::Store{PyDict_GetItemString(PyModule_GetDict(module.get()), "Dict")};
}

bool dict_check(PyObject *value) { return Py_TYPE(value) == &dict::Type; }

py::Release wrap(const Dictionary_t &map) {
  auto wrapper = (dict::Object *)PyType_GenericAlloc(&dict::Type, 0);

  assert(!wrapper->dict);
  wrapper->dict = new Dictionary_t(map);
  wrapper->base.ma_used = length(wrapper);

  return py::Release{reinterpret_cast<PyObject *>(wrapper)};
}

bool unwrap(PyObject *value, Dictionary_t *ret_object) {
  if (dict_check(value)) {
    const auto dict = ((dict::Object *)value)->dict;

    assert(dict);

    *ret_object = *dict;

    return true;
  }

  return false;
}

}  // namespace shcore
