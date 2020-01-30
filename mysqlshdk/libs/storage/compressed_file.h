/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_COMPRESSED_FILE_H_
#define MYSQLSHDK_LIBS_STORAGE_COMPRESSED_FILE_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/storage/ifile.h"

namespace mysqlshdk {
namespace storage {

enum class Compression { NONE, GZIP };

class Compressed_file : public IFile {
 public:
  Compressed_file() = delete;
  explicit Compressed_file(std::unique_ptr<IFile> file);
  Compressed_file(const Compressed_file &other) = delete;
  Compressed_file(Compressed_file &&other) = default;

  Compressed_file &operator=(const Compressed_file &other) = delete;
  Compressed_file &operator=(Compressed_file &&other) = default;

  ~Compressed_file() override = default;

  void open(Mode m) override;
  bool is_open() const override;
  int error() const override;
  void close() override;

  size_t file_size() const override;
  std::string full_path() const override;
  std::string filename() const override;
  bool exists() const override;

  bool flush() override;

  void rename(const std::string &new_name) override;

  IFile *file() const { return m_file.get(); }

 private:
  std::unique_ptr<IFile> m_file;
};

Compression to_compression(const std::string &c);

std::string to_string(Compression c);

std::string get_extension(Compression c);

std::unique_ptr<IFile> make_file(std::unique_ptr<IFile> file, Compression c);

}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_COMPRESSED_FILE_H_
