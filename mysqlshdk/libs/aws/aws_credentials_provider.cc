/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/aws/aws_credentials_provider.h"

#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_time.h"

namespace mysqlshdk {
namespace aws {

Aws_credentials_provider::Aws_credentials_provider(Context context)
    : Credentials_provider(std::move(context.name)),
      m_context(std::move(context)) {}

Aws_credentials_provider::Credentials_ptr_t
Aws_credentials_provider_traits::convert(
    const Aws_credentials_provider &self,
    const Intermediate_credentials &credentials) {
  if (credentials.access_key_id.has_value() !=
      credentials.secret_access_key.has_value()) {
    throw std::runtime_error("Partial AWS credentials found in " + self.name() +
                             ", missing the value of '" +
                             (credentials.access_key_id.has_value()
                                  ? self.secret_access_key()
                                  : self.access_key_id()) +
                             "'");
  }

  if (credentials.access_key_id.has_value() &&
      credentials.secret_access_key.has_value()) {
    auto expiration = Aws_credentials::NO_EXPIRATION;

    if (credentials.expiration.has_value()) {
      log_info("The AWS credentials are set to expire at: %s",
               credentials.expiration->c_str());

      try {
        expiration = shcore::rfc3339_to_time_point(*credentials.expiration);
      } catch (const std::exception &e) {
        throw std::runtime_error("Failed to parse 'Expiration' value '" +
                                 *credentials.expiration + "' in " +
                                 self.name());
      }
    }

    return std::make_shared<Aws_credentials>(
        *credentials.access_key_id, *credentials.secret_access_key,
        credentials.session_token.value_or(std::string{}), expiration);
  }

  return {};
}

}  // namespace aws
}  // namespace mysqlshdk
