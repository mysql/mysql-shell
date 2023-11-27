/*
 * Copyright (c) 2019, 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_REPLICA_SET_REPLICA_SET_IMPL_H_
#define MODULES_ADMINAPI_REPLICA_SET_REPLICA_SET_IMPL_H_

#include <list>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "modules/adminapi/common/api_options.h"
#include "modules/adminapi/common/async_replication_options.h"
#include "modules/adminapi/common/base_cluster_impl.h"
#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/global_topology_manager.h"
#include "modules/adminapi/common/gtid_validations.h"
#include "modules/adminapi/dba/api_options.h"
#include "modules/adminapi/replica_set/api_options.h"
#include "mysqlshdk/libs/db/connection_options.h"

namespace mysqlsh {
namespace dba {

inline constexpr const char *k_async_cluster_user_name = "mysql_innodb_rs_";

class Replica_set_impl final : public Base_cluster_impl {
 public:
  Replica_set_impl(const std::string &cluster_name,
                   const std::shared_ptr<Instance> &target,
                   const std::shared_ptr<MetadataStorage> &metadata_storage,
                   Global_topology_type topology_type);

  Replica_set_impl(const Cluster_metadata &cm,
                   const std::shared_ptr<Instance> &target,
                   const std::shared_ptr<MetadataStorage> &metadata_storage);

  Cluster_type get_type() const override {
    return Cluster_type::ASYNC_REPLICATION;
  }

  std::string get_topology_type() const override { return "pm"; }

  Global_topology_type get_async_topology_type() const {
    return m_topology_type;
  }

 public:
  static std::shared_ptr<Replica_set_impl> create(
      const std::string &full_cluster_name, Global_topology_type topology_type,
      const std::shared_ptr<Instance> &target_server,
      const Create_replicaset_options &options);

  static std::vector<std::tuple<mysqlshdk::mysql::Slave_host, std::string>>
  get_valid_slaves(
      const mysqlshdk::mysql::IInstance &instance,
      std::vector<std::tuple<mysqlshdk::mysql::Slave_host, std::string>>
          *ghost_slaves);

  shcore::Value describe();

  shcore::Value status(int extended);

  void add_instance(const std::string &instance_def,
                    const Async_replication_options &ar_options,
                    const Clone_options &clone_options,
                    const std::string &label,
                    const std::string &auth_cert_subject,
                    Recovery_progress_style progress_style, int sync_timeout,
                    bool interactive, bool dry_run);

  void rejoin_instance(const std::string &instance_def,
                       const Clone_options &clone_options,
                       Recovery_progress_style progress_style, int sync_timeout,
                       bool interactive, bool dry_run);

  void remove_instance(const std::string &instance_def,
                       std::optional<bool> force, int timeout);

  void set_primary_instance(const std::string &instance_def, uint32_t timeout,
                            bool dry_run);
  void force_primary_instance(const std::string &instance_def, uint32_t timeout,
                              bool invalidate_error_instances, bool dry_run);

  void dissolve(const replicaset::Dissolve_options &options);
  void rescan(const replicaset::Rescan_options &options);

  shcore::Value options();

  shcore::Value check_instance_state(
      const Connection_options & /*instance_def*/) {
    check_preconditions("checkInstanceState");
    throw std::logic_error("not implemented");
  }

  shcore::Value list_routers(bool only_upgrade_required) override;

  void remove_router_metadata(const std::string &router) override;

  void setup_admin_account(const std::string &username, const std::string &host,
                           const Setup_account_options &options) override;

  void setup_router_account(const std::string &username,
                            const std::string &host,
                            const Setup_account_options &options) override;

  std::list<std::shared_ptr<Instance>> connect_all_members(
      uint32_t read_timeout, bool skip_primary,
      std::list<Instance_metadata> *out_unreachable, bool silent = false);

  std::tuple<mysqlsh::dba::Instance *, mysqlshdk::mysql::Lock_scoped>
  acquire_primary_locked(mysqlshdk::mysql::Lock_mode mode,
                         std::string_view skip_lock_uuid = "") override;

  mysqlsh::dba::Instance *acquire_primary(
      bool primary_required = true, bool check_primary_status = false) override;

  Cluster_metadata get_metadata() const;

  void release_primary() override;

  std::pair<mysqlshdk::mysql::Auth_options, std::string>
  refresh_replication_user(mysqlshdk::mysql::IInstance *slave, bool dry_run);

  void drop_replication_user(const std::string &server_uuid,
                             mysqlshdk::mysql::IInstance *slave = nullptr);

  void drop_all_replication_users();

  std::pair<mysqlshdk::mysql::Auth_options, std::string>
  create_replication_user(mysqlshdk::mysql::IInstance *slave,
                          std::string_view auth_cert_subject, bool dry_run,
                          mysqlshdk::mysql::IInstance *master = nullptr);

  std::vector<Instance_metadata> get_instances_from_metadata() const override;

  void ensure_compatible_clone_donor(
      const mysqlshdk::mysql::IInstance &donor,
      const mysqlshdk::mysql::IInstance &recipient) override;

  std::shared_ptr<Global_topology_manager> get_topology_manager(
      topology::Server_global_topology **out_topology = nullptr,
      bool deep = false);

  void read_replication_options(std::string_view instance_uuid,
                                Async_replication_options *ar_options,
                                bool *has_null_options);

  // Lock methods

  [[nodiscard]] mysqlshdk::mysql::Lock_scoped get_lock_shared(
      std::chrono::seconds timeout = {}) override;

  [[nodiscard]] mysqlshdk::mysql::Lock_scoped get_lock_exclusive(
      std::chrono::seconds timeout = {}) override;

 private:
  void _set_option(const std::string &option,
                   const shcore::Value &value) override;

  void _set_instance_option(const std::string &instance_def,
                            const std::string &option,
                            const shcore::Value &value) override;

  void create(const Create_replicaset_options &options, bool dry_run);

  void adopt(Global_topology_manager *topology,
             const Create_replicaset_options &options, bool dry_run);

  void validate_add_instance(Global_topology_manager *topology,
                             mysqlshdk::mysql::IInstance *master,
                             Instance *target_instance,
                             Async_replication_options *ar_options,
                             Clone_options *clone_options,
                             const std::string &cert_subject, bool interactive);

  topology::Node_status validate_rejoin_instance(
      Global_topology_manager *topology_mng, Instance *target,
      Clone_options *clone_options, Instance_metadata *out_instance_md,
      bool interactive);

  void validate_remove_instance(Global_topology_manager *topology,
                                mysqlshdk::mysql::IInstance *master,
                                const std::string &target_address,
                                Instance *target, bool force,
                                Instance_metadata *out_instance_md,
                                bool *out_repl_working);

  Instance_id manage_instance(
      Instance *instance, const std::pair<std::string, std::string> &repl_user,
      const std::string &instance_label, Instance_id master_id,
      bool is_primary);

  void do_set_primary_instance(
      Instance *master, Instance *new_master,
      const std::list<std::shared_ptr<Instance>> &instances,
      const Async_replication_options &master_ar_options, bool dry_run);

  const topology::Server *check_target_member(
      topology::Server_global_topology *topology,
      const std::string &instance_def);

  void check_replication_applier_errors(
      topology::Server_global_topology *srv_topology,
      std::list<std::shared_ptr<Instance>> *out_online_instances,
      bool invalidate_error_instances,
      std::vector<Instance_metadata> *out_instances_md,
      std::list<Instance_id> *out_invalidate_ids,
      std::list<std::string> *out_ids_applier_off) const;

  void primary_instance_did_change(
      const std::shared_ptr<Instance> &new_primary = {});

  void invalidate_handle();

  void ensure_metadata_has_server_uuid(const mysqlsh::dba::Instance &instance);

  std::string pick_clone_donor(mysqlshdk::mysql::IInstance *recipient);

  void revert_topology_changes(mysqlshdk::mysql::IInstance *target_server,
                               bool remove_user, bool dry_run);

  Member_recovery_method validate_instance_recovery(
      Member_op_action op_action,
      const mysqlshdk::mysql::IInstance &donor_instance,
      const mysqlshdk::mysql::IInstance &target_instance,
      Member_recovery_method opt_recovery_method, bool gtid_set_is_complete,
      bool interactive);

  shcore::Dictionary_t get_topology_options();

  void update_replication_allowed_host(const std::string &host);

  void check_preconditions_and_primary_availability(
      const std::string &function_name,
      bool throw_if_primary_unavailable = true);

  void read_replication_options(const mysqlshdk::mysql::IInstance *instance,
                                Async_replication_options *ar_options,
                                bool *has_null_options = nullptr);
  void cleanup_replication_options(const mysqlshdk::mysql::IInstance &instance);

  [[nodiscard]] mysqlshdk::mysql::Lock_scoped get_lock(
      mysqlshdk::mysql::Lock_mode mode, std::chrono::seconds timeout = {});

  Global_topology_type m_topology_type;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_REPLICA_SET_REPLICA_SET_IMPL_H_
