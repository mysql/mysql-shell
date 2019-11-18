/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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
#include "modules/adminapi/common/common.h"
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
      if (!is_option_supported(version, option,
                               k_global_cluster_supported_options)) {
        throw shcore::Exception::runtime_error(
            "Option '" + option +
            "' not supported on target server "
            "version: '" +
            version.get_full() + "'");
      }
      break;
    case Clone_options::JOIN_CLUSTER:
    case Clone_options::JOIN_REPLICASET:
      if (version < mysqlshdk::mysql::k_mysql_clone_plugin_initial_version) {
        if (cluster) {
          throw shcore::Exception::runtime_error(
              "Option '" + option +
              "' not supported on target cluster. One or more members have a "
              "non-supported version: '" +
              version.get_full() + "'");
        } else {
          throw shcore::Exception::runtime_error(
              "Option '" + option +
              "' not supported on target server version: '" +
              version.get_full() + "'");
        }
      }
      break;
    case Clone_options::NONE:
      break;
  }
}

/**
 * Validate the value specified for the cloneDonor option.
 *
 * @param clone_donor string containing the value we want to set for
 * clone_valid_donor_list
 * @throw ArgumentError if the value is empty or no host and port is specified
 *                      (i.e., value is ":").
 */
void validate_clone_donor_option(std::string clone_donor) {
  clone_donor = shcore::str_strip(clone_donor);
  if (clone_donor.empty()) {
    throw shcore::Exception::argument_error(
        "Invalid value for cloneDonor, string value cannot be empty.");
  }

  if (clone_donor[0] == '[')
    throw shcore::Exception::argument_error(
        "IPv6 addresses not supported for cloneDonor");

  try {
    mysqlshdk::utils::split_host_and_port(clone_donor);
  } catch (const std::invalid_argument &e) {
    throw shcore::Exception::argument_error(
        std::string("Invalid value for cloneDonor: ") + e.what());
  }
}
}  // namespace

// ----

void Clone_options::do_unpack(shcore::Option_unpacker *unpacker) {
  std::string tmp_recovery_method;

  switch (target) {
    case NONE:
      break;
    case CREATE_CLUSTER:
      unpacker->optional(kDisableClone, &disable_clone)
          .optional(kGtidSetIsComplete, &gtid_set_is_complete);
      break;
    case JOIN_CLUSTER:
      unpacker->optional(kRecoveryMethod, &tmp_recovery_method);
      break;
    case JOIN_REPLICASET:
      unpacker->optional(kRecoveryMethod, &tmp_recovery_method)
          .optional(kCloneDonor, &clone_donor);
      break;
  }

  // Validate recoveryMethod
  if (shcore::str_caseeq(tmp_recovery_method, "auto"))
    recovery_method = Member_recovery_method::AUTO;
  else if (shcore::str_caseeq(tmp_recovery_method, "clone"))
    recovery_method = Member_recovery_method::CLONE;
  else if (shcore::str_caseeq(tmp_recovery_method, "incremental"))
    recovery_method = Member_recovery_method::INCREMENTAL;
  else
    recovery_method_str_invalid = tmp_recovery_method;
}

void Clone_options::check_option_values(
    const mysqlshdk::utils::Version &version, const GRReplicaSet *replicaset) {
  if (!disable_clone.is_null()) {
    // Validate if the disableClone option is supported on the target
    validate_clone_supported(version, kDisableClone, target);
  } else if (recovery_method == Member_recovery_method::CLONE) {
    // Validate if the recoveryMethod option is supported on the target
    validate_clone_supported(version, std::string(kRecoveryMethod) + "=clone",
                             target);

    // Verify if ALL the cluster members support clone
    if (replicaset) {
      Unpack_target trg = target;

      replicaset->execute_in_members(
          {mysqlshdk::gr::Member_state::ONLINE,
           mysqlshdk::gr::Member_state::RECOVERING},
          replicaset->get_cluster()
              ->get_target_instance()
              ->get_connection_options(),
          {}, [&trg](const std::shared_ptr<mysqlshdk::db::ISession> &session) {
            mysqlsh::dba::Instance instance(session);

            validate_clone_supported(instance.get_version(),
                                     std::string(kRecoveryMethod) + "=clone",
                                     trg, true);

            return true;
          });
    }
  }

  // Validate the recoveryMethod option when it wasn't unpack to match "auto",
  // "regular" or "clone" - recovery_method_std_invalis is not empty
  if (!recovery_method_str_invalid.empty()) {
    throw shcore::Exception::argument_error(
        std::string("Invalid value for option ") + kRecoveryMethod + ": " +
        recovery_method_str_invalid);
  }

  // If cloneDonor was set and recoveryMethod not or not set to 'clone',
  // error out
  if (!clone_donor.is_null()) {
    if (!recovery_method.is_null()) {
      if (*recovery_method != Member_recovery_method::CLONE) {
        throw shcore::Exception::argument_error(
            std::string("Option ") + kCloneDonor + " only allowed if option " +
            kRecoveryMethod + " is set to 'clone'.");
      }
    } else {
      throw shcore::Exception::argument_error(
          std::string("Option ") + kCloneDonor + " only allowed if option " +
          kRecoveryMethod + " is used and set to 'clone'.");
    }
  }

  // Validate clone_donor
  if (!clone_donor.is_null()) {
    validate_clone_donor_option(*clone_donor);
  }

  // Finally, if recoveryMethod wasn't set, set the default value of AUTO
  if (recovery_method.is_null()) {
    recovery_method = Member_recovery_method::AUTO;
  }
}

}  // namespace dba
}  // namespace mysqlsh
