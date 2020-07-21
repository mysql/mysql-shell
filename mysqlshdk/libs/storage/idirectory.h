/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_IDIRECTORY_H_
#define MYSQLSHDK_LIBS_STORAGE_IDIRECTORY_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "mysqlshdk/libs/storage/ifile.h"

namespace mysqlshdk {
namespace oci {
struct Oci_options;
}
namespace storage {

class IDirectory {
 public:
  struct File_info {
    std::string name;
    size_t size = 0;
  };

  IDirectory() = default;
  IDirectory(const IDirectory &other) = delete;
  IDirectory(IDirectory &&other) = default;

  IDirectory &operator=(const IDirectory &other) = delete;
  IDirectory &operator=(IDirectory &&other) = default;

  virtual ~IDirectory() = default;

  virtual bool exists() const = 0;

  virtual void create() = 0;

  virtual void close() = 0;

  virtual std::string full_path() const = 0;

  /**
   * Lists all files in this directory.
   *
   * @returns All files in this directory.
   */
  virtual std::vector<File_info> list_files(
      bool hidden_files = false) const = 0;

  /**
   * Provides handle to the file with the specified name in this directory.
   *
   * @param name Name of the file.
   *
   * @returns Handle to the file.
   */
  virtual std::unique_ptr<IFile> file(const std::string &name) const;

  virtual bool is_local() const = 0;

  virtual std::string join_path(const std::string &a,
                                const std::string &b) const = 0;
};

std::unique_ptr<IDirectory> make_directory(
    const std::string &path,
    const std::unordered_map<std::string, std::string> &options = {});

std::unique_ptr<IDirectory> make_directory(
    const std::string &path, const mysqlshdk::oci::Oci_options &options);

}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_IDIRECTORY_H_
