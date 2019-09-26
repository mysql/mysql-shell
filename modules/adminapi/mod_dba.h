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

#ifndef MODULES_ADMINAPI_MOD_DBA_H_
#define MODULES_ADMINAPI_MOD_DBA_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/common/provisioning_interface.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/replica_set/replica_set_impl.h"
#include "modules/mod_common.h"
#include "mysqlshdk/libs/db/session.h"
#include "scripting/types_cpp.h"
#include "shellcore/ishell_core.h"
#include "shellcore/shell_options.h"

namespace mysqlsh {
namespace dba {
/**
 * \ingroup AdminAPI
 * $(DBA_BRIEF)
 */
class SHCORE_PUBLIC Dba : public shcore::Cpp_object_bridge,
                          public std::enable_shared_from_this<Dba> {
 public:
#if DOXYGEN_JS
  Integer verbose;
  JSON checkInstanceConfiguration(InstanceDef instance, Dictionary options);
  Undefined configureReplicaSetInstance(InstanceDef instance,
                                        Dictionary options);
  Undefined configureLocalInstance(InstanceDef instance, Dictionary options);
  Undefined configureInstance(InstanceDef instance, Dictionary options);
  Cluster createCluster(String name, Dictionary options);
  ReplicaSet createReplicaSet(String name, Dictionary options);
  Undefined deleteSandboxInstance(Integer port, Dictionary options);
  Instance deploySandboxInstance(Integer port, Dictionary options);
  Undefined dropMetadataSchema(Dictionary options);
  Cluster getCluster(String name, Dictionary options);
  ReplicaSet getReplicaSet();
  Undefined killSandboxInstance(Integer port, Dictionary options);
  Undefined rebootClusterFromCompleteOutage(String clusterName,
                                            Dictionary options);
  Undefined startSandboxInstance(Integer port, Dictionary options);
  Undefined stopSandboxInstance(Integer port, Dictionary options);
  Undefined upgradeMetadata(Dictionary options);
#elif DOXYGEN_PY
  int verbose;
  JSON check_instance_configuration(InstanceDef instance, dict options);
  None configure_replica_set_instance(InstanceDef instance, dict options);
  None configure_local_instance(InstanceDef instance, dict options);
  None configure_instance(InstanceDef instance, dict options);
  Cluster create_cluster(str name, dict options);
  ReplicaSet create_replica_set(str name, dict options);
  None delete_sandbox_instance(int port, dict options);
  Instance deploy_sandbox_instance(int port, dict options);
  None drop_metadata_schema(dict options);
  Cluster get_cluster(str name, dict options);
  ReplicaSet get_replica_set();
  None kill_sandbox_instance(int port, dict options);
  None reboot_cluster_from_complete_outage(str clusterName, dict options);
  None start_sandbox_instance(int port, dict options);
  None stop_sandbox_instance(int port, dict options);
  None upgrade_metadata(dict options);
#endif

  explicit Dba(shcore::IShell_core *owner);
  virtual ~Dba();

  static std::set<std::string> _deploy_instance_opts;
  static std::set<std::string> _stop_instance_opts;
  static std::set<std::string> _default_local_instance_opts;
  static std::set<std::string> _create_cluster_opts;
  static std::set<std::string> _reboot_cluster_opts;

  virtual std::string class_name() const { return "Dba"; }

  virtual bool operator==(const Object_bridge &other) const;

  virtual void set_member(const std::string &prop, shcore::Value value);
  virtual shcore::Value get_member(const std::string &prop) const;

  Cluster_check_info check_preconditions(
      std::shared_ptr<Instance> target_instance,
      const std::string &function_name) const;

  shcore::IShell_core *get_owner() { return _shell_core; }

  virtual std::shared_ptr<mysqlshdk::db::ISession> get_active_shell_session()
      const;

  virtual void connect_to_target_group(
      std::shared_ptr<Instance> target_member_session,
      std::shared_ptr<MetadataStorage> *out_metadata,
      std::shared_ptr<Instance> *out_group_server,
      bool connect_to_primary) const;

  virtual std::shared_ptr<Instance> connect_to_target_member() const;

  std::shared_ptr<Cluster> get_cluster(
      const char *name, std::shared_ptr<MetadataStorage> metadata,
      std::shared_ptr<Instance> group_server) const;

  void do_configure_instance(
      const mysqlshdk::db::Connection_options &instance_def_,
      const shcore::Dictionary_t &options, bool local,
      Cluster_type cluster_type);

 public:  // Exported public methods
  shcore::Value check_instance_configuration(const shcore::Argument_list &args);
  // create and start
  shcore::Value deploy_sandbox_instance(const shcore::Argument_list &args,
                                        const std::string &fname);
  shcore::Value stop_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value delete_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value kill_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value start_sandbox_instance(const shcore::Argument_list &args);

  void configure_local_instance(const std::string &instance_def,
                                const shcore::Dictionary_t &options) {
    configure_local_instance(instance_def.empty()
                                 ? mysqlshdk::db::Connection_options()
                                 : get_connection_options(instance_def),
                             options);
  }

  void configure_local_instance(const shcore::Dictionary_t &instance_def,
                                const shcore::Dictionary_t &options) {
    configure_local_instance(instance_def && !instance_def->empty()
                                 ? get_connection_options(instance_def)
                                 : mysqlshdk::db::Connection_options(),
                             options);
  }

  void configure_local_instance(
      const mysqlshdk::db::Connection_options &instance_def,
      const shcore::Dictionary_t &options);

  void configure_instance(const std::string &instance_def,
                          const shcore::Dictionary_t &options) {
    configure_instance(instance_def.empty()
                           ? mysqlshdk::db::Connection_options()
                           : get_connection_options(instance_def),
                       options);
  }

  void configure_instance(const shcore::Dictionary_t &instance_def,
                          const shcore::Dictionary_t &options) {
    configure_instance(instance_def && !instance_def->empty()
                           ? get_connection_options(instance_def)
                           : mysqlshdk::db::Connection_options(),
                       options);
  }

  void configure_instance(const mysqlshdk::db::Connection_options &instance_def,
                          const shcore::Dictionary_t &options);

  void configure_replica_set_instance(const std::string &instance_def,
                                      const shcore::Dictionary_t &options) {
    configure_replica_set_instance(instance_def.empty()
                                       ? mysqlshdk::db::Connection_options()
                                       : get_connection_options(instance_def),
                                   options);
  }

  void configure_replica_set_instance(const shcore::Dictionary_t &instance_def,
                                      const shcore::Dictionary_t &options) {
    configure_replica_set_instance(instance_def && !instance_def->empty()
                                       ? get_connection_options(instance_def)
                                       : mysqlshdk::db::Connection_options(),
                                   options);
  }

  void configure_replica_set_instance(
      const mysqlshdk::db::Connection_options &instance_def,
      const shcore::Dictionary_t &options);

  shcore::Value create_cluster(const std::string &cluster_name,
                               const shcore::Dictionary_t &options);
  void upgrade_metadata(const shcore::Dictionary_t &options);

  shcore::Value get_cluster_(const shcore::Argument_list &args);
  void drop_metadata_schema(const shcore::Dictionary_t &args);

  shcore::Value reboot_cluster_from_complete_outage(
      const shcore::Argument_list &args);

  virtual std::vector<std::pair<std::string, std::string>>
  get_replicaset_instances_status(std::shared_ptr<Cluster> cluster,
                                  const shcore::Value::Map_type_ref &options);

  virtual void validate_instances_status_reboot_cluster(
      std::shared_ptr<Cluster> cluster,
      const mysqlshdk::mysql::IInstance &target_instance,
      shcore::Value::Map_type_ref options);
  virtual void validate_instances_gtid_reboot_cluster(
      std::shared_ptr<Cluster> cluster,
      const shcore::Value::Map_type_ref &options,
      const mysqlshdk::mysql::IInstance &target_instance);

  // ReplicaSets

  shcore::Value create_replica_set(const std::string &full_rs_name,
                                   const shcore::Dictionary_t &options);

  shcore::Value get_replica_set();
  std::shared_ptr<Replica_set_impl> get_replica_set(
      const std::shared_ptr<MetadataStorage> &metadata,
      const std::shared_ptr<Instance> &target_server);

  static std::shared_ptr<mysqlshdk::db::ISession> get_session(
      const mysqlshdk::db::Connection_options &args);

 protected:
  shcore::IShell_core *_shell_core;

  void init();

 private:
  std::shared_ptr<ProvisioningInterface> _provisioning_interface;

  shcore::Value exec_instance_op(const std::string &function,
                                 const shcore::Argument_list &args,
                                 const std::string &password = "");
};
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_H_
