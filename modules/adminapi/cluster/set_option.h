/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_SET_OPTION_H_
#define MODULES_ADMINAPI_CLUSTER_SET_OPTION_H_

#include <string>
#include <vector>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/command_interface.h"
#include "mysqlshdk/libs/config/config.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

class Set_option : public Command_interface {
 public:
  Set_option(Cluster_impl *cluster, const std::string &option,
             const std::string &value);
  Set_option(Cluster_impl *cluster, const std::string &option, int64_t value);
  Set_option(Cluster_impl *cluster, const std::string &option, bool value);

  ~Set_option() override;

  /**
   * Prepare the Set_option command for execution.
   * Validates parameters and others, more specifically
   * - Validate if the option is valid, being the accepted values:
   *     - exitStateAction
   *     - memberWeight
   *     - failoverConsistency
   *     - consistency
   *     - expelTimeout
   *     - clusterName
   * - Verify user privileges to execute operation;
   * - Verify if the cluster has quorum (preconditions check)
   * - Verify if all the cluster members are ONLINE
   * - Verify if all cluster members support the option
   */
  void prepare() override;

  /**
   * Execute the Set_option command.
   *
   * @return An empty shcore::Value.
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
  Cluster_impl *m_cluster = nullptr;
  // Configuration object (to read and set instance configurations).
  std::string m_option;
  mysqlshdk::utils::nullable<std::string> m_value_str;
  mysqlshdk::utils::nullable<int64_t> m_value_int;
  mysqlshdk::utils::nullable<bool> m_value_bool;
  std::vector<std::shared_ptr<mysqlsh::dba::Instance>> m_cluster_instances;
  // Configuration object (to read and set instance configurations).
  std::unique_ptr<mysqlshdk::config::Config> m_cfg;

  void connect_all_members();
  void ensure_option_valid();
  void ensure_option_supported_all_members_cluster();
  void check_disable_clone_support();
  void update_disable_clone_option(bool disable_clone);
};

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_SET_OPTION_H_
