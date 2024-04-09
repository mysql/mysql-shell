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

#ifndef MYSQLSHDK_LIBS_OCI_OCI_CLIENT_H_
#define MYSQLSHDK_LIBS_OCI_OCI_CLIENT_H_

#include <memory>
#include <string>
#include <string_view>

#include "mysqlshdk/libs/rest/signed/signed_rest_service.h"
#include "mysqlshdk/libs/rest/signed/signed_rest_service_config.h"

#include "mysqlshdk/libs/oci/oci_credentials_provider.h"
#include "mysqlshdk/libs/oci/oci_signer.h"

namespace mysqlshdk {
namespace oci {

/**
 * An OCI client.
 */
class Oci_client : public Oci_signer_config,
                   public rest::Signed_rest_service_config {
 public:
  Oci_client(Oci_credentials_provider *provider, std::string endpoint,
             std::string label);

  Oci_client(const Oci_client &) = delete;
  Oci_client(Oci_client &&) = default;

  Oci_client &operator=(const Oci_client &) = delete;
  Oci_client &operator=(Oci_client &&) = default;

  ~Oci_client() = default;

  static std::string endpoint_for(std::string_view service,
                                  std::string_view region);

  const std::string &host() const override { return m_host; }

  Oci_credentials_provider *credentials_provider() const override {
    return m_provider;
  }

  const std::string &service_endpoint() const override { return m_endpoint; }

  const std::string &service_label() const override { return m_label; }

  std::unique_ptr<rest::ISigner> signer() const override;

 protected:
  rest::Signed_rest_service *http();

  std::string get(const std::string &path);

  std::string post(const std::string &path, const std::string &body);

  inline const rest::String_response &response() const noexcept {
    return m_response;
  }

 private:
  std::string m_label;

  std::string m_endpoint;
  std::string m_host;

  Oci_credentials_provider *m_provider;

  std::unique_ptr<rest::Signed_rest_service> m_http;
  rest::String_response m_response;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_OCI_CLIENT_H_
