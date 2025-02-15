/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_REMOVE_REPLICA_INSTANCE_H_
#define MODULES_ADMINAPI_CLUSTER_REMOVE_REPLICA_INSTANCE_H_

#include <memory>
#include <string>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/replication_account.h"
#include "modules/adminapi/common/undo.h"

namespace mysqlsh::dba::cluster {

class Remove_replica_instance {
 public:
  Remove_replica_instance() = delete;

  Remove_replica_instance(
      Cluster_impl *cluster,
      std::shared_ptr<mysqlsh::dba::Instance> target_instance,
      cluster::Remove_instance_options options) noexcept
      : m_cluster_impl(cluster),
        m_target_instance(std::move(target_instance)),
        m_options(std::move(options)),
        m_repl_account_mng{*m_cluster_impl} {
    assert(cluster);
  };

  Remove_replica_instance(const Remove_replica_instance &) = delete;
  Remove_replica_instance(Remove_replica_instance &&) = delete;
  Remove_replica_instance &operator=(const Remove_replica_instance &) = delete;
  Remove_replica_instance &operator=(Remove_replica_instance &&) = delete;

  ~Remove_replica_instance() = default;

 protected:
  void do_run();

  static constexpr bool supports_undo() noexcept { return true; }
  void do_undo();

 private:
  Cluster_impl *const m_cluster_impl = nullptr;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  cluster::Remove_instance_options m_options;
  std::string m_target_read_replica_address;
  Undo_tracker m_undo_tracker;
  Replication_account m_repl_account_mng;
};

}  // namespace mysqlsh::dba::cluster

#endif  // MODULES_ADMINAPI_CLUSTER_REMOVE_REPLICA_INSTANCE_H_
