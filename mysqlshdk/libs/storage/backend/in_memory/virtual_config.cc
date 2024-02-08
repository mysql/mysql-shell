/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/storage/backend/in_memory/virtual_config.h"

#include "mysqlshdk/libs/storage/backend/in_memory/virtual_directory.h"
#include "mysqlshdk/libs/storage/backend/in_memory/virtual_file.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

Virtual_config::Virtual_config(std::size_t page_size)
    : m_fs(std::make_unique<Virtual_fs>(page_size)) {}

std::unique_ptr<IFile> Virtual_config::file(const std::string &name) const {
  return std::make_unique<Virtual_file>(name, m_fs.get());
}

std::unique_ptr<IDirectory> Virtual_config::directory(
    const std::string &name) const {
  return std::make_unique<Virtual_directory>(name, m_fs.get());
}

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk
