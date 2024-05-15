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

#include <optional>

#include "unittest/gprod_clean.h"

#include "modules/util/upgrade_checker/common.h"
#include "modules/util/upgrade_checker/sysvar_check.h"
#include "unittest/modules/util/upgrade_checker/test_utils.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"

namespace mysqlsh {
namespace upgrade_checker {

TEST(Upgrade_checker_Sysvar_platform_value, default_resolution) {
  std::optional<std::string> result;
  {  // No values set
    Sysvar_platform_value value;
    result = value.get_default(32);
    EXPECT_FALSE(result.has_value());
    result = value.get_default(64);
    EXPECT_FALSE(result.has_value());
  }

  {  // All applies to both architectures
    Sysvar_platform_value value;
    value.set_default("all");
    result = value.get_default(32);
    EXPECT_STREQ("all", result.value().c_str());
    result = value.get_default(64);
    EXPECT_STREQ("all", result.value().c_str());
  }

  {  // 32 applies to only 32
    Sysvar_platform_value value;
    value.set_default("32", 32);
    result = value.get_default(32);
    EXPECT_STREQ("32", result.value().c_str());
    result = value.get_default(64);
    EXPECT_FALSE(result.has_value());
  }

  {  // 64 applies to only 64
    Sysvar_platform_value value;
    value.set_default("64", 64);
    result = value.get_default(64);
    EXPECT_STREQ("64", result.value().c_str());
    result = value.get_default(32);
    EXPECT_FALSE(result.has_value());
  }

  {  // Specific values are used, all is ignored
    Sysvar_platform_value value;
    value.set_default("all");
    value.set_default("32", 32);
    value.set_default("64", 64);
    result = value.get_default(32);
    EXPECT_STREQ("32", result.value().c_str());
    result = value.get_default(64);
    EXPECT_STREQ("64", result.value().c_str());
  }
}

TEST(Upgrade_checker_Sysvar_platform_value, allowed_resolution) {
  std::optional<std::vector<std::string>> result;
  {  // No values set
    Sysvar_platform_value value;
    result = value.get_allowed(32);
    EXPECT_FALSE(result.has_value());
    result = value.get_allowed(64);
    EXPECT_FALSE(result.has_value());
  }

  {  // All applies to both architectures
    Sysvar_platform_value value;
    value.set_allowed({"all"});
    result = value.get_allowed(32);
    EXPECT_STREQ("all", result.value()[0].c_str());
    result = value.get_allowed(64);
    EXPECT_STREQ("all", result.value()[0].c_str());
  }

  {  // 32 applies to only 32
    Sysvar_platform_value value;
    value.set_allowed({"32"}, 32);
    result = value.get_allowed(32);
    EXPECT_STREQ("32", result.value()[0].c_str());
    result = value.get_allowed(64);
    EXPECT_FALSE(result.has_value());
  }

  {  // 64 applies to only 64
    Sysvar_platform_value value;
    value.set_allowed({"64"}, 64);
    result = value.get_allowed(64);
    EXPECT_STREQ("64", result.value()[0].c_str());
    result = value.get_allowed(32);
    EXPECT_FALSE(result.has_value());
  }

  {  // Specific values are used, all is ignored
    Sysvar_platform_value value;
    value.set_allowed({"all"});
    value.set_allowed({"32"}, 32);
    value.set_allowed({"64"}, 64);
    result = value.get_allowed(32);
    EXPECT_STREQ("32", result.value()[0].c_str());
    result = value.get_allowed(64);
    EXPECT_STREQ("64", result.value()[0].c_str());
  }
}

TEST(Upgrade_checker_Sysvar_platform_value, forbidden_resolution) {
  std::optional<std::vector<std::string>> result;
  {  // No values set
    Sysvar_platform_value value;
    result = value.get_forbidden(32);
    EXPECT_FALSE(result.has_value());
    result = value.get_forbidden(64);
    EXPECT_FALSE(result.has_value());
  }

  {  // All applies to both architectures
    Sysvar_platform_value value;
    value.set_forbidden({"all"});
    result = value.get_forbidden(32);
    EXPECT_STREQ("all", result.value()[0].c_str());
    result = value.get_forbidden(64);
    EXPECT_STREQ("all", result.value()[0].c_str());
  }

  {  // 32 applies to only 32
    Sysvar_platform_value value;
    value.set_forbidden({"32"}, 32);
    result = value.get_forbidden(32);
    EXPECT_STREQ("32", result.value()[0].c_str());
    result = value.get_forbidden(64);
    EXPECT_FALSE(result.has_value());
  }

  {  // 64 applies to only 64
    Sysvar_platform_value value;
    value.set_forbidden({"64"}, 64);
    result = value.get_forbidden(64);
    EXPECT_STREQ("64", result.value()[0].c_str());
    result = value.get_forbidden(32);
    EXPECT_FALSE(result.has_value());
  }

  {  // Specific values are used, all is ignored
    Sysvar_platform_value value;
    value.set_forbidden({"all"});
    value.set_forbidden({"32"}, 32);
    value.set_forbidden({"64"}, 64);
    result = value.get_forbidden(32);
    EXPECT_STREQ("32", result.value()[0].c_str());
    result = value.get_forbidden(64);
    EXPECT_STREQ("64", result.value()[0].c_str());
  }
}

TEST(Upgrade_checker_Sysvar_configuration, default_resolution) {
  std::optional<std::string> result;

  std::vector<size_t> archs = {32, 64};

  {  // No values set
    Sysvar_configuration config;

    // WLM: Windows, Linux, Mac
    for (auto platform : std::string("WLM")) {
      for (auto bits : archs) {
        result = config.get_default(bits, platform);
        EXPECT_FALSE(result.has_value());
      }
    }
  }

  {  // All applies to all platforms
    Sysvar_configuration config;
    config.set_default("all");

    for (auto platform : std::string("WLM")) {
      for (auto bits : archs) {
        EXPECT_STREQ("all", config.get_default(bits, platform).value().c_str());
      }
    }
  }

  {  // Unix applies to non-windows, over All
    Sysvar_configuration config;
    config.set_default("unix", 0, 'U');

    // Windows
    EXPECT_FALSE(config.get_default(32, 'W').has_value());
    EXPECT_FALSE(config.get_default(64, 'W').has_value());

    // Linux and Mac
    for (auto platform : std::string("LM")) {
      for (auto bits : archs) {
        EXPECT_STREQ("unix",
                     config.get_default(bits, platform).value().c_str());
      }
    }
  }

  {  // Windows applies to windows only
    Sysvar_configuration config;
    config.set_default("windows", 0, 'W');

    // Windows
    EXPECT_STREQ("windows", config.get_default(32, 'W').value().c_str());
    EXPECT_STREQ("windows", config.get_default(64, 'W').value().c_str());

    // Linux and Mac
    for (auto platform : std::string("LM")) {
      for (auto bits : archs) {
        EXPECT_FALSE(config.get_default(bits, platform).has_value());
      }
    }
  }

  {  // Linux applies to linux only
    Sysvar_configuration config;
    config.set_default("linux", 0, 'L');

    // Linux
    EXPECT_STREQ("linux", config.get_default(32, 'L').value().c_str());
    EXPECT_STREQ("linux", config.get_default(64, 'L').value().c_str());

    // Windows and Mac
    for (auto platform : std::string("WM")) {
      for (auto bits : archs) {
        EXPECT_FALSE(config.get_default(bits, platform).has_value());
      }
    }
  }

  {  // Macos applies to macos only
    Sysvar_configuration config;
    config.set_default("macos", 0, 'M');

    // Macos
    EXPECT_STREQ("macos", config.get_default(32, 'M').value().c_str());
    EXPECT_STREQ("macos", config.get_default(64, 'M').value().c_str());

    // Windows and Linux
    for (auto platform : std::string("WL")) {
      for (auto bits : archs) {
        EXPECT_FALSE(config.get_default(bits, platform).has_value());
      }
    }
  }

  {  // Unix overrides All
    Sysvar_configuration config;
    config.set_default("all", 0, 'A');
    config.set_default("unix", 0, 'U');

    // Windows
    EXPECT_STREQ("all", config.get_default(32, 'W').value().c_str());
    EXPECT_STREQ("all", config.get_default(64, 'W').value().c_str());

    // Linux and Mac
    for (auto platform : std::string("LM")) {
      for (auto bits : archs) {
        EXPECT_STREQ("unix",
                     config.get_default(bits, platform).value().c_str());
      }
    }
  }

  {  // Specifics override unix and all
    Sysvar_configuration config;
    config.set_default("all", 0, 'A');
    config.set_default("unix", 0, 'U');
    config.set_default("windows", 0, 'W');
    config.set_default("linux", 0, 'L');
    config.set_default("macos", 0, 'M');

    // Windows
    EXPECT_STREQ("windows", config.get_default(32, 'W').value().c_str());
    EXPECT_STREQ("windows", config.get_default(64, 'W').value().c_str());

    // Linux
    EXPECT_STREQ("linux", config.get_default(32, 'L').value().c_str());
    EXPECT_STREQ("linux", config.get_default(64, 'L').value().c_str());

    // Linux
    EXPECT_STREQ("macos", config.get_default(32, 'M').value().c_str());
    EXPECT_STREQ("macos", config.get_default(64, 'M').value().c_str());
  }
}

TEST(Upgrade_checker_Sysvar_configuration, allowed_resolution) {
  std::optional<std::vector<std::string>> result;

  std::vector<size_t> archs = {32, 64};

  {  // No values set
    Sysvar_configuration config;

    // WLM: Windows, Linux, Mac
    for (auto platform : std::string("WLM")) {
      for (auto bits : archs) {
        result = config.get_allowed(bits, platform);
        EXPECT_FALSE(result.has_value());
      }
    }
  }

  {  // All applies to all platforms
    Sysvar_configuration config;
    config.set_allowed({"all"});

    for (auto platform : std::string("WLM")) {
      for (auto bits : archs) {
        EXPECT_STREQ("all",
                     config.get_allowed(bits, platform).value()[0].c_str());
      }
    }
  }

  {  // Unix applies to non-windows, over All
    Sysvar_configuration config;
    config.set_allowed({"unix"}, 0, 'U');

    // Windows
    EXPECT_FALSE(config.get_allowed(32, 'W').has_value());
    EXPECT_FALSE(config.get_allowed(64, 'W').has_value());

    // Linux and Mac
    for (auto platform : std::string("LM")) {
      for (auto bits : archs) {
        EXPECT_STREQ("unix",
                     config.get_allowed(bits, platform).value()[0].c_str());
      }
    }
  }

  {  // Windows applies to windows only
    Sysvar_configuration config;
    config.set_allowed({"windows"}, 0, 'W');

    // Windows
    EXPECT_STREQ("windows", config.get_allowed(32, 'W').value()[0].c_str());
    EXPECT_STREQ("windows", config.get_allowed(64, 'W').value()[0].c_str());

    // Linux and Mac
    for (auto platform : std::string("LM")) {
      for (auto bits : archs) {
        EXPECT_FALSE(config.get_allowed(bits, platform).has_value());
      }
    }
  }

  {  // Linux applies to linux only
    Sysvar_configuration config;
    config.set_allowed({"linux"}, 0, 'L');

    // Linux
    EXPECT_STREQ("linux", config.get_allowed(32, 'L').value()[0].c_str());
    EXPECT_STREQ("linux", config.get_allowed(64, 'L').value()[0].c_str());

    // Windows and Mac
    for (auto platform : std::string("WM")) {
      for (auto bits : archs) {
        EXPECT_FALSE(config.get_allowed(bits, platform).has_value());
      }
    }
  }

  {  // Macos applies to macos only
    Sysvar_configuration config;
    config.set_allowed({"macos"}, 0, 'M');

    // Macos
    EXPECT_STREQ("macos", config.get_allowed(32, 'M').value()[0].c_str());
    EXPECT_STREQ("macos", config.get_allowed(64, 'M').value()[0].c_str());

    // Windows and Linux
    for (auto platform : std::string("WL")) {
      for (auto bits : archs) {
        EXPECT_FALSE(config.get_allowed(bits, platform).has_value());
      }
    }
  }

  {  // Unix overrides All
    Sysvar_configuration config;
    config.set_allowed({"all"}, 0, 'A');
    config.set_allowed({"unix"}, 0, 'U');

    // Windows
    EXPECT_STREQ("all", config.get_allowed(32, 'W').value()[0].c_str());
    EXPECT_STREQ("all", config.get_allowed(64, 'W').value()[0].c_str());

    // Linux and Mac
    for (auto platform : std::string("LM")) {
      for (auto bits : archs) {
        EXPECT_STREQ("unix",
                     config.get_allowed(bits, platform).value()[0].c_str());
      }
    }
  }

  {  // Specifics override unix and all
    Sysvar_configuration config;
    config.set_allowed({"all"}, 0, 'A');
    config.set_allowed({"unix"}, 0, 'U');
    config.set_allowed({"windows"}, 0, 'W');
    config.set_allowed({"linux"}, 0, 'L');
    config.set_allowed({"macos"}, 0, 'M');

    // Windows
    EXPECT_STREQ("windows", config.get_allowed(32, 'W').value()[0].c_str());
    EXPECT_STREQ("windows", config.get_allowed(64, 'W').value()[0].c_str());

    // Linux
    EXPECT_STREQ("linux", config.get_allowed(32, 'L').value()[0].c_str());
    EXPECT_STREQ("linux", config.get_allowed(64, 'L').value()[0].c_str());

    // Linux
    EXPECT_STREQ("macos", config.get_allowed(32, 'M').value()[0].c_str());
    EXPECT_STREQ("macos", config.get_allowed(64, 'M').value()[0].c_str());
  }
}

TEST(Upgrade_checker_Sysvar_configuration, forbidden_resolution) {
  std::optional<std::vector<std::string>> result;

  std::vector<size_t> archs = {32, 64};

  {  // No values set
    Sysvar_configuration config;

    // WLM: Windows, Linux, Mac
    for (auto platform : std::string("WLM")) {
      for (auto bits : archs) {
        result = config.get_forbidden(bits, platform);
        EXPECT_FALSE(result.has_value());
      }
    }
  }

  {  // All applies to all platforms
    Sysvar_configuration config;
    config.set_forbidden({"all"});

    for (auto platform : std::string("WLM")) {
      for (auto bits : archs) {
        EXPECT_STREQ("all",
                     config.get_forbidden(bits, platform).value()[0].c_str());
      }
    }
  }

  {  // Unix applies to non-windows, over All
    Sysvar_configuration config;
    config.set_forbidden({"unix"}, 0, 'U');

    // Windows
    EXPECT_FALSE(config.get_forbidden(32, 'W').has_value());
    EXPECT_FALSE(config.get_forbidden(64, 'W').has_value());

    // Linux and Mac
    for (auto platform : std::string("LM")) {
      for (auto bits : archs) {
        EXPECT_STREQ("unix",
                     config.get_forbidden(bits, platform).value()[0].c_str());
      }
    }
  }

  {  // Windows applies to windows only
    Sysvar_configuration config;
    config.set_forbidden({"windows"}, 0, 'W');

    // Windows
    EXPECT_STREQ("windows", config.get_forbidden(32, 'W').value()[0].c_str());
    EXPECT_STREQ("windows", config.get_forbidden(64, 'W').value()[0].c_str());

    // Linux and Mac
    for (auto platform : std::string("LM")) {
      for (auto bits : archs) {
        EXPECT_FALSE(config.get_forbidden(bits, platform).has_value());
      }
    }
  }

  {  // Linux applies to linux only
    Sysvar_configuration config;
    config.set_forbidden({"linux"}, 0, 'L');

    // Linux
    EXPECT_STREQ("linux", config.get_forbidden(32, 'L').value()[0].c_str());
    EXPECT_STREQ("linux", config.get_forbidden(64, 'L').value()[0].c_str());

    // Windows and Mac
    for (auto platform : std::string("WM")) {
      for (auto bits : archs) {
        EXPECT_FALSE(config.get_forbidden(bits, platform).has_value());
      }
    }
  }

  {  // Macos applies to macos only
    Sysvar_configuration config;
    config.set_forbidden({"macos"}, 0, 'M');

    // Macos
    EXPECT_STREQ("macos", config.get_forbidden(32, 'M').value()[0].c_str());
    EXPECT_STREQ("macos", config.get_forbidden(64, 'M').value()[0].c_str());

    // Windows and Linux
    for (auto platform : std::string("WL")) {
      for (auto bits : archs) {
        EXPECT_FALSE(config.get_forbidden(bits, platform).has_value());
      }
    }
  }

  {  // Unix overrides All
    Sysvar_configuration config;
    config.set_forbidden({"all"}, 0, 'A');
    config.set_forbidden({"unix"}, 0, 'U');

    // Windows
    EXPECT_STREQ("all", config.get_forbidden(32, 'W').value()[0].c_str());
    EXPECT_STREQ("all", config.get_forbidden(64, 'W').value()[0].c_str());

    // Linux and Mac
    for (auto platform : std::string("LM")) {
      for (auto bits : archs) {
        EXPECT_STREQ("unix",
                     config.get_forbidden(bits, platform).value()[0].c_str());
      }
    }
  }

  {  // Specifics override unix and all
    Sysvar_configuration config;
    config.set_forbidden({"all"}, 0, 'A');
    config.set_forbidden({"unix"}, 0, 'U');
    config.set_forbidden({"windows"}, 0, 'W');
    config.set_forbidden({"linux"}, 0, 'L');
    config.set_forbidden({"macos"}, 0, 'M');

    // Windows
    EXPECT_STREQ("windows", config.get_forbidden(32, 'W').value()[0].c_str());
    EXPECT_STREQ("windows", config.get_forbidden(64, 'W').value()[0].c_str());

    // Linux
    EXPECT_STREQ("linux", config.get_forbidden(32, 'L').value()[0].c_str());
    EXPECT_STREQ("linux", config.get_forbidden(64, 'L').value()[0].c_str());

    // Linux
    EXPECT_STREQ("macos", config.get_forbidden(32, 'M').value()[0].c_str());
    EXPECT_STREQ("macos", config.get_forbidden(64, 'M').value()[0].c_str());
  }
}

Sysvar_definition test_sysvar(const std::string &name = "test_variable") {
  Sysvar_definition sysvar;
  sysvar.name = name;
  sysvar.set_default("default");

  return sysvar;
}

namespace {
std::vector<Sysvar_version_check> test_get_checks(
    const std::vector<Sysvar_definition> &def_list,
    const Upgrade_info &upgrade_info) {
  Sysvar_registry reg;

  for (auto def : def_list) {
    reg.register_sysvar(std::move(def));
  }

  auto confs = reg.get_checks(upgrade_info);

  return confs;
}
}  // namespace

TEST(Upgrade_checker_Sysvar_definition, get_check_negative) {
  // No changes at all
  {
    auto sysvar = test_sysvar();
    auto ui = upgrade_info(Version(5, 7, 44), Version(8, 0, 0));
    EXPECT_FALSE(sysvar.get_check(ui).has_value());

    EXPECT_EQ(0, test_get_checks({sysvar}, ui).size());
  }

  // The default changed at the same version as the source server
  {
    auto sysvar = test_sysvar();
    sysvar.set_default("changed", Version(5, 7, 44));
    auto ui = upgrade_info(Version(5, 7, 44), Version(8, 0, 0));
    EXPECT_FALSE(sysvar.get_check(ui).has_value());

    EXPECT_EQ(0, test_get_checks({sysvar}, ui).size());
  }

  // The default change occurs after the target server version
  {
    auto sysvar = test_sysvar();
    sysvar.set_default("changed", Version(8, 0, 1));
    auto ui = upgrade_info(Version(5, 7, 44), Version(8, 0, 0));
    EXPECT_FALSE(sysvar.get_check(ui).has_value());

    EXPECT_EQ(0, test_get_checks({sysvar}, ui).size());
  }

  // The a default change happened fter source, but came back to original
  // default before target
  {
    auto sysvar = test_sysvar();
    sysvar.set_default("changed", Version(5, 7, 45));
    sysvar.set_default("default", Version(8, 0, 0));
    sysvar.set_default("changed again",
                       Version(8, 0, 1));  // This one will be ignored
    auto ui = upgrade_info(Version(5, 7, 44), Version(8, 0, 0));
    EXPECT_FALSE(sysvar.get_check(ui).has_value());

    EXPECT_EQ(0, test_get_checks({sysvar}, ui).size());
  }

  // Variable has changes but was introduced after the server version
  {
    auto sysvar = test_sysvar();
    sysvar.introduction_version = Version(5, 7, 45);
    sysvar.set_default("changed", Version(5, 7, 46));
    auto ui = upgrade_info(Version(5, 7, 44), Version(8, 0, 0));
    EXPECT_FALSE(sysvar.get_check(ui).has_value());

    EXPECT_EQ(0, test_get_checks({sysvar}, ui).size());
  }

  // Variable deprecated after server version
  {
    auto sysvar = test_sysvar();
    sysvar.deprecation_version = Version(8, 0, 1);
    auto ui = upgrade_info(Version(5, 7, 44), Version(8, 0, 0));
    EXPECT_FALSE(sysvar.get_check(ui).has_value());

    EXPECT_EQ(0, test_get_checks({sysvar}, ui).size());
  }
}

TEST(Upgrade_checker_Sysvar_definition, get_check_positive) {
  // Default changes
  {
    auto sysvar = test_sysvar();
    sysvar.set_default("changed", Version(5, 7, 45));
    auto ui = upgrade_info(Version(5, 7, 44), Version(8, 0, 0));
    EXPECT_TRUE(sysvar.get_check(ui).has_value());

    EXPECT_EQ(1, test_get_checks({sysvar}, ui).size());
  }

  // Allowed value list, even there are no further changes
  {
    auto sysvar = test_sysvar();
    sysvar.set_allowed({"allowed"});
    auto ui = upgrade_info(Version(5, 7, 44), Version(8, 0, 0));
    EXPECT_TRUE(sysvar.get_check(ui).has_value());

    EXPECT_EQ(1, test_get_checks({sysvar}, ui).size());
  }

  // Forbidden value list, even there are no further changes
  {
    auto sysvar = test_sysvar();
    sysvar.set_forbidden({"forbidden"});
    auto ui = upgrade_info(Version(5, 7, 44), Version(8, 0, 0));
    EXPECT_TRUE(sysvar.get_check(ui).has_value());

    EXPECT_EQ(1, test_get_checks({sysvar}, ui).size());
  }

  // No changes but variable is deprecated before source server version
  {
    auto sysvar = test_sysvar();
    sysvar.deprecation_version = Version(5, 7, 43);
    auto ui = upgrade_info(Version(5, 7, 44), Version(8, 0, 0));
    EXPECT_TRUE(sysvar.get_check(ui).has_value());

    EXPECT_EQ(1, test_get_checks({sysvar}, ui).size());
  }

  // No changes but variable is deprecated before target version and on target
  // version
  {
    auto sysvar = test_sysvar();
    sysvar.deprecation_version = Version(8, 0, 0);
    auto ui = upgrade_info(Version(5, 7, 44), Version(8, 0, 0));
    EXPECT_TRUE(sysvar.get_check(ui).has_value());

    ui = upgrade_info(Version(5, 7, 44), Version(8, 0, 1));
    EXPECT_TRUE(sysvar.get_check(ui).has_value());

    EXPECT_EQ(1, test_get_checks({sysvar}, ui).size());
  }

  // No changes but variable is removed after server version but before target
  // version and on target version
  {
    auto sysvar = test_sysvar();
    sysvar.removal_version = Version(8, 0, 0);
    auto ui = upgrade_info(Version(5, 7, 44), Version(8, 0, 0));
    EXPECT_TRUE(sysvar.get_check(ui).has_value());

    ui = upgrade_info(Version(5, 7, 44), Version(8, 0, 1));
    EXPECT_TRUE(sysvar.get_check(ui).has_value());

    EXPECT_EQ(1, test_get_checks({sysvar}, ui).size());
  }
}

TEST(Upgrade_checker_Sysvar_definition, get_check_default_correctness) {
  auto sysvar = test_sysvar();
  sysvar.set_default("0");
  sysvar.set_default("1", Version(1, 0, 0));
  sysvar.set_default("2", Version(2, 0, 0));
  sysvar.set_default("3", Version(3, 0, 0));
  sysvar.set_default("4", Version(4, 0, 0));
  sysvar.set_default("5", Version(5, 0, 0));

  // No changes in default
  {
    for (int source = 0; source <= 5; source++) {
      auto ui = upgrade_info(Version(source, 0, 0), Version(source, 9, 9));
      EXPECT_FALSE(sysvar.get_check(ui).has_value());

      EXPECT_EQ(0, test_get_checks({sysvar}, ui).size());
    }
  }

  // Ensures the correct initial default is selected
  for (int source = 0; source < 5; source++) {
    auto ui = upgrade_info(Version(source, 0, 0), Version(5, 0, 0));
    auto check = sysvar.get_check(ui).value();
    EXPECT_EQ(std::to_string(source), check.initial_defaults);
    EXPECT_STREQ("5", check.updated_defaults.c_str());

    auto checks = test_get_checks({sysvar}, ui);
    EXPECT_EQ(std::to_string(source), checks[0].initial_defaults);
    EXPECT_STREQ("5", checks[0].updated_defaults.c_str());
  }

  // Ensures the correct final default is selected
  for (int target = 1; target <= 5; target++) {
    {
      auto ui = upgrade_info(Version(0, 0, 0), Version(target, 0, 0));
      auto check = sysvar.get_check(ui).value();
      EXPECT_STREQ("0", check.initial_defaults.c_str());
      EXPECT_EQ(std::to_string(target), check.updated_defaults);

      auto checks = test_get_checks({sysvar}, ui);
      EXPECT_STREQ("0", checks[0].initial_defaults.c_str());
      EXPECT_EQ(std::to_string(target), checks[0].updated_defaults);
    }
  }
}

TEST(Upgrade_checker_Sysvar_definition, get_check_allowed_correctness) {
  auto sysvar = test_sysvar();
  sysvar.set_allowed({"0"});
  sysvar.set_allowed({"1"}, Version(1, 0, 0));
  sysvar.set_allowed({"2"}, Version(2, 0, 0));
  sysvar.set_allowed({"3"}, Version(3, 0, 0));
  sysvar.set_allowed({"4"}, Version(4, 0, 0));
  sysvar.set_allowed({"5"}, Version(5, 0, 0));

  // Any present allowed list represents a variable to be checked
  {
    for (int source = 0; source <= 5; source++) {
      auto ui = upgrade_info(Version(source, 0, 0), Version(source, 9, 9));
      auto check = sysvar.get_check(ui).value();
      EXPECT_EQ(std::to_string(source), check.allowed_values[0]);

      auto checks = test_get_checks({sysvar}, ui);
      EXPECT_EQ(std::to_string(source), checks[0].allowed_values[0]);
    }
  }
}

TEST(Upgrade_checker_Sysvar_definition, get_check_forbidden_correctness) {
  auto sysvar = test_sysvar();
  sysvar.set_forbidden({"0"});
  sysvar.set_forbidden({"1"}, Version(1, 0, 0));
  sysvar.set_forbidden({"2"}, Version(2, 0, 0));
  sysvar.set_forbidden({"3"}, Version(3, 0, 0));
  sysvar.set_forbidden({"4"}, Version(4, 0, 0));
  sysvar.set_forbidden({"5"}, Version(5, 0, 0));

  // Any present allowed list represents a variable to be checked
  {
    for (int source = 0; source <= 5; source++) {
      auto ui = upgrade_info(Version(source, 0, 0), Version(source, 9, 9));
      auto check = sysvar.get_check(ui).value();
      EXPECT_EQ(std::to_string(source), check.forbidden_values[0]);

      auto checks = test_get_checks({sysvar}, ui);
      EXPECT_EQ(std::to_string(source), checks[0].forbidden_values[0]);
    }
  }
}

TEST(Upgrade_checker_Sysvar_registry, get_checks_allowed) {
  // NOTE: Additional tests for check retrieval from registry were implemented
  // on UTs for Sysvar_definition.get_check
  auto sv0 = test_sysvar("variable_0");
  sv0.set_allowed({"zero-0"});
  sv0.set_allowed({"zero-1"}, Version(1, 0, 0));
  sv0.set_allowed({"zero-2"}, Version(2, 0, 0));

  auto sv1 = test_sysvar("variable_1");
  sv1.introduction_version = Version(1, 0, 0);
  sv1.set_allowed({"one-1"});
  sv1.set_allowed({"one-2"}, Version(2, 0, 0));
  sv1.set_allowed({"one-3"}, Version(3, 0, 0));

  auto sv2 = test_sysvar("variable_2");
  sv2.introduction_version = Version(2, 0, 0);
  sv2.removal_version = Version(4, 0, 0);
  sv2.set_allowed({"two-2"});
  sv2.set_allowed({"two-3"}, Version(3, 0, 0));

  // Any present allowed list represents a variable to be checked
  {
    auto ui = upgrade_info(Version(0, 0, 0), Version(0, 9, 9));
    auto checks = test_get_checks({sv0, sv1, sv2}, ui);
    ASSERT_EQ(1, checks.size());
    EXPECT_STREQ("zero-0", checks[0].allowed_values[0].c_str());
  }

  {
    auto ui = upgrade_info(Version(0, 0, 0), Version(1, 0, 0));
    auto checks = test_get_checks({sv0, sv1, sv2}, ui);
    ASSERT_EQ(1, checks.size());
    EXPECT_STREQ("zero-1", checks[0].allowed_values[0].c_str());
  }

  {
    auto ui = upgrade_info(Version(1, 0, 0), Version(1, 9, 9));
    auto checks = test_get_checks({sv0, sv1, sv2}, ui);
    ASSERT_EQ(2, checks.size());
    EXPECT_STREQ("zero-1", checks[0].allowed_values[0].c_str());
    EXPECT_STREQ("one-1", checks[1].allowed_values[0].c_str());
  }

  {
    auto ui = upgrade_info(Version(1, 0, 0), Version(2, 0, 0));
    auto checks = test_get_checks({sv0, sv1, sv2}, ui);
    ASSERT_EQ(2, checks.size());
    EXPECT_STREQ("zero-2", checks[0].allowed_values[0].c_str());
    EXPECT_STREQ("one-2", checks[1].allowed_values[0].c_str());
  }

  {
    auto ui = upgrade_info(Version(2, 0, 0), Version(2, 9, 9));
    auto checks = test_get_checks({sv0, sv1, sv2}, ui);
    ASSERT_EQ(3, checks.size());
    EXPECT_STREQ("zero-2", checks[0].allowed_values[0].c_str());
    EXPECT_STREQ("one-2", checks[1].allowed_values[0].c_str());
    EXPECT_STREQ("two-2", checks[2].allowed_values[0].c_str());
  }

  {
    auto ui = upgrade_info(Version(2, 0, 0), Version(3, 0, 0));
    auto checks = test_get_checks({sv0, sv1, sv2}, ui);
    ASSERT_EQ(3, checks.size());
    // No more changes on variable_0
    EXPECT_STREQ("zero-2", checks[0].allowed_values[0].c_str());
    EXPECT_STREQ("one-3", checks[1].allowed_values[0].c_str());
    EXPECT_STREQ("two-3", checks[2].allowed_values[0].c_str());
  }

  {
    auto ui = upgrade_info(Version(3, 0, 0), Version(4, 0, 0));
    auto checks = test_get_checks({sv0, sv1, sv2}, ui);
    ASSERT_EQ(3, checks.size());
    EXPECT_STREQ("zero-2", checks[0].allowed_values[0].c_str());
    EXPECT_STREQ("one-3", checks[1].allowed_values[0].c_str());
    EXPECT_STREQ("two-3", checks[2].allowed_values[0].c_str());
  }

  {
    auto ui = upgrade_info(Version(4, 0, 0), Version(5, 0, 0));
    auto checks = test_get_checks({sv0, sv1, sv2}, ui);
    ASSERT_EQ(2, checks.size());
    // variable_2 got removed in version 4 so is no longer selected
    EXPECT_STREQ("zero-2", checks[0].allowed_values[0].c_str());
    EXPECT_STREQ("one-3", checks[1].allowed_values[0].c_str());
  }
}

TEST(Upgrade_checker_Sysvar_registry, get_checks_forbidden) {
  // NOTE: Additional tests for check retrieval from registry were implemented
  // on UTs for Sysvar_definition.get_check
  auto sv0 = test_sysvar("variable_0");
  sv0.set_forbidden({"zero-0"});
  sv0.set_forbidden({"zero-1"}, Version(1, 0, 0));
  sv0.set_forbidden({"zero-2"}, Version(2, 0, 0));

  auto sv1 = test_sysvar("variable_1");
  sv1.introduction_version = Version(1, 0, 0);
  sv1.set_forbidden({"one-1"});
  sv1.set_forbidden({"one-2"}, Version(2, 0, 0));
  sv1.set_forbidden({"one-3"}, Version(3, 0, 0));

  auto sv2 = test_sysvar("variable_2");
  sv2.introduction_version = Version(2, 0, 0);
  sv2.removal_version = Version(4, 0, 0);
  sv2.set_forbidden({"two-2"});
  sv2.set_forbidden({"two-3"}, Version(3, 0, 0));

  // Any present allowed list represents a variable to be checked
  {
    auto ui = upgrade_info(Version(0, 0, 0), Version(0, 9, 9));
    auto checks = test_get_checks({sv0, sv1, sv2}, ui);
    ASSERT_EQ(1, checks.size());
    EXPECT_STREQ("zero-0", checks[0].forbidden_values[0].c_str());
  }

  {
    auto ui = upgrade_info(Version(0, 0, 0), Version(1, 0, 0));
    auto checks = test_get_checks({sv0, sv1, sv2}, ui);
    ASSERT_EQ(1, checks.size());
    EXPECT_STREQ("zero-1", checks[0].forbidden_values[0].c_str());
  }

  {
    auto ui = upgrade_info(Version(1, 0, 0), Version(1, 9, 9));
    auto checks = test_get_checks({sv0, sv1, sv2}, ui);
    ASSERT_EQ(2, checks.size());
    EXPECT_STREQ("zero-1", checks[0].forbidden_values[0].c_str());
    EXPECT_STREQ("one-1", checks[1].forbidden_values[0].c_str());
  }

  {
    auto ui = upgrade_info(Version(1, 0, 0), Version(2, 0, 0));
    auto checks = test_get_checks({sv0, sv1, sv2}, ui);
    ASSERT_EQ(2, checks.size());
    EXPECT_STREQ("zero-2", checks[0].forbidden_values[0].c_str());
    EXPECT_STREQ("one-2", checks[1].forbidden_values[0].c_str());
  }

  {
    auto ui = upgrade_info(Version(2, 0, 0), Version(2, 9, 9));
    auto checks = test_get_checks({sv0, sv1, sv2}, ui);
    ASSERT_EQ(3, checks.size());
    EXPECT_STREQ("zero-2", checks[0].forbidden_values[0].c_str());
    EXPECT_STREQ("one-2", checks[1].forbidden_values[0].c_str());
    EXPECT_STREQ("two-2", checks[2].forbidden_values[0].c_str());
  }

  {
    auto ui = upgrade_info(Version(2, 0, 0), Version(3, 0, 0));
    auto checks = test_get_checks({sv0, sv1, sv2}, ui);
    ASSERT_EQ(3, checks.size());
    // No more changes on variable_0
    EXPECT_STREQ("zero-2", checks[0].forbidden_values[0].c_str());
    EXPECT_STREQ("one-3", checks[1].forbidden_values[0].c_str());
    EXPECT_STREQ("two-3", checks[2].forbidden_values[0].c_str());
  }

  {
    auto ui = upgrade_info(Version(3, 0, 0), Version(4, 0, 0));
    auto checks = test_get_checks({sv0, sv1, sv2}, ui);
    ASSERT_EQ(3, checks.size());
    EXPECT_STREQ("zero-2", checks[0].forbidden_values[0].c_str());
    EXPECT_STREQ("one-3", checks[1].forbidden_values[0].c_str());
    EXPECT_STREQ("two-3", checks[2].forbidden_values[0].c_str());
  }

  {
    auto ui = upgrade_info(Version(4, 0, 0), Version(5, 0, 0));
    auto checks = test_get_checks({sv0, sv1, sv2}, ui);
    ASSERT_EQ(2, checks.size());
    // variable_2 got removed in version 4 so is no longer selected
    EXPECT_STREQ("zero-2", checks[0].forbidden_values[0].c_str());
    EXPECT_STREQ("one-3", checks[1].forbidden_values[0].c_str());
  }
}

TEST(Upgrade_checker_sysvar_registry, load_configuration) {
  Sysvar_registry reg;
  EXPECT_NO_THROW(reg.load_configuration());

  // Verifies removal and replacement properly loaded
  {
    auto sysvar = reg.get_sysvar("innodb_stats_sample_pages");
    EXPECT_TRUE(sysvar.replacement.has_value());
    EXPECT_STREQ("innodb_stats_transient_sample_pages",
                 sysvar.replacement.value().c_str());

    EXPECT_TRUE(sysvar.removal_version.has_value());
    EXPECT_EQ(Version(8, 0, 0), sysvar.removal_version.value());
  }

  // Verifies changes are properly loaded
  {
    auto sysvar = reg.get_sysvar("innodb_flush_method");

    // Checks the change from 5.7.44 to 8.0.11 for innodb_flush_method (windows
    // and non windows)
    {
      auto ui = upgrade_info(Version(5, 7, 44), Version(8, 0, 11), "WINDOWS");
      auto check = sysvar.get_check(ui);
      ASSERT_TRUE(check.has_value());
      EXPECT_STREQ("NULL", (*check).initial_defaults.c_str());
      EXPECT_STREQ("unbuffered", (*check).updated_defaults.c_str());
    }

    {
      auto ui = upgrade_info(Version(5, 7, 44), Version(8, 0, 11));
      auto check = sysvar.get_check(ui);
      ASSERT_TRUE(check.has_value());
      EXPECT_STREQ("NULL", (*check).initial_defaults.c_str());
      EXPECT_STREQ("fsync", (*check).updated_defaults.c_str());
    }

    // Checks the change from 8.0.11 to 8.4.0
    {
      auto ui = upgrade_info(Version(8, 0, 12), Version(8, 4, 0), "WINDOWS");
      auto check = sysvar.get_check(ui);
      ASSERT_FALSE(check.has_value());
    }

    // Checks the change for non-windows
    {
      auto ui = upgrade_info(Version(8, 0, 12), Version(8, 4, 0));
      auto check = sysvar.get_check(ui);
      ASSERT_TRUE(check.has_value());
      EXPECT_STREQ("fsync", (*check).initial_defaults.c_str());
      EXPECT_STREQ("O_DIRECT if supported, otherwise fsync",
                   (*check).updated_defaults.c_str());
    }
  }

  // Verifies forbidden properly loaded
  {
    auto sysvar = reg.get_sysvar("replica_parallel_workers");
    auto ui = upgrade_info(Version(8, 0, 26), Version(9, 0, 0));
    auto check = sysvar.get_check(ui);
    ASSERT_TRUE(check.has_value());
    ASSERT_FALSE(check->forbidden_values.empty());

    EXPECT_STREQ("0", check->forbidden_values[0].c_str());
  }
}

}  // namespace upgrade_checker
}  // namespace mysqlsh