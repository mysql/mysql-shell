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

#ifndef MYSQLSHDK_LIBS_OCI_FIXED_CREDENTIALS_PROVIDER_H_
#define MYSQLSHDK_LIBS_OCI_FIXED_CREDENTIALS_PROVIDER_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/utils/utils_private_key.h"

#include "mysqlshdk/libs/oci/oci_credentials.h"
#include "mysqlshdk/libs/oci/oci_credentials_provider.h"

namespace mysqlshdk {
namespace oci {

/**
 * Provides fixed credentials with no expiration date.
 */
class Fixed_credentials_provider : public Oci_credentials_provider {
 public:
  /**
   * Copies credentials from the given provider, expiration is NOT copied.
   */
  explicit Fixed_credentials_provider(const Oci_credentials_provider &provider);

  /**
   * Copies given credentials, expiration is NOT copied. Other data is copied
   * from the given provider.
   */
  Fixed_credentials_provider(
      const std::shared_ptr<Oci_credentials> &credentials,
      const Oci_credentials_provider &provider);

  Fixed_credentials_provider(const Fixed_credentials_provider &) = delete;
  Fixed_credentials_provider(Fixed_credentials_provider &&) = delete;

  Fixed_credentials_provider &operator=(const Fixed_credentials_provider &) =
      delete;
  Fixed_credentials_provider &operator=(Fixed_credentials_provider &&) = delete;

  void set_auth_key_id(std::string id);

  void set_private_key_id(const shcore::ssl::Private_key_id &id);

  using Oci_credentials_provider::set_region;

  using Oci_credentials_provider::set_tenancy_id;

 private:
  Credentials fetch_credentials() override;

  Credentials m_credentials;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_FIXED_CREDENTIALS_PROVIDER_H_
