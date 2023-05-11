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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_FILE_ADAPTER_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_FILE_ADAPTER_H_

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "mysqlshdk/libs/storage/ifile.h"

#include "mysqlshdk/libs/storage/backend/in_memory/virtual_fs.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

/**
 * Allows to use Virtual_fs::IFile through the IFile interface.
 */
class Virtual_file_adapter final : public IFile {
 public:
  explicit Virtual_file_adapter(std::unique_ptr<Virtual_fs::IFile> file);

  Virtual_file_adapter(const Virtual_file_adapter &) = delete;
  Virtual_file_adapter(Virtual_file_adapter &&) = default;

  Virtual_file_adapter &operator=(const Virtual_file_adapter &) = delete;
  Virtual_file_adapter &operator=(Virtual_file_adapter &&) = default;

  ~Virtual_file_adapter() override = default;

  void open(Mode m) override;

  bool is_open() const override;

  int error() const override { return 0; }

  void close() override;

  size_t file_size() const override;

  Masked_string full_path() const override;

  std::string filename() const override;

  bool exists() const override { return true; }

  std::unique_ptr<IDirectory> parent() const override {
    throw std::logic_error("Virtual_file_adapter::parent() - not supported");
  }

  off64_t seek(off64_t offset) override;

  off64_t tell() const override;

  ssize_t read(void *buffer, size_t length) override;

  ssize_t write(const void *, size_t) override;

  bool flush() override {
    throw std::logic_error("Virtual_file_adapter::flush() - not supported");
  }

  bool is_compressed() const override { return false; }

  bool is_local() const override { return true; }

  void rename(const std::string &) override {
    throw std::logic_error("Virtual_file_adapter::rename() - not supported");
  }

  void remove() override {
    throw std::logic_error("Virtual_file_adapter::remove() - not supported");
  }

 private:
  std::unique_ptr<Virtual_fs::IFile> m_file;
};

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_FILE_ADAPTER_H_
