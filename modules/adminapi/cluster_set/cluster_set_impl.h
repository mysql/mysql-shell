/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_SET_CLUSTER_SET_IMPL_H_
#define MODULES_ADMINAPI_CLUSTER_SET_CLUSTER_SET_IMPL_H_

#include <list>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/cluster_set/api_options.h"
#include "modules/adminapi/common/async_replication_options.h"
#include "modules/adminapi/common/base_cluster_impl.h"
#include "modules/adminapi/common/global_topology_manager.h"
#include "modules/adminapi/common/gtid_validations.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/gtid_utils.h"
#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlsh {
namespace dba {

constexpr const char *k_cluster_set_async_user_name = "mysql_innodb_cs_";
constexpr const int k_cluster_set_master_connect_retry = 3;
constexpr const int k_cluster_set_master_retry_count = 10;

// ClusterSet SSL Mode
constexpr const char k_cluster_set_attribute_ssl_mode[] =
    "opt_clusterSetReplicationSslMode";

class Cluster_set_impl : public Base_cluster_impl,
                         public std::enable_shared_from_this<Cluster_set_impl> {
 public:
  static shcore::Value create_cluster_set(
      Cluster_impl *cluster, const std::string &domain_name,
      const clusterset::Create_cluster_set_options &options);

  Cluster_type get_type() const override {
    return Cluster_type::REPLICATED_CLUSTER;
  }

  std::string get_topology_type() const override { return "pm"; }

  Global_topology_type get_global_topology_type() const {
    return m_topology_type;
  }

  shcore::Value create_replica_cluster(
      const std::string &instance_def, const std::string &cluster_name,
      Recovery_progress_style progress_style,
      const clusterset::Create_replica_cluster_options &options);
  void remove_cluster(const std::string &cluster_name,
                      const clusterset::Remove_cluster_options &options);
  void rejoin_cluster(const std::string &cluster_name,
                      const clusterset::Rejoin_cluster_options &options);
  void set_primary_cluster(
      const std::string &cluster,
      const clusterset::Set_primary_cluster_options &options);
  void force_primary_cluster(
      const std::string &cluster,
      const clusterset::Force_primary_cluster_options &options);
  shcore::Value status(int extended);
  shcore::Value describe();

  std::shared_ptr<Cluster_impl> get_primary_cluster() const;

  Cluster_set_impl(const std::string &domain_name,
                   const std::shared_ptr<Instance> &target_instance,
                   const std::shared_ptr<MetadataStorage> &metadata,
                   Global_topology_type topology_type);

  Cluster_set_impl(const Cluster_set_metadata &csmd,
                   const std::shared_ptr<Instance> &target_instance,
                   const std::shared_ptr<MetadataStorage> &metadata);

  void disconnect() override;

  mysqlsh::dba::Instance *connect_primary();
  bool reconnect_target_if_invalidated();

  mysqlsh::dba::Instance *acquire_primary(
      mysqlshdk::mysql::Lock_mode mode = mysqlshdk::mysql::Lock_mode::NONE,
      const std::string &skip_lock_uuid = "") override;

  void release_primary(mysqlsh::dba::Instance *primary = nullptr) override;

  std::list<std::shared_ptr<Cluster_impl>> connect_all_clusters(
      uint32_t read_timeout, bool skip_primary_cluster,
      std::list<Cluster_id> *inout_unreachable);

  std::shared_ptr<Cluster_impl> get_cluster(const std::string &name,
                                            bool allow_unavailable = false,
                                            bool allow_invalidated = false);

  mysqlshdk::mysql::Auth_options create_cluster_replication_user(
      Instance *cluster_primary, bool dry_run);

  void record_cluster_replication_user(
      Cluster_impl *cluster, const mysqlshdk::mysql::Auth_options &repl_user);

  void drop_cluster_replication_user(Cluster_impl *cluster);

  mysqlshdk::mysql::Auth_options refresh_cluster_replication_user(
      const std::string &user, bool dry_run);

  Member_recovery_method validate_instance_recovery(
      Member_op_action op_action, mysqlshdk::mysql::IInstance *target_instance,
      Member_recovery_method opt_recovery_method, bool gtid_set_is_complete,
      bool interactive);

  std::vector<Cluster_metadata> get_clusters() const;

  Cluster_channel_status get_replication_channel_status(
      const Cluster_impl &cluster) const;

  Cluster_global_status get_cluster_global_status(Cluster_impl *cluster) const;

  Async_replication_options get_clusterset_replication_options() const;

  bool check_gtid_consistency(Cluster_impl *cluster) const;

 protected:
  void _set_option(const std::string &option,
                   const shcore::Value &value) override;

  void _set_instance_option(const std::string &instance_def,
                            const std::string &option,
                            const shcore::Value &value) override;

  void inject_view_change_gtids(mysqlshdk::mysql::IInstance *primary,
                                mysqlshdk::mysql::IInstance *group_instance);

  void check_clusters_available(
      const std::list<std::string> &invalidate_clusters,
      std::list<Cluster_id> *inout_unreachable);
  bool check_cluster_available(const std::string &cluster_name);

  void verify_primary_cluster_not_recoverable();
  void check_gtid_set_most_recent(
      Instance *promoted,
      const std::list<std::shared_ptr<Cluster_impl>> &clusters);

 public:
  void promote_to_primary(Cluster_impl *cluster, bool preserve_channel,
                          bool dry_run);
  void delete_async_channel(Cluster_impl *cluster, bool dry_run);

  void demote_from_primary(Cluster_impl *cluster, Instance *new_primary,
                           const Async_replication_options &repl_options,
                           bool dry_run);

  void update_replica(Cluster_impl *replica, Instance *master,
                      const Async_replication_options &ar_options,
                      bool dry_run);

  void update_replica(Instance *replica, Instance *master,
                      const Async_replication_options &ar_options,
                      bool primary_instance, bool dry_run);

  void remove_replica(Instance *instance, bool dry_run);

  void update_replica_settings(Instance *instance, Instance *new_primary,
                               bool is_primary, bool dry_run);

  void remove_replica_settings(Instance *instance, bool dry_run);

  void ensure_replica_settings(Cluster_impl *replica, bool dry_run);

  shcore::Value list_routers(const std::string &router);
  void set_routing_option(const std::string &router, const std::string &option,
                          const shcore::Value &value);
  shcore::Value routing_options(const std::string &router);

  void record_in_metadata(
      const Cluster_id &seed_cluster_id,
      const clusterset::Create_cluster_set_options &m_options);

  std::shared_ptr<Cluster_impl> get_cluster_object(
      const Cluster_set_member_metadata &cluster_md,
      bool allow_unavailable = false);

  void reconcile_view_change_gtids(
      mysqlshdk::mysql::IInstance *replica,
      const mysqlshdk::mysql::Gtid_set &missing_view_gtids = {});

 private:
  // workaround for possible clang bug:
  //  error: 'mysqlsh::dba::Cluster_set_impl::list_routers' hides overloaded
  //  virtual function
  shcore::Value list_routers(bool) override {
    throw std::logic_error("invalid call");
  }

  Cluster_set_member_metadata get_primary_cluster_metadata() const;

  Cluster_set_member_metadata get_cluster_metadata(
      const Cluster_id &cluster_id) const;

  std::pair<std::string, std::string> get_cluster_repl_account(
      Cluster_impl *cluster) const;

  void ensure_transaction_set_consistent(mysqlshdk::mysql::IInstance *replica,
                                         mysqlshdk::mysql::IInstance *primary,
                                         bool dry_run);

  void primary_instance_did_change(
      const std::shared_ptr<Instance> &new_primary);

  Global_topology_type m_topology_type;
  std::shared_ptr<Cluster_impl> m_primary_cluster;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_SET_CLUSTER_SET_IMPL_H_
