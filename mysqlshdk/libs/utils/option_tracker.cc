/*
 * Copyright (c) 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/option_tracker.h"

#include <string>
#include <unordered_map>

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {
namespace option_tracker {

static const std::unordered_map<Shell_feature, std::string> k_feature_ids = {
    {Shell_feature::NONE, ""},
    {Shell_feature::SHELL_USE, "mysql_ot_msh<ctx>"},
    {Shell_feature::UTIL_COPY, "mysql_ot_msh.copy"},
    {Shell_feature::UTIL_DUMP, "mysql_ot_msh<ctx>.dump"},
    {Shell_feature::UTIL_LOAD_DUMP, "mysql_ot_msh<ctx>.dump.load"},
    {Shell_feature::UTIL_UPGRADE_CHECK, "mysql_ot_msh.upcheck"}};

std::string get_option_tracker_feature_id(Shell_feature feature_id) {
  return get_option_tracker_feature_id(k_feature_ids.at(feature_id));
}

std::string get_option_tracker_feature_id(
    const std::string &custom_feature_id) {
  auto execution_context =
      mysqlsh::current_shell_options()->get().execution_context;

  if (shcore::str_casecmp(execution_context.c_str(), "disable") == 0) {
    return "";
  }

  return shcore::str_replace(custom_feature_id, "<ctx>", execution_context);
}

}  // namespace option_tracker
}  // namespace shcore