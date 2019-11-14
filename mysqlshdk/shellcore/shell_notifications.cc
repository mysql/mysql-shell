/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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
ShellNotifications *ShellNotifications::_instance = NULL;

NotificationObserver::~NotificationObserver() {
  std::list<std::string>::iterator it, end = _notifications.end();

  for (it = _notifications.begin(); it != end; it++)
    ShellNotifications::get()->remove_observer(this, *it);

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
  if (!_instance) _instance = new ShellNotifications();

  return _instance;
}

ShellNotifications::~ShellNotifications() {}

bool ShellNotifications::add_observer(NotificationObserver *observer,
                                      const std::string &notification) {
  bool ret_val = false;

  // Gets the observer list for the given notification
  // creates the list if it is the first observer
  if (_observers.find(notification) == _observers.end())
    _observers[notification] = new ObserverList();

  // Adds the observer if it does not exists already
  ObserverList *list = _observers[notification];
  ret_val = std::find(list->begin(), list->end(), observer) == list->end();
  if (ret_val) _observers[notification]->push_back(observer);

  return ret_val;
}

bool ShellNotifications::remove_observer(NotificationObserver *observer,
                                         const std::string &notification) {
  bool ret_val = false;

  if (_observers.find(notification) != _observers.end()) {
    ObserverList *list = _observers[notification];

    ObserverList::iterator it = list->begin();
    while (it != list->end()) {
      if (*it == observer) {
        ret_val = true;
        list->erase(it);
        break;
      } else
        it++;
    }

    if (list->size() == 0) {
      delete list;
      _observers.erase(notification);
    }
  }

  return ret_val;
}

void ShellNotifications::notify(const std::string &name,
                                const shcore::Object_bridge_ref &sender,
                                shcore::Value::Map_type_ref data) {
  if (_observers.find(name) != _observers.end()) {
    ObserverList *list = _observers[name];

    ObserverList::iterator it = list->begin();
    while (it != list->end()) {
      auto next = it;
      ++next;
      (*it)->handle_notification(name, sender, data);
      it = next;
    }
  }
}

void ShellNotifications::notify(const std::string &name,
                                const shcore::Object_bridge_ref &sender) {
  notify(name, sender, shcore::Value::Map_type_ref());
}
}  // namespace shcore
