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

#include "mysqlshdk/libs/storage/compressed_file.h"

#include <utility>

namespace mysqlshdk {
namespace storage {

Compressed_file::Compressed_file(std::unique_ptr<IFile> file)
    : m_file(std::move(file)) {}

void Compressed_file::open(Mode m) { m_file->open(m); }

bool Compressed_file::is_open() const { return m_file->is_open(); }

void Compressed_file::close() { m_file->close(); }

size_t Compressed_file::file_size() const { return m_file->file_size(); }

std::string Compressed_file::full_path() const { return m_file->full_path(); }

bool Compressed_file::exists() const { return m_file->exists(); }

void Compressed_file::rename(const std::string &new_name) {
  m_file->rename(new_name);
}

std::string Compressed_file::filename() const { return m_file->filename(); }

}  // namespace storage
}  // namespace mysqlshdk
