/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mysqlshdk/libs/innodbcluster/cluster_metadata.h"
#include <cassert>
#include "mysqlshdk/libs/innodbcluster/cluster.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"

#include "scripting/types.h"  // for shcore::Exception

namespace mysqlshdk {
namespace innodbcluster {

std::shared_ptr<Metadata_mysql> Metadata_mysql::create(
    std::shared_ptr<db::ISession> session) {
  return std::shared_ptr<Metadata_mysql>(new Metadata_mysql(session));
}

Metadata_mysql::Metadata_mysql(std::shared_ptr<db::ISession> session)
    : _session(session) {}

std::shared_ptr<db::IResult> Metadata_mysql::query(const std::string &sql) {
  assert(_session);
  log_debug("InnoDB Cluster Metadata: %s", sql.c_str());
  return _session->query(sql);
}

bool Metadata_mysql::exists() {
  try {
    std::shared_ptr<db::IResult> result(_session->query(
        "SELECT * FROM mysql_innodb_cluster_metadata.schema_version"));
    const db::IRow *row = result->fetch_one();
    if (row) {
      // Enough that the table exists for now
      return true;
    }
  } catch (mysqlshdk::db::Error &e) {
    if (e.code() == ER_BAD_DB_ERROR || e.code() == ER_NO_SUCH_TABLE) {
      return false;
    }
    throw;
  }
  return false;
}

bool Metadata_mysql::get_cluster_named(const std::string &name,
                                       Cluster_info *out_info) {
  shcore::sqlstring q;

  // Get the Cluster ID
  if (name.empty()) {
    q = shcore::sqlstring(
        "SELECT cluster_id, cluster_name, default_replicaset, "
        "     description, options, attributes "
        "FROM mysql_innodb_cluster_metadata.clusters "
        "WHERE attributes->'$.default'",
        0);
  } else {
    q = shcore::sqlstring(
        "SELECT cluster_id, cluster_name, default_replicaset, "
        "     description, options, attributes "
        "FROM mysql_innodb_cluster_metadata.clusters "
        "WHERE cluster_name = ?",
        0);
    q << name;
  }
  std::shared_ptr<db::IResult> result = query(q);
  const db::IRow *row = result->fetch_one();
  if (row) {
    out_info->id = row->get_uint(0);
    out_info->name = row->is_null(1) ? "" : row->get_string(1);
    out_info->description = row->is_null(3) ? "" : row->get_string(3);
    out_info->attributes = row->is_null(5) ? "" : row->get_string(5);
    return true;
  }
  return false;
}

Replicaset_id Metadata_mysql::get_default_replicaset(
    Cluster_id cluster_id, std::vector<Group_id> *out_group_ids) {
  shcore::sqlstring q(
      "SELECT rs.replicaset_id, rs.replicaset_name, rs.topology_type, "
      "     rs.attributes->>'$.group_replication_group_name' as group_name"
      " FROM mysql_innodb_cluster_metadata.replicasets rs"
      " JOIN mysql_innodb_cluster_metadata.clusters c"
      "   ON rs.cluster_id = c.cluster_id "
      "   AND rs.replicaset_id = c.default_replicaset"
      " WHERE c.cluster_id = ?",
      0);
  q << cluster_id;

  std::shared_ptr<db::IResult> result(query(q));
  const db::IRow *row = result->fetch_one();
  if (row) {
    if (!row->is_null(3))
      out_group_ids->push_back(row->get_string(3));
    else
      throw cluster_error(Error::Metadata_inconsistent,
                          "group_name not set in replicaset " +
                              std::to_string(row->get_uint(0)));
    return row->get_uint(0);
  }
  throw cluster_error(
      Error::Metadata_inconsistent,
      "No default replicaset in cluster " + std::to_string(cluster_id));
}

std::vector<Instance_info> Metadata_mysql::get_group_instances(
    const Group_id &group_id) {
  std::vector<Instance_info> instances;
  shcore::sqlstring q;

  q = shcore::sqlstring(
      "SELECT i.mysql_server_uuid, i.instance_name, i.role,"
      "     i.addresses->>'$.mysqlClassic' as endpoint,"
      "     i.addresses->>'$.mysqlX' as xendpoint"
      " FROM mysql_innodb_cluster_metadata.instances as i"
      " JOIN mysql_innodb_cluster_metadata.replicasets as rs"
      "     ON i.replicaset_id = rs.replicaset_id"
      " WHERE rs.attributes->>'$.group_replication_group_name' = ?",
      0);
  q << group_id;

  std::shared_ptr<db::IResult> result(query(q));
  const db::IRow *row = result->fetch_one();
  while (row) {
    Instance_info info;

    info.uuid = row->get_string(0);
    info.name = row->get_string(1);
    info.classic_endpoint = row->is_null(3) ? "" : row->get_string(3);
    info.x_endpoint = row->is_null(4) ? "" : row->get_string(4);

    instances.push_back(info);
    row = result->fetch_one();
  }
  return instances;
}

}  // namespace innodbcluster
}  // namespace mysqlshdk
