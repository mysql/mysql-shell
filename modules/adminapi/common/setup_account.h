/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_SETUP_ACCOUNT_H_
#define MODULES_ADMINAPI_COMMON_SETUP_ACCOUNT_H_

#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/api_options.h"
#include "modules/command_interface.h"
#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlsh {
namespace dba {

class Setup_account : public Command_interface {
 public:
  /**
   * Constructor for the Setup account command.
   * @param name username of the account to create
   * @param host hostname of the account to create
   * @param interactive flag that enables or disables interactive
   * prompts/wizzards
   * @param update flag that must be enabled if we are going to upgrade an
   * existing account or disabled to create a new account.
   * @param dry_run Flag that enables dry-run execution of operation
   * @param password The password to use with the new account or with the
   * existing account
   * @param grants the list of grants that will be given to the account
   * @param m_primary_server An instance object pointing to the primary_server
   * of the cluster/replicaset object where the command will execute.
   * @param purpose the purpose / context with which this account is being
   * setup.
   */
  Setup_account(const std::string &name, const std::string &host,
                const Setup_account_options &options,
                std::vector<std::string> grants,
                const mysqlshdk::mysql::IInstance &m_primary_server,
                Cluster_type purpose);

  ~Setup_account() override = default;

  /**
   * Prepare the setupAccount command for execution.
   *
   * Validates parameters and others, more specifically:
   * - Ensure that a password is provided if account name does not exist;
   * - If interactive, account doesn't exist and password was not provided,
   *    ask the user for a password.
   */
  void prepare() override;

  /**
   * Execute the account creation/update operation.
   * More specifically:
   * - Create the account with the given list of privileges if the account does
   *   not exist
   * - Update an existing account list of privileges (and optionally its
   * password) i if the account already exists.
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
   */
  void finish() override;

 private:
  const std::string m_name;
  const std::string m_host;
  Setup_account_options m_options;
  const Cluster_type m_purpose;
  const std::vector<std::string> m_privilege_list;
  const mysqlshdk::mysql::IInstance &m_primary_server;
  bool m_user_exists;

  /**
   * Helper method to ask the user for the account password if it was not
   * provided but it is necessary.
   *
   */
  void prompt_for_password();

  /**
   * Helper method to create a user on a given instance with a given password
   */
  void create_account();
};
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_SETUP_ACCOUNT_H_
