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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_PROCESS_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_PROCESS_H_

#include <string>

namespace mysqlshdk {
namespace utils {

#ifndef _WIN32
/*
 * Check process lock file at the given path.
 *
 * The file is assumed to contain the pid of the process owning it.
 *
 * Returns true if:
 *    File exists, is readable and the pid corresponds to a running process.
 *
 * If the file exists but is not readable or the contents are not valid
 * (eg not a number), it will throw a runtime_error.
 *
 * Note: this style of lock file is common in Unix, but not in Windows (afaik).
 * If it turns out this is also useful in Windows, the #ifndef should be removed
 * and implemented for Windows
 */
bool check_lock_file(const std::string &path);
#endif

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_PROCESS_H_
