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

#include "mysqlshdk/libs/storage/backend/in_memory/virtual_file.h"

#include <cassert>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/include/scripting/shexcept.h"
#include "mysqlshdk/libs/storage/backend/in_memory/virtual_directory.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

Virtual_file::Virtual_file(const std::string &path, Virtual_fs *fs) : m_fs(fs) {
  auto p = Virtual_fs::split_path(path);
  // second to last element is parent
  m_parent = m_fs->directory(p.rbegin()[1]);
  assert(m_parent);
  m_name = std::move(p.back());
}

Virtual_file::Virtual_file(const std::string &name, Virtual_fs *fs,
                           Virtual_fs::Directory *parent)
    : m_fs(fs), m_parent(parent), m_name(name) {}

void Virtual_file::open(Mode m) {
  assert(!is_open());

  m_file = m_parent->file(m_name);

  if (!m_file) {
    if (Mode::READ == m) {
      throw std::runtime_error("Failed to open file " + full_path().masked() +
                               " for reading, it does not exist");
    }

    m_file = m_parent->create_file(m_name);
  }

  m_file->open(Mode::READ == m);

  if (Mode::APPEND == m) {
    m_file->seek(m_file->size());
  }
}

bool Virtual_file::is_open() const { return m_file; }

void Virtual_file::close() {
  assert(is_open());

  m_file->close();
  m_parent->publish_created_file(m_name);

  m_file = nullptr;
}

size_t Virtual_file::file_size() const {
  const auto file = m_parent->file(m_name);

  if (!file) {
    throw std::runtime_error("Failed to get the file size of " +
                             full_path().masked() + ", it does not exist");
  }

  return file->size();
}

Masked_string Virtual_file::full_path() const {
  return Virtual_fs::join_path(m_parent->name(), m_name);
}

std::string Virtual_file::filename() const { return m_name; }

bool Virtual_file::exists() const { return m_parent->file(m_name); }

std::unique_ptr<IDirectory> Virtual_file::parent() const {
  return std::make_unique<Virtual_directory>(m_parent, m_fs);
}

off64_t Virtual_file::seek(off64_t offset) {
  assert(is_open());
  return m_file->seek(offset);
}

off64_t Virtual_file::tell() const {
  assert(is_open());
  return m_file->tell();
}

ssize_t Virtual_file::read(void *buffer, std::size_t length) {
  assert(is_open());
  return m_file->read(buffer, length);
}

ssize_t Virtual_file::write(const void *buffer, std::size_t length) {
  assert(is_open());
  return m_file->write(buffer, length);
}

bool Virtual_file::flush() {
  assert(is_open());
  return true;
}

void Virtual_file::rename(const std::string &name) {
  if (exists()) {
    m_parent->rename_file(m_name, name);
  }

  m_name = name;
}

void Virtual_file::remove() {
  if (exists()) {
    m_parent->remove_file(m_name);
  }
}

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk
