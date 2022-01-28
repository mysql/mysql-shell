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

#include "unittest/gprod_clean.h"

#include "mysqlshdk/libs/aws/aws_config_file.h"

#include "unittest/gtest_clean.h"

#include <string>
#include <vector>

#include "mysqlshdk/libs/utils/utils_file.h"

namespace mysqlshdk {
namespace aws {

class Aws_config_file_test : public testing::Test {
 protected:
  void TearDown() override {
    cleanup();
    Test::TearDown();
  }

  void create_config(const std::string &path,
                     const std::string &contents = {}) {
    shcore::create_file(path, contents.empty() ? R"(# a comment
[ profile xyz ]
;another comment
region=eu-central-1
aws_session_token=
  [default] # comment
aws_access_key_id = id
aws_secret_access_key= key
] not-a=profile [
max_attempts =7

[oci]
aws_access_key_id =id1
aws_secret_access_key=key1
  aws_session_token = token
)"
                                               : contents);

    m_created_files.push_back(path);
  }

 private:
  void cleanup() {
    for (const auto &file : m_created_files) {
      shcore::delete_file(file);
    }
  }

  std::vector<std::string> m_created_files;
};

TEST_F(Aws_config_file_test, missing_file) {
  Aws_config_file config{"some-random-non-existing-file.txt"};

  EXPECT_FALSE(config.has_profile("default"));
  EXPECT_EQ(nullptr, config.get_profile("default"));

  EXPECT_FALSE(config.load());

  EXPECT_FALSE(config.has_profile("default"));
  EXPECT_EQ(nullptr, config.get_profile("default"));
}

TEST_F(Aws_config_file_test, existing_file) {
  const std::string path = "zażółć";
  create_config(path);
  Aws_config_file config{path};

  EXPECT_TRUE(config.load());

  {
    const std::string profile_name = "default";
    EXPECT_TRUE(config.has_profile(profile_name));

    const auto profile = config.get_profile(profile_name);
    ASSERT_NE(nullptr, profile);

    const auto end = profile->settings.end();
    EXPECT_TRUE(profile->access_key_id);
    EXPECT_EQ("id", *profile->access_key_id);
    EXPECT_EQ(end, profile->settings.find("aws_access_key_id"));

    EXPECT_TRUE(profile->secret_access_key);
    EXPECT_EQ("key", *profile->secret_access_key);
    EXPECT_EQ(end, profile->settings.find("aws_secret_access_key"));

    EXPECT_EQ(end, profile->settings.find("aws_session_token"));
    EXPECT_FALSE(profile->session_token);

    EXPECT_EQ(end, profile->settings.find("region"));

    ASSERT_NE(end, profile->settings.find("max_attempts"));
    EXPECT_EQ("7", profile->settings.find("max_attempts")->second);
  }

  {
    const std::string profile_name = "oci";
    EXPECT_TRUE(config.has_profile(profile_name));

    const auto profile = config.get_profile(profile_name);
    ASSERT_NE(nullptr, profile);

    const auto end = profile->settings.end();
    EXPECT_TRUE(profile->access_key_id);
    EXPECT_EQ("id1", *profile->access_key_id);
    EXPECT_EQ(end, profile->settings.find("aws_access_key_id"));

    EXPECT_TRUE(profile->secret_access_key);
    EXPECT_EQ("key1", *profile->secret_access_key);
    EXPECT_EQ(end, profile->settings.find("aws_secret_access_key"));

    EXPECT_TRUE(profile->session_token);
    EXPECT_EQ("token", *profile->session_token);
    EXPECT_EQ(end, profile->settings.find("aws_session_token"));

    EXPECT_EQ(end, profile->settings.find("region"));
  }

  {
    const std::string profile_name = "xyz";
    EXPECT_TRUE(config.has_profile(profile_name));

    const auto profile = config.get_profile(profile_name);
    ASSERT_NE(nullptr, profile);

    const auto end = profile->settings.end();
    EXPECT_FALSE(profile->access_key_id);
    EXPECT_EQ(end, profile->settings.find("aws_access_key_id"));

    EXPECT_FALSE(profile->secret_access_key);
    EXPECT_EQ(end, profile->settings.find("aws_secret_access_key"));

    EXPECT_TRUE(profile->session_token);
    EXPECT_EQ("", *profile->session_token);
    EXPECT_EQ(end, profile->settings.find("aws_session_token"));

    ASSERT_NE(end, profile->settings.find("region"));
    EXPECT_EQ("eu-central-1", profile->settings.find("region")->second);
  }

  EXPECT_FALSE(config.has_profile("missing"));
  EXPECT_EQ(nullptr, config.get_profile("missing"));
}

TEST_F(Aws_config_file_test, malformed_files) {
  {
    const std::string path = "malformed";
    create_config(path, R"(line without profile
[ profile xyz ]
region=eu-central-1
)");
    Aws_config_file config{path};

    EXPECT_THROW_MSG(config.load(), std::runtime_error,
                     "Failed to parse config file '" + path +
                         "': setting without an associated profile, in line 1: "
                         "line without profile");
  }

  {
    const std::string path = "malformed";
    create_config(path, R"([ profile xyz
region=eu-central-1
)");
    Aws_config_file config{path};

    EXPECT_THROW_MSG(
        config.load(), std::runtime_error,
        "Failed to parse config file '" + path +
            "': missing closing square bracket, in line 1: [ profile xyz");
  }

  {
    const std::string path = "malformed";
    create_config(path, R"([ profile xyz ]
region=eu-central-1
not a setting
)");
    Aws_config_file config{path};

    EXPECT_THROW_MSG(
        config.load(), std::runtime_error,
        "Failed to parse config file '" + path +
            "': expected setting-name=value, in line 3: not a setting");
  }
}

}  // namespace aws
}  // namespace mysqlshdk
