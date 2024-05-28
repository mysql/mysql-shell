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

#ifndef MODULES_ADMINAPI_COMMON_PROVISION_H_
#define MODULES_ADMINAPI_COMMON_PROVISION_H_

#include <memory>
#include <string>
#include <vector>

#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/adminapi/common/instance_pool.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/repl_config.h"

namespace mysqlsh {
namespace dba {

/**
 * Remove the specified instance from its current cluster (using Group
 * Replication).
 *
 * In more detail, the following steps are executed:
 * - Stop Group Replication;
 * - Reset/disable necessary Group Replication variables on removed instance;
 *
 * NOTE: Metedata is NOT changed by this function, it only remove the instance
 *       from the MySQL Group Replication (cluster) and update any necessary
 *       variables on the instance itself.
 *
 * @param instance target Instance object to remove from the cluster.
 * @param reset_member_actions Boolean value to indicate if the Group
 * Replication member actions should be reset
 * @param reset_repl_channels Boolean value to indicate if the internal Group
 * Replication channel must be reset
 */
void leave_cluster(const mysqlsh::dba::Instance &instance,
                   bool reset_member_actions = false,
                   bool reset_repl_channels = true, bool silent = false);

/**
 * Check the instance configuration.
 *
 * In more detail, the following checks are performed:
 * - Specific Group Replication prerequisites;
 * - server_id;
 * - MTS settings;
 * - Option file (my.cnf) configurations (when provided through the Config
 *   object);
 *
 * @param instance Instance object of the target server to check.
 * @param config Config object that holds configuration handlers to check the
 *               required settings on the server and option file (e.g., my.cnf)
 *               if available.
 * @param cluster_type Type of cluster the instance is meant for.
 * @return a vector with the invalid configurations resulting from the check.
 *
 */
std::vector<mysqlshdk::mysql::Invalid_config> check_instance_config(
    const mysqlshdk::mysql::IInstance &instance,
    const mysqlshdk::config::Config &config, Cluster_type cluster_type);

/**
 * Configure instance.
 *
 * This function updates the required configurations based on the result
 * returned by the check_instance_config() function. Therefore, it requires
 * check_instance_config() to be executed first, using its result as input to
 * perform the required configurations.
 *
 * @param config Config object that holds configuration handlers to set the
 *               required settings on the server and option file (e.g., my.cnf)
 *               if available.
 * @param invalid_configs Vector of Invalid_config structures returned by the
 *                        check_instance_config() function with all the
 *                        required information to update each configuration
 *                        (e.g., variable name, value required, type of
 *                        configuration, etc.).
 * @param version Version of the target instance
 * @return bool value indicating if a restart is required (true) or not (false).
 */
bool configure_instance(
    mysqlshdk::config::Config *config,
    const std::vector<mysqlshdk::mysql::Invalid_config> &invalid_configs,
    const mysqlshdk::utils::Version &version);

/**
 * Persist Group Replication configuration.
 *
 * This function persist (save) the Group Replication (GR) configurations to the
 * option file. This feature is required for server version that do not
 * support the SET PERSIST feature.
 *
 * @param instance Instance object of the target server to read GR
 *                 configurations.
 * @param config Config object that holds a configuration handler to the server
 *               option file (e.g., my.cnf) to save the configurations.
 *
 */
void persist_gr_configurations(const mysqlshdk::mysql::IInstance &instance,
                               mysqlshdk::config::Config *config);

/**
 * Start the cluster (Group Replication group) using the given instance.
 *
 * This function starts the Group Replication (GR) group but it does not
 * create the recovery user nor sets it, thus that step must be performed
 * afterwards to allow the instance to recover from other members in case
 * of a fault. The recovery user should be created after starting the
 * cluster to ensure that the group UUID is associated to that transaction.
 *
 * NOTE: Metadata is NOT changed by this function, it only starts MySQL
 *       Group Replication (cluster), updating any necessary variables
 *       on the target instance.
 *       Any variables changes are persisted (using SET PERSIST) if
 *       supported by the used server version (through the config object).
 *
 * @param instance target Instance object to start the cluster.
 * @param gr_opts Group_replication_options structure with the GR options
 *                to set for the instance (i.e., group_name, ssl_mode,
 *                local_address, group_seeds, ip_allowlist, member_weight,
 *                expel_timeout, exit_state_action, and failover_consistency)
 *                when defined.
 * @param requires_certificates If true, the server certificates (ssl_cert and
 *                              ssl_key) are read and set in the corresponding
 *                              'group_replication_recovery_' variables.
 * @param multi_primary optional boolean indicating the GR topology mode that
 *                      will be set. Multi-primary mode if true and
 *                      single-primary mode if false, otherwise not set.
 * @param config Config object for the target instance to start the cluster.
 */
void start_cluster(const mysqlshdk::mysql::IInstance &instance,
                   const Group_replication_options &gr_opts,
                   bool requires_certificates,
                   std::optional<bool> multi_primary,
                   mysqlshdk::config::Config *config);

/**
 * Join the instance to the cluster (Group Replication group).
 *
 * To join instances and to obtain any required information from the cluster,
 * a peer instance (group session) is used as contact node of the Group
 * Replication (GR) group. In more detail, the following steps are executed:
 * - Get needed information (GR configurations) from peer instance;
 * - Set Group Replication configurations;
 * - Set the Group Replication recovery user (using CHANGE MASTER);
 * - Start Group Replication;
 *
 * NOTE: Metadata is NOT changed by this function, it only starts MySQL
 *       Group Replication (cluster), updating any necessary variables
 *       on the instance itself.
 *       Any variables changes are persisted (using SET PERSIST) if
 *       supported by the used server version.
 *
 * @param instance target Instance object to join the cluster.
 * @param peer_instance Instance object of the member used as contact node
 *                      to join.
 * @param gr_opts Group_replication_options structure with the GR options
 *                to set for the instance (i.e., ssl_mode, local_address,
 *                group_seeds, ip_allowlist, member_weight, expel_timeout,
 *                exit_state_action, and failover_consistency) when defined.
 * @param requires_certificates If true, the server certificates (ssl_cert and
 *                              ssl_key) are read and set in the corresponding
 *                              'group_replication_recovery_' variables.
 * @param replicaset_size integer with the size of the cluster (before adding
 *                        the instance).
 * @param config Config object for the target instance to join the cluster.
 */
void join_cluster(const mysqlshdk::mysql::IInstance &instance,
                  const mysqlshdk::mysql::IInstance &peer_instance,
                  const Group_replication_options &gr_opts,
                  bool requires_certificates,
                  std::optional<uint64_t> cluster_size,
                  mysqlshdk::config::Config *config);

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_PROVISION_H_
