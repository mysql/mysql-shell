/*
 * Copyright (c) 2018, 2022, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_REMOVE_INSTANCE_H_
#define MODULES_ADMINAPI_CLUSTER_REMOVE_INSTANCE_H_

#include <memory>
#include <string>

#include "adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/command_interface.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/mysql/group_replication.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

class Remove_instance : public Command_interface {
 public:
  Remove_instance(const mysqlshdk::db::Connection_options &instance_cnx_opts,
                  const Remove_instance_options &options,
                  Cluster_impl *cluster);

  ~Remove_instance() override;

  /**
   * Prepare the remove_instance command for execution.
   * Validates parameters and others, more specifically:
   * - Validate the connection options;
   * - Set user credentials with the ones from the cluster session if missing;
   * - Ensure instance belong to cluster;
   * - Ensure instance is not the last in the cluster;
   * - Connect to the target instance and handle failures connecting to it:
   *   - if interactive (and force not used) ask the user to continue of not
   *   - if force = true continue execution (only metadata removed)
   *   - if force = false then abort operation
   * - Verify user privileges to execute operation;
   */
  void prepare() override;

  /**
   * Execute the remove_instance command.
   * More specifically:
   * - Remove the instance from the metadata;
   * - Remove the instance from the cluster (GR group);
   * - Update remaining members of the cluster;
   *
   * @return an empty shcore::Value.
   */
  shcore::Value execute() override;

  /**
   * Rollback the command.
   *
   * NOTE: Not currently used (does nothing).
   */
  void rollback() override;

  /**
   * Finalize the command execution.
   * More specifically:
   * - Close the instance connection (if previously established);
   */
  void finish() override;

 private:
  mysqlshdk::db::Connection_options m_instance_cnx_opts;
  Remove_instance_options m_options;
  Cluster_impl *m_cluster = nullptr;

  std::string m_instance_address;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  std::string m_address_in_metadata;
  bool m_skip_sync = false;
  std::string m_instance_uuid;
  uint32_t m_instance_id = 0;

  void validate_metadata_for_address(const std::string &address,
                                     Instance_metadata *out_metadata);
  Instance_metadata lookup_metadata_for_uuid(const std::string &uuid);

  /**
   * Verify if it is the last instance in the cluster, otherwise it cannot
   * be removed (dissolve must be used instead).
   */
  void ensure_not_last_instance_in_cluster(const std::string &removed_uuid);

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
  Instance_metadata remove_instance_metadata();

  /**
   * Revert the removal of the instance from the metadata.
   *
   * Re-insert the instance to the metadata using the saved state from the
   * remove_instance_metadata() function.
   *
   * @param instance_def Object with the instance state (definition) to
   * re-insert into the metadata.
   */
  void undo_remove_instance_metadata(const Instance_metadata &instance_def);

  /**
   * Helper method to prompt the to use the 'force' option if the instance is
   * not available.
   */
  bool prompt_to_force_remove();

  void check_protocol_upgrade_possible();

  /**
   * Look for a leftover recovery account (not used by any member)
   * to remove it from the cluster
   */
  void cleanup_leftover_recovery_account();
};

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_REMOVE_INSTANCE_H_
