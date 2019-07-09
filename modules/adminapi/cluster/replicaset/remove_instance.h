/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_REPLICASET_REMOVE_INSTANCE_H_
#define MODULES_ADMINAPI_REPLICASET_REMOVE_INSTANCE_H_

#include <memory>
#include <string>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/command_interface.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/mysql/group_replication.h"

namespace mysqlsh {
namespace dba {

class Remove_instance : public Command_interface {
 public:
  Remove_instance(mysqlshdk::db::Connection_options instance_cnx_opts,
                  const bool interactive,
                  mysqlshdk::utils::nullable<bool> force,
                  const ReplicaSet &replicaset);

  ~Remove_instance() override;

  /**
   * Prepare the remove_instance command for execution.
   * Validates parameters and others, more specifically:
   * - Validate the connection options;
   * - Set user credentials with the ones from the cluster session if missing;
   * - Ensure instance belong to replicaset;
   * - Ensure instance is not the last in the replicaset;
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
   * - Remove the instance from the replicaset (GR group);
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
  const bool m_interactive = false;
  mysqlshdk::utils::nullable<bool> m_force;
  const ReplicaSet &m_replicaset;

  std::string m_instance_address;
  mysqlsh::dba::Instance *m_target_instance = nullptr;
  std::unique_ptr<mysqlsh::dba::Instance> m_target_instance_protocol_upgrade;
  std::string m_address_in_metadata;
  bool m_upgrade_gr_protocol_version = false;
  mysqlshdk::utils::Version m_gr_protocol_version_to_upgrade;

  /**
   * Verify if the instance belongs to the replicaset, otherwise it cannot be
   * removed.
   */
  Instance_metadata ensure_instance_belong_to_replicaset(
      const std::string &address);

  /**
   * Verify if it is the last instance in the replicaset, otherwise it cannot
   * be removed (dissolve must be used instead).
   */
  void ensure_not_last_instance_in_replicaset();

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
   * Auxiliar function used to determine the possible cause of the operation
   * or connection failure, print a user-friendly message to the console, and
   * throw a more appropriate exception.
   *
   * @param err Initial exception that captures the failure that occurred.
   * @param md Metadata record for the instance being removed.
   * @throw the appropriate shcore::Exception after validating the possible
   * cause of the failure.
   */
  void find_failure_cause(const std::exception &err,
                          const Instance_metadata &md);

  /**
   * Auxiliar method to prompt the to use the 'force' option if the instance is
   * not available.
   */
  void prompt_to_force_remove();

  /**
   * Auxiliar method to verify if an upgrade of the protocol will be required
   * after removing the target instance
   */
  bool is_protocol_upgrade_required();
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_REPLICASET_REMOVE_INSTANCE_H_
