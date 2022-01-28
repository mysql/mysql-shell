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

#include "mysqlshdk/libs/oci/oci_bucket_config.h"

#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/rest/signed_rest_service.h"
#include "mysqlshdk/shellcore/private_key_manager.h"

#include "mysqlshdk/libs/oci/oci_bucket.h"
#include "mysqlshdk/libs/oci/oci_setup.h"
#include "mysqlshdk/libs/oci/oci_signer.h"

namespace mysqlshdk {
namespace oci {

Oci_bucket_config::Oci_bucket_config(const Oci_bucket_options &options)
    : Config(options), m_namespace(options.m_namespace) {
  if (m_config_file.empty()) {
    m_config_file = mysqlsh::current_shell_options()->get().oci_config_file;
  }

  if (m_config_profile.empty()) {
    m_config_profile = mysqlsh::current_shell_options()->get().oci_profile;
  }

  Oci_setup oci_setup(m_config_file);

  if (!oci_setup.has_profile(m_config_profile)) {
    throw std::runtime_error("The indicated OCI Profile does not exist.");
  }

  const auto &config = oci_setup.get_cfg();

  m_host = "objectstorage." + *config.get(m_config_profile, "region") +
           ".oraclecloud.com";
  m_endpoint = "https://" + m_host;

  m_tenancy_id = *config.get(m_config_profile, "tenancy");
  m_user = *config.get(m_config_profile, "user");
  m_fingerprint = *config.get(m_config_profile, "fingerprint");
  m_key_file = *config.get(m_config_profile, "key_file");

  load_key(&oci_setup);
  fetch_namespace();
}

void Oci_bucket_config::load_key(Oci_setup *setup) {
  auto key = shcore::Private_key_storage::get().contains(m_key_file);

  if (!key.second) {
    setup->load_profile(m_config_profile);

    // Check if load_profile() opened successfully private key and put it into
    // private key storage.
    key = shcore::Private_key_storage::get().contains(m_key_file);

    if (!key.second) {
      throw std::runtime_error("Cannot load \"" + m_key_file +
                               "\" private key associated with OCI "
                               "configuration profile named \"" +
                               m_config_profile + "\".");
    }
  }
}

void Oci_bucket_config::fetch_namespace() {
  if (m_namespace.empty()) {
    static std::mutex s_mutex;
    static std::unordered_map<std::string, std::string> s_tenancy_names;

    std::lock_guard<std::mutex> lock(s_mutex);
    auto tenancy_name = s_tenancy_names.find(m_tenancy_id);

    if (s_tenancy_names.end() == tenancy_name) {
      rest::Signed_rest_service service(*this);
      rest::Signed_request request("/n/");
      rest::String_response response;

      service.get(&request, &response);

      const auto &raw_data = response.buffer;

      try {
        tenancy_name =
            s_tenancy_names
                .emplace(m_tenancy_id,
                         shcore::Value::parse(raw_data.data(), raw_data.size())
                             .as_string())
                .first;
      } catch (const shcore::Exception &error) {
        log_debug2("%s\n%.*s", error.what(), static_cast<int>(raw_data.size()),
                   raw_data.data());
        throw;
      }
    }

    m_namespace = tenancy_name->second;
  }
}

std::unique_ptr<rest::Signer> Oci_bucket_config::signer() const {
  return std::make_unique<Oci_signer>(*this);
}

std::unique_ptr<storage::backend::object_storage::Bucket>
Oci_bucket_config::bucket() const {
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
    m_hash += m_bucket_name;
    m_hash += '-';
    m_hash += m_config_file;
    m_hash += '-';
    m_hash += m_config_profile;
    m_hash += '-';
    m_hash += m_namespace;
    m_hash += '-';
    m_hash += m_key_file;
    m_hash += '-';
    m_hash += m_tenancy_id;
    m_hash += '-';
    m_hash += m_user;
    m_hash += '-';
    m_hash += m_fingerprint;
  }

  return m_hash;
}

std::string Oci_bucket_config::describe_self() const {
  return "OCI ObjectStorage bucket=" + m_bucket_name;
}

}  // namespace oci
}  // namespace mysqlshdk
