/*
 * Copyright (c) 2016, 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_CLUSTER_IMPL_H_
#define MODULES_ADMINAPI_CLUSTER_CLUSTER_IMPL_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "mysql/instance.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/shell_options.h"

#include "modules/adminapi/cluster/api_options.h"
#include "modules/adminapi/cluster_set/api_options.h"
#include "modules/adminapi/common/base_cluster_impl.h"
#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/adminapi/common/gtid_validations.h"
#include "modules/adminapi/common/instance_validations.h"
#include "mysqlshdk/include/shellcore/shell_notifications.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/async_replication.h"

namespace mysqlsh {
namespace dba {

// User provided option to disabling clone
inline constexpr std::string_view k_cluster_attribute_disable_clone{
    "opt_disableClone"};
// Flag to indicate the default cluster in the metadata
inline constexpr std::string_view k_cluster_attribute_default{"default"};

// Whether group_replication_start_on_boot should be enabled in each instance
// (false by default)
inline constexpr std::string_view k_cluster_attribute_manual_start_on_boot{
    "opt_manualStartOnBoot"};
// Timestamp of when the instance was added to the group
inline constexpr std::string_view k_instance_attribute_join_time{"joinTime"};

inline constexpr std::string_view k_instance_attribute_server_id{"server_id"};

// replication account certificate subject
inline constexpr std::string_view k_instance_attribute_cert_subject{
    "opt_certSubject"};

inline constexpr const char *k_cluster_read_replica_user_name =
    "mysql_innodb_replica_";

inline constexpr const char
    *k_instance_attribute_read_replica_replication_sources =
        "replicationSources";

class MetadataStorage;
struct Cluster_metadata;

class Cluster_set_impl;
class ClusterSet;

namespace checks {
enum class Check_type;
}

enum class Replication_account_type { GROUP_REPLICATION, READ_REPLICA };

class Cluster_impl final : public Base_cluster_impl,
                           public std::enable_shared_from_this<Cluster_impl>,
                           public shcore::NotificationObserver {
 public:
  friend class Cluster;

  using Instance_md_and_gr_member =
      std::pair<Instance_metadata, mysqlshdk::gr::Member>;

  Cluster_impl(const std::shared_ptr<Cluster_set_impl> &cluster_set,
               const Cluster_metadata &metadata,
               const std::shared_ptr<Instance> &group_server,
               const std::shared_ptr<MetadataStorage> &metadata_storage,
               Cluster_availability availability);

  Cluster_impl(const Cluster_metadata &metadata,
               const std::shared_ptr<Instance> &group_server,
               const std::shared_ptr<MetadataStorage> &metadata_storage,
               Cluster_availability availability);

  Cluster_impl(const std::string &cluster_name, const std::string &group_name,
               const std::shared_ptr<Instance> &group_server,
               const std::shared_ptr<Instance> &primary_master,
               const std::shared_ptr<MetadataStorage> &metadata_storage,
               mysqlshdk::gr::Topology_mode topology_type);
  virtual ~Cluster_impl();

  // API functions
  void add_instance(const Connection_options &instance_def,
                    const cluster::Add_instance_options &options);
  void rejoin_instance(const Connection_options &instance_def,
                       const cluster::Rejoin_instance_options &options);
  void remove_instance(const Connection_options &instance_def,
                       const cluster::Remove_instance_options &options);
  shcore::Value describe();
  shcore::Value options(const bool all);
  shcore::Value status(int64_t extended);
  shcore::Value list_routers(bool only_upgrade_required) override;

  shcore::Dictionary_t routing_options(const std::string &router) override;
  void set_routing_option(const std::string &option,
                          const shcore::Value &value);
  void set_routing_option(const std::string &router, const std::string &option,
                          const shcore::Value &value) override;
  void force_quorum_using_partition_of(const Connection_options &instance_def,
                                       const bool interactive);
  void dissolve(std::optional<bool> force, const bool interactive);
  void rescan(const cluster::Rescan_options &options);
  void switch_to_single_primary_mode(const Connection_options &instance_def);
  void switch_to_multi_primary_mode();
  void set_primary_instance(
      const Connection_options &instance_def,
      const cluster::Set_primary_instance_options &options);
  shcore::Value check_instance_state(const Connection_options &instance_def);
  void reset_recovery_password(std::optional<bool> force,
                               const bool interactive);
  void fence_all_traffic();
  void fence_writes();
  void unfence_writes();
  bool is_fenced_from_writes() const;
  shcore::Value create_cluster_set(
      const std::string &domain_name,
      const clusterset::Create_cluster_set_options &options);
  std::shared_ptr<ClusterSet> get_cluster_set();

  Cluster_availability cluster_availability() const { return m_availability; }

  void check_cluster_online();

  void add_replica_instance(
      const Connection_options &instance_def,
      const cluster::Add_replica_instance_options &options);

  // Class functions
  void sanity_check() const;

  void add_metadata_for_instance(
      const mysqlshdk::db::Connection_options &instance_definition,
      Instance_type instance_type, const std::string &label = "",
      Transaction_undo *undo = nullptr, bool dry_run = false) const;

  void add_metadata_for_instance(const mysqlshdk::mysql::IInstance &instance,
                                 Instance_type instance_type,
                                 const std::string &label = "",
                                 Transaction_undo *undo = nullptr,
                                 bool dry_run = false) const;

  void remove_instance_metadata(
      const mysqlshdk::db::Connection_options &instance_def);

  void update_metadata_for_instance(
      const mysqlshdk::db::Connection_options &instance_definition,
      Instance_id instance_id = 0, const std::string &label = "") const;

  void update_metadata_for_instance(const mysqlshdk::mysql::IInstance &instance,
                                    Instance_id instance_id = 0,
                                    const std::string &label = "") const;

  bool get_disable_clone_option() const;
  void set_disable_clone_option(const bool disable_clone);

  bool get_manual_start_on_boot_option() const;

  const std::string &get_group_name() const { return m_group_name; }

  const std::string get_view_change_uuid() const;

  Cluster_type get_type() const override {
    return Cluster_type::GROUP_REPLICATION;
  }

  std::string get_topology_type() const override {
    return to_string(m_topology_type);
  }

  mysqlshdk::gr::Topology_mode get_cluster_topology_type() const {
    return m_topology_type;
  }

  void set_topology_type(mysqlshdk::gr::Topology_mode topology_type) {
    m_topology_type = topology_type;
  }

  std::pair<mysqlshdk::mysql::Auth_options, std::string>
  create_replication_user(mysqlshdk::mysql::IInstance *target,
                          std::string_view auth_cert_subject,
                          Replication_account_type account_type,
                          bool only_on_target = false,
                          mysqlshdk::mysql::Auth_options creds = {},
                          bool print_recreate_node = true,
                          bool dry_run = false) const;

  std::pair<Async_replication_options, std::string>
  create_read_replica_replication_user(mysqlshdk::mysql::IInstance *target,
                                       std::string_view auth_cert_subject,
                                       int sync_timeout,
                                       bool dry_run = false) const;

  void drop_read_replica_replication_user(
      Instance *target, bool dry_run = false,
      mysqlshdk::mysql::Sql_undo_list *undo = nullptr);

  void setup_read_replica_connection_failover(
      const Instance &replica, const Instance &source,
      cluster::Replication_sources replication_sources, bool rejoin,
      bool dry_run);

  void setup_read_replica(Instance *replica,
                          const Async_replication_options &ar_options,
                          cluster::Replication_sources replication_sources,
                          bool rejoin, bool dry_run);

  void validate_replication_sources(
      const std::set<Managed_async_channel_source,
                     std::greater<Managed_async_channel_source>> &sources,
      const std::string &target_instance_address,
      std::vector<std::string> *out_sources_canonical_address = nullptr) const;

  cluster::Replication_sources get_read_replica_replication_sources(
      const std::string &replica_uuid) const;

  /**
   * Reset the password of the recovery_account or the target instance's
   * recovery account and update the credentials for the recovery channel
   *
   * @param target           Target instance
   * @param recovery_account Username of the recovery account that will have the
   *                         password reset. If empty, the function will use the
   *                         target_instance recovery account
   * @param hosts            List of hosts on which the password must be reset
   */
  void reset_recovery_password(
      const std::shared_ptr<Instance> &target,
      const std::string &recovery_account = "",
      const std::vector<std::string> &hosts = {}) const;

  /**
   * Recreate the recovery replication account for the instance instance
   *
   * Drops and creates a new recovery account for the target instance, ensuring
   * the credentials for the recovery channel are updated too
   *
   * @param target          Target instance
   */
  std::pair<std::string, std::string> recreate_replication_user(
      const std::shared_ptr<Instance> &target,
      std::string_view auth_cert_subject, bool dry_run = false) const;

  bool drop_replication_user(mysqlshdk::mysql::IInstance *target,
                             const std::string &endpoint = "",
                             const std::string &server_uuid = "",
                             const uint32_t server_id = 0, bool dry_run = false,
                             mysqlshdk::mysql::Sql_undo_list *undo = nullptr);

  void drop_replication_users(mysqlshdk::mysql::Sql_undo_list *undo = nullptr);

  /**
   * Wipes out all replication users created/managed by the AdminAPI
   *
   * All accounts matching the format of users created by the AdminAPI are
   * removed:
   *
   *   - mysql_innodb_cluster_%
   *   - mysql_innodb_cs_%
   */
  void wipe_all_replication_users();

  bool contains_instance_with_address(const std::string &host_port) const;

  mysqlsh::dba::Instance *acquire_primary(
      bool primary_required = true, bool check_primary_status = false) override;

  Cluster_metadata get_metadata() const;

  void release_primary() override;

  shcore::Value cluster_status(int64_t extended);
  shcore::Value cluster_describe();

  Cluster_status cluster_status(int *out_num_failures_tolerated = nullptr,
                                int *out_num_failures = nullptr) const;

  void setup_admin_account(const std::string &username, const std::string &host,
                           const Setup_account_options &options) override;
  void setup_router_account(const std::string &username,
                            const std::string &host,
                            const Setup_account_options &options) override;

  /**
   * Get the lowest server version of the cluster members.
   *
   * The version information is obtained from available (ONLINE and RECOVERING)
   * members, ignoring not available instances.
   *
   * @param out_instances_addresses canonical addresses of the instances with
   * the lowest version
   *
   * @return Version object of the lowest instance version in the cluster.
   */
  mysqlshdk::utils::Version get_lowest_instance_version(
      std::vector<std::string> *out_instances_addresses = nullptr) const;

  /**
   * Determine the replication (recovery) user being used by the target
   * instance.
   *
   * A user can have several accounts (for different hostnames).
   * This method will try to obtain the recovery user from the metadata and if
   * the user is not found in the Metadata it will be obtained using the
   * get_recovery_user method (in order to support older versions of the shell).
   * It will throw an exception if the user was not created by the AdminAPI,
   * i.e. does not start with the 'mysql_innodb_cluster_' prefix
   * Note: The hostname values are not quoted
   *
   * @param target_instance Instance whose recovery user we want to know
   * @return a tuple with the recovery user name, the list of hostnames of
   * for recovery user and a boolean that is true if the information was
   * retrieved from the metadata and false otherwise.
   * @throws shcore::Exception::runtime_error if user not created by AdminAPI
   */
  std::tuple<std::string, std::vector<std::string>, bool> get_replication_user(
      const mysqlshdk::mysql::IInstance &target_instance,
      bool is_read_replica = false) const;

  /**
   * Get mismatched recovery accounts.
   *
   * This function returns instances for which the recovery account name doesn't
   * match with its server id.
   *
   * @return a list of {server_id, user} pairs, for each mismatch account
   * found.
   */
  std::unordered_map<uint32_t, std::string> get_mismatched_recovery_accounts()
      const;

  /**
   * Get unused recovery accounts / users.
   *
   * This function returns recovery accounts that exist but aren't being used
   * in either instances of a given cluster. The list excludes mismatch
   * accounts.
   *
   * @param mismatched_recovery_accounts the list of mismatched account (@see
   * get_mismatched_recovery_accounts)
   *
   * @return the list of unused recovery accounts: pairs of {users/hosts}
   */
  std::vector<std::tuple<std::string, std::string>>
  get_unused_recovery_accounts(const std::unordered_map<uint32_t, std::string>
                                   &mismatched_recovery_accounts) const;

  /**
   * Get a session to a given instance of the cluster.
   *
   * This function verifies if it is possible to connect to the given instance,
   * i.e., if it is reachable and if so returns an Instance object with an open
   * session otherwise throws an exception.
   *
   * @param instance_address String with the address <host>:<port> of the
   *                         instance to connect to.
   *
   * @returns shared_ptr to the Instance if it is reachable.
   * @throws Exception if the instance is not reachable.
   */
  std::shared_ptr<Instance> get_session_to_cluster_instance(
      const std::string &instance_address) const;

  /**
   * Setup the clone plugin usage on the cluster (enable or disable it)
   *
   * This function will install or uninstall the clone plugin on all the cluster
   * members If a member is unreachable or missing an error will be printed
   * indicating the function failed to install/uninstall the clone plugin on
   * that instance. The function will return the number of instances that failed
   * to be updated.
   *
   * @param enable_clone Boolean to indicate if the clone plugin must be
   * installed (true) or uninstalled (false.
   *
   * @return size_t with the number of instances on which the update process
   * failed
   */
  size_t setup_clone_plugin(bool enable_clone) const;

  void execute_in_members(
      const std::vector<mysqlshdk::gr::Member_state> &states,
      const mysqlshdk::db::Connection_options &cnx_opt,
      const std::vector<std::string> &ignore_instances_vector,
      const std::function<bool(const std::shared_ptr<Instance> &instance,
                               const mysqlshdk::gr::Member &gr_member)>
          &functor,
      const std::function<bool(const Instance_md_and_gr_member &md)>
          &condition = nullptr,
      bool ignore_network_conn_errors = true) const;

  void execute_in_read_replicas(
      const std::vector<mysqlshdk::mysql::Read_replica_status> &states,
      const mysqlshdk::db::Connection_options &cnx_opt,
      const std::vector<std::string> &ignore_instances_vector,
      const std::function<bool(const Instance &instance)> &functor,
      bool ignore_network_conn_errors = true) const;

  void execute_in_members(
      const std::function<bool(const std::shared_ptr<Instance> &instance,
                               const Instance_md_and_gr_member &info)>
          &on_connect,
      const std::function<bool(const shcore::Error &error,
                               const Instance_md_and_gr_member &info)>
          &on_connect_error = {}) const;

  void execute_in_read_replicas(
      const std::function<bool(const std::shared_ptr<Instance> &instance,
                               const Instance_metadata &md)> &on_connect,
      const std::function<bool(const shcore::Error &error,
                               const Instance_metadata &info)>
          &on_connect_error = {}) const;

  /**
   * Get the primary instance address.
   *
   * This function retrieves the address of a primary instance in the
   * cluster. In single primary mode, only one instance is the primary and
   * its address is returned, otherwise it is assumed that multi primary mode
   * is used. In multi primary mode, the address of any available instance
   * is returned.
   */
  mysqlshdk::db::Connection_options pick_seed_instance() const;

  /**
   * Validate the value of var_name variable on the target instance to see if it
   * is compatible with the value used on the cluster. An exception is thrown in
   * case the values are different.
   *
   * @param target_instance Target instance rejoining the cluster
   * @param var_name name of the variable to check
   */
  void validate_variable_compatibility(
      const mysqlshdk::mysql::IInstance &target_instance,
      const std::string &var_name) const;

  std::map<std::string, std::string> get_cluster_group_seeds() const;

  std::string get_cluster_group_seeds_list(
      std::string_view skip_uuid = "") const;

  /**
   * Get ONLINE and RECOVERING instances from the cluster.
   *
   * @return vector with the Instance definition of the ONLINE
   * and RECOVERING instances.
   */
  std::vector<Instance_metadata> get_active_instances(
      bool online_only = false) const;

  /**
   * Get the list of instances in the states described in the states vector.
   *
   * @param states Vector of strings with the states of members whose
   * instance_metadata we will retrieve from the cluster. If the states vector
   * is empty, return the list of all instances.
   *
   * @return vector with the Instance definitions
   */
  std::vector<Instance_metadata> get_instances(
      const std::vector<mysqlshdk::gr::Member_state> &states = {}) const;

  std::vector<Instance_metadata> get_replica_instances() const;

  std::vector<Instance_metadata> get_all_instances() const;

  std::vector<Instance_metadata> get_instances_from_metadata() const override;

  /**
   * Ensures the server_id of each instance is registered as an attribute on the
   * instances table. If an instance does not have it then inserts it.
   *
   * @param target_instance: The instance to be used to get the cluster members
   * to update the server_id attribute on the instances table.
   */
  void ensure_metadata_has_server_id(
      const mysqlshdk::mysql::IInstance &target_instance) const;

  /**
   * Checks for missing metadata recovery account entries, and fix them using
   * info from mysql.slave_master_info.
   *
   * @param allow_non_standard_format If true, account that don't follow the
   * AdminAPI format are stored, otherwise, a message is generated and the
   * account ignored.
   *
   */
  void ensure_metadata_has_recovery_accounts(bool allow_non_standard_format);

  /**
   * Get an online instance from the cluster.
   *
   * Return an online instance from the cluster that is reachable (able
   * to connect to it), excluding the one with the given UUID if specified.
   *
   * @param exclude_uuid optional string with the UUID of an instance to
   *        exclude. By default, "" (empty) meaning that no instance will be
   *        excluded.
   * @return shared pointer with an Instance object for the first online
   *         instance found, or a nullptr if no instance is available (not
   *         reachable).
   */
  std::shared_ptr<mysqlsh::dba::Instance> get_online_instance(
      const std::string &exclude_uuid = "") const;

  void validate_server_ids(
      const mysqlshdk::mysql::IInstance &target_instance) const;

  /**
   * Return list of instances registered in metadata along with their current
   * GR member state.
   */
  std::vector<Instance_md_and_gr_member> get_instances_with_state(
      bool allow_offline = false) const;

  /**
   * Iterate over all the read replicas in the cluster
   */
  void iterate_read_replicas(
      const std::function<bool(const Instance_metadata &,
                               const mysqlshdk::mysql::Replication_channel &)>
          &cb) const;

  std::unique_ptr<mysqlshdk::config::Config> create_config_object(
      const std::vector<std::string> &ignored_instances = {},
      bool skip_invalid_state = false, bool persist_only = false,
      bool best_effort = false, bool allow_cluster_offline = false) const;

  void query_group_wide_option_values(
      mysqlshdk::mysql::IInstance *target_instance,
      std::optional<std::string> *out_gr_consistency,
      std::optional<int64_t> *out_gr_member_expel_timeout) const;

  /**
   * Update the cluster members according to the removed instance.
   *
   * More specifically, remove the specified local address from the
   * group_replication_group_seeds variable of all alive members of the
   * cluster and then remove the recovery user used by the instance on the
   * other members through a primary instance.
   *
   * @param local_gr_address string with the local GR address (XCom) to remove.
   * @param dry_run boolean value to indicate if the command should do a dry
   * run
   */
  void update_group_members_for_removed_member(const std::string &server_uuid,
                                               bool dry_run = false);

  void adopt_from_gr();

  /**
   * Enables super_read_only in the whole Cluster
   *
   * This functions enables super_read_only in the Cluster, starting by the
   * primary instance and proceeding to the remaining secondary members
   */
  void enable_super_read_only_globally() const;

  void refresh_connections();

  void ensure_compatible_clone_donor(
      const mysqlshdk::mysql::IInstance &donor,
      const mysqlshdk::mysql::IInstance &recipient) override;

  // Lock methods

  [[nodiscard]] mysqlshdk::mysql::Lock_scoped get_lock_shared(
      std::chrono::seconds timeout = {}) override;

  [[nodiscard]] mysqlshdk::mysql::Lock_scoped get_lock_exclusive(
      std::chrono::seconds timeout = {}) override;

 public:
  // clusterset related methods
  const Cluster_set_member_metadata &get_clusterset_metadata() const;

  bool is_cluster_set_member(const std::string &cs_id = "") const;
  bool is_read_replica(
      const mysqlshdk::mysql::IInstance &target_instance) const;
  bool is_instance_cluster_member(
      const mysqlshdk::mysql::IInstance &target_instance) const;
  bool is_instance_cluster_set_member(
      const mysqlshdk::mysql::IInstance &target_instance) const;
  bool is_invalidated() const;
  bool is_primary_cluster() const;

  void invalidate_cluster_set_metadata_cache();

  void set_cluster_set_remove_pending(bool flag) {
    m_cs_md_remove_pending = flag;
  }

  std::shared_ptr<Cluster_set_impl> check_and_get_cluster_set_for_cluster();

  bool is_cluster_set_remove_pending() const { return m_cs_md_remove_pending; }

  std::shared_ptr<Cluster_set_impl> get_cluster_set_object(
      bool print_warnings = false, bool check_status = false) const;

  std::optional<std::string> get_comm_stack() const;

  /**
   * Reset the password for the Cluster's replication account in use for the
   * ClusterSet replication channel
   *
   * @return A mysqlshdk::mysql::Auth_options with the username and the newly
   * generated password
   */
  // XXX removethis
  mysqlshdk::mysql::Auth_options refresh_clusterset_replication_user() const;

  void update_replication_allowed_host(const std::string &host);

  /*
   * Enable skip_slave_start and configures the managed replication channel if
   * the target cluster is a replica.
   */
  void configure_cluster_set_member(
      const std::shared_ptr<mysqlsh::dba::Instance> &target_instance) const;

  /**
   * Recreate the recovery account for each Cluster member
   *
   * Required when using the 'MySQL' communication stack to ensure each active
   * member of the Cluster will use its own recovery account instead of the one
   * created whenever a new member joined since all possible seeds had to be
   * updated to use that same account. This ensures distributed recovery is
   * possible at any circumstance.
   */
  void restore_recovery_account_all_members(bool reset_password = true) const;

  /**
   * Change the recovery user credentials of all Cluster members
   *
   * @param repl_account The authentication options of the recovery account
   */
  void change_recovery_credentials_all_members(
      const mysqlshdk::mysql::Auth_options &repl_account) const;

  /**
   * Create a local recovery account for the target instance
   *
   * Required when using the 'MySQL' communication stack. The account is
   * created locally only to the target instance and with binary logging
   * disabled to not introduce errant transactions
   */
  void create_local_replication_user(
      const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
      std::string_view auth_cert_subject,
      const Group_replication_options &gr_options,
      bool propagate_credentials_donors, bool dry_run = false);

  /**
   * Create all accounts present in the current members of the cluster to the
   * target cluster
   *
   * Required when using the 'MySQL' communication stack and when the recovery
   * accounts need certificates.
   */
  void create_replication_users_at_instance(
      const std::shared_ptr<mysqlsh::dba::Instance> &target_instance);

  void update_group_peers(const mysqlshdk::mysql::IInstance &target_instance,
                          const Group_replication_options &gr_options,
                          int cluster_member_count,
                          const std::string &self_address,
                          bool group_seeds_only = false);

  void check_instance_configuration(
      const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
      bool using_clone_recovery, checks::Check_type checks_type,
      bool already_member) const;

  void wait_instance_recovery(
      const mysqlshdk::mysql::IInstance &target_instance,
      const std::string &join_begin_time,
      Recovery_progress_style progress_style) const;

  int64_t prepare_clone_recovery(
      const mysqlshdk::mysql::IInstance &target_instance,
      bool force_clone = false) const;

  Member_recovery_method check_recovery_method(
      const mysqlshdk::mysql::IInstance &target_instance,
      Member_op_action op_action,
      Member_recovery_method selected_recovery_method, bool interactive) const;

 protected:
  void _set_option(const std::string &option,
                   const shcore::Value &value) override;

  void _set_instance_option(const std::string &instance_def,
                            const std::string &option,
                            const shcore::Value &value) override;

  // Used shell options
  void init();

 private:
  std::string get_replication_user_host() const;
  void verify_topology_type_change() const;

  std::weak_ptr<Cluster_set_impl> m_cluster_set;

  void handle_notification(const std::string &name,
                           const shcore::Object_bridge_ref &sender,
                           shcore::Value::Map_type_ref data) override;

  void find_real_cluster_set_primary(Cluster_set_impl *cs) const;

  [[nodiscard]] mysqlshdk::mysql::Lock_scoped get_lock(
      mysqlshdk::mysql::Lock_mode mode, std::chrono::seconds timeout = {});

  std::string m_group_name;
  mysqlshdk::gr::Topology_mode m_topology_type =
      mysqlshdk::gr::Topology_mode::NONE;

  Cluster_availability m_availability = Cluster_availability::ONLINE;

  mutable Cluster_set_member_metadata m_cs_md;
  // cluster was removed from clusterset but local MD is not caught up
  bool m_cs_md_remove_pending = false;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_CLUSTER_IMPL_H_
