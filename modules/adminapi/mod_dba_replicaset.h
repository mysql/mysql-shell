/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <string>
#include <set>
#include <vector>

#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "modules/adminapi/mod_dba_provisioning_interface.h"
#include "modules/adminapi/mod_dba_common.h"
#include "mysqlshdk/libs/db/connection_options.h"

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
  ReplicaSet(const std::string &name, const std::string &topology_type,
             const std::string &group_name,
             std::shared_ptr<MetadataStorage> metadata_storage);
  virtual ~ReplicaSet();

  static std::set<std::string> _add_instance_opts;
  static std::set<std::string> _remove_instance_opts;

  virtual std::string class_name() const { return "ReplicaSet"; }
  virtual std::string &append_descr(std::string &s_out, int indent = -1,
                                    int quote_strings = 0) const;
  virtual bool operator == (const Object_bridge &other) const;

  virtual shcore::Value get_member(const std::string &prop) const;

  void set_id(uint64_t id) { _id = id; }
  uint64_t get_id() { return _id; }

  void set_name(std::string name) { _name = name; }
  const std::string &get_name() const { return _name; }

  void set_cluster(std::shared_ptr<Cluster> cluster) { _cluster = cluster; }
  std::shared_ptr<Cluster> get_cluster() const { return _cluster.lock(); }

  std::string get_topology_type() const { return _topology_type; }

  const std::string &get_group_name() const {
    return _group_name;
  }

  void set_group_name(const std::string &group_name);

  void add_instance_metadata(
      const mysqlshdk::db::Connection_options &instance_definition,
      const std::string &label = "");

  void remove_instance_metadata(
      const mysqlshdk::db::Connection_options &instance_def);

  void adopt_from_gr();

  std::vector<std::string> get_online_instances();

  std::vector<Instance_definition> get_instances_from_metadata();

  static char const *kTopologyPrimaryMaster;
  static char const *kTopologyMultiMaster;

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
#endif

  shcore::Value add_instance_(const shcore::Argument_list &args);
  shcore::Value add_instance(
      const mysqlshdk::db::Connection_options &connection_options,
      const shcore::Argument_list &args,
      const std::string &existing_replication_user = "",
      const std::string &existing_replication_password = "",
      bool overwrite_seed = false, const std::string &group_name = "");

  shcore::Value check_instance_state(const shcore::Argument_list &args);
  shcore::Value rejoin_instance_(const shcore::Argument_list &args);
  shcore::Value rejoin_instance
    (mysqlshdk::db::Connection_options *instance_def,
     const shcore::Value::Map_type_ref &options);
  shcore::Value remove_instance_(const shcore::Argument_list &args);
  shcore::Value remove_instance(const shcore::Argument_list &args);
  shcore::Value dissolve(const shcore::Argument_list &args);
  shcore::Value disable(const shcore::Argument_list &args);
  shcore::Value retrieve_instance_state(const shcore::Argument_list &args);
  shcore::Value rescan(const shcore::Argument_list &args);
  shcore::Value force_quorum_using_partition_of(
      const shcore::Argument_list &args);
  shcore::Value force_quorum_using_partition_of_(
      const shcore::Argument_list &args);
  shcore::Value get_status(
      const mysqlsh::dba::ReplicationGroupState &state) const;

  void remove_instances_from_gr(
      const std::vector<Instance_definition> &instances);
  void remove_instance_from_gr(const std::string &instance_str,
                               const mysqlshdk::db::Connection_options &data);
  ReplicationGroupState check_preconditions(
      std::shared_ptr<mysqlshdk::db::ISession> group_session,
      const std::string& function_name) const;
  void remove_instances(const std::vector<std::string> &remove_instances);
  void rejoin_instances(const std::vector<std::string> &rejoin_instances,
                        const shcore::Value::Map_type_ref &options);

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
  void init();

  bool do_join_replicaset(const mysqlshdk::db::Connection_options &instance,
                          mysqlshdk::db::Connection_options *peer,
                          const std::string &super_user_password,
                          const std::string &repl_user,
                          const std::string &repl_user_password,
                          const std::string &ssl_mode,
                          const std::string &ip_whitelist,
                          const std::string &group_name = "",
                          const std::string &local_address = "",
                          const std::string &group_seeds = "");

  std::string get_peer_instance();

  void validate_instance_address(
      std::shared_ptr<mysqlshdk::db::ISession> session,
      const std::string &hostname, int port);

  shcore::Value::Map_type_ref _rescan(const shcore::Argument_list &args);
  std::string get_cluster_group_seeds(
      std::shared_ptr<mysqlshdk::db::ISession> instance_session);

  std::weak_ptr<Cluster> _cluster;
  std::shared_ptr<MetadataStorage> _metadata_storage;
  std::shared_ptr<ProvisioningInterface> _provisioning_interface;
  std::shared_ptr<mysqlshdk::db::ISession> get_session(
    const mysqlshdk::db::Connection_options &args);
};
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_REPLICASET_H_
