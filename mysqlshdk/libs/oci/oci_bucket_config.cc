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

#include "mysqlshdk/libs/oci/oci_bucket_config.h"

#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/rest/signed/signed_request.h"
#include "mysqlshdk/libs/rest/signed/signed_rest_service.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/oci/api_key_credentials_provider.h"
#include "mysqlshdk/libs/oci/instance_principal_credentials_provider.h"
#include "mysqlshdk/libs/oci/instance_principal_delegation_token_credentials_provider.h"
#include "mysqlshdk/libs/oci/oci_bucket.h"
#include "mysqlshdk/libs/oci/resource_principal_credentials_provider.h"
#include "mysqlshdk/libs/oci/security_token_credentials_provider.h"

namespace mysqlshdk {
namespace oci {

Oci_bucket_config::Oci_bucket_config(const Oci_bucket_options &options)
    : Bucket_config(options),
      Config_file(options),
      Config_profile(options),
      m_namespace(options.m_namespace) {
  m_label = "OCI-OS";

  set_config_file();
  set_profile();
  set_authentication(options);

  resolve_credentials();
  configure_endpoint();
  fetch_namespace();
}

void Oci_bucket_config::set_config_file() {
  if (!m_config_file.empty()) {
    log_debug("The OCI config file set via '%s' option",
              Oci_bucket_options::config_file_option());
  }

  if (m_config_file.empty()) {
    static constexpr auto k_env_var = "OCI_CLI_CONFIG_FILE";

    if (const auto env = shcore::get_env(k_env_var); env.has_value()) {
      m_config_file = *env;
      log_debug("The OCI config file set via '%s' environment variable",
                k_env_var);
    }
  }

  if (m_config_file.empty()) {
    m_config_file = mysqlsh::current_shell_options()->get().oci_config_file;
    log_debug("The OCI config file set via '" SHCORE_OCI_CONFIG_FILE
              "' Shell option");
  }

  log_info("The OCI config file set to: %s", m_config_file.c_str());
}

void Oci_bucket_config::set_profile() {
  if (!m_config_profile.empty()) {
    log_debug("The OCI profile set via '%s' option",
              Oci_bucket_options::profile_option());
  }

  if (m_config_profile.empty()) {
    static constexpr auto k_env_var = "OCI_CLI_PROFILE";

    if (const auto env = shcore::get_env(k_env_var); env.has_value()) {
      m_config_profile = *env;
      log_debug("The OCI profile set via '%s' environment variable", k_env_var);
    }
  }

  if (m_config_profile.empty()) {
    m_config_profile = mysqlsh::current_shell_options()->get().oci_profile;
    log_debug("The OCI profile set via '" SHCORE_OCI_PROFILE "' Shell option");
  }

  log_info("The OCI profile set to: %s", m_config_profile.c_str());
}

void Oci_bucket_config::set_authentication(const Oci_bucket_options &options) {
  auto auth = options.authentication();

  if (auth.has_value()) {
    log_debug("The OCI authentication set via '%s' option",
              Oci_bucket_options::auth_option());
  }

  if (!auth.has_value()) {
    static constexpr auto k_env_var = "OCI_CLI_AUTH";

    if (const auto env = shcore::get_env(k_env_var); env.has_value()) {
      try {
        auth = to_authentication(*env);
      } catch (const std::exception &e) {
        throw std::invalid_argument(shcore::str_format(
            "Invalid value of '%s' environment variable, %s.", k_env_var,
            e.what()));
      }

      log_debug("The OCI authentication set via '%s' environment variable",
                k_env_var);
    }
  }

  if (!auth.has_value()) {
    auth = Authentication::API_KEY;
    log_debug("The OCI authentication set to the default value");
  }

  m_auth = *auth;

  log_info("The OCI authentication set to: %s", to_string(m_auth));
}

void Oci_bucket_config::resolve_credentials() {
  switch (m_auth) {
    case Authentication::API_KEY:
      m_credentials_provider = std::make_unique<Api_key_credentials_provider>(
          m_config_file, m_config_profile);
      break;

    case Authentication::INSTANCE_PRINCIPAL:
      m_credentials_provider =
          std::make_unique<Instance_principal_credentials_provider>();
      break;

    case Authentication::RESOURCE_PRINCIPAL:
      m_credentials_provider =
          std::make_unique<Resource_principal_credentials_provider>();
      break;

    case Authentication::SECURITY_TOKEN:
      m_credentials_provider =
          std::make_unique<Security_token_credentials_provider>(
              m_config_file, m_config_profile);
      break;

    case Authentication::INSTANCE_OBO_USER:
      m_credentials_provider = std::make_unique<
          Instance_principal_delegation_token_credentials_provider>(
          m_config_file, m_config_profile);
      break;
  }

  m_credentials_provider->initialize();
}

void Oci_bucket_config::fetch_namespace() {
  if (!m_namespace.empty()) {
    return;
  }

  static std::mutex s_mutex;
  static std::unordered_map<std::string, std::string> s_tenancy_names;

  std::lock_guard<std::mutex> lock(s_mutex);
  auto tenancy_name =
      s_tenancy_names.find(m_credentials_provider->tenancy_id());

  if (s_tenancy_names.end() == tenancy_name) {
    rest::Signed_rest_service service(*this);
    rest::Signed_request request("/n/");
    rest::String_response response;

    service.get(&request, &response);

    const auto &raw_data = response.buffer;

    try {
      tenancy_name =
          s_tenancy_names
              .emplace(m_credentials_provider->tenancy_id(),
                       shcore::Value::parse({raw_data.data(), raw_data.size()})
                           .as_string())
              .first;
    } catch (const shcore::Exception &error) {
      log_debug2("%s\n%.*s", error.what(), static_cast<int>(raw_data.size()),
                 raw_data.data());
      throw;
    }
  }

  m_namespace = tenancy_name->second;

  // namespace was missing, need to update the endpoint
  configure_endpoint();
}

std::unique_ptr<rest::ISigner> Oci_bucket_config::signer() const {
  return std::make_unique<Oci_signer>(*this);
}

std::unique_ptr<storage::backend::object_storage::Container>
Oci_bucket_config::container() const {
  return oci_bucket();
}

std::unique_ptr<Oci_bucket> Oci_bucket_config::oci_bucket() const {
  return std::make_unique<Oci_bucket>(shared_ptr<Oci_bucket_config>());
}

const std::string &Oci_bucket_config::hash() const {
  if (m_hash.empty()) {
    m_hash.reserve(512);

    m_hash += m_label;
    m_hash += '-';
    m_hash += m_endpoint;
    m_hash += '-';
    m_hash += m_container_name;
    m_hash += '-';
    m_hash += m_config_file;
    m_hash += '-';
    m_hash += m_config_profile;
    m_hash += '-';
    m_hash += m_namespace;
    m_hash += '-';
    m_hash += m_credentials_provider->credentials()->auth_key_id();
  }

  return m_hash;
}

std::string Oci_bucket_config::describe_self() const {
  return "OCI ObjectStorage bucket=" + m_container_name;
}

void Oci_bucket_config::configure_endpoint() {
  m_host = "objectstorage." + m_credentials_provider->region() +
           ".oci.customer-oci.com";

  if (!m_namespace.empty()) {
    m_host = m_namespace + '.' + m_host;
  }

  m_endpoint = "https://" + m_host;
}

}  // namespace oci
}  // namespace mysqlshdk
