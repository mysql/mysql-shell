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

#ifndef MYSQLSHDK_LIBS_AWS_AWS_CREDENTIALS_H_
#define MYSQLSHDK_LIBS_AWS_AWS_CREDENTIALS_H_

#include <string>

#include "mysqlshdk/libs/rest/signed/credentials.h"

namespace mysqlshdk {
namespace aws {

class Aws_credentials final : public rest::Credentials {
 public:
  Aws_credentials() = delete;

  Aws_credentials(std::string access_key_id, std::string secret_access_key,
                  std::string session_token = {},
                  Time_point expiration = NO_EXPIRATION);

  Aws_credentials(const Aws_credentials &) = default;
  Aws_credentials(Aws_credentials &&) = default;

  Aws_credentials &operator=(const Aws_credentials &) = default;
  Aws_credentials &operator=(Aws_credentials &&) = default;

  ~Aws_credentials() override = default;

  bool anonymous_access() const noexcept override {
    return m_access_key_id.empty() && m_secret_access_key.empty();
  }

  inline const std::string &access_key_id() const noexcept {
    return m_access_key_id;
  }

  inline const std::string &secret_access_key() const noexcept {
    return m_secret_access_key;
  }

  inline const std::string &session_token() const noexcept {
    return m_session_token;
  }

 private:
  std::string m_access_key_id;
  std::string m_secret_access_key;
  std::string m_session_token;
};

}  // namespace aws
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AWS_AWS_CREDENTIALS_H_
