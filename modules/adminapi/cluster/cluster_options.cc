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

#include "modules/adminapi/cluster/cluster_options.h"
#include "modules/adminapi/cluster/replicaset/replicaset_options.h"
#include "modules/adminapi/common/common.h"
#include "mysqlshdk/libs/mysql/group_replication.h"

namespace mysqlsh {
namespace dba {

Cluster_options::Cluster_options(const Cluster_impl &cluster, bool all)
    : m_cluster(cluster), m_all(all) {}

Cluster_options::~Cluster_options() {}

void Cluster_options::prepare() {
  // Nothing to be done. Metadata session validation is done by the
  // MetadataStorage class itself
}

/**
 * Get the configuration options from a specific ReplicaSet
 *
 * This function gets the options from a specific ReplicaSet by creating an
 * object of Options and executing it
 *
 * @param replicaset target ReplicaSet to get the options from
 *
 * @return a shcore::Value containing a dictionary object with the command
 * output
 */
shcore::Value Cluster_options::get_replicaset_options(
    const ReplicaSet &replicaset) {
  // Create the Replicaset_options command and execute it.
  Replicaset_options op_replicaset_options(replicaset, m_all);
  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope(
      [&op_replicaset_options]() { op_replicaset_options.finish(); });
  // Prepare the Options command execution (validations).
  op_replicaset_options.prepare();

  // Execute Options operations.
  return op_replicaset_options.execute();
}

shcore::Value Cluster_options::execute() {
  shcore::Dictionary_t dict = shcore::make_dict();

  (*dict)["clusterName"] = shcore::Value(m_cluster.get_name());

  // Get the default replicaSet options
  {
    std::shared_ptr<ReplicaSet> default_replicaset =
        m_cluster.get_default_replicaset();

    if (default_replicaset == nullptr) {
      throw shcore::Exception::logic_error(
          "Default ReplicaSet not initialized.");
    }

    (*dict)["defaultReplicaSet"] =
        shcore::Value(get_replicaset_options(*default_replicaset));
  }

  // TODO(.) As soon as Support for multiple ReplicaSets is added, we must
  // iterate all replicasets and get the options for each one

  return shcore::Value(dict);
}

void Cluster_options::rollback() {
  // Do nothing right now, but it might be used in the future when
  // transactional command execution feature will be available.
}

void Cluster_options::finish() {}

}  // namespace dba
}  // namespace mysqlsh
