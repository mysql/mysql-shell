/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "modules/adminapi/mod_dba_sql.h"
#include "utils/utils_sqlstring.h"
#include <string>

namespace mysqlsh {
namespace dba {
GRInstanceType get_gr_instance_type(mysqlsh::mysql::Connection* connection) {
  GRInstanceType ret_val = GRInstanceType::Standalone;

  std::string query("select count(*) "
                    "from performance_schema.replication_group_members "
                    "where MEMBER_ID = @@server_uuid AND MEMBER_STATE IS NOT NULL AND MEMBER_STATE <> 'OFFLINE';");

  try {
    auto result = connection->run_sql(query);
    auto row = result->fetch_one();

    if (row) {
      if (row->get_value(0).as_int() != 0)
        ret_val = GRInstanceType::GroupReplication;
    }
  } catch (shcore::Exception& error) {
    if (error.code() != 1146) // Tables doesn't exist
      throw;
  }

  // The server is part of a Replication Group
  // Let's see if it is registered in the Metadata Store
  if (ret_val == GRInstanceType::GroupReplication) {
    query = "select count(*) from mysql_innodb_cluster_metadata.instances where mysql_server_uuid = @@server_uuid";
    try {
      auto result = connection->run_sql(query);
      auto row = result->fetch_one();

      if (row) {
        if (row->get_value(0).as_int() != 0)
          ret_val = GRInstanceType::InnoDBCluster;
      }
    } catch (shcore::Exception& error) {
      if (error.code() != 1146) // Tables doesn't exist
        throw;
    }
  }

  return ret_val;
}

void get_port_and_datadir(mysqlsh::mysql::Connection* connection, int &port, std::string& datadir) {
  std::string query("select @@port, @@datadir;");

  // Any error will bubble up right away
  auto result = connection->run_sql(query);
  auto row = result->fetch_one();

  port = row->get_value(0).as_int();
  datadir = row->get_value(1).as_string();
}

void get_gtid_state_variables(mysqlsh::mysql::Connection* connection, std::string &executed, std::string &purged) {
  // Retrieves the
  std::string query("show global variables where Variable_name in ('gtid_purged', 'gtid_executed')");

  // Any error will bubble up right away
  auto result = connection->run_sql(query);

  // Selects the gtid_executed value
  auto row = result->fetch_one();
  executed = row->get_value(1).as_string();

  // Selects the gtid_purged value
  row = result->fetch_one();
  purged = row->get_value(1).as_string();
}

SlaveReplicationState get_slave_replication_state(mysqlsh::mysql::Connection* connection, std::string &slave_executed) {
  SlaveReplicationState ret_val;

  if (slave_executed.empty())
    ret_val = SlaveReplicationState::New;
  else {
    std::string query;
    query = shcore::sqlstring("select gtid_subset(?, @@global.gtid_executed)", 0) << slave_executed;

    auto result = connection->run_sql(query);
    auto row = result->fetch_one();

    int slave_is_subset = row->get_value(0).as_int();

    if (1 == slave_is_subset) {
      // If purged has more gtids than the executed on the slave
      // it means some data will not be recoverable
      query = shcore::sqlstring("select gtid_subtract(@@global.gtid_purged, ?)", 0) << slave_executed;

      result = connection->run_sql(query);
      row = result->fetch_one();

      std::string missed = row->get_value(0).as_string();

      if (missed.empty())
        ret_val = SlaveReplicationState::Recoverable;
      else
        ret_val = SlaveReplicationState::Irrecoverable;
    } else
      // If slave executed gtids are not a subset of master's it means the slave has
      // own data not valid for the cluster where it is being checked
      ret_val = SlaveReplicationState::Diverged;
  }

  return ret_val;
}

// This function will return the Group Replication State as seen from an instance within the group
// For that reason, this function should be called ONLY when the instance has been validated to be NOT Standalone
ReplicationGroupState get_replication_group_state(mysqlsh::mysql::Connection* connection, GRInstanceType source_type) {
  ReplicationGroupState ret_val;
  int sum, count;

  // Sets the source instance type
  ret_val.source_type = source_type;

  // Gets the session uuid + the master uuid
  std::string uuid_query("SELECT @@server_uuid, VARIABLE_VALUE FROM performance_schema.global_status WHERE VARIABLE_NAME = 'group_replication_primary_member';");

  auto result = connection->run_sql(uuid_query);
  auto row = result->fetch_one();
  ret_val.source = row->get_value(0).as_string();
  ret_val.master = row->get_value(1).as_string();

  // Gets the cluster instance status count
  std::string instance_state_query("SELECT MEMBER_STATE FROM performance_schema.replication_group_members WHERE MEMBER_ID = '" + ret_val.source + "'");
  result = connection->run_sql(instance_state_query);
  row = result->fetch_one();
  std::string state = row->get_value(0).as_string();

  if (state == "ONLINE") {
    // On multimaster the primary member status is empty
    if (ret_val.master.empty() || ret_val.source == ret_val.master)
      ret_val.source_state = ManagedInstance::State::OnlineRW;
    else
      ret_val.source_state = ManagedInstance::State::OnlineRO;
  } else if (state == "RECOVERING")
    ret_val.source_state = ManagedInstance::State::Recovering;
  else if (state == "OFFLINE")
    ret_val.source_state = ManagedInstance::State::Offline;
  else if (state == "UNREACHABLE")
    ret_val.source_state = ManagedInstance::State::Unreachable;
  else if (state == "ERROR")
    ret_val.source_state = ManagedInstance::State::Error;

  // Gets the cluster instance status count
  // The quorum is said to be true if #UNREACHABLE_NODES < (TOTAL_NODES/2)
  std::string state_count_query("SELECT CAST(SUM(IF(member_state = 'UNREACHABLE', 1, 0)) AS SIGNED) AS UNREACHABLE,  COUNT(*) AS TOTAL FROM performance_schema.replication_group_members");

  result = connection->run_sql(state_count_query);
  row = result->fetch_one();
  int unreachable_count = row->get_value(0).as_int();
  int instance_count = row->get_value(1).as_int();

  if (unreachable_count >= (double(instance_count) / 2))
    ret_val.quorum = ReplicationQuorum::State::Quorumless;
  else
    ret_val.quorum = ReplicationQuorum::State::Normal;

  return ret_val;
}

std::string get_plugin_status(mysqlsh::mysql::Connection *connection, std::string plugin_name) {
  std::string query, status;
  query = shcore::sqlstring("SELECT PLUGIN_STATUS FROM INFORMATION_SCHEMA.PLUGINS WHERE PLUGIN_NAME = ?", 0) << plugin_name;

  // Any error will bubble up right away
  auto result = connection->run_sql(query);

  // Selects the PLUGIN_STATUS value
  auto row = result->fetch_one();

  if (row)
    status = row->get_value(0).as_string();
  else
    throw shcore::Exception::runtime_error("'" + plugin_name + "' could not be queried");

  return status;
}

bool get_server_variable(mysqlsh::mysql::Connection *connection, std::string name,
                         std::string &value, bool throw_on_error) {
  bool ret_val = true;
  std::string query = "SELECT @@" + name;

  try {
    auto result = connection->run_sql(query);
    auto row = result->fetch_one();

    if (row)
      value = row->get_value(0).as_string();
    else
      ret_val = false;
  } catch (shcore::Exception& error) {
    if (throw_on_error)
      throw;
  }
}

void set_global_variable(mysqlsh::mysql::Connection *connection, const std::string &name,
                          const std::string &value) {
  std::string query, query_raw = "SET GLOBAL " + name + " = ?";
  query = shcore::sqlstring(query_raw.c_str(), 0) << value;

  auto result = connection->run_sql(query);
}

bool get_status_variable(mysqlsh::mysql::Connection *connection, const std::string &name,
                          std::string &value, bool throw_on_error) {
  bool ret_val = false;
  std::string query = "SHOW STATUS LIKE '" + name + "'";

  try {
    auto result = connection->run_sql(query);
    auto row = result->fetch_one();

    if (row) {
      value = row->get_value(1).as_string();
      ret_val = true;
    }
  } catch (shcore::Exception &error) {
    if (throw_on_error)
      throw;
  }

  if (!ret_val && throw_on_error)
    throw shcore::Exception::runtime_error("'" + name + "' could not be queried");

  return ret_val;
}
}
}
