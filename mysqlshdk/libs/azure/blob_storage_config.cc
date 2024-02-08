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

#include "mysqlshdk/libs/azure/blob_storage_config.h"

#include <memory>
#include <stdexcept>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/azure/blob_storage_container.h"
#include "mysqlshdk/libs/azure/signer.h"
#include "mysqlshdk/libs/config/config_file.h"
#include "mysqlshdk/libs/db/generic_uri.h"
#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace mysqlshdk {
namespace azure {

// Env Vars
constexpr const char k_azure_storage_account_env[] = "AZURE_STORAGE_ACCOUNT";
constexpr const char k_azure_storage_key_env[] = "AZURE_STORAGE_KEY";
constexpr const char k_azure_storage_sas_token_env[] =
    "AZURE_STORAGE_SAS_TOKEN";
constexpr const char k_azure_storage_connection_string_env[] =
    "AZURE_STORAGE_CONNECTION_STRING";

// Connection String Elements
constexpr const char k_conn_string_account_name[] = "AccountName";
constexpr const char k_conn_string_account_key[] = "AccountKey";
constexpr const char k_conn_string_blob_endpoint[] = "BlobEndpoint";
constexpr const char k_conn_string_endpoint_suffix[] = "EndpointSuffix";
constexpr const char k_conn_string_defaults_endpoint_protocol[] =
    "DefaultEndpointsProtocol";

// Config File
constexpr const char k_config_storage[] = "storage";
constexpr const char k_config_connection_string[] = "connection_string";
constexpr const char k_config_account[] = "account";
constexpr const char k_config_key[] = "key";
constexpr const char k_config_sas_token[] = "sas_token";

Blob_storage_config::Blob_storage_config(const Blob_storage_options &options,
                                         bool enable_env_vars)
    : Config(options, DEFAULT_BLOB_BLOCK_SIZE),
      m_account_name(options.m_storage_account),
      m_sas_token(options.m_storage_sas_token),
      m_operation(options.m_operation) {
  m_sas_token_source =
      shcore::str_format("the '%s' option", options.storage_sas_token_option());

  if (enable_env_vars) {
    load_env_vars();
  }

  load_config(options.m_config_file);

  // Default values as needed
  if (m_endpoint_protocol.empty()) m_endpoint_protocol = "https";
  if (m_endpoint.empty()) {
    m_endpoint = m_endpoint_protocol + "://" + m_account_name;
    if (!m_endpoint_suffix.empty()) {
      m_endpoint += ".blob.";
      m_endpoint += m_endpoint_suffix;
    } else {
      m_endpoint += ".blob.core.windows.net";
    }
  }

  if (!m_sas_token.empty()) {
    m_sas_token_data =
        std::move(mysqlshdk::db::uri::Generic_uri("host?" + m_sas_token).query);
  }

  validate_config();
}

std::unique_ptr<rest::Signer> Blob_storage_config::signer() const {
  return std::make_unique<Signer>(*this);
}

std::unique_ptr<storage::backend::object_storage::Container>
Blob_storage_config::container() const {
  return blob_container();
}

std::unique_ptr<Blob_container> Blob_storage_config::blob_container() const {
  return std::make_unique<Blob_container>(shared_ptr<Blob_storage_config>());
}

const std::string &Blob_storage_config::hash() const {
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
    m_hash += m_account_name;
    m_hash += '-';
    m_hash += m_account_key;
  }

  return m_hash;
}

std::string Blob_storage_config::describe_self() const {
  return "Azure Container=" + m_container_name;
}

void Blob_storage_config::load_connection_string(
    const std::string &connection_string) {
  auto parse_tokens = [this](std::string_view item) {
    if (item.empty()) return true;

    auto config = shcore::str_split(item, "=", 1);
    auto key = shcore::str_lower(shcore::str_strip(config[0]));
    std::string value = config.size() > 1 ? shcore::str_strip(config[1]) : "";

    if (m_endpoint.empty() &&
        shcore::str_caseeq(key, k_conn_string_blob_endpoint)) {
      mysqlshdk::db::uri::Generic_uri uri(value);
      m_endpoint = uri.as_uri(mysqlshdk::db::uri::formats::scheme_transport());
      m_endpoint_protocol = uri.scheme;
      m_endpoint_path = uri.path;
    }

    if (m_endpoint_protocol.empty() &&
        shcore::str_caseeq(key, k_conn_string_defaults_endpoint_protocol)) {
      m_endpoint_protocol = std::move(value);
    }

    if (m_account_name.empty() &&
        shcore::str_caseeq(key, k_conn_string_account_name)) {
      m_account_name = std::move(value);
    }

    if (m_account_key.empty() &&
        shcore::str_caseeq(key, k_conn_string_account_key)) {
      m_account_key = std::move(value);
    }

    if (m_endpoint_suffix.empty() &&
        shcore::str_caseeq(key, k_conn_string_endpoint_suffix)) {
      m_endpoint_suffix = std::move(value);
    }

    return true;
  };

  shcore::str_itersplit(connection_string, parse_tokens, ";", -1, true);
}

void Blob_storage_config::load_env_vars() {
  if (const auto data = getenv(k_azure_storage_connection_string_env)) {
    load_connection_string(data);
  }

  if (m_account_name.empty()) {
    if (const auto data = getenv(k_azure_storage_account_env)) {
      m_account_name.assign(data);
    }
  }

  if (m_account_key.empty()) {
    if (const auto data = getenv(k_azure_storage_key_env)) {
      m_account_key.assign(data);
    }
  }

  if (m_sas_token.empty()) {
    if (const auto data = getenv(k_azure_storage_sas_token_env)) {
      m_sas_token = data;
      m_sas_token_source = shcore::str_format("the '%s' environment variable",
                                              k_azure_storage_sas_token_env);
    }
  }
}

std::string Blob_storage_config::get_default_config_path() const {
  return shcore::path::join_path(shcore::get_home_dir(), ".azure", "config");
}

void Blob_storage_config::load_config(const std::string &user_config_path) {
  bool is_default = user_config_path.empty();
  std::string config_file{is_default ? get_default_config_path()
                                     : user_config_path};

  if (!shcore::path_exists(config_file)) {
    if (!is_default) {
      throw std::invalid_argument(shcore::str_format(
          "The azure configuration file '%s' does not exist.",
          Blob_storage_options::config_file_option()));
    }

    // No loading to be done, early exit.
    return;
  } else if (!shcore::is_file(config_file)) {
    throw std::invalid_argument(
        shcore::str_format("The azure configuration file '%s' is invalid.",
                           Blob_storage_options::config_file_option()));
  }

  mysqlshdk::config::Config_file config;
  config.read(config_file);

  auto read_config = [&config](const std::string &section,
                               const std::string &option) -> std::string {
    if (config.has_option(section, option)) {
      return *config.get(section, option);
    } else {
      return {};
    }
  };

  std::string connection_string =
      read_config(k_config_storage, k_config_connection_string);
  if (!connection_string.empty()) {
    load_connection_string(connection_string);
  }

  if (m_account_name.empty()) {
    m_account_name = read_config(k_config_storage, k_config_account);
  }

  if (m_account_key.empty()) {
    m_account_key = read_config(k_config_storage, k_config_key);
  }

  if (m_sas_token.empty()) {
    m_sas_token = read_config(k_config_storage, k_config_sas_token);
    m_sas_token_source = shcore::str_format(
        "the '%s' option at the '%s' section at the '%s' configuration "
        "file",
        k_config_sas_token, k_config_storage, config_file.c_str());
  }
}

bool Blob_storage_config::valid() const {
  return !m_account_name.empty() && !m_container_name.empty() &&
         (!m_account_key.empty() || !m_sas_token_data.empty());
}

std::string Blob_storage_config::describe_url(const std::string &url) const {
  return "prefix='" + url + "'";
}

namespace {
const std::unordered_map<std::string, std::string> kSasAttributes = {
    {"sv", "Signed Version"},
    {"ss", "Signed Services"},
    {"srt", "Signed Resource Types"},
    {"sr", "Signed Resource"},
    {"sp", "Signed Permissions"},
    {"se", "Expiration Time"},
    {"st", "Start Time"},
    {"spr", "Signed Protocols"},
    {"sig", "Signature"}};

const std::unordered_map<char, std::string> kSasPermissions = {
    {'r', "Read"}, {'l', "List"}, {'w', "Write"}, {'c', "Create"}};

const std::vector<std::string> kAccountSpecificSasTokens = {"ss", "srt"};

}  // namespace

void Blob_storage_config::throw_sas_token_error(
    const std::string &error) const {
  throw std::invalid_argument(
      shcore::str_format("The Shared Access Signature Token defined at %s is "
                         "invalid, %s",
                         m_sas_token_source.c_str(), error.c_str()));
}

void Blob_storage_config::validate_config() const {
  if (m_account_name.empty()) {
    throw std::runtime_error(
        shcore::str_format("The Azure Storage Account('%s') is not defined.",
                           Blob_storage_options::storage_account_option()));
  }

  if (m_account_name.length() < 3 || m_account_name.length() > 24) {
    throw std::invalid_argument(
        "The specified Azure Storage Account name is invalid, expected 3 to 24 "
        "characters: " +
        m_account_name);
  }

  if (![this]() {
        for (const auto c : m_account_name) {
          if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z'))) {
            return false;
          }
        }

        return true;
      }()) {
    throw std::invalid_argument(
        "The specified Azure Storage Account name is invalid, expected numbers "
        "and lowercase characters: " +
        m_account_name);
  }

  log_info("Used Account: '%s'", m_account_name.c_str());

  if (m_sas_token_data.empty()) {
    if (m_account_key.empty()) {
      throw std::runtime_error(shcore::str_format(
          "No Azure Storage Account Key or Shared Access Signature Token "
          "('%s') is defined.",
          Blob_storage_options::storage_sas_token_option()));
    }
  } else {
    // Determine if this is an account sas token
    auto account_sas_token = std::find_first_of(
        m_sas_token_data.begin(), m_sas_token_data.end(),
        kAccountSpecificSasTokens.begin(), kAccountSpecificSasTokens.end(),
        [](const std::pair<std::string, std::optional<std::string>> &member,
           const std::string &token) { return member.first == token; });

    std::vector<std::string> required = {"sv", "sp", "se", "sig"};

    if (account_sas_token != m_sas_token_data.end()) {
      required.push_back("srt");
      required.push_back("ss");
    } else {
      required.push_back("sr");
    }

    std::vector<std::string> missing_attributes;

    // Ensures the SAS token has the required elements
    for (const auto &attribute : required) {
      const auto &item = std::find_if(
          m_sas_token_data.begin(), m_sas_token_data.end(),
          [&attribute](const std::pair<std::string, std::optional<std::string>>
                           &member) { return member.first == attribute; });

      if (item == m_sas_token_data.end()) {
        missing_attributes.push_back(kSasAttributes.at(attribute));
      } else {
        if (item->first == "ss" &&
            item->second.value_or("").find("b") == std::string::npos) {
          throw_sas_token_error(
              "it is missing access to the Blob Storage Service");
        } else if (item->first == "sp") {
          std::string actual_permissions = item->second.value_or("");
          std::string required_permissions = "lr";
          std::vector<std::string> missing_permissions;

          for (const auto p : required_permissions) {
            if (actual_permissions.find(p) == std::string::npos) {
              missing_permissions.push_back(kSasPermissions.at(p));
            }
          }

          if (m_operation == Blob_storage_options::Operation::WRITE &&
              actual_permissions.find('c') == std::string::npos &&
              actual_permissions.find('w') == std::string::npos) {
            missing_permissions.push_back(
                shcore::str_format("%s or %s", kSasPermissions.at('c').c_str(),
                                   kSasPermissions.at('w').c_str()));
          }

          if (!missing_permissions.empty()) {
            throw_sas_token_error(shcore::str_format(
                "it is missing the following permissions: %s",
                shcore::str_join(missing_permissions, ", ").c_str()));
          }
        } else if (item->first == "sr" && item->second.value_or("") != "c") {
          throw_sas_token_error("does not give access to the container");
        } else if (item->first == "srt") {
          if (item->second.value_or("").find("c") == std::string::npos) {
            throw_sas_token_error("does not give access to the container");
          }

          if (item->second.value_or("").find("o") == std::string::npos) {
            throw_sas_token_error(
                "does not give access to the container objects");
          }
        }
      }

      if (!missing_attributes.empty()) {
        throw_sas_token_error(shcore::str_format(
            "the following attributes are missing: %s",
            shcore::str_join(missing_attributes, ", ").c_str()));
      }
    }
  }
}
}  // namespace azure
}  // namespace mysqlshdk
