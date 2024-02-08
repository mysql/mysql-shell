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

#ifndef MYSQLSHDK_LIBS_AWS_S3_BUCKET_CONFIG_H_
#define MYSQLSHDK_LIBS_AWS_S3_BUCKET_CONFIG_H_

#include <memory>
#include <optional>
#include <string>

#include "mysqlshdk/libs/storage/backend/object_storage_config.h"

#include "mysqlshdk/libs/aws/aws_config_file.h"
#include "mysqlshdk/libs/aws/aws_credentials.h"
#include "mysqlshdk/libs/aws/aws_credentials_provider.h"
#include "mysqlshdk/libs/aws/s3_bucket_options.h"

namespace mysqlshdk {
namespace aws {

class S3_bucket;

class S3_bucket_config
    : public storage::backend::object_storage::Bucket_config {
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

  std::unique_ptr<rest::IRetry_strategy> retry_strategy() const override;

  std::unique_ptr<storage::backend::object_storage::Container> container()
      const override;

  std::unique_ptr<S3_bucket> s3_bucket() const;

  const std::string &hash() const override;

  const std::string &host() const { return m_host; }

  const std::string &profile() const { return m_config_profile; }

  const std::string &credentials_file() const { return m_credentials_file; }

  const std::string &config_file() const { return m_config_file; }

  const std::string &region() const { return m_region; }

  Aws_credentials_provider *credentials_provider() const {
    return m_credentials_provider.get();
  }

 private:
  std::string describe_self() const override;

  void load_profile(std::optional<Aws_config_file::Profile> *target);

  void validate_profile() const;

  void use_path_style_access();

  void setup_profile_name();

  void setup_credentials_file();

  void setup_config_file();

  void setup_config_file(std::string *target);

  void setup_region_name();

  void setup_endpoint_uri();

  void setup_credentials_provider();

  std::string m_label = "AWS-S3-OS";

  bool m_explicit_profile = false;
  std::string m_credentials_file;
  std::string m_region;
  std::string m_endpoint;

  std::string m_host;
  bool m_path_style_access = false;

  std::optional<Aws_config_file::Profile> m_profile_from_credentials_file;
  std::optional<Aws_config_file::Profile> m_profile_from_config_file;

  std::unique_ptr<Aws_credentials_provider> m_credentials_provider;

  mutable std::string m_hash;
};

using S3_bucket_config_ptr = std::shared_ptr<const S3_bucket_config>;

}  // namespace aws
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AWS_S3_BUCKET_CONFIG_H_
