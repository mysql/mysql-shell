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

#ifndef MODULES_ADMINAPI_CLUSTER_CHECK_INSTANCE_STATE_H_
#define MODULES_ADMINAPI_CLUSTER_CHECK_INSTANCE_STATE_H_

#include <memory>
#include <string>

#include "adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/command_interface.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

class Check_instance_state : public Command_interface {
 public:
  Check_instance_state(const Cluster_impl &cluster,
                       const std::shared_ptr<mysqlsh::dba::Instance> &target);

  ~Check_instance_state() override;

  /**
   * Prepare the Cluster check_instance_state command for execution.
   * Validates parameters and others, more specifically:
   * - Validate the connection options
   * - Ensure the target instance does not belong to the Cluster
   * - Ensure that the target instance is reachable
   * - Ensure the target instance has a valid GR state, being the only accepted
   *   state Standalone
   */
  void prepare() override;

  /**
   * Execute the Cluster check_instance_state command.
   * More specifically:
   * - Verifies the instance gtid state in relation to the Cluster.
   *
   * @return an shcore::Value containing a dictionary object with the command
   * output
   */
  shcore::Value execute() override;

 private:
  const Cluster_impl &m_cluster;
  std::string m_address_in_metadata;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  bool m_clone_available = false;

  void ensure_instance_valid_gr_state();
  shcore::Dictionary_t collect_instance_state();
  void print_instance_state_info(shcore::Dictionary_t instance_state);
};

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_CHECK_INSTANCE_STATE_H_
