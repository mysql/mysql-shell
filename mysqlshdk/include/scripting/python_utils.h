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

#ifndef _PYTHON_UTILS_H_
#define _PYTHON_UTILS_H_

#include <utility>

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

  ~WillEnterPython() noexcept {
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

  ~WillLeavePython() noexcept { PyEval_RestoreThread(save); }
};

namespace shcore::py {
/**
 * Used to store borrowed references, references passed from Python (i.e. as
 * arguments), or new references if such reference is returned to Python.
 *
 * Increases the reference count when acquiring the Python object, decreases it
 * when releasing the object.
 */
class Store {
 public:
  Store() = default;

  explicit Store(PyObject *py) noexcept : m_object(py) { Py_XINCREF(m_object); }

  Store(const Store &other) noexcept { *this = other; }
  Store &operator=(const Store &other) noexcept {
    // since Py_XDECREF and Py_XINCREF are essentially a ref count impl, this
    // design is safe as far as self-assignment is concerned, so there's no need
    // to check.

    Py_XDECREF(m_object);

    m_object = other.m_object;
    Py_XINCREF(m_object);

    return *this;
  }

  Store(Store &&other) noexcept { *this = std::move(other); }
  Store &operator=(Store &&other) noexcept {
    Py_XDECREF(m_object);
    m_object = std::exchange(other.m_object, nullptr);
    return *this;
  }

  ~Store() noexcept { Py_XDECREF(m_object); }

  Store &reset() noexcept {
    Py_XDECREF(m_object);
    m_object = nullptr;
    return *this;
  }

  Store &operator=(std::nullptr_t) noexcept { return reset(); }

  explicit operator bool() const { return nullptr != m_object; }

  PyObject *get() noexcept { return m_object; }
  PyObject *get() const noexcept { return m_object; }

 private:
  PyObject *m_object = nullptr;
};

/**
 * Used to automatically release new references.
 *
 * Does not increase the reference count, decreases it when releasing the Python
 * object.
 */
class Release {
 public:
  static Release incref(PyObject *py) {
    Py_XINCREF(py);
    return Release{py};
  }

 public:
  Release() = default;
  explicit Release(PyObject *py) noexcept : m_object(py) {}

  Release(const Release &other) = delete;
  Release &operator=(const Release &other) = delete;

  Release(Release &&other) noexcept { *this = std::move(other); }
  Release &operator=(Release &&other) noexcept {
    Py_XDECREF(m_object);
    m_object = std::exchange(other.m_object, nullptr);
    return *this;
  }

  ~Release() noexcept { Py_XDECREF(m_object); }

  Release &reset() noexcept {
    Py_XDECREF(m_object);
    m_object = nullptr;
    return *this;
  }

  Release &operator=(std::nullptr_t) noexcept { return reset(); }

  explicit operator bool() const { return nullptr != m_object; }

  PyObject *get() noexcept { return m_object; }
  PyObject *get() const noexcept { return m_object; }

  PyObject *release() noexcept { return std::exchange(m_object, nullptr); }

 private:
  PyObject *m_object = nullptr;
};
}  // namespace shcore::py

#endif
