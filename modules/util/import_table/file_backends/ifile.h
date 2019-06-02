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

#ifndef MODULES_UTIL_IMPORT_TABLE_FILE_BACKENDS_IFILE_H_
#define MODULES_UTIL_IMPORT_TABLE_FILE_BACKENDS_IFILE_H_

#include <memory>
#include <string>

#if defined(_WIN32)
using off64_t = __int64;
using ssize_t = __int64;
#elif defined(__APPLE__)
using off64_t = off_t;
#endif

namespace mysqlsh {
namespace import_table {

class IFile {
 public:
  IFile() = default;
  IFile(const IFile &other) = delete;
  IFile(IFile &&other) = default;

  IFile &operator=(const IFile &other) = delete;
  IFile &operator=(IFile &&other) = default;

  virtual ~IFile() = default;

  virtual void open() = 0;
  virtual bool is_open() = 0;
  virtual void close() = 0;

  virtual size_t file_size() = 0;
  virtual std::string file_name() = 0;
  virtual off64_t seek(off64_t offset) = 0;
  virtual ssize_t read(void *buffer, size_t length) = 0;
};

std::unique_ptr<IFile> make_file_handler(const std::string &filepath);

}  // namespace import_table
}  // namespace mysqlsh

#endif  // MODULES_UTIL_IMPORT_TABLE_FILE_BACKENDS_IFILE_H_
