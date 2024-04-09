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

#ifndef MYSQLSHDK_LIBS_OCI_RESOURCE_PRINCIPAL_CREDENTIALS_PROVIDER_H_
#define MYSQLSHDK_LIBS_OCI_RESOURCE_PRINCIPAL_CREDENTIALS_PROVIDER_H_

#include <memory>

#include "mysqlshdk/libs/oci/oci_credentials_provider.h"

namespace mysqlshdk {
namespace oci {

/**
 * Resource principal authentication.
 */
class Resource_principal_credentials_provider final
    : public Oci_credentials_provider {
 public:
  Resource_principal_credentials_provider();

  Resource_principal_credentials_provider(
      const Resource_principal_credentials_provider &) = delete;
  Resource_principal_credentials_provider(
      Resource_principal_credentials_provider &&) = delete;

  Resource_principal_credentials_provider &operator=(
      const Resource_principal_credentials_provider &) = delete;
  Resource_principal_credentials_provider &operator=(
      Resource_principal_credentials_provider &&) = delete;

 private:
  Credentials fetch_credentials() override;

  std::unique_ptr<Oci_credentials_provider> m_resource_principal;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_RESOURCE_PRINCIPAL_CREDENTIALS_PROVIDER_H_
