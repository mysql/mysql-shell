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

#include "modules/adminapi/replicaset/topology_configuration_command.h"

#include <vector>

#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/validations.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {

Topology_configuration_command::Topology_configuration_command(
    ReplicaSet *replicaset, const shcore::NamingStyle &naming_style)
    : m_replicaset(replicaset), m_naming_style(naming_style) {
  assert(replicaset);
  m_cluster_session_instance = shcore::make_unique<mysqlshdk::mysql::Instance>(
      m_replicaset->get_cluster()->get_group_session());
}

Topology_configuration_command::~Topology_configuration_command() {}

void Topology_configuration_command::ensure_server_version() {
  if (m_cluster_session_instance->get_version() <
      mysqlshdk::utils::Version(8, 0, 13)) {
    throw shcore::Exception::runtime_error(
        "Operation not supported on target server version: '" +
        m_cluster_session_instance->get_version().get_full() + "'");
  }
}

void Topology_configuration_command::
    ensure_target_instance_belongs_to_replicaset(
        const std::string &instance_address,
        const std::string &metadata_address) {
  auto console = mysqlsh::current_console();

  // Check if the instance belongs to the ReplicaSet
  log_debug("Checking if the instance belongs to the Replicaset.");

  bool is_instance_on_md =
      m_replicaset->get_cluster()
          ->get_metadata_storage()
          ->is_instance_on_replicaset(m_replicaset->get_id(), metadata_address);

  if (!is_instance_on_md) {
    std::string err_msg = "The instance '" + instance_address +
                          "' does not belong to the ReplicaSet: '" +
                          m_replicaset->get_member("name").get_string() + "'.";
    throw shcore::Exception::runtime_error(err_msg);
  }
}

void Topology_configuration_command::ensure_all_members_replicaset_online() {
  auto console = mysqlsh::current_console();

  log_debug("Checking if all members of the Replicaset are ONLINE.");

  // Get all cluster instances
  std::vector<Instance_definition> instance_defs =
      m_replicaset->get_cluster()
          ->get_metadata_storage()
          ->get_replicaset_instances(m_replicaset->get_id(), true);

  // Get cluster session to use the same authentication credentials for all
  // replicaset instances.
  Connection_options cluster_cnx_opt =
      m_cluster_session_instance->get_connection_options();

  // Check if all instances have the ONLINE state
  for (const Instance_definition &instance_def : instance_defs) {
    mysqlshdk::gr::Member_state state =
        mysqlshdk::gr::to_member_state(instance_def.state);

    if (state != mysqlshdk::gr::Member_state::ONLINE) {
      console->print_error(
          "The instance '" + instance_def.endpoint + "' has the status: '" +
          mysqlshdk::gr::to_string(state) + "'. All members must be ONLINE.");

      throw shcore::Exception::runtime_error(
          "One or more instances of the cluster are not ONLINE.");
    } else {
      // Instance is ONLINE, initialize and populate the internal replicaset
      // instances list

      // Establish a session to the instance
      // Set login credentials to connect to instance.
      // NOTE: It is assumed that the same login credentials can be used to
      // connect to all replicaset instances.
      std::string instance_address = instance_def.endpoint;

      Connection_options instance_cnx_opts =
          shcore::get_connection_options(instance_address, false);
      instance_cnx_opts.set_login_options_from(cluster_cnx_opt);

      log_debug("Connecting to instance '%s'.", instance_address.c_str());
      std::shared_ptr<mysqlshdk::db::ISession> session;

      mysqlshdk::mysql::Instance instance;

      // Establish a session to the instance
      try {
        session = mysqlshdk::db::mysql::Session::create();
        session->connect(instance_cnx_opts);

        // Add the instance to instances internal list
        m_cluster_instances.emplace_back(
            shcore::make_unique<mysqlshdk::mysql::Instance>(session));
      } catch (const std::exception &err) {
        log_debug("Failed to connect to instance: %s", err.what());

        console->print_error(
            "Unable to connect to instance '" + instance_address +
            "'. Please, verify connection credentials and make sure the "
            "instance is available.");

        throw shcore::Exception::runtime_error(err.what());
      }
    }
  }
}

void Topology_configuration_command::ensure_version_all_members_replicaset(
    mysqlshdk::utils::Version version) {
  auto console = mysqlsh::current_console();

  log_debug("Checking if all members of the Replicaset have a version >= %s.",
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
    if (instance_uri == cluster_session_uri) continue;

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

  log_debug("Updating Replicaset value of topology_type to %s in the Metadata.",
            mysqlshdk::gr::to_string(topology_mode).c_str());

  // Since we're switching to single-primary mode, the active session may not
  // be to the newly elected primary, thereby we must get a session to
  // it to perform the Metadata changes
  if (!mysqlshdk::gr::is_primary(*m_cluster_session_instance)) {
    log_debug(
        "%s is not a primary, will try to find one and reconnect",
        m_cluster_session_instance->get_connection_options().as_uri().c_str());

    auto metadata(mysqlshdk::innodbcluster::Metadata_mysql::create(
        m_cluster_session_instance->get_session()));

    mysqlshdk::innodbcluster::Cluster_group_client cluster(
        metadata, m_cluster_session_instance->get_session());

    std::shared_ptr<mysqlshdk::db::ISession> session_to_primary;

    // Find the primary
    std::string primary_uri;
    try {
      primary_uri = cluster.find_uri_to_any_primary(
          mysqlshdk::innodbcluster::Protocol_type::Classic);
    } catch (const std::exception &e) {
      throw shcore::Exception::runtime_error(
          std::string("Unable to find a primary member in the cluster: ") +
          e.what());
    }

    // Get the session of the primary
    mysqlshdk::db::Connection_options coptions(primary_uri);
    primary_uri =
        coptions.as_uri(mysqlshdk::db::uri::formats::only_transport());

    for (const auto &instance : m_cluster_instances) {
      std::string instance_uri = instance->get_connection_options().as_uri(
          mysqlshdk::db::uri::formats::only_transport());

      if (instance_uri == primary_uri) {
        session_to_primary = instance->get_session();
      }
    }

    std::shared_ptr<MetadataStorage> new_metadata;

    new_metadata = shcore::make_unique<MetadataStorage>(session_to_primary);

    // Update the topology mode in the Metadata
    new_metadata->update_replicaset_topology_mode(m_replicaset->get_id(),
                                                  topology_mode);
  } else {
    // Update the topology mode in the Metadata
    m_replicaset->get_cluster()
        ->get_metadata_storage()
        ->update_replicaset_topology_mode(m_replicaset->get_id(),
                                          topology_mode);
  }
}

void Topology_configuration_command::print_replicaset_members_role_changes() {
  auto console = mysqlsh::current_console();

  // Get the current members list
  std::vector<ReplicaSet::Instance_info> replicaset_instances =
      m_replicaset->get_instances();

  // Get the final status of the members
  std::vector<mysqlshdk::gr::Member> final_members_info(
      mysqlshdk::gr::get_members(*m_cluster_session_instance));

  const ReplicaSet::Instance_info *instance;

  for (const auto &member : m_initial_members_info) {
    std::string old_role = mysqlshdk::gr::to_string(member.role);

    for (const auto &i : replicaset_instances) {
      if (i.uuid == member.uuid) {
        instance = &i;

        for (const auto &member_new : final_members_info) {
          std::string new_role;

          if (member_new.uuid == member.uuid) {
            new_role = mysqlshdk::gr::to_string(member_new.role);

            if (new_role != old_role) {
              console->print_info("Instance '" + instance->classic_endpoint +
                                  "' was switched from " + old_role + " to " +
                                  new_role + ".");
            } else {
              console->print_info("Instance '" + instance->classic_endpoint +
                                  "' remains " + old_role + ".");
            }
          }
        }
      }
    }
  }

  console->println();

  // Warn the user if the current active session is not to the primary anymore
  if (!mysqlshdk::gr::is_primary(*m_cluster_session_instance)) {
    console->print_warning(
        "The cluster internal session is not the primary member "
        "anymore. For cluster management operations please obtain a "
        "fresh cluster handle using <Dba>." +
        shcore::get_member_name("getCluster", m_naming_style) + "().");
    console->println();
  }
}

void Topology_configuration_command::prepare() {
  // Verify if the instance of the current cluster session has a version
  // >= 8.0.13
  // We can throw immediately and avoid the further checks such as the check to
  // verify if any of the cluster members has a version < 8.0.13
  ensure_server_version();

  // Verify user privileges to execute operation;
  ensure_user_privileges(*m_cluster_session_instance);

  // Verify if all the cluster members are ONLINE
  ensure_all_members_replicaset_online();

  // Verify if all cluster members have a version >= 8.0.13
  ensure_version_all_members_replicaset(mysqlshdk::utils::Version(8, 0, 13));

  // Initialize the members_info status at this moment
  m_initial_members_info =
      mysqlshdk::gr::get_members(*m_cluster_session_instance);

  // Get the ReplicaSet Config Object
  m_cfg = m_replicaset->create_config_object();
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
  m_cluster_session_instance.reset();
  m_initial_members_info.clear();
}

}  // namespace dba
}  // namespace mysqlsh
