/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/replicaset/switch_to_single_primary_mode.h"

#include <string>
#include <vector>

#include "modules/adminapi/common/metadata_storage.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/innodbcluster/cluster_metadata.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {

Switch_to_single_primary_mode::Switch_to_single_primary_mode(
    const mysqlshdk::db::Connection_options &instance_cnx_opts,
    ReplicaSet *replicaset)
    : Topology_configuration_command(replicaset),
      m_instance_cnx_opts(instance_cnx_opts) {}

Switch_to_single_primary_mode::~Switch_to_single_primary_mode() {}

void Switch_to_single_primary_mode::prepare() {
  // Do the base class validations
  Topology_configuration_command::prepare();

  // If the connections options are used:
  //   - Validate the connection options;
  //   - Ensure instance belong to replicaset;
  if (m_instance_cnx_opts.has_data()) {
    std::string target_instance_address = m_instance_cnx_opts.as_uri(
        mysqlshdk::db::uri::formats::only_transport());

    log_debug("Verifying connection options.");

    validate_connection_options(m_instance_cnx_opts);

    m_address_in_md = mysqlsh::dba::get_report_host_address(
        m_instance_cnx_opts, m_replicaset->get_cluster()
                                 ->get_group_session()
                                 ->get_connection_options());
    ensure_target_instance_belongs_to_replicaset(target_instance_address,
                                                 m_address_in_md);
  }
}

shcore::Value Switch_to_single_primary_mode::execute() {
  auto console = mysqlsh::current_console();

  console->print_info("Switching cluster '" +
                      m_replicaset->get_cluster()->get_name() +
                      "' to Single-Primary mode...");
  console->println();

  // Execute the UDF: SELECT group_replication_switch_to_single_primary_mode()
  {
    if (m_instance_cnx_opts.has_data()) {
      // Get the server_uuid of the instance
      std::string target_instance_uuid =
          m_replicaset->get_cluster()
              ->get_metadata_storage()
              ->get_new_metadata()
              ->get_instance_uuid_by_address(m_address_in_md);

      mysqlshdk::gr::switch_to_single_primary_mode(*m_cluster_session_instance,
                                                   target_instance_uuid);
    } else {
      mysqlshdk::gr::switch_to_single_primary_mode(*m_cluster_session_instance);
    }
  }

  // Update the auto-increment values in all replicaset members:
  //   - auto_increment_increment = 1
  //   - auto_increment_offset = 2
  {
    log_debug("Updating auto_increment values of replicaset members");

    // Call update_auto_increment to do the job in all instances
    mysqlshdk::gr::update_auto_increment(
        m_cfg.get(), mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY);

    m_cfg->apply();
  }

  // Update the Metadata schema to change the replicasets.topology_type value to
  // "pm"
  {
    log_debug(
        "Updating Replicaset value of topology_type to Single-Primary in the "
        "Metadata.");

    update_topology_mode_metadata(mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY);

    // Update the Replicaset object topology_type
    m_replicaset->set_topology_type(ReplicaSet::kTopologySinglePrimary);
  }

  // Print information about the instances role changes
  print_replicaset_members_role_changes();

  // Print warning about connections that expect a R/W connection
  console->print_warning(
      "Existing connections that expected a R/W connection must be "
      "disconnected, i.e. instances that became SECONDARY.");

  console->println();

  console->print_info(
      "The cluster successfully switched to Single-Primary mode.");

  return shcore::Value();
}

void Switch_to_single_primary_mode::rollback() {}

void Switch_to_single_primary_mode::finish() {}

}  // namespace dba
}  // namespace mysqlsh
