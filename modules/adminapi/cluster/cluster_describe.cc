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

#include "modules/adminapi/cluster/cluster_describe.h"
#include "modules/adminapi/replicaset/replicaset_describe.h"

namespace mysqlsh {
namespace dba {

Cluster_describe::Cluster_describe(const Cluster_impl &cluster)
    : m_cluster(cluster) {}

Cluster_describe::~Cluster_describe() {}

void Cluster_describe::prepare() {}

shcore::Value Cluster_describe::get_replicaset_description(
    const ReplicaSet &replicaset) {
  shcore::Value ret;

  // Create the Replicaset_describe command and execute it.
  Replicaset_describe op_replicaset_describe(replicaset);
  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope(
      [&op_replicaset_describe]() { op_replicaset_describe.finish(); });
  // Prepare the Replicaset_describe command execution (validations).
  op_replicaset_describe.prepare();
  // Execute Replicaset_describe operations.
  ret = op_replicaset_describe.execute();

  return ret;
}

shcore::Value Cluster_describe::execute() {
  shcore::Dictionary_t dict = shcore::make_dict();

  (*dict)["clusterName"] = shcore::Value(m_cluster.get_name());

  // Get the default replicaSet description
  {
    std::shared_ptr<ReplicaSet> default_replicaset =
        m_cluster.get_default_replicaset();

    if (default_replicaset == nullptr) {
      throw shcore::Exception::logic_error(
          "Default ReplicaSet not initialized.");
    }

    (*dict)["defaultReplicaSet"] =
        shcore::Value(get_replicaset_description(*default_replicaset));
  }

  // Iterate all replicasets and get the description for each one
  // NOTE: to be done only when multiple replicasets are supported

  return shcore::Value(dict);
}

void Cluster_describe::rollback() {
  // Do nothing right now, but it might be used in the future when
  // transactional command execution feature will be available.
}

void Cluster_describe::finish() {}

}  // namespace dba
}  // namespace mysqlsh
