/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_REPLICASET_REPLICASET_OPTIONS_H_
#define MODULES_ADMINAPI_REPLICASET_REPLICASET_OPTIONS_H_

#include <map>
#include <string>
#include <vector>

#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/command_interface.h"

namespace mysqlsh {
namespace dba {

class Replicaset_options : public Command_interface {
 public:
  Replicaset_options(const ReplicaSet &replicaset,
                     mysqlshdk::utils::nullable<bool> all);

  ~Replicaset_options() override;

  /**
   * Prepare the Replicaset_options command for execution.
   * Validates parameters and others, more specifically:
   *   - Gets the current members list
   *   - Connects to every ReplicaSet member and populates the internal
   * connection lists
   */
  void prepare() override;

  /**
   * Execute the Replicaset_options command.
   * More specifically:
   * - Lists the configuration options of the ReplicaSet and its Instances.
   *
   * @return an shcore::Value containing a dictionary object with the command
   * output
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
   * - Reset all auxiliary (temporary) data used for the operation execution.
   */
  void finish() override;

 private:
  const ReplicaSet &m_replicaset;
  mysqlshdk::utils::nullable<bool> m_all;

  std::vector<ReplicaSet::Instance_info> m_instances;
  std::map<std::string, std::shared_ptr<mysqlshdk::db::ISession>>
      m_member_sessions;
  std::map<std::string, std::string> m_member_connect_errors;

  void connect_to_members();

  shcore::Array_t collect_global_options();

  shcore::Array_t get_instance_options(
      const mysqlshdk::mysql::Instance &instance);

  shcore::Dictionary_t collect_replicaset_options();
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_REPLICASET_REPLICASET_OPTIONS_H_
