/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/dba/provision.h"

#include <vector>

#include "mysqlshdk/libs/mysql/group_replication.h"
#include "utils/utils_general.h"

namespace mysqlsh {
namespace dba {

void leave_replicaset(const mysqlshdk::mysql::Instance &instance,
                      std::shared_ptr<mysqlsh::IConsole> console) {
  std::string instance_address = instance.get_connection_options().as_uri(
      mysqlshdk::db::uri::formats::only_transport());

  // Check if the instance is actively member of the cluster before trying to
  // stop it (otherwise stop might fail).
  mysqlshdk::gr::Member_state state = mysqlshdk::gr::get_member_state(instance);
  if (state != mysqlshdk::gr::Member_state::OFFLINE &&
      state != mysqlshdk::gr::Member_state::MISSING) {
    // Stop Group Replication (metadata already removed)
    console->print_info(
        "Attempting to leave from the Group Replication group...");
    mysqlshdk::gr::stop_group_replication(instance);
    // Get final state and log info.
    state = mysqlshdk::gr::get_member_state(instance);
    log_debug("Instance state after stopping Group Replication: %s",
              mysqlshdk::gr::to_string(state).c_str());
  } else {
    console->print_note("The instance '" + instance_address + "' is " +
                        mysqlshdk::gr::to_string(state) +
                        ", Group Replication stop skipped.");
  }

  // Disable and persist GR start on boot and reset values for
  // group_replication_bootstrap_group, group_replication_force_members,
  // group_replication_group_seeds and group_replication_local_address
  // NOTE: Only for server supporting SET PERSIST, version must be >= 8.0.11
  // due to BUG#26495619.
  log_debug(
      "Disabling needed group replication variables after stopping Group "
      "Replication, using SET PERSIST (if supported)");
  if (instance.get_version() >= mysqlshdk::utils::Version(8, 0, 11)) {
    const char *k_gr_remove_instance_vars_default[]{
        "group_replication_bootstrap_group", "group_replication_force_members",
        "group_replication_group_seeds", "group_replication_local_address"};
    instance.set_sysvar("group_replication_start_on_boot", false,
                        mysqlshdk::mysql::Var_qualifier::PERSIST);

    for (auto gr_var : k_gr_remove_instance_vars_default) {
      instance.set_sysvar_default(gr_var,
                                  mysqlshdk::mysql::Var_qualifier::PERSIST);
    }

    bool persist_load = *instance.get_sysvar_bool(
        "persisted_globals_load", mysqlshdk::mysql::Var_qualifier::GLOBAL);
    if (!persist_load) {
      std::string warn_msg =
          "On instance '" + instance_address +
          "' the persisted cluster configuration will not be loaded upon "
          "reboot since 'persisted-globals-load' is set to 'OFF'. Please set "
          "'persisted-globals-load' to 'ON' on the configuration file or set "
          "the 'group_replication_start_on_boot' variable to 'OFF' in the "
          "server configuration file, otherwise it might rejoin the cluster "
          "upon restart.";
      console->print_warning(warn_msg);
    }
  } else {
    std::string warn_msg =
        "On instance '" + instance_address +
        "' configuration cannot be persisted since MySQL version " +
        instance.get_version().get_base() +
        " does not support the SET PERSIST command (MySQL version >= 8.0.11 "
        "required). Please set the 'group_replication_start_on_boot' variable "
        "to 'OFF' in the server configuration file, otherwise it might rejoin "
        "the cluster upon restart.";
    console->print_warning(warn_msg);
  }
}

}  // namespace dba
}  // namespace mysqlsh
