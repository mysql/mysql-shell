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

#ifndef MYSQLSHDK_LIBS_REST_SIGNED_CREDENTIALS_RESOLVER_H_
#define MYSQLSHDK_LIBS_REST_SIGNED_CREDENTIALS_RESOLVER_H_

#include <list>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace rest {

/**
 * Given a list of credential providers, finds the first one which returns valid
 * credentials.
 */
template <typename Provider>
class Credentials_resolver {
 public:
  Credentials_resolver() = delete;

  Credentials_resolver(const Credentials_resolver &) = delete;
  Credentials_resolver(Credentials_resolver &&) = default;

  Credentials_resolver &operator=(const Credentials_resolver &) = delete;
  Credentials_resolver &operator=(Credentials_resolver &&) = default;

  ~Credentials_resolver() = default;

  /**
   * Adds a new credential provider to the list.
   *
   * @param provider Credential provider.
   */
  void add(std::unique_ptr<Provider> provider) {
    m_providers.emplace_back(std::move(provider));
  }

  /**
   * Finds a credential provider which has valid credentials.
   *
   * @returns Credential provider.
   * @throws std::runtime_error If a valid credential provider cannot be found.
   */
  std::unique_ptr<Provider> resolve() {
    // remove providers which cannot be used
    for (auto it = m_providers.begin(); it != m_providers.end();) {
      if (!(*it)->available()) {
        it = m_providers.erase(it);
      } else {
        ++it;
      }
    }

    if (m_providers.empty()) {
      throw std::runtime_error(
          shcore::str_format("Could not select the %s credentials provider, "
                             "please see log for more details",
                             Provider::s_storage_name));
    }

    for (auto &provider : m_providers) {
      if (provider->initialize()) {
        log_info("Selected %s credentials provider: %s",
                 Provider::s_storage_name, provider->name().c_str());
        return std::move(provider);
      }
    }

    throw std::runtime_error(shcore::str_format(
        "%s, tried: %s", m_resolve_error_msg,
        shcore::str_join(m_providers, ", ", [](const auto &p) {
          return p->name();
        }).c_str()));
  }

 protected:
  /**
   * Creates the resolver.
   *
   * @param resolve_error_msg Error message which is reported if a valid
   *        provider was not found.
   */
  explicit Credentials_resolver(const char *resolve_error_msg)
      : m_resolve_error_msg(resolve_error_msg) {}

 private:
  std::list<std::unique_ptr<Provider>> m_providers;
  const char *m_resolve_error_msg;
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_SIGNED_CREDENTIALS_RESOLVER_H_
