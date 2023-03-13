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

#ifndef MODULES_ADMINAPI_DBA_REBOOT_CLUSTER_FROM_COMPLETE_OUTAGE_H_
#define MODULES_ADMINAPI_DBA_REBOOT_CLUSTER_FROM_COMPLETE_OUTAGE_H_

#include "modules/adminapi/common/undo.h"
#include "modules/adminapi/mod_dba.h"

namespace mysqlsh::dba {

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
   * Retrieves info regarding the clusters ClusterSet (if any)
   */
  void retrieve_cs_info(Cluster_impl *cluster);

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
      const std::vector<std::shared_ptr<Instance>> &instances, bool force,
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
  void reboot_seed();

 protected:
  shcore::Scoped_callback_list m_undo_list;
  Undo_tracker m_undo_tracker;

 private:
  Dba *m_dba;
  std::shared_ptr<Cluster> m_cluster;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  Reboot_cluster_options m_options;
  bool m_is_autorejoining = false;
  bool m_already_member = false;

  struct Cluster_set_info {
    bool is_member;
    bool is_primary;
    bool is_primary_invalidated;
    bool removed_from_set;
    Cluster_global_status primary_status;
    bool is_init = false;
  } m_cs_info = {};
};

}  // namespace mysqlsh::dba

#endif  // MODULES_ADMINAPI_DBA_REBOOT_CLUSTER_FROM_COMPLETE_OUTAGE_H_
