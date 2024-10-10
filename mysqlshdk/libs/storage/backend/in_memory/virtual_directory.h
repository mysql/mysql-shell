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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_DIRECTORY_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_DIRECTORY_H_

#include <memory>
#include <stdexcept>
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

  void remove() override {
    throw std::logic_error{"Virtual_directory::remove()"};
  }

  bool is_empty() const override;

  Masked_string full_path() const override;

  Directory_listing list(bool) const override;

  std::unique_ptr<IFile> file(const std::string &name,
                              const File_options &) const override;

  std::unique_ptr<IDirectory> directory(const std::string &) const override {
    throw std::logic_error{"Virtual_directory::directory()"};
  }

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
