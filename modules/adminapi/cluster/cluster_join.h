/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_CLUSTER_JOIN_H_
#define MODULES_ADMINAPI_CLUSTER_CLUSTER_JOIN_H_

#include <memory>
#include <string>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/adminapi/common/instance_validations.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

class Cluster_join {
 public:
  Cluster_join(Cluster_impl *cluster, mysqlsh::dba::Instance *primary_instance,
               const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
               const Group_replication_options &gr_options,
               const Clone_options &clone_options, bool interactive,
               std::optional<bool> switch_comm_stack = false);

  /**
   * Prepare the Cluster_join command for execution.
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
   *
   * bootstrap and rejoin will skip one-time preparations, but configurations
   * will be reset.
   */
  void prepare_join(
      const mysqlshdk::utils::nullable<std::string> &instance_label);

  bool check_rejoinable(bool *out_uuid_mistmatch = nullptr);
  bool prepare_rejoin(bool *out_uuid_mistmatch = nullptr);

  void prepare_reboot();

  /**
   * Execute the Cluster_join command.
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
   * bootstrap and rejoin will skip one-time preparations, but configurations
   * will be reset.
   */
  void join(Recovery_progress_style wait_recovery);
  void rejoin();
  void reboot();

 private:
  void ensure_instance_check_installed_schema_version() const;
  /**
   * Validate the use of IPv6 addresses on the localAddress of the
   * target instance and check if the target instance supports usage of
   * IPv6 on the localAddress values being used on the cluster instances.
   */
  void validate_local_address_ip_compatibility(
      const std::string &local_address, const std::string &group_seeds,
      checks::Check_type check_type) const;

  void resolve_local_address(Group_replication_options *gr_options,
                             const Group_replication_options &user_gr_options,
                             checks::Check_type check_type);

  void resolve_ssl_mode();
  /**
   * Create a recovery account for the target instance with the required
   * privileges
   */
  bool create_replication_user();

  /**
   * Create a local recovery account for the target instance
   *
   * Required when using the 'MySQL' communication stack. The account is created
   * locally only to the target instance and with binary logging disabled to not
   * introduce errant transactions
   */
  void create_local_replication_user();
  void clean_replication_user();
  void log_used_gr_options();
  void ensure_unique_server_id() const;
  void store_cloned_replication_account() const;
  void restore_group_replication_account() const;
  void check_cluster_members_limit() const;

  /**
   * Check if the target instance supported the communication protocol in used
   * by the Cluster
   */
  void check_comm_stack_support();

  void refresh_target_connections();

  void check_instance_configuration(checks::Check_type type);

  Member_recovery_method check_recovery_method(bool clone_disabled);
  void wait_recovery(const std::string &join_begin_time,
                     Recovery_progress_style progress_style);

  void update_group_peers(int cluster_member_count,
                          const std::string &self_address,
                          bool group_seeds_only = false);

  /*
   * Handle the loading/unloading of the clone plugin on the target cluster
   * and the target instance
   */
  void handle_clone_plugin_state(bool enable_clone);

  /*
   * Enable skip_slave_start and configures the managed replication channel if
   * the target cluster is a replica.
   */
  void configure_cluster_set_member();

  /**
   * Change the recovery user credentials of all Cluster members
   *
   * @param repl_account The authentication options of the recovery account
   * @param repl_account_host The hostname of the recovery account
   */
  void change_recovery_credentials_all_members(
      const mysqlshdk::mysql::Auth_options &repl_account,
      const std::string &repl_account_host) const;

  /**
   * Recreate the recovery account for each Cluster member
   *
   * Required when using the 'MySQL' communication stack to ensure each active
   * member of the Cluster will use its own recovery account instead of the one
   * created whenever a new member joined since all possible seeds had to be
   * updated to use that same account. This ensures distributed recovery is
   * possible at any circumstance.
   */
  void restore_recovery_account_all_members() const;

  /**
   * Validate the options used in addInstance() and rejoinInstance()
   *
   * The function validates if the options used in
   * addInstance()/rejoinInstance() are accepted according to the communication
   * stack in use by the Cluster
   */
  void validate_add_rejoin_options() const;

 private:
  Cluster_impl *m_cluster = nullptr;
  mysqlsh::dba::Instance *m_primary_instance;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  Group_replication_options m_gr_opts;
  std::string m_account_host;
  Clone_options m_clone_opts;
  bool m_interactive = false;

  mysqlshdk::utils::nullable<std::string> m_instance_label;

  bool m_already_member = false;
  bool m_is_autorejoining = false;
  std::optional<bool> m_is_switching_comm_stack;

  std::string m_comm_stack;
};

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_CLUSTER_JOIN_H_
