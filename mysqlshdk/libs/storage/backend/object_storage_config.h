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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_CONFIG_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_CONFIG_H_

#include <cassert>
#include <memory>
#include <string>

#include "mysqlshdk/libs/rest/signed_rest_service.h"
#include "mysqlshdk/libs/storage/config.h"

#include "mysqlshdk/libs/storage/backend/object_storage_bucket_options.h"

namespace mysqlshdk {
namespace storage {
namespace backend {
namespace object_storage {

class Bucket;

template <class T>
class Bucket_options;

class Config : public storage::Config, public rest::Signed_rest_service_config {
 public:
  Config() = delete;

  Config(const Config &) = delete;
  Config(Config &&) = default;

  Config &operator=(const Config &) = delete;
  Config &operator=(Config &&) = default;

  ~Config() override = default;

  bool valid() const override { return true; }

  const std::string &bucket_name() const { return m_bucket_name; }

  std::size_t part_size() const { return m_part_size; }

  virtual std::unique_ptr<Bucket> bucket() const = 0;

  virtual const std::string &hash() const = 0;

  // 128 MB
  static constexpr std::size_t DEFAULT_MULTIPART_PART_SIZE = 128 * 1024 * 1024;

 protected:
  template <class T>
  explicit Config(const Bucket_options<T> &options)
      : m_bucket_name(options.m_bucket_name),
        m_config_file(options.m_config_file),
        m_config_profile(options.m_config_profile),
        m_bucket_option(T::bucket_name_option()) {
    assert(!m_bucket_name.empty());
  }

  std::string m_bucket_name;
  std::string m_config_file;
  std::string m_config_profile;
  std::size_t m_part_size = DEFAULT_MULTIPART_PART_SIZE;

 private:
  std::string describe_url(const std::string &url) const override;

  std::unique_ptr<storage::IFile> file(const std::string &path) const override;

  std::unique_ptr<storage::IDirectory> directory(
      const std::string &path) const override;

  void fail_if_uri(const std::string &path) const;

  std::string m_bucket_option;
};

using Config_ptr = std::shared_ptr<const Config>;

}  // namespace object_storage
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_CONFIG_H_
