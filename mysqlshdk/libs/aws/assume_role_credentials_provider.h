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

#ifndef MYSQLSHDK_LIBS_AWS_ASSUME_ROLE_CREDENTIALS_PROVIDER_H_
#define MYSQLSHDK_LIBS_AWS_ASSUME_ROLE_CREDENTIALS_PROVIDER_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/aws/aws_credentials_provider.h"
#include "mysqlshdk/libs/aws/aws_credentials_resolver.h"
#include "mysqlshdk/libs/aws/aws_settings.h"
#include "mysqlshdk/libs/aws/sts_client.h"

namespace mysqlshdk {
namespace aws {

/**
 * Provider which allows to assume a role to gain additional (or different)
 * permissions, or get permissions to perform actions in a different AWS
 * account.
 */
class Assume_role_credentials_provider : public Aws_credentials_provider {
 public:
  Assume_role_credentials_provider(const Settings &settings,
                                   const std::string &profile);

  Assume_role_credentials_provider(const Assume_role_credentials_provider &) =
      delete;
  Assume_role_credentials_provider(Assume_role_credentials_provider &&) =
      delete;

  Assume_role_credentials_provider &operator=(
      const Assume_role_credentials_provider &) = delete;
  Assume_role_credentials_provider &operator=(
      Assume_role_credentials_provider &&) = delete;

  ~Assume_role_credentials_provider() override = default;

  bool available() const noexcept override;

 private:
  Credentials fetch_credentials() override;

  void handle_source_profile(const Profile *profile, const Settings &settings);

  void handle_credential_source(const Profile *profile,
                                const Settings &settings);

  void initialize_sts();

  Credentials assume_role();

  std::string m_region;
  const Profile *m_profile = nullptr;
  Aws_credentials_resolver m_resolver;
  std::unique_ptr<Aws_credentials_provider> m_provider;
  std::unique_ptr<Sts_client> m_sts;
};

}  // namespace aws
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AWS_ASSUME_ROLE_CREDENTIALS_PROVIDER_H_
