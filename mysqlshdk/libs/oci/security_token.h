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

#ifndef MYSQLSHDK_LIBS_OCI_SECURITY_TOKEN_H_
#define MYSQLSHDK_LIBS_OCI_SECURITY_TOKEN_H_

#include <ctime>
#include <string>

#include "mysqlshdk/libs/utils/utils_jwt.h"

namespace mysqlshdk {
namespace oci {

class Security_token final {
 public:
  Security_token() = default;

  explicit Security_token(std::string token);

  Security_token(const Security_token &) = delete;
  Security_token(Security_token &&) = default;

  Security_token &operator=(const Security_token &) = delete;
  Security_token &operator=(Security_token &&) = default;

  static Security_token from_json(std::string_view json);

  inline const std::string &to_string() const noexcept { return m_token; }

  inline const shcore::Jwt &jwt() const noexcept { return m_jwt; }

  std::string auth_key_id() const;

  std::time_t expiration() const;

 private:
  std::string m_token;
  shcore::Jwt m_jwt;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_SECURITY_TOKEN_H_
