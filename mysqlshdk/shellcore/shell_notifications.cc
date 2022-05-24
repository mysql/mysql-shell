/*
 * Copyright (c) 2016, 2022, Oracle and/or its affiliates.
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

#include "shellcore/shell_notifications.h"
#include <algorithm>

namespace shcore {

NotificationObserver::~NotificationObserver() {
  for (const auto &notify : _notifications)
    ShellNotifications::get()->remove_observer(this, notify);

  _notifications.clear();
}

void NotificationObserver::observe_notification(
    const std::string &notification) {
  if (ShellNotifications::get()->add_observer(this, notification))
    _notifications.push_back(notification);
}

void NotificationObserver::ignore_notification(
    const std::string &notification) {
  if (ShellNotifications::get()->remove_observer(this, notification))
    _notifications.remove(notification);
}

ShellNotifications *ShellNotifications::get() {
  static ShellNotifications instance;
  return &instance;
}

bool ShellNotifications::add_observer(NotificationObserver *observer,
                                      const std::string &notification) {
  std::lock_guard lock(m_mutex);

  return m_observers[notification]
      .emplace(observer)
      .second;  // true if inserted
}

bool ShellNotifications::remove_observer(NotificationObserver *observer,
                                         const std::string &notification) {
  std::lock_guard lock(m_mutex);

  auto it = m_observers.find(notification);
  if (it == m_observers.end()) return false;

  auto &list = it->second;

  auto ret_val = list.erase(observer) != 0;
  if (list.empty()) m_observers.erase(it);

  return ret_val;
}

void ShellNotifications::notify(const std::string &name,
                                const shcore::Object_bridge_ref &sender,
                                shcore::Value::Map_type_ref data) const {
  std::lock_guard lock(m_mutex);

  auto it = m_observers.find(name);
  if (it == m_observers.end()) return;

  for (const auto &notify : it->second)
    notify->handle_notification(name, sender, data);
}

void ShellNotifications::notify(const std::string &name,
                                const shcore::Object_bridge_ref &sender) const {
  notify(name, sender, shcore::Value::Map_type_ref());
}
}  // namespace shcore
