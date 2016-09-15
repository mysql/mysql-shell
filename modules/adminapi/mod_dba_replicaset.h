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

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include <set>
#include "mod_dba_provisioning_interface.h"

namespace mysh {
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

  virtual void append_json(shcore::JSON_dumper& dumper) const;

  virtual shcore::Value get_member(const std::string &prop) const;

  void set_id(uint64_t id) { _id = id; }
  uint64_t get_id() { return _id; }

  void set_name(std::string name) { _name = name; }

  void set_json_mode(int mode) { _json_mode = mode; }

  void set_cluster(std::shared_ptr<Cluster> cluster) { _cluster = cluster; }
  std::shared_ptr<Cluster> get_cluster() const { return _cluster; }

  std::string get_topology_type() const { return _topology_type; }

  static char const *kTopologyPrimaryMaster;
  static char const *kTopologyMultiMaster;

#if DOXYGEN_JS
  String getName();
  Undefined addInstance(String conn);
  Undefined addInstance(Document doc);
  Undefined rejoinInstance(String name);
  Undefined removeInstance(String name);
  Undefined removeInstance(Document doc);
  Undefined dissolve(Document doc);
  Undefined disable();

#elif DOXYGEN_PY
  str get_name();
  None add_instance(str conn);
  None add_instance(Document doc);
  None rejoin_instance(str name);
  None remove_instance(str name);
  None remove_instance(Document doc);
  None dissolve(Document doc);
  None disable();
#endif

  shcore::Value add_instance_(const shcore::Argument_list &args);
  shcore::Value add_instance(const shcore::Argument_list &args);
  shcore::Value rejoin_instance(const shcore::Argument_list &args);
  shcore::Value remove_instance_(const shcore::Argument_list &args);
  shcore::Value remove_instance(const shcore::Argument_list &args);
  shcore::Value dissolve(const shcore::Argument_list &args);
  shcore::Value disable(const shcore::Argument_list &args);

protected:
  uint64_t _id;
  std::string _name;
  std::string _topology_type;
  // TODO: add missing fields, rs_type, etc

private:
  // This flag will be used to determine what should be included on the JSON output for the object
  // 0 standard
  // 1 means status
  // 2 means describe
  int _json_mode;
  void append_json_status(shcore::JSON_dumper& dumper) const;
  void append_json_description(shcore::JSON_dumper& dumper) const;
  void init();

  bool do_join_replicaset(const std::string &instance_url,
      const std::string &peer_instance_url,
      const std::string &super_user_password,
      const std::string &repl_user, const std::string &repl_user_password);

  std::shared_ptr<Cluster> _cluster;
  std::shared_ptr<MetadataStorage> _metadata_storage;
  std::shared_ptr<ProvisioningInterface> _provisioning_interface;

protected:
  virtual int get_default_port() { return 33060; };
};
}
}

#endif  // _MOD_DBA_ADMIN_REPLICASET_H_
