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

#include "modules/util/upgrade_checker/upgrade_check_options.h"

#include "modules/util/upgrade_checker/common.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/scripting/types_cpp.h"

namespace mysqlsh {
namespace upgrade_checker {

const shcore::Option_pack_def<Upgrade_check_options>
    &Upgrade_check_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Upgrade_check_options>()
          .optional("outputFormat", &Upgrade_check_options::output_format)
          .optional("targetVersion", &Upgrade_check_options::set_target_version)
          .optional("configPath", &Upgrade_check_options::config_path)
          .optional("password", &Upgrade_check_options::password, "",
                    shcore::Option_extract_mode::CASE_SENSITIVE,
                    shcore::Option_scope::CLI_DISABLED);

  return opts;
}

mysqlshdk::utils::Version Upgrade_check_options::get_target_version() const {
  return target_version.value_or(mysqlshdk::utils::Version(MYSH_VERSION));
}

void Upgrade_check_options::set_target_version(const std::string &value) {
  if (!value.empty()) {
    if (latest_versions.contains(value)) {
      target_version = latest_versions.at(value);
    } else {
      target_version = Version(value);
    }
  }
}

}  // namespace upgrade_checker
}  // namespace mysqlsh