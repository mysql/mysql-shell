/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_RESET_RECOVERY_ACCOUNTS_PASSWORD_H_
#define MODULES_ADMINAPI_CLUSTER_RESET_RECOVERY_ACCOUNTS_PASSWORD_H_

#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/command_interface.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

class Reset_recovery_accounts_password : public Command_interface {
 public:
  Reset_recovery_accounts_password(const bool interactive,
                                   mysqlshdk::utils::nullable<bool> force,
                                   const Cluster_impl &cluster);

  ~Reset_recovery_accounts_password() override = default;

  /**
   * Prepare the resetRecoveryAccountsPassword command for execution.
   *
   * Validates parameters and others, more specifically:
   * - Ensure ONLINE instances are reachable;
   * - Handle NOT ONLINE instances:
   *    - if interactive (and force not used) ask the user to continue
   *    - if force = true continue execution (dont reset the instance recovery
   * account password)
   *    - if force = false then abort operation
   *
   *   NOTE: Similar 'force' option handling is applied to unreachable
   *   instances.
   */
  void prepare() override;

  /**
   * Execute the reset of the recovery accounts.
   * More specifically:
   * - Iterate through all online cluster instances and reset the recovery
   * account password that they use.
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
   *
   * Closes the instances' connections (if previously established)).
   */
  void finish() override;

 private:
  const bool m_interactive;
  mysqlshdk::utils::nullable<bool> m_force;
  std::vector<std::shared_ptr<mysqlsh::dba::Instance>> m_online_instances;
  std::vector<std::string> m_skipped_instances;
  const Cluster_impl &m_cluster;

  /**
   * Auxiliar method to ask the user if he wants to continue with the operation
   * in case one or more instances are not ONLINE. This prompt is equivalent
   * to the use of the 'force' option for the resetRecoveryAccountsPassword
   * operation.
   *
   * @return boolean value with the result of the user confirmation, true if
   * the user answered 'yes' and wants to continue with the operation,
   * otherwise false.
   */
  bool prompt_to_force_reset() const;

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
   * NOTE: The object's member lists of instances to reset password will be
   * properly updated by this function.
   *
   * @param instance_address String with the address <host>:<port> of the
   *                         instance to check.
   */
  void ensure_instance_reachable(const std::string &instance_address);

  /**
   * Auxiliary function to handle instance that are not online.
   *
   * This function performs the necessary action depending on the specified
   * 'force' option value (or the response from the user in case it is
   * requested), issuing an error or updating the object's member list of
   * instances to be skipped.
   *
   * @param instance_address
   */
  void handle_not_online_instances(const std::string &instance_address,
                                   const std::string &instance_state);
};

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_RESET_RECOVERY_ACCOUNTS_PASSWORD_H_
