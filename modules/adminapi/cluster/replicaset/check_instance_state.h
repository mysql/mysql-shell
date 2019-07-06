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

#ifndef MODULES_ADMINAPI_REPLICASET_CHECK_INSTANCE_STATE_H_
#define MODULES_ADMINAPI_REPLICASET_CHECK_INSTANCE_STATE_H_

#include <string>

#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/command_interface.h"

namespace mysqlsh {
namespace dba {

class Check_instance_state : public Command_interface {
 public:
  Check_instance_state(
      const ReplicaSet &replicaset,
      const mysqlshdk::db::Connection_options &instance_cnx_opts);

  ~Check_instance_state() override;

  /**
   * Prepare the ReplicaSet check_instance_state command for execution.
   * Validates parameters and others, more specifically:
   * - Validate the connection options
   * - Ensure the target instance does not belong to the ReplicaSet
   * - Ensure that the target instance is reachable
   * - Ensure the target instance has a valid GR state, being the only accepted
   *   state Standalone
   */
  void prepare() override;

  /**
   * Execute the ReplicaSet check_instance_state command.
   * More specifically:
   * - Verifies the instance gtid state in relation to the ReplicaSet.
   *
   * @return an shcore::Value containing a dictionary object with the command
   * output
   */
  shcore::Value execute() override;

  /**
   * Rollback the command.
   *
   * NOTE: Not currently used.
   */
  void rollback() override;

  /**
   * Finalize the command execution.
   */
  void finish() override;

 private:
  const ReplicaSet &m_replicaset;
  mysqlshdk::db::Connection_options m_instance_cnx_opts;
  std::string m_target_instance_address;
  std::string m_address_in_metadata;
  std::unique_ptr<mysqlshdk::mysql::Instance> m_target_instance;
  bool m_clone_available = false;

  void ensure_target_instance_reachable();
  void ensure_instance_valid_gr_state();
  shcore::Dictionary_t collect_instance_state();
  void print_instance_state_info(shcore::Dictionary_t instance_state);
};
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_REPLICASET_CHECK_INSTANCE_STATE_H_
