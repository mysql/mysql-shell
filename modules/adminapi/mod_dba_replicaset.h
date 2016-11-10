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

#ifndef _MOD_DBA_ADMIN_REPLICASET_H_
#define _MOD_DBA_ADMIN_REPLICASET_H_

#define JSON_STANDARD_OUTPUT 0
#define JSON_STATUS_OUTPUT 1
#define JSON_TOPOLOGY_OUTPUT 2
#define JSON_RESCAN_OUTPUT 3

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include <set>
#include "mod_dba_provisioning_interface.h"

namespace mysqlsh {
namespace mysql {
class ClassicSession;
}

namespace dba {
class MetadataStorage;
class Cluster;

#if DOXYGEN_CPP
/**
* Represents a ReplicaSet
*/
#endif
class ReplicaSet : public std::enable_shared_from_this<ReplicaSet>, public shcore::Cpp_object_bridge {
public:
  ReplicaSet(const std::string &name, const std::string &topology_type,
            std::shared_ptr<MetadataStorage> metadata_storage);
  virtual ~ReplicaSet();

  static std::set<std::string> _add_instance_opts, _remove_instance_opts;

  virtual std::string class_name() const { return "ReplicaSet"; }
  virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
  virtual bool operator == (const Object_bridge &other) const;

  virtual shcore::Value get_member(const std::string &prop) const;

  void set_id(uint64_t id) { _id = id; }
  uint64_t get_id() { return _id; }

  void set_name(std::string name) { _name = name; }

  void set_cluster(std::shared_ptr<Cluster> cluster) { _cluster = cluster; }
  std::shared_ptr<Cluster> get_cluster() const { return _cluster; }

  std::string get_topology_type() const { return _topology_type; }

  void add_instance_metadata(const shcore::Value::Map_type_ref &instance_definition);
  void remove_instance_metadata(const shcore::Argument_list &instance_options);

  void adopt_from_gr();

  std::vector<std::string> get_instances_gr();
  std::vector<std::string> get_instances_md();

  static char const *kTopologyPrimaryMaster;
  static char const *kTopologyMultiMaster;

#if DOXYGEN_JS
  String getName();
  Undefined addInstance(InstanceDef instance, String password);
  Undefined rejoinInstance(IndtanceDef instance);
  Undefined removeInstance(InstanceDef instance);
  Undefined dissolve(Dictionary options);
  Undefined disable();
  Undefined rescan();

#elif DOXYGEN_PY
  str get_name();
  None add_instance(InstanceDef instance, str password);
  None rejoin_instance(InstanceDef instance);
  None remove_instance(InstanceDef instance);
  None dissolve(Dictionary options);
  None disable();
  None rescan();
#endif

  shcore::Value add_instance_(const shcore::Argument_list &args);
  shcore::Value add_instance(const shcore::Argument_list &args);
  shcore::Value check_instance_state(const shcore::Argument_list &args);
  shcore::Value rejoin_instance(const shcore::Argument_list &args);
  shcore::Value remove_instance_(const shcore::Argument_list &args);
  shcore::Value remove_instance(const shcore::Argument_list &args);
  shcore::Value dissolve(const shcore::Argument_list &args);
  shcore::Value disable(const shcore::Argument_list &args);
  shcore::Value retrieve_instance_state(const shcore::Argument_list &args);
  shcore::Value rescan(const shcore::Argument_list &args);

private:
  //TODO these should go to a GroupReplication file
  friend Cluster;
  struct NewInstanceInfo {
    std::string member_id;
    std::string host;
    int port;
  };
  struct MissingInstanceInfo {
    std::string id;
    std::string name;
    std::string host;
  };
  std::vector<NewInstanceInfo> get_newly_discovered_instances();
  std::vector<MissingInstanceInfo> get_unavailable_instances();

  shcore::Value get_status() const;
  shcore::Value get_description() const;

protected:
  uint64_t _id;
  std::string _name;
  std::string _topology_type;
  // TODO: add missing fields, rs_type, etc

private:
  void init();

  bool do_join_replicaset(const std::string &instance_url,
      const std::string &peer_instance_url,
      const std::string &super_user_password,
      const std::string &repl_user, const std::string &repl_user_password);

  std::string get_peer_instance();

  void create_repl_account(mysqlsh::mysql::ClassicSession *session,
                           const std::string &username,
                           const std::string &password);

  void validate_instance_address(std::shared_ptr<mysqlsh::mysql::ClassicSession> session,
                                 const std::string &hostname, int port);

  shcore::Value::Map_type_ref _rescan(const shcore::Argument_list &args);

  std::shared_ptr<Cluster> _cluster;
  std::shared_ptr<MetadataStorage> _metadata_storage;
  std::shared_ptr<ProvisioningInterface> _provisioning_interface;

protected:
  virtual int get_default_port() { return 3306; };
};
}
}

#endif  // _MOD_DBA_ADMIN_REPLICASET_H_
