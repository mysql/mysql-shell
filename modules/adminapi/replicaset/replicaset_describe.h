/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_REPLICASET_REPLICASET_DESCRIBE_H_
#define MODULES_ADMINAPI_REPLICASET_REPLICASET_DESCRIBE_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "modules/adminapi/mod_dba_cluster.h"
#include "modules/command_interface.h"
#include "mysqlshdk/libs/mysql/group_replication.h"

namespace mysqlsh {
namespace dba {

class Replicaset_describe : public Command_interface {
 public:
  explicit Replicaset_describe(const ReplicaSet &replicaset);

  ~Replicaset_describe() override;

  /**
   * Prepare the Replicaset_describe command for execution.
   * Validates parameters and others, more specifically:
   *   - Ensure the cluster is still registered in the metadata
   *   - Ensure the topology type didn't change as registered in the metadata
   *   - Gets the current members list
   */
  void prepare() override;

  /**
   * Execute the Replicaset_describe command.
   * More specifically:
   * - Describes the structure of the ReplicaSet and its Instances.
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
  std::shared_ptr<Cluster> m_cluster;

  std::vector<ReplicaSet::Instance_info> m_instances;

  void feed_metadata_info(shcore::Dictionary_t dict,
                          const ReplicaSet::Instance_info &info);

  void feed_member_info(shcore::Dictionary_t dict,
                        const mysqlshdk::gr::Member &member);

  shcore::Array_t get_topology(
      const std::vector<mysqlshdk::gr::Member> &member_info);

  shcore::Dictionary_t collect_replicaset_description();
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_REPLICASET_REPLICASET_DESCRIBE_H_
