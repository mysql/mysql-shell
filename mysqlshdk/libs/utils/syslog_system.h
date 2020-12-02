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

#ifndef MYSQLSHDK_LIBS_UTILS_SYSLOG_SYSTEM_H_
#define MYSQLSHDK_LIBS_UTILS_SYSLOG_SYSTEM_H_

#include "mysqlshdk/libs/utils/syslog_level.h"

namespace shcore {
namespace syslog {

/**
 * Opens access to the system log using the given name.
 *
 * Not thread-safe.
 *
 * @param name name used to identify the application
 */
void open(const char *name);

/**
 * Logs given message to the system log using the specified log level.
 *
 * Not thread-safe.
 *
 * @param level log level
 * @param msg message to be logged
 */
void log(Level level, const char *msg);

/**
 * Closes access to the system log.
 *
 * Not thread-safe.
 */
void close();

/**
 * Callback invoked when a new message is logged.
 *
 * @param level log level
 * @param msg message
 * @param data user data given when registering the hook
 */
using callback_t = void (*)(Level level, const char *msg, void *data);

/**
 * Registers a hook which will be called whenever a new message is logged.
 *
 * Not thread-safe.
 *
 * @param hook callback
 * @param data user data
 */
void add_hook(callback_t hook, void *data);

/**
 * Removes the given hook.
 *
 * Hook which was not previously registered is ignored.
 *
 * Not thread-safe.
 *
 * @param hook callback
 * @param data user data
 */
void remove_hook(callback_t hook, void *data);

}  // namespace syslog
}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_SYSLOG_SYSTEM_H_
