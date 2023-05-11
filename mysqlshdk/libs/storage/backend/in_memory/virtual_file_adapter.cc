/*
 * Copyright (c) 2023, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/storage/backend/in_memory/virtual_file_adapter.h"

#include "mysqlshdk/libs/storage/backend/in_memory/virtual_fs.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

Virtual_file_adapter::Virtual_file_adapter(
    std::unique_ptr<Virtual_fs::IFile> file)
    : m_file(std::move(file)) {}

void Virtual_file_adapter::open(Mode m) {
  if (Mode::APPEND == m) {
    throw std::logic_error(
        "Virtual_file_adapter::open(Mode::APPEND) - not supported");
  }

  m_file->open(Mode::READ == m);
}

bool Virtual_file_adapter::is_open() const { return m_file->is_open(); }

void Virtual_file_adapter::close() { m_file->close(); }

size_t Virtual_file_adapter::file_size() const { return m_file->size(); }

Masked_string Virtual_file_adapter::full_path() const { return m_file->name(); }

std::string Virtual_file_adapter::filename() const {
  // IFile::name() can return just the file name or the full OS path, so
  // full_path() returns IFile::name(), and here we return just the basename
  return shcore::path::basename(m_file->name());
}

off64_t Virtual_file_adapter::seek(off64_t offset) {
  return m_file->seek(offset);
}

off64_t Virtual_file_adapter::tell() const { return m_file->tell(); }

ssize_t Virtual_file_adapter::read(void *buffer, size_t length) {
  return m_file->read(buffer, length);
}

ssize_t Virtual_file_adapter::write(const void *buffer, size_t length) {
  return m_file->write(buffer, length);
}

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk
