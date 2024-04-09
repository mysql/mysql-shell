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

#include "mysqlshdk/libs/oci/federation_client.h"

#include "mysqlshdk/libs/utils/utils_json.h"

namespace mysqlshdk {
namespace oci {

Federation_client::Federation_client(Federation_credentials_provider *provider)
    : Oci_client(provider, Oci_client::endpoint_for("auth", provider->region()),
                 "OCI-X509") {
  log_debug("OCI X509 federation client: %s", service_endpoint().c_str());
}

Security_token Federation_client::token(
    const X509_federation_details &details) {
  shcore::JSON_dumper json;

  json.start_object();
  json.append("certificate", details.certificate);
  json.append("publicKey", details.public_key);

  if (!details.intermediate_certificates.empty()) {
    json.append("intermediateCertificates");
    json.start_array();

    for (const auto &cert : details.intermediate_certificates) {
      json.append(cert);
    }

    json.end_array();
  }

  json.end_object();

  return Security_token::from_json(post("/v1/x509", json.str()));
}

}  // namespace oci
}  // namespace mysqlshdk
