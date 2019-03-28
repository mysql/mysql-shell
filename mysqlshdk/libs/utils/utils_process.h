/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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
bool check_lock_file(const std::string &path, const char *pid_format = "%zd");
#endif

std::string run_and_catch_output(const char *const *argv,
                                 bool catch_stderr = false,
                                 int *out_rc = nullptr);

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_PROCESS_H_
