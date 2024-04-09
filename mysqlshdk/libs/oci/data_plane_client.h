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

#ifndef MYSQLSHDK_LIBS_OCI_DATA_PLANE_CLIENT_H_
#define MYSQLSHDK_LIBS_OCI_DATA_PLANE_CLIENT_H_

#include <cinttypes>
#include <optional>
#include <string>

#include "mysqlshdk/libs/oci/oci_client.h"
#include "mysqlshdk/libs/oci/oci_credentials_provider.h"
#include "mysqlshdk/libs/oci/security_token.h"

namespace mysqlshdk {
namespace oci {

struct Generate_user_security_token_details {
  std::string public_key;
  std::optional<uint8_t> session_expiration_in_minutes{};
};

/**
 * Client which implements Identity and Access Management Data Plane API.
 */
class Data_plane_client final : public Oci_client {
 public:
  explicit Data_plane_client(Oci_credentials_provider *provider);

  Data_plane_client(const Data_plane_client &) = delete;
  Data_plane_client(Data_plane_client &&) = default;

  Data_plane_client &operator=(const Data_plane_client &) = delete;
  Data_plane_client &operator=(Data_plane_client &&) = default;

  ~Data_plane_client() = default;

  Security_token generate_user_security_token(
      const Generate_user_security_token_details &details);

  Security_token refresh_user_security_token(const Security_token &token);
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_DATA_PLANE_CLIENT_H_
