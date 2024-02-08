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

#ifndef MODULES_ADMINAPI_MOD_DBA_CLUSTER_H_
#define MODULES_ADMINAPI_MOD_DBA_CLUSTER_H_

#include <memory>
#include <string>

#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/shell_options.h"

#include "modules/adminapi/base_cluster.h"
#include "modules/adminapi/cluster/api_options.h"
#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/cluster_set/api_options.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/adminapi/mod_dba_cluster_set.h"
#include "mysqlshdk/libs/db/connection_options.h"

namespace mysqlsh {
namespace dba {

/**
 * \ingroup AdminAPI
 * $(CLUSTER_BRIEF)
 *
 * $(CLUSTER_DETAIL)
 */
class Cluster : public std::enable_shared_from_this<Cluster>,
                public Base_cluster {
 public:
#if DOXYGEN_JS
  String name;  //!< $(CLUSTER_GETNAME_BRIEF)
  Undefined addInstance(InstanceDef instance, Dictionary options);
  ClusterSet createClusterSet(String domainName, Dictionary options);
  ClusterSet getClusterSet();
  Dictionary describe();
  Undefined disconnect();
  Undefined dissolve(Dictionary options);
  Undefined forceQuorumUsingPartitionOf(InstanceDef instance);
  String getName();
  Dictionary listRouters(Dictionary options);
  Undefined setRoutingOption(String option, String value);
  Undefined setRoutingOption(String router, String option, String value);
  Dictionary routingOptions(String router);
  Dictionary rejoinInstance(InstanceDef instance, Dictionary options);
  Undefined removeInstance(InstanceDef instance, Dictionary options);
  Undefined rescan(Dictionary options);
  Undefined resetRecoveryAccountsPassword(Dictionary options);
  Dictionary status(Dictionary options);
  Undefined switchToSinglePrimaryMode(InstanceDef instance);
  Undefined switchToMultiPrimaryMode();
  Undefined setPrimaryInstance(InstanceDef instance, Dictionary options);
  Dictionary options(Dictionary options);
  Undefined setOption(String option, String value);
  Undefined setInstanceOption(InstanceDef instance, String option,
                              String value);
  Boolean removeRouterMetadata(RouterDef routerDef);
  Undefined setupAdminAccount(String user, Dictionary options);
  Undefined setupRouterAccount(String user, Dictionary options);
  Undefined fenceAllTraffic();
  Undefined fenceWrites();
  Undefined unfenceWrites();
  Undefined addReplicaInstance(InstanceDef instance, Dictionary options);
#elif DOXYGEN_PY
  str name;  //!< $(CLUSTER_GETNAME_BRIEF)
  None add_instance(InstanceDef instance, dict options);
  ClusterSet create_cluster_set(str domainName, dict options);
  ClusterSet get_cluster_set();
  dict describe();
  None disconnect();
  None dissolve(dict options);
  None force_quorum_using_partition_of(InstanceDef instance);
  str get_name();
  dict list_routers(dict options);
  None set_routing_option(str option, str value);
  None set_routing_option(str router, str option, str value);
  dict routing_options(str router);
  dict rejoin_instance(InstanceDef instance, dict options);
  None remove_instance(InstanceDef instance, dict options);
  None rescan(dict options);
  None reset_recovery_accounts_password(dict options);
  dict status(dict options);
  None switch_to_single_primary_mode(InstanceDef instance);
  None switch_to_multi_primary_mode();
  None set_primary_instance(InstanceDef instance, dict options);
  dict options(dict options);
  None set_option(str option, str value);
  None set_instance_option(InstanceDef instance, str option, str value);
  bool remove_router_metadata(RouterDef routerDef);
  None setup_admin_account(str user, dict options);
  None setup_router_account(str user, dict options);
  None fence_all_traffic();
  None fence_writes();
  None unfence_writes();
  None add_replica_instance(InstanceDef instance, dict options);
#endif

  explicit Cluster(const std::shared_ptr<Cluster_impl> &impl);

  virtual ~Cluster();

  std::string class_name() const override { return "Cluster"; }

  std::shared_ptr<Cluster_impl> impl() const { return m_impl; }

  Base_cluster_impl *base_impl() const override { return m_impl.get(); }

  void assert_valid(const std::string &option_name) const override;

  void add_instance(const Connection_options &instance_def,
                    const shcore::Option_pack_ref<cluster::Add_instance_options>
                        &options = {});
  void rejoin_instance(
      const Connection_options &instance_def,
      const shcore::Option_pack_ref<cluster::Rejoin_instance_options> &options =
          {});
  void remove_instance(
      const Connection_options &instance_def,
      const shcore::Option_pack_ref<cluster::Remove_instance_options> &options =
          {});
  shcore::Value get_replicaset(const shcore::Argument_list &args);
  shcore::Value describe(void);
  shcore::Value status(
      const shcore::Option_pack_ref<cluster::Status_options> &options);
  void dissolve(const shcore::Option_pack_ref<Force_options> &options);
  void rescan(const shcore::Option_pack_ref<cluster::Rescan_options> &options);
  void reset_recovery_accounts_password(
      const shcore::Option_pack_ref<Force_options> &options);
  void force_quorum_using_partition_of(const Connection_options &instance_def);
  void disconnect();

  void remove_router_metadata(const std::string &router_def);

  void switch_to_single_primary_mode(
      const Connection_options &instance_def = Connection_options());

  void switch_to_multi_primary_mode(void);
  void set_primary_instance(
      const Connection_options &instance_def,
      const shcore::Option_pack_ref<cluster::Set_primary_instance_options>
          &options = {});

  shcore::Value options(
      const shcore::Option_pack_ref<cluster::Options_options> &options);

  void set_option(const std::string &option, const shcore::Value &value);

  void set_instance_option(const Connection_options &instance_def,
                           const std::string &option,
                           const shcore::Value &value);

  void fence_all_traffic();
  void fence_writes();
  void unfence_writes();

  // Read-Replicas
  void add_replica_instance(
      const std::string &instance_def,
      const shcore::Option_pack_ref<cluster::Add_replica_instance_options>
          &options = {});

  // ClusterSet
  shcore::Value create_cluster_set(
      const std::string &domain_name,
      const shcore::Option_pack_ref<clusterset::Create_cluster_set_options>
          &options);

  std::shared_ptr<ClusterSet> get_cluster_set() const;

 protected:
  // Used shell options
  void init();

 private:
  std::shared_ptr<Cluster_impl> m_impl;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_CLUSTER_H_
