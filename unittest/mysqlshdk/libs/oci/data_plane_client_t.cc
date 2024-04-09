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

#include "mysqlshdk/libs/oci/data_plane_client.h"

#include <memory>

#include "unittest/gtest_clean.h"
#include "unittest/mysqlshdk/libs/oci/oci_tests.h"
#include "unittest/test_utils.h"

#include "mysqlshdk/libs/oci/api_key_credentials_provider.h"
#include "mysqlshdk/libs/oci/fixed_credentials_provider.h"
#include "mysqlshdk/libs/oci/security_token.h"

namespace mysqlshdk {
namespace oci {
namespace {

class Oci_data_plane_client_tests : public testing::Oci_tests {
 protected:
  void SetUp() override {
    Oci_tests::SetUp();

    if (!should_skip()) {
      m_provider = std::make_unique<Api_key_credentials_provider>(
          m_oci_config_file, "DEFAULT");
      m_provider->initialize();
    }
  }

  std::unique_ptr<Api_key_credentials_provider> m_provider;
};

TEST_F(Oci_data_plane_client_tests, generate_user_security_token) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  Data_plane_client client{m_provider.get()};
  const auto public_key = m_provider->credentials()->private_key().public_key();

  EXPECT_NO_THROW(
      client.generate_user_security_token({public_key.to_string()}));

  EXPECT_NO_THROW(
      client.generate_user_security_token({public_key.to_string(), 10}));
}

TEST_F(Oci_data_plane_client_tests, refresh_user_security_token) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  Data_plane_client client{m_provider.get()};
  const auto credentials = m_provider->credentials();
  const auto public_key = credentials->private_key().public_key();
  const auto token =
      client.generate_user_security_token({public_key.to_string()});

  // in order to refresh the token, request needs to be signed using that token
  Fixed_credentials_provider refresh_credentials{
      std::make_shared<Oci_credentials>(token.auth_key_id(),
                                        credentials->private_key_id()),
      *m_provider};
  refresh_credentials.initialize();
  Data_plane_client refresh_client{&refresh_credentials};

  EXPECT_NO_THROW(refresh_client.refresh_user_security_token(token));
}

}  // namespace
}  // namespace oci
}  // namespace mysqlshdk
