/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_SET_CREATE_REPLICA_CLUSTER_H_
#define MODULES_ADMINAPI_CLUSTER_SET_CREATE_REPLICA_CLUSTER_H_

#include <memory>
#include <string>

#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/dba/create_cluster.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/command_interface.h"

namespace mysqlsh {
namespace dba {
namespace clusterset {

// # of seconds to wait until clone starts
constexpr const int k_clone_start_timeout = 30;

class Create_replica_cluster : public Command_interface {
 public:
  Create_replica_cluster(
      Cluster_set_impl *cluster_set, mysqlsh::dba::Instance *primary_instance,
      const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
      const std::string &cluster_name, Recovery_progress_style progress_style,
      const Create_replica_cluster_options &options);

  ~Create_replica_cluster() override;

  /**
   * Prepare the Create_replica_cluster command for execution.
   */
  void prepare() override;

  /**
   * Execute the Create_replica_cluster command.
   *
   * @return shcore::Value with the new ClusterSet object created.
   */
  shcore::Value execute() override;

 private:
  shcore::Option_pack_ref<mysqlsh::dba::Create_cluster_options>
  prepare_create_cluster_options();

  void ensure_compatible_clone_donor(const std::string &instance_def);

  void handle_clone(const Async_replication_options &ar_options,
                    const std::string &repl_account_host, bool dry_run);

  std::shared_ptr<Cluster_impl> create_cluster_object(
      const mysqlshdk::mysql::Auth_options &repl_credentials,
      const std::string &repl_account_host);

  /**
   * Recreate the recovery account at the target instance on which the Replica
   * Cluster is being created
   *
   * Required for when using the 'MySQL' communication stack to ensure the
   * account is available on all ClusterSet members
   *
   * @param cluster       Cluster Object
   * @param recovery_user Username for the recreated account
   * @param recovery_host Hostname for the recreated account
   */
  void recreate_recovery_account(const std::shared_ptr<Cluster_impl> cluster,
                                 std::string *recovery_user = nullptr,
                                 std::string *recovery_host = nullptr);

 private:
  Cluster_set_impl *m_cluster_set = nullptr;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  mysqlsh::dba::Instance *m_primary_instance = nullptr;
  const std::string &m_cluster_name;
  Recovery_progress_style m_progress_style;
  Create_replica_cluster_options m_options;
  Cluster_ssl_mode m_ssl_mode = Cluster_ssl_mode::NONE;

  std::unique_ptr<Create_cluster> m_op_create_cluster;
};

}  // namespace clusterset
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_SET_CREATE_REPLICA_CLUSTER_H_
