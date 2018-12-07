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

#ifndef MODULES_ADMINAPI_REPLICASET_TOPOLOGY_CONFIGURATION_COMMAND_H_
#define MODULES_ADMINAPI_REPLICASET_TOPOLOGY_CONFIGURATION_COMMAND_H_

#include <string>
#include <vector>

#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/command_interface.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace dba {

class Topology_configuration_command : public Command_interface {
 public:
  Topology_configuration_command(ReplicaSet *replicaset,
                                 const shcore::NamingStyle &naming_style);

  ~Topology_configuration_command() override;

  /**
   * Prepare the Topology_configuration_command command for execution.
   * Validates parameters and others, more specifically:
   * - If the connections options are used:
   *   - Validate the connection options;
   *   - Ensure instance belong to replicaset;
   * - Verify user privileges to execute operation;
   * - Verify if all the cluster members are ONLINE
   * - Verify if all cluster members have a version >= 8.0.13
   * - Verify if the cluster has quorum
   */
  void prepare() override;

  /**
   * Execute the Cluster_topology_configuration command.
   *
   * NOTE: execution is implemented by sub-class
   *
   * @return An empty shcore::Value.
   */
  shcore::Value execute() override;

  /**
   * Rollback the command.
   *
   * NOTE: Not currently used.
   */
  void rollback() override;

  /**
   * Finalize the command execution.
   */
  void finish() override;

 protected:
  ReplicaSet *m_replicaset = nullptr;
  const shcore::NamingStyle m_naming_style;

  std::unique_ptr<mysqlshdk::mysql::Instance> m_cluster_session_instance;

  // Configuration object (to read and set instance configurations).
  std::unique_ptr<mysqlshdk::config::Config> m_cfg;

  /**
   * Verify if the instance of the current cluster session has a supported
   * version
   */
  void ensure_server_version();

  /**
   * Verify if the instance belongs to the replicaset
   */
  void ensure_target_instance_belongs_to_replicaset(
      const std::string &instance_address);

  /**
   * Verify if all replicaset members are ONLINE and initialize the internal
   * replicaset instances list
   */
  void ensure_all_members_replicaset_online();

  /**
   * Verify if all replicaset members have a version >= version
   */
  void ensure_version_all_members_replicaset(mysqlshdk::utils::Version version);

  /**
   * Auxiliary method to update the topology_type value in the Metadata.
   */
  void update_topology_mode_metadata(
      const mysqlshdk::gr::Topology_mode &topology_mode);

  /**
   * Print information about the replicaset members role changes
   */
  void print_replicaset_members_role_changes();

 private:
  std::vector<mysqlshdk::gr::Member> m_initial_members_info;
  std::vector<std::unique_ptr<mysqlshdk::mysql::Instance>> m_cluster_instances;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_REPLICASET_TOPOLOGY_CONFIGURATION_COMMAND_H_
