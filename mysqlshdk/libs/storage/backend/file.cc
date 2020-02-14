/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/storage/backend/file.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cassert>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <utility>

#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace mysqlshdk {
namespace storage {
namespace backend {

File::File(const std::string &filename)
    : m_filepath(
          shcore::path::expand_user(utils::strip_scheme(filename, "file"))) {}

void File::open(Mode m) {
  assert(!is_open());
  m_fd = -1;

#ifdef _WIN32
  int oflag = _O_BINARY;

  switch (m) {
    case Mode::READ:
      oflag |= _O_RDONLY;
      break;

    case Mode::WRITE:
      oflag |= _O_CREAT | _O_WRONLY | _O_TRUNC;
      break;

    case Mode::APPEND:
      oflag |= _O_CREAT | _O_WRONLY | _O_APPEND;
      break;
  }

  _sopen_s(&m_fd, m_filepath.c_str(), oflag, _SH_DENYWR, _S_IREAD | _S_IWRITE);
#else
  int flags = 0;

  switch (m) {
    case Mode::READ:
      flags |= O_RDONLY;
      break;

    case Mode::WRITE:
      flags |= O_CREAT | O_WRONLY | O_TRUNC;
      break;

    case Mode::APPEND:
      flags |= O_CREAT | O_WRONLY | O_APPEND;
      break;
  }

  m_fd = ::open(m_filepath.c_str(), flags, S_IRUSR | S_IWUSR | S_IRGRP);
#endif

  if (!is_open()) {
    throw std::runtime_error("Cannot open file '" + full_path() +
                             "': " + shcore::errno_to_string(errno));
  }
}

bool File::is_open() const { return m_fd >= 0; }

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

size_t File::file_size() const { return shcore::file_size(m_filepath); }

std::string File::full_path() const { return m_filepath; }

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

ssize_t File::write(const void *buffer, size_t length) {
  assert(is_open());
#ifdef _WIN32
  const int bytes = _write(m_fd, buffer, length);
#else
  const ssize_t bytes = ::write(m_fd, buffer, length);
#endif
  return bytes;
}

bool File::flush() {
  assert(is_open());
#ifdef _WIN32
  const int ret = _commit(m_fd);
#else
  const int ret = ::fsync(m_fd);
#endif
  return 0 == ret;
}

bool File::exists() const { return shcore::is_file(full_path()); }

void File::rename(const std::string &new_name) {
  assert(!is_open());

  if (std::any_of(std::begin(new_name), std::end(new_name),
                  shcore::path::is_path_separator)) {
    throw std::logic_error("Move operation is not supported.");
  }

  auto new_path =
      shcore::path::join_path(shcore::path::dirname(full_path()), new_name);

  if (exists()) {
    shcore::rename_file(full_path(), new_path);
  }

  m_filepath = std::move(new_path);
}

std::string File::filename() const {
  return shcore::path::basename(full_path());
}

}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
