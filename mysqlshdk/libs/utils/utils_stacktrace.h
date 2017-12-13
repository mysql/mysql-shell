/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

// These routines allow retrieving the call stack, but are platform
// dependent. Look at the .cc file for supported platforms.
// They will not do anything if the functionality is not supported.

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_STACKTRACE_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_STACKTRACE_H_

#include <string>
#include <vector>

namespace mysqlshdk {
namespace utils {

void init_stacktrace();

void print_stacktrace();

std::vector<std::string> get_stacktrace();

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_STACKTRACE_H_
