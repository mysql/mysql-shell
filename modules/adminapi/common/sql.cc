/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/common/sql.h"
#include <utils/utils_general.h>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include "modules/adminapi/common/metadata_storage.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"

namespace mysqlsh {
namespace dba {
GRInstanceType get_gr_instance_type(
    std::shared_ptr<mysqlshdk::db::ISession> connection) {
  GRInstanceType ret_val = GRInstanceType::Standalone;

  std::string query(
      "select count(*) "
      "from performance_schema.replication_group_members "
      "where MEMBER_ID = @@server_uuid AND MEMBER_STATE IS "
      "NOT NULL AND MEMBER_STATE <> 'OFFLINE';");

  try {
    auto result = connection->query(query);
    auto row = result->fetch_one();

    if (row) {
      if (row->get_int(0) != 0) {
        log_debug("Instance type check: %s: GR is active",
                  connection->get_connection_options().as_uri().c_str());
        ret_val = GRInstanceType::GroupReplication;
      } else {
        log_debug("Instance type check: %s: GR is not active",
                  connection->get_connection_options().as_uri().c_str());
      }
    }
  } catch (const mysqlshdk::db::Error &error) {
    auto e = shcore::Exception::mysql_error_with_code_and_state(
        error.what(), error.code(), error.sqlstate());

    log_debug("Error querying GR member state: %s: %i %s",
              connection->get_connection_options().as_uri().c_str(),
              error.code(), error.what());

    // SELECT command denied to user 'test_user'@'localhost' for table
    // 'replication_group_members' (MySQL Error 1142)
    if (e.code() == 1142) {
      ret_val = GRInstanceType::Unknown;
    } else if (error.code() != ER_NO_SUCH_TABLE) {  // Tables doesn't exists
      throw shcore::Exception::mysql_error_with_code_and_state(
          error.what(), error.code(), error.sqlstate());
    }
  }

  // The server is part of a Replication Group
  // Let's see if it is registered in the Metadata Store
  if (ret_val == GRInstanceType::GroupReplication) {
    query =
        "select count(*) from mysql_innodb_cluster_metadata.instances "
        "where mysql_server_uuid = @@server_uuid";
    try {
      auto result = connection->query(query);
      auto row = result->fetch_one();

      if (row) {
        if (row->get_int(0) != 0) {
          log_debug("Instance type check: %s: Metadata record found",
                    connection->get_connection_options().as_uri().c_str());
          ret_val = GRInstanceType::InnoDBCluster;
        } else {
          log_debug("Instance type check: %s: Metadata record not found",
                    connection->get_connection_options().as_uri().c_str());
        }
      }
    } catch (const mysqlshdk::db::Error &error) {
      log_debug("Error querying metadata: %s: %i %s",
                connection->get_connection_options().as_uri().c_str(),
                error.code(), error.what());

      // Ignore error table does not exist (error 1146) for 5.7 or database
      // does not exist (error 1049) for 8.0, when metadata is not available.
      if (error.code() != ER_NO_SUCH_TABLE && error.code() != ER_BAD_DB_ERROR)
        throw shcore::Exception::mysql_error_with_code_and_state(
            error.what(), error.code(), error.sqlstate());
    }
  } else {
    // This is a standalone instance, check if it has metadata schema.
    const std::string schema = "mysql_innodb_cluster_metadata";

    query = shcore::sqlstring("show databases like ?", 0) << schema;

    auto result = connection->query(query);
    auto row = result->fetch_one();

    if (row && row->get_string(0) == schema) {
      log_debug("Instance type check: %s: Metadata found",
                connection->get_connection_options().as_uri().c_str());
      ret_val = GRInstanceType::StandaloneWithMetadata;

      query =
          "select count(*) from mysql_innodb_cluster_metadata.instances "
          "where mysql_server_uuid = @@server_uuid";

      result = connection->query(query);
      row = result->fetch_one();

      if (row) {
        if (row->get_int(0) != 0) {
          log_debug("Instance type check: %s: instance is in Metadata",
                    connection->get_connection_options().as_uri().c_str());
          ret_val = GRInstanceType::StandaloneInMetadata;
        } else {
          log_debug("Instance type check: %s: instance is not in Metadata",
                    connection->get_connection_options().as_uri().c_str());
        }
      }
    }
  }

  return ret_val;
}

void get_port_and_datadir(std::shared_ptr<mysqlshdk::db::ISession> connection,
                          int *port, std::string *datadir) {
  assert(port);
  assert(datadir);

  std::string query("select @@port, @@datadir;");

  // Any error will bubble up right away
  auto result = connection->query(query);
  auto row = result->fetch_one();

  *port = row->get_int(0);
  *datadir = row->get_string(1);
}

// This function will return the Group Replication State as seen from an
// instance within the group.
// For that reason, this function should be called ONLY when the instance has
// been validated to be NOT Standalone
Cluster_check_info get_replication_group_state(
    const mysqlshdk::mysql::IInstance &connection, GRInstanceType source_type) {
  Cluster_check_info ret_val;

  // Sets the source instance type
  ret_val.source_type = source_type;

  mysqlshdk::gr::Member_state member_state;
  std::string member_id;
  std::string group_name;
  bool single_primary_mode = false;
  bool has_quorum = false;
  bool is_primary = false;

  if (!mysqlshdk::gr::get_group_information(
          connection, &member_state, &member_id, &group_name,
          &single_primary_mode, &has_quorum, &is_primary)) {
    // GR not running (shouldn't happen)
    member_state = mysqlshdk::gr::Member_state::OFFLINE;
  }

  switch (member_state) {
    case mysqlshdk::gr::Member_state::ONLINE:
      if (is_primary)
        ret_val.source_state = ManagedInstance::State::OnlineRW;
      else
        ret_val.source_state = ManagedInstance::State::OnlineRO;
      break;
    case mysqlshdk::gr::Member_state::RECOVERING:
      ret_val.source_state = ManagedInstance::State::Recovering;
      break;
    case mysqlshdk::gr::Member_state::OFFLINE:
    case mysqlshdk::gr::Member_state::MISSING:
      ret_val.source_state = ManagedInstance::State::Offline;
      break;
    case mysqlshdk::gr::Member_state::ERROR:
      ret_val.source_state = ManagedInstance::State::Error;
      break;
    case mysqlshdk::gr::Member_state::UNREACHABLE:
      ret_val.source_state = ManagedInstance::State::Unreachable;
      break;
  }

  if (!has_quorum)
    ret_val.quorum = ReplicationQuorum::State::Quorumless;
  else
    ret_val.quorum = ReplicationQuorum::State::Normal;

  return ret_val;
}

/*
 * Retrieves the list of group replication
 * addresses of the peer instances of
 * instance_host
 */
std::vector<std::string> get_peer_seeds(
    std::shared_ptr<mysqlshdk::db::ISession> connection,
    const std::string &instance_host) {
  std::vector<std::string> ret_val;
  shcore::sqlstring query = shcore::sqlstring(
      "SELECT JSON_UNQUOTE(addresses->'$.grLocal') "
      "FROM mysql_innodb_cluster_metadata.instances "
      "WHERE addresses->'$.mysqlClassic' <> ? "
      "AND replicaset_id IN (SELECT replicaset_id "
      "FROM mysql_innodb_cluster_metadata.instances "
      "WHERE addresses->'$.mysqlClassic' = ?)",
      0);

  query << instance_host.c_str();
  query << instance_host.c_str();
  query.done();

  try {
    // Get current GR group seeds value
    auto result =
        connection->query("SELECT @@global.group_replication_group_seeds");
    auto row = result->fetch_one();
    std::string group_seeds_str = row->get_string(0);
    if (!group_seeds_str.empty())
      ret_val = shcore::split_string(group_seeds_str, ",");

    // Get the list of known seeds from the metadata.
    result = connection->query(query);
    row = result->fetch_one();
    while (row) {
      std::string seed = row->get_string(0);

      if (std::find(ret_val.begin(), ret_val.end(), seed) == ret_val.end()) {
        // Only add seed from metadata if not already in the GR group seeds.
        ret_val.push_back(seed);
      }
      row = result->fetch_one();
    }
  } catch (const mysqlshdk::db::Error &error) {
    log_warning("Unable to retrieve group seeds for instance '%s': %s",
                instance_host.c_str(), error.what());
  }

  return ret_val;
}

std::vector<std::pair<std::string, int>> get_open_sessions(
    std::shared_ptr<mysqlshdk::db::ISession> connection) {
  std::vector<std::pair<std::string, int>> ret;

  std::string query(
      "SELECT CONCAT(PROCESSLIST_USER, '@', PROCESSLIST_HOST) AS acct, "
      "COUNT(*) FROM performance_schema.threads WHERE type = 'foreground' "
      " AND name in ('thread/mysqlx/worker', 'thread/sql/one_connection')"
      " AND processlist_id <> connection_id()"
      "GROUP BY acct;");

  // Any error will bubble up right away
  auto result = connection->query(query);
  auto row = result->fetch_one();

  while (row) {
    if (!row->is_null(0)) {
      ret.emplace_back(row->get_string(0), row->get_int(1));
    }

    row = result->fetch_one();
  }

  return ret;
}

Instance_metadata query_instance_info(
    const mysqlshdk::mysql::IInstance &instance) {
  int xport = -1;
  std::string local_gr_address;

  // Get the required data from the joining instance to store in the metadata

  // Get the MySQL X port.
  try {
    xport = instance.queryf_one_int(0, -1, "SELECT @@mysqlx_port");
  } catch (const std::exception &e) {
    log_info(
        "The X plugin is not enabled on instance '%s'. No value will be "
        "assumed for the X protocol address.",
        instance.descr().c_str());
  }

  // Get the local GR host data.
  try {
    local_gr_address =
        instance.get_sysvar_string("group_replication_local_address")
            .get_safe("");
  } catch (const std::exception &e) {
  }

  Instance_metadata instance_def;

  instance_def.address = instance.get_canonical_hostname();
  instance_def.role_type = "HA";
  instance_def.endpoint = instance.get_canonical_address();
  if (xport != -1)
    instance_def.xendpoint =
        instance.get_canonical_hostname() + ":" + std::to_string(xport);
  instance_def.grendpoint = local_gr_address;
  instance_def.uuid = instance.get_uuid();

  // default label
  instance_def.label = instance_def.endpoint;

  return instance_def;
}
}  // namespace dba
}  // namespace mysqlsh
