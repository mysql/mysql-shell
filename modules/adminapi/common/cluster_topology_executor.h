/*
 * Copyright (c) 2022, 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_CLUSTER_TOPOLOGY_EXECUTOR_H_
#define MODULES_ADMINAPI_COMMON_CLUSTER_TOPOLOGY_EXECUTOR_H_

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/mod_dba.h"
#include "modules/adminapi/replica_set/api_options.h"
#include "modules/adminapi/replica_set/replica_set_impl.h"
#include "mysqlshdk/include/shellcore/console.h"

namespace mysqlsh::dba {

// Policy-based design pattern:
//
//  - Compile-time strategy pattern variant using template metaprogramming
//  - The "host" class (Cluster_topology_executor) takes a type parameter input
//    when instantiated which then implements the desired interface
//  - Defines a common method to execute the command's interface: run()

template <typename TCluster_topology_op>
class Cluster_topology_executor : private TCluster_topology_op {
  using TCluster_topology_op::do_run;
  using TCluster_topology_op::m_undo_list;

 public:
  template <typename... Args>
  explicit Cluster_topology_executor(Args &&... args)
      : TCluster_topology_op(std::forward<Args>(args)...) {}

  template <typename... TArgs>
  auto run(TArgs &&... args) {
    try {
      shcore::Scoped_callback undo([&]() { m_undo_list.cancel(); });

      return do_run(std::forward<TArgs>(args)...);
    } catch (...) {
      // Handle exceptions
      auto console = mysqlsh::current_console();

      console->print_error(format_active_exception());

      if (!m_undo_list.empty()) {
        console->print_note("Reverting changes...");

        m_undo_list.call();

        console->print_info();
        console->print_info("Changes successfully reverted.");
      }

      throw;
    }
  }
};

// Cluster ops

namespace cluster {
class Add_instance {
 public:
  Add_instance() = delete;

  explicit Add_instance(
      Cluster_impl *cluster,
      const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
      const cluster::Add_instance_options &options)
      : m_cluster_impl(cluster),
        m_target_instance(target_instance),
        m_options(options) {
    assert(m_cluster_impl);

    // Init progress_style
    if (m_options.wait_recovery == 0) {
      m_progress_style = Recovery_progress_style::NOWAIT;
    } else if (m_options.wait_recovery == 1) {
      m_progress_style = Recovery_progress_style::NOINFO;
    } else if (m_options.wait_recovery == 2) {
      m_progress_style = Recovery_progress_style::TEXTUAL;
    } else {
      m_progress_style = Recovery_progress_style::PROGRESSBAR;
    }
  }

  Add_instance(const Add_instance &) = delete;
  Add_instance(Add_instance &&) = delete;
  Add_instance &operator=(const Add_instance &) = delete;
  Add_instance &operator=(Add_instance &&) = delete;

  ~Add_instance() = default;

 protected:
  void do_run();

 private:
  void prepare();
  void check_cluster_members_limit() const;
  Member_recovery_method check_recovery_method(bool clone_disabled);
  void resolve_local_address(Group_replication_options *gr_options,
                             const Group_replication_options &user_gr_options);

  void check_and_resolve_instance_configuration();

  /*
   * Handle the loading/unloading of the clone plugin on the target cluster
   * and the target instance
   */

  void handle_clone_plugin_state(bool enable_clone) const;
  bool create_replication_user();
  void clean_replication_user();
  void wait_recovery(const std::string &join_begin_time,
                     Recovery_progress_style progress_style) const;
  void refresh_target_connections() const;
  void store_local_replication_account() const;
  void store_cloned_replication_account() const;
  void restore_group_replication_account() const;

 protected:
  shcore::Scoped_callback_list m_undo_list;

 private:
  Cluster_impl *m_cluster_impl = nullptr;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  std::shared_ptr<mysqlsh::dba::Instance> m_primary_instance;
  Add_instance_options m_options;
  Recovery_progress_style m_progress_style = Recovery_progress_style::TEXTUAL;
  bool m_already_member = false;
  std::string m_comm_stack;
  std::string m_account_host;
};

class Remove_instance {
 public:
  Remove_instance() = delete;

  explicit Remove_instance(Cluster_impl *cluster,
                           const Connection_options &instance_def,
                           const cluster::Remove_instance_options &options)
      : m_cluster_impl(cluster),
        m_target_cnx_opts(instance_def),
        m_options(options) {
    assert(m_cluster_impl);

    m_target_address =
        m_target_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());
  }

  Remove_instance(const Remove_instance &) = delete;
  Remove_instance(Remove_instance &&) = delete;
  Remove_instance &operator=(const Remove_instance &) = delete;
  Remove_instance &operator=(Remove_instance &&) = delete;

  ~Remove_instance() = default;

 protected:
  void do_run();

 private:
  void prepare();
  void validate_metadata_for_address(Instance_metadata *out_metadata) const;

  /**
   * Verify if it is the last instance in the cluster, otherwise it cannot
   * be removed (dissolve must be used instead).
   */
  void ensure_not_last_instance_in_cluster(
      const std::string &removed_uuid) const;

  /**
   * Remove the target instance from metadata.
   *
   * This functions save the instance details (Instance_metadata) at the
   * begining to be able to revert the operation if needed (add it back to the
   * metadata).
   *
   * The operation is performed in a transaction, meaning that the removal is
   * completely performed or nothing is removed if some error occur during the
   * operation.
   *
   * @return an Instance_metadata object with the state information of the
   * removed instance, in order enable this operation to be reverted using this
   * data if needed.
   */
  Instance_metadata remove_instance_metadata() const;

  /**
   * Revert the removal of the instance from the metadata.
   *
   * Re-insert the instance to the metadata using the saved state from the
   * remove_instance_metadata() function.
   *
   * @param instance_def Object with the instance state (definition) to
   * re-insert into the metadata.
   */
  void undo_remove_instance_metadata(
      const Instance_metadata &instance_def) const;

  /**
   * Helper method to prompt the to use the 'force' option if the instance is
   * not available.
   */
  bool prompt_to_force_remove() const;

  void check_protocol_upgrade_possible() const;

  /**
   * Look for a leftover recovery account (not used by any member)
   * to remove it from the cluster
   */
  void cleanup_leftover_recovery_account() const;

 protected:
  shcore::Scoped_callback_list m_undo_list;

 private:
  Cluster_impl *m_cluster_impl = nullptr;
  std::string m_target_address;
  std::string m_instance_uuid;
  std::string m_address_in_metadata;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  mysqlshdk::db::Connection_options m_target_cnx_opts;
  cluster::Remove_instance_options m_options;
  uint32_t m_instance_id = 0;
  bool m_skip_sync = false;
};

class Rejoin_instance {
 public:
  Rejoin_instance() = delete;

  explicit Rejoin_instance(
      Cluster_impl *cluster,
      const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
      cluster::Rejoin_instance_options options, bool switch_comm_stack = false,
      bool ignore_cluster_set = false, bool within_reboot_cluster = false)
      : m_cluster_impl(cluster),
        m_target_instance(target_instance),
        m_options{std::move(options)},
        m_is_switching_comm_stack(switch_comm_stack),
        m_ignore_cluster_set(ignore_cluster_set),
        m_within_reboot_cluster(within_reboot_cluster) {
    assert(m_cluster_impl);
  }

  Rejoin_instance(const Rejoin_instance &) = delete;
  Rejoin_instance(Rejoin_instance &&) = delete;
  Rejoin_instance &operator=(const Rejoin_instance &) = delete;
  Rejoin_instance &operator=(Rejoin_instance &&) = delete;

  ~Rejoin_instance() = default;

 protected:
  void do_run();

 private:
  bool check_rejoinable();
  void resolve_local_address(Group_replication_options *gr_options,
                             const Group_replication_options &user_gr_options);

  void check_and_resolve_instance_configuration();
  bool create_replication_user();

 protected:
  shcore::Scoped_callback_list m_undo_list;

 private:
  Cluster_impl *m_cluster_impl = nullptr;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  std::shared_ptr<mysqlsh::dba::Instance> m_primary_instance;
  Rejoin_instance_options m_options;
  bool m_is_switching_comm_stack = false;
  bool m_is_autorejoining = false;
  bool m_uuid_mismatch = false;
  std::string m_comm_stack;
  bool m_ignore_cluster_set = false;
  std::string m_account_host;
  bool m_within_reboot_cluster = false;
  std::string m_auth_cert_subject;
};

}  // namespace cluster

struct Cluster_set_info {
  bool is_member;
  bool is_primary;
  bool is_primary_invalidated;
  bool removed_from_set;
  Cluster_global_status primary_status;
};

class Reboot_cluster_from_complete_outage {
 public:
  Reboot_cluster_from_complete_outage() = delete;

  explicit Reboot_cluster_from_complete_outage(
      Dba *dba, std::shared_ptr<Cluster> cluster,
      const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
      const Reboot_cluster_options &options)
      : m_dba(dba),
        m_cluster(cluster),
        m_target_instance(target_instance),
        m_options(options) {
    assert(dba);
    assert(cluster);
  }

  Reboot_cluster_from_complete_outage(
      const Reboot_cluster_from_complete_outage &) = delete;
  Reboot_cluster_from_complete_outage(Reboot_cluster_from_complete_outage &&) =
      delete;
  Reboot_cluster_from_complete_outage &operator=(
      const Reboot_cluster_from_complete_outage &) = delete;
  Reboot_cluster_from_complete_outage &operator=(
      Reboot_cluster_from_complete_outage &&) = delete;

  ~Reboot_cluster_from_complete_outage() = default;

 protected:
  std::shared_ptr<Cluster> do_run();

 private:
  /*
   * Retrieves all instances of the cluster.
   */
  std::vector<std::shared_ptr<Instance>> retrieve_instances(
      std::vector<Instance_metadata> *instances_unreachable);

  /*
   * It verifies which of the online instances of the cluster has the
   * GTID superset. If also checks the command options such as picking an
   * explicit instance and the force option.
   */
  std::shared_ptr<Instance> pick_best_instance_gtid(
      const std::vector<std::shared_ptr<Instance>> &instances,
      bool is_cluster_set_member, bool force,
      std::string_view intended_instance);

  void check_instance_configuration();

  void resolve_local_address(Group_replication_options *gr_options);

  /**
   * Validate the use of IPv6 addresses on the localAddress of the
   * target instance and check if the target instance supports usage of
   * IPv6 on the localAddress values being used on the cluster instances.
   */
  void validate_local_address_ip_compatibility(
      const std::string &local_address) const;

  /*
   * Reboots the seed instance
   */
  void reboot_seed(const Cluster_set_info &cs_info);

 protected:
  shcore::Scoped_callback_list m_undo_list;

 private:
  Dba *m_dba;
  std::shared_ptr<Cluster> m_cluster;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  Reboot_cluster_options m_options;
  bool m_is_autorejoining = false;
  bool m_already_member = false;
};

}  // namespace mysqlsh::dba

#endif  // MODULES_ADMINAPI_COMMON_CLUSTER_TOPOLOGY_EXECUTOR_H_
