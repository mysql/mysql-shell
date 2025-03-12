/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/aws/container_credentials_provider.h"

#include <chrono>
#include <memory>
#include <stdexcept>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_time.h"

#include "unittest/gtest_clean.h"
#include "unittest/test_utils/cleanup.h"
#include "unittest/test_utils/shell_test_env.h"
#include "unittest/test_utils/test_server.h"

namespace mysqlshdk {
namespace aws {
namespace {

using tests::Cleanup;

class Aws_container_credentials_provider_test : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    s_test_server = std::make_unique<tests::Test_server>();

    if (!s_test_server->start_server(8080, true, false)) {
      // kill the process (if it's there), all test cases will fail with
      // appropriate message
      TearDownTestCase();
    }
  }

  static void TearDownTestCase() {
    if (s_test_server && s_test_server->is_alive()) {
      s_test_server->stop_server();
    }
  }

  void SetUp() override {
    if (!s_test_server->is_alive()) {
      s_test_server->start_server(s_test_server->port(), true, false);
    }

    m_cleanup += clear_aws_env_vars();
  }

  [[nodiscard]] static Cleanup clear_aws_env_vars() {
    Cleanup c;

    c += Cleanup::unset_env_var("AWS_CONTAINER_CREDENTIALS_RELATIVE_URI");
    c += Cleanup::unset_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI");
    c += Cleanup::unset_env_var("AWS_CONTAINER_AUTHORIZATION_TOKEN_FILE");
    c += Cleanup::unset_env_var("AWS_CONTAINER_AUTHORIZATION_TOKEN");

    return c;
  }

  void set_full_uri(const char *access_key, const char *secret_key,
                    const char *expiration) {
    std::string query;

    if (access_key) {
      query += "AccessKeyId";
      query += '=';
      query += access_key;
      query += '&';
    }

    if (secret_key) {
      query += "SecretAccessKey";
      query += '=';
      query += secret_key;
      query += '&';
    }

    if (expiration) {
      query += "Expiration";
      query += '=';
      query += expiration;
      query += '&';
    }

    if (!query.empty()) {
      query.pop_back();
    }

    m_cleanup +=
        Cleanup::set_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI",
                             s_test_server->address() + "/ecs?" + query);
  }

  void set_authorization_token(const char *token) {
    // NOTE: test server sends back the authorization token as the session token
    m_cleanup +=
        Cleanup::set_env_var("AWS_CONTAINER_AUTHORIZATION_TOKEN", token);
  }

  void set_authorization_token_file(const char *token) {
    const std::string path = shcore::get_absolute_path("zażółć");
    m_cleanup += Cleanup::write_file(path, token, true);
    m_cleanup +=
        Cleanup::set_env_var("AWS_CONTAINER_AUTHORIZATION_TOKEN_FILE", path);
  }

  static std::unique_ptr<tests::Test_server> s_test_server;
  Cleanup m_cleanup;
};

std::unique_ptr<tests::Test_server>
    Aws_container_credentials_provider_test::s_test_server;

#define FAIL_IF_NO_SERVER                               \
  if (!s_test_server->is_alive()) {                     \
    FAIL() << "The HTTP test server is not available."; \
  }

TEST_F(Aws_container_credentials_provider_test, no_env_vars) {
  {
    Container_credentials_provider provider;
    // WL15885-TSFR_2_1_3
    EXPECT_EQ("", provider.full_uri());
    EXPECT_EQ(false, provider.initialize());
  }

  {
    const auto c =
        Cleanup::set_env_var("AWS_CONTAINER_AUTHORIZATION_TOKEN", "token");

    EXPECT_EQ(false, Container_credentials_provider{}.initialize());
  }
}

TEST_F(Aws_container_credentials_provider_test, invalid_relative_uri) {
  // WL15885-TSFR_2_1_1
  m_cleanup += Cleanup::set_env_var("AWS_CONTAINER_CREDENTIALS_RELATIVE_URI",
                                    "/?wrong?uri");
  // WL15885-TSFR_2_1_2
  m_cleanup += Cleanup::set_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI",
                                    "http://localhost/notused");

  Container_credentials_provider provider;
  // check if the final URI is correctly constructed
  EXPECT_EQ("http://169.254.170.2/?wrong?uri", provider.full_uri());
  EXPECT_THROW_LIKE(provider.initialize(), mysqlshdk::rest::Credentials_error,
                    "Invalid URI for ECS credentials");
}

TEST_F(Aws_container_credentials_provider_test, invalid_full_uri) {
  {
    const auto c = Cleanup::set_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI",
                                        "http://10.11.12.13/auth");

    Container_credentials_provider provider;
    EXPECT_EQ("http://10.11.12.13/auth", provider.full_uri());
    EXPECT_THROW_LIKE(provider.initialize(), mysqlshdk::rest::Credentials_error,
                      "Unsupported host '10.11.12.13', allowed values are: a "
                      "loopback address, or one of: 169.254.170.2, "
                      "169.254.170.23, fd00:ec2::23, localhost");
  }

  {
    const auto c = Cleanup::set_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI",
                                        "http://localhost/?wrong?uri");

    Container_credentials_provider provider;
    EXPECT_EQ("http://localhost/?wrong?uri", provider.full_uri());
    EXPECT_THROW_LIKE(provider.initialize(), mysqlshdk::rest::Credentials_error,
                      "Invalid URI for ECS credentials");
  }
}

TEST_F(Aws_container_credentials_provider_test, valid_full_uri) {
  // WL15885-TSFR_2_2_1
  {
    const auto c = Cleanup::set_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI",
                                        "http://169.254.170.2/example");

    Container_credentials_provider provider;
    EXPECT_EQ("http://169.254.170.2/example", provider.full_uri());
    EXPECT_THROW_LIKE(provider.initialize(), mysqlshdk::rest::Credentials_error,
                      "Failed to connect", "Connection timed out",
                      "Connection timeout", "Timeout was reached",
                      "Couldn't connect to server");
  }

  {
    const auto c = Cleanup::set_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI",
                                        "http://localhost:12345/a/b/c");

    Container_credentials_provider provider;
    EXPECT_EQ("http://localhost:12345/a/b/c", provider.full_uri());
    EXPECT_THROW_LIKE(provider.initialize(), mysqlshdk::rest::Credentials_error,
                      "Failed to connect", "Connection refused",
                      "Connection timed out");
  }

  {
    const auto c = Cleanup::set_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI",
                                        "http://127.0.0.1:12345");

    Container_credentials_provider provider;
    EXPECT_EQ("http://127.0.0.1:12345", provider.full_uri());
    EXPECT_THROW_LIKE(provider.initialize(), mysqlshdk::rest::Credentials_error,
                      "Failed to connect", "Connection refused",
                      "Connection timed out");
  }

  {
    const auto c = Cleanup::set_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI",
                                        "http://[::1]:12345");

    Container_credentials_provider provider;
    EXPECT_EQ("http://[::1]:12345", provider.full_uri());
    EXPECT_THROW_LIKE(provider.initialize(), mysqlshdk::rest::Credentials_error,
                      "Failed to connect", "Connection refused",
                      "Connection timed out");
  }

  {
    const auto c = Cleanup::set_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI",
                                        "http://169.254.170.23");

    Container_credentials_provider provider;
    EXPECT_EQ("http://169.254.170.23", provider.full_uri());
    EXPECT_THROW_LIKE(provider.initialize(), mysqlshdk::rest::Credentials_error,
                      "Failed to connect", "Connection timed out",
                      "Connection timeout", "Timeout was reached",
                      "Couldn't connect to server");
  }

  {
    const auto c = Cleanup::set_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI",
                                        "http://[fd00:ec2::23]");

    Container_credentials_provider provider;
    EXPECT_EQ("http://[fd00:ec2::23]", provider.full_uri());
    EXPECT_THROW_LIKE(provider.initialize(), mysqlshdk::rest::Credentials_error,
                      "Failed to connect", "Connection timed out",
                      "Connection timeout", "Timeout was reached",
                      "Couldn't connect to server");
  }
}

TEST_F(Aws_container_credentials_provider_test, no_credentials) {
  FAIL_IF_NO_SERVER

  set_full_uri(nullptr, nullptr, nullptr);
  EXPECT_EQ(false, Container_credentials_provider{}.initialize());
}

TEST_F(Aws_container_credentials_provider_test, no_access_key) {
  FAIL_IF_NO_SERVER

  set_full_uri(nullptr, "secret-key", nullptr);

  Container_credentials_provider provider;
  EXPECT_THROW_LIKE(provider.initialize(), std::runtime_error,
                    "Partial AWS credentials found in ECS credentials, missing "
                    "the value of 'AccessKeyId'");
}

TEST_F(Aws_container_credentials_provider_test, invalid_access_key) {
  FAIL_IF_NO_SERVER

  set_full_uri("1", "secret-key", nullptr);

  Container_credentials_provider provider;
  EXPECT_THROW_LIKE(
      provider.initialize(), mysqlshdk::rest::Credentials_error,
      "JSON object should contain a 'AccessKeyId' key with a string value");
}

TEST_F(Aws_container_credentials_provider_test, no_secret_key) {
  FAIL_IF_NO_SERVER

  set_full_uri("access-key", nullptr, nullptr);

  Container_credentials_provider provider;
  EXPECT_THROW_LIKE(provider.initialize(), std::runtime_error,
                    "Partial AWS credentials found in ECS credentials, missing "
                    "the value of 'SecretAccessKey'");
}

TEST_F(Aws_container_credentials_provider_test, invalid_secret_key) {
  FAIL_IF_NO_SERVER

  set_full_uri("access-key", "1", nullptr);

  Container_credentials_provider provider;
  EXPECT_THROW_LIKE(
      provider.initialize(), mysqlshdk::rest::Credentials_error,
      "JSON object should contain a 'SecretAccessKey' key with a string value");
}

TEST_F(Aws_container_credentials_provider_test, no_expiration) {
  FAIL_IF_NO_SERVER

  set_full_uri("access-key", "secret-key", nullptr);

  Container_credentials_provider provider;

  ASSERT_TRUE(provider.initialize());

  EXPECT_EQ("access-key", provider.credentials()->access_key_id());
  EXPECT_EQ("secret-key", provider.credentials()->secret_access_key());
  EXPECT_EQ("", provider.credentials()->session_token());
  EXPECT_EQ(Aws_credentials::NO_EXPIRATION,
            provider.credentials()->expiration());
}

TEST_F(Aws_container_credentials_provider_test, invalid_expiration_type) {
  FAIL_IF_NO_SERVER

  set_full_uri("access-key", "secret-key", "1");

  Container_credentials_provider provider;
  EXPECT_THROW_LIKE(
      provider.initialize(), mysqlshdk::rest::Credentials_error,
      "JSON object should contain a 'Expiration' key with a string value");
}

TEST_F(Aws_container_credentials_provider_test, invalid_expiration_value) {
  FAIL_IF_NO_SERVER

  set_full_uri("access-key", "secret-key", "2023-04-12");

  Container_credentials_provider provider;
  EXPECT_THROW_LIKE(
      provider.initialize(), std::runtime_error,
      "Failed to parse 'Expiration' value '2023-04-12' in ECS credentials");
}

TEST_F(Aws_container_credentials_provider_test, valid_expiration_value) {
  FAIL_IF_NO_SERVER

  const auto expiration = shcore::future_time_rfc3339(std::chrono::hours(1));
  set_full_uri("access-key", "secret-key", expiration.c_str());

  Container_credentials_provider provider;

  ASSERT_TRUE(provider.initialize());

  EXPECT_EQ("access-key", provider.credentials()->access_key_id());
  EXPECT_EQ("secret-key", provider.credentials()->secret_access_key());
  EXPECT_EQ("", provider.credentials()->session_token());
  EXPECT_EQ(shcore::rfc3339_to_time_point(expiration),
            provider.credentials()->expiration());
}

TEST_F(Aws_container_credentials_provider_test, invalid_session_token) {
  FAIL_IF_NO_SERVER

  set_full_uri("access-key", "secret-key", nullptr);
  set_authorization_token("1");

  Container_credentials_provider provider;
  EXPECT_THROW_LIKE(
      provider.initialize(), mysqlshdk::rest::Credentials_error,
      "JSON object should contain a 'Token' key with a string value");
}

TEST_F(Aws_container_credentials_provider_test, invalid_session_token_string) {
  set_full_uri("access-key", "secret-key", nullptr);
  set_authorization_token("a\rb");

  Container_credentials_provider provider;
  EXPECT_THROW_LIKE(
      provider.initialize(), mysqlshdk::rest::Credentials_error,
      "The ECS authorization token contains disallowed newline character");
}

TEST_F(Aws_container_credentials_provider_test, valid_session_token) {
  FAIL_IF_NO_SERVER

  // WL15885-TSFR_2_4_1
  set_full_uri("access-key", "secret-key", nullptr);
  set_authorization_token("authorization-token");

  Container_credentials_provider provider;

  ASSERT_TRUE(provider.initialize());

  EXPECT_EQ("access-key", provider.credentials()->access_key_id());
  EXPECT_EQ("secret-key", provider.credentials()->secret_access_key());
  EXPECT_EQ("authorization-token", provider.credentials()->session_token());
  EXPECT_EQ(Aws_credentials::NO_EXPIRATION,
            provider.credentials()->expiration());
}

TEST_F(Aws_container_credentials_provider_test, missing_session_token_file) {
  set_full_uri("access-key", "secret-key", nullptr);
  m_cleanup += Cleanup::set_env_var("AWS_CONTAINER_AUTHORIZATION_TOKEN_FILE",
                                    "/missing/file");

  Container_credentials_provider provider;
  EXPECT_THROW_LIKE(provider.initialize(), mysqlshdk::rest::Credentials_error,
                    "Failed to read authorization token file: ");
}

TEST_F(Aws_container_credentials_provider_test, invalid_session_token_file) {
  set_full_uri("access-key", "secret-key", nullptr);
  set_authorization_token_file("a\nb");

  Container_credentials_provider provider;
  EXPECT_THROW_LIKE(
      provider.initialize(), mysqlshdk::rest::Credentials_error,
      "The ECS authorization token contains disallowed newline character");
}

TEST_F(Aws_container_credentials_provider_test, valid_session_token_file) {
  FAIL_IF_NO_SERVER

  set_full_uri("access-key", "secret-key", nullptr);
  set_authorization_token_file("authorization-token");

  Container_credentials_provider provider;

  ASSERT_TRUE(provider.initialize());

  EXPECT_EQ("access-key", provider.credentials()->access_key_id());
  EXPECT_EQ("secret-key", provider.credentials()->secret_access_key());
  EXPECT_EQ("authorization-token", provider.credentials()->session_token());
  EXPECT_EQ(Aws_credentials::NO_EXPIRATION,
            provider.credentials()->expiration());
}

TEST_F(Aws_container_credentials_provider_test, session_token_priority) {
  FAIL_IF_NO_SERVER

  set_full_uri("access-key", "secret-key", nullptr);
  // file has higher priority
  set_authorization_token_file("authorization-token");
  set_authorization_token("unused-token");

  Container_credentials_provider provider;

  ASSERT_TRUE(provider.initialize());

  EXPECT_EQ("access-key", provider.credentials()->access_key_id());
  EXPECT_EQ("secret-key", provider.credentials()->secret_access_key());
  EXPECT_EQ("authorization-token", provider.credentials()->session_token());
  EXPECT_EQ(Aws_credentials::NO_EXPIRATION,
            provider.credentials()->expiration());
}

}  // namespace
}  // namespace aws
}  // namespace mysqlshdk
