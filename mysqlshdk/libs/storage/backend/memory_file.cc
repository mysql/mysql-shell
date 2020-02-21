/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/storage/backend/memory_file.h"

#include <iterator>
#include <string>

namespace mysqlshdk {
namespace storage {
namespace backend {

Memory_file::Memory_file(const std::string & /* filename */) {
  m_content.reserve(4096);
}

void Memory_file::open(Mode m) {
  m_open_mode = m;
  switch (m) {
    case Mode::READ:
      m_offset = 0;
      break;
    case Mode::WRITE:
      m_offset = 0;
      m_content.resize(0);
      break;
    case Mode::APPEND:
      m_offset = m_content.size();
      break;
  }
}

bool Memory_file::is_open() const { return !m_open_mode.is_null(); }

void Memory_file::close() { m_open_mode.reset(); }

size_t Memory_file::file_size() const { return m_content.size(); }

std::string Memory_file::full_path() const {
#ifdef _WIN32
  return "NUL";
#else
  return "/dev/null";
#endif  // _WIN32
}

std::string Memory_file::filename() const {
#ifdef _WIN32
  return "NUL";
#else
  return "/dev/null";
#endif  // _WIN32
}

bool Memory_file::exists() const { return true; }

off64_t Memory_file::seek(off64_t offset) {
  if (offset < 0) {
    offset = 0;
  }

  if (static_cast<size_t>(offset) > m_content.size()) {
    offset = m_content.size();
  }

  m_offset = offset;
  return 0;
}

ssize_t Memory_file::read(void *buffer, size_t length) {
  char *p = static_cast<char *>(buffer);
  auto r = m_content.copy(p, length, m_offset);
  m_offset += r;
  return r;
}

ssize_t Memory_file::write(const void *buffer, size_t length) {
  char *p = static_cast<char *>(const_cast<void *>(buffer));
  m_content.insert(m_content.begin() + m_offset, p, p + length);
  m_offset += length;
  return length;
}

bool Memory_file::flush() { return true; }

void Memory_file::rename(const std::string & /* new_name */) {}

}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
