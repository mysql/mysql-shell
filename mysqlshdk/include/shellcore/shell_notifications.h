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

#ifndef _SHELLNOTIFICATIONS_H_
#define _SHELLNOTIFICATIONS_H_

#include "scripting/common.h"
#include "scripting/types.h"

namespace shcore {
class SHCORE_PUBLIC NotificationObserver {
 public:
  virtual void handle_notification(const std::string &name,
                                   const shcore::Object_bridge_ref &sender,
                                   shcore::Value::Map_type_ref data) = 0;
  void observe_notification(const std::string &notification);
  void ignore_notification(const std::string &notification);
  virtual ~NotificationObserver();

 private:
  std::list<std::string> _notifications;
};

typedef std::list<NotificationObserver *> ObserverList;

class SHCORE_PUBLIC ShellNotifications {
 private:
  ShellNotifications() {}
  std::map<std::string, ObserverList *> _observers;

  static ShellNotifications *_instance;

 public:
  static ShellNotifications *get();
  virtual ~ShellNotifications();

  bool add_observer(NotificationObserver *observer,
                    const std::string &notification);
  bool remove_observer(NotificationObserver *observer,
                       const std::string &notification);
  void notify(const std::string &name, const shcore::Object_bridge_ref &sender,
              shcore::Value::Map_type_ref data);
  void notify(const std::string &name, const shcore::Object_bridge_ref &sender);
};
}  // namespace shcore

#define DEBUG_NOTIFICATION(X)                                        \
  {                                                                  \
    shcore::Value::Map_type_ref data(new shcore::Value::Map_type()); \
    (*data)["value"] = shcore::Value(X);                             \
    shcore::ShellNotifications::get()->notify(                       \
        "SN_DEBUGGER", shcore::Object_bridge_ref(), data);           \
  }

#endif  // _SHELLCORE_H_
