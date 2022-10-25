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

#include "mysqlshdk/libs/aws/aws_credentials_provider.h"

#include <stdexcept>
#include <utility>

namespace mysqlshdk {
namespace aws {

Aws_credentials_provider::Aws_credentials_provider(Context context)
    : m_context(std::move(context)) {}

bool Aws_credentials_provider::initialize() {
  m_credentials.reset();

  const auto credentials = fetch_credentials();

  if (credentials.access_key_id.has_value() !=
      credentials.secret_access_key.has_value()) {
    throw std::runtime_error("Partial AWS credentials found in " + name() +
                             ", missing the value of '" +
                             (credentials.access_key_id.has_value()
                                  ? secret_access_key()
                                  : access_key_id()) +
                             "'");
  }

  if (credentials.access_key_id.has_value() &&
      credentials.secret_access_key.has_value()) {
    m_credentials = std::make_shared<Aws_credentials>(
        *credentials.access_key_id, *credentials.secret_access_key,
        credentials.session_token.value_or(std::string{}),
        credentials.expiration);

    // TODO(pawel): the credential refreshing logic should go here, thread
    // should re-fetch the credentials when they expire
  }

  return !!m_credentials;
}

}  // namespace aws
}  // namespace mysqlshdk
