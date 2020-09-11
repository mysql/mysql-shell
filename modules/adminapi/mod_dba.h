/*
 * Copyright (c) 2016, 2020, Oracle and/or its affiliates.
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
#include "modules/adminapi/mod_dba_replica_set.h"
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
  Session session;
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
  Cluster rebootClusterFromCompleteOutage(String clusterName,
                                          Dictionary options);
  Undefined startSandboxInstance(Integer port, Dictionary options);
  Undefined stopSandboxInstance(Integer port, Dictionary options);
  Undefined upgradeMetadata(Dictionary options);
#elif DOXYGEN_PY
  int verbose;
  Session session;
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
  Cluster reboot_cluster_from_complete_outage(str clusterName, dict options);
  None start_sandbox_instance(int port, dict options);
  None stop_sandbox_instance(int port, dict options);
  None upgrade_metadata(dict options);
#endif

  explicit Dba(shcore::IShell_core *owner);
  explicit Dba(const std::shared_ptr<ShellBaseSession> &session);
  virtual ~Dba();

  virtual std::string class_name() const { return "Dba"; }

  virtual bool operator==(const Object_bridge &other) const;

  virtual void set_member(const std::string &prop, shcore::Value value);
  virtual shcore::Value get_member(const std::string &prop) const;

  // NOTE: BUG#30628479 is applicable to all the API functions and the root
  // cause is that several instances of the MetadataStorage class are created
  // during a function execution. To solve this, all the API functions should
  // create a single instance of the MetadataStorage class and pass it down
  // through the chain call. In other words, the following function should be
  // deprecated in favor of the version below.
  Cluster_check_info check_preconditions(
      std::shared_ptr<Instance> target_instance,
      const std::string &function_name) const;

  Cluster_check_info check_preconditions(
      std::shared_ptr<MetadataStorage> metadata,
      const std::string &function_name) const;

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
  shcore::Value check_instance_configuration(
      const mysqlshdk::utils::nullable<Connection_options> &instance_def = {},
      const shcore::Dictionary_t &options = {});
  // create and start
  shcore::Value deploy_sandbox_instance(const shcore::Argument_list &args,
                                        const std::string &fname);
  shcore::Value stop_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value delete_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value kill_sandbox_instance(const shcore::Argument_list &args);
  shcore::Value start_sandbox_instance(const shcore::Argument_list &args);

  void configure_local_instance(
      const mysqlshdk::utils::nullable<Connection_options> &instance_def = {},
      const shcore::Dictionary_t &options = {});

  void configure_instance(
      const mysqlshdk::utils::nullable<Connection_options> &instance_def = {},
      const shcore::Dictionary_t &options = {});

  void configure_replica_set_instance(
      const mysqlshdk::utils::nullable<Connection_options> &instance_def = {},
      const shcore::Dictionary_t &options = {});

  shcore::Value create_cluster(const std::string &cluster_name,
                               const shcore::Dictionary_t &options);
  void upgrade_metadata(const shcore::Dictionary_t &options);

  std::shared_ptr<Cluster> get_cluster(
      const mysqlshdk::utils::nullable<std::string> &cluster_name = {},
      const shcore::Dictionary_t &options = {}) const;
  void drop_metadata_schema(const shcore::Dictionary_t &args);

  std::shared_ptr<Cluster> reboot_cluster_from_complete_outage(
      const mysqlshdk::utils::nullable<std::string> &cluster_name = {},
      const shcore::Dictionary_t &options = {});

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

  std::shared_ptr<ReplicaSet> get_replica_set();
  std::shared_ptr<Replica_set_impl> get_replica_set(
      const std::shared_ptr<MetadataStorage> &metadata,
      const std::shared_ptr<Instance> &target_server);

 protected:
  shcore::IShell_core *_shell_core = nullptr;
  std::shared_ptr<ShellBaseSession> m_session;

  void init();

 private:
  ProvisioningInterface _provisioning_interface;

  shcore::Value exec_instance_op(const std::string &function,
                                 const shcore::Argument_list &args,
                                 const std::string &password = "");

  void verify_create_cluster_deprecations(const shcore::Dictionary_t &options);
};
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_H_
