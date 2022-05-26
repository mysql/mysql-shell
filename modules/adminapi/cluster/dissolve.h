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

#ifndef MODULES_ADMINAPI_CLUSTER_DISSOLVE_H_
#define MODULES_ADMINAPI_CLUSTER_DISSOLVE_H_

#include <memory>
#include <string>
#include <vector>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/command_interface.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/utils/nullable.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

class Dissolve : public Command_interface {
 public:
  Dissolve(const bool interactive, mysqlshdk::utils::nullable<bool> force,
           Cluster_impl *cluster);

  ~Dissolve() override;

  /**
   * Prepare the dissolve command for execution.
   * Validates parameters and others, more specifically:
   * - Find and set primary instance (only apply to single primary mode);
   * - Ensure ONLINE and RECOVERING instances are reachable;
   * - Ensure reachable instance can sync cluster transactions (no replication
   *   errors);
   * - Handle unavailable instance (not ONLINE nor RECOVERING):
   *    - if interactive (and force not used) ask the user to continue of not
   *    - if force = true continue execution (only metadata removed)
   *    - if force = false then abort operation
   *
   *   NOTE: Similiar 'force' option handling is applied to unreachable
   *   instances or instances unable to sync with the cluster.
   */
  void prepare() override;

  /**
   * Execute the dissolve command.
   * More specifically:
   * - Remove the cluster data (including clusters and instances) from the
   *   metadata;
   * - For all reachable instances (except primary if there is only one):
   *    - Sync cluster transactions (to ensure removal of cluster data from
   *      metadata is applied on the instance);
   *    - Remove the instance from the cluster (GR group);
   *    - Remove replication (recovery) users from the instance;
   * - On the primary (if there is only one):
   *    - Remove the instance from the cluster (GR group);
   *    - Remove replication (recovery) users from the instance;
   * - Mark cluster as dissolved;
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
  const bool m_interactive;
  mysqlshdk::utils::nullable<bool> m_force;
  Cluster_impl *m_cluster = nullptr;
  std::vector<std::shared_ptr<mysqlsh::dba::Instance>> m_available_instances;
  std::vector<std::string> m_skipped_instances;
  std::vector<std::string> m_sync_error_instances;
  std::string m_primary_uuid;
  bool m_is_clusterset_member = false;

  /**
   * Auxiliar method to prompt the user to confirm the execution of the
   * dissolve() operation.
   */
  void prompt_to_confirm_dissolve() const;

  /**
   * Auxiliar method to ask the user if he want to continue with the operation
   * in case an issue was detected on an instances. This prompt is equivalente
   * to the use of the 'force' option for the dissolve operation.
   *
   * @return boolean value with the result of the user confirmation, true if
   * the user answered 'yes' and wants to continue with the operation,
   * otherwise false.
   */
  bool prompt_to_force_dissolve() const;

  /**
   * Validate if the given instance is reachable.
   *
   * This function verifies if it is possible to connect the the given instance,
   * i.e., if it is reachable. In case it is not reachable an error might be
   * issued depending on the specified 'force' option value (or the response
   * from the user in case it is requested).
   *
   * The connection options from the cluster, more specifically the
   * authentication options will be used to connect to the instance, since it
   * assumed that the same login credentials can be used to connect to all
   * instances.
   *
   * NOTE: The object's member lists of instances to dissolve or skip will be
   * properly updated by this function.
   *
   * @param instance_address String with the address <host>:<port> of the
   *                         instance to check.
   */
  void ensure_instance_reachable(const std::string &instance_address);

  /**
   * Validate if the given instance is able to catch up with current cluster
   * transactions.
   *
   * This allow to identify replication errors or high replication lags that
   * can lead to the failure to apply remaining cluster transactions on the
   * instances to remove. This function is used as a prevention to try to
   * ensure that the instance will be able to apply the transaction to remove
   * the cluster from the metadata, anticipating any previous replication issue.
   *
   * In case the instance is not able to sync with the cluster transactions an
   * error might be issued depending on the specified 'force' option value (or
   * the response from the user in case it is requested).
   *
   * NOTE: The object's member list of instances that failed to sync will be
   * properly updated by this function.
   */
  void ensure_transactions_sync();

  /**
   * Auxiliary function to handle instance that are not available.
   *
   * This function performs the necessary action depending on the specified
   * 'force' option value (or the response from the user in case it is
   * requested), issuing an error or updating the object's member list of
   * instances to be skipped.
   *
   * @param instance_address
   */
  void handle_unavailable_instances(const std::string &instance_address,
                                    const std::string &instance_state);

  /**
   * Auxiliary function to remove an instance.
   *
   * This function avoids duplication of code, since the primary need to be
   * remove last in single primary mode. In more detail, this function includes
   * the removal of the instance from the cluster and removal all replication
   * users (including all associated exception and force option handling).
   *
   * @param instance_address string with the adress of the instance to be
   *        removed from the cluster.
   * @param instance_index index of the instance to remove in the internal list
   *        of available instances.
   */
  void remove_instance(const std::string &instance_address,
                       const std::size_t instance_index);
};

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_DISSOLVE_H_
