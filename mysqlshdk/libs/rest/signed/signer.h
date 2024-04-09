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

#ifndef MYSQLSHDK_LIBS_REST_SIGNED_SIGNER_H_
#define MYSQLSHDK_LIBS_REST_SIGNED_SIGNER_H_

#include <ctime>
#include <memory>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/rest/headers.h"
#include "mysqlshdk/libs/rest/response.h"
#include "mysqlshdk/libs/rest/signed/signed_request.h"

namespace mysqlshdk {
namespace rest {

class ISigner {
 public:
  ISigner() = default;

  ISigner(const ISigner &) = default;
  ISigner(ISigner &&) = default;

  ISigner &operator=(const ISigner &) = default;
  ISigner &operator=(ISigner &&) = default;

  virtual ~ISigner() = default;

  virtual void initialize() {}

  virtual bool should_sign_request(const Signed_request &request) const = 0;

  virtual Headers sign_request(const Signed_request &request,
                               time_t now) const = 0;

  virtual bool refresh_auth_data() = 0;

  virtual bool auth_data_expired(time_t now) const = 0;

  virtual bool is_authorization_error(const Signed_request &,
                                      const Response &response) const {
    return Response::Status_code::UNAUTHORIZED == response.status;
  }
};

template <typename Provider>
class Signer_config {
 public:
  Signer_config() = default;

  Signer_config(const Signer_config &) = default;
  Signer_config(Signer_config &&) = default;

  Signer_config &operator=(const Signer_config &) = default;
  Signer_config &operator=(Signer_config &&) = default;

  virtual ~Signer_config() = default;

  virtual Provider *credentials_provider() const = 0;
};

template <typename Provider>
class Signer : public ISigner {
 public:
  Signer() = delete;

  explicit Signer(const Signer_config<Provider> &config)
      : m_credentials_provider(config.credentials_provider()) {}

  Signer(const Signer &) = default;
  Signer(Signer &&) = default;

  Signer &operator=(const Signer &) = default;
  Signer &operator=(Signer &&) = default;

  ~Signer() override = default;

  void initialize() override { update_credentials(); }

  bool should_sign_request(const Signed_request &) const override {
    return !m_credentials->anonymous_access();
  }

  bool refresh_auth_data() override { return update_credentials(); }

  bool auth_data_expired(time_t now) const override {
    return m_credentials->expired(now);
  }

 protected:
  Provider::Credentials_ptr_t m_credentials;

 private:
  virtual void on_credentials_set() = 0;

  bool update_credentials() {
    if (auto credentials = m_credentials_provider->credentials()) {
      if (!m_credentials || credentials.get() != m_credentials.get() ||
          credentials->expiration() != m_credentials->expiration()) {
        if (m_credentials) {
          log_info("The %s credentials have been refreshed.",
                   Provider::s_storage_name);
        }

        set_credentials(std::move(credentials));

        return true;
      }
    } else {
      throw std::runtime_error(shcore::str_format(
          "Failed to update the %s credentials", Provider::s_storage_name));
    }

    return false;
  }

  void set_credentials(Provider::Credentials_ptr_t credentials) {
    m_credentials = std::move(credentials);
    on_credentials_set();
  }

  Provider *m_credentials_provider;
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_SIGNED_SIGNER_H_
