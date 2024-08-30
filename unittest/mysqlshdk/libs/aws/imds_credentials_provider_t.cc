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

#include "mysqlshdk/libs/aws/imds_credentials_provider.h"

#include <chrono>
#include <memory>
#include <stdexcept>
#include <unordered_map>

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

class Aws_imds_credentials_provider_test : public ::testing::Test {
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
    m_cleanup.add([this]() {
      if (shcore::is_file(m_config_file)) {
        shcore::delete_file(m_config_file);
      }
    });

    m_expiration = shcore::future_time_rfc3339(std::chrono::hours(1));
  }

  [[nodiscard]] static Cleanup clear_aws_env_vars() {
    Cleanup c;

    c += Cleanup::unset_env_var("AWS_EC2_METADATA_DISABLED");
    c += Cleanup::unset_env_var("AWS_EC2_METADATA_SERVICE_ENDPOINT");
    c += Cleanup::unset_env_var("AWS_EC2_METADATA_SERVICE_ENDPOINT_MODE");
    c += Cleanup::unset_env_var("AWS_METADATA_SERVICE_TIMEOUT");
    c += Cleanup::unset_env_var("AWS_METADATA_SERVICE_NUM_ATTEMPTS");

    return c;
  }

  void set_endpoint(const char *access_key, const char *secret_key,
                    const char *session_token, const char *expiration,
                    bool use_token = true) {
    std::string full_path;

    if (access_key) {
      full_path += "AccessKeyId";
      full_path += '/';
      full_path += access_key;
      full_path += '/';
    }

    if (secret_key) {
      full_path += "SecretAccessKey";
      full_path += '/';
      full_path += secret_key;
      full_path += '/';
    }

    if (session_token) {
      full_path += "Token";
      full_path += '/';
      full_path += session_token;
      full_path += '/';
    }

    if (expiration) {
      full_path += "Expiration";
      full_path += '/';
      full_path += expiration;
      full_path += '/';
    }

    full_path += "security/";
    if (use_token) full_path += "123456QWERTY";
    full_path += '/';

    full_path += "role/test-role/";
    full_path += "path/";

    m_cleanup +=
        Cleanup::set_env_var("AWS_EC2_METADATA_SERVICE_ENDPOINT",
                             s_test_server->address() + "/imds/" + full_path);
  }

  Settings settings(
      const std::unordered_map<Setting, std::string> &options = {}) {
    Settings result;
    std::string contents = "[default]\n";

    for (const auto &option : options) {
      contents += result.name(option.first);
      contents += " = ";
      contents += option.second;
      contents += '\n';
    }

    shcore::create_file(m_config_file, contents);

    result.add_user_option(Setting::CONFIG_FILE, m_config_file);
    result.initialize();

    return result;
  }

  static std::unique_ptr<tests::Test_server> s_test_server;
  Cleanup m_cleanup;
  std::string m_expiration;
  std::string m_config_file = shcore::get_absolute_path("test_config_file");
};

std::unique_ptr<tests::Test_server>
    Aws_imds_credentials_provider_test::s_test_server;

#define FAIL_IF_NO_SERVER                               \
  if (!s_test_server->is_alive()) {                     \
    FAIL() << "The HTTP test server is not available."; \
  }

TEST_F(Aws_imds_credentials_provider_test, no_server) {
  // there's no IMDS server, initialize should not throw
  bool b = true;
  const auto s = settings();
  EXPECT_NO_THROW(b = Imds_credentials_provider{s}.initialize());
  EXPECT_FALSE(b);
}

TEST_F(Aws_imds_credentials_provider_test, initialization) {
  {
    // IMDS is explicitly disabled
    const auto c = Cleanup::set_env_var("AWS_EC2_METADATA_DISABLED", "TRUE");
    const auto s = settings();

    Imds_credentials_provider provider{s};
    EXPECT_FALSE(provider.available());
    EXPECT_FALSE(provider.initialize());
  }

  {
    // IMDS is still explicitly disabled
    const auto c = Cleanup::set_env_var("AWS_EC2_METADATA_DISABLED", "true");
    const auto s = settings();

    Imds_credentials_provider provider{s};
    EXPECT_FALSE(provider.available());
    EXPECT_FALSE(provider.initialize());
  }

  {
    // IMDS is enabled
    // WL15885-TSFR_3_1_1
    // WL15885-TSFR_3_3_3
    // WL15885-TSFR_3_4_3
    // WL15885-TSFR_3_5_3
    const auto c = Cleanup::set_env_var("AWS_EC2_METADATA_DISABLED", "FALSE");
    const auto s = settings();

    Imds_credentials_provider provider{s};
    EXPECT_TRUE(provider.available());
    EXPECT_EQ("http://169.254.169.254/", provider.endpoint());
    EXPECT_EQ(1, provider.timeout());
    EXPECT_EQ(1, provider.attempts());
  }

  {
    // override endpoint via env var
    const auto c = Cleanup::set_env_var("AWS_EC2_METADATA_SERVICE_ENDPOINT",
                                        "http://11.22.33.44");

    {
      const auto s = settings();
      Imds_credentials_provider provider{s};
      EXPECT_EQ("http://11.22.33.44/", provider.endpoint());
    }

    {
      // WL15885-TSFR_3_2_1
      const auto s = settings(
          {{Setting::EC2_METADATA_SERVICE_ENDPOINT, "http://22.33.44.55/"}});
      Imds_credentials_provider provider{s};
      EXPECT_EQ("http://11.22.33.44/", provider.endpoint());
    }

    {
      const auto cc = Cleanup::set_env_var(
          "AWS_EC2_METADATA_SERVICE_ENDPOINT_MODE", "ipv6");
      const auto s = settings();
      Imds_credentials_provider provider{s};
      EXPECT_EQ("http://11.22.33.44/", provider.endpoint());
    }

    {
      const auto s =
          settings({{Setting::EC2_METADATA_SERVICE_ENDPOINT_MODE, "ipv6"}});
      Imds_credentials_provider provider{s};
      EXPECT_EQ("http://11.22.33.44/", provider.endpoint());
    }
  }

  {
    // override endpoint via env var - wrong value (missing scheme)
    const auto c = Cleanup::set_env_var("AWS_EC2_METADATA_SERVICE_ENDPOINT",
                                        "11.22.33.44");
    const auto s = settings();

    EXPECT_THROW_LIKE(Imds_credentials_provider{s}, std::invalid_argument,
                      "Invalid IMDS endpoint '11.22.33.44': Scheme is missing");
  }

  {
    // override endpoint via env var - IPv6
    const auto c = Cleanup::set_env_var("AWS_EC2_METADATA_SERVICE_ENDPOINT",
                                        "http://[fe80::260:8ff:fe52:f9d8]");
    const auto s = settings();
    Imds_credentials_provider provider{s};
    EXPECT_EQ("http://[fe80::260:8ff:fe52:f9d8]/", provider.endpoint());
  }

  {
    // override endpoint via env var - wrong value (malformed URL)
    const auto c = Cleanup::set_env_var("AWS_EC2_METADATA_SERVICE_ENDPOINT",
                                        "http://11.22.33..44");
    const auto s = settings();

    EXPECT_THROW_LIKE(
        Imds_credentials_provider{s}, std::invalid_argument,
        "Invalid IMDS endpoint 'http://11.22.33..44': Hostname is not valid");
  }

  // override endpoint via profile
  {
    Cleanup cc =
        Cleanup::set_env_var("AWS_EC2_METADATA_SERVICE_ENDPOINT_MODE", "ipv6");
    const auto s = settings(
        {{Setting::EC2_METADATA_SERVICE_ENDPOINT, "http://22.33.44.55/"}});
    Imds_credentials_provider provider{s};
    EXPECT_EQ("http://22.33.44.55/", provider.endpoint());
  }

  {
    const auto s = settings(
        {{Setting::EC2_METADATA_SERVICE_ENDPOINT, "http://22.33.44.55/"},
         {Setting::EC2_METADATA_SERVICE_ENDPOINT_MODE, "ipv6"}});
    Imds_credentials_provider provider{s};
    EXPECT_EQ("http://22.33.44.55/", provider.endpoint());
  }

  {
    // WL15885-TSFR_3_2_2
    const auto s = settings(
        {{Setting::EC2_METADATA_SERVICE_ENDPOINT, "http://22.33.44.55/"}});
    Imds_credentials_provider provider{s};
    EXPECT_EQ("http://22.33.44.55/", provider.endpoint());
  }

  {
    // override endpoint via profile - IPv6
    const auto s = settings({{Setting::EC2_METADATA_SERVICE_ENDPOINT,
                              "http://[fe80::260:8ff:fe52:f9d8]"}});
    Imds_credentials_provider provider{s};
    EXPECT_EQ("http://[fe80::260:8ff:fe52:f9d8]/", provider.endpoint());
  }

  {
    // override endpoint via profile - wrong value (missing scheme)
    const auto s =
        settings({{Setting::EC2_METADATA_SERVICE_ENDPOINT, "11.22.33.44"}});

    EXPECT_THROW_LIKE(Imds_credentials_provider{s}, std::invalid_argument,
                      "Invalid IMDS endpoint '11.22.33.44': Scheme is missing");
  }

  {
    // override endpoint via profile - wrong value (malformed URL)
    const auto s = settings(
        {{Setting::EC2_METADATA_SERVICE_ENDPOINT, "http://11.22.33..44"}});

    EXPECT_THROW_LIKE(
        Imds_credentials_provider{s}, std::invalid_argument,
        "Invalid IMDS endpoint 'http://11.22.33..44': Hostname is not valid");
  }

  {
    // set endpoint mode via env var - IPv6
    Cleanup c =
        Cleanup::set_env_var("AWS_EC2_METADATA_SERVICE_ENDPOINT_MODE", "IPV6");

    {
      const auto s = settings();
      Imds_credentials_provider provider{s};
      EXPECT_EQ("http://[fd00:ec2::254]/", provider.endpoint());
    }

    {
      const auto s =
          settings({{Setting::EC2_METADATA_SERVICE_ENDPOINT_MODE, "ipv4"}});
      Imds_credentials_provider provider{s};
      EXPECT_EQ("http://[fd00:ec2::254]/", provider.endpoint());
    }

    {
      // WL15885-TSFR_3_3_1
      const auto s =
          settings({{Setting::EC2_METADATA_SERVICE_ENDPOINT_MODE, "bogus"}});
      Imds_credentials_provider provider{s};
      EXPECT_EQ("http://[fd00:ec2::254]/", provider.endpoint());
    }
  }

  {
    // set endpoint mode via env var - IPv4
    Cleanup c =
        Cleanup::set_env_var("AWS_EC2_METADATA_SERVICE_ENDPOINT_MODE", "IPv4");

    {
      const auto s = settings();
      Imds_credentials_provider provider{s};
      EXPECT_EQ("http://169.254.169.254/", provider.endpoint());
    }

    {
      const auto s =
          settings({{Setting::EC2_METADATA_SERVICE_ENDPOINT_MODE, "ipv6"}});
      Imds_credentials_provider provider{s};
      EXPECT_EQ("http://169.254.169.254/", provider.endpoint());
    }
  }

  {
    // set endpoint mode via env var - wrong value
    // WL15885-TSFR_3_3_3_1
    Cleanup c =
        Cleanup::set_env_var("AWS_EC2_METADATA_SERVICE_ENDPOINT_MODE", "wrong");
    const auto s = settings();

    EXPECT_THROW_LIKE(Imds_credentials_provider{s}, std::invalid_argument,
                      "Invalid IMDS endpoint mode: wrong");
  }

  {
    // set endpoint mode via profile - IPv6
    // WL15885-TSFR_3_3_2
    const auto s =
        settings({{Setting::EC2_METADATA_SERVICE_ENDPOINT_MODE, "ipv6"}});
    Imds_credentials_provider provider{s};
    EXPECT_EQ("http://[fd00:ec2::254]/", provider.endpoint());
  }

  {
    // set endpoint mode via profile - IPv4
    const auto s =
        settings({{Setting::EC2_METADATA_SERVICE_ENDPOINT_MODE, "IPv4"}});
    Imds_credentials_provider provider{s};
    EXPECT_EQ("http://169.254.169.254/", provider.endpoint());
  }

  {
    // set endpoint mode via profile - wrong value
    // WL15885-TSFR_3_3_3_1
    const auto s =
        settings({{Setting::EC2_METADATA_SERVICE_ENDPOINT_MODE, "another"}});

    EXPECT_THROW_LIKE(Imds_credentials_provider{s}, std::invalid_argument,
                      "Invalid IMDS endpoint mode: another");
  }

  {
    // set timeout via env var
    const auto c = Cleanup::set_env_var("AWS_METADATA_SERVICE_TIMEOUT", "12");

    {
      const auto s = settings();
      Imds_credentials_provider provider{s};
      EXPECT_EQ(12, provider.timeout());
      EXPECT_EQ(1, provider.attempts());
    }

    {
      // WL15885-TSFR_3_4_1
      const auto s = settings({{Setting::METADATA_SERVICE_TIMEOUT, "23"}});
      Imds_credentials_provider provider{s};
      EXPECT_EQ(12, provider.timeout());
      EXPECT_EQ(1, provider.attempts());
    }
  }

  // set timeout via env var - wrong values
  {
    // WL15885-TSFR_3_4_1_1
    const auto c =
        Cleanup::set_env_var("AWS_METADATA_SERVICE_TIMEOUT", "wrong");
    const auto s = settings();

    EXPECT_THROW_LIKE(Imds_credentials_provider{s}, std::invalid_argument,
                      "Invalid value of 'metadata_service_timeout'");
  }

  {
    // WL15885-TSFR_3_4_2_1
    const auto c = Cleanup::set_env_var("AWS_METADATA_SERVICE_TIMEOUT", "0");
    const auto s = settings();

    EXPECT_THROW_LIKE(Imds_credentials_provider{s}, std::invalid_argument,
                      "must be greater than or equal to 1");
  }

  {
    const auto c = Cleanup::set_env_var("AWS_METADATA_SERVICE_TIMEOUT", "-1");
    const auto s = settings();

    EXPECT_THROW_LIKE(Imds_credentials_provider{s}, std::invalid_argument,
                      "must be greater than or equal to 1");
  }

  {
    // set timeout via profile
    // WL15885-TSFR_3_4_2
    const auto s = settings({{Setting::METADATA_SERVICE_TIMEOUT, "23"}});
    Imds_credentials_provider provider{s};
    EXPECT_EQ(23, provider.timeout());
    EXPECT_EQ(1, provider.attempts());
  }

  // set timeout via profile - wrong values
  {
    // WL15885-TSFR_3_4_1_1
    const auto s = settings({{Setting::METADATA_SERVICE_TIMEOUT, "wrong"}});

    EXPECT_THROW_LIKE(Imds_credentials_provider{s}, std::invalid_argument,
                      "Invalid value of 'metadata_service_timeout'");
  }

  {
    // WL15885-TSFR_3_4_2_1
    const auto s = settings({{Setting::METADATA_SERVICE_TIMEOUT, "0"}});

    EXPECT_THROW_LIKE(Imds_credentials_provider{s}, std::invalid_argument,
                      "must be greater than or equal to 1, got: 0");
  }

  {
    const auto s = settings({{Setting::METADATA_SERVICE_TIMEOUT, "-1"}});

    EXPECT_THROW_LIKE(Imds_credentials_provider{s}, std::invalid_argument,
                      "must be greater than or equal to 1, got: -1");
  }

  {
    // set number of attempts via env var
    const auto c =
        Cleanup::set_env_var("AWS_METADATA_SERVICE_NUM_ATTEMPTS", "12");

    {
      const auto s = settings();
      Imds_credentials_provider provider{s};
      EXPECT_EQ(1, provider.timeout());
      EXPECT_EQ(12, provider.attempts());
    }

    {
      // WL15885-TSFR_3_5_1
      const auto s = settings({{Setting::METADATA_SERVICE_NUM_ATTEMPTS, "23"}});
      Imds_credentials_provider provider{s};
      EXPECT_EQ(1, provider.timeout());
      EXPECT_EQ(12, provider.attempts());
    }
  }

  // set number of attempts via env var - wrong values
  {
    // WL15885-TSFR_3_5_1_1
    Cleanup c =
        Cleanup::set_env_var("AWS_METADATA_SERVICE_NUM_ATTEMPTS", "wrong");
    const auto s = settings();

    EXPECT_THROW_LIKE(Imds_credentials_provider{s}, std::invalid_argument,
                      "Invalid value of 'metadata_service_num_attempts'");
  }

  {
    const auto c =
        Cleanup::set_env_var("AWS_METADATA_SERVICE_NUM_ATTEMPTS", "0");
    const auto s = settings();

    EXPECT_THROW_LIKE(Imds_credentials_provider{s}, std::invalid_argument,
                      "must be greater than or equal to 1");
  }

  {
    // WL15885-TSFR_3_5_2_1
    const auto c =
        Cleanup::set_env_var("AWS_METADATA_SERVICE_NUM_ATTEMPTS", "-1");
    const auto s = settings();

    EXPECT_THROW_LIKE(Imds_credentials_provider{s}, std::invalid_argument,
                      "must be greater than or equal to 1");
  }

  {
    // set number of attempts via profile
    // WL15885-TSFR_3_5_2
    const auto s = settings({{Setting::METADATA_SERVICE_NUM_ATTEMPTS, "23"}});
    Imds_credentials_provider provider{s};
    EXPECT_EQ(1, provider.timeout());
    EXPECT_EQ(23, provider.attempts());
  }

  // set number of attempts via profile - wrong values
  {
    // WL15885-TSFR_3_5_1_1
    const auto s =
        settings({{Setting::METADATA_SERVICE_NUM_ATTEMPTS, "wrong"}});

    EXPECT_THROW_LIKE(Imds_credentials_provider{s}, std::invalid_argument,
                      "Invalid value of 'metadata_service_num_attempts'");
  }

  {
    const auto s = settings({{Setting::METADATA_SERVICE_NUM_ATTEMPTS, "0"}});

    EXPECT_THROW_LIKE(Imds_credentials_provider{s}, std::invalid_argument,
                      "must be greater than or equal to 1, got: 0");
  }

  {
    // WL15885-TSFR_3_5_2_1
    const auto s = settings({{Setting::METADATA_SERVICE_NUM_ATTEMPTS, "-1"}});

    EXPECT_THROW_LIKE(Imds_credentials_provider{s}, std::invalid_argument,
                      "must be greater than or equal to 1, got: -1");
  }
}

TEST_F(Aws_imds_credentials_provider_test, success) {
  FAIL_IF_NO_SERVER

  for (const auto use_token : {true, false}) {
    set_endpoint("access-key", "secret-key", "session-token",
                 m_expiration.c_str(), use_token);

    const auto s = settings();
    Imds_credentials_provider provider{s};
    EXPECT_TRUE(provider.available());
    ASSERT_TRUE(provider.initialize());

    EXPECT_EQ("access-key", provider.credentials()->access_key_id());
    EXPECT_EQ("secret-key", provider.credentials()->secret_access_key());
    EXPECT_EQ("session-token", provider.credentials()->session_token());
    EXPECT_EQ(shcore::rfc3339_to_time_point(m_expiration),
              provider.credentials()->expiration());
  }
}

TEST_F(Aws_imds_credentials_provider_test, no_access_key) {
  FAIL_IF_NO_SERVER

  // WL15885-TSFR_3_7_1
  set_endpoint(nullptr, "secret-key", "session-token", m_expiration.c_str());

  const auto s = settings();
  Imds_credentials_provider provider{s};
  EXPECT_THROW_LIKE(
      provider.initialize(), std::runtime_error,
      "JSON object should contain a 'AccessKeyId' key with a string value");
}

TEST_F(Aws_imds_credentials_provider_test, invalid_access_key) {
  FAIL_IF_NO_SERVER

  set_endpoint("1", "secret-key", "session-token", m_expiration.c_str());

  const auto s = settings();
  Imds_credentials_provider provider{s};
  EXPECT_THROW_LIKE(
      provider.initialize(), std::runtime_error,
      "JSON object should contain a 'AccessKeyId' key with a string value");
}

TEST_F(Aws_imds_credentials_provider_test, no_secret_key) {
  FAIL_IF_NO_SERVER

  set_endpoint("access-key", nullptr, "session-token", m_expiration.c_str());

  const auto s = settings();
  Imds_credentials_provider provider{s};
  EXPECT_THROW_LIKE(
      provider.initialize(), std::runtime_error,
      "JSON object should contain a 'SecretAccessKey' key with a string value");
}

TEST_F(Aws_imds_credentials_provider_test, invalid_secret_key) {
  FAIL_IF_NO_SERVER

  set_endpoint("access-key", "1", "session-token", m_expiration.c_str());

  const auto s = settings();
  Imds_credentials_provider provider{s};
  EXPECT_THROW_LIKE(
      provider.initialize(), std::runtime_error,
      "JSON object should contain a 'SecretAccessKey' key with a string value");
}

TEST_F(Aws_imds_credentials_provider_test, no_session_token) {
  FAIL_IF_NO_SERVER

  set_endpoint("access-key", "secret-key", nullptr, m_expiration.c_str());

  const auto s = settings();
  Imds_credentials_provider provider{s};
  EXPECT_THROW_LIKE(
      provider.initialize(), std::runtime_error,
      "JSON object should contain a 'Token' key with a string value");
}

TEST_F(Aws_imds_credentials_provider_test, invalid_session_token) {
  FAIL_IF_NO_SERVER

  set_endpoint("access-key", "secret-key", "1", m_expiration.c_str());

  const auto s = settings();
  Imds_credentials_provider provider{s};
  EXPECT_THROW_LIKE(
      provider.initialize(), std::runtime_error,
      "JSON object should contain a 'Token' key with a string value");
}

TEST_F(Aws_imds_credentials_provider_test, no_expiration) {
  FAIL_IF_NO_SERVER

  set_endpoint("access-key", "secret-key", "session-token", nullptr);

  const auto s = settings();
  Imds_credentials_provider provider{s};
  EXPECT_THROW_LIKE(
      provider.initialize(), std::runtime_error,
      "JSON object should contain a 'Expiration' key with a string value");
}

TEST_F(Aws_imds_credentials_provider_test, invalid_expiration_type) {
  FAIL_IF_NO_SERVER

  set_endpoint("access-key", "secret-key", "session-token", "1");

  const auto s = settings();
  Imds_credentials_provider provider{s};
  EXPECT_THROW_LIKE(
      provider.initialize(), std::runtime_error,
      "JSON object should contain a 'Expiration' key with a string value");
}

TEST_F(Aws_imds_credentials_provider_test, invalid_expiration_value) {
  FAIL_IF_NO_SERVER

  set_endpoint("access-key", "secret-key", "session-token", "2023-04-17");

  const auto s = settings();
  Imds_credentials_provider provider{s};
  EXPECT_THROW_LIKE(
      provider.initialize(), std::runtime_error,
      "Failed to parse 'Expiration' value '2023-04-17' in IMDS credentials");
}

}  // namespace
}  // namespace aws
}  // namespace mysqlshdk
