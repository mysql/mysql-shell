/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster/switch_to_multi_primary_mode.h"

#include <string>
#include <vector>

#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/sql.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

Switch_to_multi_primary_mode::Switch_to_multi_primary_mode(
    Cluster_impl *cluster)
    : Topology_configuration_command(cluster) {}

Switch_to_multi_primary_mode::~Switch_to_multi_primary_mode() {}

void Switch_to_multi_primary_mode::prepare() {
  // Do the base class validations
  Topology_configuration_command::prepare();
}

shcore::Value Switch_to_multi_primary_mode::execute() {
  auto console = mysqlsh::current_console();
  console->print_info("Switching cluster '" + m_cluster->get_name() +
                      "' to Multi-Primary mode...");
  console->print_info();

  // Execute the UDF: SELECT group_replication_switch_to_multi_primary_mode()
  mysqlshdk::gr::switch_to_multi_primary_mode(*m_cluster_session_instance);

  // Update the auto-increment values in all cluster members:
  //   - auto_increment_increment = 7
  //   - auto_increment_offset = 1 + server_id % 7
  {
    log_debug("Updating auto_increment values of cluster members");

    // Call update_auto_increment to do the job in all instances
    mysqlshdk::gr::update_auto_increment(
        m_cfg.get(), mysqlshdk::gr::Topology_mode::MULTI_PRIMARY);

    m_cfg->apply();
  }

  // Update the Metadata schema to change the clusters.topology_type value to
  // "pm"
  {
    log_debug(
        "Updating cluster value of topology_type to single-primary in the "
        "Metadata.");

    update_topology_mode_metadata(mysqlshdk::gr::Topology_mode::MULTI_PRIMARY);

    // Update the Cluster object topology_type
    m_cluster->set_topology_type(mysqlshdk::gr::Topology_mode::MULTI_PRIMARY);
  }

  // Print information about the instances role changes
  print_cluster_members_role_changes();

  console->print_info(
      "The cluster successfully switched to Multi-Primary mode.");

  return shcore::Value();
}

void Switch_to_multi_primary_mode::rollback() {}

void Switch_to_multi_primary_mode::finish() {}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
