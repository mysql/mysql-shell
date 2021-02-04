/*
 * Copyright (c) 2019, 2021, Oracle and/or its affiliates.
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
#include "mysqlshdk/libs/db/connection_options.h"

namespace mysqlsh {
namespace dba {

class Replica_set_impl : public Base_cluster_impl {
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

  shcore::Value describe() {
    check_preconditions("describe");
    throw std::logic_error("not implemented");
  }

  shcore::Value status(int extended);

  void add_instance(const std::string &instance_def,
                    const Async_replication_options &ar_options,
                    const Clone_options &clone_options,
                    const mysqlshdk::null_string &label,
                    Recovery_progress_style progress_style, int sync_timeout,
                    bool interactive, bool dry_run);

  void rejoin_instance(const std::string &instance_def,
                       const Clone_options &clone_options,
                       Recovery_progress_style progress_style, int sync_timeout,
                       bool interactive, bool dry_run);

  void remove_instance(const std::string &instance_def,
                       const mysqlshdk::null_bool &force, int timeout);

  void set_primary_instance(const std::string &instance_def, uint32_t timeout,
                            bool dry_run);
  void force_primary_instance(const std::string &instance_def, uint32_t timeout,
                              bool invalidate_error_instances, bool dry_run);

  shcore::Value options();

  void dissolve(const shcore::Dictionary_t & /*opts*/ = {}) {
    check_preconditions("dissolve");
    throw std::logic_error("not supported");
  }

  shcore::Value check_instance_state(
      const Connection_options & /*instance_def*/) {
    check_preconditions("checkInstanceState");
    throw std::logic_error("not implemented");
  }

  shcore::Value list_routers(bool only_upgrade_required) override;

  void setup_admin_account(const std::string &username, const std::string &host,
                           const Setup_account_options &options) override;

  void setup_router_account(const std::string &username,
                            const std::string &host,
                            const Setup_account_options &options) override;

  std::list<Scoped_instance> connect_all_members(
      uint32_t read_timeout, bool skip_primary,
      std::list<Instance_metadata> *out_unreachable) override;

  mysqlsh::dba::Instance *acquire_primary(
      mysqlshdk::mysql::Lock_mode mode = mysqlshdk::mysql::Lock_mode::NONE,
      const std::string &skip_lock_uuid = "") override;

  void release_primary(mysqlsh::dba::Instance *primary = nullptr) override;

  mysqlshdk::mysql::Auth_options create_replication_user(
      mysqlshdk::mysql::IInstance *slave, bool dry_run);

  mysqlshdk::mysql::Auth_options refresh_replication_user(
      mysqlshdk::mysql::IInstance *slave, bool dry_run);

  void drop_replication_user(mysqlshdk::mysql::IInstance *slave);

 private:
  void _set_option(const std::string &option,
                   const shcore::Value &value) override;

  void _set_instance_option(const std::string &instance_def,
                            const std::string &option,
                            const shcore::Value &value) override;

  void create(const std::string &instance_label, bool dry_run);

  void adopt(Global_topology_manager *topology,
             const std::string &instance_label, bool dry_run);

  void validate_add_instance(Global_topology_manager *topology,
                             mysqlshdk::mysql::IInstance *master,
                             Instance *target_instance,
                             const Async_replication_options &ar_options,
                             Clone_options *clone_options, bool interactive);

  void validate_rejoin_instance(Global_topology_manager *topology_mng,
                                Instance *target, Clone_options *clone_options,
                                Instance_metadata *out_instance_md,
                                bool interactive);

  void validate_remove_instance(Global_topology_manager *topology,
                                mysqlshdk::mysql::IInstance *master,
                                const std::string &target_address,
                                Instance *target, bool force,
                                Instance_metadata *out_instance_md,
                                bool *out_repl_working);

  Instance_id manage_instance(Instance *instance,
                              const std::string &instance_label,
                              Instance_id master_id, bool is_primary);

  void do_set_primary_instance(Instance *master, Instance *new_master,
                               const std::list<Scoped_instance> &instances,
                               const Async_replication_options &ar_options,
                               bool dry_run);

  std::shared_ptr<Global_topology_manager> setup_topology_manager(
      topology::Server_global_topology **out_topology = nullptr,
      bool deep = false);

  Cluster_metadata get_metadata() const;
  std::vector<Instance_metadata> get_instances_from_metadata() const;

  const topology::Server *check_target_member(
      topology::Server_global_topology *topology,
      const std::string &instance_def);

  void check_replication_applier_errors(
      topology::Server_global_topology *srv_topology,
      std::list<Scoped_instance> *out_online_instances,
      bool invalidate_error_instances,
      std::vector<Instance_metadata> *out_instances_md,
      std::list<Instance_id> *out_invalidate_ids) const;

  void primary_instance_did_change(
      const std::shared_ptr<Instance> &new_primary = {});

  void invalidate_handle();

  void ensure_compatible_donor(const std::string &instance_def,
                               mysqlshdk::mysql::IInstance *recipient);

  std::string pick_clone_donor(mysqlshdk::mysql::IInstance *recipient);

  void revert_topology_changes(mysqlshdk::mysql::IInstance *target_server,
                               bool remove_user, bool dry_run);

  void refresh_target_connections(mysqlshdk::mysql::IInstance *recipient);

  void handle_clone(const std::shared_ptr<mysqlsh::dba::Instance> &recipient,
                    const Clone_options &clone_options,
                    const Async_replication_options &ar_options,
                    const Recovery_progress_style &progress_style,
                    int sync_timeout, bool dry_run);

  Member_recovery_method validate_instance_recovery(
      Member_op_action op_action, mysqlshdk::mysql::IInstance *donor_instance,
      mysqlshdk::mysql::IInstance *target_instance,
      Member_recovery_method opt_recovery_method, bool gtid_set_is_complete,
      bool interactive);

  shcore::Dictionary_t get_topology_options();

  Global_topology_type m_topology_type;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_REPLICA_SET_REPLICA_SET_IMPL_H_
