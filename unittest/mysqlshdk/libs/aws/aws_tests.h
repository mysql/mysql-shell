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

#ifndef UNITTEST_MYSQLSHDK_LIBS_AWS_AWS_TESTS_H_
#define UNITTEST_MYSQLSHDK_LIBS_AWS_AWS_TESTS_H_

#include "unittest/gtest_clean.h"

#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/aws/s3_bucket.h"
#include "mysqlshdk/libs/aws/s3_bucket_config.h"

#include "unittest/test_utils/shell_test_env.h"

namespace mysqlshdk {
namespace aws {

#define SKIP_IF_NO_AWS_CONFIGURATION \
  do {                               \
    if (should_skip()) {             \
      SKIP_TEST(skip_reason());      \
    }                                \
  } while (false)

class Aws_s3_tests : public testing::Test,
                     public ::testing::WithParamInterface<std::string> {
 protected:
  void SetUp() override {
    Test::SetUp();

    setup_test();
    clean_bucket();
  }

  void TearDown() override {
    clean_bucket();
    Test::TearDown();
  }

  void setup_test() {
    // Note that these object names are in alphabetical order on purpose
    m_objects = {
        "sakila.sql",
        "sakila/actor.csv",
        "sakila/actor_metadata.txt",
        "sakila/address.csv",
        "sakila/address_metadata.txt",
        "sakila/category.csv",
        "sakila/category_metadata.txt",
        "sakila_metadata.txt",
        "sakila_tables.txt",
        "uncommon%25%name.txt",
        "uncommon's name.txt",
    };

    const auto read_var = [this](const char *name, std::string *out,
                                 bool required = true) {
      const auto var = getenv(name);

      if (var) {
        *out = var;
      } else if (required) {
        skip("Missing environment variable: " + std::string(name));
      }
    };

    read_var("MYSQLSH_S3_BUCKET_NAME", &m_bucket_name);
    // we add a suffix to the bucket name, to test virtual-style and path-style
    // access
    m_bucket_name += GetParam();

    read_var("MYSQLSH_AWS_SHARED_CREDENTIALS_FILE", &m_credentials_file, false);
    read_var("MYSQLSH_AWS_CONFIG_FILE", &m_config_file, false);
    read_var("MYSQLSH_AWS_PROFILE", &m_profile, false);
    read_var("MYSQLSH_S3_ENDPOINT_OVERRIDE", &m_endpoint_override, false);

    if (!should_skip()) {
      try {
        S3_bucket bucket(get_config());

        if (!bucket.exists()) {
          skip("The AWS bucket '" + m_bucket_name + "' does not exist");
        }
      } catch (const std::exception &e) {
        skip("Failed to setup the AWS S3 tests: " + std::string{e.what()});
      }
    }
  }

  void create_objects(S3_bucket &bucket) {
    for (const auto &name : m_objects) {
      bucket.put_object(name, "0", 1);
    }
  }

  std::shared_ptr<S3_bucket_config> get_config(const std::string &bucket = {}) {
    const auto options = shcore::make_dict(
        S3_bucket_options::bucket_name_option(),
        bucket.empty() ? m_bucket_name : bucket,
        S3_bucket_options::credentials_file_option(), m_credentials_file,
        S3_bucket_options::config_file_option(), m_config_file,
        S3_bucket_options::profile_option(), m_profile,
        S3_bucket_options::endpoint_override_option(), m_endpoint_override);

    S3_bucket_options parsed_options;
    S3_bucket_options::options().unpack(options, &parsed_options);

    return std::make_shared<S3_bucket_config>(parsed_options);
  }

  void clean_bucket(S3_bucket &bucket, bool clean_uploads = true) {
    auto objects = bucket.list_objects();

    if (!objects.empty()) {
      bucket.delete_objects(objects);

      objects = bucket.list_objects();
      EXPECT_TRUE(objects.empty());
    }

    if (clean_uploads) {
      auto uploads = bucket.list_multipart_uploads();
      if (!uploads.empty()) {
        for (const auto &upload : uploads) {
          bucket.abort_multipart_upload(upload);
        }

        uploads = bucket.list_multipart_uploads();
        EXPECT_TRUE(uploads.empty());
      }
    }
  }

  void skip(const std::string &reason) { m_skip_reasons.emplace_back(reason); }

  bool should_skip() const { return !m_skip_reasons.empty(); }

  std::string skip_reason() const {
    return shcore::str_join(m_skip_reasons, "\n");
  }

  std::string multipart_file_data() const {
    return shcore::get_random_string(k_multipart_file_size, "0123456789ABCDEF");
  }

  static constexpr std::size_t k_min_part_size = 5242880;  // 5 MiB
  static constexpr std::size_t k_multipart_file_size = k_min_part_size + 1024;
  static constexpr std::size_t k_max_part_size = std::min<uint64_t>(
      UINT64_C(5368709120), std::numeric_limits<std::size_t>::max());

  std::vector<std::string> m_objects;

 private:
  void clean_bucket() {
    if (!should_skip()) {
      S3_bucket bucket(get_config());
      clean_bucket(bucket);
    }
  }

  std::vector<std::string> m_skip_reasons;
  std::string m_bucket_name;
  std::string m_credentials_file;
  std::string m_config_file;
  std::string m_profile;
  std::string m_endpoint_override;
};

inline std::string endpoint_override() {
  std::string endpoint;

  if (const auto env = getenv("MYSQLSH_S3_ENDPOINT_OVERRIDE")) {
    endpoint = env;
  }

  return endpoint;
}

inline std::string format_suffix(
    const testing::TestParamInfo<std::string> &info) {
  if (info.param.empty()) {
    return "no_suffix";
  } else {
    return "suffix_" + shcore::str_replace(info.param, ".", "");
  }
}

inline auto suffixes() {
  // if endpoint override is not specified, tests both virtual-style and
  // path-style access, otherwise path-style access is always used and there's
  // no reason to test it twice
  std::vector<std::string> result = {""};

  if (endpoint_override().empty()) {
    result.emplace_back(".new");
  }

  return ::testing::ValuesIn(result);
}

}  // namespace aws
}  // namespace mysqlshdk

#endif  // UNITTEST_MYSQLSHDK_LIBS_AWS_AWS_TESTS_H_
