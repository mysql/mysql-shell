/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_ADD_INSTANCE_H_
#define MODULES_ADMINAPI_CLUSTER_ADD_INSTANCE_H_

#include <string>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/command_interface.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

class Add_instance : public Command_interface {
 public:
  Add_instance(const mysqlshdk::db::Connection_options &instance_cnx_opts,
               Cluster_impl *cluster,
               const Group_replication_options &gr_options,
               const Clone_options &clone_options,
               const mysqlshdk::utils::nullable<std::string> &instance_label,
               bool interactive, Recovery_progress_style wait_recovery,
               const std::string &replication_user = "",
               const std::string &replication_password = "",
               bool rebooting = false);

  Add_instance(const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
               Cluster_impl *cluster,
               const Group_replication_options &gr_options,
               const Clone_options &clone_options,
               const mysqlshdk::utils::nullable<std::string> &instance_label,
               bool interactive, Recovery_progress_style wait_recovery,
               const std::string &replication_user = "",
               const std::string &replication_password = "",
               bool rebooting = false);

  ~Add_instance() override;

  /**
   * Prepare the add_instance command for execution.
   * Validates parameters and others, more specifically:
   * - Create or reuse Instance for target and peer;
   * - Check GR options;
   * - Validate target instance address;
   * - Validate replication filters;
   * - Resolve the SSL Mode (use same as cluster);
   * - Ensure target instance does not belong to cluster;
   * - Validate target instance UUID;
   * - Get the report host value (to be used by GR and Metadata);
   * - Resolve the GR local address;
   * - Validate options (failover consistency and expel timeout) in cluster;
   * - Check Instance configuration (if needed);
   * - Prepare Config object;
   */
  void prepare() override;

  /**
   * Execute the add_instance command.
   * More specifically:
   * - Log used GR options/settings;
   * - Handle creation of recovery (replication) user;
   * - Install GR plugin (if needed);
   * - If seed instance: start cluster (bootstrap GR);
   * - If not seed instance: join cluster;
   * - Add instance to Metadata (if needed);
   * - Update GR group seeds on cluster members;
   * - Update auto-increment setting in cluster members;
   *
   * @return an empty shcore::Value.
   */
  shcore::Value execute() override;

  /**
   * Rollback the command.
   * Perform some cleaning in case the operation fails:
   * - drop the replication (recovery) user if created.
   */
  void rollback() override;

  /**
   * Finalize the command execution.
   * More specifically:
   * - Close the target instance and peer connection (if established by this
   *   operation);
   */
  void finish() override;

 private:
  mysqlshdk::db::Connection_options m_instance_cnx_opts;
  Cluster_impl *m_cluster = nullptr;
  Group_replication_options m_gr_opts;
  Clone_options m_clone_opts;
  mysqlshdk::utils::nullable<std::string> m_instance_label;
  bool m_interactive;
  std::string m_rpl_user;
  std::string m_rpl_pwd;

  // TODO(pjesus): remove 'm_rebooting' for refactor of reboot cluster
  //               (WL#11561), since mysqlsh::dba::start_cluster() should be
  //               used directly instead of the Add_instance operation.
  bool m_rebooting;

  std::string m_instance_address;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  bool m_reuse_session_for_target_instance = false;
  std::string m_address_in_metadata;
  std::shared_ptr<mysqlsh::dba::Instance> m_peer_instance;
  bool m_use_cluster_session_for_peer = true;

  // Configuration object (to read and set instance configurations).
  std::unique_ptr<mysqlshdk::config::Config> m_cfg;

  bool m_clone_supported = false;
  bool m_clone_disabled = false;

  Recovery_progress_style m_progress_style;

  int64_t m_restore_clone_threshold = 0;

  void ensure_instance_check_installed_schema_version() const;

  /**
   * Validate the use of IPv6 addresses on the localAddress of the
   * target instance and check if the target instance supports usage of
   * IPv6 on the localAddress values being used on the cluster instances.
   */
  void validate_local_address_ip_compatibility() const;

  void resolve_ssl_mode();
  void handle_gr_protocol_version();
  bool handle_replication_user();
  void clean_replication_user();
  void log_used_gr_options();
  void ensure_unique_server_id() const;
  void handle_recovery_account() const;
  void update_change_master() const;

  void refresh_target_connections();

  /*
   * Handle the loading/unloading of the clone plugin on the target cluster
   * and the target instance
   */
  void handle_clone_plugin_state(bool enable_clone);
};

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_ADD_INSTANCE_H_
