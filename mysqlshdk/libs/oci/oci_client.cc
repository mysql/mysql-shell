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

#include "mysqlshdk/libs/oci/oci_client.h"

#include <utility>

#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace oci {

Oci_client::Oci_client(Oci_credentials_provider *provider, std::string endpoint,
                       std::string label)
    : m_label(std::move(label)),
      m_endpoint(std::move(endpoint)),
      m_provider(provider) {
  m_host = storage::utils::strip_scheme(m_endpoint);

  if (m_host == m_endpoint) {
    // endpoint has no scheme
    m_endpoint = "https://" + m_host;
  }
}

std::string Oci_client::endpoint_for(std::string_view service,
                                     std::string_view region) {
  return shcore::str_format("https://%.*s.%.*s.oraclecloud.com",
                            static_cast<int>(service.length()), service.data(),
                            static_cast<int>(region.length()), region.data());
}

std::unique_ptr<rest::ISigner> Oci_client::signer() const {
  return std::make_unique<Oci_signer>(*this);
}

rest::Signed_rest_service *Oci_client::http() {
  if (!m_http) {
    m_http = std::make_unique<rest::Signed_rest_service>(*this);
  }

  return m_http.get();
}

std::string Oci_client::get(const std::string &path) {
  rest::Signed_request request{path};

  m_response.body->clear();

  http()->get(&request, &m_response);

  return std::move(m_response.buffer).steal_buffer();
}

std::string Oci_client::post(const std::string &path, const std::string &body) {
  rest::Signed_request request{path};
  request.body = body.c_str();
  request.size = body.length();

  m_response.body->clear();

  http()->post(&request, &m_response);

  return std::move(m_response.buffer).steal_buffer();
}

}  // namespace oci
}  // namespace mysqlshdk
