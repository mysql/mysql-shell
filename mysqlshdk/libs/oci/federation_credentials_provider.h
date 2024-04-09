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

#ifndef MYSQLSHDK_LIBS_OCI_FEDERATION_CREDENTIALS_PROVIDER_H_
#define MYSQLSHDK_LIBS_OCI_FEDERATION_CREDENTIALS_PROVIDER_H_

#include <memory>

#include "mysqlshdk/libs/utils/utils_private_key.h"

#include "mysqlshdk/libs/oci/instance_metadata_retriever.h"
#include "mysqlshdk/libs/oci/oci_credentials_provider.h"

namespace mysqlshdk {
namespace oci {

/**
 * Provides credentials for X509 federation request.
 *
 * NOTE: credentials are permanent, even though it's possible to determine their
 * expiration time. This is because data in the federation request needs to be
 * in sync with the credentials, and having an expiration time would cause the
 * credentials to refresh when requests fails with 401 HTTP error,
 * desynchronizing the data. Instead, retriever should be refreshed before
 * initializing this provider.
 */
class Federation_credentials_provider final : public Oci_credentials_provider {
 public:
  explicit Federation_credentials_provider(
      const Instance_metadata_retriever &retriever);

  Federation_credentials_provider(const Federation_credentials_provider &) =
      delete;
  Federation_credentials_provider(Federation_credentials_provider &&) = delete;

  Federation_credentials_provider &operator=(
      const Federation_credentials_provider &) = delete;
  Federation_credentials_provider &operator=(
      Federation_credentials_provider &&) = delete;

 private:
  Credentials fetch_credentials() override;

  const Instance_metadata_retriever &m_instance_metadata;
  shcore::ssl::Keychain m_keychain;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_FEDERATION_CREDENTIALS_PROVIDER_H_
