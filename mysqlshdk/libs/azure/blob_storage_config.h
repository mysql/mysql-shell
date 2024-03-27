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

#ifndef MYSQLSHDK_LIBS_AZURE_BLOB_STORAGE_CONFIG_H_
#define MYSQLSHDK_LIBS_AZURE_BLOB_STORAGE_CONFIG_H_

#include <memory>
#include <optional>
#include <string>

#include "mysqlshdk/libs/azure/blob_storage_options.h"
#include "mysqlshdk/libs/storage/backend/object_storage_config.h"

namespace mysqlshdk {
namespace azure {

class Blob_container;

class Blob_storage_config
    : public storage::backend::object_storage::Config,
      public storage::backend::object_storage::mixin::Config_file {
 public:
  Blob_storage_config() = delete;

  explicit Blob_storage_config(const Blob_storage_options &options,
                               bool enable_env_vars = true);

  Blob_storage_config(const Blob_storage_config &) = delete;
  Blob_storage_config(Blob_storage_config &&) = default;

  Blob_storage_config &operator=(const Blob_storage_config &) = delete;
  Blob_storage_config &operator=(Blob_storage_config &&) = default;

  ~Blob_storage_config() override = default;

  const std::string &service_endpoint_path() const { return m_endpoint_path; }

  std::unique_ptr<rest::ISigner> signer() const override;

  std::unique_ptr<storage::backend::object_storage::Container> container()
      const override;

  std::unique_ptr<Blob_container> blob_container() const;

  bool valid() const override;
  const std::string &hash() const override;

  std::string describe_url(const std::string &url) const override;

  static constexpr std::size_t DEFAULT_BLOB_BLOCK_SIZE = 128 * 1024 * 1024;

  // Signature Caching in Azure is based not only in PATH and METHOD but also
  // in the HEADERS, so it is less likely to be a time saver considering
  // encoding of the headers is needed anyway to get the cache key
  bool signature_caching_enabled() const override { return false; }

  const std::string &account_name() const { return m_account_name; }
  const std::string &account_key() const { return m_account_key; }
  const std::string &sas_token() const { return m_sas_token; }
  const std::string &endpoint() const { return m_endpoint; }
  const std::string &endpoint_path() const { return m_endpoint_path; }

 private:
  friend class Signer;
  friend class Blob_container;

  std::string describe_self() const override;

  void load_connection_string(const std::string &connection_string);
  void load_env_vars();
  void load_config(const std::string &path);
  void validate_config() const;
  std::string get_default_config_path() const;
  void throw_sas_token_error(const std::string &error) const;

  std::string m_endpoint_protocol;
  std::string m_endpoint_suffix;
  std::string m_endpoint_path;
  std::string m_account_name;
  std::string m_account_key;
  std::string m_sas_token;
  std::vector<std::pair<std::string, std::optional<std::string>>>
      m_sas_token_data;
  mutable std::string m_hash;

  std::string m_sas_token_source;
  Blob_storage_options::Operation m_operation;
};

using Storage_config_ptr = std::shared_ptr<const Blob_storage_config>;

}  // namespace azure
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AZURE_BLOB_STORAGE_CONFIG_H_
