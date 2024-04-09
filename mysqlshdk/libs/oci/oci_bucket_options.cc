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

#include "mysqlshdk/libs/oci/oci_bucket_options.h"

#include <memory>
#include <stdexcept>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/oci/oci_bucket_config.h"

namespace mysqlshdk {
namespace oci {

const shcore::Option_pack_def<Oci_bucket_options>
    &Oci_bucket_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Oci_bucket_options>()
          .optional(bucket_name_option(), &Oci_bucket_options::m_container_name)
          .optional(namespace_option(), &Oci_bucket_options::m_namespace)
          .optional(config_file_option(), &Oci_bucket_options::m_config_file)
          .optional(profile_option(), &Oci_bucket_options::m_config_profile)
          .optional(auth_option(), &Oci_bucket_options::set_auth)
          .on_done(&Oci_bucket_options::on_unpacked_options);

  return opts;
}

std::shared_ptr<Oci_bucket_config> Oci_bucket_options::oci_config() const {
  return std::make_shared<Oci_bucket_config>(*this);
}

std::shared_ptr<storage::backend::object_storage::Config>
Oci_bucket_options::create_config() const {
  return oci_config();
}

std::vector<const char *> Oci_bucket_options::get_secondary_options() const {
  return {config_file_option(), profile_option(), namespace_option(),
          auth_option()};
}

bool Oci_bucket_options::has_value(const char *option) const {
  if (shcore::str_caseeq(option, bucket_name_option())) {
    return !m_container_name.empty();
  } else if (shcore::str_caseeq(option, config_file_option())) {
    return !m_config_file.empty();
  } else if (shcore::str_caseeq(option, profile_option())) {
    return !m_config_profile.empty();
  } else if (shcore::str_caseeq(option, namespace_option())) {
    return !m_namespace.empty();
  } else if (shcore::str_caseeq(option, auth_option())) {
    return !m_auth_str.empty();
  }
  return false;
}

void Oci_bucket_options::set_auth(const std::string &auth) {
  if ("" == auth) {
    // use the default value
  } else if ("api_key" == auth) {
    m_auth = Auth::API_KEY;
  } else if ("security_token" == auth) {
    m_auth = Auth::SECURITY_TOKEN;
  } else if ("instance_principal" == auth) {
    m_auth = Auth::INSTANCE_PRINCIPAL;
  } else if ("resource_principal" == auth) {
    m_auth = Auth::RESOURCE_PRINCIPAL;
  } else {
    throw std::invalid_argument(shcore::str_format(
        "Invalid value of '%s' option, expected one of: api_key, "
        "instance_principal, resource_principal, security_token, but got: %s.",
        auth_option(), auth.c_str()));
  }

  m_auth_str = auth;
}

void Oci_bucket_options::on_unpacked_options() const {
  Object_storage_options::on_unpacked_options();

  if (Auth::INSTANCE_PRINCIPAL == auth() ||
      Auth::RESOURCE_PRINCIPAL == auth()) {
    for (const auto disallowed : {config_file_option(), profile_option()}) {
      if (has_value(disallowed)) {
        throw std::invalid_argument(
            shcore::str_format("The option '%s' cannot be used when the '%s' "
                               "option is set to: %s.",
                               disallowed, auth_option(), m_auth_str.c_str()));
      }
    }
  }
}

}  // namespace oci
}  // namespace mysqlshdk
