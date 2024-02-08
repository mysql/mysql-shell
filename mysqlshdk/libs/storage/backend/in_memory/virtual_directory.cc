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

#include "mysqlshdk/libs/storage/backend/in_memory/virtual_directory.h"

#include <stdexcept>

#include "mysqlshdk/libs/storage/backend/in_memory/virtual_file.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

Virtual_directory::Virtual_directory(const std::string &name, Virtual_fs *fs)
    : m_fs(fs), m_dir(fs->directory(name)), m_name(name) {}

Virtual_directory::Virtual_directory(Virtual_fs::Directory *dir, Virtual_fs *fs)
    : m_fs(fs), m_dir(dir), m_name(dir->name()) {}

bool Virtual_directory::exists() const {
  if (!m_dir) {
    m_dir = m_fs->directory(m_name);
  }

  return m_dir;
}

void Virtual_directory::create() {
  if (exists()) {
    throw std::runtime_error("Failed create directory " + m_name +
                             ", it already exists");
  }

  m_dir = m_fs->create_directory(m_name);
}

Masked_string Virtual_directory::full_path() const { return m_name; }

std::unordered_set<IDirectory::File_info> Virtual_directory::list_files(
    bool) const {
  if (!exists()) {
    throw std::runtime_error("Failed to list files, directory " + m_name +
                             " does not exist");
  }

  return m_dir->list_files();
}

std::unordered_set<IDirectory::File_info> Virtual_directory::filter_files(
    const std::string &pattern) const {
  if (!exists()) {
    throw std::runtime_error("Failed to filter files, directory " + m_name +
                             " does not exist");
  }

  return m_dir->list_files(pattern);
}

std::unique_ptr<IFile> Virtual_directory::file(const std::string &name,
                                               const File_options &) const {
  if (!exists()) {
    throw std::runtime_error("Failed to fetch file, directory " + m_name +
                             " does not exist");
  }

  return std::make_unique<Virtual_file>(name, m_fs, m_dir);
}

std::string Virtual_directory::join_path(const std::string &a,
                                         const std::string &b) const {
  return Virtual_fs::join_path(a, b);
}

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk
