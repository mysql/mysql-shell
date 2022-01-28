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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_MEMORY_FILE_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_MEMORY_FILE_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/storage/ifile.h"
#include "mysqlshdk/libs/utils/nullable.h"

namespace mysqlshdk {
namespace storage {
namespace backend {

class Memory_file : public IFile {
 public:
  Memory_file() = delete;
  explicit Memory_file(const std::string &filename);
  Memory_file(const Memory_file &other) = delete;
  Memory_file(Memory_file &&other) = default;

  Memory_file &operator=(const Memory_file &other) = delete;
  Memory_file &operator=(Memory_file &&other) = default;

  ~Memory_file() override = default;

  void open(Mode m) override;
  bool is_open() const override;
  int error() const override;
  void close() override;

  size_t file_size() const override;
  Masked_string full_path() const override;
  std::string filename() const override;
  bool exists() const override;

  std::unique_ptr<IDirectory> parent() const override {
    throw std::logic_error("Memory_file::parent() - not implemented");
  }

  off64_t seek(off64_t offset) override;

  off64_t tell() const override { return m_offset; }

  ssize_t read(void *buffer, size_t length) override;
  ssize_t write(const void *buffer, size_t length) override;
  bool flush() override;

  void rename(const std::string &new_name) override;
  void remove() override;

  bool is_local() const override { return true; }

  void set_content(const std::string &s);
  const std::string &content() const { return m_content; }

 private:
  std::string m_content;
  off64_t m_offset = 0;
  mysqlshdk::utils::nullable<Mode> m_open_mode{nullptr};
};
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_MEMORY_FILE_H_
