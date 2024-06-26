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

#ifndef MYSQLSHDK_LIBS_OCI_OCI_BUCKET_CONFIG_H_
#define MYSQLSHDK_LIBS_OCI_OCI_BUCKET_CONFIG_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/storage/backend/object_storage_config.h"

#include "mysqlshdk/libs/oci/oci_bucket_options.h"
#include "mysqlshdk/libs/oci/oci_credentials_provider.h"
#include "mysqlshdk/libs/oci/oci_signer.h"

namespace mysqlshdk {
namespace oci {

class Oci_bucket;

class Oci_bucket_config
    : public storage::backend::object_storage::Bucket_config,
      public Oci_signer_config,
      public storage::backend::object_storage::mixin::Config_file,
      public storage::backend::object_storage::mixin::Config_profile,
      public storage::backend::object_storage::mixin::Host {
 public:
  Oci_bucket_config() = delete;

  explicit Oci_bucket_config(const Oci_bucket_options &options);

  Oci_bucket_config(const Oci_bucket_config &) = delete;
  Oci_bucket_config(Oci_bucket_config &&) = default;

  Oci_bucket_config &operator=(const Oci_bucket_config &) = delete;
  Oci_bucket_config &operator=(Oci_bucket_config &&) = default;

  ~Oci_bucket_config() override = default;

  const std::string &oci_namespace() const { return m_namespace; }

  std::unique_ptr<rest::ISigner> signer() const override;

  std::unique_ptr<storage::backend::object_storage::Container> container()
      const override;

  std::unique_ptr<Oci_bucket> oci_bucket() const;

  const std::string &hash() const override;

  const std::string &host() const override { return m_host; }

  Oci_credentials_provider *credentials_provider() const override {
    return m_credentials_provider.get();
  }

 protected:
  std::string describe_self() const override;

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(Oci_os_tests, file_write_multipart_upload);
  FRIEND_TEST(Oci_os_tests, file_append_resume_interrupted_upload);
  FRIEND_TEST(Oci_os_tests, file_write_multipart_errors);
  FRIEND_TEST(Oci_os_tests, file_auto_cancel_multipart_upload);
#endif

  void resolve_credentials(Oci_bucket_options::Auth auth);

  void fetch_namespace();

  void configure_endpoint();

  std::string m_namespace;
  mutable std::string m_hash;

  std::unique_ptr<Oci_credentials_provider> m_credentials_provider;
};

using Oci_bucket_config_ptr = std::shared_ptr<const Oci_bucket_config>;

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_OCI_BUCKET_CONFIG_H_
