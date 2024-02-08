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
#include "unittest/test_utils.h"

namespace mysqlsh {
namespace upgrade_checker {

namespace {

[[maybe_unused]] Upgrade_info upgrade_info(Version server, Version target) {
  Upgrade_info si;

  si.server_version = std::move(server);
  si.target_version = std::move(target);

  return si;
}

Version prev_version(const Version &version) {
  auto major = version.get_major();
  auto minor = version.get_minor();
  auto patch = version.get_patch();

  if (--patch < 0) {
    patch = 99;
    if (--minor < 0) {
      minor = 99;
      if (--major < 0) {
        throw std::logic_error("Invalid version used!");
      }
    }
  }

  return Version(major, minor, patch);
}

Version low_version(const Version &version, int count) {
  Version result = version;
  while (count--) {
    result = prev_version(result);
  }

  return result;
}

Version next_version(const Version &version) {
  auto major = version.get_major();
  auto minor = version.get_minor();
  auto patch = version.get_patch();

  if (++patch > 99) {
    patch = 0;
    if (++minor > 99) {
      minor = 0;
      if (major > 99) {
        throw std::logic_error("Invalid version used!");
      }
    }
  }

  return Version(major, minor, patch);
}

Version up_version(const Version &version, int count) {
  Version result = version;
  while (count--) {
    result = next_version(result);
  }

  return result;
}

void test_basic_version_scenarios(const std::forward_list<Version> &versions) {
  Version_condition condition(versions);

  for (const auto &version : versions) {
    // Condition does not met if source and target are below the version
    EXPECT_FALSE(condition.evaluate(
        upgrade_info(low_version(version, 2), low_version(version, 1))));

    // Condition mets if target version matches the version
    EXPECT_TRUE(
        condition.evaluate(upgrade_info(prev_version(version), version)));

    // Condition mets if version is between source and target
    EXPECT_TRUE(condition.evaluate(
        upgrade_info(prev_version(version), next_version(version))));

    // Condition does not met if source is version
    EXPECT_FALSE(
        condition.evaluate(upgrade_info(version, next_version(version))));

    // Condition does not met if source is above version
    EXPECT_FALSE(condition.evaluate(
        upgrade_info(next_version(version), up_version(version, 2))));
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

  {
    Custom_condition condition(
        [=](const Upgrade_info &info) { return info.target_version < ver; });

    EXPECT_TRUE(condition.evaluate(upgrade_info(ver, low_version(ver, 1))));

    EXPECT_FALSE(condition.evaluate(upgrade_info(ver, ver)));
  }

  {
    auto temp_info = upgrade_info(ver, ver);
    Custom_condition condition([](const Upgrade_info &info) {
      return shcore::str_beginswith(info.server_os, "WIN");
    });

    temp_info.server_os = "WIN32";
    EXPECT_TRUE(condition.evaluate(temp_info));

    temp_info.server_os = "WIN64";
    EXPECT_TRUE(condition.evaluate(temp_info));

    temp_info.server_os = "Linux";
    EXPECT_FALSE(condition.evaluate(temp_info));
  }
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
