/* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include "unittest/test_utils/admin_api_test.h"
#include <fstream>
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
  data->push_back({
    "SELECT cluster_id, cluster_name, default_replicaset, description, "
    "options, attributes FROM mysql_innodb_cluster_metadata.clusters "
    "WHERE cluster_name = '" + cluster_name + "'",
    {"cluster_id", "cluster_name", "default_replicaset", "description",
     "options", "attributes"},
    {mysqlshdk::db::Type::Integer, mysqlshdk::db::Type::String,
     mysqlshdk::db::Type::Integer, mysqlshdk::db::Type::Bytes,
     mysqlshdk::db::Type::Json, mysqlshdk::db::Type::Json},
    {
      {"1", cluster_name.c_str(), "1", "Test Cluster", "null",
        "{\"default\": true}"}
    }
  });
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
        nullable<std::string> primary_uuid,
        nullable<std::string> instance_uuid,
        nullable<std::string> instance_state,
        nullable<int> instance_count,
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
            {"SELECT MEMBER_STATE FROM performance_schema.replication_group_members "
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

}  // namespace tests
