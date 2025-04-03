/*
 * Copyright (c) 2022, 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_AWS_AWS_CREDENTIALS_PROVIDER_H_
#define MYSQLSHDK_LIBS_AWS_AWS_CREDENTIALS_PROVIDER_H_

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "mysqlshdk/libs/rest/signed/credentials_provider.h"
#include "mysqlshdk/libs/utils/utils_json.h"

#include "mysqlshdk/libs/aws/aws_credentials.h"

namespace mysqlshdk {
namespace aws {

class Aws_credentials_provider;

struct Aws_credentials_provider_traits {
  using Provider_t = Aws_credentials_provider;
  using Credentials_t = Aws_credentials;

  struct Intermediate_credentials {
    std::optional<std::string> access_key_id;
    std::optional<std::string> secret_access_key;
    std::optional<std::string> session_token;
    std::optional<std::string> expiration;
  };

  static std::shared_ptr<Credentials_t> convert(
      const Provider_t &self, Intermediate_credentials &&credentials);
};

class Aws_credentials_provider
    : public rest::Credentials_provider<Aws_credentials_provider_traits> {
 public:
  static constexpr auto s_storage_name = "AWS";

  Aws_credentials_provider(const Aws_credentials_provider &) = delete;
  Aws_credentials_provider(Aws_credentials_provider &&) = delete;

  Aws_credentials_provider &operator=(const Aws_credentials_provider &) =
      delete;
  Aws_credentials_provider &operator=(Aws_credentials_provider &&) = delete;

  inline const char *access_key_id() const noexcept {
    return m_context.access_key_id;
  }

  inline const char *secret_access_key() const noexcept {
    return m_context.secret_access_key;
  }

 protected:
  struct Context {
    std::string name;
    const char *access_key_id;
    const char *secret_access_key;
  };

  explicit Aws_credentials_provider(Context context);

  /**
   * Parses a JSON string with credentials.
   *
   * @param json String to parse.
   * @param token_key Name of the key holding a value of session token.
   * @param required How many of the credential values are required (order as in
   *                 the Credentials structure).
   * @param validation Extra validation.
   *
   * @returns Parsed credentials.
   */
  Credentials parse_json_credentials(
      const std::string &json, const char *token_key, int required = 0,
      std::function<void(const shcore::json::JSON &)> validation = {}) const;

 private:
  Context m_context;
};

}  // namespace aws
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AWS_AWS_CREDENTIALS_PROVIDER_H_
