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

#ifndef UNITTEST_MODULES_DUMMY_DUMPDIR_H_
#define UNITTEST_MODULES_DUMMY_DUMPDIR_H_

#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/utils/utils_string.h"

class Dummy_dump_file : public mysqlshdk::storage::IFile {
 public:
  Dummy_dump_file(const std::string &name, size_t size)
      : m_name(name), m_size(size) {}

  void open(mysqlshdk::storage::Mode) {}
  bool is_open() const { return true; }
  int error() const { return 0; }
  void close() {}

  size_t file_size() const { return m_size; }

  std::string full_path() const { return m_name; }

  std::string filename() const { return m_name; }

  bool exists() const { return true; }

  off64_t seek(off64_t offset) {
    m_offset = offset;
    return offset;
  }

  off64_t tell() const { return m_offset; }

  ssize_t read(void *, size_t length) {
    if (m_offset < m_size) {
      size_t d = std::min(length, m_size - m_offset);
      m_offset += d;
      return d;
    }
    return 0;
  }

  ssize_t write(const void *, size_t) { throw std::logic_error("bad call"); }

  bool flush() { return true; }

  void rename(const std::string &) {}

  void remove() {}

  std::unique_ptr<mysqlshdk::storage::IDirectory> parent() const { return {}; }

 private:
  std::string m_name;
  size_t m_size;
  size_t m_offset = 0;
};

// Mocks a dump directory based on a file list
class Dummy_dump_directory : public mysqlshdk::storage::IDirectory {
 public:
  Dummy_dump_directory(const std::string &dumpdir, const File_info *file_list,
                       size_t file_count)
      : m_dumpdir(dumpdir), m_file_list(file_list, file_list + file_count) {}

  bool exists() const { return true; }

  void create() {}

  void close() {}

  std::string full_path() const { return m_dumpdir; }

  std::vector<File_info> list_files(bool) const { return m_file_list; }

  std::unique_ptr<mysqlshdk::storage::IFile> file(
      const std::string &name) const {
    if (shcore::str_endswith(name, ".sql") ||
        shcore::str_endswith(name, ".json")) {
      return mysqlshdk::storage::make_file(join_path(m_dumpdir, name));
    } else if (shcore::str_endswith(name, ".zst")) {
      for (const auto &f : m_file_list) {
        if (f.name == name) {
          return std::make_unique<Dummy_dump_file>(name, f.size);
        }
      }
      throw std::runtime_error(name + ": File not found");
    } else {
      throw std::logic_error("Unexpected file type " + name);
    }
  }

  std::string join_path(const std::string &a, const std::string &b) const {
    return shcore::path::join_path(a, b);
  }

 private:
  std::string m_dumpdir;
  std::vector<File_info> m_file_list;
};

#endif  // UNITTEST_MODULES_DUMMY_DUMPDIR_H_
