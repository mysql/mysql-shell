/*
 * Copyright (c) 2015, 2020, Oracle and/or its affiliates.
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

#ifndef _PYTHON_UTILS_H_
#define _PYTHON_UTILS_H_

// Include and avoid warnings from v8
#if defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#if defined(__clang__) && \
    ((__clang_major__ == 3 && __clang_minor__ >= 8) || __clang_major__ > 3)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#endif

#include <Python.h>

#if defined(__clang__) && \
    ((__clang_major__ == 3 && __clang_minor__ >= 8) || __clang_major__ > 3)
#pragma clang diagnostic pop
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#define PyString_FromString PyUnicode_FromString
#define PyInt_Check PyLong_Check
#define PyInt_FromLong PyLong_FromLong

// Must be placed when Python code will be called
struct WillEnterPython {
  PyGILState_STATE state;
  bool locked;

  WillEnterPython() : state(PyGILState_Ensure()), locked(true) {}

  ~WillEnterPython() {
    if (locked) PyGILState_Release(state);
  }

  void release() {
    if (locked) PyGILState_Release(state);
    locked = false;
  }
};

// Must be placed when non-python code will be called from a Python
// handler/callback
struct WillLeavePython {
  PyThreadState *save;

  WillLeavePython() : save(PyEval_SaveThread()) {}

  ~WillLeavePython() { PyEval_RestoreThread(save); }
};

#endif
