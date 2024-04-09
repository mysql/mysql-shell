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

#include "mysqlshdk/libs/oci/security_token.h"

#include <utility>

#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace oci {

Security_token::Security_token(std::string token)
    : m_token(std::move(token)), m_jwt(shcore::Jwt::from_string(m_token)) {}

Security_token Security_token::from_json(std::string_view json) {
  const auto parsed = shcore::json::parse_object_or_throw(json);
  return Security_token{shcore::json::required(parsed, "token")};
}

std::string Security_token::auth_key_id() const {
  return shcore::str_format("ST$%s", m_token.c_str());
}

std::time_t Security_token::expiration() const {
  return m_jwt.payload().get_uint("exp");
}

}  // namespace oci
}  // namespace mysqlshdk
