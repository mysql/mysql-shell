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

#ifndef _MOD_DBA_H_
#define _MOD_DBA_H_

#include "modules/mod_common.h"
#include "modules/mod_mysql_session.h"
#include "shellcore/types_cpp.h"
#include "shellcore/ishell_core.h"
#include "modules/base_session.h"
#include "mod_dba_cluster.h"
#include <set>
#include <map>
#include "mod_dba_provisioning_interface.h"
#include "modules/adminapi/mod_dba_common.h"

namespace mysqlsh {
namespace dba {
/**
* $(DBA_BRIEF)
*/
class SHCORE_PUBLIC Dba : public shcore::Cpp_object_bridge, public std::enable_shared_from_this<Dba> {
public:
  Dba(shcore::IShell_core* owner);
  virtual ~Dba();

  static std::set<std::string> _deploy_instance_opts;
  static std::set<std::string> _stop_instance_opts;
  static std::set<std::string> _default_local_instance_opts;
  static std::set<std::string> _create_cluster_opts;

  virtual std::string class_name() const { return "Dba"; };

  virtual bool operator == (const Object_bridge &other) const;

  virtual void set_member(const std::string &prop, shcore::Value value);
  virtual shcore::Value get_member(const std::string &prop) const;

  std::shared_ptr<ShellDevelopmentSession> get_active_session() const;
  ReplicationGroupState check_preconditions(const std::string& function_name) const;
  virtual int get_default_port() { return 33060; };
  int get_default_instance_port() { return 3306; }

  shcore::Value check_instance_config(const shcore::Argument_list &args);
  shcore::Value deploy_sandbox_instance(const shcore::Argument_list &args, const std::string &fname); // create and start
  shcore::Value stop_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value delete_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value kill_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value start_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value config_local_instance(const shcore::Argument_list &args);

  shcore::Value clone_instance(const shcore::Argument_list &args);
  shcore::Value reset_instance(const shcore::Argument_list &args);

  shcore::Value reset_session(const shcore::Argument_list &args);
  shcore::Value create_cluster(const shcore::Argument_list &args);
  shcore::Value get_cluster(const shcore::Argument_list &args) const;
  shcore::Value drop_metadata_schema(const shcore::Argument_list &args);

  shcore::Value reboot_cluster_from_complete_outage(const shcore::Argument_list &args);

  shcore::IShell_core* get_owner() { return _shell_core; }

  std::vector<std::pair<std::string, std::string>> get_replicaset_instances_status(std::string *out_cluster_name,
          const shcore::Value::Map_type_ref &options);

  void validate_instances_status_reboot_cluster(const shcore::Argument_list &args);
  void validate_instances_gtid_reboot_cluster(std::string *out_cluster_name,
                                              const shcore::Value::Map_type_ref &options,
                                              const std::shared_ptr<ShellDevelopmentSession> &instance_session);

#if DOXYGEN_JS
  Integer verbose;
  Cluster createCluster(String name, Dictionary options);
  Undefined deleteSandboxInstance(Integer port, Dictionary options);
  Instance deploySandboxInstance(Integer port, Dictionary options);
  Undefined dropMetadataSchema(Dictionary options);
  Cluster getCluster(String name);
  Undefined killSandboxInstance(Integer port, Dictionary options);
  Undefined resetSession(Session session);
  Undefined startSandboxInstance(Integer port, Dictionary options);
  Undefined stopSandboxInstance(Integer port, Dictionary options);
  Undefined checkInstanceConfig(InstanceDef instance, Dictionary options);
  Instance configLocalInstance(InstanceDef instance, Dictionary options);
  Undefined rebootClusterFromCompleteOutage(String clusterName, Dictionary options);
#elif DOXYGEN_PY
  int verbose;
  Cluster create_cluster(str name, dict options);
  None delete_sandbox_instance(int port, dict options);
  Instance deploy_sandbox_instance(int port, dict options);
  None drop_cluster(str name);
  None drop_metadata_schema(dict options);
  Cluster get_cluster(str name);
  None kill_sandbox_instance(int port, dict options);
  None reset_session(Session session);
  None start_sandbox_instance(int port, dict options);
  None stop_sandbox_instance(int port, dict options);
  None check_instance_config(InstanceDef instance, dict options);
  JSON config_local_instance(InstanceDef instance, dict options);
  None reboot_cluster_from_complete_outage(str clusterName, dict options);
#endif

  static std::shared_ptr<mysqlsh::mysql::ClassicSession> get_session(const shcore::Argument_list& args);

protected:
  std::shared_ptr<mysqlsh::ShellDevelopmentSession> _custom_session;
  shcore::IShell_core *_shell_core;

  void init();

private:
  std::shared_ptr<MetadataStorage> _metadata_storage;
  uint64_t _connection_id;
  std::shared_ptr<ProvisioningInterface> _provisioning_interface;

  shcore::Value exec_instance_op(const std::string &function, const shcore::Argument_list &args);
  shcore::Value::Map_type_ref _check_instance_config(const shcore::Argument_list &args, bool allow_update);

  static std::map <std::string, std::shared_ptr<mysqlsh::mysql::ClassicSession> > _session_cache;
};
}
}

#endif
