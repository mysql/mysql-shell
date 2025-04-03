/*
 * Copyright (c) 2022, 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_OCI_OCI_BUCKET_OPTIONS_H_
#define MYSQLSHDK_LIBS_OCI_OCI_BUCKET_OPTIONS_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/storage/backend/object_storage_options.h"

namespace mysqlshdk {
namespace oci {

class Oci_bucket_config;

enum class Authentication {
  API_KEY,
  SECURITY_TOKEN,
  INSTANCE_PRINCIPAL,
  RESOURCE_PRINCIPAL,
  INSTANCE_OBO_USER,
};

class Oci_bucket_options
    : public storage::backend::object_storage::Object_storage_options,
      public storage::backend::object_storage::mixin::Config_file,
      public storage::backend::object_storage::mixin::Config_profile {
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

  static constexpr const char *auth_option() { return "ociAuth"; }

  static const shcore::Option_pack_def<Oci_bucket_options> &options();

  std::shared_ptr<Oci_bucket_config> oci_config() const;

  inline const std::optional<Authentication> &authentication() const noexcept {
    return m_auth;
  }

 protected:
  std::shared_ptr<storage::backend::object_storage::Config> create_config()
      const override;

  std::string m_namespace;

  const char *get_main_option() const override { return bucket_name_option(); }

  void on_unpacked_options() const override;

 private:
  friend class Oci_bucket_config;

  std::vector<const char *> get_secondary_options() const override;

  bool has_value(const char *option) const override;

  void set_auth(const std::string &auth);

  std::optional<Authentication> m_auth;
  std::string m_auth_str;
};

Authentication to_authentication(std::string_view auth);
const char *to_string(Authentication auth);

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_OCI_BUCKET_OPTIONS_H_
