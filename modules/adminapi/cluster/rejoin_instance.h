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

#ifndef MODULES_ADMINAPI_CLUSTER_REJOIN_INSTANCE_H_
#define MODULES_ADMINAPI_CLUSTER_REJOIN_INSTANCE_H_

#include "modules/adminapi/cluster/add_instance.h"
#include "modules/adminapi/cluster/cluster_impl.h"

namespace mysqlsh::dba::cluster {

class Rejoin_instance : private Add_instance {
 public:
  Rejoin_instance() = delete;

  Rejoin_instance(
      Cluster_impl *cluster,
      const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
      const cluster::Rejoin_instance_options &options,
      bool switch_comm_stack = false, bool ignore_cluster_set = false,
      bool within_reboot_cluster = false)
      : Add_instance(cluster, target_instance, options.gr_options,
                     options.clone_options),
        m_options(options),
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

  static constexpr bool supports_undo() noexcept { return false; }

 private:
  bool check_rejoinable();

 private:
  const Rejoin_instance_options m_options;
  bool m_is_switching_comm_stack = false;
  bool m_uuid_mismatch = false;
  bool m_ignore_cluster_set = false;
  std::string m_account_host;
  bool m_within_reboot_cluster = false;
};

}  // namespace mysqlsh::dba::cluster

#endif  // MODULES_ADMINAPI_CLUSTER_REJOIN_INSTANCE_H_
