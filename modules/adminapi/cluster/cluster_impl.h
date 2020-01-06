/*
 * Copyright (c) 2016, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/shell_options.h"

#include "modules/adminapi/cluster/base_cluster_impl.h"
#include "modules/adminapi/cluster/replicaset/replicaset.h"
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

// Timestamp of when the instance was added to the group
constexpr const char k_instance_attribute_join_time[] = "joinTime";

class MetadataStorage;
struct Cluster_metadata;

class Cluster_impl : public Base_cluster_impl {
 public:
  friend class Cluster;

  Cluster_impl(const Cluster_metadata &metadata,
               const std::shared_ptr<Instance> &group_server,
               const std::shared_ptr<MetadataStorage> &metadata_storage);

  Cluster_impl(const std::string &cluster_name, const std::string &group_name,
               const std::shared_ptr<Instance> &group_server,
               const std::shared_ptr<MetadataStorage> &metadata_storage);
  virtual ~Cluster_impl();

  std::shared_ptr<GRReplicaSet> get_default_replicaset() const {
    return _default_replica_set;
  }

  std::shared_ptr<GRReplicaSet> create_default_replicaset(
      const std::string &name, bool multi_primary);

  bool get_disable_clone_option() const;
  void set_disable_clone_option(const bool disable_clone);

  const std::string &get_group_name() const { return m_group_name; }

  Cluster_type get_type() const override {
    return Cluster_type::GROUP_REPLICATION;
  }

  std::string get_topology_type() const override {
    return get_default_replicaset()->get_topology_type();
  }

  mysqlshdk::mysql::Auth_options create_replication_user(
      mysqlshdk::mysql::IInstance *target);

  void drop_replication_user(mysqlshdk::mysql::IInstance *target);

  bool contains_instance_with_address(const std::string &host_port);

  std::list<Scoped_instance> connect_all_members(
      uint32_t, bool, std::list<Instance_metadata> *) override {
    throw std::logic_error("not implemented");
  }

  mysqlsh::dba::Instance *acquire_primary(
      mysqlshdk::mysql::Lock_mode mode = mysqlshdk::mysql::Lock_mode::NONE,
      const std::string &skip_lock_uuid = "") override;

  void release_primary(mysqlsh::dba::Instance *primary = nullptr) override;

 public:
  // TODO(alfredo) - whoever refactors these methods, should move them to
  // the API Cluster class. Equivalent methods should also exist here,
  // but for internal use (and thus, no options in Dictionary_t,
  // no shcore::Values etc). Parameter syntax validations should also be assumed
  // to have already been done.
  shcore::Value describe();
  shcore::Value options(const shcore::Dictionary_t &options);
  shcore::Value status(uint64_t extended);
  shcore::Value list_routers(bool only_upgrade_required) override;
  void remove_instance(const Connection_options &instance_def,
                       const shcore::Dictionary_t &options = {});
  void add_instance(const Connection_options &instance_def,
                    const shcore::Dictionary_t &options);
  void rejoin_instance(const Connection_options &instance_def,
                       const shcore::Dictionary_t &options);

  void set_option(const std::string &option, const shcore::Value &value);

  Cluster_check_info check_preconditions(
      const std::string &function_name) const override;

 protected:
  std::string m_group_name;
  std::shared_ptr<GRReplicaSet> _default_replica_set;

  // Used shell options
  void init();

 public:
  void force_quorum_using_partition_of(const Connection_options &instance_def);

  void dissolve(const shcore::Dictionary_t &options);

  void rescan(const shcore::Dictionary_t &options);

  void switch_to_single_primary_mode(const Connection_options &instance_def);
  void switch_to_multi_primary_mode();
  void set_primary_instance(const Connection_options &instance_def);
  void set_instance_option(const Connection_options &instance_def,
                           const std::string &option,
                           const shcore::Value &value);
  shcore::Value check_instance_state(const Connection_options &instance_def);
  void reset_recovery_password(const shcore::Dictionary_t &options);
  void setup_admin_account(
      const std::string &username, const std::string &host, bool interactive,
      bool update, bool dry_run,
      const mysqlshdk::utils::nullable<std::string> &password) override;
  void setup_router_account(
      const std::string &username, const std::string &host, bool interactive,
      bool update, bool dry_run,
      const mysqlshdk::utils::nullable<std::string> &password) override;

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
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_CLUSTER_IMPL_H_
