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

#ifndef MYSQLSHDK_LIBS_REST_SIGNED_CREDENTIALS_PROVIDER_H_
#define MYSQLSHDK_LIBS_REST_SIGNED_CREDENTIALS_PROVIDER_H_

#include <cassert>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlshdk {
namespace rest {

/**
 * Fetches the credentials (if they are available), updates them when needed.
 *
 * Traits is expected to contain:
 *  - Provider_t - type of the provider
 *  - Credentials_t - type of the credentials provided
 *  - Intermediate_credentials - a structure to store the intermediate
 *    credentials
 *  - static std::shared_ptr<Credentials_t> convert(
 *        const Provider_t &self, const Intermediate_credentials &credentials) -
 *      a method to convert intermediate credentials into the final ones
 *
 * Traits::Provider_t is expected to contain:
 *  - s_storage_name - name of the storage which this provider handles
 */
template <typename Traits>
class Credentials_provider {
 public:
  using Provider_t = Traits::Provider_t;
  using Credentials_t = Traits::Credentials_t;
  using Credentials_ptr_t = std::shared_ptr<Credentials_t>;

  Credentials_provider(const Credentials_provider &) = delete;
  Credentials_provider(Credentials_provider &&) = delete;

  Credentials_provider &operator=(const Credentials_provider &) = delete;
  Credentials_provider &operator=(Credentials_provider &&) = delete;

  virtual ~Credentials_provider() = default;

  /**
   * Provides name of this provider.
   */
  inline const std::string &name() const noexcept { return m_name; }

  /**
   * Initializes the provider, needs to be called once before this provider is
   * used.
   *
   * @returns true If initialization was successful.
   */
  bool initialize() {
    if (!m_initialize_called) {
      m_credentials = get_credentials();

      if (m_credentials) {
        m_temporary = m_credentials->temporary();
      }

      // we don't want to try again even if we got no credentials, another try
      // will also fail
      m_initialize_called = true;
    }

    return !!m_credentials;
  }

  /**
   * Gets credentials fetched by this provider.
   *
   * @returns Fetched credentials, or nullptr.
   */
  Credentials_ptr_t credentials() const {
    assert(m_initialize_called);

    if (m_temporary) {
      const_cast<Credentials_provider *>(this)->maybe_refresh();
    }

    return m_credentials;
  }

  /**
   * Whether this provider is able to fetch credentials.
   */
  virtual bool available() const noexcept = 0;

 protected:
  using Credentials = Traits::Intermediate_credentials;

  /**
   * Creates a provider with the given name.
   *
   * @param name Name of the provider.
   */
  explicit Credentials_provider(std::string name) : m_name(std::move(name)) {}

 private:
  /**
   * Fetches the credentials in their intermediate form.
   */
  virtual Credentials fetch_credentials() = 0;

  Credentials_ptr_t get_credentials() {
    if (!available()) {
      return {};
    }

    Credentials credentials;

    try {
      credentials = fetch_credentials();
    } catch (const std::exception &e) {
      log_error("Failed to fetch %s credentials using provider: %s, error: %s",
                Provider_t::s_storage_name, name().c_str(), e.what());
      throw;
    }

    return Traits::convert(*static_cast<Provider_t *>(this), credentials);
  }

  void maybe_refresh() {
    std::lock_guard lock{m_update_mutex};

    if (m_credentials->expired()) {
      m_credentials = get_credentials();
    }
  }

  std::string m_name;
  bool m_initialize_called = false;

  Credentials_ptr_t m_credentials;
  bool m_temporary = false;
  std::mutex m_update_mutex;
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_SIGNED_CREDENTIALS_PROVIDER_H_
