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

#ifndef MODULES_ADMINAPI_CLUSTER_OPTIONS_H_
#define MODULES_ADMINAPI_CLUSTER_OPTIONS_H_

#include <map>
#include <string>
#include <vector>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/command_interface.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

class Options : public Command_interface {
 public:
  Options(const Cluster_impl &cluster, bool all);

  ~Options() override;

  /**
   * Prepare the cluster Options command for execution.
   * Validates parameters and others, more specifically:
   *   - Gets the current members list
   *   - Connects to every Cluster member and populates the internal
   * connection lists
   */
  void prepare() override;

  /**
   * Execute the cluster Options command.
   * More specifically:
   * - Obtains the Cluster's configuration options and iterates through all of
   * the Cluster's instance to obtain the configuration options
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
  const Cluster_impl &m_cluster;
  bool m_all = false;

  std::vector<Instance_metadata> m_instances;
  std::map<std::string, std::shared_ptr<Instance>> m_member_sessions;
  std::map<std::string, std::string> m_member_connect_errors;

  shcore::Value get_cluster_options();

  void connect_to_members();

  shcore::Array_t collect_global_options();

  shcore::Array_t get_instance_options(const mysqlsh::dba::Instance &instance);

  shcore::Dictionary_t collect_default_replicaset_options();
};

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_OPTIONS_H_
