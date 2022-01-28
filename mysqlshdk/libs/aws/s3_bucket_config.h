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

#ifndef MYSQLSHDK_LIBS_AWS_S3_BUCKET_CONFIG_H_
#define MYSQLSHDK_LIBS_AWS_S3_BUCKET_CONFIG_H_

#include <memory>
#include <optional>
#include <string>

#include "mysqlshdk/libs/storage/backend/object_storage_config.h"

#include "mysqlshdk/libs/aws/s3_bucket_options.h"

namespace mysqlshdk {
namespace aws {

class S3_bucket;

class S3_bucket_config : public storage::backend::object_storage::Config {
 public:
  S3_bucket_config() = delete;

  explicit S3_bucket_config(const S3_bucket_options &options);

  S3_bucket_config(const S3_bucket_config &) = delete;
  S3_bucket_config(S3_bucket_config &&) = default;

  S3_bucket_config &operator=(const S3_bucket_config &) = delete;
  S3_bucket_config &operator=(S3_bucket_config &&) = default;

  ~S3_bucket_config() override = default;

  bool path_style_access() const { return m_path_style_access; }

  const std::string &service_endpoint() const override { return m_endpoint; }

  const std::string &service_label() const override { return m_label; }

  std::unique_ptr<rest::Signer> signer() const override;

  std::unique_ptr<storage::backend::object_storage::Bucket> bucket()
      const override;

  std::unique_ptr<S3_bucket> s3_bucket() const;

  const std::string &hash() const override;

  void set_part_size(std::size_t size) { m_part_size = size; }

 private:
  friend class Aws_signer;

  std::string describe_self() const override;

  void load_profile(const std::string &path, bool is_config_file);

  void validate_profile() const;

  void use_path_style_access();

  std::string m_label = "AWS-S3-OS";
  std::string m_host;
  std::string m_endpoint;
  bool m_path_style_access = false;
  std::string m_credentials_file;
  std::optional<std::string> m_access_key_id;
  std::optional<std::string> m_secret_access_key;
  std::optional<std::string> m_session_token;
  std::string m_region;
  mutable std::string m_hash;
};

using S3_bucket_config_ptr = std::shared_ptr<const S3_bucket_config>;

}  // namespace aws
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AWS_S3_BUCKET_CONFIG_H_
