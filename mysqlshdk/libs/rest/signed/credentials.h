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

#ifndef MYSQLSHDK_LIBS_REST_SIGNED_CREDENTIALS_H_
#define MYSQLSHDK_LIBS_REST_SIGNED_CREDENTIALS_H_

#include <chrono>
#include <ctime>

namespace mysqlshdk {
namespace rest {

/**
 * Base class which represents credentials used to sign a request.
 */
class Credentials {
 public:
  using Clock = std::chrono::system_clock;
  using Time_point = std::chrono::time_point<Clock>;

  /**
   * Credentials that do not expire.
   */
  static constexpr auto NO_EXPIRATION = Time_point::max();

  /**
   * Creates credentials which do not expire.
   */
  Credentials() : Credentials(NO_EXPIRATION) {}

  /**
   * Creates credentials which expire at a given point in time.
   *
   * @param expiration When credentials are going to expire.
   */
  explicit Credentials(Time_point expiration) : m_expiration(expiration) {
    if (NO_EXPIRATION == m_expiration) {
      m_use_threshold = false;
    } else {
      // we adjust the expiration time so credentials are refreshed before they
      // actually expire; we check if we don't end up with a value that's in the
      // past, as some of our tests use small values
      m_use_threshold = Clock::now() + k_expiration_threshold < m_expiration;
    }
  }

  /**
   * Creates credentials which expire in X seconds. If expires_in is zero,
   * credentials do not expire.
   *
   * @param expires_in When credentials are going to expire.
   */
  explicit Credentials(std::chrono::seconds expires_in)
      : Credentials(expires_in > std::chrono::seconds::zero()
                        ? Clock::now() + expires_in
                        : NO_EXPIRATION) {}

  Credentials(const Credentials &) = default;
  Credentials(Credentials &&) = default;

  Credentials &operator=(const Credentials &) = default;
  Credentials &operator=(Credentials &&) = default;

  virtual ~Credentials() = default;

  /**
   * Whether these credentials allow for an anonymous access.
   */
  virtual bool anonymous_access() const noexcept = 0;

  /**
   * Checks whether these credentials are temporary (will expire in some time).
   */
  inline bool temporary() const noexcept {
    return NO_EXPIRATION != m_expiration;
  }

  /**
   * Checks if credentials have expired.
   */
  inline bool expired() const noexcept { return expired(Clock::now()); }

  /**
   * Checks if credentials have expired.
   *
   * @param now Current time.
   */
  inline bool expired(Time_point now) const noexcept {
    return now > m_expiration - (m_use_threshold ? k_expiration_threshold
                                                 : k_no_expiration_threshold);
  }

  /**
   * Checks if credentials have expired.
   *
   * @param now Current time.
   */
  inline bool expired(std::time_t now) const noexcept {
    return expired(Clock::from_time_t(now));
  }

  /**
   * Provides the expiration time.
   */
  inline Time_point expiration() const noexcept { return m_expiration; }

 private:
  static constexpr auto k_expiration_threshold = std::chrono::minutes(5);
  static constexpr std::chrono::minutes k_no_expiration_threshold{};

  Time_point m_expiration;
  bool m_use_threshold;
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_SIGNED_CREDENTIALS_H_
