/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/oci/instance_principal_delegation_token_credentials_provider.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/oci/config_file.h"
#include "mysqlshdk/libs/oci/security_token.h"

namespace mysqlshdk {
namespace oci {

Instance_principal_delegation_token_credentials_provider::
    Instance_principal_delegation_token_credentials_provider(
        const std::string &config_file, const std::string &config_profile)
    : Instance_principal_credentials_provider(
          "instance_principal_with_delegation_token") {
  auto config = Config_file{config_file, config_profile};
  m_delegation_token_file = shcore::path::expand_user(
      config.config_option(Config_file::Entry::DELEGATION_TOKEN_FILE));

  if (!shcore::is_file(m_delegation_token_file)) {
    throw std::runtime_error(shcore::str_format(
        "The 'delegation_token_file' entry of the OCI profile '%s' is set to "
        "'%s', which does not exist",
        config_profile.c_str(), m_delegation_token_file.c_str()));
  }

  log_info("Using delegation_token_file: %s", m_delegation_token_file.c_str());
}

Instance_principal_delegation_token_credentials_provider::Credentials
Instance_principal_delegation_token_credentials_provider::fetch_credentials() {
  Security_token token;

  try {
    token = Security_token{
        shcore::str_strip(shcore::get_text_file(m_delegation_token_file))};
  } catch (const std::exception &e) {
    throw std::runtime_error(
        shcore::str_format("Failed to read the delegation token from '%s': %s",
                           m_delegation_token_file.c_str(), e.what()));
  }

  auto credentials =
      Instance_principal_credentials_provider::fetch_credentials();

  // use the lower of the two expiration values
  credentials.expiration = std::min(
      credentials.expiration.value_or(std::numeric_limits<std::time_t>::max()),
      token.expiration());

  // add the token to the headers
  credentials.extra_headers.emplace("opc-obo-token", token.to_string());

  return credentials;
}

}  // namespace oci
}  // namespace mysqlshdk
