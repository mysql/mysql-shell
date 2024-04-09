/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/oci/api_key_credentials_provider.h"

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace oci {

Api_key_credentials_provider::Api_key_credentials_provider(
    const std::string &config_file, const std::string &config_profile)
    : Config_credentials_provider(config_file, config_profile, "api_key") {
  m_credentials.auth_key_id = shcore::str_format(
      "%s/%s/%s", tenancy_id().c_str(), config_option("user").c_str(),
      config_option("fingerprint").c_str());
}

Api_key_credentials_provider::Credentials
Api_key_credentials_provider::fetch_credentials() {
  return m_credentials;
}

}  // namespace oci
}  // namespace mysqlshdk
