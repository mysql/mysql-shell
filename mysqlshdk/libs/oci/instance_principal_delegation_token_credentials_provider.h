/*
 * Copyright (c) 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_OCI_INSTANCE_PRINCIPAL_DELEGATION_TOKEN_CREDENTIALS_PROVIDER_H_
#define MYSQLSHDK_LIBS_OCI_INSTANCE_PRINCIPAL_DELEGATION_TOKEN_CREDENTIALS_PROVIDER_H_

#include <string>

#include "mysqlshdk/libs/oci/instance_principal_credentials_provider.h"

namespace mysqlshdk {
namespace oci {

/**
 * Instance principal with delegation token authentication.
 */
class Instance_principal_delegation_token_credentials_provider final
    : public Instance_principal_credentials_provider {
 public:
  Instance_principal_delegation_token_credentials_provider(
      const std::string &config_file, const std::string &config_profile);

  Instance_principal_delegation_token_credentials_provider(
      const Instance_principal_delegation_token_credentials_provider &) =
      delete;
  Instance_principal_delegation_token_credentials_provider(
      Instance_principal_delegation_token_credentials_provider &&) = delete;

  Instance_principal_delegation_token_credentials_provider &operator=(
      const Instance_principal_delegation_token_credentials_provider &) =
      delete;
  Instance_principal_delegation_token_credentials_provider &operator=(
      Instance_principal_delegation_token_credentials_provider &&) = delete;

  ~Instance_principal_delegation_token_credentials_provider() override =
      default;

 private:
  Credentials fetch_credentials() override;

  std::string m_delegation_token_file;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_INSTANCE_PRINCIPAL_DELEGATION_TOKEN_CREDENTIALS_PROVIDER_H_
