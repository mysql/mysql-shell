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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_FILE_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_FILE_H_

#include <memory>
#include <stdexcept>

#include "mysqlshdk/libs/storage/ifile.h"

#include "mysqlshdk/libs/storage/backend/in_memory/virtual_fs.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

class Virtual_file : public IFile {
 public:
  Virtual_file(const std::string &path, Virtual_fs *fs);

  Virtual_file(const std::string &name, Virtual_fs *fs,
               Virtual_fs::Directory *parent);

  Virtual_file(const Virtual_file &other) = delete;
  Virtual_file(Virtual_file &&other) = default;

  Virtual_file &operator=(const Virtual_file &other) = delete;
  Virtual_file &operator=(Virtual_file &&other) = default;

  ~Virtual_file() override = default;

  void open(Mode m) override;

  bool is_open() const override;

  int error() const override {
    // this throws exceptions
    return 0;
  }

  void close() override;

  size_t file_size() const override;

  Masked_string full_path() const override;

  std::string filename() const override;

  bool exists() const override;

  std::unique_ptr<IDirectory> parent() const override;

  off64_t seek(off64_t offset) override;

  off64_t tell() const override;

  ssize_t read(void *buffer, std::size_t length) override;

  ssize_t write(const void *buffer, std::size_t length) override;

  bool flush() override;

  bool is_local() const override { return true; }

  void rename(const std::string &name) override;

  void remove() override;

  Virtual_fs::IFile *file() const { return m_file; }

 private:
  Virtual_fs *m_fs;
  Virtual_fs::Directory *m_parent;
  std::string m_name;
  Virtual_fs::IFile *m_file = nullptr;
};

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_FILE_H_
