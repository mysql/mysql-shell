/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_CONFIG_H_
#define MYSQLSHDK_LIBS_STORAGE_CONFIG_H_

#include <cassert>
#include <memory>
#include <string>

namespace mysqlshdk {
namespace storage {

class IFile;
class IDirectory;

class Config;
using Config_ptr = std::shared_ptr<const Config>;

class Config : public std::enable_shared_from_this<Config> {
 public:
  Config(const Config &) = delete;
  Config(Config &&) = default;

  Config &operator=(const Config &) = delete;
  Config &operator=(Config &&) = default;

  virtual ~Config() = default;

  virtual bool valid() const = 0;

  std::string description() const;

  std::string describe(const std::string &url) const;

 protected:
  Config() = default;

  template <typename T>
  std::shared_ptr<const T> shared_ptr() const {
    return std::static_pointer_cast<const T>(shared_from_this());
  }

 private:
  friend std::unique_ptr<IFile> make_file(const std::string &,
                                          const Config_ptr &);
  friend std::unique_ptr<IDirectory> make_directory(const std::string &,
                                                    const Config_ptr &);

  std::unique_ptr<IFile> make_file(const std::string &path) const;

  std::unique_ptr<IDirectory> make_directory(const std::string &path) const;

  virtual std::string describe_self() const = 0;

  virtual std::string describe_url(const std::string &url) const = 0;

  virtual std::unique_ptr<IFile> file(const std::string &path) const = 0;

  virtual std::unique_ptr<IDirectory> directory(
      const std::string &path) const = 0;
};

}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_CONFIG_H_
