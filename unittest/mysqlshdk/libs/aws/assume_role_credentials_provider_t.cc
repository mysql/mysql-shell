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

#include "mysqlshdk/libs/aws/assume_role_credentials_provider.h"

#include <stdexcept>

#include "mysqlshdk/libs/utils/utils_file.h"

#include "unittest/gtest_clean.h"
#include "unittest/test_utils/cleanup.h"
#include "unittest/test_utils/shell_test_env.h"

namespace mysqlshdk {
namespace aws {
namespace {

using tests::Cleanup;

class Aws_assume_role_credentials_provider_test : public ::testing::Test {
 protected:
  void SetUp() override {
    m_cleanup += clear_aws_env_vars();
    m_cleanup.add([this]() {
      delete_file(m_config_file);
      delete_file(m_credentials_file);
    });
  }

  [[nodiscard]] static Cleanup clear_aws_env_vars() {
    Cleanup c;

    c += Cleanup::unset_env_var("AWS_EC2_METADATA_DISABLED");
    c += Cleanup::unset_env_var("AWS_ACCESS_KEY_ID");
    c += Cleanup::unset_env_var("AWS_SECRET_ACCESS_KEY");
    c += Cleanup::unset_env_var("AWS_CONTAINER_CREDENTIALS_RELATIVE_URI");
    c += Cleanup::unset_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI");

    return c;
  }

  static void delete_file(const std::string &path) {
    if (shcore::is_file(path)) {
      shcore::delete_file(path);
    }
  }

  Settings settings(const std::string &config_file,
                    const std::string &credentials_file = {}) {
    Settings result;

    shcore::create_file(m_config_file, config_file);
    result.add_user_option(Setting::CONFIG_FILE, m_config_file);

    if (!credentials_file.empty()) {
      shcore::create_file(m_credentials_file, credentials_file);
      result.add_user_option(Setting::CREDENTIALS_FILE, m_credentials_file);
    }

    result.initialize();

    return result;
  }

  Cleanup m_cleanup;
  std::string m_config_file = shcore::get_absolute_path("test_config_file");
  std::string m_credentials_file =
      shcore::get_absolute_path("test_credentials_file");
};

TEST_F(Aws_assume_role_credentials_provider_test, no_role_arn) {
  const auto s = settings(R"([no-role-arn]
aws_access_key_id = access-key
aws_secret_access_key = secret-key
[empty-role-arn]
role_arn =
aws_access_key_id = access-key-2
aws_secret_access_key = secret-key-2
[empty-role-arn-with-credential-source]
role_arn =
credential_source = Environment
aws_access_key_id = access-key-3
aws_secret_access_key = secret-key-3
[role-arn-with-token-file]
role_arn = test-role
web_identity_token_file = token-file
)");

  EXPECT_FALSE(
      Assume_role_credentials_provider(s, "invalid-profile").initialize());
  // WL15885-TSFR_1_1_1_1
  EXPECT_FALSE(Assume_role_credentials_provider(s, "no-role-arn").initialize());
  EXPECT_FALSE(
      Assume_role_credentials_provider(s, "empty-role-arn").initialize());
  // WL15885-TSFR_1_1_1_2
  EXPECT_FALSE(Assume_role_credentials_provider(
                   s, "empty-role-arn-with-credential-source")
                   .initialize());
  // WL15885-TSFR_1_1_2_1
  EXPECT_FALSE(Assume_role_credentials_provider(s, "role-arn-with-token-file")
                   .initialize());
}

TEST_F(Aws_assume_role_credentials_provider_test, invalid_settings) {
  const auto s = settings(R"([default]
aws_access_key_id = access-key
aws_secret_access_key = secret-key

[profile has-mfa]
role_arn = test-role
credential_source = Environment
mfa_serial = MFA

[profile invalid-duration]
role_arn = test-role
credential_source = Environment
duration_seconds = 10s

[profile missing-source]
role_arn = test-role

[profile empty-source]
role_arn = test-role
credential_source =
source_profile =

[profile two-sources]
role_arn = test-role
credential_source = Environment
source_profile = default
)");

  EXPECT_THROW_LIKE(Assume_role_credentials_provider(s, "has-mfa"),
                    std::invalid_argument,
                    "Profile 'has-mfa' is using multi-factor authentication "
                    "device to assume a role, this is not supported");

  // WL15885-TSFR_1_7_3
  EXPECT_THROW_LIKE(Assume_role_credentials_provider(s, "invalid-duration"),
                    std::invalid_argument,
                    "Profile 'invalid-duration' contains invalid value of "
                    "'duration_seconds' setting: ");

  // WL15885-TSFR_1_4_1
  EXPECT_THROW_LIKE(Assume_role_credentials_provider(s, "missing-source"),
                    std::invalid_argument,
                    "Profile 'missing-source' contains neither "
                    "'source_profile' nor 'credential_source' settings");

  EXPECT_THROW_LIKE(Assume_role_credentials_provider(s, "empty-source"),
                    std::invalid_argument,
                    "Profile 'empty-source' contains neither 'source_profile' "
                    "nor 'credential_source' settings");

  // WL15885-TSFR_1_5_1
  EXPECT_THROW_LIKE(Assume_role_credentials_provider(s, "two-sources"),
                    std::invalid_argument,
                    "Profile 'two-sources' contains both 'source_profile' and "
                    "'credential_source' settings");
}

TEST_F(Aws_assume_role_credentials_provider_test, credential_source) {
  const auto s = settings(R"([default]
aws_access_key_id = access-key
aws_secret_access_key = secret-key

[profile uses-env]
role_arn = test-role
credential_source = Environment

[profile uses-imds]
role_arn = test-role
credential_source = Ec2InstanceMetadata

[profile uses-ecs]
role_arn = test-role
credential_source = EcsContainer

[profile invalid-value]
role_arn = test-role
credential_source = environment
)");

  {
    // WL15885-TSFR_1_5_2
    // WL15885-TSFR_1_9_2
    Assume_role_credentials_provider provider{s, "uses-env"};

    EXPECT_THROW_LIKE(provider.initialize(), std::runtime_error,
                      "Could not obtain credentials to assume role using "
                      "profile 'uses-env': The AWS access and secret keys were "
                      "not found, tried: environment variables");
  }

  {
    // WL15885-TSFR_1_9_4
    Assume_role_credentials_provider provider{s, "uses-imds"};

    EXPECT_THROW_LIKE(provider.initialize(), std::runtime_error,
                      "Could not obtain credentials to assume role using "
                      "profile 'uses-imds': The AWS access and secret keys "
                      "were not found, tried: IMDS credentials");
  }

  {
    // WL15885-TSFR_1_9_3
    const auto c = Cleanup::set_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI",
                                        "http://127.0.0.1");
    Assume_role_credentials_provider provider{s, "uses-ecs"};

    EXPECT_THROW_LIKE(provider.initialize(), std::runtime_error,
                      "Could not obtain credentials to assume role using "
                      "profile 'uses-ecs': Failed to fetch ECS credentials");
  }

  // WL15885-TSFR_1_2_4_1
  EXPECT_THROW_LIKE(Assume_role_credentials_provider(s, "invalid-value"),
                    std::invalid_argument,
                    "Profile 'invalid-value' has invalid value of "
                    "'credential_source' setting: environment");
}

TEST_F(Aws_assume_role_credentials_provider_test, source_profile) {
  const auto s = settings(R"([default]
aws_access_key_id = access-key
aws_secret_access_key = secret-key

# self reference with static credentials
[profile self-reference-ok]
role_arn = test-role
source_profile = self-reference-ok
aws_access_key_id = access-key-2
aws_secret_access_key = secret-key-2

# self reference, missing secret key
[profile self-reference-wrong-1]
role_arn = test-role
source_profile = self-reference-wrong-1
aws_access_key_id = access-key

# self reference, missing access key
[profile self-reference-wrong-2]
role_arn = test-role
source_profile = self-reference-wrong-2
aws_secret_access_key = secret-key

# self reference, missing static credentials
[profile self-reference-wrong-3]
role_arn = test-role
source_profile = self-reference-wrong-3

# references a profile which does not exist
[profile references-unknown-profile]
role_arn = test-role
source_profile = unknown

# longer chain which ends with self reference
[profile source-is-self-ok]
role_arn = test-role-2
source_profile = self-reference-ok

# longer chain which ends with an invalid self reference
[profile source-is-self-wrong]
role_arn = test-role-2
source_profile = self-reference-wrong-2

# profiles create a cycle
[profile cycle-1]
role_arn = test-role
source_profile = cycle-2

[profile cycle-2]
role_arn = test-role
source_profile = cycle-1

[profile cycle-3]
role_arn = test-role-1
source_profile = cycle-4

[profile cycle-4]
role_arn = test-role-2
source_profile = cycle-5

[profile cycle-5]
role_arn = test-role-2
source_profile = cycle-1

# longer chain which ends with another role assumed with environment vars
[profile source-is-env]
role_arn = test-role-2
source_profile = uses-env

[profile uses-env]
role_arn = test-role
credential_source = Environment

# longer chain which ends with another role assumed with IMDS
[profile source-is-imds]
role_arn = test-role-2
source_profile = uses-imds

[profile uses-imds]
role_arn = test-role
credential_source = Ec2InstanceMetadata

# longer chain which ends with another role assumed with ECS
[profile source-is-ecs]
role_arn = test-role-2
source_profile = uses-ecs

[profile uses-ecs]
role_arn = test-role
credential_source = EcsContainer

# longer chain which ends with process credentials
[profile source-is-process]
role_arn = test-role-2
source_profile = uses-process

[profile uses-process]
credential_process = oops

# longer chain which ends with static credentials
[profile source-is-static]
role_arn = test-role-2
source_profile = default

# longer chain which ends with static credentials in credentials file
[profile source-is-credentials-file]
role_arn = test-role-2
source_profile = creds
)",
                          R"([creds]
aws_access_key_id = access-key
aws_secret_access_key = secret-key
)");

  // WL15885-TSFR_1_3_2_1
  EXPECT_NO_THROW(Assume_role_credentials_provider(s, "self-reference-ok"));

  // WL15885-TSFR_1_9_1
  EXPECT_THROW_LIKE(
      Assume_role_credentials_provider(s, "self-reference-wrong-1"),
      std::invalid_argument, "create a cycle");

  EXPECT_THROW_LIKE(
      Assume_role_credentials_provider(s, "self-reference-wrong-2"),
      std::invalid_argument, "create a cycle");

  EXPECT_THROW_LIKE(
      Assume_role_credentials_provider(s, "self-reference-wrong-3"),
      std::invalid_argument, "create a cycle");

  // WL15885-TSFR_1_3_4_1
  EXPECT_THROW_LIKE(
      Assume_role_credentials_provider(s, "references-unknown-profile"),
      std::invalid_argument,
      "Profile 'unknown' referenced from profile 'references-unknown-profile' "
      "does not exist");

  // WL15885-TSFR_1_3_1_2
  EXPECT_NO_THROW(Assume_role_credentials_provider(s, "source-is-self-ok"));

  EXPECT_THROW_LIKE(Assume_role_credentials_provider(s, "source-is-self-wrong"),
                    std::invalid_argument, "create a cycle");

  // WL15885-TSFR_1_3_3_1
  EXPECT_THROW_LIKE(Assume_role_credentials_provider(s, "cycle-1"),
                    std::invalid_argument, "create a cycle");

  EXPECT_THROW_LIKE(Assume_role_credentials_provider(s, "cycle-3"),
                    std::invalid_argument, "create a cycle");

  {
    Assume_role_credentials_provider provider{s, "source-is-env"};

    EXPECT_THROW_LIKE(
        provider.initialize(), std::runtime_error,
        "Could not obtain credentials to assume role using profile "
        "'source-is-env': Could not obtain credentials to assume role using "
        "profile 'uses-env': The AWS access and secret keys were not found, "
        "tried: environment variables");
  }

  {
    // WL15885-TSFR_3_6_1
    Assume_role_credentials_provider provider{s, "source-is-imds"};

    EXPECT_THROW_LIKE(
        provider.initialize(), std::runtime_error,
        "Could not obtain credentials to assume role using profile "
        "'source-is-imds': Could not obtain credentials to assume role using "
        "profile 'uses-imds': The AWS access and secret keys were not found, "
        "tried: IMDS credentials");
  }

  {
    // WL15885-TSFR_2_2_1_1
    const auto c = Cleanup::set_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI",
                                        "http://example.com");
    Assume_role_credentials_provider provider{s, "source-is-ecs"};

    EXPECT_THROW_LIKE(
        provider.initialize(), std::runtime_error,
        "Could not obtain credentials to assume role using profile "
        "'source-is-ecs': Could not obtain credentials to assume role using "
        "profile 'uses-ecs': Invalid URI for ECS credentials "
        "'http://example.com': Unsupported host 'example.com', allowed values "
        "are: a loopback address, or one of: 169.254.170.2, 169.254.170.23, "
        "fd00:ec2::23, localhost");
  }

  {
    // WL15885-TSFR_2_3_1
    Assume_role_credentials_provider provider{s, "source-is-ecs"};

    EXPECT_THROW_LIKE(
        provider.initialize(), std::runtime_error,
        "Could not obtain credentials to assume role using profile "
        "'source-is-ecs': Could not obtain credentials to assume role using "
        "profile 'uses-ecs': Could not select the AWS credentials provider, "
        "please see log for more details");
  }

  {
    // WL15885-TSFR_2_5_1
    const auto c = Cleanup::set_env_var("AWS_CONTAINER_CREDENTIALS_FULL_URI",
                                        "http://127.0.0.1");
    Assume_role_credentials_provider provider{s, "source-is-ecs"};

    EXPECT_THROW_LIKE(
        provider.initialize(), std::runtime_error,
        "Could not obtain credentials to assume role using profile "
        "'source-is-ecs': Could not obtain credentials to assume role using "
        "profile 'uses-ecs': Failed to fetch ECS credentials");
  }

  {
    Assume_role_credentials_provider provider{s, "source-is-process"};

    EXPECT_THROW_LIKE(provider.initialize(), std::runtime_error,
                      "Could not obtain credentials to assume role using "
                      "profile 'source-is-process': AWS credential process: "
                      "process returned an error code");
  }

  EXPECT_NO_THROW(Assume_role_credentials_provider(s, "source-is-static"));

  EXPECT_NO_THROW(
      Assume_role_credentials_provider(s, "source-is-credentials-file"));
}

}  // namespace
}  // namespace aws
}  // namespace mysqlshdk
