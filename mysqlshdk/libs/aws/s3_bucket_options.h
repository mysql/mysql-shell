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

#ifndef MYSQLSHDK_LIBS_AWS_S3_BUCKET_OPTIONS_H_
#define MYSQLSHDK_LIBS_AWS_S3_BUCKET_OPTIONS_H_

#include <memory>
#include <string>

#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/storage/backend/object_storage_bucket_options.h"

namespace mysqlshdk {
namespace aws {

class S3_bucket_config;

class S3_bucket_options
    : public storage::backend::object_storage::Bucket_options<
          S3_bucket_options> {
 public:
  S3_bucket_options() = default;

  S3_bucket_options(const S3_bucket_options &) = default;
  S3_bucket_options(S3_bucket_options &&) = default;

  S3_bucket_options &operator=(const S3_bucket_options &) = default;
  S3_bucket_options &operator=(S3_bucket_options &&) = default;

  ~S3_bucket_options() override = default;

  static constexpr const char *bucket_name_option() { return "s3BucketName"; }

  static constexpr const char *credentials_file_option() {
    return "s3CredentialsFile";
  }

  static constexpr const char *config_file_option() { return "s3ConfigFile"; }

  static constexpr const char *profile_option() { return "s3Profile"; }

  static constexpr const char *endpoint_override_option() {
    return "s3EndpointOverride";
  }

  static const shcore::Option_pack_def<S3_bucket_options> &options();

  std::shared_ptr<S3_bucket_config> s3_config() const;

 private:
  friend class S3_bucket_config;

  void on_unpacked_options() const override;

  std::shared_ptr<storage::backend::object_storage::Config> create_config()
      const override;

  std::string m_credentials_file;
  std::string m_endpoint_override;
};

}  // namespace aws
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AWS_S3_BUCKET_OPTIONS_H_
