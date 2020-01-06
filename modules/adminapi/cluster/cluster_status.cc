/*
 * Copyright (c) 2018, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/cluster/cluster_status.h"
#include "modules/adminapi/cluster/replicaset/replicaset_status.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "mysqlshdk/libs/mysql/group_replication.h"

namespace mysqlsh {
namespace dba {

Cluster_status::Cluster_status(
    const Cluster_impl &cluster,
    const mysqlshdk::utils::nullable<uint64_t> &extended)
    : m_cluster(cluster), m_extended(extended) {}

Cluster_status::~Cluster_status() {}

void Cluster_status::prepare() {}

shcore::Value Cluster_status::get_replicaset_status(
    const GRReplicaSet &replicaset) {
  shcore::Value ret;

  // Create the Replicaset_status command and execute it.
  Replicaset_status op_replicaset_status(replicaset, m_extended);
  // Always execute finish when leaving "try catch".
  auto finally = shcore::on_leave_scope(
      [&op_replicaset_status]() { op_replicaset_status.finish(); });
  // Prepare the Replicaset_status command execution (validations).
  op_replicaset_status.prepare();
  // Execute Replicaset_status operations.
  ret = op_replicaset_status.execute();

  return ret;
}

shcore::Value Cluster_status::execute() {
  shcore::Dictionary_t dict = shcore::make_dict();

  (*dict)["clusterName"] = shcore::Value(m_cluster.get_name());

  // Get the default replicaSet options
  {
    std::shared_ptr<GRReplicaSet> default_replicaset =
        m_cluster.get_default_replicaset();

    if (default_replicaset == nullptr) {
      throw shcore::Exception::logic_error(
          "Default ReplicaSet not initialized.");
    }

    (*dict)["defaultReplicaSet"] =
        shcore::Value(get_replicaset_status(*default_replicaset));
  }

  // Gets the metadata version
  if (!m_extended.is_null() && *m_extended >= 1) {
    auto version = mysqlsh::dba::metadata::installed_version(
        m_cluster.get_target_server());
    (*dict)["metadataVersion"] = shcore::Value(version.get_base());
  }

  // Iterate all replicasets and get the status for each one

  std::string addr = m_cluster.get_target_server()->get_canonical_address();
  (*dict)["groupInformationSourceMember"] = shcore::Value(addr);

  auto md_server = m_cluster.get_metadata_storage()->get_md_server();

  // metadata server, if its a different one
  if (md_server &&
      md_server->get_uuid() != m_cluster.get_target_server()->get_uuid()) {
    (*dict)["metadataServer"] =
        shcore::Value(md_server->get_canonical_address());
  }

  return shcore::Value(dict);
}

void Cluster_status::rollback() {
  // Do nothing right now, but it might be used in the future when
  // transactional command execution feature will be available.
}

void Cluster_status::finish() {}

}  // namespace dba
}  // namespace mysqlsh
