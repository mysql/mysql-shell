/*
 * Copyright (c) 2016, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
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
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/common/provisioning_interface.h"
#include "modules/adminapi/dba/api_options.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/adminapi/mod_dba_cluster_set.h"
#include "modules/adminapi/mod_dba_replica_set.h"
#include "modules/adminapi/replica_set/replica_set_impl.h"
#include "modules/mod_common.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/libs/db/session.h"
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
  Undefined configureInstance(InstanceDef instance, Dictionary options);
  Cluster createCluster(String name, Dictionary options);
  ReplicaSet createReplicaSet(String name, Dictionary options);
  Undefined deleteSandboxInstance(Integer port, Dictionary options);
  Instance deploySandboxInstance(Integer port, Dictionary options);
  Undefined dropMetadataSchema(Dictionary options);
  Cluster getCluster(String name);
  ClusterSet getClusterSet();
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
  None configure_instance(InstanceDef instance, dict options);
  Cluster create_cluster(str name, dict options);
  ReplicaSet create_replica_set(str name, dict options);
  None delete_sandbox_instance(int port, dict options);
  Instance deploy_sandbox_instance(int port, dict options);
  None drop_metadata_schema(dict options);
  Cluster get_cluster(str name);
  ClusterSet get_cluster_set();
  ReplicaSet get_replica_set();
  None kill_sandbox_instance(int port, dict options);
  Cluster reboot_cluster_from_complete_outage(str clusterName, dict options);
  None start_sandbox_instance(int port, dict options);
  None stop_sandbox_instance(int port, dict options);
  None upgrade_metadata(dict options);
#endif

  explicit Dba(shcore::IShell_core *owner);
  explicit Dba(const std::shared_ptr<ShellBaseSession> &session);
  virtual ~Dba() = default;

  virtual std::string class_name() const { return "Dba"; }

  virtual bool operator==(const Object_bridge &other) const;

  virtual void set_member(const std::string &prop, shcore::Value value);
  virtual shcore::Value get_member(const std::string &prop) const;

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
      std::shared_ptr<Instance> group_server, bool reboot_cluster = false,
      bool allow_invalidated = false) const;

  void do_configure_instance(mysqlshdk::db::Connection_options instance_def,
                             const Configure_instance_options &options,
                             Cluster_type purpose);

 public:  // Exported public methods
  shcore::Value check_instance_configuration(
      const std::optional<Connection_options> &instance_def = {},
      const shcore::Option_pack_ref<Check_instance_configuration_options>
          &options = {});
  // create and start
  void deploy_sandbox_instance(
      int port,
      const shcore::Option_pack_ref<Deploy_sandbox_options> &options = {});
  void stop_sandbox_instance(
      int port,
      const shcore::Option_pack_ref<Stop_sandbox_options> &options = {});
  void delete_sandbox_instance(
      int port,
      const shcore::Option_pack_ref<Common_sandbox_options> &options = {});
  void kill_sandbox_instance(
      int port,
      const shcore::Option_pack_ref<Common_sandbox_options> &options = {});
  void start_sandbox_instance(
      int port,
      const shcore::Option_pack_ref<Common_sandbox_options> &options = {});

  void configure_instance(
      const std::optional<Connection_options> &instance_def = {},
      const shcore::Option_pack_ref<Configure_cluster_instance_options>
          &options = {});

  void configure_replica_set_instance(
      const std::optional<Connection_options> &instance_def = {},
      const shcore::Option_pack_ref<Configure_replicaset_instance_options>
          &options = {});

  shcore::Value create_cluster(
      const std::string &cluster_name,
      const shcore::Option_pack_ref<Create_cluster_options> &options = {});
  void upgrade_metadata(
      const shcore::Option_pack_ref<Upgrade_metadata_options> &options);

  std::shared_ptr<Cluster> get_cluster(
      const std::optional<std::string> &cluster_name = {}) const;
  void drop_metadata_schema(
      const shcore::Option_pack_ref<Drop_metadata_schema_options> &options);

  std::shared_ptr<Cluster> reboot_cluster_from_complete_outage(
      const std::optional<std::string> &cluster_name = {},
      const shcore::Option_pack_ref<Reboot_cluster_options> &options = {});

  // ReplicaSets

  shcore::Value create_replica_set(
      const std::string &full_rs_name,
      const shcore::Option_pack_ref<Create_replicaset_options> &options);

  std::shared_ptr<ReplicaSet> get_replica_set();
  std::shared_ptr<Replica_set_impl> get_replica_set(
      const std::shared_ptr<MetadataStorage> &metadata,
      const std::shared_ptr<Instance> &target_server);

  // ClusterSet
  std::shared_ptr<ClusterSet> get_cluster_set();

  std::shared_ptr<Instance> check_preconditions(
      const Command_conditions &conds,
      const std::shared_ptr<Instance> &group_server);

 protected:
  shcore::IShell_core *_shell_core = nullptr;
  std::shared_ptr<ShellBaseSession> m_session;

  void init();

 private:
  ProvisioningInterface _provisioning_interface;

  void exec_instance_op(const std::string &function, int port,
                        const std::string &sandbox_dir,
                        const std::string &password = "");
};
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_H_
