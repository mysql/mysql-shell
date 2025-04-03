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

#include "mysqlshdk/libs/oci/instance_principal_credentials_provider.h"

#include <cassert>
#include <cinttypes>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "mysqlshdk/libs/rest/rest_service.h"
#include "mysqlshdk/libs/rest/retry_strategy.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_private_key.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/utils_x509.h"

#include "mysqlshdk/libs/oci/oci_regions.h"
#include "mysqlshdk/libs/oci/security_token.h"

namespace mysqlshdk {
namespace oci {

namespace {

using shcore::ssl::Private_key;

}  // namespace

Instance_principal_credentials_provider::
    Instance_principal_credentials_provider()
    : Instance_principal_credentials_provider("instance_principal") {}

Instance_principal_credentials_provider::
    Instance_principal_credentials_provider(std::string name)
    : Oci_credentials_provider(std::move(name)) {
  try {
    m_instance_metadata = std::make_unique<Instance_metadata_retriever>();
    m_instance_metadata->refresh();
  } catch (const std::exception &e) {
    log_error("Failed to fetch instance metadata: %s", e.what());
    throw std::runtime_error(
        "Instance principal authentication can only be used on OCI compute "
        "instances");
  }

  set_region(m_instance_metadata->region());
  set_tenancy_id(m_instance_metadata->tenancy_id());
}

Instance_principal_credentials_provider::Credentials
Instance_principal_credentials_provider::fetch_credentials() {
  if (!m_credentials_provider) {
    m_credentials_provider =
        std::make_unique<Federation_credentials_provider>(*m_instance_metadata);
  } else {
    // refresh the metadata, we need to have an up to date credentials and
    // matching certificates
    m_instance_metadata->refresh();
  }

  m_credentials_provider->initialize();

  if (!m_federation_client) {
    m_federation_client =
        std::make_unique<Federation_client>(m_credentials_provider.get());
  }

  const auto private_key = Private_key::generate();
  m_keychain.use(private_key);

  Security_token token;
  bool retry = true;

  while (true) {
    try {
      token = m_federation_client->token(
          {m_instance_metadata->leaf_certificate().to_string(),
           private_key.public_key().to_string(),
           {m_instance_metadata->intermediate_certificate().to_string()}});
      break;
    } catch (const std::exception &e) {
      if (retry) {
        // force metadata refresh, we retry only once as scenario for failure
        // is: certificates were fetched just before they have expired, request
        // was done just after they have expired
        m_instance_metadata->refresh();
        m_credentials_provider->initialize();
        retry = false;

        log_info("Failed to fetch instance principal credentials: %s, retrying",
                 e.what());
      } else {
        log_error("Failed to fetch instance principal credentials: %s",
                  e.what());
        throw;
      }
    }
  }

  return {token.auth_key_id(), m_keychain.id().id(), token.expiration()};
}

}  // namespace oci
}  // namespace mysqlshdk
