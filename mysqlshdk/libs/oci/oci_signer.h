/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MYSQLSHDK_LIBS_OCI_OCI_SIGNER_H_
#define MYSQLSHDK_LIBS_OCI_OCI_SIGNER_H_

#include <openssl/pem.h>

#include <memory>
#include <string>

#include "mysqlshdk/libs/rest/signed_rest_service.h"

#include "mysqlshdk/libs/oci/oci_bucket_config.h"

namespace mysqlshdk {
namespace oci {

class Oci_signer : public rest::Signer {
 public:
  Oci_signer() = delete;

  explicit Oci_signer(const Oci_bucket_config &config);

  Oci_signer(const Oci_signer &) = default;
  Oci_signer(Oci_signer &&) = default;

  Oci_signer &operator=(const Oci_signer &) = default;
  Oci_signer &operator=(Oci_signer &&) = default;

  ~Oci_signer() override = default;

  bool should_sign_request(const rest::Signed_request *) const override {
    return true;
  }

  rest::Headers sign_request(const rest::Signed_request *request,
                             time_t now) const override;

  bool refresh_auth_data() override { return false; }

 protected:
  void set_auth_key_id(const std::string &auth_key_id) {
    m_auth_key_id = auth_key_id;
  }

  void set_private_key(const std::shared_ptr<EVP_PKEY> &private_key) {
    m_private_key = private_key;
  }

 private:
  std::string m_host;
  std::string m_auth_key_id;
  std::shared_ptr<EVP_PKEY> m_private_key;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_OCI_SIGNER_H_
