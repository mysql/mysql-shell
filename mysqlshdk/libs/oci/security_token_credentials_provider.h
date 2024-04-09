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

#ifndef MYSQLSHDK_LIBS_OCI_SECURITY_TOKEN_CREDENTIALS_PROVIDER_H_
#define MYSQLSHDK_LIBS_OCI_SECURITY_TOKEN_CREDENTIALS_PROVIDER_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/oci/config_credentials_provider.h"
#include "mysqlshdk/libs/oci/data_plane_client.h"
#include "mysqlshdk/libs/oci/fixed_credentials_provider.h"
#include "mysqlshdk/libs/oci/security_token.h"

namespace mysqlshdk {
namespace oci {

/**
 * Provides session token-based authentication, authentication details are read
 * from the config file and a token file.
 */
class Security_token_credentials_provider final
    : public Config_credentials_provider {
 public:
  Security_token_credentials_provider(const std::string &config_file,
                                      const std::string &config_profile);

  Security_token_credentials_provider(
      const Security_token_credentials_provider &) = delete;
  Security_token_credentials_provider(Security_token_credentials_provider &&) =
      delete;

  Security_token_credentials_provider &operator=(
      const Security_token_credentials_provider &) = delete;
  Security_token_credentials_provider &operator=(
      Security_token_credentials_provider &&) = delete;

 private:
  Credentials fetch_credentials() override;

  void read_token_file();

  void refresh_token();

  std::string m_security_token_file;
  Security_token m_security_token;
  bool m_security_token_expired = false;

  std::unique_ptr<Data_plane_client> m_data_plane_client;
  std::unique_ptr<Fixed_credentials_provider> m_data_plane_credentials_provider;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_SECURITY_TOKEN_CREDENTIALS_PROVIDER_H_
