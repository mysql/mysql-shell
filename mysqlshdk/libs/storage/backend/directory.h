/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_DIRECTORY_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_DIRECTORY_H_

#include <string>
#include <vector>

#include "mysqlshdk/libs/storage/idirectory.h"

namespace mysqlshdk {
namespace storage {
namespace backend {

class Directory : public IDirectory {
 public:
  Directory() = delete;

  explicit Directory(const std::string &dir);

  Directory(const Directory &other) = delete;
  Directory(Directory &&other) = default;

  Directory &operator=(const Directory &other) = delete;
  Directory &operator=(Directory &&other) = default;

  ~Directory() override = default;

  bool exists() const override;

  void create() override;

  Masked_string full_path() const override;

  std::unordered_set<IDirectory::File_info> list_files(
      bool hidden_files = false) const override;

  std::unordered_set<IDirectory::File_info> filter_files(
      const std::string &pattern) const override;

  bool is_local() const override { return true; }

 protected:
  std::string join_path(const std::string &a,
                        const std::string &b) const override;

 private:
  std::string m_path;
};

}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_DIRECTORY_H_
