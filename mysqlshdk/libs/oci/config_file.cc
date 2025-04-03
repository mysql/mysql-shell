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

#include "mysqlshdk/libs/oci/config_file.h"

#include <cassert>
#include <stdexcept>
#include <string>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace mysqlshdk {
namespace oci {

Config_file::Config_file(const std::string &config_file,
                         const std::string &config_profile)
    : m_config_file(shcore::path::expand_user(config_file)),
      m_config_profile(config_profile),
      m_config(mysqlshdk::config::Case::SENSITIVE,
               mysqlshdk::config::Escape::NO) {
  log_info("Using the OCI config file '%s' and a profile named: '%s'",
           m_config_file.c_str(), m_config_profile.c_str());
}

std::string Config_file::config_option(Entry entry) {
  const char *env_var = nullptr;
  const char *name = nullptr;
  // if entry is optional, we don't report any errors, and simply return an
  // empty string
  bool optional = false;
#define DONT_FAIL_IF_OPTIONAL \
  do {                        \
    if (optional) {           \
      return {};              \
    }                         \
  } while (false)

  switch (entry) {
    case Entry::USER:
      env_var = "OCI_CLI_USER";
      name = "user";
      break;

    case Entry::FINGERPRINT:
      env_var = "OCI_CLI_FINGERPRINT";
      name = "fingerprint";
      break;

    case Entry::KEY_FILE:
      env_var = "OCI_CLI_KEY_FILE";
      name = "key_file";
      break;

    case Entry::PASS_PHRASE:
      env_var = "OCI_CLI_PASSPHRASE";
      name = "pass_phrase";
      optional = true;
      break;

    case Entry::TENANCY:
      env_var = "OCI_CLI_TENANCY";
      name = "tenancy";
      break;

    case Entry::REGION:
      env_var = "OCI_CLI_REGION";
      name = "region";
      break;

    case Entry::SECURITY_TOKEN_FILE:
      env_var = "OCI_CLI_SECURITY_TOKEN_FILE";
      name = "security_token_file";
      break;

    case Entry::DELEGATION_TOKEN_FILE:
      name = "delegation_token_file";
      break;
  }

  assert(name);

  if (env_var) {
    if (const auto env = shcore::get_env(env_var); env.has_value()) {
      log_info("The OCI config entry '%s' set via '%s' environment variable",
               name, env_var);
      return *env;
    }
  }

  if (!m_config_is_read) {
    try {
      m_config.read(m_config_file);
    } catch (const std::exception &e) {
      DONT_FAIL_IF_OPTIONAL;
      log_error("Failed to read the OCI config file '%s': %s",
                m_config_file.c_str(), e.what());
      throw std::runtime_error("Could not read the OCI config file.");
    }

    if (!m_config.has_group(m_config_profile)) {
      DONT_FAIL_IF_OPTIONAL;
      log_error(
          "The OCI config file '%s' does not contain a profile named: '%s'",
          m_config_file.c_str(), m_config_profile.c_str());
      throw std::runtime_error("The indicated OCI Profile does not exist.");
    }

    m_config_is_read = true;
  }

  if (!m_config.has_option(m_config_profile, name)) {
    DONT_FAIL_IF_OPTIONAL;
    throw std::runtime_error(shcore::str_format(
        "The OCI profile '%s' does not contain an entry named: '%s'",
        m_config_profile.c_str(), name));
  }

  auto option = m_config.get(m_config_profile, name);

  if (!option.has_value()) {
    DONT_FAIL_IF_OPTIONAL;
    throw std::runtime_error(shcore::str_format(
        "The OCI profile '%s' has an entry '%s' with no value",
        m_config_profile.c_str(), name));
  }

#undef DONT_FAIL_IF_OPTIONAL

  return std::move(*option);
}

}  // namespace oci
}  // namespace mysqlshdk
