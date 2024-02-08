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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_CONFIG_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_CONFIG_H_

#include <memory>

#include "mysqlshdk/libs/storage/config.h"

#include "mysqlshdk/libs/storage/backend/in_memory/virtual_fs.h"

namespace mysqlshdk {
namespace storage {
namespace in_memory {

class Virtual_config : public Config {
 public:
  explicit Virtual_config(std::size_t page_size);

  Virtual_config(const Virtual_config &) = delete;
  Virtual_config(Virtual_config &&) = default;

  Virtual_config &operator=(const Virtual_config &) = delete;
  Virtual_config &operator=(Virtual_config &&) = default;

  ~Virtual_config() override = default;

  bool valid() const override { return true; }

  /**
   * Provides access to the underlying virtual file system.
   */
  Virtual_fs *fs() const { return m_fs.get(); }

 private:
  std::string describe_self() const override { return "in-memory FS"; }

  std::string describe_url(const std::string &) const override { return {}; }

  std::unique_ptr<IFile> file(const std::string &name) const override;

  std::unique_ptr<IDirectory> directory(const std::string &name) const override;

  std::unique_ptr<Virtual_fs> m_fs;
};

}  // namespace in_memory
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_IN_MEMORY_VIRTUAL_CONFIG_H_
