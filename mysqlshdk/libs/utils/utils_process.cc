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

#include "mysqlshdk/libs/utils/utils_process.h"
#ifndef _WIN32
#include <signal.h>
#endif
#include "mysqlshdk/libs/utils/utils_file.h"

namespace mysqlshdk {
namespace utils {

#ifndef _WIN32
bool check_lock_file(const std::string &path) {
  std::string data = shcore::get_text_file(path);
  int64_t pid = std::stoi(data);
  if (kill(pid, 0) < 0) {
    if (errno == ESRCH) {
      // invalid pid
      return false;
    } else if (errno == EPERM) {
      // no permission to kill the process means the process exists
      return true;
    }
  }
  // kill would succeed, process exists
  return true;
}
#endif

}  // namespace utils
}  // namespace mysqlshdk
