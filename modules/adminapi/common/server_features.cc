/*
 * Copyright (c) 2023, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/server_features.h"

#include <functional>

#include "modules/adminapi/common/common.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/group_replication.h"

namespace mysqlsh::dba {

std::string get_communication_stack(
    const mysqlshdk::mysql::IInstance &target_instance) {
  // Get the communication stack in used. The default value must be XCOM if the
  // sysvar doesn't exist (< 8.0.27)
  return target_instance
      .get_sysvar_string("group_replication_communication_stack")
      .value_or(kCommunicationStackXCom);
}

int64_t get_transaction_size_limit(
    const mysqlshdk::mysql::IInstance &target_instance) {
  // Get it from the primary member
  return target_instance.get_sysvar_int(kGrTransactionSizeLimit).value_or(0);
}

std::optional<bool> get_paxos_single_leader_enabled(
    const mysqlshdk::mysql::IInstance &target_instance) {
  // The table performance_schema.replication_group_communication_information
  // is a pluggable table (P_S v2), meaning that it won't exist if the Group
  // Replication plugin is either uninstalled or disabled
  auto is_gr_active = std::invoke(
      [](const mysqlshdk::mysql::IInstance &instance) {
        std::optional<std::string> plugin_state =
            instance.get_plugin_status(mysqlshdk::gr::k_gr_plugin_name);
        if (!plugin_state.has_value() ||
            plugin_state.value_or("DISABLED") != "ACTIVE") {
          return false;
        }
        return true;
      },
      target_instance);

  // Get the effective value of paxos_single_leader in the cluster
  if (!is_gr_active) {
    log_info(
        "Unable to retrieve the value of "
        "WRITE_CONSENSUS_SINGLE_LEADER_CAPABLE: Group Replication is not "
        "active on the instance '%s'",
        target_instance.descr().c_str());

    return std::nullopt;
  }

  int paxos_single_leader = target_instance.queryf_one_int(
      0, 0,
      "SELECT WRITE_CONSENSUS_SINGLE_LEADER_CAPABLE FROM "
      "performance_schema.replication_group_communication_information");

  return paxos_single_leader == 1;
}

bool supports_mysql_communication_stack(
    const mysqlshdk::utils::Version &version) {
  return version >= k_mysql_communication_stack_initial_version;
}

bool supports_paxos_single_leader(const mysqlshdk::utils::Version &version) {
  return version >= k_paxos_single_leader_initial_version;
}

bool supports_mysql_clone(const mysqlshdk::utils::Version &version) {
  return version >= k_mysql_clone_plugin_initial_version;
}

bool supports_repl_channel_compression(
    const mysqlshdk::utils::Version &version) {
  return version >= mysqlshdk::utils::Version(8, 0, 18);
}

bool supports_repl_channel_network_namespace(
    const mysqlshdk::utils::Version &version) {
  return version >= mysqlshdk::utils::Version(8, 0, 22);
}

}  // namespace mysqlsh::dba
