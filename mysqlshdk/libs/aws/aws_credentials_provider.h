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

#ifndef MYSQLSHDK_LIBS_AWS_AWS_CREDENTIALS_PROVIDER_H_
#define MYSQLSHDK_LIBS_AWS_AWS_CREDENTIALS_PROVIDER_H_

#include <memory>
#include <optional>
#include <string>

#include "mysqlshdk/libs/aws/aws_credentials.h"

namespace mysqlshdk {
namespace aws {

class Aws_credentials_provider {
 public:
  Aws_credentials_provider(const Aws_credentials_provider &) = delete;
  Aws_credentials_provider(Aws_credentials_provider &&) = default;

  Aws_credentials_provider &operator=(const Aws_credentials_provider &) =
      delete;
  Aws_credentials_provider &operator=(Aws_credentials_provider &&) = default;

  virtual ~Aws_credentials_provider();

  std::shared_ptr<Aws_credentials> credentials() const;

  inline const std::string &name() const noexcept { return m_context.name; }

  inline const char *access_key_id() const noexcept {
    return m_context.access_key_id;
  }

  inline const char *secret_access_key() const noexcept {
    return m_context.secret_access_key;
  }

  bool initialize();

 protected:
  struct Credentials {
    std::optional<std::string> access_key_id;
    std::optional<std::string> secret_access_key;
    std::optional<std::string> session_token;
    Aws_credentials::Time_point expiration = Aws_credentials::NO_EXPIRATION;
  };

  struct Context {
    std::string name;
    const char *access_key_id;
    const char *secret_access_key;
  };

  explicit Aws_credentials_provider(Context context);

 private:
  class Updater;

  virtual Credentials fetch_credentials() = 0;

  std::shared_ptr<Aws_credentials> get_credentials();

  std::shared_ptr<Aws_credentials> m_credentials;
  Context m_context;
  bool m_initialized = false;
  std::unique_ptr<Updater> m_updater;
};

}  // namespace aws
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AWS_AWS_CREDENTIALS_PROVIDER_H_
