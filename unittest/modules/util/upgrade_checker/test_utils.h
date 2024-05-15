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

#ifndef UNITTEST_MODULES_UTIL_UPGRADE_CHECKER_TEST_UTILS_H_
#define UNITTEST_MODULES_UTIL_UPGRADE_CHECKER_TEST_UTILS_H_

#include "modules/util/upgrade_checker/common.h"

#include <optional>
#include <string>

#include "modules/util/upgrade_checker/upgrade_check_config.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/utils/version.h"
#include "unittest/test_utils.h"

#define SKIP_IF_SERVER_LOWER_THAN(version)                           \
  do {                                                               \
    auto v = Version(version);                                       \
    if (_target_server_version < v) {                                \
      SKIP_TEST("This test requires running against MySQL server " + \
                v.get_full());                                       \
    }                                                                \
  } while (false)

namespace mysqlsh {
namespace upgrade_checker {
extern std::unordered_map<std::string, Version> k_latest_versions;

extern std::unordered_set<std::string> k_sys_schemas;

Upgrade_info upgrade_info(Version server, Version target,
                          std::string server_os = "LINUX",
                          size_t server_bits = 64);

Upgrade_info upgrade_info(const std::string &server, const std::string &target);

Upgrade_check_config create_config(std::optional<Version> server_version = {},
                                   std::optional<Version> target_version = {},
                                   const std::string &server_os = "LINUX",
                                   size_t server_bits = 64);

Version before_version(const Version &version);

Version before_version(const Version &version, int count);

Version after_version(const Version &version);

Version after_version(const Version &version, int count);

class Upgrade_checker_test : public Shell_core_test_wrapper {
 public:
  std::string deploy_sandbox(const shcore::Dictionary_t &conf, int *port);
};

std::string remove_quoted_strings(const std::string &source,
                                  const std::unordered_set<std::string> &items);

class Checker_cache;

void override_sysvar(Checker_cache *cache, const std::string &name,
                     const std::string &value,
                     const std::string &source = "SERVER");
}  // namespace upgrade_checker
}  // namespace mysqlsh

#endif  // UNITTEST_MODULES_UTIL_UPGRADE_CHECKER_TEST_UTILS_H_