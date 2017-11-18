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
#include "modules/adminapi/mod_dba.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/admin_api_test.h"
#include "unittest/test_utils/mocks/modules/adminapi/mock_dba.h"

namespace testing {

class Dba_test : public tests::Admin_api_test {
 protected:
  virtual void SetUp() {
    Admin_api_test::SetUp();

    // Resets the shell in non interactive mode
    _options->session_type = mysqlsh::SessionType::Classic;
    _options->uri = "user:@localhost:" + _mysql_sandbox_port1;
    _options->wizards = false;
    reset_shell();
    _mock_started = false;
  }

  virtual void TearDown() {
    if (_mock_started)
      stop_server_mock(_mysql_sandbox_nport1);

    Admin_api_test::TearDown();
  }

  // Initializes the server mock, establishes the connection
  // and retrieves the global Dba object
  void start_mocks(bool start_mock_server) {
    if (start_mock_server) {
      start_server_mock(_mysql_sandbox_nport1, _queries);
      _mock_started = true;
    }

    _dba.initialize(_interactive_shell->shell_context().get(), true);

    std::shared_ptr<mysqlsh::dba::Dba> shared_dba(&_dba, SharedDoNotDelete());
    auto dba = shcore::Value(
        std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(shared_dba));
    _interactive_shell->shell_context()->set_global("dba", dba);
  }

  std::shared_ptr<mysqlsh::mysql::ClassicSession> get_classic_session() {
    auto session = _interactive_shell->shell_context()->get_dev_session();
    return std::dynamic_pointer_cast<mysqlsh::mysql::ClassicSession>(session);
  }

  StrictMock<Mock_dba> _dba;
  std::vector<testing::Fake_result_data> _queries;
  bool _mock_started;
};

TEST_F(Dba_test, create_cluster_with_cluster_admin_type) {
  // Sets the required statements for the session
  add_instance_type_queries(&_queries, mysqlsh::dba::Standalone);
  add_replication_filters_query(&_queries, "", "");
  add_get_server_variable_query(&_queries, "GLOBAL.require_secure_transport",
                                mysqlshdk::db::Type::Integer, "1");

  start_mocks(true);

  _interactive_shell->connect(_options->connection_options());

  shcore::Argument_list args;
  args.push_back(shcore::Value("dev"));

  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["clusterAdminType"] = shcore::Value("local");
  args.push_back(shcore::Value(map));

  try {
    _dba.create_cluster(args);
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS(
        "Dba.createCluster: Invalid values in the options: clusterAdminType",
        error);
  }

  _interactive_shell->shell_context()->get_dev_session()->close();
}

// Regression test for BUG #26159339: SHELL: ADMINAPI DOES NOT TAKE
// GROUP_NAME INTO ACCOUNT
TEST_F(Dba_test, get_cluster_with_invalid_gr_group_name_001) {
  // @@group_replication_group_name (instance)
  //-------------------------------------
  // fd4b70e8-5cb1-11e7-a68b-b86b230042b9
  //-------------------------------------

  // group_replication_group_name (metadata)
  // ---------------------------------------
  // fd4b70e8-5cb1-11e7-a68b-b86b230042b0
  // ---------------------------------------

  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
                           std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
                           {}, std::string("ONLINE"), 3, 0);

  add_get_server_variable_query(&_queries, "group_replication_group_name",
                                mysqlshdk::db::Type::String,
                                "fd4b70e8-5cb1-11e7-a68b-b86b230042b9");

  add_show_databases_query(&_queries, "mysql_innodb_cluster_metadata",
                           "mysql_innodb_cluster_metadata");

  add_md_group_name_query(&_queries, "fd4b70e8-5cb1-11e7-a68b-b86b230042b0");

  start_mocks(true);

  _interactive_shell->connect(_options->connection_options());

  shcore::Argument_list args;
  args.push_back(shcore::Value("testCluster"));

  auto cluster = _dba.create_mock_cluster("testCluster")
                     .as_object<mysqlsh::dba::Cluster>();
  EXPECT_CALL(_dba.get_metadata(), get_cluster("testCluster"))
      .WillRepeatedly(Return(cluster));

  try {
    _dba.get_cluster(args);
  } catch (const shcore::Exception &e) {
    std::string error = e.what();

    std::string instance_address =
        "localhost:" + std::to_string(_mysql_sandbox_nport1);

    MY_EXPECT_OUTPUT_CONTAINS(
        "Dba.getCluster: Dba.getCluster: Unable to get cluster. The instance "
        "'" +
            instance_address +
            "' may belong to a different ReplicaSet as the "
            "one registered in the Metadata since the value of "
            "'group_replication_group_name' does not match the one registered "
            "in the "
            "ReplicaSet's Metadata: possible split-brain scenario. Please "
            "connect to "
            "another member of the ReplicaSet to get the Cluster.",
        error);
  }

  _interactive_shell->shell_context()->get_dev_session()->close();
}

// ----------------------------


class Dba_preconditions: public Dba_test {
  virtual void set_preconditions() = 0;

  virtual void SetUp() {
    Dba_test::SetUp();

    set_preconditions();

    start_mocks(true);

    _interactive_shell->connect(_options->connection_options());
  }

  virtual void TearDown() {
    _interactive_shell->shell_context()->get_dev_session()->close();

    Dba_test::TearDown();
  }
};


class Dba_preconditions_standalone : public Dba_preconditions {
  virtual void set_preconditions() {
    add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});
  }
};


TEST_F(Dba_preconditions_standalone, get_cluster_fails) {
  EXPECT_THROW_LIKE(_dba.get_cluster({}),
                    shcore::Exception,
                    "Dba.getCluster: This function is not available through a "
                    "session to a standalone instance");
}

TEST_F(Dba_preconditions_standalone, create_cluster_succeeds) {
  // Create Cluster is allowed on standalone instance, the precondition
  // validation passes
  shcore::Argument_list args;
  args.push_back(shcore::Value("1nvalidName"));
  EXPECT_THROW_LIKE(_dba.create_cluster(args),
                    shcore::Exception,
                    "Dba.createCluster: The Cluster name can only start with "
                    "an alphabetic or the '_' character.");
}

TEST_F(Dba_preconditions_standalone, drop_metadata_schema_fails) {
  // getCluster is not allowed on standalone instances
  shcore::Argument_list args;
  args.push_back(shcore::Value::new_map());
  EXPECT_THROW_LIKE(_dba.drop_metadata_schema(args),
                    shcore::Exception,
                    "Dba.dropMetadataSchema: This function is not available "
                    "through a session to a standalone instance");
}

TEST_F(Dba_preconditions_standalone,
       reboot_cluster_from_complete_outage_succeeds) {
  std::shared_ptr<mysqlsh::dba::Cluster> nothing;
  EXPECT_CALL(_dba.get_metadata(), get_default_cluster()).
    WillOnce(Return(nothing));

  EXPECT_THROW_LIKE(_dba.reboot_cluster_from_complete_outage({}),
                    shcore::Exception,
                    "Dba.rebootClusterFromCompleteOutage: No default cluster "
                    "is configured.");
}

class Dba_preconditions_unmanaged_gr : public Dba_preconditions {
  virtual void set_preconditions() {
    add_precondition_queries(&_queries, mysqlsh::dba::GroupReplication,
                           std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
                           {}, std::string("ONLINE"), 3, 0);
  }
};


TEST_F(Dba_preconditions_unmanaged_gr, get_cluster_fails) {
  // getCluster is not allowed on standalone instances
  EXPECT_THROW_LIKE(_dba.get_cluster({}),
                    shcore::Exception,
                    "Dba.getCluster: This function is not available through a "
                    "session to an instance belonging to an unmanaged "
                    "replication group");
}

TEST_F(Dba_preconditions_unmanaged_gr, create_cluster_succeeds) {
  // Create Cluster is allowed on standalone instance, the precondition
  // validation passes
  shcore::Argument_list args;
  args.push_back(shcore::Value("1nvalidName"));
  EXPECT_THROW_LIKE(_dba.create_cluster(args),
                    shcore::Exception,
                    "Dba.createCluster: The Cluster name can only start with "
                    "an alphabetic or the '_' character.");
}

TEST_F(Dba_preconditions_unmanaged_gr, drop_metadata_schema_fails) {
  // getCluster is not allowed on standalone instances
  shcore::Argument_list args;
  args.push_back(shcore::Value::new_map());
  EXPECT_THROW_LIKE(_dba.drop_metadata_schema(args),
                    shcore::Exception,
                    "Dba.dropMetadataSchema: This function is not available "
                    "through a session to an instance belonging to an "
                    "unmanaged replication group");
}

TEST_F(Dba_preconditions_unmanaged_gr,
       reboot_cluster_from_complete_outage_succeeds) {
  std::shared_ptr<mysqlsh::dba::Cluster> nothing;
  EXPECT_CALL(_dba.get_metadata(), get_default_cluster()).
    WillOnce(Return(nothing));

  EXPECT_THROW_LIKE(_dba.reboot_cluster_from_complete_outage({}),
                    shcore::Exception,
                    "Dba.rebootClusterFromCompleteOutage: No default cluster "
                    "is configured.");
}

class Dba_preconditions_innodb : public Dba_preconditions {
  virtual void set_preconditions() {
    add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
                           std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
                           {}, std::string("ONLINE"), 3, 0);
  }
};


TEST_F(Dba_preconditions_innodb, get_cluster_succeeds) {
  std::shared_ptr<mysqlsh::dba::Cluster> nothing;
  EXPECT_CALL(_dba.get_metadata(), get_default_cluster()).
    WillOnce(Return(nothing));

  EXPECT_THROW_LIKE(_dba.get_cluster({}),
                    shcore::Exception,
                    "Dba.getCluster: No default cluster is configured.");
}

TEST_F(Dba_preconditions_innodb, create_cluster_fails) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("1nvalidName"));
  EXPECT_THROW_LIKE(_dba.create_cluster(args),
                    shcore::Exception,
                    "Dba.createCluster: Unable to create cluster. The instance "
                    "'localhost:" + _mysql_sandbox_port1 + "' already belongs "
                    "to an InnoDB cluster. Use <Dba>.getCluster() to access "
                    "it.");
}

TEST_F(Dba_preconditions_innodb, drop_metadata_schema_succeeds) {
  shcore::Argument_list args;
  args.push_back(shcore::Value::new_map());
  EXPECT_THROW_LIKE(_dba.drop_metadata_schema(args),
                    shcore::Exception,
                    "Dba.dropMetadataSchema: No operation executed, use the "
                    "'force' option");
}

TEST_F(Dba_preconditions_innodb, reboot_cluster_from_complete_outage_succeeds) {
  std::shared_ptr<mysqlsh::dba::Cluster> nothing;
  EXPECT_CALL(_dba.get_metadata(), get_default_cluster()).
    WillOnce(Return(nothing));

  EXPECT_THROW_LIKE(_dba.reboot_cluster_from_complete_outage({}),
                    shcore::Exception,
                    "Dba.rebootClusterFromCompleteOutage: No default cluster "
                    "is configured.");
}
}  // namespace testing
