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

#ifndef MODULES_ADMINAPI_MOD_DBA_CLUSTER_H_
#define MODULES_ADMINAPI_MOD_DBA_CLUSTER_H_

#include <memory>
#include <string>

#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/shell_options.h"

#include "modules/adminapi/mod_dba_common.h"
#include "modules/adminapi/mod_dba_replicaset.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/innodbcluster/cluster_metadata.h"

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
#if DOXYGEN_JS
  String name;  //!< $(CLUSTER_GETNAME_BRIEF)
  Undefined addInstance(InstanceDef instance, Dictionary options);
  Dictionary checkInstanceState(InstanceDef instance, String password);
  String describe();
  Undefined disconnect();
  Undefined dissolve(Dictionary options);
  Undefined forceQuorumUsingPartitionOf(InstanceDef instance, String password);
  String getName();
  Undefined rejoinInstance(InstanceDef instance, Dictionary options);
  Undefined removeInstance(InstanceDef instance, Dictionary options);
  Undefined rescan(Dictionary options);
  String status(Dictionary options);
  Undefined switchToSinglePrimaryMode(InstanceDef instance);
  Undefined switchToMultiPrimaryMode();
  Undefined setPrimaryInstance(InstanceDef instance);
#elif DOXYGEN_PY
  str name;  //!< $(CLUSTER_GETNAME_BRIEF)
  None add_instance(InstanceDef instance, dict options);
  dict check_instance_state(InstanceDef instance, str password);
  str describe();
  None disconnect();
  None dissolve(dict options);
  None force_quorum_using_partition_of(InstanceDef instance, str password);
  str get_name();
  None rejoin_instance(InstanceDef instance, dict options);
  None remove_instance(InstanceDef instance, dict options);
  None rescan(dict options);
  str status(dict options);
  None switch_to_single_primary_mode(InstanceDef instance);
  None switch_to_multi_primary_mode();
  None set_primary_instance(InstanceDef instance);
#endif

  Cluster(const std::string &name,
          std::shared_ptr<mysqlshdk::db::ISession> group_session,
          std::shared_ptr<MetadataStorage> metadata_storage);
  virtual ~Cluster();

  std::string class_name() const override { return "Cluster"; }
  std::string &append_descr(std::string &s_out, int indent = -1,
                            int quote_strings = 0) const override;
  void append_json(shcore::JSON_dumper &dumper) const override;
  bool operator==(const Object_bridge &other) const override;

  shcore::Value call(const std::string &name,
                     const shcore::Argument_list &args) override;
  shcore::Value get_member(const std::string &prop) const override;

  uint64_t get_id() const { return _id; }
  void set_id(uint64_t id) { _id = id; }

  std::shared_ptr<ReplicaSet> get_default_replicaset() {
    return _default_replica_set;
  }

  void set_default_replicaset(const std::string &name,
                              const std::string &topology_type,
                              const std::string &group_name);

  std::shared_ptr<ReplicaSet> create_default_replicaset(
      const std::string &name, bool multi_primary,
      const std::string &group_name, bool is_adopted);

  std::string get_name() { return _name; }
  std::string get_description() { return _description; }
  void assert_valid(const std::string &option_name) const;
  void set_description(std::string description) { _description = description; }

  void set_name(const std::string &name) { _name = name; }

  void set_option(const std::string &option, const shcore::Value &value);
  void set_options(const std::string &json) {
    _options = shcore::Value::parse(json).as_map();
  }
  std::string get_options() { return shcore::Value(_options).json(false); }

  void set_attribute(const std::string &attribute, const shcore::Value &value);
  void set_attributes(const std::string &json) {
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
      const shcore::Argument_list &args, bool multi_primary, bool is_adopted,
      const std::string &replication_user, const std::string &replication_pwd,
      const std::string &group_name);

  std::shared_ptr<mysqlshdk::db::ISession> get_group_session() {
    return _group_session;
  }

 public:
  shcore::Value add_instance(const shcore::Argument_list &args);
  shcore::Value rejoin_instance(const shcore::Argument_list &args);
  shcore::Value remove_instance(const shcore::Argument_list &args);
  shcore::Value get_replicaset(const shcore::Argument_list &args);
  shcore::Value describe(const shcore::Argument_list &args);
  shcore::Value status(const shcore::Argument_list &args);
  shcore::Value dissolve(const shcore::Argument_list &args);
  shcore::Value check_instance_state(const shcore::Argument_list &args);
  void rescan(const shcore::Dictionary_t &options);
  shcore::Value force_quorum_using_partition_of(
      const shcore::Argument_list &args);
  shcore::Value disconnect(const shcore::Argument_list &args);
  template <typename T>
  void switch_to_single_primary_mode_t(const T &instance_def) {
    switch_to_single_primary_mode(get_connection_options(instance_def));
  }
  void switch_to_single_primary_mode(const std::string &instance_def) {
    switch_to_single_primary_mode_t(instance_def);
  }
  void switch_to_single_primary_mode(const shcore::Dictionary_t &instance_def) {
    switch_to_single_primary_mode_t(instance_def);
  }
  void switch_to_single_primary_mode(void) {
    switch_to_single_primary_mode(mysqlshdk::db::Connection_options{});
  }
  void switch_to_multi_primary_mode(void);
  void set_primary_instance(const std::string &instance_def);
  void set_primary_instance(const shcore::Dictionary_t &instance_def);

  Cluster_check_info check_preconditions(
      const std::string &function_name) const;

  std::shared_ptr<MetadataStorage> get_metadata_storage() const {
    return _metadata_storage;
  }

  // new metadata object
  std::shared_ptr<mysqlshdk::innodbcluster::Metadata_mysql> metadata() const;

  /**
   * Synchronize transactions on target instance.
   *
   * Wait for all current cluster transactions to be applied on the specified
   * target instance.
   *
   * @param target_instance instance to wait for transaction to be applied.
   *
   * @throw RuntimeError if the timeout is reached when waiting for
   * transactions to be applied.
   */
  void sync_transactions(
      const mysqlshdk::mysql::IInstance &target_instance) const;

  /**
   * Mark the cluster as invalid (e.g., dissolved).
   */
  void invalidate() { m_invalidated = true; }

 protected:
  uint64_t _id;
  std::string _name;
  std::shared_ptr<ReplicaSet> _default_replica_set;
  std::string _description;
  shcore::Value::Map_type_ref _accounts;
  shcore::Value::Map_type_ref _options;
  shcore::Value::Map_type_ref _attributes;
  bool m_invalidated;
  // Session to a member of the group so we can query its status and other
  // stuff from pfs
  std::shared_ptr<mysqlshdk::db::ISession> _group_session;
  std::shared_ptr<MetadataStorage> _metadata_storage;
  // Used shell options
  void init();

 private:
  std::shared_ptr<ProvisioningInterface> _provisioning_interface;

  void set_account_data(const std::string &account, const std::string &key,
                        const std::string &value);
  std::string get_account_data(const std::string &account,
                               const std::string &key);
  shcore::Value::Map_type_ref _rescan(const shcore::Argument_list &args);

  void switch_to_single_primary_mode(const Connection_options &instance_def);
  void set_primary_instance(const Connection_options &instance_def);
};
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_CLUSTER_H_
