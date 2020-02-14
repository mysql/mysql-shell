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

#ifndef MYSQLSHDK_LIBS_STORAGE_COMPRESSION_GZ_FILE_H_
#define MYSQLSHDK_LIBS_STORAGE_COMPRESSION_GZ_FILE_H_

#include <memory>
#include <stdexcept>

#include "mysqlshdk/libs/storage/compressed_file.h"

namespace mysqlshdk {
namespace storage {
namespace compression {

class Gz_file : public Compressed_file {
 public:
  Gz_file() = delete;

  explicit Gz_file(std::unique_ptr<IFile> file);

  Gz_file(const Gz_file &other) = delete;
  Gz_file(Gz_file &&other) = default;

  Gz_file &operator=(const Gz_file &other) = delete;
  Gz_file &operator=(Gz_file &&other) = default;

  ~Gz_file() override = default;

  off64_t seek(off64_t) override {
    throw std::logic_error("Gz_file::seek() - not implemented");
  }

  ssize_t read(void *, size_t) override {
    throw std::logic_error("Gz_file::read() - not implemented");
  }

  ssize_t write(const void *, size_t) override {
    throw std::logic_error("Gz_file::write() - not implemented");
  }
};

}  // namespace compression
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_COMPRESSION_GZ_FILE_H_
