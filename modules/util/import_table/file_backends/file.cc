/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/util/import_table/file_backends/file.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cassert>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace mysqlsh {
namespace import_table {

File::File(const std::string &filename)
    : m_filepath(shcore::path::expand_user(filename)) {}

void File::open() {
  assert(!is_open());
  m_fd = -1;
#ifdef _WIN32
  _sopen_s(&m_fd, m_filepath.c_str(), _O_BINARY | _O_RDONLY, _SH_DENYWR,
           _S_IREAD);
#else
  m_fd = ::open(m_filepath.c_str(), O_RDONLY);
#endif
}

bool File::is_open() { return m_fd >= 0; }

void File::close() {
  if (m_fd >= 0) {
#ifdef _WIN32
    ::_close(m_fd);
#else
    ::close(m_fd);
#endif
  }
  m_fd = -1;
}

size_t File::file_size() { return shcore::file_size(m_filepath); }

std::string File::file_name() { return m_filepath; }

off64_t File::seek(off64_t offset) {
  assert(is_open());
#if defined(_WIN32)
  return _lseeki64(m_fd, offset, SEEK_SET);
#elif defined(__APPLE__)
  return lseek(m_fd, offset, SEEK_SET);
#else
  return lseek64(m_fd, offset, SEEK_SET);
#endif
}

ssize_t File::read(void *buffer, size_t length) {
  assert(is_open());
#ifdef _WIN32
  const int bytes = _read(m_fd, buffer, length);
#else
  const ssize_t bytes = ::read(m_fd, buffer, length);
#endif
  return bytes;
}

}  // namespace import_table
}  // namespace mysqlsh
