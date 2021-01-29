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
#include "modules/adminapi/replica_set/api_options.h"

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
namespace replicaset {

const shcore::Option_pack_def<Wait_recovery_option>
    &Wait_recovery_option::options() {
  static const auto opts =
      shcore::Option_pack_def<Wait_recovery_option>().optional(
          kWaitRecovery, &Wait_recovery_option::set_wait_recovery);

  return opts;
}

void Wait_recovery_option::set_wait_recovery(int value) {
  if (value < 1 || value > 3) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value '%d' for option '%s'. It must be an "
                           "integer in the range [1, 3].",
                           value, kWaitRecovery));
  }

  wait_recovery = value;
}

const shcore::Option_pack_def<Rejoin_instance_options>
    &Rejoin_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Rejoin_instance_options>()
          .include(&Rejoin_instance_options::clone_options)
          .optional(kDryRun, &Rejoin_instance_options::dry_run)
          .include<Wait_recovery_option>()
          .include<Timeout_option>()
          .include<Interactive_option>();

  return opts;
}

const shcore::Option_pack_def<Add_instance_options>
    &Add_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Add_instance_options>()
          .include<Rejoin_instance_options>()
          .include(&Add_instance_options::ar_options)
          .optional(kLabel, &Add_instance_options::instance_label);
  return opts;
}

const shcore::Option_pack_def<Gtid_wait_timeout_option>
    &Gtid_wait_timeout_option::options() {
  static const auto opts =
      shcore::Option_pack_def<Gtid_wait_timeout_option>().optional(
          kTimeout, &Gtid_wait_timeout_option::m_timeout);

  return opts;
}

int Gtid_wait_timeout_option::timeout() const {
  return m_timeout.get_safe(
      current_shell_options()->get().dba_gtid_wait_timeout);
}

const shcore::Option_pack_def<Remove_instance_options>
    &Remove_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Remove_instance_options>()
          .optional(kForce, &Remove_instance_options::force)
          .include<Gtid_wait_timeout_option>();

  return opts;
}

const shcore::Option_pack_def<Status_options> &Status_options::options() {
  static const auto opts = shcore::Option_pack_def<Status_options>().optional(
      kExtended, &Status_options::set_extended);

  return opts;
}

void Status_options::set_extended(int value) {
  // Validate extended option Integer [0, 2] or Boolean.
  if (value < 0 || value > 2) {
    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value '%d' for option '%s'. It must be an integer in the "
        "range [0, 2].",
        value, kExtended));
  }

  extended = value;
}

const shcore::Option_pack_def<Set_primary_instance_options>
    &Set_primary_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Set_primary_instance_options>()
          .optional(kDryRun, &Set_primary_instance_options::dry_run)
          .include<Gtid_wait_timeout_option>();

  return opts;
}

const shcore::Option_pack_def<Force_primary_instance_options>
    &Force_primary_instance_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Force_primary_instance_options>()
          .include<Set_primary_instance_options>()
          .optional(kInvalidateErrorInstances,
                    &Force_primary_instance_options::invalidate_instances);

  return opts;
}

}  // namespace replicaset
}  // namespace dba
}  // namespace mysqlsh
