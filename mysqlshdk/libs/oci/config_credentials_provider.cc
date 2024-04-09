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

#include "mysqlshdk/libs/oci/config_credentials_provider.h"

#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace oci {

Config_credentials_provider::Config_credentials_provider(
    const std::string &config_file, const std::string &config_profile,
    std::string name)
    : Oci_credentials_provider(std::move(name)),
      m_oci_setup(config_file),
      m_config_profile(config_profile) {
  if (!m_oci_setup.has_profile(m_config_profile)) {
    log_error("The OCI config file '%s' does not contain a profile named: '%s'",
              config_file.c_str(), m_config_profile.c_str());
    throw std::runtime_error("The indicated OCI Profile does not exist.");
  }

  log_info("Using the OCI config file '%s' and a profile named: '%s'",
           config_file.c_str(), m_config_profile.c_str());

  set_region(config_option("region"));
  set_tenancy_id(config_option("tenancy"));

  m_credentials.private_key_id = load_key(config_option("key_file")).id();
}

std::string Config_credentials_provider::config_option(const char *name) const {
  const auto &config = m_oci_setup.get_cfg();

  if (!config.has_option(m_config_profile, name)) {
    throw std::runtime_error(shcore::str_format(
        "The OCI profile '%s' does not contain an entry named: %s",
        m_config_profile.c_str(), name));
  }

  auto option = config.get(m_config_profile, name);

  if (!option.has_value()) {
    throw std::runtime_error(shcore::str_format(
        "The OCI profile '%s' has an entry '%s' with no value",
        m_config_profile.c_str(), name));
  }

  return std::move(*option);
}

shcore::ssl::Private_key_id Config_credentials_provider::load_key(
    const std::string &key_file) {
  auto id = shcore::ssl::Private_key_id::from_path(key_file);

  if (!shcore::ssl::Private_key_storage::instance().contains(id)) {
    m_oci_setup.load_profile(m_config_profile);

    // Check if load_profile() opened successfully private key and put it into
    // private key storage.
    if (!shcore::ssl::Private_key_storage::instance().contains(id)) {
      throw std::runtime_error(
          shcore::str_format("Cannot load '%s' private key associated with OCI "
                             "configuration profile named '%s'.",
                             key_file.c_str(), m_config_profile.c_str()));
    }
  }

  return id;
}

}  // namespace oci
}  // namespace mysqlshdk
