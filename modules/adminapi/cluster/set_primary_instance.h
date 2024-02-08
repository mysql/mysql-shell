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

#ifndef MODULES_ADMINAPI_CLUSTER_SET_PRIMARY_INSTANCE_H_
#define MODULES_ADMINAPI_CLUSTER_SET_PRIMARY_INSTANCE_H_

#include <string>

#include "modules/adminapi/cluster/topology_configuration_command.h"
#include "mysqlshdk/libs/db/connection_options.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

class Set_primary_instance final : public Topology_configuration_command {
 public:
  Set_primary_instance(
      const mysqlshdk::db::Connection_options &instance_cnx_opts,
      Cluster_impl *cluster,
      const cluster::Set_primary_instance_options &options);

  ~Set_primary_instance() override = default;

  /**
   * Prepare the Set_primary_instance command for execution.
   * Validates parameters and others, more specifically:
   *   - Validate the connection options;
   *   - Ensure instance belong to cluster;
   * - Verify if the cluster is in single-primary mode
   * - Verify if all cluster members have a version >= 8.0.13
   * - Verify if the cluster quorum
   * - Verify if all the cluster members are ONLINE
   * - Verify user privileges to execute operation;
   */
  void prepare() override;

  /**
   * Execute the set_primary_instance command.
   * More specifically:
   * - Execute the UDF: SELECT group_replication_set_as_primary(member_uuid)
   *
   * @return An empty shcore::Value.
   */
  shcore::Value execute() override;

  /**
   * Rollback the command.
   *
   * NOTE: Not currently used.
   */
  void rollback() override {}

  /**
   * Finalize the command execution.
   */
  void finish() override {}

 private:
  std::optional<uint32_t> m_runningTransactionsTimeout;
  mysqlshdk::db::Connection_options m_instance_cnx_opts;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  std::string m_target_uuid;

  /**
   * Verify if the cluster is in single-primary mode
   */
  void ensure_single_primary_mode();
};

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_SET_PRIMARY_INSTANCE_H_
