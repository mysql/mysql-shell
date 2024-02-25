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
#include "modules/util/upgrade_checker/upgrade_check_condition.h"
#include "unittest/modules/util/upgrade_checker/test_utils.h"
#include "unittest/test_utils.h"

namespace mysqlsh {
namespace upgrade_checker {

namespace {

void test_basic_version_scenarios(const std::forward_list<Version> &versions) {
  Version_condition condition(versions);

  for (const auto &version : versions) {
    // Condition does not met if source and target are below the version
    EXPECT_FALSE(condition.evaluate(
        upgrade_info(before_version(version, 2), before_version(version, 1))));

    // Condition mets if target version matches the version
    EXPECT_TRUE(
        condition.evaluate(upgrade_info(before_version(version), version)));

    // Condition mets if version is between source and target
    EXPECT_TRUE(condition.evaluate(
        upgrade_info(before_version(version), after_version(version))));

    // Condition does not met if source is version
    EXPECT_FALSE(
        condition.evaluate(upgrade_info(version, after_version(version))));

    // Condition does not met if source is above version
    EXPECT_FALSE(condition.evaluate(
        upgrade_info(after_version(version), after_version(version, 2))));
  }
}

}  // namespace

TEST(Version_condition, test_single_version) {
  test_basic_version_scenarios({Version(8, 4, 0)});
}

TEST(Version_condition, test_multi_version) {
  // The base scenarios met for any registered version
  test_basic_version_scenarios({Version(8, 3, 0), Version(8, 4, 0)});

  // Condition mets if any or more registered versions are between source and
  // target
  Version_condition condition({Version(8, 3, 0), Version(8, 4, 0)});

  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 2, 0), Version(8, 5, 0))));

  // Condition does not met if no version is between source and target
  EXPECT_FALSE(
      condition.evaluate(upgrade_info(Version(8, 3, 1), Version(8, 3, 9))));
}

TEST(Version_condition, test_custom_condition) {
  Version ver(8, 0, 20);
  std::string test_string = "Some_text_to_test";

  {
    Custom_condition condition(
        [=](const Upgrade_info &info) { return info.target_version < ver; },
        test_string);

    EXPECT_TRUE(condition.evaluate(upgrade_info(ver, before_version(ver, 1))));

    EXPECT_FALSE(condition.evaluate(upgrade_info(ver, ver)));

    EXPECT_STREQ(test_string.c_str(), condition.description().c_str());
  }

  {
    auto temp_info = upgrade_info(ver, ver);
    Custom_condition condition(
        [](const Upgrade_info &info) {
          return shcore::str_beginswith(info.server_os, "WIN");
        },
        test_string);

    temp_info.server_os = "WIN32";
    EXPECT_TRUE(condition.evaluate(temp_info));

    temp_info.server_os = "WIN64";
    EXPECT_TRUE(condition.evaluate(temp_info));

    temp_info.server_os = "Linux";
    EXPECT_FALSE(condition.evaluate(temp_info));

    EXPECT_STREQ(test_string.c_str(), condition.description().c_str());
  }
}

TEST(Life_cycle_condition, test_full_spec) {
  Life_cycle_condition condition(Version(8, 2, 0), Version(8, 3, 0),
                                 Version(8, 4, 0));

  // 1: Condition is not met if upgrade starts before the feature was
  // available, no matter the target version
  EXPECT_FALSE(
      condition.evaluate(upgrade_info(Version(8, 1, 0), Version(8, 1, 5))));
  EXPECT_FALSE(
      condition.evaluate(upgrade_info(Version(8, 1, 0), Version(8, 2, 0))));
  EXPECT_FALSE(
      condition.evaluate(upgrade_info(Version(8, 1, 0), Version(8, 3, 0))));
  EXPECT_FALSE(
      condition.evaluate(upgrade_info(Version(8, 1, 0), Version(8, 4, 0))));
  EXPECT_FALSE(
      condition.evaluate(upgrade_info(Version(8, 1, 0), Version(8, 5, 0))));

  // 2: Condition is not met if upgrade starts after the feature was removed
  EXPECT_FALSE(
      condition.evaluate(upgrade_info(Version(8, 4, 0), Version(8, 5, 0))));
  EXPECT_FALSE(
      condition.evaluate(upgrade_info(Version(8, 4, 1), Version(8, 5, 0))));

  // 3: Condition is met in any other case (start version is within the
  // lifecycle)

  // 3.1 Verifying after feature introduction
  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 2, 0), Version(8, 5, 0))));
  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 2, 1), Version(8, 5, 0))));

  // 3.2 Verifying after feature deprecation
  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 3, 0), Version(8, 5, 0))));
  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 3, 1), Version(8, 5, 0))));

  // 4: Condition is met even if the upgrade does not cross the deprecation
  // version or removal versions

  // 4.1: Met when upgrade is before deprecation
  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 2, 0), Version(8, 2, 1))));

  // 4.2: Met when upgrade is after deprecation but before removal
  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 3, 0), Version(8, 3, 1))));
}

TEST(Life_cycle_condition, test_nog_start) {
  // 1: If no start version defined, condition is met as soon as it doesn't
  // start after the removal (same tests as in 1 in previous test but with
  // opposite result)
  Life_cycle_condition condition({}, Version(8, 3, 0), Version(8, 4, 0));

  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 1, 0), Version(8, 1, 5))));
  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 1, 0), Version(8, 2, 0))));
  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 1, 0), Version(8, 3, 0))));
  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 1, 0), Version(8, 4, 0))));
  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 1, 0), Version(8, 5, 0))));
}

TEST(Life_cycle_condition, test_no_start_no_deprecation) {
  // Condition is met as long as the start version is < than the removal
  // version
  Life_cycle_condition condition({}, {}, Version(8, 4, 0));

  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(5, 7, 0), Version(8, 1, 5))));
  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 0, 0), Version(8, 2, 0))));
  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 3, 0), Version(8, 3, 0))));
  EXPECT_FALSE(
      condition.evaluate(upgrade_info(Version(8, 4, 0), Version(8, 4, 1))));
}

TEST(Life_cycle_condition, test_no_start_no_removal) {
  // Condition is met independently of the upgrade start and end versions
  Life_cycle_condition condition({}, Version(8, 4, 0), {});

  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(5, 7, 0), Version(8, 1, 5))));
  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 0, 0), Version(8, 2, 0))));
  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 4, 0), Version(8, 4, 1))));
  EXPECT_TRUE(
      condition.evaluate(upgrade_info(Version(8, 5, 0), Version(8, 5, 1))));
}

TEST(Life_cycle_condition, test_description) {
  {
    Life_cycle_condition condition(Version(8, 2, 0), Version(8, 3, 0),
                                   Version(8, 4, 0));
    EXPECT_STREQ(condition.description().c_str(),
                 "Server version is at least 8.2.0 and lower than 8.4.0 and "
                 "the target version is at least 8.3.0");
  }
  {
    Life_cycle_condition condition(Version(8, 2, 0), Version(8, 3, 0), {});
    EXPECT_STREQ(condition.description().c_str(),
                 "Server version is at least 8.2.0 and the target version is "
                 "at least 8.3.0");
  }
  {
    Life_cycle_condition condition(Version(8, 2, 0), {}, Version(8, 4, 0));
    EXPECT_STREQ(condition.description().c_str(),
                 "Server version is at least 8.2.0 and lower than 8.4.0 and "
                 "the target version is at least 8.4.0");
  }

  {
    Life_cycle_condition condition({}, Version(8, 3, 0), Version(8, 4, 0));
    EXPECT_STREQ(condition.description().c_str(),
                 "Server version is lower than 8.4.0 and the target version is "
                 "at least 8.3.0");
  }
  {
    Life_cycle_condition condition({}, Version(8, 3, 0), {});
    EXPECT_STREQ(condition.description().c_str(),
                 "Target version is at least 8.3.0");
  }
  {
    Life_cycle_condition condition({}, {}, Version(8, 4, 0));
    EXPECT_STREQ(condition.description().c_str(),
                 "Server version is lower than 8.4.0 and the target version is "
                 "at least 8.4.0");
  }
}

}  // namespace upgrade_checker

}  // namespace mysqlsh
