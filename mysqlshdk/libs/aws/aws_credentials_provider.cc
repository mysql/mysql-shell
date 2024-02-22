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

#include <chrono>
#include <mutex>
#include <stdexcept>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_time.h"

namespace mysqlshdk {
namespace aws {

class Aws_credentials_provider::Updater final {
 public:
  explicit Updater(Aws_credentials_provider *provider) : m_provider(provider) {}

  Updater(const Updater &) = delete;
  Updater(Updater &&) = delete;

  Updater &operator=(const Updater &) = delete;
  Updater &operator=(Updater &&) = delete;

  ~Updater() = default;

  std::shared_ptr<Aws_credentials> credentials() {
    std::lock_guard lock{m_mutex};

    if (m_provider->m_credentials && m_provider->m_credentials->temporary() &&
        m_provider->m_credentials->expired()) {
      m_provider->m_credentials = m_provider->get_credentials();
    }

    return m_provider->m_credentials;
  }

 private:
  std::mutex m_mutex;
  Aws_credentials_provider *m_provider;
};

Aws_credentials_provider::Aws_credentials_provider(Context context)
    : m_context(std::move(context)) {}

Aws_credentials_provider::~Aws_credentials_provider() = default;

std::shared_ptr<Aws_credentials> Aws_credentials_provider::credentials() const {
  return m_updater ? m_updater->credentials() : m_credentials;
}

bool Aws_credentials_provider::initialize() {
  if (!m_initialized) {
    m_credentials = get_credentials();

    if (m_credentials && m_credentials->temporary()) {
      m_updater = std::make_unique<Updater>(this);
    }

    m_initialized = true;
  }

  return !!credentials();
}

std::shared_ptr<Aws_credentials> Aws_credentials_provider::get_credentials() {
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
    Aws_credentials::Time_point expiration = Aws_credentials::NO_EXPIRATION;

    if (credentials.expiration.has_value()) {
      log_info("The AWS credentials are set to expire at: %s",
               credentials.expiration->c_str());

      try {
        expiration = shcore::rfc3339_to_time_point(*credentials.expiration);
      } catch (const std::exception &e) {
        throw std::runtime_error("Failed to parse 'Expiration' value '" +
                                 *credentials.expiration + "' in " + name());
      }

      // we adjust the expiration time so credentials are refreshed before they
      // actually expire; the minimum duration specified by AWS docs is 15
      // minutes, but we check if we don't end up with a value that's in the
      // past, as some of our tests use values smaller than that
      if (const auto adjusted_expiration = expiration - std::chrono::minutes(5);
          Aws_credentials::Clock::now() < adjusted_expiration) {
        log_info("The expiration of AWS credentials has been adjusted");
        expiration = adjusted_expiration;
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
