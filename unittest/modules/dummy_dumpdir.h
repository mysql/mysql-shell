/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace tests {

class Dummy_dump_directory;

class Dummy_dump_file : public mysqlshdk::storage::IFile {
 public:
  Dummy_dump_file(const Dummy_dump_directory *parent, const std::string &name,
                  size_t size)
      : m_name(name), m_size(size), m_parent(parent) {}

  void open(mysqlshdk::storage::Mode) override {}
  bool is_open() const override { return true; }
  int error() const override { return 0; }
  void close() override {}

  size_t file_size() const override { return m_size; }

  mysqlshdk::Masked_string full_path() const override { return m_name; }

  std::string filename() const override { return m_name; }

  bool exists() const override { return true; }

  off64_t seek(off64_t offset) override {
    m_offset = offset;
    return offset;
  }

  off64_t tell() const override { return m_offset; }

  ssize_t read(void *, size_t length) override {
    if (m_offset < m_size) {
      size_t d = std::min(length, m_size - m_offset);
      m_offset += d;
      return d;
    }
    return 0;
  }

  ssize_t write(const void *, size_t) override {
    throw std::logic_error("bad call");
  }

  bool flush() override { return true; }

  void rename(const std::string &) override {}

  void remove() override {}

  std::unique_ptr<mysqlshdk::storage::IDirectory> parent() const override;

  bool is_local() const override { return false; }

 private:
  std::string m_name;
  size_t m_size;
  size_t m_offset = 0;
  const Dummy_dump_directory *m_parent = nullptr;
};

// Mocks a dump directory based on a file list
class Dummy_dump_directory : public mysqlshdk::storage::IDirectory {
 public:
  Dummy_dump_directory(const std::string &dumpdir, const File_info *file_list,
                       size_t file_count)
      : m_dumpdir(dumpdir), m_file_list(file_list, file_list + file_count) {}

  bool exists() const override { return true; }

  void create() override {}

  void close() override {}

  mysqlshdk::Masked_string full_path() const override { return m_dumpdir; }

  std::unordered_set<File_info> list_files(bool) const override {
    return {m_file_list.begin(), m_file_list.end()};
  }

  std::unordered_set<File_info> filter_files(
      const std::string &pattern) const override {
    std::unordered_set<File_info> filtered_files;
    for (const auto &f : m_file_list) {
      if (shcore::match_glob(pattern, f.name())) {
        filtered_files.emplace(f);
      }
    }
    return filtered_files;
  }

  std::unique_ptr<mysqlshdk::storage::IFile> file(
      const std::string &name,
      const mysqlshdk::storage::File_options &options = {}) const override {
    if (shcore::str_endswith(name, ".sql") ||
        shcore::str_endswith(name, ".json")) {
      return mysqlshdk::storage::make_file(join_path(m_dumpdir, name), options);
    } else if (shcore::str_endswith(name, ".zst") ||
               shcore::str_endswith(name, ".idx")) {
      for (const auto &f : m_file_list) {
        if (f.name() == name) {
          return std::make_unique<Dummy_dump_file>(this, name, f.size());
        }
      }
      throw std::runtime_error(name + ": File not found");
    } else {
      throw std::logic_error("Unexpected file type " + name);
    }
  }

  bool is_local() const override { return false; }

  std::string join_path(const std::string &a,
                        const std::string &b) const override {
    return shcore::path::join_path(a, b);
  }

 private:
  friend class Dummy_dump_file;

  std::string m_dumpdir;
  std::vector<File_info> m_file_list;
};

}  // namespace tests

#endif  // UNITTEST_MODULES_DUMMY_DUMPDIR_H_
