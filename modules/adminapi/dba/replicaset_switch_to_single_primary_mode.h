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

#ifndef MODULES_ADMINAPI_DBA_REPLICASET_SWITCH_TO_SINGLE_PRIMARY_MODE_H_
#define MODULES_ADMINAPI_DBA_REPLICASET_SWITCH_TO_SINGLE_PRIMARY_MODE_H_

#include <string>

#include "modules/adminapi/dba/replicaset_topology_configuration_command.h"
#include "mysqlshdk/libs/db/connection_options.h"

namespace mysqlsh {
namespace dba {

class Switch_to_single_primary_mode : public Topology_configuration_command {
 public:
  Switch_to_single_primary_mode(
      const mysqlshdk::db::Connection_options &instance_cnx_opts,
      std::shared_ptr<Cluster> cluster, std::shared_ptr<ReplicaSet> replicaset,
      const shcore::NamingStyle &naming_style);

  ~Switch_to_single_primary_mode() override;

  /**
   * Prepare the Switch_to_single_primary_mode command for execution.
   * Validates parameters and others, more specifically:
   * - If the connections options are used:
   *   - Validate the connection options;
   *   - Ensure instance belong to replicaset;
   * - Verify user privileges to execute operation;
   * - Verify if all the cluster members are ONLINE
   * - Verify if all cluster members have a version >= 8.0.13
   * - Verify if the cluster has quorum
   * - Prepare the internal configuration object
   */
  void prepare() override;

  /**
   * Execute the switch_to_single_primary_mode command.
   * More specifically:
   * - Execute the UDF: SELECT group_replication_switch_to_single_primary_mode()
   * - Update the auto-increment values in all cluster members:
   *   - auto_increment_increment = 1
   *   - auto_increment_offset = 2
   * - Update the Metadata schema to change the replicasets.topology_type value
   *   to "pm"
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

 private:
  const mysqlshdk::db::Connection_options &m_instance_cnx_opts;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_DBA_REPLICASET_SWITCH_TO_SINGLE_PRIMARY_MODE_H_
