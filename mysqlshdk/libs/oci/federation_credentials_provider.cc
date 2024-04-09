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

#include "mysqlshdk/libs/oci/federation_credentials_provider.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace oci {

namespace {

using shcore::ssl::Hash;

}  // namespace

Federation_credentials_provider::Federation_credentials_provider(
    const Instance_metadata_retriever &retriever)
    : Oci_credentials_provider("X.509 federation"),
      m_instance_metadata(retriever) {
  set_region(m_instance_metadata.region());
  set_tenancy_id(m_instance_metadata.tenancy_id());
}

Federation_credentials_provider::Credentials
Federation_credentials_provider::fetch_credentials() {
  if (const auto tenancy = m_instance_metadata.tenancy_id();
      tenancy_id() != tenancy) {
    log_error("Tenancy ID has changed unexpectedly, previous: %s, updated: %s",
              tenancy_id().c_str(), tenancy.c_str());
    throw std::runtime_error(
        "Unexpected update of tenancy OCID in the leaf certificate");
  }

  m_keychain.use(m_instance_metadata.private_key());

  return {shcore::str_format("%s/fed-x509/%s", tenancy_id().c_str(),
                             m_instance_metadata.leaf_certificate()
                                 .fingerprint(Hash::RESTRICTED_SHA1)
                                 .c_str()),
          m_keychain.id().id()};
}

}  // namespace oci
}  // namespace mysqlshdk
