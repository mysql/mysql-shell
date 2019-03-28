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

#include "mysqlshdk/libs/utils/utils_process.h"
#include <cstdio>
#include <cstring>
#ifndef _WIN32
#include <signal.h>
#endif
#include <errno.h>
#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/libs/utils/utils_file.h"

namespace mysqlshdk {
namespace utils {

#ifndef _WIN32
bool check_lock_file(const std::string &path, const char *pid_format) {
  std::string data;
  if (!shcore::load_text_file(path, data)) {
    if (errno == ENOENT) return false;
    throw std::runtime_error(path + ": " + strerror(errno));
  }
  size_t pid = 0;
  if (sscanf(data.c_str(), pid_format, &pid) != 1) {
    throw std::runtime_error("Unexpected format in lock file " + path);
  }
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

std::string run_and_catch_output(const char *const *argv, bool catch_stderr,
                                 int *out_rc) {
  shcore::Process p(argv, catch_stderr);

  p.start();
  std::string r = p.read_all();

  if (out_rc)
    *out_rc = p.wait();
  else
    p.wait();

  return r;
}

}  // namespace utils
}  // namespace mysqlshdk
