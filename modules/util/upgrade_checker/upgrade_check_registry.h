/*
 * Copyright (c) 2017, 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_REGISTRY_H_
#define MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_REGISTRY_H_

#include <forward_list>
#include <functional>
#include <utility>
#include <vector>

#include "modules/util/upgrade_checker/common.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/enumset.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace upgrade_checker {

class Upgrade_check;

class Upgrade_check_registry {
 public:
  using Creator =
      std::function<std::unique_ptr<Upgrade_check>(const Upgrade_info &)>;

  struct Creator_info {
    std::forward_list<mysqlshdk::utils::Version> versions;
    Creator creator;
    Target target;
  };

  using Collection = std::vector<Creator_info>;

  template <class... Ts>
  static bool register_check(Creator creator, Target target, Ts... params) {
    std::forward_list<std::string> vs{params...};
    std::forward_list<mysqlshdk::utils::Version> vers;
    for (const auto &v : vs) vers.emplace_front(mysqlshdk::utils::Version(v));
    s_available_checks.emplace_back(
        Creator_info{std::move(vers), creator, target});
    return true;
  }

  static bool register_check(Creator creator, Target target,
                             const mysqlshdk::utils::Version &ver) {
    s_available_checks.emplace_back(Creator_info{
        std::forward_list<mysqlshdk::utils::Version>{ver}, creator, target});
    return true;
  }

  static void register_manual_check(const char *ver, const char *name,
                                    Upgrade_issue::Level level, Target target);

  static void prepare_translation_file(const char *filename);

  static std::vector<std::unique_ptr<Upgrade_check>> create_checklist(
      const Upgrade_info &info,
      Target_flags flags = Target_flags::all().unset(Target::MDS_SPECIFIC));

 private:
  static Collection s_available_checks;
};

}  // namespace upgrade_checker
}  // namespace mysqlsh

#endif  // MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_REGISTRY_H_