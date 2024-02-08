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

#include "mysqlshdk/libs/storage/backend/in_memory/virtual_fs.h"

#include <stdexcept>

#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/storage/backend/in_memory/allocated_file.h"
#include "mysqlshdk/libs/storage/backend/in_memory/synchronized_file.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

namespace {

constexpr auto k_path_separator = '/';

bool contains_path_separator(const std::string &name) {
  return std::string::npos != name.find(k_path_separator);
}

}  // namespace

Virtual_fs::Directory::Directory(const std::string &name, Virtual_fs *fs)
    : m_fs(fs), m_name(name) {}

Virtual_fs::IFile *Virtual_fs::Directory::file(const std::string &name) const {
  std::lock_guard lock{m_mutex};
  const auto d = m_files.find(name);
  return d == m_files.end() ? nullptr : d->second.get();
}

Virtual_fs::IFile *Virtual_fs::Directory::create_file(const std::string &name) {
  if (name.empty()) {
    throw std::runtime_error("Unable to create file with an empty name");
  }

  if (contains_path_separator(name)) {
    throw std::runtime_error("Unable to create file: " + name +
                             ", name contains an invalid character");
  }

  std::lock_guard lock{m_mutex};

  if (m_files.find(name) != m_files.end() ||
      m_created_files.find(name) != m_created_files.end()) {
    throw std::runtime_error("Unable to create file: " + name +
                             ", it already exists in directory " + m_name);
  }

  if (m_fs->m_uses_synchronized_io && m_fs->m_uses_synchronized_io(name)) {
    // these files are available right away, reader and writer are going to
    // synchronize I/O operations
    return m_files
        .emplace(name, std::make_unique<Synchronized_file>(
                           name, &m_fs->m_interrupted))
        .first->second.get();
  } else {
    return m_created_files
        .emplace(name,
                 std::make_unique<Allocated_file>(name, &m_fs->m_allocator))
        .first->second.get();
  }
}

std::unordered_set<IDirectory::File_info> Virtual_fs::Directory::list_files(
    const std::string &pattern) const {
  std::unordered_set<IDirectory::File_info> result;
  std::lock_guard lock{m_mutex};

  for (const auto &file : m_files) {
    if (pattern.empty() || shcore::match_glob(pattern, file.second->name())) {
      result.emplace(file.second->name(), file.second->size());
    }
  }

  return result;
}

void Virtual_fs::Directory::rename_file(const std::string &old_name,
                                        const std::string &new_name) {
  if (new_name.empty()) {
    throw std::runtime_error("Unable to rename file: " + old_name +
                             ", new name is empty");
  }

  if (contains_path_separator(new_name)) {
    throw std::runtime_error("Unable to rename file: " + old_name +
                             ", new name " + new_name +
                             " contains an invalid character");
  }

  std::lock_guard lock{m_mutex};
  auto file = m_files.find(old_name);
  auto source = &m_files;

  if (file == source->end()) {
    file = m_created_files.find(old_name);
    source = &m_created_files;
  }

  if (file != source->end()) {
    if (m_files.find(new_name) != m_files.end() ||
        m_created_files.find(new_name) != m_created_files.end()) {
      throw std::runtime_error("Unable to rename file: " + old_name +
                               ", file " + new_name +
                               " already exists in directory " + m_name);
    }

    auto node = source->extract(file);
    node.key() = new_name;
    node.mapped()->rename(new_name);
    source->insert(std::move(node));
  } else {
    throw std::runtime_error("Unable to rename file: " + old_name +
                             ", it does not exist");
  }
}

void Virtual_fs::Directory::remove_file(const std::string &name) {
  std::lock_guard lock{m_mutex};
  auto file = m_files.find(name);
  auto source = &m_files;

  if (file == source->end()) {
    file = m_created_files.find(name);
    source = &m_created_files;
  }

  if (file != source->end()) {
    if (file->second->is_open()) {
      throw std::runtime_error("Unable to remove file: " + name +
                               ", it is still open");
    }

    source->erase(file);
  } else {
    throw std::runtime_error("Unable to remove file: " + name +
                             ", it does not exist");
  }
}

void Virtual_fs::Directory::publish_created_file(const std::string &name) {
  std::lock_guard lock{m_mutex};
  auto node = m_created_files.extract(name);

  if (node) {
    m_files.insert(std::move(node));
  }
}

Virtual_fs::Virtual_fs(std::size_t page_size, std::size_t block_size)
    : m_allocator(page_size, block_size) {}

Virtual_fs::Directory *Virtual_fs::directory(const std::string &name) const {
  std::lock_guard lock{m_mutex};
  const auto d = m_dirs.find(name);
  return d == m_dirs.end() ? nullptr : d->second.get();
}

Virtual_fs::Directory *Virtual_fs::create_directory(const std::string &name) {
  if (name.empty()) {
    throw std::runtime_error("Unable to create directory with an empty name");
  }

  if (contains_path_separator(name)) {
    throw std::runtime_error("Unable to create directory: " + name +
                             ", name contains an invalid character");
  }

  std::lock_guard lock{m_mutex};

  if (m_dirs.find(name) != m_dirs.end()) {
    throw std::runtime_error("Unable to create directory: " + name +
                             ", it already exists");
  }

  return m_dirs.emplace(name, std::make_unique<Directory>(name, this))
      .first->second.get();
}

std::string Virtual_fs::join_path(const std::string &a, const std::string &b) {
  return a + k_path_separator + b;
}

std::vector<std::string> Virtual_fs::split_path(const std::string &path) {
  return shcore::str_split(path, std::string{1, k_path_separator});
}

void Virtual_fs::interrupt() { m_interrupted = true; }

void Virtual_fs::set_uses_synchronized_io(
    std::function<bool(std::string_view)> callback) {
  m_uses_synchronized_io = std::move(callback);
}

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk
