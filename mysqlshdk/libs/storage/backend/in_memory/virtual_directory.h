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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_DIRECTORY_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_DIRECTORY_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/storage/idirectory.h"

#include "mysqlshdk/libs/storage/backend/in_memory/virtual_fs.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

class Virtual_directory : public IDirectory {
 public:
  Virtual_directory(const std::string &name, Virtual_fs *fs);

  Virtual_directory(Virtual_fs::Directory *dir, Virtual_fs *fs);

  Virtual_directory(const Virtual_directory &other) = delete;
  Virtual_directory(Virtual_directory &&other) = default;

  Virtual_directory &operator=(const Virtual_directory &other) = delete;
  Virtual_directory &operator=(Virtual_directory &&other) = default;

  ~Virtual_directory() override = default;

  bool exists() const override;

  void create() override;

  Masked_string full_path() const override;

  std::unordered_set<File_info> list_files(bool) const override;

  std::unordered_set<File_info> filter_files(
      const std::string &pattern) const override;

  std::unique_ptr<IFile> file(const std::string &name,
                              const File_options &) const override;

  bool is_local() const override { return true; }

  std::string join_path(const std::string &a,
                        const std::string &b) const override;

 private:
  Virtual_fs *m_fs;
  mutable Virtual_fs::Directory *m_dir;
  std::string m_name;
};

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_DIRECTORY_H_
