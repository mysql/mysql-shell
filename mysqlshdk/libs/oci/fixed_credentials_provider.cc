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

#include "mysqlshdk/libs/oci/fixed_credentials_provider.h"

#include <cassert>
#include <utility>

namespace mysqlshdk {
namespace oci {

Fixed_credentials_provider::Fixed_credentials_provider(
    const Oci_credentials_provider &provider)
    : Fixed_credentials_provider(get_current_credentials(provider), provider) {}

Fixed_credentials_provider::Fixed_credentials_provider(
    const std::shared_ptr<Oci_credentials> &credentials,
    const Oci_credentials_provider &provider)
    : Oci_credentials_provider{"fixed credentials"} {
  assert(credentials);

  set_auth_key_id(credentials->auth_key_id());
  set_private_key_id(credentials->private_key_id());

  set_region(provider.region());
  set_tenancy_id(provider.tenancy_id());
}

void Fixed_credentials_provider::set_auth_key_id(std::string id) {
  m_credentials.auth_key_id = std::move(id);
}

void Fixed_credentials_provider::set_private_key_id(
    const shcore::ssl::Private_key_id &id) {
  m_credentials.private_key_id = id.id();
}

Fixed_credentials_provider::Credentials
Fixed_credentials_provider::fetch_credentials() {
  return m_credentials;
}

}  // namespace oci
}  // namespace mysqlshdk
