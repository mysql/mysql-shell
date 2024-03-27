/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_CONFIG_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_CONFIG_H_

#include <cassert>
#include <memory>
#include <string>

#include "mysqlshdk/libs/rest/signed/signed_rest_service_config.h"
#include "mysqlshdk/libs/storage/config.h"

namespace mysqlshdk {
namespace storage {
namespace backend {
namespace object_storage {

class Container;
class Object_storage_options;

class Config : public storage::Config, public rest::Signed_rest_service_config {
 public:
  Config() = delete;

  Config(const Config &) = delete;
  Config(Config &&) = default;

  Config &operator=(const Config &) = delete;
  Config &operator=(Config &&) = default;

  ~Config() override = default;

  bool valid() const override { return true; }

  const std::string &container_name() const { return m_container_name; }

  std::size_t part_size() const { return m_part_size; }
  void set_part_size(std::size_t size) { m_part_size = size; }

  virtual const std::string &hash() const = 0;

  virtual std::unique_ptr<Container> container() const = 0;

  const std::string &service_endpoint() const override { return m_endpoint; }

  const std::string &service_label() const override { return m_label; }

 protected:
  explicit Config(const Object_storage_options &options, std::size_t part_size);

  std::string m_container_name;
  std::size_t m_part_size;

  std::string m_label;
  std::string m_endpoint;

 private:
  std::string describe_url(const std::string &url) const override;

  std::unique_ptr<storage::IFile> file(const std::string &path) const override;

  std::unique_ptr<storage::IDirectory> directory(
      const std::string &path) const override;

  void fail_if_uri(const std::string &path) const;

  std::string m_container_name_option;
};

using Config_ptr = std::shared_ptr<const Config>;

class Bucket_config : public Config {
 public:
  // 128 MB
  static constexpr std::size_t DEFAULT_MULTIPART_PART_SIZE = 128 * 1024 * 1024;

  const std::string &bucket_name() const { return container_name(); }

 protected:
  Bucket_config() = delete;

  Bucket_config(const Bucket_config &) = delete;
  Bucket_config(Bucket_config &&) = default;

  Bucket_config &operator=(const Bucket_config &) = delete;
  Bucket_config &operator=(Bucket_config &&) = default;

  ~Bucket_config() override = default;

 protected:
  explicit Bucket_config(const Object_storage_options &options);
};

namespace mixin {
// Various mixins, note that destuctors are not virtual.

class Config_file {
 public:
  Config_file(const Config_file &) = default;
  Config_file(Config_file &&) = default;

  Config_file &operator=(const Config_file &) = default;
  Config_file &operator=(Config_file &&) = default;

  ~Config_file() = default;

  inline const std::string &config_file() const noexcept {
    return m_config_file;
  }

 protected:
  Config_file() = default;

  std::string m_config_file;
};

class Credentials_file {
 public:
  Credentials_file(const Credentials_file &) = default;
  Credentials_file(Credentials_file &&) = default;

  Credentials_file &operator=(const Credentials_file &) = default;
  Credentials_file &operator=(Credentials_file &&) = default;

  ~Credentials_file() = default;

  inline const std::string &credentials_file() const noexcept {
    return m_credentials_file;
  }

 protected:
  Credentials_file() = default;

  std::string m_credentials_file;
};

class Config_profile {
 public:
  Config_profile(const Config_profile &) = default;
  Config_profile(Config_profile &&) = default;

  Config_profile &operator=(const Config_profile &) = default;
  Config_profile &operator=(Config_profile &&) = default;

  ~Config_profile() = default;

  inline const std::string &config_profile() const noexcept {
    return m_config_profile;
  }

 protected:
  Config_profile() = default;

  std::string m_config_profile;
};

class Host {
 public:
  Host(const Host &) = default;
  Host(Host &&) = default;

  Host &operator=(const Host &) = default;
  Host &operator=(Host &&) = default;

  ~Host() = default;

  inline const std::string &host() const noexcept { return m_host; }

 protected:
  Host() = default;

  std::string m_host;
};

}  // namespace mixin

}  // namespace object_storage
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_CONFIG_H_
