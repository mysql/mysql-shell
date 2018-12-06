/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_MOD_DBA_REPLICASET_H_
#define MODULES_ADMINAPI_MOD_DBA_REPLICASET_H_

#define JSON_STANDARD_OUTPUT 0
#define JSON_STATUS_OUTPUT 1
#define JSON_TOPOLOGY_OUTPUT 2
#define JSON_RESCAN_OUTPUT 3

#include <set>
#include <string>
#include <vector>

#include "modules/adminapi/mod_dba_common.h"
#include "modules/adminapi/mod_dba_provisioning_interface.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/shell_options.h"

namespace mysqlsh {
namespace dba {
class MetadataStorage;
class Cluster;

#if DOXYGEN_CPP
/**
 * Represents a ReplicaSet
 */
#endif
class ReplicaSet : public std::enable_shared_from_this<ReplicaSet>,
                   public shcore::Cpp_object_bridge {
 public:
  using Instance_info = mysqlshdk::innodbcluster::Instance_info;

  ReplicaSet(const std::string &name, const std::string &topology_type,
             const std::string &group_name,
             std::shared_ptr<MetadataStorage> metadata_storage);
  virtual ~ReplicaSet();

  static std::set<std::string> _rejoin_instance_opts;

  std::string class_name() const override { return "ReplicaSet"; }
  std::string &append_descr(std::string &s_out, int indent = -1,
                            int quote_strings = 0) const override;
  void append_json(shcore::JSON_dumper &dumper) const override;
  bool operator==(const Object_bridge &other) const override;

  shcore::Value get_member(const std::string &prop) const override;

  void set_id(uint64_t id) { _id = id; }
  uint64_t get_id() const { return _id; }

  void set_name(std::string name) { _name = name; }
  const std::string &get_name() const { return _name; }

  void set_cluster(std::shared_ptr<Cluster> cluster) { _cluster = cluster; }
  std::shared_ptr<Cluster> get_cluster() const { return _cluster.lock(); }

  std::string get_topology_type() const { return _topology_type; }

  void set_topology_type(const std::string &topology_type) {
    _topology_type = topology_type;
  }

  const std::string &get_group_name() const { return _group_name; }

  void sanity_check() const;

  void set_group_name(const std::string &group_name);

  void add_instance_metadata(
      const mysqlshdk::db::Connection_options &instance_definition,
      const std::string &label = "");

  void remove_instance_metadata(
      const mysqlshdk::db::Connection_options &instance_def);

  void adopt_from_gr();

  std::vector<std::string> get_online_instances();

  std::vector<Instance_definition> get_instances_from_metadata();

  std::vector<Instance_info> get_instances();

  void execute_in_members(
      const std::vector<std::string> &states,
      const mysqlshdk::db::Connection_options &cnx_opt,
      const std::vector<std::string> &ignore_instances_vector,
      std::function<bool(std::shared_ptr<mysqlshdk::db::ISession> session)>
          functor,
      bool ignore_network_conn_errors = true);

  static char const *kTopologySinglePrimary;
  static char const *kTopologyMultiPrimary;

#if DOXYGEN_JS
  String getName();
  Undefined addInstance(InstanceDef instance, Dictionary options);
  Undefined rejoinInstance(IndtanceDef instance, Dictionary options);
  Undefined removeInstance(InstanceDef instance, Dictionary options);
  Undefined dissolve(Dictionary options);
  Undefined disable();
  Undefined rescan();
  String describe();
  String status();
  Undefined forceQuorumUsingPartitionOf(InstanceDef instance, String password);
  Undefined switchToSinglePrimaryMode(InstanceDef instance);
  Undefined switchToMultiPrimaryMode();
  Undefined setPrimaryInstance(InstanceDef instance);
  Undefined setInstanceOption(InstanceDef instance, String option,
                              String value);

#elif DOXYGEN_PY
  str get_name();
  None add_instance(InstanceDef instance, dict options);
  None rejoin_instance(InstanceDef instance, dict options);
  None remove_instance(InstanceDef instance, dict options);
  None dissolve(Dictionary options);
  None disable();
  None rescan();
  str describe();
  str status();
  None force_quorum_using_partition_of(InstanceDef instance, str password);
  None switch_to_single_primary_mode(InstanceDef instance);
  None switch_to_multi_primary_mode();
  None set_primary_instance(InstanceDef instance);
  None set_instance_option(InstanceDef instance, str option, str value);
#endif

  shcore::Value add_instance_(const shcore::Argument_list &args);
  shcore::Value add_instance(
      const mysqlshdk::db::Connection_options &connection_options,
      const shcore::Argument_list &args,
      const std::string &existing_replication_user = "",
      const std::string &existing_replication_password = "",
      bool overwrite_seed = false, const std::string &group_name = "",
      bool skip_instance_check = false, bool skip_rpl_user = false);

  shcore::Value check_instance_state(const Connection_options &instance_def);
  shcore::Value rejoin_instance_(const shcore::Argument_list &args);
  shcore::Value rejoin_instance(mysqlshdk::db::Connection_options *instance_def,
                                const shcore::Value::Map_type_ref &options);
  shcore::Value remove_instance(const shcore::Argument_list &args);
  shcore::Value dissolve(const shcore::Argument_list &args);
  void rescan(const shcore::Dictionary_t &options);
  shcore::Value force_quorum_using_partition_of(
      const shcore::Argument_list &args);
  shcore::Value force_quorum_using_partition_of_(
      const shcore::Argument_list &args);
  shcore::Value get_status(const mysqlsh::dba::Cluster_check_info &state) const;

  Cluster_check_info check_preconditions(
      std::shared_ptr<mysqlshdk::db::ISession> group_session,
      const std::string &function_name) const;
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
   * replicaset and remove the replication user on the target instance and
   * other members (if remove_rpl_user_on_group = true).
   *
   * @param local_gr_address string with the local GR address (XCom) to remove.
   * @param instance target instance that was removed from the replicaset.
   * @param remove_rpl_user_on_group boolean indicating if the replication
   *        (recovery) user used by the instance should be removed on the
   *        remaining members of the GR group (replicaset). If true then remove
   *        the recovery user used by the instance on the other members through
   *        a primary instance, otherwise skip it (just remove replication users
   *        on the target instance).
   */
  void update_group_members_for_removed_member(
      const std::string &local_gr_address,
      const mysqlshdk::mysql::Instance &instance,
      bool remove_rpl_user_on_group);

  /**
   * Remove the replication (recovery) users from the given instance.
   *
   * @param instance target Instance object to remove the replication users.
   * @param remove_rpl_user_on_group boolean indicating if the replication
   *        (recovery) user used by the instance should be removed on the
   *        remaining members of the GR group (replicaset). If true then remove
   *        the recovery user used by the instance on the other members through
   *        a primary instance, otherwise skip it (just remove replication users
   *        on the target instance).
   */
  void remove_replication_users(const mysqlshdk::mysql::Instance &instance,
                                bool remove_rpl_user_on_group);

  /**
   * Get the primary instance address.
   *
   * This funtion retrieves the address of a primary instance in the
   * replicaset. In single primary mode, only one instance is the primary and
   * its address is returned, otherwise it is assumed that multi primary mode
   * is used. In multi primary mode, the address of any available instance
   * is returned.
   */
  mysqlshdk::db::Connection_options pick_seed_instance();

 private:
  // TODO(miguel) these should go to a GroupReplication file
  friend Cluster;

  shcore::Value get_description() const;
  void verify_topology_type_change() const;

 protected:
  uint64_t _id;
  std::string _name;
  std::string _topology_type;
  std::string _group_name;
  // TODO(miguel): add missing fields, rs_type, etc

 private:
  bool do_join_replicaset(
      const mysqlshdk::db::Connection_options &instance,
      mysqlshdk::db::Connection_options *peer, const std::string &repl_user,
      const std::string &repl_user_password, const std::string &ssl_mode,
      const std::string &ip_whitelist,
      mysqlshdk::utils::nullable<int64_t> member_weight,
      mysqlshdk::utils::nullable<int64_t> expel_timeout,
      const std::string &group_name = "", const std::string &local_address = "",
      const std::string &group_seeds = "",
      const std::string &exit_state_action = "",
      const std::string &failover_consistency = "", bool skip_rpl_user = false);

  void validate_instance_address(
      std::shared_ptr<mysqlshdk::db::ISession> session,
      const std::string &hostname, int port);
  void validate_server_uuid(
      std::shared_ptr<mysqlshdk::db::ISession> instance_session);

  std::string get_cluster_group_seeds(
      std::shared_ptr<mysqlshdk::db::ISession> instance_session = nullptr);

  std::weak_ptr<Cluster> _cluster;

  std::shared_ptr<MetadataStorage> _metadata_storage;
  std::shared_ptr<ProvisioningInterface> _provisioning_interface;
};
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_REPLICASET_H_
