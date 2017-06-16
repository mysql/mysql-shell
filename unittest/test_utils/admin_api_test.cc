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

#include <fstream>
#include "unittest/test_utils/admin_api_test.h"
#include "mysqlshdk/include/scripting/types.h"
#include "utils/utils_general.h"
#include "utils/utils_json.h"
#include "utils/utils_file.h"
#include "utils/utils_string.h"
#include "test_utils/mocks/mysqlshdk/libs/db/mock_result.h"

namespace tests {
void Admin_api_test::SetUp() {
  Shell_core_test_wrapper::SetUp();

  std::vector<std::string> path_components = {_sandbox_dir,
    _mysql_sandbox_port1, "my.cnf"};
  _sandbox_cnf_1 = shcore::str_join(path_components, _path_splitter);

  path_components[1] = _mysql_sandbox_port2;
  _sandbox_cnf_2 = shcore::str_join(path_components, _path_splitter);

  path_components[1] = _mysql_sandbox_port3;
  _sandbox_cnf_3 = shcore::str_join(path_components, _path_splitter);
}

void Admin_api_test::add_instance_type_queries
  (std::vector<testing::Fake_result_data> *data,
   mysqlsh::dba::GRInstanceType type) {
  data->push_back({
    "select count(*) "
    "from performance_schema.replication_group_members "
    "where MEMBER_ID = @@server_uuid AND MEMBER_STATE IS NOT NULL "
    "AND MEMBER_STATE <> 'OFFLINE';",
    {"count(*)"},
    {mysqlshdk::db::Type::LongLong},
    {
      {type == mysqlsh::dba::Standalone ? "0" : "1"}
    }
  });

  if (type != mysqlsh::dba::Standalone) {
  data->push_back({
    "select count(*) "
    "from mysql_innodb_cluster_metadata.instances "
    "where mysql_server_uuid = @@server_uuid",
    {"count(*)"},
    { mysqlshdk::db::Type::LongLong},
    {
      {type == mysqlsh::dba::InnoDBCluster ? "1" : "0"}
    }
  });
  }
}

void Admin_api_test::add_get_server_variable_query(
    std::vector<testing::Fake_result_data> *data,
    const std::string& variable,
    mysqlshdk::db::Type type,
    const std::string& value) {
  data->push_back({
    "SELECT @@" + variable,
    {"@@" + variable},
    {type},
    {
      {value}
    }
  });
}

void Admin_api_test::add_replication_filters_query(
    std::vector<testing::Fake_result_data> *data,
    const std::string& binlog_do_db,
    const std::string& binlog_ignore_db) {
  data->push_back({
    "SHOW MASTER STATUS",
    {"File", "Position", "Binlog_Do_DB", "Binlog_Ignore_DB",
      "Executed_Gtid_Set"},
    { 
      mysqlshdk::db::Type::VarString, 
      mysqlshdk::db::Type::LongLong, 
      mysqlshdk::db::Type::VarString, 
      mysqlshdk::db::Type::VarString,
      mysqlshdk::db::Type::VarString
    },
    {
      {"", "0", binlog_do_db.c_str(), binlog_ignore_db.c_str(), ""}
    }
  });
}
}  // namespace tests
