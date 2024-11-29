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

#ifndef MYSQLSHDK_LIBS_STORAGE_IDIRECTORY_H_
#define MYSQLSHDK_LIBS_STORAGE_IDIRECTORY_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/utils/masked_value.h"

#include "mysqlshdk/libs/storage/config.h"
#include "mysqlshdk/libs/storage/entry.h"
#include "mysqlshdk/libs/storage/ifile.h"

namespace mysqlshdk {
namespace storage {

class IDirectory {
 public:
  IDirectory() = default;
  IDirectory(const IDirectory &other) = delete;
  IDirectory(IDirectory &&other) = default;

  IDirectory &operator=(const IDirectory &other) = delete;
  IDirectory &operator=(IDirectory &&other) = default;

  virtual ~IDirectory() = default;

  virtual bool exists() const = 0;

  virtual void create() = 0;

  /**
   * Deletes the directory (if it's empty).
   */
  virtual void remove() = 0;

  virtual bool is_empty() const = 0;

  virtual Masked_string full_path() const = 0;

  /**
   * Lists all entries in this directory.
   *
   * @returns All entries in this directory.
   */
  virtual Directory_listing list(bool hidden_files = false) const = 0;

  /**
   * Provides handle to the file with the specified name in this directory.
   *
   * @param name Name of the file.
   * @param options File backend specific options.
   *
   * @returns Handle to the file.
   */
  virtual std::unique_ptr<IFile> file(const std::string &name,
                                      const File_options &options = {}) const;

  /**
   * Provides handle to the subdirectory with the specified name in this
   * directory.
   *
   * @param name Name of the subdirectory.
   *
   * @returns Handle to the subdirectory.
   */
  virtual std::unique_ptr<IDirectory> directory(const std::string &name) const;

  virtual bool is_local() const = 0;

  virtual std::string join_path(const std::string &a,
                                const std::string &b) const = 0;
};

std::unique_ptr<IDirectory> make_directory(const std::string &path);

std::unique_ptr<IDirectory> make_directory(const std::string &path,
                                           const Config_ptr &config);

}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_IDIRECTORY_H_
