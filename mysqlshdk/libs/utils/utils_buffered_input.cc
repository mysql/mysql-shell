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

#include "mysqlshdk/libs/utils/utils_buffered_input.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <deque>
#include <string>

namespace shcore {

void Buffered_input::open(const std::string &filepath_) {
  close();
#ifdef _WIN32
  m_fd = ::_open(filepath_.c_str(), O_RDONLY);
#else
  m_fd = ::open(filepath_.c_str(), O_RDONLY);
#endif
  if (m_fd < 0) {
    int err = errno;
    throw std::runtime_error(filepath_ + ": " + errno_to_string(err) +
                             " (error code " + std::to_string(err) + ")");
  }
}

void Buffered_input::close() {
  if (m_fd > 0) {
#ifdef _WIN32
    ::_close(m_fd);
#else
    ::close(m_fd);
#endif
  }
}

std::string Buffered_input::get_double_quoted_string() {
  std::string s;
  s += get();

  while (!eof()) {
    switch (peek()) {
      case '\\':
        s += get();
        s += get();
        break;

      case '"':
        s += get();
        return s;

      default:
        s += get();
    }
  }

  throw std::out_of_range("Incomplete quoted string");
}

void Buffered_input::fill_buffer() {
  if (m_eof) {
    return;
  }

  m_pos = m_buffer;
#ifdef _WIN32
  int bytes = ::_read(m_fd, m_buffer, BUFFER_SIZE);
#else
  ssize_t bytes = ::read(m_fd, m_buffer, BUFFER_SIZE);
#endif

  if (bytes < 0) {
    bytes = 0;
  }

  m_end = m_buffer + bytes;

  if (m_pos == m_end) {
    m_eof = true;
    *m_pos = '\0';
  }
}

}  // namespace shcore
