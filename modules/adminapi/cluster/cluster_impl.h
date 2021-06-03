/*
 * Copyright (c) 2016, 2021, Oracle and/or its affiliates.
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
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "mysql/group_replication.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/shell_options.h"

#include "modules/adminapi/cluster/api_options.h"
#include "modules/adminapi/common/base_cluster_impl.h"
#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"

namespace mysqlsh {
namespace dba {

// User provided option to disabling clone
constexpr const char k_cluster_attribute_disable_clone[] = "opt_disableClone";
// Flag to indicate the default cluster in the metadata
constexpr const char k_cluster_attribute_default[] = "default";

// Whether group_replication_start_on_boot should be enabled in each instance
// (false by default)
constexpr const char k_cluster_attribute_manual_start_on_boot[] =
    "opt_manualStartOnBoot";

// Timestamp of when the instance was added to the group
constexpr const char k_instance_attribute_join_time[] = "joinTime";

constexpr const char k_instance_attribute_server_id[] = "server_id";

class MetadataStorage;
struct Cluster_metadata;

using Instance_md_and_gr_member =
    std::pair<Instance_metadata, mysqlshdk::gr::Member>;

class Cluster_impl : public Base_cluster_impl {
 public:
  friend class Cluster;

  Cluster_impl(const Cluster_metadata &metadata,
               const std::shared_ptr<Instance> &group_server,
               const std::shared_ptr<MetadataStorage> &metadata_storage);

  Cluster_impl(const std::string &cluster_name, const std::string &group_name,
               const std::shared_ptr<Instance> &group_server,
               const std::shared_ptr<MetadataStorage> &metadata_storage,
               mysqlshdk::gr::Topology_mode topology_type);
  virtual ~Cluster_impl();

  // API functions
  void add_instance(const Connection_options &instance_def,
                    const cluster::Add_instance_options &options,
                    Recovery_progress_style progress_style);

  void rejoin_instance(const Connection_options &instance_def,
                       const Group_replication_options &gr_options,
                       bool interactive, bool skip_precondition_check);

  void remove_instance(const Connection_options &instance_def,
                       const mysqlshdk::null_bool &force,
                       const bool interactive);
  shcore::Value describe();
  shcore::Value options(const bool all);
  shcore::Value status(uint64_t extended);
  shcore::Value list_routers(bool only_upgrade_required) override;
  void force_quorum_using_partition_of(const Connection_options &instance_def,
                                       const bool interactive);
  void dissolve(const mysqlshdk::null_bool &force, const bool interactive);
  void rescan(const cluster::Rescan_options &options);
  void switch_to_single_primary_mode(const Connection_options &instance_def);
  void switch_to_multi_primary_mode();
  void set_primary_instance(const Connection_options &instance_def);
  shcore::Value check_instance_state(const Connection_options &instance_def);
  void reset_recovery_password(const mysqlshdk::null_bool &force,
                               const bool interactive);

  // Class functions
  void sanity_check() const;

  void add_metadata_for_instance(
      const mysqlshdk::db::Connection_options &instance_definition,
      const std::string &label = "") const;

  void add_metadata_for_instance(const mysqlshdk::mysql::IInstance &instance,
                                 const std::string &label = "") const;

  void remove_instance_metadata(
      const mysqlshdk::db::Connection_options &instance_def);

  void update_metadata_for_instance(
      const mysqlshdk::db::Connection_options &instance_definition) const;

  void update_metadata_for_instance(
      const mysqlshdk::mysql::IInstance &instance) const;

  bool get_disable_clone_option() const;
  void set_disable_clone_option(const bool disable_clone);

  bool get_manual_start_on_boot_option() const;

  const std::string &get_group_name() const { return m_group_name; }

  Cluster_type get_type() const override {
    return Cluster_type::GROUP_REPLICATION;
  }

  std::string get_topology_type() const override {
    return to_string(m_topology_type);
  }

  mysqlshdk::gr::Topology_mode get_cluster_topology_type() const {
    return m_topology_type;
  }

  void set_topology_type(const mysqlshdk::gr::Topology_mode &topology_type) {
    m_topology_type = topology_type;
  }

  mysqlshdk::mysql::Auth_options create_replication_user(
      mysqlshdk::mysql::IInstance *target);

  void drop_replication_user(mysqlshdk::mysql::IInstance *target);

  bool contains_instance_with_address(const std::string &host_port) const;

  std::list<Scoped_instance> connect_all_members(
      uint32_t, bool, std::list<Instance_metadata> *) override {
    throw std::logic_error("not implemented");
  }

  mysqlsh::dba::Instance *acquire_primary(
      mysqlshdk::mysql::Lock_mode mode = mysqlshdk::mysql::Lock_mode::NONE,
      const std::string &skip_lock_uuid = "") override;

  void release_primary(mysqlsh::dba::Instance *primary = nullptr) override;

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
   * @return Version object of the lowest instance version in the cluster.
   */
  mysqlshdk::utils::Version get_lowest_instance_version() const;

  /**
   * Determine the replication(recovery) user being used by the target instance.
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
      const mysqlshdk::mysql::IInstance &target_instance) const;

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
  size_t setup_clone_plugin(bool enable_clone);

  void execute_in_members(
      const std::vector<mysqlshdk::gr::Member_state> &states,
      const mysqlshdk::db::Connection_options &cnx_opt,
      const std::vector<std::string> &ignore_instances_vector,
      const std::function<bool(const std::shared_ptr<Instance> &instance)>
          &functor,
      const std::function<bool(const Instance_md_and_gr_member &md)>
          &condition = nullptr,
      bool ignore_network_conn_errors = true) const;

  /**
   * Get the primary instance address.
   *
   * This funtion retrieves the address of a primary instance in the
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

  std::string get_cluster_group_seeds(
      const std::shared_ptr<Instance> &target_instance = nullptr) const;

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
   * @param states Vector of strings with the states of members whose
   * instance_metadata we will retrieve from the cluster. If the states vector
   * is empty, return the list of all instances.
   *
   * @return vector with the Instance definitions
   */
  std::vector<Instance_metadata> get_instances(
      const std::vector<mysqlshdk::gr::Member_state> &states = {}) const;

  /**
   * Ensures the server_id of each instance is registered as an attribute on the
   * instances table. If an instance does not have it then inserts it.
   *
   * @param target_instance: The instance to be used to get the cluster members
   * to update the server_id attribute on the instances table.
   */
  void ensure_metadata_has_server_id(
      const mysqlshdk::mysql::IInstance &target_instance);

  /**
   * Checks for missing metadata recovery account entries, and fix them using
   * info from mysql.slave_master_info.
   *
   */
  void ensure_metadata_has_recovery_accounts();

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

  void validate_server_uuid(const mysqlshdk::mysql::IInstance &instance) const;

  void validate_server_id(
      const mysqlshdk::mysql::IInstance &target_instance) const;

  /**
   * Return list of instances registered in metadata along with their current
   * GR member state.
   */
  std::vector<Instance_md_and_gr_member> get_instances_with_state() const;

  std::unique_ptr<mysqlshdk::config::Config> create_config_object(
      const std::vector<std::string> &ignored_instances = {},
      bool skip_invalid_state = false, bool persist_only = false) const;

  void query_group_wide_option_values(
      mysqlshdk::mysql::IInstance *target_instance,
      mysqlshdk::utils::nullable<std::string> *out_gr_consistency,
      mysqlshdk::utils::nullable<int64_t> *out_gr_member_expel_timeout) const;

  /**
   * Update the cluster members according to the removed instance.
   *
   * More specifically, remove the specified local address from the
   * group_replication_group_seeds variable of all alive members of the
   * cluster and then remove the recovery user used by the instance on the
   * other members through a primary instance.
   *
   * @param local_gr_address string with the local GR address (XCom) to remove.
   */
  void update_group_members_for_removed_member(
      const std::string &local_gr_address);

  void adopt_from_gr();

  /**
   * Validate the GTID consistency in regard to the cluster for an instance
   * rejoining
   *
   * This function checks if the target instance has errant transactions, or if
   * the binary logs have been purged from all cluster members to block the
   * rejoin. It also checks if the GTID set is empty which, for an instance that
   * is registered as a cluster member should not happen, to block its rejoin
   * too.
   *
   * @param target_instance Target instance rejoining the cluster
   */
  void validate_rejoin_gtid_consistency(
      const mysqlshdk::mysql::IInstance &target_instance);

 protected:
  void _set_option(const std::string &option,
                   const shcore::Value &value) override;

  void _set_instance_option(const std::string &instance_def,
                            const std::string &option,
                            const shcore::Value &value) override;

  // Used shell options
  void init();

 private:
  void verify_topology_type_change() const;

  std::string m_group_name;
  mysqlshdk::gr::Topology_mode m_topology_type =
      mysqlshdk::gr::Topology_mode::NONE;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_CLUSTER_IMPL_H_
