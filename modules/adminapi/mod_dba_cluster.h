/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MOD_DBA_ADMIN_CLUSTER_H_
#define _MOD_DBA_ADMIN_CLUSTER_H_

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

#include "mod_dba_replicaset.h"

#define ACC_USER "username"
#define ACC_PASSWORD "password"
#define ACC_INSTANCE_ADMIN "instanceAdmin"
#define ACC_CLUSTER_READER "clusterReader"
#define ACC_REPLICATION_USER "replicationUser"
#define ACC_CLUSTER_ADMIN "clusterAdmin"

#define OPT_ADMIN_TYPE "adminType"

#define ATT_DEFAULT "default"

namespace mysh {
namespace dba {
class MetadataStorage;
/**
* Represents a Cluster
*/
class Cluster : public std::enable_shared_from_this<Cluster>, public shcore::Cpp_object_bridge {
public:
  Cluster(const std::string &name, std::shared_ptr<MetadataStorage> metadata_storage);
  virtual ~Cluster();

  virtual std::string class_name() const { return "Cluster"; }
  virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
  virtual bool operator == (const Object_bridge &other) const;

  virtual void append_json(shcore::JSON_dumper& dumper) const;

  virtual shcore::Value get_member(const std::string &prop) const;

  const uint64_t get_id() { return _id; }
  void set_id(uint64_t id) { _id = id; }
  std::shared_ptr<ReplicaSet> get_default_replicaset() { return _default_replica_set; }
  void set_default_replicaset(std::shared_ptr<ReplicaSet> default_rs);
  std::string get_name() { return _name; }
  void set_master_key(const std::string &cluster_password) { _master_key = cluster_password; }
  std::string get_master_key() { return _master_key; }
  std::string get_description() { return _description; }
  void set_description(std::string description) { _description = description; };

  void set_account_user(const std::string& account, const std::string& value) { set_account_data(account, ACC_USER, value); }
  std::string get_account_user(const std::string& account) { return get_account_data(account, ACC_USER); }
  void set_account_password(const std::string& account, const std::string& value) { set_account_data(account, ACC_PASSWORD, value); }
  std::string get_account_password(const std::string& account) { return get_account_data(account, ACC_PASSWORD); }
  std::string get_accounts_data();
  void set_accounts_data(const std::string& json);

  void set_option(const std::string& option, const shcore::Value &value);
  void set_options(const std::string& json) { _options = shcore::Value::parse(json).as_map(); }
  std::string get_options() { return shcore::Value(_options).json(false); }

  void set_attribute(const std::string& attribute, const shcore::Value &value);
  void set_attributes(const std::string& json) { _attributes = shcore::Value::parse(json).as_map(); }
  std::string get_attributes() { return shcore::Value(_attributes).json(false); }

  std::shared_ptr<ProvisioningInterface> get_provisioning_interface() {
    return _provisioning_interface;
  }

  void set_provisioning_interface(std::shared_ptr<ProvisioningInterface> provisioning_interface) {
    _provisioning_interface = provisioning_interface;
  }

#if DOXYGEN_JS
  String getName();
  String getAdminType();
  Undefined addSeedInstance(String conn);
  Undefined addSeedInstance(Document doc);
  Undefined addInstance(String conn);
  Undefined addInstance(Document doc);
  Undefined rejoinInstance(String doc);
  Undefined removeInstance(String name);
  Undefined removeInstance(Document doc);
  String describe();
  Undefined dissolve(Document doc);
#elif DOXYGEN_PY
  str get_name();
  str get_admin_type();
  None add_seed_instance(str conn);
  None add_seed_instance(Document doc);
  None add_instance(str conn);
  None add_instance(Document doc);
  None rejoin_instance(str conn);
  None remove_instance(str name);
  None remove_instance(Document doc);
  str describe();
  None dissolve(Document doc);
#endif

  shcore::Value add_seed_instance(const shcore::Argument_list &args);
  shcore::Value add_instance(const shcore::Argument_list &args);
  shcore::Value rejoin_instance(const shcore::Argument_list &args);
  shcore::Value remove_instance(const shcore::Argument_list &args);
  shcore::Value get_replicaset(const shcore::Argument_list &args);
  shcore::Value describe(const shcore::Argument_list &args);
  shcore::Value status(const shcore::Argument_list &args);
  shcore::Value dissolve(const shcore::Argument_list &args);

protected:
  uint64_t _id;
  std::string _name;
  std::shared_ptr<ReplicaSet> _default_replica_set;
  std::string _master_key;
  std::string _description;
  shcore::Value::Map_type_ref _accounts;
  shcore::Value::Map_type_ref _options;
  shcore::Value::Map_type_ref _attributes;

private:
  // This flag will be used to determine what should be included on the JSON output for the object
  // 0 standard
  // 1 means status
  // 2 means describe
  int _json_mode;
  void init();

  std::shared_ptr<MetadataStorage> _metadata_storage;
  std::shared_ptr<ProvisioningInterface> _provisioning_interface;

  void set_account_data(const std::string& account, const std::string& key, const std::string& value);
  std::string get_account_data(const std::string& account, const std::string& key);
};
}
}

#endif  // _MOD_DBA_ADMIN_CLUSTER_H_
