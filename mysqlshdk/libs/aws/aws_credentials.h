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

#ifndef MYSQLSHDK_LIBS_AWS_AWS_CREDENTIALS_H_
#define MYSQLSHDK_LIBS_AWS_AWS_CREDENTIALS_H_

#include <chrono>
#include <ctime>
#include <string>

namespace mysqlshdk {
namespace aws {

class Aws_credentials final {
 public:
  using Clock = std::chrono::system_clock;
  using Time_point = std::chrono::time_point<Clock>;

  static constexpr auto NO_EXPIRATION = Time_point::max();

  Aws_credentials() = delete;

  Aws_credentials(const std::string &access_key_id,
                  const std::string &secret_access_key,
                  const std::string &session_token = {},
                  Time_point expiration = NO_EXPIRATION);

  Aws_credentials(const Aws_credentials &) = default;
  Aws_credentials(Aws_credentials &&) = default;

  Aws_credentials &operator=(const Aws_credentials &) = default;
  Aws_credentials &operator=(Aws_credentials &&) = default;

  ~Aws_credentials() = default;

  bool operator==(const Aws_credentials &creds) const;

  bool operator!=(const Aws_credentials &creds) const;

  inline bool anonymous_access() const noexcept {
    return m_access_key_id.empty() && m_secret_access_key.empty();
  }

  inline bool temporary() const noexcept {
    return NO_EXPIRATION != m_expiration;
  }

  inline bool expired() const noexcept { return expired(Clock::now()); }

  inline bool expired(Time_point now) const noexcept {
    return now > m_expiration;
  }

  inline bool expired(std::time_t now) const noexcept {
    return expired(Clock::from_time_t(now));
  }

  inline Time_point expiration() const noexcept { return m_expiration; }

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
  Time_point m_expiration;
};

}  // namespace aws
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AWS_AWS_CREDENTIALS_H_
