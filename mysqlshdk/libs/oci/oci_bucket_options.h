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

#ifndef MYSQLSHDK_LIBS_OCI_OCI_BUCKET_OPTIONS_H_
#define MYSQLSHDK_LIBS_OCI_OCI_BUCKET_OPTIONS_H_

#include <memory>
#include <string>

#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/storage/backend/object_storage_bucket_options.h"

namespace mysqlshdk {
namespace oci {

class Oci_bucket_config;

class Oci_bucket_options
    : public storage::backend::object_storage::Bucket_options<
          Oci_bucket_options> {
 public:
  Oci_bucket_options() = default;

  Oci_bucket_options(const Oci_bucket_options &) = default;
  Oci_bucket_options(Oci_bucket_options &&) = default;

  Oci_bucket_options &operator=(const Oci_bucket_options &) = default;
  Oci_bucket_options &operator=(Oci_bucket_options &&) = default;

  ~Oci_bucket_options() override = default;

  static constexpr const char *bucket_name_option() { return "osBucketName"; }

  static constexpr const char *namespace_option() { return "osNamespace"; }

  static constexpr const char *config_file_option() { return "ociConfigFile"; }

  static constexpr const char *profile_option() { return "ociProfile"; }

  static const shcore::Option_pack_def<Oci_bucket_options> &options();

  std::shared_ptr<Oci_bucket_config> oci_config() const;

 protected:
  void on_unpacked_options() const override;

  std::shared_ptr<storage::backend::object_storage::Config> create_config()
      const override;

  std::string m_namespace;

 private:
  friend class Oci_bucket_config;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_OCI_BUCKET_OPTIONS_H_
