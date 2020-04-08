/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/include/shellcore/scoped_contexts.h"

#include <stack>

namespace mysqlsh {

namespace {

template <typename T>
class Scoped_storage {
 public:
  void push(const std::shared_ptr<T> &value) {
    assert(value);
    m_objects.push(value);
  }

  void pop(const std::shared_ptr<T> &value) {
    assert(!m_objects.empty() && m_objects.top() == value);
    (void)value;  // silence warning if NDEBUG=0
    m_objects.pop();
  }

  std::shared_ptr<T> get(bool allow_empty = false) const {
    if (allow_empty && m_objects.empty()) return std::shared_ptr<T>();

    assert(!m_objects.empty());
    return m_objects.top();
  }

 private:
  std::stack<std::shared_ptr<T>> m_objects;
};

class Multi_storage : public Scoped_storage<mysqlsh::IConsole>,
                      public Scoped_storage<mysqlsh::Shell_options>,
                      public Scoped_storage<shcore::Interrupts>,
                      public Scoped_storage<shcore::Logger> {};

thread_local Multi_storage mstorage;

}  // namespace

template <typename T>
Global_scoped_object<T>::Global_scoped_object(
    const std::shared_ptr<T> &scoped_value,
    const std::function<void(const std::shared_ptr<T> &)> &deleter)
    : m_deleter(deleter), m_scoped_value(scoped_value) {
  if (scoped_value) mstorage.Scoped_storage<T>::push(scoped_value);
}

template <typename T>
Global_scoped_object<T>::~Global_scoped_object() {
  if (m_deleter) {
    m_deleter(m_scoped_value);
  }

  if (m_scoped_value) {
    mstorage.Scoped_storage<T>::pop(m_scoped_value);
  }
}
template <typename T>
std::shared_ptr<T> Global_scoped_object<T>::get() const {
  return m_scoped_value;
}

template class Global_scoped_object<shcore::Interrupts>;
template class Global_scoped_object<mysqlsh::IConsole>;
template class Global_scoped_object<mysqlsh::Shell_options>;
template class Global_scoped_object<shcore::Logger>;

std::shared_ptr<IConsole> current_console(bool allow_empty) {
  return mstorage.Scoped_storage<mysqlsh::IConsole>::get(allow_empty);
}

std::shared_ptr<Shell_options> current_shell_options(bool allow_empty) {
  return mstorage.Scoped_storage<mysqlsh::Shell_options>::get(allow_empty);
}

}  // namespace mysqlsh

std::shared_ptr<shcore::Interrupts> shcore::current_interrupt(
    bool allow_empty) {
  return mysqlsh::mstorage.mysqlsh::Scoped_storage<shcore::Interrupts>::get(
      allow_empty);
}

std::shared_ptr<shcore::Logger> shcore::current_logger(bool allow_empty) {
  return mysqlsh::mstorage.mysqlsh::Scoped_storage<shcore::Logger>::get(
      allow_empty);
}
