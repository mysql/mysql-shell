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

#include <set>

#include "modules/adminapi/cluster_set/api_options.h"
#include "modules/adminapi/common/common.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"

namespace mysqlsh {
namespace dba {
namespace clusterset {

const std::set<std::string> kClusterSetReplicationSslModeValues = {
    kClusterSSLModeAuto, kClusterSSLModeRequired, kClusterSSLModeDisabled};

const shcore::Option_pack_def<Create_cluster_set_options>
    &Create_cluster_set_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Create_cluster_set_options>()
          .optional(kDryRun, &Create_cluster_set_options::dry_run)
          .optional(kClusterSetReplicationSslMode,
                    &Create_cluster_set_options::set_ssl_mode);

  return opts;
}

void Create_cluster_set_options::set_ssl_mode(const std::string &value) {
  if (kClusterSetReplicationSslModeValues.count(shcore::str_upper(value)) ==
      0) {
    std::string valid_values =
        shcore::str_join(kClusterSetReplicationSslModeValues, ",");
    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value for %s option. Supported values: %s.",
        kClusterSetReplicationSslMode, valid_values.c_str()));
  }

  // Set the ssl-mode
  ssl_mode = to_cluster_ssl_mode(value);
}

const shcore::Option_pack_def<Create_replica_cluster_options>
    &Create_replica_cluster_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Create_replica_cluster_options>()
          .include<Interactive_option>()
          .optional(kDryRun, &Create_replica_cluster_options::dry_run)
          .optional(kRecoveryVerbosity,
                    &Create_replica_cluster_options::set_recovery_verbosity)
          .optional(kTimeout, &Create_replica_cluster_options::set_timeout)
          .include(&Create_replica_cluster_options::gr_options)
          .include(&Create_replica_cluster_options::clone_options);

  return opts;
}

void Create_replica_cluster_options::set_timeout(int value) {
  if (value < 0) {
    throw shcore::Exception::argument_error(
        shcore::str_format("%s option must be >= 0", kTimeout));
  }

  timeout = value;
}

void Create_replica_cluster_options::set_recovery_verbosity(int value) {
  // Validate waitRecovery option UInteger [0, 2]
  if (value < 0 || value > 2) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value '%d' for option '%s'. It must be an "
                           "integer in the range [0, 2].",
                           value, kRecoveryVerbosity));
  }

  recovery_verbosity = value;
}

const shcore::Option_pack_def<Remove_cluster_options>
    &Remove_cluster_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Remove_cluster_options>()
          .optional(kDryRun, &Remove_cluster_options::dry_run)
          .optional(kTimeout, &Remove_cluster_options::set_timeout)
          .optional(kForce, &Remove_cluster_options::force);

  return opts;
}

void Remove_cluster_options::set_timeout(int value) {
  if (value < 0) {
    throw shcore::Exception::argument_error(
        shcore::str_format("%s option must be >= 0", kTimeout));
  }

  timeout = value;
}

const shcore::Option_pack_def<Status_options> &Status_options::options() {
  static const auto opts = shcore::Option_pack_def<Status_options>().optional(
      kExtended, &Status_options::set_extended);

  return opts;
}

void Status_options::set_extended(uint64_t value) {
  // Validate extended option UInteger [0, 3] or Boolean.
  if (value > 3) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value '%" PRIu64
                           "' for option '%s'. It must be an integer in the "
                           "range [0, 3].",
                           value, kExtended));
  }

  extended = value;
}

}  // namespace clusterset
}  // namespace dba

}  // namespace mysqlsh
