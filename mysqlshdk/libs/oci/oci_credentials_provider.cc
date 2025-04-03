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

#include "mysqlshdk/libs/oci/oci_credentials_provider.h"

#include <cassert>
#include <chrono>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_private_key.h"

namespace mysqlshdk {
namespace oci {

Oci_credentials_provider::Oci_credentials_provider(std::string name)
    : Credentials_provider(std::move(name)) {}

void Oci_credentials_provider::set_region(std::string region) {
  m_region = std::move(region);
}

void Oci_credentials_provider::set_tenancy_id(std::string tenancy_id) {
  m_tenancy_id = std::move(tenancy_id);
}

Oci_credentials_provider::Credentials_ptr_t
Oci_credentials_provider_traits::convert(
    const Oci_credentials_provider &self,
    Intermediate_credentials &&credentials) {
  // ensure that these are set
  assert(!credentials.auth_key_id.empty());
  assert(!credentials.private_key_id.empty());

  auto expiration = Oci_credentials::NO_EXPIRATION;

  if (credentials.expiration.has_value()) {
    const auto exp = *credentials.expiration;
    expiration = Oci_credentials::Clock::from_time_t(exp);

    const auto exp_str = mysqlshdk::utils::fmttime(
        "%a, %d %b %Y %H:%M:%S GMT", mysqlshdk::utils::Time_type::GMT, &exp);
    log_info(
        "The OCI credentials fetched by %s provider are set to expire at: %s",
        self.name().c_str(), exp_str.c_str());
  }

  return std::make_shared<Oci_credentials>(
      std::move(credentials.auth_key_id),
      shcore::ssl::Private_key_id{std::move(credentials.private_key_id)},
      std::move(expiration), std::move(credentials.extra_headers));
}

}  // namespace oci
}  // namespace mysqlshdk
