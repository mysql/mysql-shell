/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_OCI_OCI_CREDENTIALS_H_
#define MYSQLSHDK_LIBS_OCI_OCI_CREDENTIALS_H_

#include <string>

#include "mysqlshdk/libs/rest/headers.h"
#include "mysqlshdk/libs/rest/signed/credentials.h"
#include "mysqlshdk/libs/utils/utils_private_key.h"

namespace mysqlshdk {
namespace oci {

/**
 * Minimum expiration time is 5 minutes, we need an expiration threshold smaller
 * than that.
 */
class Oci_credentials final : public rest::Credentials<2> {
 public:
  Oci_credentials() = delete;

  Oci_credentials(std::string auth_key_id,
                  shcore::ssl::Private_key_id private_key_id,
                  Time_point expiration = NO_EXPIRATION,
                  rest::Headers &&extra_headers = {});

  Oci_credentials(const Oci_credentials &) = default;
  Oci_credentials(Oci_credentials &&) = default;

  Oci_credentials &operator=(const Oci_credentials &) = default;
  Oci_credentials &operator=(Oci_credentials &&) = default;

  ~Oci_credentials() override = default;

  bool anonymous_access() const noexcept override { return false; }

  inline const std::string &auth_key_id() const noexcept {
    return m_auth_key_id;
  }

  inline const shcore::ssl::Private_key_id &private_key_id() const noexcept {
    return m_private_key_id;
  }

  inline const shcore::ssl::Private_key &private_key() const noexcept {
    return m_private_key;
  }

  inline const rest::Headers &extra_headers() const noexcept {
    return m_extra_headers;
  }

 private:
  std::string m_auth_key_id;
  shcore::ssl::Private_key_id m_private_key_id;
  shcore::ssl::Private_key m_private_key;
  rest::Headers m_extra_headers;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_OCI_CREDENTIALS_H_
