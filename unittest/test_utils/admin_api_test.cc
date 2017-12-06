/* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms, as
   designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.
   This program is distributed in the hope that it will be useful,  but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
   the GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include "unittest/test_utils/admin_api_test.h"
#include <fstream>
#include "modules/adminapi/mod_dba_common.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "test_utils/mocks/mysqlshdk/libs/db/mock_result.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_json.h"
#include "utils/utils_string.h"

namespace tests {
void Admin_api_test::SetUp() {
  Shell_core_test_wrapper::SetUp();
  // The Admin_api_test are meant to be run using mock server
  ignore_session_notifications();
}

void Admin_api_test::add_instance_type_queries(
    std::vector<testing::Fake_result_data> *data,
    mysqlsh::dba::GRInstanceType type) {
  data->push_back(
      {"select count(*) "
       "from performance_schema.replication_group_members "
       "where MEMBER_ID = @@server_uuid AND MEMBER_STATE IS NOT NULL "
       "AND MEMBER_STATE <> 'OFFLINE';",
       {"count(*)"},
       {mysqlshdk::db::Type::Integer},
       {{type == mysqlsh::dba::Standalone ? "0" : "1"}}});

  if (type != mysqlsh::dba::Standalone) {
    data->push_back(
        {"select count(*) "
         "from mysql_innodb_cluster_metadata.instances "
         "where mysql_server_uuid = @@server_uuid",
         {"count(*)"},
         {mysqlshdk::db::Type::Integer},
         {{type == mysqlsh::dba::InnoDBCluster ? "1" : "0"}}});
  }
}

void Admin_api_test::add_get_server_variable_query(
    std::vector<testing::Fake_result_data> *data, const std::string &variable,
    mysqlshdk::db::Type type, const std::string &value) {
  data->push_back(
      {"SELECT @@" + variable, {"@@" + variable}, {type}, {{value}}});
}

void Admin_api_test::add_set_global_variable_query(
    std::vector<testing::Fake_result_data> *data, const std::string &variable,
    const std::string &value) {
  std::string query, query_raw = "SET GLOBAL " + variable + " = ?";
  query = shcore::sqlstring(query_raw.c_str(), 0) << value;

  data->push_back({query, {}, {}, {}});
}

void Admin_api_test::add_show_databases_query(
    std::vector<testing::Fake_result_data> *data, const std::string &variable,
    const std::string &value) {
  data->push_back({"show databases like '" + variable + "'",
                   {variable},
                   {mysqlshdk::db::Type::String},
                   {{value}}});
}

void Admin_api_test::add_replication_filters_query(
    std::vector<testing::Fake_result_data> *data,
    const std::string &binlog_do_db, const std::string &binlog_ignore_db) {
  data->push_back(
      {"SHOW MASTER STATUS",
       {"File", "Position", "Binlog_Do_DB", "Binlog_Ignore_DB",
        "Executed_Gtid_Set"},
       {mysqlshdk::db::Type::String, mysqlshdk::db::Type::Integer,
        mysqlshdk::db::Type::String, mysqlshdk::db::Type::String,
        mysqlshdk::db::Type::String},
       {{"", "0", binlog_do_db.c_str(), binlog_ignore_db.c_str(), ""}}});
}

void Admin_api_test::add_ps_gr_group_members_query(
    std::vector<testing::Fake_result_data> *data,
    const std::vector<std::vector<std::string>> &values) {
  data->push_back(
      {"SELECT member_id FROM performance_schema.replication_group_members",
       {"member_id"},
       {mysqlshdk::db::Type::String},
       {values}});
}

void Admin_api_test::add_ps_gr_group_members_full_query(
    std::vector<testing::Fake_result_data> *data, const std::string &member_id,
    const std::vector<std::vector<std::string>> &values) {
  data->push_back(
      {"SELECT MEMBER_ID, MEMBER_HOST, MEMBER_PORT FROM "
       "performance_schema.replication_group_members "
       "WHERE MEMBER_ID = '" +
           member_id + "'",
       {"MEMBER_ID", "MEMBER_HOST", "MEMBER_PORT"},
       {mysqlshdk::db::Type::String, mysqlshdk::db::Type::String,
        mysqlshdk::db::Type::Integer},
       {values}});
}

void Admin_api_test::add_md_group_members_query(
    std::vector<testing::Fake_result_data> *data,
    const std::vector<std::vector<std::string>> &values) {
  data->push_back(
      {"SELECT mysql_server_uuid FROM mysql_innodb_cluster_metadata.instances "
       "WHERE replicaset_id = 1",
       {"mysql_server_uuid"},
       {mysqlshdk::db::Type::String},
       {values}});
}

void Admin_api_test::add_md_group_members_full_query(
    std::vector<testing::Fake_result_data> *data,
    const std::string &mysql_server_uuid,
    const std::vector<std::vector<std::string>> &values) {
  data->push_back(
      {"SELECT mysql_server_uuid, instance_name, "
       "JSON_UNQUOTE(JSON_EXTRACT(addresses, \"$.mysqlClassic\")) AS host "
       "FROM mysql_innodb_cluster_metadata.instances "
       "WHERE mysql_server_uuid = '" +
           mysql_server_uuid + "'",
       {"mysql_server_uuid", "instance_name", "host"},
       {mysqlshdk::db::Type::String, mysqlshdk::db::Type::String,
        mysqlshdk::db::Type::String},
       {values}});
}

void Admin_api_test::add_gr_primary_member_query(
    std::vector<testing::Fake_result_data> *data,
    const std::string &primary_uuid) {
  data->push_back(
      {"SELECT variable_value "
       "FROM performance_schema.global_status "
       "WHERE variable_name = 'group_replication_primary_member'",
       {"variable_value"},
       {mysqlshdk::db::Type::String},
       {{primary_uuid}}});
}

void Admin_api_test::add_member_state_query(
    std::vector<testing::Fake_result_data> *data, const std::string &address,
    const std::string &mysql_server_uuid, const std::string &instance_name,
    const std::string &member_state) {
  data->push_back(
      {"SELECT mysql_server_uuid, instance_name, member_state "
       "FROM mysql_innodb_cluster_metadata.instances "
       "LEFT JOIN performance_schema.replication_group_members "
       "ON `mysql_server_uuid`=`member_id` "
       "WHERE addresses->\"$.mysqlClassic\" = '" +
           address + "'",
       {"mysql_server_uuid", "instance_name", "member_state"},
       {mysqlshdk::db::Type::String, mysqlshdk::db::Type::String,
        mysqlshdk::db::Type::String},
       {{mysql_server_uuid, instance_name, member_state}}});
}

void Admin_api_test::add_md_group_name_query(
    std::vector<testing::Fake_result_data> *data, const std::string &value) {
  data->push_back(
      {"SELECT JSON_UNQUOTE(JSON_EXTRACT(attributes, "
       "\"$.group_replication_group_name\")) AS group_replication_group_name "
       "FROM mysql_innodb_cluster_metadata.replicasets "
       "WHERE replicaset_id = 1",
       {"group_replication_group_name"},
       {mysqlshdk::db::Type::String},
       {{value}}});
}

void Admin_api_test::add_get_cluster_matching_query(
    std::vector<testing::Fake_result_data> *data,
    const std::string &cluster_name) {
  data->push_back(
      {"SELECT cluster_id, cluster_name, default_replicaset, description, "
       "options, attributes FROM mysql_innodb_cluster_metadata.clusters "
       "WHERE cluster_name = '" +
           cluster_name + "'",
       {"cluster_id", "cluster_name", "default_replicaset", "description",
        "options", "attributes"},
       {mysqlshdk::db::Type::Integer, mysqlshdk::db::Type::String,
        mysqlshdk::db::Type::Integer, mysqlshdk::db::Type::Bytes,
        mysqlshdk::db::Type::Json, mysqlshdk::db::Type::Json},
       {{"1", cluster_name.c_str(), "1", "Test Cluster", "null",
         "{\"default\": true}"}}});
}

void Admin_api_test::add_get_replicaset_query(
    std::vector<testing::Fake_result_data> *data,
    const std::string &replicaset_name) {
  data->push_back(
      {"SELECT replicaset_name, topology_type FROM "
       "mysql_innodb_cluster_metadata.replicasets WHERE "
       "replicaset_id = 1",
       {"replicaset_name", "topology_type"},
       {mysqlshdk::db::Type::String, mysqlshdk::db::Type::Enum},
       {{"default", "pm"}}});
}

void Admin_api_test::add_is_instance_on_rs_query(
    std::vector<testing::Fake_result_data> *data,
    const std::string &replicaset_id, const std::string &instance_address) {
  data->push_back(
      {"SELECT COUNT(*) as count FROM mysql_innodb_cluster_metadata.instances "
       "WHERE replicaset_id = " +
           replicaset_id +
           " AND "
           "addresses->'$.mysqlClassic' = '" +
           instance_address + "'",
       {"count"},
       {mysqlshdk::db::Type::Integer},
       {{"1"}}});
}

void Admin_api_test::add_precondition_queries(
    std::vector<testing::Fake_result_data> *data,
    mysqlsh::dba::GRInstanceType instance_type,
    nullable<std::string> primary_uuid, nullable<std::string> instance_uuid,
    nullable<std::string> instance_state, nullable<int> instance_count,
    nullable<int> unreachable_count) {
  add_instance_type_queries(data, instance_type);

  if (instance_type == mysqlsh::dba::Standalone && !primary_uuid.is_null())
    throw std::logic_error(
        "There is not primary UUID on a standalone instance");

  if (primary_uuid) {
    std::string primary_id = *primary_uuid;
    std::string member_id = *primary_uuid;

    if (!instance_uuid.is_null())
      member_id = *instance_uuid;

    data->push_back(
        {"SELECT @@server_uuid, VARIABLE_VALUE FROM "
         "performance_schema.global_status WHERE VARIABLE_NAME "
         "= 'group_replication_primary_member';",
         {"@@server_uuid", "VARIABLE_VALUE"},
         {mysqlshdk::db::Type::String, mysqlshdk::db::Type::String},
         {{member_id.c_str(), primary_id.c_str()}}});

    if (!instance_state.is_null()) {
      data->push_back(
          {"SELECT MEMBER_STATE FROM "
           "performance_schema.replication_group_members "
           "WHERE MEMBER_ID = '" +
               member_id + "'",
           {"MEMBER_STATE"},
           {mysqlshdk::db::Type::String},
           {{*instance_state}}});
    }

    if (!instance_count.is_null() && !unreachable_count.is_null()) {
      std::string count = std::to_string(*instance_count);
      std::string unreachable = std::to_string(*unreachable_count);

      data->push_back(
          {"SELECT CAST(SUM(IF(member_state = 'UNREACHABLE', 1, 0)) AS SIGNED) "
           "AS UNREACHABLE,  COUNT(*) AS TOTAL FROM "
           "performance_schema.replication_group_members",
           {"SIGNED", "UNREACHABLE"},
           {mysqlshdk::db::Type::Integer, mysqlshdk::db::Type::Integer},
           {{unreachable, count}}});
    }
  }
}

void Admin_api_test::add_super_read_only_queries(
    std::vector<testing::Fake_result_data> *data, bool super_read_only,
    bool query_open_sessions,
    std::vector<std::vector<std::string>> open_sessions) {
  add_get_server_variable_query(data, "super_read_only",
                                mysqlshdk::db::Type::Integer,
                                super_read_only ? "1" : "0");

  if (query_open_sessions) {
    data->push_back(
        {"SELECT CONCAT(PROCESSLIST_USER, '@', PROCESSLIST_HOST) AS acct, "
         "COUNT(*) FROM performance_schema.threads WHERE type = 'foreground' "
         "GROUP BY acct;",
         {"acct", "COUNT(*)"},
         {mysqlshdk::db::Type::String, mysqlshdk::db::Type::Integer},
         open_sessions});
  }
}

void Admin_api_test::add_is_gtid_subset_query(
    std::vector<testing::Fake_result_data> *data, const std::string &subset,
    const std::string &set, bool success) {
  std::string query = shcore::sqlstring("SELECT GTID_SUBSET(?, ?)", 0)
                      << subset << set;

  data->push_back({query,
                   {"GTID_SUBSET"},
                   {mysqlshdk::db::Type::Integer},
                   {{success ? "1" : "0"}}});
}

/**
 * Adds the queries about cluster admin privileges based on the received
 * parameters
 * @param data result vector that will hold the queries and data added by this
 * function.
 * @param user: the user to be used on the queries
 * @param host: the host to be used on the queries
 * @param non_grantable: a formatted string with the privileges to
 * be marked as non grantable, format is as follows:
 *   GLOBAL_LIST|MD_LIST|MYSQL_LIST
 * Where each of the *LIST components is a comma separated list if the
 * privileges to be marked as non grantable for the specific domain (Global,
 * Metadata, MySQL)
 *
 * @param missing: a formatted string with the privileges to
 * be marked as missing, format is as follows:
 *   GLOBAL_LIST|MD_LIST|MYSQL_LIST
 * Where each of the *LIST components is a comma separated list if the
 * privileges to be marked as missing for the specific domain (Global, Metadata,
 * MySQL)
 */
void Admin_api_test::add_validate_cluster_admin_user_privileges_queries(
    std::vector<testing::Fake_result_data> *data, const std::string &user,
    const std::string &host, const std::string &non_grantable,
    const std::string &missing) {
  std::vector<std::string> ng_global;
  std::vector<std::string> ng_metadata;
  std::vector<std::string> ng_mysql;
  std::vector<std::string> missing_global;
  std::vector<std::string> missing_metadata;
  std::vector<std::string> missing_mysql;

  // Parses the privileges to be marked as non grantable
  if (!non_grantable.empty()) {
    auto ng_list = shcore::split_string(non_grantable, "|", false);
    assert(ng_list.size() == 3);

    if (!ng_list[0].empty())
      ng_global = shcore::split_string(ng_list[0], ",", true);
    if (!ng_list[1].empty())
      ng_metadata = shcore::split_string(ng_list[1], ",", true);
    if (!ng_list[2].empty())
      ng_mysql = shcore::split_string(ng_list[2], ",", true);
  }

  // Parses the privileges to be missing
  if (!missing.empty()) {
    auto missing_list = shcore::split_string(missing, "|", false);
    assert(missing_list.size() == 3);

    if (!missing_list[0].empty())
      missing_global = shcore::split_string(missing_list[0], ",", true);
    if (!missing_list[1].empty())
      missing_metadata = shcore::split_string(missing_list[1], ",", true);
    if (!missing_list[2].empty())
      missing_mysql = shcore::split_string(missing_list[2], ",", true);
  }

  // Creates the list of global privileges to be returned on the
  // query for the globals;

  // Global privileges includes all in k_global_privileges and
  // k_metadata_schema_privileges
  std::vector<std::vector<std::string>> global_privileges;
  std::set<std::string> all_globals;
  all_globals.insert(mysqlsh::dba::k_global_privileges.begin(),
                     mysqlsh::dba::k_global_privileges.end());
  all_globals.insert(mysqlsh::dba::k_metadata_schema_privileges.begin(),
                     mysqlsh::dba::k_metadata_schema_privileges.end());

  for (const auto &privilege : all_globals) {
    // Skips the privileges marked as missing
    if (std::find(missing_global.begin(), missing_global.end(), privilege) ==
        missing_global.end()) {
      // Creates a record for each of the remaining ones with the
      // indicated grantable value
      if (std::find(ng_global.begin(), ng_global.end(), privilege) ==
          ng_global.end()) {
        global_privileges.push_back({privilege, "YES"});
      } else {
        global_privileges.push_back({privilege, "NO"});
      }
    }
  }

  // Creates the expected query as passing the records to be returned
  shcore::sqlstring query;
  data->push_back({"SELECT CURRENT_USER()",
                  {"CURRENT_USER()"},
                  {mysqlshdk::db::Type::String},
                  {{shcore::make_account(user, host)}}});

  query = shcore::sqlstring(
            "SELECT COUNT(*) from mysql.user WHERE User = ? AND Host = '%'", 0)
          << user;
  data->push_back({query.str(),
                  {"COUNT(*)"},
                  {mysqlshdk::db::Type::Integer},
                  {{"0"}}});

  query = shcore::sqlstring(
              "SELECT privilege_type, is_grantable"
              " FROM information_schema.user_privileges"
              " WHERE grantee = ?",
              0)
          << shcore::make_account(user, host);

  data->push_back({query.str(),
                   {"privilege_type", "is_grantable"},
                   {mysqlshdk::db::Type::String, mysqlshdk::db::Type::String},
                   global_privileges});

  // If any of the global privileges is either missing or non grantable
  // a second query will be sent asking about the schema privileges
  if (!ng_global.empty() || !missing_global.empty()) {
    std::vector<std::vector<std::string>> schema_privileges;

    // Creates the  list of records to be returned for the metadata schema
    for (const auto &privilege : mysqlsh::dba::k_metadata_schema_privileges) {
      // Skips privileges marked as missing for the metadata schema
      if (std::find(missing_metadata.begin(), missing_metadata.end(),
                    privilege) == missing_metadata.end()) {
        if (std::find(ng_metadata.begin(), ng_metadata.end(), privilege) ==
            ng_metadata.end()) {
          global_privileges.push_back(
              {"mysql_innodb_cluster_metadata", privilege, "YES"});
        } else {
          schema_privileges.push_back(
              {"mysql_innodb_cluster_metadata", privilege, "NO"});
        }
      }
    }

    // Creates the  list of records to be returned for the metadata schema
    for (const auto &privilege : mysqlsh::dba::k_mysql_schema_privileges) {
      // Skips privileges marked as missing for the mysql schema
      if (std::find(missing_mysql.begin(), missing_mysql.end(), privilege) ==
          missing_mysql.end()) {
        if (std::find(ng_mysql.begin(), ng_mysql.end(), privilege) ==
            ng_mysql.end()) {
          global_privileges.push_back({"mysql", privilege, "YES"});
        } else {
          schema_privileges.push_back({"mysql", privilege, "NO"});
        }
      }
    }

    // Adds the second query with the records to be returned
    query = shcore::sqlstring(
                "SELECT table_schema, privilege_type, is_grantable"
                " FROM information_schema.schema_privileges"
                " WHERE grantee = ?",
                0)
            << shcore::make_account(user, host);

    data->push_back({query.str(),
                     {"table_schema", "privilege_type", "is_grantable"},
                     {mysqlshdk::db::Type::String, mysqlshdk::db::Type::String,
                      mysqlshdk::db::Type::String},
                     schema_privileges});
  }
}

/**
 * Add the "fake" queries data for the get_peer_seeds() function to be used
 * by mock tests.
 *
 * @param data vector that will be changed by the function to include all the
 *             "fake" (mock) data results.
 * @param metada_values "fake" values returned by the query to the metadata
 *                      with the known GR addresses.
 * @param gr_group_seed_value "fake" value returned by the
 *                            group_replication_group_seeds variable.
 * @param instance_address target instance address to use in the queries.
 */
void Admin_api_test::add_get_peer_seeds_queries(
    std::vector<testing::Fake_result_data> *data,
    const std::vector<std::vector<std::string>> &metada_values,
    const std::string &gr_group_seed_value,
    const std::string &instance_address) {
  data->push_back({"SELECT @@global.group_replication_group_seeds",
                   {"group_replication_group_seeds"},
                   {mysqlshdk::db::Type::String},
                   {
                       {{gr_group_seed_value}}
                   }});
  data->push_back({
      "SELECT JSON_UNQUOTE(addresses->'$.grLocal') "
      "FROM mysql_innodb_cluster_metadata.instances "
      "WHERE addresses->'$.mysqlClassic' <> '" + instance_address + "' "
      "AND replicaset_id IN (SELECT replicaset_id "
      "FROM mysql_innodb_cluster_metadata.instances "
      "WHERE addresses->'$.mysqlClassic' = '" + instance_address + "')",
      {"JSON_UNQUOTE(addresses->'$.grLocal')"},
      {mysqlshdk::db::Type::String},
      {metada_values}});
}

}  // namespace tests
