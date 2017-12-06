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

#ifndef MODULES_ADMINAPI_MOD_DBA_H_
#define MODULES_ADMINAPI_MOD_DBA_H_

#include <set>
#include <map>
#include <utility>
#include <vector>
#include <string>

#include "modules/mod_common.h"
#include "scripting/types_cpp.h"
#include "shellcore/ishell_core.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/mod_dba_provisioning_interface.h"
#include "modules/adminapi/mod_dba_common.h"
#include "mysqlshdk/libs/db/session.h"

namespace mysqlsh {
namespace dba {
/**
* \ingroup AdminAPI
* $(DBA_BRIEF)
*/
class SHCORE_PUBLIC Dba : public shcore::Cpp_object_bridge,
                          public std::enable_shared_from_this<Dba> {
 public:
  explicit Dba(shcore::IShell_core *owner);
  virtual ~Dba();

  static std::set<std::string> _deploy_instance_opts;
  static std::set<std::string> _stop_instance_opts;
  static std::set<std::string> _default_local_instance_opts;
  static std::set<std::string> _create_cluster_opts;
  static std::set<std::string> _reboot_cluster_opts;

  virtual std::string class_name() const { return "Dba"; }

  virtual bool operator == (const Object_bridge &other) const;

  virtual void set_member(const std::string &prop, shcore::Value value);
  virtual shcore::Value get_member(const std::string &prop) const;

  std::shared_ptr<mysqlshdk::db::ISession> get_active_session() const;
  ReplicationGroupState check_preconditions(
      const std::string& function_name) const;
  virtual int get_default_port() const { return 33060; }
  int get_default_instance_port() { return 3306; }

  shcore::Value check_instance_configuration(const shcore::Argument_list &args);
  // create and start
  shcore::Value deploy_sandbox_instance(const shcore::Argument_list &args,
                                        const std::string &fname);
  shcore::Value stop_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value delete_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value kill_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value start_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value configure_local_instance(const shcore::Argument_list &args);

  shcore::Value clone_instance(const shcore::Argument_list &args);
  shcore::Value reset_instance(const shcore::Argument_list &args);

  shcore::Value reset_session(const shcore::Argument_list &args);
  shcore::Value create_cluster(const shcore::Argument_list &args);
  shcore::Value get_cluster(const shcore::Argument_list &args) const;
  shcore::Value drop_metadata_schema(const shcore::Argument_list &args);

  shcore::Value reboot_cluster_from_complete_outage(
      const shcore::Argument_list &args);

  shcore::IShell_core* get_owner() { return _shell_core; }

  virtual std::vector<std::pair<std::string, std::string>>
      get_replicaset_instances_status(
          std::string *out_cluster_name,
          const shcore::Value::Map_type_ref &options);

  virtual void validate_instances_status_reboot_cluster(
      const shcore::Argument_list &args);
  virtual void validate_instances_gtid_reboot_cluster(
      std::string *out_cluster_name,
      const shcore::Value::Map_type_ref &options,
      const std::shared_ptr<mysqlshdk::db::ISession> &instance_session);

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
  Undefined checkInstanceConfiguration(
      InstanceDef instance, Dictionary options);
  Instance configureLocalInstance(InstanceDef instance, Dictionary options);
  Undefined rebootClusterFromCompleteOutage(
      String clusterName, Dictionary options);
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
  None check_instance_configuration(InstanceDef instance, dict options);
  JSON configure_local_instance(InstanceDef instance, dict options);
  None reboot_cluster_from_complete_outage(str clusterName, dict options);
#endif

  static std::shared_ptr<mysqlshdk::db::ISession> get_session(
      const mysqlshdk::db::Connection_options &args);

  virtual shcore::Value::Map_type_ref _check_instance_configuration(
      const mysqlshdk::db::Connection_options &instance_def,
      const shcore::Value::Map_type_ref &options, bool allow_update);

 protected:
  std::shared_ptr<mysqlshdk::db::ISession> _custom_session;
  shcore::IShell_core *_shell_core;

  void init();

  // Added for limited mock support
  Dba() {}
  void set_owner(shcore::IShell_core *shell_core) {
    _shell_core = shell_core;
    init();
  }
  std::shared_ptr<MetadataStorage> _metadata_storage;

 private:
  uint64_t _connection_id;
  std::shared_ptr<ProvisioningInterface> _provisioning_interface;

  shcore::Value exec_instance_op(const std::string &function,
                                 const shcore::Argument_list &args);
  static std::map <std::string, std::shared_ptr<
                  mysqlshdk::db::ISession> > _session_cache;
};
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_H_
