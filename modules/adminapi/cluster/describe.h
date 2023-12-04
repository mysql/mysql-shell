/*
 * Copyright (c) 2019, 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_DESCRIBE_H_
#define MODULES_ADMINAPI_CLUSTER_DESCRIBE_H_

#include <vector>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/command_interface.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

class Describe final : public Command_interface {
 public:
  explicit Describe(const Cluster_impl &cluster) : m_cluster(cluster) {}

  /**
   * Prepare the Cluster Describe command for execution.
   * Validates parameters and others, more specifically:
   *   - Ensure the cluster is still registered in the metadata
   *   - Ensure the topology type didn't change as registered in the metadata
   *   - Gets the current members list
   */
  void prepare() override;

  /**
   * Execute the cluster describe command.
   * More specifically:
   * - Describes the structure of the Cluster and its Instances.
   *
   * @return an shcore::Value containing a dictionary object with the command
   * output
   */
  shcore::Value execute() override;

  /**
   * Finalize the command execution.
   * More specifically:
   * - Reset all auxiliary (temporary) data used for the operation execution.
   */
  void finish() override;

 private:
  const Cluster_impl &m_cluster;
  std::vector<Instance_metadata> m_instances;

  shcore::Value get_default_replicaset_description();

  void feed_metadata_info(shcore::Dictionary_t dict,
                          const Instance_metadata &info);

  shcore::Array_t get_topology();

  shcore::Dictionary_t collect_replicaset_description();
};

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_DESCRIBE_H_
