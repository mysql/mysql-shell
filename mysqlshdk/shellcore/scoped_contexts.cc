/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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
  std::shared_ptr<T> get() const {
    assert(!m_objects.empty());
    return m_objects.top();
  }

  void push(const std::shared_ptr<T> &object) {
    assert(object);
    m_objects.push(object);
  }

  void pop(const std::shared_ptr<T> &object) {
    assert(!m_objects.empty() && m_objects.top() == object);
    m_objects.pop();
  }

 private:
  // TODO(pawel): use separate stack per each thread
  std::stack<std::shared_ptr<T>> m_objects;
};

Scoped_storage<mysqlsh::IConsole> g_console_storage;

Scoped_storage<mysqlsh::Shell_options> g_options_storage;

}  // namespace

Scoped_console::Scoped_console(
    const std::shared_ptr<mysqlsh::IConsole> &console)
    : m_console{console} {
  g_console_storage.push(m_console);
}

Scoped_console::~Scoped_console() { g_console_storage.pop(m_console); }

std::shared_ptr<mysqlsh::IConsole> Scoped_console::get() const {
  return m_console;
}

Scoped_shell_options::Scoped_shell_options(
    const std::shared_ptr<mysqlsh::Shell_options> &options)
    : m_options{options} {
  g_options_storage.push(m_options);
}

Scoped_shell_options::~Scoped_shell_options() {
  g_options_storage.pop(m_options);
}

std::shared_ptr<mysqlsh::Shell_options> Scoped_shell_options::get() const {
  return m_options;
}

std::shared_ptr<IConsole> current_console() { return g_console_storage.get(); }

std::shared_ptr<Shell_options> current_shell_options() {
  return g_options_storage.get();
}

}  // namespace mysqlsh
