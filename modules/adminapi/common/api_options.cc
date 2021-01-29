/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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
#include "modules/adminapi/cluster/api_options.h"

#include <cinttypes>
#include <utility>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "shellcore/shell_options.h"

namespace mysqlsh {
namespace dba {

const shcore::Option_pack_def<Timeout_option> &Timeout_option::options() {
  static const auto opts = shcore::Option_pack_def<Timeout_option>().optional(
      kTimeout, &Timeout_option::set_timeout);

  return opts;
}

void Timeout_option::set_timeout(int value) {
  if (value < 0) {
    throw shcore::Exception::argument_error(
        shcore::str_format("%s option must be >= 0", kTimeout));
  }

  timeout = value;
}

const shcore::Option_pack_def<Interactive_option>
    &Interactive_option::options() {
  static const auto opts =
      shcore::Option_pack_def<Interactive_option>().optional(
          kInteractive, &Interactive_option::m_interactive);

  return opts;
}

bool Interactive_option::interactive() const {
  return m_interactive.get_safe(current_shell_options()->get().wizards);
}

const shcore::Option_pack_def<Password_interactive_options>
    &Password_interactive_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Password_interactive_options>()
          .optional(mysqlshdk::db::kPassword,
                    &Password_interactive_options::set_password)
          .optional(mysqlshdk::db::kDbPassword,
                    &Password_interactive_options::set_password, "",
                    shcore::Option_extract_mode::CASE_INSENSITIVE,
                    shcore::Option_scope::DEPRECATED)
          .include<Interactive_option>();

  return opts;
}

void Password_interactive_options::set_password(const std::string &option,
                                                const std::string &value) {
  if (option == mysqlshdk::db::kDbPassword) {
    handle_deprecated_option(mysqlshdk::db::kDbPassword,
                             mysqlshdk::db::kPassword, !password.is_null(),
                             true);
  }

  password = value;
}

const shcore::Option_pack_def<Force_interactive_options>
    &Force_interactive_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Force_interactive_options>()
          .optional(kForce, &Force_interactive_options::force)
          .include<Interactive_option>();

  return opts;
}

const shcore::Option_pack_def<List_routers_options>
    &List_routers_options::options() {
  static const auto opts =
      shcore::Option_pack_def<List_routers_options>().optional(
          kOnlyUpgradeRequired, &List_routers_options::only_upgrade_required);

  return opts;
}

const shcore::Option_pack_def<Setup_account_options>
    &Setup_account_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Setup_account_options>()
          .optional(kDryRun, &Setup_account_options::dry_run)
          .optional(kUpdate, &Setup_account_options::update)
          .include<Password_interactive_options>();

  return opts;
}

}  // namespace dba
}  // namespace mysqlsh
