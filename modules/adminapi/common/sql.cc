/*
 * Copyright (c) 2016, 2022, Oracle and/or its affiliates.
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
#include <mysqld_error.h>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "mysqld_error.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"

namespace mysqlsh {
namespace dba {

TargetType::Type get_gr_instance_type(
    const mysqlshdk::mysql::IInstance &instance) {
  TargetType::Type ret_val = TargetType::Standalone;

  std::string query(
      "select count(*) "
      "from performance_schema.replication_group_members "
      "where MEMBER_ID = @@server_uuid AND MEMBER_STATE IS "
      "NOT NULL AND MEMBER_STATE <> 'OFFLINE';");

  try {
    auto result = instance.query(query);
    auto row = result->fetch_one();

    if (row) {
      if (row->get_int(0) != 0) {
        log_debug("Instance type check: %s: GR is active",
                  instance.descr().c_str());
        ret_val = TargetType::GroupReplication;
      } else {
        log_debug("Instance type check: %s: GR is not active",
                  instance.descr().c_str());
      }
    }
  } catch (const shcore::Error &error) {
    auto e =
        shcore::Exception::mysql_error_with_code(error.what(), error.code());

    log_info("Error querying GR member state: %s: %i %s",
             instance.descr().c_str(), error.code(), error.what());

    // SELECT command denied to user 'test_user'@'localhost' for table
    // 'replication_group_members' (MySQL Error 1142)
    if (e.code() == ER_TABLEACCESS_DENIED_ERROR) {
      ret_val = TargetType::Unknown;
    } else if (error.code() != ER_NO_SUCH_TABLE) {  // Tables doesn't exists
      throw shcore::Exception::mysql_error_with_code(error.what(),
                                                     error.code());
    }
  }

  // The server is part of a Replication Group
  // Let's see if it is registered in the Metadata Store
  if (ret_val == TargetType::GroupReplication) {
    // In Metadata schema versions higher than 1.0.1
    // instances.mysql_server_uuid uses the collation ascii_general_ci which
    // then when doing comparisons with the sysvar @@server_uuid will results
    // in an illegal mix of collations. For that reason, we must do the right
    // cast of @@server_uuid to ascii_general_ci.
    // To work with all versions of the Metadata schema, we simply cast
    // mysql_server_uuid too.
    query =
        "SELECT COUNT(*) FROM mysql_innodb_cluster_metadata.instances WHERE "
        "CAST(mysql_server_uuid AS CHAR CHARACTER SET ASCII) = "
        "CAST(@@server_uuid AS CHAR CHARACTER SET ASCII)";

    try {
      auto result = instance.query(query);
      auto row = result->fetch_one();

      if (row) {
        if (row->get_int(0) != 0) {
          log_debug("Instance type check: %s: Metadata record found",
                    instance.descr().c_str());
          ret_val = TargetType::InnoDBCluster;
        } else {
          log_debug("Instance type check: %s: Metadata record not found",
                    instance.descr().c_str());
        }
      }
    } catch (const shcore::Error &error) {
      log_debug("Error querying metadata: %s: %i %s", instance.descr().c_str(),
                error.code(), error.what());

      // Ignore error table does not exist (error 1146) for 5.7 or database
      // does not exist (error 1049) for 8.0, when metadata is not available.
      if (error.code() != ER_NO_SUCH_TABLE && error.code() != ER_BAD_DB_ERROR)
        throw shcore::Exception::mysql_error_with_code(error.what(),
                                                       error.code());
    }
  } else {
    // This is a standalone instance or belongs to a ReplicaSet, check if it has
    // metadata schema.
    const std::string schema = "mysql_innodb_cluster_metadata";

    query = shcore::sqlstring("show databases like ?", 0) << schema;

    auto result = instance.query(query);
    auto row = result->fetch_one();

    if (row && row->get_string(0) == schema) {
      log_debug("Instance type check: %s: Metadata found",
                instance.descr().c_str());
      ret_val = TargetType::StandaloneWithMetadata;

      // In Metadata schema versions higher than 1.0.1
      // instances.mysql_server_uuid uses the collation ascii_general_ci which
      // then when doing comparisons with the sysvar @@server_uuid will results
      // in an illegal mix of collations. For that reason, we must do the right
      // cast of @@server_uuid to ascii_general_ci.
      // To work with all versions of the Metadata schema, we simply cast
      // mysql_server_uuid too.
      query =
          "SELECT COUNT(*) FROM mysql_innodb_cluster_metadata.instances WHERE "
          "CAST(mysql_server_uuid AS CHAR CHARACTER SET ASCII) = "
          "CAST(@@server_uuid AS CHAR CHARACTER SET ASCII)";

      result = instance.query(query);
      row = result->fetch_one();

      if (row) {
        if (row->get_int(0) != 0) {
          log_debug("Instance type check: %s: instance is in Metadata",
                    instance.descr().c_str());
          ret_val = TargetType::StandaloneInMetadata;
        } else {
          log_debug("Instance type check: %s: instance is not in Metadata",
                    instance.descr().c_str());
        }
      }
    }
  }

  return ret_val;
}

void get_port_and_datadir(const mysqlshdk::mysql::IInstance &instance,
                          int *port, std::string *datadir) {
  assert(port);
  assert(datadir);

  std::string query("select @@port, @@datadir;");

  // Any error will bubble up right away
  auto result = instance.query(query);
  auto row = result->fetch_one();

  *port = row->get_int(0);
  *datadir = row->get_string(1);
}

// This function will return the Group Replication State as seen from an
// instance within the group.
// For that reason, this function should be called ONLY when the instance has
// been validated to be NOT Standalone
Cluster_check_info get_replication_group_state(
    const mysqlshdk::mysql::IInstance &connection,
    TargetType::Type source_type) {
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
          connection, &member_state, &member_id, &group_name, nullptr,
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
    ret_val.quorum = ReplicationQuorum::States::Quorumless;
  else
    ret_val.quorum = ReplicationQuorum::States::Normal;

  return ret_val;
}

std::vector<std::pair<std::string, int>> get_open_sessions(
    const mysqlshdk::mysql::IInstance &instance) {
  std::vector<std::pair<std::string, int>> ret;

  std::string query(
      "SELECT CONCAT(PROCESSLIST_USER, '@', PROCESSLIST_HOST) AS acct, "
      "COUNT(*) FROM performance_schema.threads WHERE type = 'foreground' "
      " AND name in ('thread/mysqlx/worker', 'thread/sql/one_connection')"
      " AND processlist_id <> connection_id()"
      "GROUP BY acct;");

  // Any error will bubble up right away
  auto result = instance.query(query);
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
    const mysqlshdk::mysql::IInstance &instance, bool validate_gr_endpoint) {
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

    if (validate_gr_endpoint && local_gr_address.empty())
      throw shcore::Exception::error_with_code(
          "", "group_replication_local_address is empty",
          SHERR_DBA_EMPTY_LOCAL_ADDRESS);

    // Set debug trap to simulate exception / not be able to read string
    DBUG_EXECUTE_IF("dba_instance_query_gr_local_address", {
      throw shcore::Exception::error_with_code(
          "", "group_replication_local_address is empty",
          SHERR_DBA_EMPTY_LOCAL_ADDRESS);
    });

  } catch (const std::exception &e) {
    log_error(
        "Unable to read Group Replication local address on instance '%s': %s",
        instance.descr().c_str(), e.what());
    throw;
  }

  Instance_metadata instance_def;

  instance_def.address = instance.get_canonical_address();
  instance_def.endpoint = instance.get_canonical_address();
  if (xport != -1)
    instance_def.xendpoint = mysqlshdk::utils::make_host_and_port(
        instance.get_canonical_hostname(), xport);

  instance_def.grendpoint = local_gr_address;
  instance_def.uuid = instance.get_uuid();
  instance_def.server_id = instance.get_server_id();

  // default label
  instance_def.label = instance_def.endpoint;

  return instance_def;
}
}  // namespace dba
}  // namespace mysqlsh
