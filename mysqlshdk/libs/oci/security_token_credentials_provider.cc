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

#include "mysqlshdk/libs/oci/security_token_credentials_provider.h"

#include <cassert>
#include <stdexcept>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace oci {

Security_token_credentials_provider::Security_token_credentials_provider(
    const std::string &config_file, const std::string &config_profile)
    : Config_credentials_provider(config_file, config_profile,
                                  "security_token") {
  m_security_token_file =
      shcore::path::expand_user(config_option("security_token_file"));

  if (!shcore::is_file(m_security_token_file)) {
    throw std::runtime_error(shcore::str_format(
        "The 'security_token_file' entry of the OCI profile '%s' is set to "
        "'%s', which does not exist",
        config_profile.c_str(), m_security_token_file.c_str()));
  }

  log_info("Using security_token_file: %s", m_security_token_file.c_str());
}

Security_token_credentials_provider::Credentials
Security_token_credentials_provider::fetch_credentials() {
  if (m_security_token.to_string().empty()) {
    // first call, read the token from file
    read_token_file();
  } else {
    // otherwise, refresh the token
    refresh_token();
  }

  m_credentials.auth_key_id = m_security_token.auth_key_id();
  m_credentials.expiration = m_security_token.expiration();

  if (Oci_credentials::Clock::from_time_t(*m_credentials.expiration) <=
      Oci_credentials::Clock::now()) {
    m_security_token_expired = true;

    throw std::runtime_error(
        shcore::str_format("The OCI session of the profile '%s' has expired "
                           "and cannot be refreshed",
                           config_profile().c_str()));
  }

  return m_credentials;
}

void Security_token_credentials_provider::read_token_file() {
  try {
    m_security_token = Security_token{
        shcore::str_strip(shcore::get_text_file(m_security_token_file))};
  } catch (const std::exception &e) {
    throw std::runtime_error(
        shcore::str_format("Failed to read the OCI session of the profile '%s' "
                           "from the security token file: %s",
                           config_profile().c_str(), e.what()));
  }
}

void Security_token_credentials_provider::refresh_token() {
  if (m_security_token_expired) {
    // refresh won't work
    return;
  }

  // token is being refreshed, so it's close to the expiration period, we cannot
  // use this instance to fetch the credentials, as it would trigger the refresh
  // again
  if (!m_data_plane_credentials_provider) {
    m_data_plane_credentials_provider =
        std::make_unique<Fixed_credentials_provider>(*this);
  } else {
    const auto credentials = current_credentials();
    assert(credentials);

    m_data_plane_credentials_provider->set_auth_key_id(
        credentials->auth_key_id());
    m_data_plane_credentials_provider->set_private_key_id(
        credentials->private_key_id());
  }

  m_data_plane_credentials_provider->initialize();

  if (!m_data_plane_client) {
    m_data_plane_client = std::make_unique<Data_plane_client>(
        m_data_plane_credentials_provider.get());
  }

  const auto error = [this](const char *msg) {
    return std::runtime_error(shcore::str_format(
        "Failed to refresh the OCI session of the profile '%s': %s",
        config_profile().c_str(), msg));
  };

  try {
    m_security_token =
        m_data_plane_client->refresh_user_security_token(m_security_token);
  } catch (const mysqlshdk::rest::Response_error &e) {
    if (mysqlshdk::rest::Response::Status_code::UNAUTHORIZED ==
        e.status_code()) {
      m_security_token_expired = true;

      throw error("session is no longer valid and cannot be refreshed");
    } else {
      throw error(e.format().c_str());
    }
  } catch (const std::exception &e) {
    throw error(e.what());
  }

  if (!shcore::create_file(m_security_token_file,
                           m_security_token.to_string())) {
    throw error("cannot update the security token file");
  }
}

}  // namespace oci
}  // namespace mysqlshdk
