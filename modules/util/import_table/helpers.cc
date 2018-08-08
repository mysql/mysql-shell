/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/util/import_table/helpers.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

#include <string>

namespace mysqlsh {
namespace import_table {
namespace detail {

int file_open(const std::string &pathname) {
  int fd = -1;
#ifdef _WIN32
  _sopen_s(&fd, pathname.c_str(), _O_BINARY | _O_RDONLY, _SH_DENYWR, _S_IREAD);
#else
  fd = ::open(pathname.c_str(), O_RDONLY);
#endif
  return fd;
}

void file_close(int fd) {
  if (fd >= 0) {
#ifdef _WIN32
    _close(fd);
#else
    close(fd);
#endif
  }
}
}  // namespace detail
}  // namespace import_table
}  // namespace mysqlsh
