/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_UTIL_IMPORT_TABLE_FILE_BACKENDS_FILE_H_
#define MODULES_UTIL_IMPORT_TABLE_FILE_BACKENDS_FILE_H_

#include <memory>
#include <string>

#include "modules/util/import_table/file_backends/ifile.h"

namespace mysqlsh {
namespace import_table {

class File : public IFile {
 public:
  File() = delete;
  explicit File(const std::string &filename);
  File(const File &other) = delete;
  File(File &&other) = default;

  File &operator=(const File &other) = delete;
  File &operator=(File &&other) = default;

  ~File() override = default;

  void open() override;
  bool is_open() override;
  void close() override;

  size_t file_size() override;
  std::string file_name() override;
  off64_t seek(off64_t offset) override;
  ssize_t read(void *buffer, size_t length) override;

 private:
  int m_fd = -1;
  std::string m_filepath;
};

}  // namespace import_table
}  // namespace mysqlsh

#endif  // MODULES_UTIL_IMPORT_TABLE_FILE_BACKENDS_FILE_H_
