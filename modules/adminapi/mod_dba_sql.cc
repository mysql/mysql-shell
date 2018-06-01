/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/mod_dba_sql.h"
#include <utils/utils_general.h>
#include <algorithm>
#include <random>
#include <string>
#include <utility>
#include <vector>
#include "utils/utils_sqlstring.h"

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
  } catch (mysqlshdk::db::Error &error) {
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
    } catch (mysqlshdk::db::Error &error) {
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
    }
  }

  return ret_val;
}

void get_port_and_datadir(std::shared_ptr<mysqlshdk::db::ISession> connection,
                          int &port, std::string &datadir) {
  std::string query("select @@port, @@datadir;");

  // Any error will bubble up right away
  auto result = connection->query(query);
  auto row = result->fetch_one();

  port = row->get_int(0);
  datadir = row->get_string(1);
}

void get_gtid_state_variables(
    std::shared_ptr<mysqlshdk::db::ISession> connection, std::string &executed,
    std::string &purged) {
  // Retrieves the
  std::string query(
      "show global variables where Variable_name in ('gtid_purged', "
      "'gtid_executed')");

  // Any error will bubble up right away
  auto result = connection->query(query);

  auto row = result->fetch_one();

  if (row->get_string(0) == "gtid_executed") {
    executed = row->get_string(1);
    row = result->fetch_one();
    purged = row->get_string(1);
  } else {
    purged = row->get_string(1);
    row = result->fetch_one();
    executed = row->get_string(1);
  }
}

SlaveReplicationState get_slave_replication_state(
    std::shared_ptr<mysqlshdk::db::ISession> connection,
    const std::string &slave_executed) {
  SlaveReplicationState ret_val;

  if (slave_executed.empty()) {
    ret_val = SlaveReplicationState::New;
  } else {
    std::string query;
    query =
        shcore::sqlstring("select gtid_subset(?, @@global.gtid_executed)", 0)
        << slave_executed;

    auto result = connection->query(query);
    auto row = result->fetch_one();

    int slave_is_subset = row->get_int(0);

    if (1 == slave_is_subset) {
      // If purged has more gtids than the executed on the slave
      // it means some data will not be recoverable
      query =
          shcore::sqlstring("select gtid_subtract(@@global.gtid_purged, ?)", 0)
          << slave_executed;

      result = connection->query(query);
      row = result->fetch_one();

      std::string missed = row->get_string(0);

      if (missed.empty())
        ret_val = SlaveReplicationState::Recoverable;
      else
        ret_val = SlaveReplicationState::Irrecoverable;
    } else {
      // If slave executed gtids are not a subset of master's it means the
      // slave has own data not valid for the cluster where it is being checked
      ret_val = SlaveReplicationState::Diverged;
    }
  }

  return ret_val;
}

// This function will return the Group Replication State as seen from an
// instance within the group.
// For that reason, this function should be called ONLY when the instance has
// been validated to be NOT Standalone
Cluster_check_info get_replication_group_state(
    std::shared_ptr<mysqlshdk::db::ISession> connection,
    GRInstanceType source_type) {
  Cluster_check_info ret_val;

  // Sets the source instance type
  ret_val.source_type = source_type;

  // Gets the session uuid + the master uuid
  std::string uuid_query(
      "SELECT @@server_uuid, VARIABLE_VALUE FROM "
      "performance_schema.global_status WHERE VARIABLE_NAME = "
      "'group_replication_primary_member';");

  auto result = connection->query(uuid_query);
  auto row = result->fetch_one();
  if (!row) {
    log_warning(
        "Query on performance_schema.global_status returned no rows. Probably "
        "missing privileges");
    throw std::runtime_error(
        "Error querying GR state. Probably insufficient privileges.");
  }
  ret_val.source = row->get_string(0);
  ret_val.master = row->get_string(1);

  // Gets the cluster instance status count
  std::string instance_state_query(
      "SELECT MEMBER_STATE FROM performance_schema.replication_group_members "
      "WHERE MEMBER_ID = '" +
      ret_val.source + "'");
  result = connection->query(instance_state_query);
  row = result->fetch_one();
  if (!row) {
    log_warning(
        "Query on performance_schema.replication_group_members returned no "
        "rows. Probably missing privileges");
    throw std::runtime_error(
        "Error querying GR state. Probably insufficient privileges.");
  }

  std::string state = row->get_string(0);

  if (state == "ONLINE") {
    // On multiPrimary the primary member status is empty
    if (ret_val.master.empty() || ret_val.source == ret_val.master)
      ret_val.source_state = ManagedInstance::State::OnlineRW;
    else
      ret_val.source_state = ManagedInstance::State::OnlineRO;
  } else if (state == "RECOVERING") {
    ret_val.source_state = ManagedInstance::State::Recovering;
  } else if (state == "OFFLINE") {
    ret_val.source_state = ManagedInstance::State::Offline;
  } else if (state == "UNREACHABLE") {
    ret_val.source_state = ManagedInstance::State::Unreachable;
  } else if (state == "ERROR") {
    ret_val.source_state = ManagedInstance::State::Error;
  }

  // Gets the cluster instance status count
  // The quorum is said to be true if #UNREACHABLE_NODES < (TOTAL_NODES/2)
  std::string state_count_query(
      "SELECT CAST(SUM(IF(member_state = 'UNREACHABLE', 1, 0)) AS SIGNED) AS "
      "UNREACHABLE,  COUNT(*) AS TOTAL FROM "
      "performance_schema.replication_group_members");

  result = connection->query(state_count_query);
  row = result->fetch_one();
  int unreachable_count = row->get_int(0);
  int instance_count = row->get_int(1);

  if (unreachable_count >= (static_cast<double>(instance_count) / 2))
    ret_val.quorum = ReplicationQuorum::State::Quorumless;
  else
    ret_val.quorum = ReplicationQuorum::State::Normal;

  return ret_val;
}

/**
 * Get the state of the instance identified by the specified address.
 *
 * @param connection Connection to the cluster instance that is going to be
 *                   used to obtain the status of the target instance.
 *                   It can be any (alive) instance in the cluster that is not
 *                   the target instance to check.
 * @param address string in the format <host>:<port> with the address that
 *                identifies the target cluster instance to obtain its state.
 * @return ManagedInstance::State corresponding to the current state of the
 *         instance identified by the given address from the point of view
 *         of the instance used for the connection argument.
 */
ManagedInstance::State get_instance_state(
    std::shared_ptr<mysqlshdk::db::ISession> connection,
    const std::string &address) {
  // Get the primary uuid
  std::string uuid_query(
      "SELECT variable_value "
      "FROM performance_schema.global_status "
      "WHERE variable_name = 'group_replication_primary_member'");
  auto result = connection->query(uuid_query);
  auto row = result->fetch_one();
  std::string primary_uuid = row->get_string(0);

  // Get the state information of the instance with the given address.
  shcore::sqlstring query = shcore::sqlstring(
      "SELECT mysql_server_uuid, instance_name, member_state "
      "FROM mysql_innodb_cluster_metadata.instances "
      "LEFT JOIN performance_schema.replication_group_members "
      "ON `mysql_server_uuid`=`member_id` "
      "WHERE addresses->\"$.mysqlClassic\" = ?",
      0);
  query << address;
  query.done();

  result = connection->query(query);
  row = result->fetch_one();
  if (!row) {
    throw shcore::Exception::runtime_error(
        "Unable to retreive status information for the instance '" + address +
        "'. The instance might no longer be part of the cluster.");
  }
  std::string instance_uuid = row->get_as_string(0);
  std::string state = row->get_as_string(2);

  if (state.compare("ONLINE") == 0) {
    // For multiPrimary the primary uuid is empty
    if (primary_uuid.empty() || primary_uuid.compare(instance_uuid) == 0)
      return ManagedInstance::State::OnlineRW;
    else
      return ManagedInstance::State::OnlineRO;
  } else if (state.compare("RECOVERING") == 0) {
    return ManagedInstance::State::Recovering;
  } else if (state.compare("OFFLINE") == 0) {
    return ManagedInstance::State::Offline;
  } else if (state.compare("UNREACHABLE") == 0) {
    return ManagedInstance::State::Unreachable;
  } else if (state.compare("ERROR") == 0) {
    return ManagedInstance::State::Error;
  } else if (state.compare("NULL") == 0) {
    return ManagedInstance::State::Missing;
  } else {
    throw shcore::Exception::runtime_error("The instance '" + address +
                                           "' has an unexpected status: '" +
                                           state + "'.");
  }
}

std::string get_plugin_status(
    std::shared_ptr<mysqlshdk::db::ISession> connection,
    std::string plugin_name) {
  std::string query, status;
  query = shcore::sqlstring(
              "SELECT PLUGIN_STATUS FROM INFORMATION_SCHEMA.PLUGINS WHERE "
              "PLUGIN_NAME = ?",
              0)
          << plugin_name;

  // Any error will bubble up right away
  auto result = connection->query(query);

  // Selects the PLUGIN_STATUS value
  auto row = result->fetch_one();

  if (row)
    status = row->get_string(0);
  else
    throw shcore::Exception::runtime_error("'" + plugin_name +
                                           "' could not be queried");

  return status;
}

// This function verifies if a server with the given UUID is part
// Of the replication group members using connection
bool is_server_on_replication_group(
    std::shared_ptr<mysqlshdk::db::ISession> connection,
    const std::string &uuid) {
  std::string query;
  query = shcore::sqlstring(
              "select count(*) from "
              "performance_schema.replication_group_members where "
              "member_id = ?",
              0)
          << uuid;

  // Any error will bubble up right away
  auto result = connection->query(query);

  // Selects the PLUGIN_STATUS value
  auto row = result->fetch_one();

  int count;
  if (row)
    count = row->get_int(0);
  else
    throw shcore::Exception::runtime_error(
        "Unable to verify Group Replication "
        "membership");

  return count == 1;
}

bool get_server_variable(std::shared_ptr<mysqlshdk::db::ISession> connection,
                         const std::string &name, std::string &value,
                         bool throw_on_error) {
  bool ret_val = true;
  std::string query = "SELECT @@" + name;

  try {
    auto result = connection->query(query);
    auto row = result->fetch_one();

    if (row)
      value = row->get_string(0);
    else
      ret_val = false;
  } catch (mysqlshdk::db::Error &error) {
    if (throw_on_error) {
      throw shcore::Exception::mysql_error_with_code_and_state(
          error.what(), error.code(), error.sqlstate());
    } else {
      log_warning("Unable to read server variable '%s': %s", name.c_str(),
                  error.what());
      ret_val = false;
    }
  }

  return ret_val;
}

bool get_server_variable(std::shared_ptr<mysqlshdk::db::ISession> connection,
                         const std::string &name, int &value,
                         bool throw_on_error) {
  bool ret_val = true;
  std::string query = "SELECT @@" + name;

  try {
    auto result = connection->query(query);
    auto row = result->fetch_one();

    if (row)
      value = row->get_int(0);
    else
      ret_val = false;
  } catch (mysqlshdk::db::Error &error) {
    if (throw_on_error) {
      throw shcore::Exception::mysql_error_with_code_and_state(
          error.what(), error.code(), error.sqlstate());
    } else {
      log_warning("Unable to read server variable '%s': %s", name.c_str(),
                  error.what());
      ret_val = false;
    }
  }

  return ret_val;
}

void set_global_variable(std::shared_ptr<mysqlshdk::db::ISession> connection,
                         const std::string &name, const std::string &value) {
  std::string query, query_raw = "SET GLOBAL " + name + " = ?";
  query = shcore::sqlstring(query_raw.c_str(), 0) << value;

  connection->execute(query);
}

bool get_status_variable(std::shared_ptr<mysqlshdk::db::ISession> connection,
                         const std::string &name, std::string &value,
                         bool throw_on_error) {
  bool ret_val = false;
  std::string query = "SHOW STATUS LIKE '" + name + "'";

  try {
    auto result = connection->query(query);
    auto row = result->fetch_one();

    if (row) {
      value = row->get_string(1);
      ret_val = true;
    }
  } catch (mysqlshdk::db::Error &error) {
    if (throw_on_error)
      throw shcore::Exception::mysql_error_with_code_and_state(
          error.what(), error.code(), error.sqlstate());
  }

  if (!ret_val && throw_on_error)
    throw shcore::Exception::runtime_error("'" + name +
                                           "' could not be queried");

  return ret_val;
}

bool is_gtid_subset(std::shared_ptr<mysqlshdk::db::ISession> connection,
                    const std::string &subset, const std::string &set) {
  int ret_val = 0;
  std::string query = shcore::sqlstring("SELECT GTID_SUBSET(?, ?)", 0)
                      << subset << set;

  try {
    auto result = connection->query(query);
    auto row = result->fetch_one();

    if (row) ret_val = row->get_int(0);
  } catch (mysqlshdk::db::Error &error) {
    throw shcore::Exception::mysql_error_with_code_and_state(
        error.what(), error.code(), error.sqlstate());
  }

  return (ret_val == 1);
}

/*
 * Get the master status information of the server and return a Map with the
 * retrieved information:
 * - FILE
 * - POSITION
 * - BINLOG_DO_DB
 * - BINLOG_IGNORE_DB
 * - EXECUTED_GTID_SET
 */
// NOTE(rennox): This function should NOT return a schore::Value, either a tuple
// or a struct should be used
shcore::Value get_master_status(
    std::shared_ptr<mysqlshdk::db::ISession> connection) {
  shcore::Value::Map_type_ref status(new shcore::Value::Map_type);
  std::string query = "SHOW MASTER STATUS";
  auto result = connection->query(query);
  auto row = result->fetch_one();

  if (row) {
    (*status)["FILE"] = shcore::Value(row->get_string(0));
    (*status)["POSITION"] = shcore::Value(row->get_int(1));
    (*status)["BINLOG_DO_DB"] = shcore::Value(row->get_string(2));
    (*status)["BINLOG_IGNORE_DB"] = shcore::Value(row->get_string(3));
    (*status)["EXECUTED_GTID_SET"] = shcore::Value(row->get_string(4));
  }
  return shcore::Value(status);
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
  } catch (mysqlshdk::db::Error &error) {
    log_warning("Unable to retrieve group seeds for instance '%s': %s",
                instance_host.c_str(), error.what());
  }

  return ret_val;
}

/*
 * Generates a random password
 * with length equal to PASSWORD_LENGTH
 */
std::string generate_password(size_t password_length) {
  std::random_device rd;
  static const char *alpha_lower = "abcdefghijklmnopqrstuvwxyz";
  static const char *alpha_upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  static const char *special_chars = "~@#%$^&*()-_=+]}[{|;:.>,</?";
  static const char *numeric = "1234567890";

  assert(PASSWORD_LENGTH >= 20);

  if (password_length < PASSWORD_LENGTH) password_length = PASSWORD_LENGTH;

  auto get_random = [&rd](size_t size, const char *source) {
    std::string data;

    std::uniform_int_distribution<int> dist_num(0, strlen(source) - 1);

    for (size_t i = 0; i < size; i++) {
      char random = source[dist_num(rd)];

      // Make sure there are no consecutive values
      if (i == 0) {
        data += random;
      } else {
        if (random != data[i - 1])
          data += random;
        else
          i--;
      }
    }

    return data;
  };

  // Generate a password

  // Fill the password string with special chars
  std::string pwd = get_random(password_length, special_chars);

  // Replace 8 random non-consecutive chars by
  // 4 upperCase alphas and 4 lowerCase alphas
  std::string alphas = get_random(4, alpha_upper);
  alphas += get_random(4, alpha_lower);
  std::random_shuffle(alphas.begin(), alphas.end());

  std::uniform_int_distribution<int> rand_pos(0, pwd.length() - 1);
  size_t lower = 0;
  size_t step = password_length / 8;

  for (size_t index = 0; index < 8; index++) {
    std::uniform_int_distribution<int> rand_pos(lower,
                                                ((index + 1) * step) - 1);
    size_t position = rand_pos(rd);
    lower = position + 2;
    pwd[position] = alphas[index];
  }

  // Replace 3 random non-consecutive chars
  // by 3 numeric chars
  std::string numbers = get_random(3, numeric);
  lower = 0;

  step = password_length / 3;
  for (size_t index = 0; index < 3; index++) {
    std::uniform_int_distribution<int> rand_pos(lower,
                                                ((index + 1) * step) - 1);
    size_t position = rand_pos(rd);
    lower = position + 2;
    pwd[position] = numbers[index];
  }

  return pwd;
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
}  // namespace dba
}  // namespace mysqlsh
