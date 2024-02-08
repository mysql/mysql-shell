/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/adminapi/cluster/topology_configuration_command.h"

#include <vector>

#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/validations.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

Topology_configuration_command::Topology_configuration_command(
    Cluster_impl *cluster)
    : m_cluster(cluster) {
  assert(cluster);
  m_cluster_session_instance = m_cluster->get_cluster_server();
}

void Topology_configuration_command::ensure_server_version() {
  if (m_cluster_session_instance->get_version() <
      mysqlshdk::utils::Version(8, 0, 13)) {
    throw shcore::Exception::runtime_error(
        "Operation not supported on target server version: '" +
        m_cluster_session_instance->get_version().get_full() + "'");
  }
}

void Topology_configuration_command::ensure_target_instance_belongs_to_cluster(
    const std::string &target_uuid, const std::string &target_address) {
  auto console = mysqlsh::current_console();

  // Check if the instance belongs to the Cluster
  log_debug("Checking if the instance belongs to the cluster.");

  Instance_metadata rr_md;

  try {
    rr_md =
        m_cluster->get_metadata_storage()->get_instance_by_uuid(target_uuid);
  } catch (const shcore::Exception &err) {
    if (err.code() != SHERR_DBA_MEMBER_METADATA_MISSING) throw;

    std::string err_msg = "The instance '" + target_address +
                          "' does not belong to the Cluster: '" +
                          m_cluster->get_name() + "'.";
    throw shcore::Exception::runtime_error(err_msg);
  }

  // It belongs to the MD, check now if it is a Read-Replica
  if (rr_md.instance_type == Instance_type::READ_REPLICA) {
    std::string err_msg =
        "Unable to set '" + target_address +
        "' as the primary instance of the Cluster: instance is a Read-Replica.";
    throw shcore::Exception::runtime_error(err_msg);
  }
}

void Topology_configuration_command::connect_all_members() {
  // Get cluster session to use the same authentication credentials for all
  // cluster instances.
  Connection_options cluster_cnx_opt =
      m_cluster_session_instance->get_connection_options();

  auto console = mysqlsh::current_console();

  // Check if all instances have the ONLINE state
  for (const auto &instance_def : m_cluster->get_instances_with_state()) {
    // Instance is ONLINE, initialize and populate the internal cluster
    // instances list

    // Establish a session to the instance
    // Set login credentials to connect to instance.
    // NOTE: It is assumed that the same login credentials can be used to
    // connect to all cluster instances.
    Connection_options instance_cnx_opts =
        shcore::get_connection_options(instance_def.first.endpoint, false);
    instance_cnx_opts.set_login_options_from(cluster_cnx_opt);

    log_debug("Connecting to instance '%s'.",
              instance_def.first.endpoint.c_str());

    // Establish a session to the instance
    try {
      // Add the instance to instances internal list
      m_cluster_instances.emplace_back(Instance::connect(instance_cnx_opts));
    } catch (const std::exception &err) {
      log_debug("Failed to connect to instance: %s", err.what());

      console->print_error(
          "Unable to connect to instance '" + instance_def.first.endpoint +
          "'. Please, verify connection credentials and make sure the "
          "instance is available.");

      throw shcore::Exception::runtime_error(err.what());
    }
  }
}

void Topology_configuration_command::ensure_version_all_members_cluster(
    mysqlshdk::utils::Version version) {
  auto console = mysqlsh::current_console();

  log_debug("Checking if all members of the cluster have a version >= %s.",
            version.get_full().c_str());

  std::string cluster_session_uri =
      m_cluster_session_instance->get_connection_options().as_uri(
          mysqlshdk::db::uri::formats::only_transport());

  // Check if any instance has a version < 8.0.13
  for (const auto &instance : m_cluster_instances) {
    std::string instance_uri = instance->get_connection_options().as_uri(
        mysqlshdk::db::uri::formats::only_transport());
    // Skip the current cluster instance since it was already checked in
    // ensure_server_version()
    if (mysqlshdk::utils::are_endpoints_equal(instance_uri,
                                              cluster_session_uri))
      continue;

    // Verify if the instance version is supported
    if (instance->get_version() < version) {
      std::string instance_address = instance->get_connection_options().as_uri(
          mysqlshdk::db::uri::formats::only_transport());

      console->print_error("The instance '" + instance_address +
                           "' has the version " +
                           instance->get_version().get_full() +
                           " which does not support this operation.");

      throw shcore::Exception::runtime_error(
          "One or more instances of the cluster have a version that does not "
          "support this operation.");
    }
  }
}

void Topology_configuration_command::update_topology_mode_metadata(
    const mysqlshdk::gr::Topology_mode &topology_mode) {
  auto console = mysqlsh::current_console();

  log_debug("Updating cluster value of topology_type to %s in the Metadata.",
            mysqlshdk::gr::to_string(topology_mode).c_str());

  // Since we're switching to single-primary mode, the active session may not
  // be to the newly elected primary, thereby we must get a session to
  // it to perform the Metadata changes
  m_cluster->acquire_primary();

  shcore::Scoped_callback finally([&]() { m_cluster->release_primary(); });

  // Update the topology mode in the Metadata
  m_cluster->get_metadata_storage()->update_cluster_topology_mode(
      m_cluster->get_id(), topology_mode);
}

void Topology_configuration_command::print_cluster_members_role_changes() {
  // Get the current members list
  std::vector<Instance_metadata> cluster_instances = m_cluster->get_instances();

  // ensure a consistent order is used when writting to the console
  std::sort(cluster_instances.begin(), cluster_instances.end(),
            [](const auto &a, const auto &b) {
              return std::less<std::string>()(a.address, b.address);
            });

  // Get the final status of the members
  std::vector<mysqlshdk::gr::Member> final_members_info(
      mysqlshdk::gr::get_members(*m_cluster_session_instance));

  std::vector<std::string> remained, switched;
  for (const auto &member : m_initial_members_info) {
    auto old_role = mysqlshdk::gr::to_string(member.role);

    for (const auto &instance : cluster_instances) {
      if (instance.uuid != member.uuid) continue;

      for (const auto &member_new : final_members_info) {
        if (member_new.uuid != instance.uuid) continue;

        auto new_role = mysqlshdk::gr::to_string(member_new.role);

        if (new_role != old_role) {
          switched.push_back(shcore::str_format(
              "Instance '%s' was switched from %s to %s.",
              instance.endpoint.c_str(), old_role.c_str(), new_role.c_str()));
        } else {
          remained.push_back(shcore::str_format("Instance '%s' remains %s.",
                                                instance.endpoint.c_str(),
                                                old_role.c_str()));
        }
      }
    }
  }

  auto console = mysqlsh::current_console();

  for (const auto &i : remained) console->print_info(i);
  for (const auto &i : switched) console->print_info(i);
  console->print_info();
}

void Topology_configuration_command::prepare() {
  // Verify if the instance of the current cluster session has a version
  // >= 8.0.13
  // We can throw immediately and avoid the further checks such as the check
  // to verify if any of the cluster members has a version < 8.0.13
  ensure_server_version();

  // Verify user privileges to execute operation;
  ensure_user_privileges(*m_cluster_session_instance);

  // Establishes a session to all the cluster members
  connect_all_members();

  // Verify if all cluster members have a version >= 8.0.13
  ensure_version_all_members_cluster(mysqlshdk::utils::Version(8, 0, 13));

  // Initialize the members_info status at this moment
  m_initial_members_info =
      mysqlshdk::gr::get_members(*m_cluster_session_instance);

  // Get the Cluster Config Object
  m_cfg = m_cluster->create_config_object();
}

shcore::Value Topology_configuration_command::execute() {
  return shcore::Value();
}

void Topology_configuration_command::rollback() {}

void Topology_configuration_command::finish() {
  // Close all sessions to cluster instances.
  for (const auto &instance : m_cluster_instances) {
    instance->close_session();
  }

  // Reset all auxiliary (temporary) data used for the operation execution.
  m_initial_members_info.clear();
}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
