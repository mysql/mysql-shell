/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_CLUSTER_REPLICASET_REPLICASET_H_
#define MODULES_ADMINAPI_CLUSTER_REPLICASET_REPLICASET_H_

#define JSON_STANDARD_OUTPUT 0
#define JSON_STATUS_OUTPUT 1
#define JSON_TOPOLOGY_OUTPUT 2
#define JSON_RESCAN_OUTPUT 3

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/adminapi/common/provisioning_interface.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/shell_options.h"

namespace mysqlsh {
namespace dba {
struct Instance_metadata;
class Cluster_impl;

// TODO(rennox): there are some functions that are similar to Replicaset
// and to AsyncReplicaSets but it requires that this Replicaset is refactored
// to inherit from Base_cluster_impl. This refactoring should be considered.
class GRReplicaSet {
 public:
  GRReplicaSet(const std::string &name, const std::string &topology_type);
  virtual ~GRReplicaSet();

  void set_name(const std::string &name) { _name = name; }
  const std::string &get_name() const { return _name; }

  void set_cluster(Cluster_impl *cluster) { m_cluster = cluster; }

  /**
   * Get the cluster parent object.
   *
   * @return shared_ptr to Cluster parent object.
   */
  Cluster_impl *get_cluster() const { return m_cluster; }

  std::string get_topology_type() const { return _topology_type; }

  void set_topology_type(const std::string &topology_type) {
    _topology_type = topology_type;
  }

  void sanity_check() const;

  void add_instance_metadata(
      const mysqlshdk::db::Connection_options &instance_definition,
      const std::string &label = "") const;

  void add_instance_metadata(const mysqlshdk::mysql::IInstance &instance,
                             const std::string &label = "") const;

  void remove_instance_metadata(
      const mysqlshdk::db::Connection_options &instance_def);

  void adopt_from_gr();

  /**
   * Get ONLINE and RECOVERING instances from the cluster.
   *
   * @return vector with the Instance definition of the ONLINE
   * and RECOVERING instances.
   */
  std::vector<Instance_metadata> get_active_instances() const;

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

  std::vector<Instance_metadata> get_instances() const;

  /**
   * Return list of instances registered in metadata along with their current
   * GR member state.
   */
  std::vector<std::pair<Instance_metadata, mysqlshdk::gr::Member>>
  get_instances_with_state() const;

  std::unique_ptr<mysqlshdk::config::Config> create_config_object(
      std::vector<std::string> ignored_instances = {},
      bool skip_invalid_state = false) const;

  void execute_in_members(
      const std::vector<mysqlshdk::gr::Member_state> &states,
      const mysqlshdk::db::Connection_options &cnx_opt,
      const std::vector<std::string> &ignore_instances_vector,
      const std::function<bool(const std::shared_ptr<Instance> &instance)>
          &functor,
      bool ignore_network_conn_errors = true) const;

  static char const *kTopologySinglePrimary;
  static char const *kTopologyMultiPrimary;
  static const char *kWarningDeprecateSslMode;

  void add_instance(const mysqlshdk::db::Connection_options &connection_options,
                    const shcore::Dictionary_t &options);
  shcore::Value check_instance_state(const Connection_options &instance_def);

  shcore::Value rejoin_instance(mysqlshdk::db::Connection_options *instance_def,
                                const shcore::Value::Map_type_ref &options);
  shcore::Value remove_instance(const shcore::Argument_list &args);
  void dissolve(const shcore::Dictionary_t &options);
  void rescan(const shcore::Dictionary_t &options);
  shcore::Value force_quorum_using_partition_of(
      const shcore::Argument_list &args);

  void remove_instances(const std::vector<std::string> &remove_instances);
  void rejoin_instances(const std::vector<std::string> &rejoin_instances,
                        const shcore::Value::Map_type_ref &options);
  void set_instance_option(const Connection_options &instance_def,
                           const std::string &option,
                           const shcore::Value &value);

  void switch_to_single_primary_mode(const Connection_options &instance_def);
  void switch_to_multi_primary_mode();
  void set_primary_instance(const Connection_options &instance_def);

  /**
   * Update the replicaset members according to the removed instance.
   *
   * More specifically, remove the specified local address from the
   * group_replication_group_seeds variable of all alive members of the
   * replicaset and then remove the recovery user used by the instance on the
   * other members through a primary instance.
   *
   * @param local_gr_address string with the local GR address (XCom) to remove.
   * @param instance target instance that was removed from the replicaset.
   */
  void update_group_members_for_removed_member(
      const std::string &local_gr_address,
      const mysqlsh::dba::Instance &instance);

  /**
   * Get the primary instance address.
   *
   * This funtion retrieves the address of a primary instance in the
   * replicaset. In single primary mode, only one instance is the primary and
   * its address is returned, otherwise it is assumed that multi primary mode
   * is used. In multi primary mode, the address of any available instance
   * is returned.
   */
  mysqlshdk::db::Connection_options pick_seed_instance() const;

  void validate_server_uuid(const mysqlshdk::mysql::IInstance &instance) const;

  void validate_server_id(
      const mysqlshdk::mysql::IInstance &target_instance) const;

  std::string get_cluster_group_seeds(
      const std::shared_ptr<Instance> &target_instance = nullptr) const;

  void query_group_wide_option_values(
      mysqlshdk::mysql::IInstance *target_instance,
      mysqlshdk::utils::nullable<std::string> *out_gr_consistency,
      mysqlshdk::utils::nullable<int64_t> *out_gr_member_expel_timeout) const;

 private:
  friend Cluster_impl;

  void verify_topology_type_change() const;

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
  std::string _name;
  std::string _topology_type;

 private:
  Cluster_impl *m_cluster;
};
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_REPLICASET_REPLICASET_H_
