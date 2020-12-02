/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/syslog_system.h"

#include <syslog.h>

namespace shcore {
namespace syslog {

void open(const char *name) { ::openlog(name, LOG_NDELAY | LOG_PID, LOG_USER); }

void log(Level level, const char *msg) {
  int priority = LOG_INFO;

  switch (level) {
    case Level::ERROR:
      priority = LOG_ERR;
      break;

    case Level::WARN:
      priority = LOG_WARNING;
      break;

    case Level::INFO:
      priority = LOG_INFO;
      break;
  }

  ::syslog(priority, "%s", msg);
}

void close() { ::closelog(); }

}  // namespace syslog
}  // namespace shcore
