/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_REJOIN_REPLICA_INSTANCE_H_
#define MODULES_ADMINAPI_CLUSTER_REJOIN_REPLICA_INSTANCE_H_

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/undo.h"

namespace mysqlsh::dba::cluster {

class Rejoin_replica_instance {
 public:
  Rejoin_replica_instance() = delete;

  Rejoin_replica_instance(
      Cluster_impl *cluster,
      const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
      const cluster::Rejoin_instance_options &options)
      : m_cluster_impl(cluster),
        m_target_instance(target_instance),
        m_options(options) {
    assert(m_cluster_impl);
  }

  Rejoin_replica_instance(const Rejoin_replica_instance &) = delete;
  Rejoin_replica_instance(Rejoin_replica_instance &&) = delete;
  Rejoin_replica_instance &operator=(const Rejoin_replica_instance &) = delete;
  Rejoin_replica_instance &operator=(Rejoin_replica_instance &&) = delete;

  ~Rejoin_replica_instance() = default;

 protected:
  void do_run();

  static constexpr bool supports_undo() noexcept { return true; }
  void do_undo();

 private:
  bool check_rejoinable();
  void prepare();
  Member_recovery_method validate_instance_recovery();
  void validate_replication_channels();
  std::shared_ptr<Instance> get_default_donor_instance();

 private:
  Cluster_impl *m_cluster_impl = nullptr;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  std::shared_ptr<mysqlsh::dba::Instance> m_donor_instance;
  Rejoin_instance_options m_options;
  std::string m_account_host;
  std::string m_target_read_replica_address;
  std::string m_auth_cert_subject;
  Replication_sources m_replication_sources;
  Undo_tracker m_undo_tracker;
};

}  // namespace mysqlsh::dba::cluster

#endif  // MODULES_ADMINAPI_CLUSTER_REJOIN_REPLICA_INSTANCE_H_
