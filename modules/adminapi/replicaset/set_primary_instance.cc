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

#include <string>
#include <vector>

#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/replicaset/set_primary_instance.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/innodbcluster/cluster_metadata.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "utils/utils_sqlstring.h"

namespace mysqlsh {
namespace dba {

Set_primary_instance::Set_primary_instance(
    const mysqlshdk::db::Connection_options &instance_cnx_opts,
    ReplicaSet *replicaset, const shcore::NamingStyle &naming_style)
    : Topology_configuration_command(replicaset, naming_style),
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
      m_replicaset->get_cluster()->get_group_session(), nullptr);

  // Get the topology mode from the metadata.
  mysqlshdk::gr::Topology_mode metadata_topology_mode =
      m_replicaset->get_cluster()
          ->get_metadata_storage()
          ->get_replicaset_topology_mode(m_replicaset->get_id());

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

  // - Validate the connection options;
  log_debug("Verifying connection options.");
  validate_connection_options(m_instance_cnx_opts);

  // - Ensure instance belong to replicaset;
  std::string target_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());
  m_address_in_md = mysqlsh::dba::get_report_host_address(
      m_instance_cnx_opts, m_replicaset->get_cluster()
                               ->get_group_session()
                               ->get_connection_options());
  ensure_target_instance_belongs_to_replicaset(target_instance_address,
                                               m_address_in_md);

  Topology_configuration_command::prepare();
}

shcore::Value Set_primary_instance::execute() {
  auto console = mysqlsh::current_console();
  std::string target_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());

  console->print_info("Setting instance '" + target_instance_address +
                      "' as the primary instance of cluster '" +
                      m_replicaset->get_cluster()->get_name() + "'...");
  console->println();

  // Execute the UDF: SELECT group_replication_set_as_primary(member_uuid)
  {
    // Get the server_uuid of the instance
    std::string target_instance_uuid =
        m_replicaset->get_cluster()
            ->get_metadata_storage()
            ->get_new_metadata()
            ->get_instance_uuid_by_address(m_address_in_md);

    mysqlshdk::gr::set_as_primary(*m_cluster_session_instance,
                                  target_instance_uuid);
  }

  // Print information about the instances role changes
  print_replicaset_members_role_changes();

  console->print_info("The instance '" + target_instance_address +
                      "' was successfully elected as primary.");

  return shcore::Value();
}

void Set_primary_instance::rollback() {}

void Set_primary_instance::finish() {}

}  // namespace dba
}  // namespace mysqlsh
