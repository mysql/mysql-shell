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

#ifndef MODULES_ADMINAPI_CLUSTER_CLUSTER_SET_OPTION_H_
#define MODULES_ADMINAPI_CLUSTER_CLUSTER_SET_OPTION_H_

#include <string>
#include <vector>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/cluster/replicaset/set_option.h"
#include "modules/command_interface.h"
#include "mysqlshdk/libs/config/config.h"

namespace mysqlsh {
namespace dba {

class Cluster_set_option : public Command_interface {
 public:
  Cluster_set_option(Cluster_impl *cluster, const std::string &option,
                     const std::string &value);
  Cluster_set_option(Cluster_impl *cluster, const std::string &option,
                     int64_t value);
  Cluster_set_option(Cluster_impl *cluster, const std::string &option,
                     bool value);

  ~Cluster_set_option() override;

  /**
   * Prepare the Cluster_set_option command for execution.
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

  std::unique_ptr<Set_option> m_replicaset_set_option;

  void ensure_option_valid();
  void check_disable_clone_support();
  void update_disable_clone_option(bool disable_clone);
};
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_CLUSTER_SET_OPTION_H_
