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

#ifndef MYSQLSHDK_LIBS_OCI_OCI_BUCKET_CONFIG_H_
#define MYSQLSHDK_LIBS_OCI_OCI_BUCKET_CONFIG_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/storage/backend/object_storage_config.h"

#include "mysqlshdk/libs/oci/oci_bucket_options.h"

namespace mysqlshdk {
namespace oci {

class Oci_bucket;
class Oci_setup;

class Oci_bucket_config : public storage::backend::object_storage::Config {
 public:
  Oci_bucket_config() = delete;

  explicit Oci_bucket_config(const Oci_bucket_options &options);

  Oci_bucket_config(const Oci_bucket_config &) = delete;
  Oci_bucket_config(Oci_bucket_config &&) = default;

  Oci_bucket_config &operator=(const Oci_bucket_config &) = delete;
  Oci_bucket_config &operator=(Oci_bucket_config &&) = default;

  ~Oci_bucket_config() override = default;

  const std::string &oci_namespace() const { return m_namespace; }

  const std::string &service_endpoint() const override { return m_endpoint; }

  const std::string &service_label() const override { return m_label; }

  std::unique_ptr<rest::Signer> signer() const override;

  std::unique_ptr<storage::backend::object_storage::Bucket> bucket()
      const override;

  std::unique_ptr<Oci_bucket> oci_bucket() const;

  const std::string &hash() const override;

  void set_part_size(std::size_t size) { m_part_size = size; }

 protected:
  std::string describe_self() const override;

 private:
  friend class Oci_signer;

  void load_key(Oci_setup *setup);

  void fetch_namespace();

  std::string m_label = "OCI-OS";
  std::string m_namespace;
  std::string m_host;
  std::string m_endpoint;
  std::string m_tenancy_id;
  std::string m_user;
  std::string m_fingerprint;
  std::string m_key_file;
  mutable std::string m_hash;
};

using Oci_bucket_config_ptr = std::shared_ptr<const Oci_bucket_config>;

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_OCI_BUCKET_CONFIG_H_
