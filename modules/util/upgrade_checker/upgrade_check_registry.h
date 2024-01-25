/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_REGISTRY_H_
#define MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_REGISTRY_H_

#include <forward_list>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "modules/util/upgrade_checker/common.h"
#include "modules/util/upgrade_checker/upgrade_check_condition.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/enumset.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace upgrade_checker {

class Upgrade_check;
class Upgrade_check_config;

template <typename T>
concept Version_source = std::is_constructible_v<Version, T>;
class Upgrade_check_registry {
 public:
  using Creator =
      std::function<std::unique_ptr<Upgrade_check>(const Upgrade_info &)>;
  using Upgrade_check_vec = std::vector<std::unique_ptr<Upgrade_check>>;

  struct Creator_info {
    std::unique_ptr<Condition> condition;
    Creator creator;
    Target target;
  };

  using Collection = std::vector<Creator_info>;

  static const Target_flags k_target_flags_default;

  template <Version_source... Ts>
  static bool register_check(Creator creator, Target target, Ts... params) {
    std::forward_list<Version> vs{Version(params)...};

    std::unique_ptr<Condition> condition =
        std::make_unique<Version_condition>(std::move(vs));

    return register_check(std::move(creator), target, std::move(condition));
  }

  static bool register_check(Creator creator, Target target, Version ver) {
    std::unique_ptr<Condition> condition =
        std::make_unique<Version_condition>(std::move(ver));
    return register_check(std::move(creator), target, std::move(condition));
  }

  static bool register_check(Creator creator, Target target,
                             std::unique_ptr<Condition> condition) {
    s_available_checks.emplace_back(
        Creator_info{std::move(condition), creator, target});

    return true;
  }

  static bool register_check(Creator creator, Target target) {
    return register_check(std::move(creator), target,
                          std::unique_ptr<Condition>{});
  }

  static bool register_check(Creator creator, Target target,
                             Custom_condition::Callback cb,
                             std::string description) {
    std::unique_ptr<Condition> condition = std::make_unique<Custom_condition>(
        std::move(cb), std::move(description));
    return register_check(std::move(creator), target, std::move(condition));
  }

  static void register_manual_check(const char *ver, std::string_view name,
                                    Upgrade_issue::Level level, Target target);

  static std::vector<std::unique_ptr<Upgrade_check>> create_checklist(
      const Upgrade_check_config &config, bool include_all = false,
      Upgrade_check_vec *rejected = nullptr);

 private:
  static Collection s_available_checks;
};

}  // namespace upgrade_checker
}  // namespace mysqlsh

#endif  // MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_REGISTRY_H_