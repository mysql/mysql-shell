/*
* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "unittest/test_utils/admin_api_test.h"
#include "modules/adminapi/mod_dba.h"

namespace tests {

class Dba_test: public Admin_api_test {
 protected:
  virtual void SetUp() {
    Admin_api_test::SetUp();

    // Resets the shell in non interactive mode
    _options->session_type = mysqlsh::SessionType::Classic;
    _options->uri = "user:@localhost:" + _mysql_sandbox_port1;
    _options->wizards = false;
    reset_shell();
  }

  virtual void TearDown() {
    _interactive_shell->shell_context()
                      ->get_dev_session()
                      ->close();

    stop_server_mock(_mysql_sandbox_nport1);

    Admin_api_test::TearDown();
  }

  // Initializes the server mock, establishes the connection
  // and retrieves the global Dba object
  void init_test() {
    start_server_mock(_mysql_sandbox_nport1, _queries);

    _interactive_shell->connect(true);

    auto dba_value = _interactive_shell->shell_context()->get_global("dba");
    _dba = dba_value.as_object<mysqlsh::dba::Dba>();
  }

  std::shared_ptr<mysqlsh::mysql::ClassicSession> get_classic_session() {
    auto session = _interactive_shell->shell_context()->get_dev_session();
    return std::dynamic_pointer_cast<mysqlsh::mysql::ClassicSession>(session);
  }

  std::shared_ptr<mysqlsh::dba::Dba> _dba;
  std::vector<testing::Fake_result_data> _queries;
};

TEST_F(Dba_test, create_cluster_with_cluster_admin_type) {
  // Sets the required statements for the session
  add_instance_type_queries(&_queries, mysqlsh::dba::Standalone);
  add_replication_filters_query(&_queries, "", "");
  add_get_server_variable_query(&_queries,
                                "GLOBAL.require_secure_transport",
                                mysqlshdk::db::Type::LongLong, "1");

  init_test();

  shcore::Argument_list args;
  args.push_back(shcore::Value("dev"));

  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["clusterAdminType"] = shcore::Value("local");
  args.push_back(shcore::Value(map));

  try {
    _dba->create_cluster(args);
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS("Dba.createCluster: Invalid values in the options: clusterAdminType" , error);
  }
}

}  // namespace tests
