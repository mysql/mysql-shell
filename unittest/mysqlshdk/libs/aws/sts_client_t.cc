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

#include "unittest/gprod_clean.h"

#include "mysqlshdk/libs/aws/sts_client.h"

#include "unittest/gtest_clean.h"

#include "mysqlshdk/libs/aws/aws_credentials_provider.h"
#include "mysqlshdk/libs/aws/aws_credentials_resolver.h"
#include "mysqlshdk/libs/aws/aws_settings.h"

#include "unittest/mysqlshdk/libs/aws/aws_tests.h"
#include "unittest/test_utils/cleanup.h"

namespace mysqlshdk {
namespace aws {
namespace {

using tests::Cleanup;

class Aws_sts_client_test : public Aws_tests {
 protected:
  void SetUp() override {
    Aws_tests::SetUp();

    if (!m_credentials_file.empty()) {
      m_cleanup += Cleanup::set_env_var("AWS_SHARED_CREDENTIALS_FILE",
                                        m_credentials_file);
    }

    if (!m_config_file.empty()) {
      m_cleanup += Cleanup::set_env_var("AWS_CONFIG_FILE", m_config_file);
    }

    if (!m_profile.empty()) {
      m_cleanup += Cleanup::set_env_var("AWS_PROFILE", m_profile);
    }

    if (!m_region.empty()) {
      m_cleanup += Cleanup::set_env_var("AWS_REGION", m_region);
    }

    m_settings.initialize();
  }

 protected:
  std::unique_ptr<Aws_credentials_provider> provider() {
    Aws_credentials_resolver resolver;
    resolver.add_profile_providers(m_settings,
                                   m_settings.get_string(Setting::PROFILE));

    return resolver.resolve();
  }

  Settings m_settings;

 private:
  Cleanup m_cleanup;
};

TEST_F(Aws_sts_client_test, assume_role) {
  if (m_role.empty()) {
    skip("This test uses an AWS role which is not available");
  }

  SKIP_IF_NO_AWS_CONFIGURATION;

  const auto p = provider();
  Sts_client client{m_settings.get_string(Setting::REGION), p.get()};

  Assume_role_request request;
  request.arn = m_role;
  request.session_name = "test-session-name";
  request.duration_seconds = 900;
  request.external_id = "some-id";
  EXPECT_NO_THROW(client.assume_role(request));
}

}  // namespace
}  // namespace aws
}  // namespace mysqlshdk
