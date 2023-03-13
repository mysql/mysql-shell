/*
 * Copyright (c) 2020, 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_ADD_INSTANCE_H_
#define MODULES_ADMINAPI_CLUSTER_ADD_INSTANCE_H_

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/clone_options.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/undo.h"

namespace mysqlsh::dba::cluster {

class Add_instance {
 public:
  Add_instance() = delete;

  Add_instance(Cluster_impl *cluster,
               const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
               const cluster::Add_instance_options &options)
      : m_cluster_impl(cluster),
        m_target_instance(target_instance),
        m_gr_opts(options.gr_options),
        m_clone_opts(options.clone_options),
        m_options(options) {
    assert(m_cluster_impl);
  };

  Add_instance(Cluster_impl *cluster,
               const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
               const Group_replication_options &gr_options,
               const Clone_options &clone_options)
      : m_cluster_impl(cluster),
        m_target_instance(target_instance),
        m_gr_opts(gr_options),
        m_clone_opts(clone_options) {
    assert(m_cluster_impl);
  }

  Add_instance(const Add_instance &) = delete;
  Add_instance(Add_instance &&) = delete;
  Add_instance &operator=(const Add_instance &) = delete;
  Add_instance &operator=(Add_instance &&) = delete;

  ~Add_instance() = default;

 protected:
  void do_run();
  void prepare(checks::Check_type check_type,
               bool *is_switching_comm_stack = nullptr);

 private:
  void check_cluster_members_limit() const;
  void check_and_resolve_instance_configuration(
      checks::Check_type check_type, bool is_switching_comm_stack = false);
  void resolve_local_address(checks::Check_type check_type,
                             Group_replication_options *gr_options,
                             const Group_replication_options &user_gr_options);
  void store_local_replication_account() const;
  void store_cloned_replication_account() const;
  void restore_group_replication_account() const;
  void refresh_target_connections() const;

 protected:
  shcore::Scoped_callback_list m_undo_list;
  Undo_tracker m_undo_tracker;
  Cluster_impl *m_cluster_impl = nullptr;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  std::shared_ptr<mysqlsh::dba::Instance> m_primary_instance;
  Group_replication_options m_gr_opts;
  Clone_options m_clone_opts;
  bool m_is_autorejoining = false;
  std::string m_comm_stack;
  std::string m_auth_cert_subject;

 private:
  Add_instance_options m_options;
  bool m_already_member = false;
  std::string m_account_host;
};

}  // namespace mysqlsh::dba::cluster

#endif  // MODULES_ADMINAPI_CLUSTER_ADD_INSTANCE_H_
