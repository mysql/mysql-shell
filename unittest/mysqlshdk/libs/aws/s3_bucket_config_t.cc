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

#include "unittest/gprod_clean.h"

#include "mysqlshdk/libs/aws/s3_bucket_config.h"

#include "unittest/gtest_clean.h"

#include <memory>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_time.h"

#include "mysqlshdk/libs/aws/s3_bucket_options.h"

#include "unittest/test_utils/cleanup.h"

namespace mysqlshdk {
namespace aws {

using tests::Cleanup;

class Aws_s3_bucket_config_test : public testing::Test {
 protected:
  class Config_builder final {
   public:
    Config_builder() {
      m_options =
          shcore::make_dict(S3_bucket_options::bucket_name_option(), "bucket");
    }

    Config_builder &credentials(const std::string &value) {
      m_options->emplace(S3_bucket_options::credentials_file_option(), value);
      return *this;
    }

    Config_builder &config(const std::string &value) {
      m_options->emplace(S3_bucket_options::config_file_option(), value);
      return *this;
    }

    Config_builder &profile(const std::string &value) {
      m_options->emplace(S3_bucket_options::profile_option(), value);
      return *this;
    }

    Config_builder &region(const std::string &value) {
      m_options->emplace(S3_bucket_options::region_option(), value);
      return *this;
    }

    Config_builder &endpoint(const std::string &value) {
      m_options->emplace(S3_bucket_options::endpoint_override_option(), value);
      return *this;
    }

    std::shared_ptr<S3_bucket_config> build() const {
      S3_bucket_options parsed_options;
      S3_bucket_options::options().unpack(m_options, &parsed_options);

      return std::make_shared<S3_bucket_config>(parsed_options);
    }

   private:
    shcore::Dictionary_t m_options;
  };

  void SetUp() override {
    Test::SetUp();
    m_cleanup += clear_aws_env_vars();
    m_script_path = shcore::get_absolute_path(k_script_name);
  }

  static std::string default_config_file() {
    return default_config_path("config");
  }

  static std::string default_credentials_file() {
    return default_config_path("credentials");
  }

  [[nodiscard]] static Cleanup write_default_config(
      const std::string &contents) {
    return write_file(default_config_file(), contents);
  }

  [[nodiscard]] static Cleanup write_default_credentials(
      const std::string &contents) {
    return write_file(default_credentials_file(), contents);
  }

  [[nodiscard]] static Cleanup write_file(const std::string &path,
                                          const std::string &contents) {
    Cleanup c;
    const auto dir = shcore::path::dirname(path);

    if (!shcore::path_exists(dir)) {
      shcore::create_directory(dir, false);
      c.add([dir]() { shcore::remove_directory(dir); });
    }

    if (shcore::is_file(path)) {
      auto backup = path + ".backup";
      std::string suffix;
      int i = 0;

      while (shcore::is_file(backup + suffix)) {
        suffix = "." + std::to_string(i++);
      }

      backup += suffix;

      shcore::rename_file(path, backup);
      c.add([path, backup]() { shcore::rename_file(backup, path); });
    }

    shcore::create_file(path, contents);
    c.add([path]() { shcore::delete_file(path); });

    return c;
  }

  [[nodiscard]] static Cleanup unset_env_var(const char *name) {
    return Cleanup::unset_env_var(name);
  }

  [[nodiscard]] static Cleanup set_env_var(const char *name,
                                           const std::string &value) {
    return Cleanup::set_env_var(name, value);
  }

  [[nodiscard]] static Cleanup clear_aws_env_vars() {
    Cleanup c;

    c += unset_env_var("AWS_PROFILE");
    c += unset_env_var("AWS_DEFAULT_PROFILE");
    c += unset_env_var("AWS_SHARED_CREDENTIALS_FILE");
    c += unset_env_var("AWS_CONFIG_FILE");
    c += unset_env_var("AWS_REGION");
    c += unset_env_var("AWS_DEFAULT_REGION");
    c += unset_env_var("AWS_ACCESS_KEY_ID");
    c += unset_env_var("AWS_SECRET_ACCESS_KEY");
    c += unset_env_var("AWS_SESSION_TOKEN");

    return c;
  }

  static std::string default_config(bool with_session_token = false) {
    std::string config;

    config += "[default]\n";
    config += "region = default-region\n";
    config += "aws_access_key_id = default-access-key\n";
    config += "aws_secret_access_key = default-secret-key\n";

    if (with_session_token) {
      config += "aws_session_token = default-session-token\n";
    }

    return config;
  }

  static std::string shell_executable() {
    return shcore::path::join_path(shcore::get_binary_folder(), "mysqlsh");
  }

  std::string invalid_config(bool with_session_token = false) const {
    std::string config;

    config += "[profile " + k_profile_name + "]\n";
    config += "region = invalid-region\n";
    config += "aws_access_key_id = invalid-access-key\n";
    config += "aws_secret_access_key = invalid-secret-key\n";

    if (with_session_token) {
      config += "aws_session_token = invalid-session-token\n";
    }

    return config;
  }

  std::string valid_config(bool with_session_token = false) const {
    std::string config;

    config += "[profile " + k_profile_name + "]\n";
    config += "region = " + k_region_name + "\n";
    config += "aws_access_key_id = " + k_access_key + "\n";
    config += "aws_secret_access_key = " + k_secret_key + "\n";

    if (with_session_token) {
      config += "aws_session_token = " + k_session_token + "\n";
    }

    return config;
  }

  shcore::Dictionary_t valid_process_creds(
      bool with_session_token = false,
      const std::string &expiration = {}) const {
    auto creds = shcore::make_dict("Version", 1, "AccessKeyId", k_access_key,
                                   "SecretAccessKey", k_secret_key);

    if (with_session_token) {
      creds->emplace("SessionToken", k_session_token);
    }

    if (!expiration.empty()) {
      creds->emplace("Expiration", expiration);
    }

    return creds;
  }

  std::string invalid_process_config() const {
    std::string config;

    config += "[profile " + k_profile_name + "]\n";
    config += "credential_process = ooops\n";

    return config;
  }

  std::string valid_process_config() const {
    std::string config;

    config += "[profile " + k_profile_name + "]\n";
    config += "credential_process = " + shell_executable() + " --py --file " +
              m_script_path + "\n";

    return config;
  }

  static std::string py_print_json(const shcore::Dictionary_t &json) {
    return "print('''" + shcore::Value(json).json(true) + "''')\n";
  }

  [[nodiscard]] Cleanup write_process_script(const shcore::Dictionary_t &json) {
    return write_process_script(py_print_json(json));
  }

  [[nodiscard]] Cleanup write_process_script(const std::string &contents) {
    return write_file(m_script_path, contents);
  }

  const std::string k_profile_name = "valid-profile";
  const std::string k_credentials_file = "valid-credentials-file";
  const std::string k_config_file = "valid-config-file";
  const std::string k_region_name = "valid-region";
  const std::string k_access_key = "valid-access-key";
  const std::string k_secret_key = "valid-secret-key";
  const std::string k_session_token = "valid-session-token";
  const std::string k_endpoint = "https://example.com";
  const std::string k_script_name = "creds.py";
  std::string m_script_path;

 private:
  static std::string default_config_path(const std::string &filename) {
    static const auto s_home_dir = shcore::get_home_dir();
    return shcore::path::join_path(s_home_dir, ".aws", filename);
  }

  Cleanup m_cleanup;
};

TEST_F(Aws_s3_bucket_config_test, profile) {
  Cleanup cleanup;
  cleanup += write_default_config(valid_config());

  {
    // option overrides AWS_PROFILE environment variable
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_PROFILE", "invalid");

    const auto config = Config_builder{}.profile(k_profile_name).build();

    EXPECT_EQ(k_profile_name, config->config_profile());
  }

  {
    // option overrides AWS_DEFAULT_PROFILE environment variable
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_DEFAULT_PROFILE", "invalid");

    const auto config = Config_builder{}.profile(k_profile_name).build();

    EXPECT_EQ(k_profile_name, config->config_profile());
  }

  {
    // option overrides default value
    Cleanup c;
    c += clear_aws_env_vars();

    const auto config = Config_builder{}.profile(k_profile_name).build();

    EXPECT_EQ(k_profile_name, config->config_profile());
  }

  {
    // AWS_PROFILE overrides AWS_DEFAULT_PROFILE environment variable
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_PROFILE", k_profile_name);
    c += set_env_var("AWS_DEFAULT_PROFILE", "invalid");

    const auto config = Config_builder{}.build();

    EXPECT_EQ(k_profile_name, config->config_profile());
  }

  {
    // AWS_DEFAULT_PROFILE environment variable overrides default value
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_DEFAULT_PROFILE", k_profile_name);

    const auto config = Config_builder{}.build();

    EXPECT_EQ(k_profile_name, config->config_profile());
  }

  {
    // default value
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(default_config());

    const auto config = Config_builder{}.build();

    EXPECT_EQ("default", config->config_profile());
  }
}

TEST_F(Aws_s3_bucket_config_test, credentials_file) {
  Cleanup cleanup;
  cleanup += write_default_credentials(invalid_config());
  cleanup += write_default_config(invalid_config());
  cleanup += write_file(k_credentials_file, default_config());

  {
    // option overrides AWS_SHARED_CREDENTIALS_FILE environment variable
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_SHARED_CREDENTIALS_FILE", "invalid");

    const auto config =
        Config_builder{}.credentials(k_credentials_file).build();

    EXPECT_EQ(k_credentials_file, config->credentials_file());
  }

  {
    // option overrides default value
    Cleanup c;
    c += clear_aws_env_vars();

    const auto config =
        Config_builder{}.credentials(k_credentials_file).build();

    EXPECT_EQ(k_credentials_file, config->credentials_file());
  }

  {
    // AWS_SHARED_CREDENTIALS_FILE environment variable overrides default value
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_SHARED_CREDENTIALS_FILE", k_credentials_file);

    const auto config = Config_builder{}.build();

    EXPECT_EQ(k_credentials_file, config->credentials_file());
  }

  {
    // default value
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_credentials(default_config());
    c += write_file(k_credentials_file, invalid_config());

    const auto config = Config_builder{}.build();

    EXPECT_EQ(default_credentials_file(), config->credentials_file());
  }
}

TEST_F(Aws_s3_bucket_config_test, config_file) {
  Cleanup cleanup;
  cleanup += write_default_credentials(invalid_config());
  cleanup += write_default_config(invalid_config());
  cleanup += write_file(k_config_file, default_config());

  {
    // option overrides AWS_CONFIG_FILE environment variable
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_CONFIG_FILE", "invalid");

    const auto config = Config_builder{}.config(k_config_file).build();

    EXPECT_EQ(k_config_file, config->config_file());
  }

  {
    // option overrides default value
    Cleanup c;
    c += clear_aws_env_vars();

    const auto config = Config_builder{}.config(k_config_file).build();

    EXPECT_EQ(k_config_file, config->config_file());
  }

  {
    // AWS_CONFIG_FILE environment variable overrides default value
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_CONFIG_FILE", k_config_file);

    const auto config = Config_builder{}.build();

    EXPECT_EQ(k_config_file, config->config_file());
  }

  {
    // default value
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(default_config());
    c += write_file(k_config_file, invalid_config());

    const auto config = Config_builder{}.build();

    EXPECT_EQ(default_config_file(), config->config_file());
  }
}

TEST_F(Aws_s3_bucket_config_test, region) {
  Cleanup cleanup;
  cleanup += write_default_config(default_config());

  {
    // option overrides AWS_REGION environment variable
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_REGION", "invalid");

    const auto config = Config_builder{}.region(k_region_name).build();

    EXPECT_EQ(k_region_name, config->region());
  }

  {
    // option overrides AWS_DEFAULT_REGION environment variable
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_DEFAULT_REGION", "invalid");

    const auto config = Config_builder{}.region(k_region_name).build();

    EXPECT_EQ(k_region_name, config->region());
  }

  {
    // option overrides config-file value and default value
    Cleanup c;
    c += clear_aws_env_vars();

    const auto config = Config_builder{}.region(k_region_name).build();

    EXPECT_EQ(k_region_name, config->region());
  }

  {
    // AWS_REGION overrides AWS_DEFAULT_REGION environment variable
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_REGION", k_region_name);
    c += set_env_var("AWS_DEFAULT_REGION", "invalid");

    const auto config = Config_builder{}.build();

    EXPECT_EQ(k_region_name, config->region());
  }

  {
    // AWS_DEFAULT_REGION environment variable overrides config-file value and
    // default value
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_DEFAULT_REGION", k_region_name);

    const auto config = Config_builder{}.build();

    EXPECT_EQ(k_region_name, config->region());
  }

  {
    // config-file value overrides default value
    Cleanup c;
    c += clear_aws_env_vars();

    const auto config = Config_builder{}.build();

    EXPECT_EQ("default-region", config->region());
  }

  {
    // default value
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_credentials(default_config());
    c += write_default_config(invalid_config());

    const auto config = Config_builder{}.build();

    EXPECT_EQ("us-east-1", config->region());
  }
}

TEST_F(Aws_s3_bucket_config_test, endpoint) {
  Cleanup cleanup;
  cleanup += write_default_config(default_config());

  {
    // option overrides default value
    Cleanup c;
    c += clear_aws_env_vars();

    const auto config = Config_builder{}.endpoint(k_endpoint).build();

    EXPECT_EQ(k_endpoint, config->service_endpoint());
  }

  {
    // default value
    Cleanup c;
    c += clear_aws_env_vars();

    const auto config = Config_builder{}.build();

    EXPECT_EQ("https://bucket.s3.default-region.amazonaws.com",
              config->service_endpoint());
  }
}

TEST_F(Aws_s3_bucket_config_test, env_var_credentials) {
  Cleanup cleanup;
  cleanup += write_default_credentials(invalid_config(true));
  cleanup += write_default_config(invalid_config(true));

  {
    // environment variables override config files
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_ACCESS_KEY_ID", k_access_key);
    c += set_env_var("AWS_SECRET_ACCESS_KEY", k_secret_key);

    const auto config = Config_builder{}.build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ("", credentials->session_token());
  }

  {
    // session token set to an empty string
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_ACCESS_KEY_ID", k_access_key);
    c += set_env_var("AWS_SECRET_ACCESS_KEY", k_secret_key);
    c += set_env_var("AWS_SESSION_TOKEN", "");

    const auto config = Config_builder{}.build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ("", credentials->session_token());
  }

  {
    // session token set to some value
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_ACCESS_KEY_ID", k_access_key);
    c += set_env_var("AWS_SECRET_ACCESS_KEY", k_secret_key);
    c += set_env_var("AWS_SESSION_TOKEN", k_session_token);

    const auto config = Config_builder{}.build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ(k_session_token, credentials->session_token());
  }

  {
    // partial credentials, missing access key
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_SECRET_ACCESS_KEY", k_secret_key);

    EXPECT_THROW(Config_builder{}.build(), std::runtime_error);
  }

  {
    // partial credentials, missing secret key
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_ACCESS_KEY_ID", k_access_key);

    EXPECT_THROW(Config_builder{}.build(), std::runtime_error);
  }

  {
    // if profile is set, environment variables are not used
    Cleanup c;
    c += clear_aws_env_vars();
    c += set_env_var("AWS_ACCESS_KEY_ID", "invalid");
    c += set_env_var("AWS_SECRET_ACCESS_KEY", "invalid");
    c += write_default_credentials(valid_config());

    const auto config = Config_builder{}.profile(k_profile_name).build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ("", credentials->session_token());
  }
}

TEST_F(Aws_s3_bucket_config_test, credentials_file_credentials) {
  {
    // credentials file has priority
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_credentials(valid_config());
    c += write_default_config(invalid_config(true));

    const auto config = Config_builder{}.profile(k_profile_name).build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ("", credentials->session_token());
  }

  {
    // credentials file has priority over credential_process setting in config
    // file
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_credentials(valid_config());
    c += write_default_config(invalid_process_config());

    const auto config = Config_builder{}.profile(k_profile_name).build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ("", credentials->session_token());
  }

  {
    // credentials file has priority, session token set in the other file is not
    // used
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_credentials(valid_config());
    c += write_default_config(valid_config(true));

    const auto config = Config_builder{}.profile(k_profile_name).build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ("", credentials->session_token());
  }

  {
    // session token set to an empty string
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_credentials(valid_config() + "aws_session_token=\n");
    c += write_default_config(invalid_config(true));

    const auto config = Config_builder{}.profile(k_profile_name).build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ("", credentials->session_token());
  }

  {
    // session token set to some value
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_credentials(valid_config(true));
    c += write_default_config(invalid_config(true));

    const auto config = Config_builder{}.profile(k_profile_name).build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ(k_session_token, credentials->session_token());
  }

  {
    // partial credentials, missing access key
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_credentials("[default]\naws_secret_access_key=key");

    EXPECT_THROW(Config_builder{}.build(), std::runtime_error);
  }

  {
    // partial credentials, missing secret key
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_credentials("[default]\naws_access_key_id=key");

    EXPECT_THROW(Config_builder{}.build(), std::runtime_error);
  }
}

TEST_F(Aws_s3_bucket_config_test, process_credentials) {
  {
    // process
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script(valid_process_creds());

    const auto config = Config_builder{}.profile(k_profile_name).build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ("", credentials->session_token());
    EXPECT_FALSE(credentials->temporary());
    EXPECT_FALSE(credentials->expired());
  }

  {
    // process has priority over explicit credentials in config file
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config() + invalid_config());
    c += write_process_script(valid_process_creds());

    const auto config = Config_builder{}.profile(k_profile_name).build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ("", credentials->session_token());
    EXPECT_FALSE(credentials->temporary());
    EXPECT_FALSE(credentials->expired());
  }

  {
    // session token set to an empty string
    const auto creds = valid_process_creds(true);
    creds->set("SessionToken", shcore::Value(""));

    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script(creds);

    const auto config = Config_builder{}.profile(k_profile_name).build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ("", credentials->session_token());
    EXPECT_FALSE(credentials->temporary());
    EXPECT_FALSE(credentials->expired());
    EXPECT_EQ(Aws_credentials::NO_EXPIRATION, credentials->expiration());
  }

  const auto EXPECT_EXPIRATION = [](Aws_credentials::Time_point expected,
                                    Aws_credentials::Time_point actual) {
    // shcore::time_point_to_rfc3339() does not include information about
    // fractions of seconds, these need to be converted to seconds before we can
    // compare them
    EXPECT_EQ(std::chrono::floor<std::chrono::seconds>(expected),
              std::chrono::floor<std::chrono::seconds>(actual));
  };

  {
    // session token set to an empty string + expiration
    const auto expiration =
        Aws_credentials::Clock::now() - std::chrono::seconds(1);
    const auto creds =
        valid_process_creds(true, shcore::time_point_to_rfc3339(expiration));
    creds->set("SessionToken", shcore::Value(""));

    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script(creds);

    const auto config = Config_builder{}.profile(k_profile_name).build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ("", credentials->session_token());
    EXPECT_TRUE(credentials->temporary());
    EXPECT_TRUE(credentials->expired());
    EXPECT_EXPIRATION(expiration, credentials->expiration());
  }

  {
    // session token set to some value
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script(valid_process_creds(true));

    const auto config = Config_builder{}.profile(k_profile_name).build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ(k_session_token, credentials->session_token());
    EXPECT_FALSE(credentials->temporary());
    EXPECT_FALSE(credentials->expired());
    EXPECT_EQ(Aws_credentials::NO_EXPIRATION, credentials->expiration());
  }

  {
    // session token set to some value + expiration
    const auto expiration =
        Aws_credentials::Clock::now() + std::chrono::minutes(10);

    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script(
        valid_process_creds(true, shcore::time_point_to_rfc3339(expiration)));

    const auto config = Config_builder{}.profile(k_profile_name).build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ(k_session_token, credentials->session_token());
    EXPECT_TRUE(credentials->temporary());
    EXPECT_FALSE(credentials->expired());
    EXPECT_EXPIRATION(expiration, credentials->expiration());
  }

  {
    // expiration
    const auto expiration =
        Aws_credentials::Clock::now() + std::chrono::minutes(10);
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script(
        valid_process_creds(false, shcore::time_point_to_rfc3339(expiration)));

    const auto config = Config_builder{}.profile(k_profile_name).build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ("", credentials->session_token());
    EXPECT_TRUE(credentials->temporary());
    EXPECT_FALSE(credentials->expired());
    EXPECT_EXPIRATION(expiration, credentials->expiration());
  }

  {
    // partial credentials, missing access key
    const auto creds = valid_process_creds();
    creds->erase("AccessKeyId");

    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script(creds);

    EXPECT_THROW(Config_builder{}.profile(k_profile_name).build(),
                 std::runtime_error);
  }

  {
    // partial credentials, missing secret key
    const auto creds = valid_process_creds();
    creds->erase("SecretAccessKey");

    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script(creds);

    EXPECT_THROW(Config_builder{}.profile(k_profile_name).build(),
                 std::runtime_error);
  }

  {
    // invalid process
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(invalid_process_config());

    EXPECT_THROW(Config_builder{}.profile(k_profile_name).build(),
                 std::runtime_error);
  }

  {
    // non-zero exit code
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script(py_print_json(valid_process_creds()) +
                              "import sys\n"
                              "sys.exit(1)\n");

    EXPECT_THROW(Config_builder{}.profile(k_profile_name).build(),
                 std::runtime_error);
  }

  {
    // empty output
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script("");

    EXPECT_THROW(Config_builder{}.profile(k_profile_name).build(),
                 std::runtime_error);
  }

  {
    // JSON syntax error
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script("print('''{ 'SecretAccessKey' 1 }''')");

    EXPECT_THROW(Config_builder{}.profile(k_profile_name).build(),
                 std::runtime_error);
  }

  {
    // JSON is not an object
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script("print('''[ \"SecretAccessKey\", 'value' ]''')");

    EXPECT_THROW(Config_builder{}.profile(k_profile_name).build(),
                 std::runtime_error);
  }

  {
    // no Version
    const auto creds = valid_process_creds();
    creds->erase("Version");

    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script(creds);

    EXPECT_THROW(Config_builder{}.profile(k_profile_name).build(),
                 std::runtime_error);
  }

  {
    // Version is not an int
    const auto creds = valid_process_creds();
    creds->set("Version", shcore::Value("1"));

    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script(creds);

    EXPECT_THROW(Config_builder{}.profile(k_profile_name).build(),
                 std::runtime_error);
  }

  {
    // unsupported Version
    const auto creds = valid_process_creds();
    creds->set("Version", shcore::Value(2));

    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script(creds);

    EXPECT_THROW(Config_builder{}.profile(k_profile_name).build(),
                 std::runtime_error);
  }

  {
    // AccessKeyId is not a string
    const auto creds = valid_process_creds();
    creds->set("AccessKeyId", shcore::Value(1));

    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script(creds);

    EXPECT_THROW(Config_builder{}.profile(k_profile_name).build(),
                 std::runtime_error);
  }

  {
    // SecretAccessKey is not a string
    const auto creds = valid_process_creds();
    creds->set("SecretAccessKey", shcore::Value(1));

    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script(creds);

    EXPECT_THROW(Config_builder{}.profile(k_profile_name).build(),
                 std::runtime_error);
  }

  {
    // SessionToken is not a string
    const auto creds = valid_process_creds();
    creds->set("SessionToken", shcore::Value(1));

    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script(creds);

    EXPECT_THROW(Config_builder{}.profile(k_profile_name).build(),
                 std::runtime_error);
  }

  {
    // Expiration is not a string
    const auto creds = valid_process_creds();
    creds->set("Expiration", shcore::Value(1));

    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_process_config());
    c += write_process_script(creds);

    EXPECT_THROW(Config_builder{}.profile(k_profile_name).build(),
                 std::runtime_error);
  }
}

TEST_F(Aws_s3_bucket_config_test, config_file_credentials) {
  {
    // config file
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_config());

    const auto config = Config_builder{}.profile(k_profile_name).build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ("", credentials->session_token());
  }

  {
    // session token set to an empty string
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_config() + "aws_session_token=\n");

    const auto config = Config_builder{}.profile(k_profile_name).build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ("", credentials->session_token());
  }

  {
    // session token set to some value
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config(valid_config(true));

    const auto config = Config_builder{}.profile(k_profile_name).build();
    const auto credentials = config->credentials_provider()->credentials();

    ASSERT_NE(nullptr, credentials);
    EXPECT_EQ(k_access_key, credentials->access_key_id());
    EXPECT_EQ(k_secret_key, credentials->secret_access_key());
    EXPECT_EQ(k_session_token, credentials->session_token());
  }

  {
    // partial credentials, missing access key
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config("[default]\naws_secret_access_key=key");

    EXPECT_THROW(Config_builder{}.build(), std::runtime_error);
  }

  {
    // partial credentials, missing secret key
    Cleanup c;
    c += clear_aws_env_vars();
    c += write_default_config("[default]\naws_access_key_id=key");

    EXPECT_THROW(Config_builder{}.build(), std::runtime_error);
  }
}

TEST_F(Aws_s3_bucket_config_test, no_credentials) {
  Cleanup c;
  c += clear_aws_env_vars();
  c += write_default_credentials(invalid_config());
  c += write_default_config(invalid_config());

  EXPECT_THROW(Config_builder{}.build(), std::runtime_error);
}

}  // namespace aws
}  // namespace mysqlshdk
