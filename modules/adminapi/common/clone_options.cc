/*
 * Copyright (c) 2019, 2021, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/clone_options.h"

#include <set>
#include <string>
#include <vector>
#include "adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/common.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/utils/utils_net.h"

namespace mysqlsh {
namespace dba {

constexpr const char kRecoveryMethod[] = "recoveryMethod";
constexpr const char kCloneDonor[] = "cloneDonor";

namespace {
/**
 * Validate if clone is supported on the target instance version so the option
 * is supported too.
 *
 * @param version version of the target instance
 * @param option option name
 * @param target Clone_options unpack target: CREATE_CLUSTER or JOIN_CLUSTER
 * @param cluster boolean value to indicate whether the validation is happening
 * for the whole cluster or not
 * @throw RuntimeError if the option is not supported on the target instance
 */
void validate_clone_supported(const mysqlshdk::utils::Version &version,
                              const std::string &option,
                              Clone_options::Unpack_target target,
                              bool cluster = false) {
  // Any clone option shall only be allowed if the target MySQL
  // server version is >= k_mysql_clone_plugin_initial_version
  switch (target) {
    case Clone_options::CREATE_CLUSTER:
      if (!is_option_supported(
              version, option,
              {{kDisableClone,
                {"",
                 mysqlshdk::mysql::k_mysql_clone_plugin_initial_version,
                 {}}}})) {
        throw shcore::Exception::runtime_error(shcore::str_format(
            "Option '%s' not supported on target server version: '%s'",
            option.c_str(), version.get_full().c_str()));
      }
      break;
    case Clone_options::JOIN_CLUSTER:
    case Clone_options::CREATE_REPLICA_CLUSTER:
    case Clone_options::JOIN_REPLICASET:
      if (version < mysqlshdk::mysql::k_mysql_clone_plugin_initial_version) {
        if (cluster) {
          throw shcore::Exception::runtime_error(shcore::str_format(
              "Option '%s' not supported on target cluster. One or more "
              "members have a non-supported version: '%s'",
              option.c_str(), version.get_full().c_str()));
        } else {
          throw shcore::Exception::runtime_error(shcore::str_format(
              "Option '%s' not supported on target server version: '%s'",
              option.c_str(), version.get_full().c_str()));
        }
      }
      break;
    case Clone_options::NONE:
      break;
  }
}

}  // namespace

// ----

void Clone_options::check_option_values(
    const mysqlshdk::utils::Version &version, const Cluster_impl *cluster) {
  if (!disable_clone.is_null()) {
    // Validate if the disableClone option is supported on the target
    validate_clone_supported(version, kDisableClone, target);
  } else if (recovery_method == Member_recovery_method::CLONE) {
    // Validate if the recoveryMethod option is supported on the target
    validate_clone_supported(version, std::string(kRecoveryMethod) + "=clone",
                             target);

    // Verify if ALL the cluster members support clone
    if (cluster) {
      Unpack_target trg = target;

      cluster->execute_in_members(
          {mysqlshdk::gr::Member_state::ONLINE,
           mysqlshdk::gr::Member_state::RECOVERING},
          cluster->get_cluster_server()->get_connection_options(), {},
          [&trg](const std::shared_ptr<Instance> &instance,
                 const mysqlshdk::gr::Member &) {
            validate_clone_supported(instance->get_version(),
                                     std::string(kRecoveryMethod) + "=clone",
                                     trg, true);

            return true;
          });
    }
  }

  // Finally, if recoveryMethod wasn't set, set the default value of AUTO
  if (recovery_method.is_null()) {
    recovery_method = Member_recovery_method::AUTO;
  }
}

void Clone_options::set_clone_donor(const std::string &value) {
  clone_donor = shcore::str_strip(value);

  if (clone_donor->empty()) {
    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value for %s, string value cannot be empty.", kCloneDonor));
  }

  if ((*clone_donor)[0] == '[')
    throw shcore::Exception::argument_error(
        shcore::str_format("IPv6 addresses not supported for %s", kCloneDonor));

  try {
    mysqlshdk::utils::split_host_and_port(*clone_donor);
  } catch (const std::invalid_argument &e) {
    throw shcore::Exception::argument_error(
        shcore::str_format("Invalid value for %s: %s", kCloneDonor, e.what()));
  }

  // If cloneDonor was set and recoveryMethod not or not set to 'clone',
  // error out
  if (!recovery_method.is_null()) {
    if (*recovery_method != Member_recovery_method::CLONE) {
      throw shcore::Exception::argument_error(shcore::str_format(
          "Option %s only allowed if option %s is set to 'clone'.", kCloneDonor,
          kRecoveryMethod));
    }
  } else {
    throw shcore::Exception::argument_error(shcore::str_format(
        "Option %s only allowed if option %s is used and set to 'clone'.",
        kCloneDonor, kRecoveryMethod));
  }
}

const shcore::Option_pack_def<Create_cluster_clone_options>
    &Create_cluster_clone_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Create_cluster_clone_options>()
          .optional(kDisableClone,
                    &Create_cluster_clone_options::set_disable_clone)
          .optional(kGtidSetIsComplete,
                    &Create_cluster_clone_options::set_gtid_set_is_complete);

  return opts;
}

void Create_cluster_clone_options::set_disable_clone(bool value) {
  disable_clone = value;
}

void Create_cluster_clone_options::set_gtid_set_is_complete(bool value) {
  gtid_set_is_complete = value;
}

const shcore::Option_pack_def<Join_cluster_clone_options>
    &Join_cluster_clone_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Join_cluster_clone_options>().optional(
          kRecoveryMethod, &Join_cluster_clone_options::set_recovery_method);

  return opts;
}

void Join_cluster_clone_options::set_recovery_method(const std::string &value) {
  // Validate recoveryMethod
  if (shcore::str_caseeq(value, "auto")) {
    recovery_method = Member_recovery_method::AUTO;
  } else if (shcore::str_caseeq(value, "clone")) {
    recovery_method = Member_recovery_method::CLONE;
  } else if (shcore::str_caseeq(value, "incremental")) {
    recovery_method = Member_recovery_method::INCREMENTAL;
  } else {
    throw shcore::Exception::argument_error(shcore::str_format(
        "Invalid value for option %s: %s", kRecoveryMethod, value.c_str()));
  }
}

const shcore::Option_pack_def<Join_replicaset_clone_options>
    &Join_replicaset_clone_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Join_replicaset_clone_options>()
          .include<Join_cluster_clone_options>()
          .optional(kCloneDonor,
                    &Join_replicaset_clone_options::set_clone_donor);

  return opts;
}

const shcore::Option_pack_def<Create_replica_cluster_clone_options>
    &Create_replica_cluster_clone_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Create_replica_cluster_clone_options>()
          .include<Join_cluster_clone_options>()
          .optional(kCloneDonor,
                    &Create_replica_cluster_clone_options::set_clone_donor);

  return opts;
}

}  // namespace dba
}  // namespace mysqlsh
