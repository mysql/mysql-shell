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

#include "modules/util/upgrade_checker/common.h"
#include "modules/util/upgrade_checker/feature_life_cycle_check.h"
#include "modules/util/upgrade_checker/upgrade_check_creators.h"
#include "unittest/modules/util/upgrade_checker/test_utils.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"

namespace mysqlsh {
namespace upgrade_checker {

TEST(Upgrade_checker_test_common, get_issue_level) {
  Feature_definition feature{
      "some", Version(8, 0, 0), Version(8, 0, 30), Version(8, 2, 0), {}};

  {
    // Testing UC before feature was available
    Upgrade_info info;
    info.server_version = Version(5, 7, 44);
    EXPECT_THROW(get_issue_level(feature, info), std::logic_error);
  }
  {
    // Testing UC when feature was removed
    Upgrade_info info;
    info.server_version = Version(8, 2, 0);
    EXPECT_THROW(get_issue_level(feature, info), std::logic_error);
  }
  {
    // Testing UC after feature was removed
    Upgrade_info info;
    info.server_version = Version(8, 2, 1);
    EXPECT_THROW(get_issue_level(feature, info), std::logic_error);
  }
  {
    // Testing UC to before feature was deprecated
    Upgrade_info info;
    info.server_version = Version(8, 0, 0);
    info.target_version = Version(8, 0, 29);
    EXPECT_EQ(Upgrade_issue::Level::NOTICE, get_issue_level(feature, info));
  }
  {
    // Testing UC to when feature was deprecated
    Upgrade_info info;
    info.server_version = Version(8, 0, 0);
    info.target_version = Version(8, 0, 30);
    EXPECT_EQ(Upgrade_issue::Level::WARNING, get_issue_level(feature, info));
  }
  {
    // Testing UC to after feature was deprecated but before removed
    Upgrade_info info;
    info.server_version = Version(8, 0, 0);
    info.target_version = Version(8, 0, 31);
    EXPECT_EQ(Upgrade_issue::Level::WARNING, get_issue_level(feature, info));
  }
  {
    // Testing UC to when feature was removed
    Upgrade_info info;
    info.server_version = Version(8, 0, 0);
    info.target_version = Version(8, 2, 0);
    EXPECT_EQ(Upgrade_issue::Level::ERROR, get_issue_level(feature, info));
  }
  {
    // Testing UC to after feature was removed
    Upgrade_info info;
    info.server_version = Version(8, 0, 0);
    info.target_version = Version(8, 3, 0);
    EXPECT_EQ(Upgrade_issue::Level::ERROR, get_issue_level(feature, info));
  }

  {
    // Feature with no start will give NOTICE for any upgrade before deprecation
    Feature_definition feature_no_start{
        "some", {}, Version(8, 0, 30), Version(8, 2, 0), {}};
    Upgrade_info info;
    info.server_version = Version(0, 0, 0);
    info.target_version = Version(8, 0, 29);
    EXPECT_EQ(Upgrade_issue::Level::NOTICE,
              get_issue_level(feature_no_start, info));
  }

  {
    // Feature with no deprecation will give WARNING for any upgrade before
    // removal
    Feature_definition feature_no_start_no_deprecation{
        "some", {}, {}, Version(8, 2, 0), {}};

    Feature_definition feature_no_deprecation{
        "some", Version(8, 0, 0), {}, Version(8, 2, 0), {}};

    Upgrade_info info;
    info.server_version = Version(0, 0, 0);
    info.target_version = Version(8, 0, 29);
    EXPECT_EQ(Upgrade_issue::Level::WARNING,
              get_issue_level(feature_no_start_no_deprecation, info));

    // Adjust the start to be after feature was introduced
    info.server_version = Version(8, 0, 0);
    EXPECT_EQ(Upgrade_issue::Level::WARNING,
              get_issue_level(feature_no_deprecation, info));
  }

  {
    // Feature with no removal will give WARNING for any upgrade after
    // deprecation

    Feature_definition feature_no_removal{
        "some", Version(8, 0, 0), Version(8, 2, 0), {}, {}};

    Upgrade_info info;
    info.server_version = Version(8, 0, 0);
    info.target_version = Version(99, 99, 99);
    EXPECT_EQ(Upgrade_issue::Level::WARNING,
              get_issue_level(feature_no_removal, info));
  }
}

TEST(Auth_method_usage_check, enabled_and_features) {
  {
    // UC Start version is after all registered features were removed
    Upgrade_info info;
    info.server_version = Version(8, 4, 0);
    Auth_method_usage_check check(info);
    EXPECT_FALSE(check.enabled());
  }
  {
    // UC Start version before introduction of fido_authentication
    Upgrade_info info;
    info.server_version = Version(8, 0, 26);
    Auth_method_usage_check check(info);
    EXPECT_TRUE(check.enabled());
    EXPECT_TRUE(check.has_feature("sha256_password"));
    EXPECT_TRUE(check.has_feature("mysql_native_password"));
    EXPECT_FALSE(check.has_feature("authentication_fido"));
  }
  {
    // UC Start version after introduction of fido_authentication
    Upgrade_info info;
    info.server_version = Version(8, 0, 27);
    Auth_method_usage_check check(info);
    EXPECT_TRUE(check.enabled());
    EXPECT_TRUE(check.has_feature("sha256_password"));
    EXPECT_TRUE(check.has_feature("mysql_native_password"));
    EXPECT_TRUE(check.has_feature("authentication_fido"));
  }
}

TEST_F(Upgrade_checker_test, auth_method_usage_check_query) {
  // This test ensures the query for auth method usage works for both first
  // factor and second factor authentication, for the latter, it uses webauthn
  // authentication which is available 8.2.0
  SKIP_IF_SERVER_LOWER_THAN("8.2.0");

  Upgrade_info server_info;
  server_info.server_version = _target_server_version;

  int sb_port = 0;
  auto uri = deploy_sandbox(
      shcore::make_dict("plugin-load-add", "authentication_webauthn.so",
                        "authentication-webauthn-rp-id", "mysql.com"),
      &sb_port);

  ASSERT_FALSE(uri.empty());

  const auto session = create_mysql_session(uri);

  {
    // Test creating an auth method check, by marking "caching_sha2_password" as
    // deprecated
    Auth_method_usage_check check(server_info);
    check.add_feature({"caching_sha2_password",
                       {},
                       after_version(_target_server_version),
                       {},
                       {}},
                      server_info);

    // Identify users by 1st authentication factor (plugin column)
    auto result = session->query(check.build_query(server_info));

    bool found_user = false;
    auto record = result->fetch_one();
    while (record && !found_user) {
      if (record->get_string(0) == "root@localhost") {
        EXPECT_STREQ("[\"caching_sha2_password\"]",
                     record->get_string(1).c_str());
        found_user = true;
      } else {
        record = result->fetch_one();
      }
    }

    EXPECT_TRUE(found_user);
  }

  {
    // Test creating an auth method check, by marking "authentication_webauthn"
    // as deprecated
    Auth_method_usage_check check(server_info);
    check.add_feature({"authentication_webauthn",
                       {},
                       after_version(_target_server_version),
                       {},
                       {}},
                      server_info);

    // Negative case, no users using webauthn
    auto webauthn_query = check.build_query(server_info);
    auto result = session->query(webauthn_query);
    EXPECT_EQ(nullptr, result->fetch_one());

    // Now verifies user through 2nd auth factor
    session->execute(
        "create user webauthntest identified by 'mypwd' and identified with "
        "authentication_webauthn");

    result = session->query(webauthn_query);
    auto record = result->fetch_one();
    EXPECT_STREQ("webauthntest@%", record->get_string(0).c_str());
    EXPECT_STREQ("[\"caching_sha2_password\", \"authentication_webauthn\"]",
                 record->get_string(1).c_str());
    record = result->fetch_one();
    EXPECT_EQ(nullptr, record);
  }

  session->close();

  testutil->destroy_sandbox(sb_port, true);
}

namespace {
using Records = std::vector<std::vector<std::string>>;

void set_expectation(testing::Mock_session *session, const std::string &query,
                     const Records &records) {
  auto &result = session->expect_query(query).then(
      {"user"});  // Doesn't matter, column name is not used

  for (const auto &record : records) {
    result.add_row(record);
  }
}

// Level, Description, Link
using Issue_expect = std::tuple<Upgrade_issue::Level, std::string, std::string>;
using Issue_expect_list = std::vector<Issue_expect>;
using Versions = std::vector<std::pair<Version, Version>>;

void test_feature_check(
    const std::string &test_id,
    std::function<std::unique_ptr<Upgrade_check>(const Upgrade_info &info)>
        creator,
    const Versions &cases, const Records &records,
    const Issue_expect_list &expect) {
  auto msession = std::make_shared<testing::Mock_session>();

  for (const auto &versions : cases) {
    auto ui = upgrade_info(std::get<0>(versions), std::get<1>(versions));

    auto check = creator(ui);
    auto feature_check = dynamic_cast<Feature_life_cycle_check *>(check.get());

    set_expectation(msession.get(), feature_check->build_query(ui), records);

    auto issues = feature_check->run(msession, ui);

    SCOPED_TRACE(shcore::str_format(
        "Testing  %s with source as %s and target as %s", test_id.c_str(),
        ui.server_version.get_base().c_str(),
        ui.target_version.get_base().c_str()));

    EXPECT_EQ(expect.size(), issues.size());
    for (size_t index = 0; index < expect.size(); index++) {
      const auto level = std::get<0>(expect[index]);
      const auto &message = std::get<1>(expect[index]);
      const auto &doclink = std::get<2>(expect[index]);

      EXPECT_EQ(level, issues[index].level);
      EXPECT_STREQ(message.c_str(), issues[index].description.c_str());
      EXPECT_STREQ(doclink.c_str(), issues[index].doclink.c_str());
    }
  }
}

/**
 * @brief Returns the start/target versions which used in an UC operation would
 * result in a notice for the feature
 *
 * @param feature The feature to be used as reference
 * @return Versions The list of start/target version pairs
 */
Versions get_notice_versions(const Feature_definition &feature) {
  Versions versions;

  if (feature.start.has_value()) {
    auto start = *feature.start;
    if (feature.deprecated.has_value()) {
      auto deprecated = *feature.deprecated;
      versions.emplace_back(start, before_version(deprecated));
      versions.emplace_back(after_version(start), before_version(deprecated));
    }
  }

  return versions;
}

/**
 * @brief Returns the start/target versions which used in an UC operation would
 * result in a warning for the feature
 *
 * @param feature The feature to be used as reference
 * @return Versions The list of start/target version pairs
 */
Versions get_warning_versions(const Feature_definition &feature) {
  Versions versions;

  if (feature.start.has_value()) {
    auto start = *feature.start;

    if (feature.deprecated.has_value()) {
      auto deprecated = *feature.deprecated;
      versions.emplace_back(start, deprecated);
      versions.emplace_back(after_version(start), deprecated);
      versions.emplace_back(start, after_version(deprecated));
      versions.emplace_back(after_version(start), after_version(deprecated));
    }

    if (feature.removed.has_value()) {
      auto removed = *feature.removed;
      versions.emplace_back(start, before_version(removed));
      versions.emplace_back(after_version(start), before_version(removed));
    }
  } else {
    if (feature.deprecated.has_value()) {
      auto deprecated = *feature.deprecated;
      versions.emplace_back(Version(0, 0, 0), deprecated);
      versions.emplace_back(Version(0, 0, 0), after_version(deprecated));
    }
    if (feature.removed.has_value()) {
      auto removed = *feature.removed;
      versions.emplace_back(Version(0, 0, 0), before_version(removed));
    } else {
      versions.emplace_back(Version(0, 0, 0), Version(99, 99, 99));
    }
  }

  if (feature.deprecated.has_value()) {
    auto deprecated = *feature.deprecated;
    versions.emplace_back(before_version(deprecated), deprecated);
    versions.emplace_back(before_version(deprecated),
                          after_version(deprecated));

    if (feature.removed.has_value()) {
      auto removed = *feature.removed;
      versions.emplace_back(before_version(deprecated),
                            before_version(removed));
      versions.emplace_back(deprecated, before_version(removed));
      versions.emplace_back(after_version(deprecated), before_version(removed));
    } else {
      versions.emplace_back(before_version(deprecated), Version(99, 99, 99));
      versions.emplace_back(deprecated, Version(99, 99, 99));
      versions.emplace_back(after_version(deprecated), Version(99, 99, 99));
    }
  }

  return versions;
}

/**
 * @brief Returns the start/target versions which used in an UC operation would
 * result in a error for the feature
 *
 * @param feature The feature to be used as reference
 * @return Versions The list of start/target version pairs
 */
Versions get_error_versions(const Feature_definition &feature) {
  Versions versions;

  if (feature.removed.has_value()) {
    auto removed = *feature.removed;

    if (feature.start.has_value()) {
      auto start = *feature.start;
      versions.emplace_back(start, removed);
      versions.emplace_back(after_version(start), removed);
      versions.emplace_back(start, after_version(removed));
      versions.emplace_back(after_version(start), after_version(removed));
    }

    if (feature.deprecated.has_value()) {
      auto deprecated = *feature.deprecated;
      versions.emplace_back(before_version(deprecated), removed);
      versions.emplace_back(before_version(deprecated), after_version(removed));
      versions.emplace_back(deprecated, removed);
      versions.emplace_back(deprecated, after_version(removed));
      versions.emplace_back(after_version(deprecated), removed);
      versions.emplace_back(after_version(deprecated), after_version(removed));
    }
  }

  return versions;
}

using Issue_tracking = std::map<std::string, bool>;
void validate_expected(Upgrade_issue::Level level,
                       const Issue_tracking &expected,
                       const Issue_tracking &actual) {
  for (const auto &plugin : actual) {
    std::string context;

    if (plugin.second) {
      context = shcore::str_format(
          "Plugin %s got some %ss but they are unexpected.",
          plugin.first.c_str(), Upgrade_issue::level_to_string(level));
    } else {
      context = shcore::str_format("Plugin %s missed expected %ss.",
                                   plugin.first.c_str(),
                                   Upgrade_issue::level_to_string(level));
    }

    SCOPED_TRACE(context.c_str());
    EXPECT_EQ(expected.at(plugin.first), plugin.second);
  }
}

const std::map<std::string, std::string> k_plugin_doclink = {
    {"authentication_fido",
     "https://dev.mysql.com/doc/refman/8.3/en/"
     "webauthn-pluggable-authentication.html"},
    {"sha256_password",
     "https://dev.mysql.com/doc/refman/8.0/en/"
     "caching-sha2-pluggable-authentication.html"},
    {"mysql_native_password",
     "https://dev.mysql.com/doc/refman/8.0/en/"
     "caching-sha2-pluggable-authentication.html"},
    {"keyring_file",
     "https://dev.mysql.com/doc/refman/8.0/en/keyring-file-component.html"},
    {"keyring_encrypted_file",
     "https://dev.mysql.com/doc/refman/8.0/en/"
     "keyring-encrypted-file-component.html"},
    {"keyring_oci",
     "https://dev.mysql.com/doc/mysql-security-excerpt/8.3/en/"
     "keyring-oci-plugin.html"},
};

}  // namespace

TEST(Auth_method_usage_check, notices) {
  Upgrade_info info;
  Auth_method_usage_check check(info);

  std::map<std::string, bool> tested_plugins = {
      {"authentication_fido", false},
      {"sha256_password", false},
      {"mysql_native_password", false}};

  auto features = check.get_features();
  for (const auto feature : features) {
    auto versions = get_notice_versions(*feature);
    if (!versions.empty()) {
      // Ensures the test is updated with new auth plugins
      EXPECT_NO_THROW(tested_plugins.at(feature->id));
      tested_plugins[feature->id] = true;

      Records records = {
          {"sample@localhost", "[\"" + feature->id + "\"]"},
          {"another@localhost",
           "[\"caching_sha2_password\", \"" + feature->id + "\"]"}};
      auto notice = shcore::str_format(
          "Notice: The following users are using the '%s' "
          "authentication method which will be deprecated as of MySQL %s.\n"
          "Consider switching the users to a different authentication method "
          "(i.e. %s).\n"
          "\n"
          "sample@localhost\n"
          "another@localhost\n",
          feature->id.c_str(), (*feature->deprecated).get_base().c_str(),
          (*feature->replacement).c_str());

      auto msession = std::make_shared<testing::Mock_session>();
      test_feature_check(
          shcore::str_format("Notice in %s", feature->id.c_str()),
          &get_auth_method_usage_check, versions, records,
          {{Upgrade_issue::Level::NOTICE, notice,
            k_plugin_doclink.at(feature->id)}});
    }
  }

  validate_expected(
      Upgrade_issue::Level::NOTICE,
      {{"authentication_fido", true},
       {"sha256_password", false},         // No start version so no notices
       {"mysql_native_password", false}},  // No start version so no notices
      tested_plugins);
}

TEST(Auth_method_usage_check, warnings) {
  Upgrade_info info;
  Auth_method_usage_check check(info);

  std::map<std::string, bool> tested_plugins = {
      {"authentication_fido", false},
      {"sha256_password", false},
      {"mysql_native_password", false}};

  auto features = check.get_features();
  for (const auto feature : features) {
    auto versions = get_warning_versions(*feature);
    if (!versions.empty()) {
      // Ensures the test is updated with new auth plugins
      EXPECT_NO_THROW(tested_plugins.at(feature->id));
      tested_plugins[feature->id] = true;

      Records records = {
          {"sample@localhost", "[\"" + feature->id + "\"]"},
          {"another@localhost",
           "[\"caching_sha2_password\", \"" + feature->id + "\"]"}};
      auto warning = shcore::str_format(
          "Warning: The following users are using the '%s' "
          "authentication method which is deprecated as of MySQL %s and will "
          "be removed in a future release.\n"
          "Consider switching the users to a different authentication method "
          "(i.e. %s).\n"
          "\n"
          "sample@localhost\n"
          "another@localhost\n",
          feature->id.c_str(), (*feature->deprecated).get_base().c_str(),
          (*feature->replacement).c_str());

      auto msession = std::make_shared<testing::Mock_session>();
      test_feature_check(
          shcore::str_format("Warning in %s", feature->id.c_str()),
          &get_auth_method_usage_check, versions, records,
          {{Upgrade_issue::Level::WARNING, warning,
            k_plugin_doclink.at(feature->id)}});
    }
  }

  validate_expected(Upgrade_issue::Level::WARNING,
                    {{"authentication_fido", true},
                     {"sha256_password", true},
                     {"mysql_native_password", true}},
                    tested_plugins);
}

TEST(Auth_method_usage_check, errors) {
  Upgrade_info info;
  Auth_method_usage_check check(info);

  std::map<std::string, bool> tested_plugins = {
      {"authentication_fido", false},
      {"sha256_password", false},
      {"mysql_native_password", false}};

  auto features = check.get_features();
  for (const auto feature : features) {
    auto versions = get_error_versions(*feature);
    if (!versions.empty()) {
      // Ensures the test is updated with new auth plugins
      EXPECT_NO_THROW(tested_plugins.at(feature->id));
      tested_plugins[feature->id] = true;

      Records records = {
          {"sample@localhost", "[\"" + feature->id + "\"]"},
          {"another@localhost",
           "[\"caching_sha2_password\", \"" + feature->id + "\"]"}};
      auto error = shcore::str_format(
          "Error: The following users are using the '%s' "
          "authentication method which is removed as of MySQL %s.\n"
          "The users must be deleted or re-created with a different "
          "authentication method (i.e. %s).\n"
          "\n"
          "sample@localhost\n"
          "another@localhost\n",
          feature->id.c_str(), (*feature->removed).get_base().c_str(),
          (*feature->replacement).c_str());

      test_feature_check(shcore::str_format("Error in %s", feature->id.c_str()),
                         &get_auth_method_usage_check, versions, records,
                         {{Upgrade_issue::Level::ERROR, error,
                           k_plugin_doclink.at(feature->id)}});
    }
  }

  validate_expected(Upgrade_issue::Level::WARNING,
                    {{"authentication_fido", true},
                     {"sha256_password", true},
                     {"mysql_native_password", true}},
                    tested_plugins);
}

TEST(Auth_method_usage_check, mixed) {
  Records records = {{"sample@localhost", "[\"sha256_password\"]"},
                     {"another@localhost",
                      "[\"mysql_native_password\", \"authentication_fido\"]"}};

  auto msession = std::make_shared<testing::Mock_session>();

  Versions versions;
  versions.emplace_back("8.0.27", "8.0.33");
  test_feature_check(
      "Using the three deprecated auth methods", &get_auth_method_usage_check,
      versions, records,
      {
          {Upgrade_issue::Level::NOTICE,
           "Notice: The following users are using the 'authentication_fido' "
           "authentication method which will be deprecated as of MySQL 8.2.0.\n"
           "Consider switching the users to a different authentication method "
           "(i.e. authentication_webauthn).\n"
           "\n"
           "another@localhost\n",
           "https://dev.mysql.com/doc/refman/8.3/en/"
           "webauthn-pluggable-authentication.html"},
          {Upgrade_issue::Level::WARNING,
           "Warning: The following users are using the 'mysql_native_password' "
           "authentication method which is deprecated as of MySQL 8.0.0 and "
           "will be removed in a future release.\n"
           "Consider switching the users to a different authentication method "
           "(i.e. caching_sha2_password).\n"
           "\n"
           "another@localhost\n",
           "https://dev.mysql.com/doc/refman/8.0/en/"
           "caching-sha2-pluggable-authentication.html"},
          {Upgrade_issue::Level::WARNING,
           "Warning: The following users are using the 'sha256_password' "
           "authentication method which is deprecated as of MySQL 8.0.0 and "
           "will be removed in a future release.\n"
           "Consider switching the users to a different authentication method "
           "(i.e. caching_sha2_password).\n"
           "\n"
           "sample@localhost\n",
           "https://dev.mysql.com/doc/refman/8.0/en/"
           "caching-sha2-pluggable-authentication.html"},
      });
}

TEST(Plugin_usage_check, enabled_and_features) {
  {
    // UC Start version is after all registered features were removed
    Upgrade_info info;
    info.server_version = Version(8, 4, 0);
    Plugin_usage_check check(info);
    EXPECT_FALSE(check.enabled());
  }
  {
    // UC Start version before introduction of fido_authentication
    Upgrade_info info;
    info.server_version = Version(8, 0, 26);
    Plugin_usage_check check(info);
    EXPECT_TRUE(check.enabled());
    EXPECT_TRUE(check.has_feature("keyring_file"));
    EXPECT_TRUE(check.has_feature("keyring_encrypted_file"));
    EXPECT_TRUE(check.has_feature("keyring_oci"));
    EXPECT_FALSE(check.has_feature("authentication_fido"));
  }
  {
    // UC Start version after introduction of fido_authentication
    Upgrade_info info;
    info.server_version = Version(8, 0, 27);
    Plugin_usage_check check(info);
    EXPECT_TRUE(check.enabled());
    EXPECT_TRUE(check.has_feature("keyring_file"));
    EXPECT_TRUE(check.has_feature("keyring_encrypted_file"));
    EXPECT_TRUE(check.has_feature("keyring_oci"));
    EXPECT_TRUE(check.has_feature("authentication_fido"));
  }
}

TEST_F(Upgrade_checker_test, plugin_usage_check_query_negative) {
  // This test ensures the query for plugin usage works for both first
  // factor and second factor authentication, for the latter, it uses webauthn
  // authentication plugn which is available 8.2.0
  SKIP_IF_SERVER_LOWER_THAN("8.2.0");

  Upgrade_info server_info;
  server_info.server_version = _target_server_version;

  {
    // Negative case, the plugin is not loaded
    int sb_port = 0;
    auto uri = deploy_sandbox(shcore::make_dict(), &sb_port);
    ASSERT_FALSE(uri.empty());
    const auto session = create_mysql_session(uri);

    Plugin_usage_check check(server_info);
    check.add_feature({"authentication_webauthn",
                       {},
                       after_version(_target_server_version),
                       {},
                       {}},
                      server_info);

    // Negative case, plugin is not loaded
    auto webauthn_query = check.build_query(server_info);
    auto result = session->query(webauthn_query);
    EXPECT_EQ(nullptr, result->fetch_one());

    session->close();
    testutil->destroy_sandbox(sb_port, true);
  }
}

TEST_F(Upgrade_checker_test, plugin_usage_check_query_positive) {
  // This test ensures the query for plugin usage works for both first
  // factor and second factor authentication, for the latter, it uses webauthn
  // authentication plugn which is available 8.2.0
  SKIP_IF_SERVER_LOWER_THAN("8.2.0");

  Upgrade_info server_info;
  server_info.server_version = _target_server_version;
  {
    // Positive case, the plugin is loaded
    int sb_port = 0;
    auto uri = deploy_sandbox(
        shcore::make_dict("plugin-load-add", "authentication_webauthn.so",
                          "authentication-webauthn-rp-id", "mysql.com"),
        &sb_port);

    ASSERT_FALSE(uri.empty());

    const auto session = create_mysql_session(uri);

    Plugin_usage_check check(server_info);
    check.add_feature({"authentication_webauthn",
                       {},
                       after_version(_target_server_version),
                       {},
                       {}},
                      server_info);

    // Negative case, plugin is not loaded
    auto webauthn_query = check.build_query(server_info);
    auto result = session->query(webauthn_query);
    auto record = result->fetch_one();
    EXPECT_STREQ("authentication_webauthn", record->get_string(0).c_str());
    EXPECT_EQ(nullptr, result->fetch_one());

    session->close();
    testutil->destroy_sandbox(sb_port, true);
  }
}

TEST(Plugin_usage_check, notices) {
  Upgrade_info info;
  Plugin_usage_check check(info);

  std::map<std::string, bool> tested_plugins = {
      {"authentication_fido", false},
      {"keyring_file", false},
      {"keyring_encrypted_file", false},
      {"keyring_oci", false}};

  auto features = check.get_features();
  for (const auto feature : features) {
    auto versions = get_notice_versions(*feature);
    if (!versions.empty()) {
      // Ensures the test is updated with new auth plugins
      EXPECT_NO_THROW(tested_plugins.at(feature->id));
      tested_plugins[feature->id] = true;

      Records records = {{feature->id}};
      auto notice = shcore::str_format(
          "Notice: The '%s' plugin will be deprecated as of MySQL "
          "%s.\nConsider using %s instead.\n",
          feature->id.c_str(), (*feature->deprecated).get_base().c_str(),
          (*feature->replacement).c_str());

      auto msession = std::make_shared<testing::Mock_session>();
      test_feature_check(
          shcore::str_format("Notice in %s", feature->id.c_str()),
          &get_plugin_usage_check, versions, records,
          {{Upgrade_issue::Level::NOTICE, notice,
            k_plugin_doclink.at(feature->id)}});
    }
  }

  validate_expected(
      Upgrade_issue::Level::NOTICE,
      {{"authentication_fido", true},
       {"keyring_file", false},            // No start version so no notices
       {"keyring_encrypted_file", false},  // No start version so no notices
       {"keyring_oci", false}},            // No start version so no notices
      tested_plugins);
}

TEST(Plugin_usage_check, warnings) {
  Upgrade_info info;
  Plugin_usage_check check(info);

  std::map<std::string, bool> tested_plugins = {
      {"authentication_fido", false},
      {"keyring_file", false},
      {"keyring_encrypted_file", false},
      {"keyring_oci", false}};

  auto features = check.get_features();
  for (const auto feature : features) {
    auto versions = get_warning_versions(*feature);
    if (!versions.empty()) {
      // Ensures the test is updated with new auth plugins
      EXPECT_NO_THROW(tested_plugins.at(feature->id));
      tested_plugins[feature->id] = true;

      Records records = {{feature->id}};
      auto notice = shcore::str_format(
          "Warning: The '%s' plugin is deprecated as of MySQL %s and will be "
          "removed in a future release.\nConsider using %s instead.\n",
          feature->id.c_str(), (*feature->deprecated).get_base().c_str(),
          (*feature->replacement).c_str());

      auto msession = std::make_shared<testing::Mock_session>();
      test_feature_check(
          shcore::str_format("Warning in %s", feature->id.c_str()),
          &get_plugin_usage_check, versions, records,
          {{Upgrade_issue::Level::WARNING, notice,
            k_plugin_doclink.at(feature->id)}});
    }
  }

  validate_expected(
      Upgrade_issue::Level::WARNING,
      {{"authentication_fido", true},
       {"keyring_file", true},            // No start version so no notices
       {"keyring_encrypted_file", true},  // No start version so no notices
       {"keyring_oci", true}},            // No start version so no notices
      tested_plugins);
}

TEST(Plugin_usage_check, errors) {
  Upgrade_info info;
  Plugin_usage_check check(info);

  std::map<std::string, bool> tested_plugins = {
      {"authentication_fido", false},
      {"keyring_file", false},
      {"keyring_encrypted_file", false},
      {"keyring_oci", false}};

  auto features = check.get_features();
  for (const auto feature : features) {
    auto versions = get_error_versions(*feature);
    if (!versions.empty()) {
      // Ensures the test is updated with new auth plugins
      EXPECT_NO_THROW(tested_plugins.at(feature->id));
      tested_plugins[feature->id] = true;

      Records records = {{feature->id}};
      auto notice = shcore::str_format(
          "Error: The '%s' plugin is removed as of MySQL %s.\nIt must not be "
          "used anymore, please use %s instead.\n",
          feature->id.c_str(), (*feature->removed).get_base().c_str(),
          (*feature->replacement).c_str());

      auto msession = std::make_shared<testing::Mock_session>();
      test_feature_check(shcore::str_format("Error in %s", feature->id.c_str()),
                         &get_plugin_usage_check, versions, records,
                         {{Upgrade_issue::Level::ERROR, notice,
                           k_plugin_doclink.at(feature->id)}});
    }
  }

  validate_expected(
      Upgrade_issue::Level::ERROR,
      {{"authentication_fido", true},
       {"keyring_file", true},            // No start version so no notices
       {"keyring_encrypted_file", true},  // No start version so no notices
       {"keyring_oci", true}},            // No start version so no notices
      tested_plugins);
}

TEST(Plugin_usage_check, mixed) {
  Records records = {{"authentication_fido"},
                     {"keyring_file"},
                     {"keyring_encrypted_file"},
                     {"keyring_oci"}};

  auto msession = std::make_shared<testing::Mock_session>();

  Versions versions;
  versions.emplace_back("8.0.27", "8.0.35");
  test_feature_check(
      "Using the four deprecated plugins", &get_plugin_usage_check, versions,
      records,
      {{Upgrade_issue::Level::NOTICE,
        "Notice: The 'authentication_fido' plugin will be deprecated as of "
        "MySQL 8.2.0.\nConsider using 'authentication_webauthn' plugin "
        "instead.\n",
        "https://dev.mysql.com/doc/refman/8.3/en/"
        "webauthn-pluggable-authentication.html"},
       {Upgrade_issue::Level::WARNING,
        "Warning: The 'keyring_encrypted_file' plugin is deprecated as of "
        "MySQL 8.0.34 and will be removed in a future release.\n"
        "Consider using the 'component_encrypted_keyring_file' component "
        "instead.\n",
        "https://dev.mysql.com/doc/refman/8.0/en/"
        "keyring-encrypted-file-component.html"},
       {Upgrade_issue::Level::WARNING,
        "Warning: The 'keyring_file' plugin is deprecated as of MySQL "
        "8.0.34 and will be removed in a future release.\n"
        "Consider using the 'component_keyring_file' component instead.\n",
        "https://dev.mysql.com/doc/refman/8.0/en/keyring-file-component.html"},
       {Upgrade_issue::Level::WARNING,
        "Warning: The 'keyring_oci' plugin is deprecated as of "
        "MySQL 8.0.31 and will be removed in a future release.\n"
        "Consider using the 'component_keyring_oci' component instead.\n",
        "https://dev.mysql.com/doc/mysql-security-excerpt/8.3/en/"
        "keyring-oci-plugin.html"}});
}

}  // namespace upgrade_checker

}  // namespace mysqlsh
