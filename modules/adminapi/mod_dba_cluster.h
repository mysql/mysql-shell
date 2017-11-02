/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef MODULES_ADMINAPI_MOD_DBA_CLUSTER_H_
#define MODULES_ADMINAPI_MOD_DBA_CLUSTER_H_

#include <string>

#include "scripting/types.h"
#include "scripting/types_cpp.h"

#include "modules/adminapi/mod_dba_replicaset.h"
#include "modules/adminapi/mod_dba_common.h"
#include "mysqlshdk/libs/db/connection_options.h"

#define ACC_USER "username"
#define ACC_PASSWORD "password"

#define ATT_DEFAULT "default"

namespace mysqlsh {
namespace dba {
class MetadataStorage;

/**
 * \ingroup AdminAPI
 * $(CLUSTER_BRIEF)
 */
class Cluster : public std::enable_shared_from_this<Cluster>,
                public shcore::Cpp_object_bridge {
 public:
  Cluster(const std::string &name,
          std::shared_ptr<MetadataStorage> metadata_storage);
  virtual ~Cluster();

  virtual std::string class_name() const { return "Cluster"; }
  virtual std::string &append_descr(std::string &s_out, int indent = -1,
                                    int quote_strings = 0) const;
  virtual bool operator == (const Object_bridge &other) const;

  virtual shcore::Value get_member(const std::string &prop) const;

  uint64_t get_id() const { return _id; }
  void set_id(uint64_t id) { _id = id; }
  std::shared_ptr<mysqlshdk::db::ISession> get_session() {return _session; }
  std::shared_ptr<ReplicaSet> get_default_replicaset() {
      return _default_replica_set;
  }
  void set_default_replicaset(std::shared_ptr<ReplicaSet> default_rs);
  std::string get_name() { return _name; }
  std::string get_description() { return _description; }
  void assert_not_dissolved(const std::string &option_name) const;
  void set_description(std::string description) { _description = description; }

  void set_option(const std::string& option, const shcore::Value &value);
  void set_options(const std::string& json) {
    _options = shcore::Value::parse(json).as_map();
  }
  std::string get_options() { return shcore::Value(_options).json(false); }

  void set_attribute(const std::string& attribute, const shcore::Value &value);
  void set_attributes(const std::string& json) {
    _attributes = shcore::Value::parse(json).as_map();
  }
  std::string get_attributes() {
    return shcore::Value(_attributes).json(false);
  }

  std::shared_ptr<ProvisioningInterface> get_provisioning_interface() {
    return _provisioning_interface;
  }

  void set_provisioning_interface(
      std::shared_ptr<ProvisioningInterface> provisioning_interface) {
    _provisioning_interface = provisioning_interface;
  }

  shcore::Value add_seed_instance(
      const mysqlshdk::db::Connection_options &connection_options,
      const shcore::Argument_list &args, bool multi_master, bool is_adopted,
      const std::string &replication_user, const std::string &replication_pwd,
      const std::string &group_name);

 public:
  shcore::Value add_instance(const shcore::Argument_list &args);
  shcore::Value rejoin_instance(const shcore::Argument_list &args);
  shcore::Value remove_instance(const shcore::Argument_list &args);
  shcore::Value get_replicaset(const shcore::Argument_list &args);
  shcore::Value describe(const shcore::Argument_list &args);
  shcore::Value status(const shcore::Argument_list &args);
  shcore::Value dissolve(const shcore::Argument_list &args);
  shcore::Value check_instance_state(const shcore::Argument_list &args);
  shcore::Value rescan(const shcore::Argument_list &args);
  shcore::Value force_quorum_using_partition_of(
      const shcore::Argument_list &args);

  ReplicationGroupState check_preconditions(
      const std::string& function_name) const;

#if DOXYGEN_JS
  String name;  //!< $(CLUSTER_NAME_BRIEF)
  String getName();
  String getAdminType();
  Undefined addInstance(InstanceDef instance, Dictionary options);
  Undefined rejoinInstance(InstanceDef instance, Dictionary options);
  Undefined removeInstance(InstanceDef instance, Dictionary options);
  Dictionary checkInstanceState(InstanceDef instance, String password);
  String describe();
  String status();
  Undefined dissolve(Dictionary options);
  Undefined rescan();
  Undefined forceQuorumUsingPartitionOf(InstanceDef instance, String password);
#elif DOXYGEN_PY
  str name;  //!< $(CLUSTER_NAME_BRIEF)
  str get_name();
  None add_instance(InstanceDef instance, dict options);
  None rejoin_instance(InstanceDef instance, dict options);
  None remove_instance(InstanceDef instance, dict options);
  dict check_instance_state(InstanceDef instance, str password);
  str describe();
  str status();
  None dissolve(Dictionary options);
  None rescan();
  None force_quorum_using_partition_of(InstanceDef instance, str password);
#endif

 protected:
  Cluster() : _dissolved(false) { init(); }  // To support basic mocking

  uint64_t _id;
  std::string _name;
  std::shared_ptr<ReplicaSet> _default_replica_set;
  std::string _description;
  shcore::Value::Map_type_ref _accounts;
  shcore::Value::Map_type_ref _options;
  shcore::Value::Map_type_ref _attributes;
  bool _dissolved;
  std::shared_ptr<mysqlshdk::db::ISession> _session;
  std::shared_ptr<MetadataStorage> _metadata_storage;
  void init();

 private:
  std::shared_ptr<ProvisioningInterface> _provisioning_interface;

  void set_account_data(const std::string &account, const std::string &key,
                        const std::string &value);
  std::string get_account_data(const std::string &account,
                               const std::string &key);
  shcore::Value::Map_type_ref _rescan(const shcore::Argument_list &args);
};
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_CLUSTER_H_
