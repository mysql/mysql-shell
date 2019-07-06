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
constexpr const char kGtidSetIsComplete[] = "gtidSetIsComplete";

namespace {
/**
 * Validate if clone is supported on the target instance version so the option
 * is supported too.
 *
 * @param version version of the target instance
 * @param option option name
 * @param target Clone_options unpack target: CREATE or JOIN
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
    case Clone_options::CREATE:
      if (!is_option_supported(version, option,
                               k_global_cluster_supported_options)) {
        throw shcore::Exception::runtime_error(
            "Option '" + option +
            "' not supported on target server "
            "version: '" +
            version.get_full() + "'");
      }
      break;
    case Clone_options::JOIN:
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
              "' not supported on target server "
              "version: '" +
              version.get_full() + "'");
        }
      }
      break;
    case Clone_options::NONE:
      break;
  }
}

}  // namespace

// ----

void Clone_options::do_unpack(shcore::Option_unpacker *unpacker) {
  switch (target) {
    case NONE:
      break;
    case CREATE:
      unpacker->optional(kDisableClone, &disable_clone)
          .optional(kGtidSetIsComplete, &gtid_set_is_complete);
      break;
    case JOIN: {
      std::string s;
      unpacker->optional(kRecoveryMethod, &s);
      if (shcore::str_caseeq(s, "auto"))
        recovery_method = Member_recovery_method::AUTO;
      else if (shcore::str_caseeq(s, "clone"))
        recovery_method = Member_recovery_method::CLONE;
      else if (shcore::str_caseeq(s, "incremental"))
        recovery_method = Member_recovery_method::INCREMENTAL;
      else
        recovery_method_str_invalid = s;
    } break;
  }
}

void Clone_options::check_option_values(
    const mysqlshdk::utils::Version &version, const ReplicaSet *replicaset) {
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
              ->get_group_session()
              ->get_connection_options(),
          {}, [&trg](const std::shared_ptr<mysqlshdk::db::ISession> &session) {
            mysqlshdk::mysql::Instance instance(session);

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
}

}  // namespace dba
}  // namespace mysqlsh
