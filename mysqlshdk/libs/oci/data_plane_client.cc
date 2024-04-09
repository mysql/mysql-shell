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

#include "mysqlshdk/libs/oci/data_plane_client.h"

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_json.h"

namespace mysqlshdk {
namespace oci {

Data_plane_client::Data_plane_client(Oci_credentials_provider *provider)
    : Oci_client(provider, Oci_client::endpoint_for("auth", provider->region()),
                 "OCI-AUTH") {
  log_debug("OCI data plane client: %s", service_endpoint().c_str());
}

Security_token Data_plane_client::generate_user_security_token(
    const Generate_user_security_token_details &details) {
  shcore::JSON_dumper json;

  json.start_object();
  json.append("publicKey", details.public_key);

  if (details.session_expiration_in_minutes.has_value()) {
    json.append("sessionExpirationInMinutes",
                *details.session_expiration_in_minutes);
  }

  json.end_object();

  return Security_token::from_json(
      post("/v1/token/upst/actions/GenerateUpst", json.str()));
}

Security_token Data_plane_client::refresh_user_security_token(
    const Security_token &token) {
  shcore::JSON_dumper json;

  json.start_object();
  json.append("currentToken", token.to_string());
  json.end_object();

  return Security_token::from_json(
      post("/v1/authentication/refresh", json.str()));
}

}  // namespace oci
}  // namespace mysqlshdk
