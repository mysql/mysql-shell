/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/include/shellcore/scoped_contexts.h"

#include <atomic>
#include <stack>

namespace mysqlsh {

namespace {
template <typename T>
class Scoped_storage {
 public:
  void push(const std::shared_ptr<T> &value) {
    std::lock_guard<std::mutex> lock(m_mtx);
    assert(value);
    m_objects.push(value);
  }

  void pop(const std::shared_ptr<T> &value) noexcept {
    std::lock_guard<std::mutex> lock(m_mtx);
    assert(!m_objects.empty() && m_objects.top() == value);
    (void)value;  // silence warning if NDEBUG=0
    m_objects.pop();
  }

  std::shared_ptr<T> get(bool allow_empty = false) const noexcept {
    std::lock_guard<std::mutex> lock(m_mtx);
    if (allow_empty && m_objects.empty()) return nullptr;

    assert(!m_objects.empty());
    return m_objects.top();
  }

  bool empty() const noexcept {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_objects.empty();
  }

 private:
  std::stack<std::shared_ptr<T>> m_objects;
  mutable std::mutex m_mtx;
};

class Multi_storage : public Scoped_storage<mysqlsh::IConsole>,
                      public Scoped_storage<mysqlsh::Shell_options>,
                      public Scoped_storage<shcore::Interrupts>,
                      public Scoped_storage<shcore::Logger>,
                      public Scoped_storage<mysqlshdk::ssh::Ssh_manager>,
                      public Scoped_storage<shcore::Log_sql>,
                      public Scoped_storage<shcore::Sql_handler_registry> {};

thread_local Multi_storage mstorage;

std::atomic<Multi_storage *> g_main_mstorage{nullptr};

}  // namespace

template <typename T>
Global_scoped_object<T>::Global_scoped_object(
    std::shared_ptr<T> scoped_value,
    std::function<void(const std::shared_ptr<T> &)> on_delete_cb)
    : m_scoped_value{std::move(scoped_value)},
      m_on_delete_cb{std::move(on_delete_cb)} {
  if (m_scoped_value) mstorage.Scoped_storage<T>::push(m_scoped_value);

  // assumes the first scoped object is created in the main thread
  Multi_storage *desired{nullptr};
  g_main_mstorage.compare_exchange_strong(desired, &mstorage);
}

template <typename T>
Global_scoped_object<T>::~Global_scoped_object() {
  if (m_on_delete_cb) m_on_delete_cb(m_scoped_value);
  if (m_scoped_value) mstorage.Scoped_storage<T>::pop(m_scoped_value);
}

template class Global_scoped_object<shcore::Interrupts>;
template class Global_scoped_object<mysqlsh::IConsole>;
template class Global_scoped_object<mysqlsh::Shell_options>;
template class Global_scoped_object<shcore::Logger>;
template class Global_scoped_object<mysqlshdk::ssh::Ssh_manager>;
template class Global_scoped_object<shcore::Log_sql>;
template class Global_scoped_object<shcore::Sql_handler_registry>;

namespace detail {
template <typename T>
std::shared_ptr<T> current_scoped_value(bool allow_empty) {
  auto g_storage = g_main_mstorage.load();
  auto storage =
      mstorage.Scoped_storage<T>::empty() && g_storage ? g_storage : &mstorage;
  return storage->Scoped_storage<T>::get(allow_empty);
}
}  // namespace detail

std::shared_ptr<IConsole> current_console(bool allow_empty) {
  return detail::current_scoped_value<IConsole>(allow_empty);
}

std::shared_ptr<Shell_options> current_shell_options(bool allow_empty) {
  return detail::current_scoped_value<Shell_options>(allow_empty);
}
}  // namespace mysqlsh

std::shared_ptr<shcore::Interrupts> shcore::current_interrupt(
    bool allow_empty) {
  return mysqlsh::detail::current_scoped_value<shcore::Interrupts>(allow_empty);
}

std::shared_ptr<shcore::Logger> shcore::current_logger(bool allow_empty) {
  return mysqlsh::detail::current_scoped_value<shcore::Logger>(allow_empty);
}

std::shared_ptr<mysqlshdk::ssh::Ssh_manager>
mysqlshdk::ssh::current_ssh_manager(bool allow_empty) {
  return mysqlsh::detail::current_scoped_value<mysqlshdk::ssh::Ssh_manager>(
      allow_empty);
}

std::shared_ptr<shcore::Log_sql> shcore::current_log_sql(bool allow_empty) {
  return mysqlsh::detail::current_scoped_value<shcore::Log_sql>(allow_empty);
}

std::shared_ptr<shcore::Sql_handler_registry>
shcore::current_sql_handler_registry(bool allow_empty) {
  return mysqlsh::detail::current_scoped_value<shcore::Sql_handler_registry>(
      allow_empty);
}
