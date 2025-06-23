/*
 * Copyright (c) 2017, 2025, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_OPTIONS_H_
#define MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_OPTIONS_H_

#include <optional>
#include <string>

#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/utils/version.h"

#include "modules/util/upgrade_checker/common.h"

namespace mysqlsh {
namespace upgrade_checker {

struct Upgrade_check_options {
  using Check_id_uset = std::unordered_set<std::string>;

  static const shcore::Option_pack_def<Upgrade_check_options> &options();

  std::optional<mysqlshdk::utils::Version> target_version;
  std::string config_path;
  std::string output_format;
  std::optional<std::string> password;
  Check_id_set include_list;
  Check_id_set exclude_list;
  bool list_checks = false;
  bool skip_target_version_check = false;

  mysqlshdk::utils::Version get_target_version() const;

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(Upgrade_check_options, set_target_version);
#endif
  void include(const Check_id_uset &value);
  void exclude(const Check_id_uset &value);

  void set_target_version(const std::string &value);
  void verify_options();
};

}  // namespace upgrade_checker
}  // namespace mysqlsh

#endif  // MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_OPTIONS_H_
