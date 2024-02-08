/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_MOD_DBA_CLUSTER_SET_H_
#define MODULES_ADMINAPI_MOD_DBA_CLUSTER_SET_H_

#include <memory>
#include <string>
#include "modules/adminapi/base_cluster.h"
#include "modules/adminapi/cluster_set/api_options.h"
#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/common/common.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/shell_options.h"

namespace mysqlsh {
namespace dba {

/**
 * \ingroup AdminAPI
 * $(CLUSTERSET_BRIEF)
 *
 * $(CLUSTERSET)
 */
class ClusterSet : public std::enable_shared_from_this<ClusterSet>,
                   public Base_cluster {
 public:
#if DOXYGEN_JS
  String name;  //!< $(CLUSTERSET_GETNAME_BRIEF)
  String getName();
  Undefined disconnect();
  Cluster createReplicaCluster(InstanceDef instance, String clusterName,
                               Dictionary options);
  Undefined removeCluster(String clusterName, Dictionary options);
  Undefined rejoinCluster(String clusterName, Dictionary options);
  Undefined setPrimaryCluster(String clusterName, Dictionary options);
  Undefined forcePrimaryCluster(String clusterName, Dictionary options);
  Dictionary status(Dictionary options);
  Dictionary describe();
  Undefined setRoutingOption(String option, String value);
  Undefined setRoutingOption(String router, String option, String value);
  Undefined setupAdminAccount(String user, Dictionary options);
  Undefined setupRouterAccount(String user, Dictionary options);
  Dictionary routingOptions(String router);
  Dictionary listRouters(String router);
  Undefined setOption(String option, String value);
  Dictionary options();
#elif DOXYGEN_PY
  str name;  //!< $(CLUSTERSET_GETNAME_BRIEF)
  str get_name();
  None disconnect();
  Cluster create_replica_cluster(InstanceDef instance, str clusterName,
                                 dict options);
  None remove_cluster(str clusterName, dict options);
  None rejoin_cluster(str clusterName, dict options);
  None set_primary_cluster(str clusterName, dict options);
  None force_primary_cluster(str clusterName, dict options);
  dict status(dict options);
  dict describe();
  None set_routing_option(str option, str value);
  None set_routing_option(str router, str option, str value);
  None setup_admin_account(str user, dict options);
  None setup_router_account(str user, dict options);
  dict routing_options(str router);
  dict list_routers(str router);
  None set_option(str option, str value);
  dict options();
#endif

  explicit ClusterSet(const std::shared_ptr<Cluster_set_impl> &clusterset);

  virtual ~ClusterSet();

  std::string class_name() const override { return "ClusterSet"; }

 public:
  void disconnect();

  shcore::Value create_replica_cluster(
      const std::string &instance_def, const std::string &cluster_name,
      shcore::Option_pack_ref<clusterset::Create_replica_cluster_options>
          options = {});

  void remove_cluster(const std::string &cluster_name,
                      const shcore::Option_pack_ref<
                          clusterset::Remove_cluster_options> &options = {});

  void set_primary_cluster(
      const std::string &cluster_name,
      const shcore::Option_pack_ref<clusterset::Set_primary_cluster_options>
          &options = {});

  void force_primary_cluster(
      const std::string &cluster_name,
      const shcore::Option_pack_ref<clusterset::Force_primary_cluster_options>
          &options);

  void rejoin_cluster(
      const std::string &cluster_name,
      const shcore::Option_pack_ref<clusterset::Rejoin_cluster_options>
          &options);

  shcore::Value status(
      const shcore::Option_pack_ref<clusterset::Status_options> &options);
  shcore::Value describe();

  std::shared_ptr<Cluster_set_impl> impl() const { return m_impl; }
  Base_cluster_impl *base_impl() const override { return m_impl.get(); }

  shcore::Value options();

  void set_option(const std::string &option, const shcore::Value &value);

  shcore::Value list_routers(const std::string &router);

 protected:
  void init();

 private:
  std::shared_ptr<Cluster_set_impl> m_impl;

  void assert_valid(const std::string &function_name) const override;

  template <typename TCallback>
  auto execute_with_pool(TCallback &&f, bool interactive = false) const {
    // Init the connection pool
    Scoped_instance_pool scoped_pool(impl()->get_metadata_storage(),
                                     interactive,
                                     impl()->default_admin_credentials());

    return f();
  }
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_CLUSTER_SET_H_
