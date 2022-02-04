/*
 * Copyright (c) 2018, 2022, Oracle and/or its affiliates.
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

#include <string>
#include <vector>

#include "modules/adminapi/cluster/set_primary_instance.h"
#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/sql.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "utils/utils_sqlstring.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

Set_primary_instance::Set_primary_instance(
    const mysqlshdk::db::Connection_options &instance_cnx_opts,
    Cluster_impl *cluster)
    : Topology_configuration_command(cluster),
      m_instance_cnx_opts(instance_cnx_opts) {
  assert(m_instance_cnx_opts.has_data());
}

Set_primary_instance::~Set_primary_instance() {}

void Set_primary_instance::ensure_single_primary_mode() {
  // First ensure that the topology mode match the one registered in the
  // metadata and if not, throw immediately
  // Get the primary UUID value to determine GR mode:
  // UUID (not empty) -> single-primary or "" (empty) -> multi-primary
  std::string gr_primary_uuid = mysqlshdk::gr::get_group_primary_uuid(
      *m_cluster->get_cluster_server(), nullptr);

  // Get the topology mode from the metadata.
  mysqlshdk::gr::Topology_mode metadata_topology_mode =
      m_cluster->get_metadata_storage()->get_cluster_topology_mode(
          m_cluster->get_id());

  // Check if the topology mode match and report needed change in the
  // metadata.
  if ((gr_primary_uuid.empty() &&
       metadata_topology_mode ==
           mysqlshdk::gr::Topology_mode::SINGLE_PRIMARY) ||
      (!gr_primary_uuid.empty() &&
       metadata_topology_mode == mysqlshdk::gr::Topology_mode::MULTI_PRIMARY)) {
    throw shcore::Exception::runtime_error(
        "Operation not allowed: The cluster topology-mode does not match the "
        "one registered in the Metadata. Please use the <Cluster>.rescan() "
        "to repair the issue.");
  } else {
    // If the cluster is in multi-primary mode, throw an error
    if (gr_primary_uuid.empty() &&
        metadata_topology_mode == mysqlshdk::gr::Topology_mode::MULTI_PRIMARY) {
      throw shcore::Exception::runtime_error(
          "Operation not allowed: The cluster is in Multi-Primary mode.");
    }
  }
}

void Set_primary_instance::prepare() {
  // Verify if the cluster is in single-primary mode
  ensure_single_primary_mode();

  m_instance_cnx_opts.set_login_options_from(
      m_cluster->get_cluster_server()->get_connection_options());

  // - Ensure instance belong to cluster;
  std::string target_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());

  std::string address_in_md;
  try {
    m_target_instance = Instance::connect(m_instance_cnx_opts);

    m_target_uuid = m_target_instance->get_uuid();
    address_in_md = m_target_instance->get_canonical_address();
  } catch (const shcore::Error &e) {
    log_debug("Failed query target instance '%s': %s",
              target_instance_address.c_str(), e.format().c_str());
  }

  ensure_target_instance_belongs_to_cluster(target_instance_address,
                                            address_in_md);

  Topology_configuration_command::prepare();
}

shcore::Value Set_primary_instance::execute() {
  auto console = mysqlsh::current_console();
  std::string target_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());

  console->print_info("Setting instance '" + target_instance_address +
                      "' as the primary instance of cluster '" +
                      m_cluster->get_name() + "'...");
  console->print_info();

  // Restore the replication channel if the cluster belongs to a ClusterSet and
  // is a replica cluster
  if (m_cluster->is_cluster_set_member() && !m_cluster->is_primary_cluster()) {
    // We must stop the replication channel at the primary instance first
    // since GR won't allow changing the primary when there are running
    // replication channels.

    // Stop the channel, ensuring all transactions in queue are applied first to
    // ensure no transaction loss
    stop_channel(m_cluster->get_cluster_server().get(),
                 k_clusterset_async_channel_name, true, false);

    // Elect the new primary before starting the replication channel
    mysqlshdk::gr::set_as_primary(*m_cluster_session_instance, m_target_uuid);

    start_channel(m_target_instance.get(), k_clusterset_async_channel_name,
                  false);
  } else {
    // Just elect the new primary
    mysqlshdk::gr::set_as_primary(*m_cluster_session_instance, m_target_uuid);
  }

  // Print information about the instances role changes
  print_cluster_members_role_changes();

  console->print_info("The instance '" + target_instance_address +
                      "' was successfully elected as primary.");

  return shcore::Value();
}

void Set_primary_instance::rollback() {}

void Set_primary_instance::finish() {}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
