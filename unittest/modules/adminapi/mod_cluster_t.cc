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

using mysqlsh::dba::MetadataStorage;

class Cluster_test : public tests::Admin_api_test {
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
  }

  void reset_cluster(const std::string& name) {
    start_mocks(true);

    _interactive_shell->connect(true);

    EXPECT_CALL(_mock_metadata, set_session(_)).
      WillOnce(Invoke(&_mock_metadata, &MetadataStorage::set_session));

    _mock_metadata.set_session(_interactive_shell
      ->shell_context()->get_dev_session());

    std::shared_ptr<StrictMock<Mock_metadata_storage>> metadata(
      &_mock_metadata, SharedDoNotDelete());
    auto md_storage =
        std::dynamic_pointer_cast<MetadataStorage>(metadata);
     _cluster.initialize(name, md_storage);
  }

  std::shared_ptr<mysqlsh::mysql::ClassicSession> get_classic_session() {
    auto session = _interactive_shell->shell_context()->get_dev_session();
    return std::dynamic_pointer_cast<mysqlsh::mysql::ClassicSession>(session);
  }

  std::shared_ptr<mysqlsh::ShellBaseSession> create_base_session(int port) {
    std::shared_ptr<mysqlsh::ShellBaseSession> session;

    mysqlshdk::db::Connection_options session_args;
    session_args.set_host("localhost");
    session_args.set_port(port);
    session_args.set_user("user");
    session_args.set_password("");

    session = mysqlsh::Shell::connect_session(session_args,
                                              mysqlsh::SessionType::Classic);

    return session;
  }

  StrictMock<Mock_cluster> _cluster;
  std::vector<testing::Fake_result_data> _queries;
  StrictMock<Mock_metadata_storage> _mock_metadata;

  bool _mock_started;
};


// ----------------------------


class Cluster_preconditions: public Cluster_test {
  virtual void set_preconditions() = 0;

  virtual void SetUp() {
    Cluster_test::SetUp();

    set_preconditions();

    reset_cluster("dev");
  }

  virtual void TearDown() {
    _interactive_shell->shell_context()->get_dev_session()->close();

    Cluster_test::TearDown();
  }
};


class Cluster_preconditions_standalone : public Cluster_preconditions {
  virtual void set_preconditions() {
    add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});
    add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});
    add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});
    add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});
    add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});
    add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});
    add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});
    add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});
    add_precondition_queries(&_queries, mysqlsh::dba::Standalone, {});
  }
};

TEST_F(Cluster_preconditions_standalone, all_functions) {
  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("some@instance:4321"));
    EXPECT_THROW_LIKE(_cluster.add_instance(args),
                      shcore::Exception,
                      "Cluster.addInstance: This function is not available "
                      "through a session to a standalone instance");
  }

  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("some@instance:4321"));
    EXPECT_THROW_LIKE(_cluster.rejoin_instance(args),
                      shcore::Exception,
                      "Cluster.rejoinInstance: This function is not available "
                      "through a session to a standalone instance");
  }

  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("some@instance:4321"));
    EXPECT_THROW_LIKE(_cluster.remove_instance(args),
                      shcore::Exception,
                      "Cluster.removeInstance: This function is not available "
                      "through a session to a standalone instance");
  }

  {
    EXPECT_THROW_LIKE(_cluster.describe({}),
                      shcore::Exception,
                      "Cluster.describe: This function is not available "
                      "through a session to a standalone instance");
  }

  {
    EXPECT_THROW_LIKE(_cluster.status({}),
                      shcore::Exception,
                      "Cluster.status: This function is not available "
                      "through a session to a standalone instance");
  }

  {
    EXPECT_THROW_LIKE(_cluster.dissolve({}),
                      shcore::Exception,
                      "Cluster.dissolve: This function is not available "
                      "through a session to a standalone instance");
  }

  {
    EXPECT_THROW_LIKE(_cluster.rescan({}),
                      shcore::Exception,
                      "Cluster.rescan: This function is not available "
                      "through a session to a standalone instance");
  }

  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("some@instance:4321"));
    EXPECT_THROW_LIKE(_cluster.force_quorum_using_partition_of(args),
                      shcore::Exception,
                      "Cluster.forceQuorumUsingPartitionOf: This function is "
                      "not available through a session to a standalone "
                      "instance");
  }

  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("some@instance:4321"));
    EXPECT_THROW_LIKE(_cluster.check_instance_state(args),
                      shcore::Exception,
                      "Cluster.checkInstanceState: This function is not "
                      "available through a session to a standalone instance");
  }
}

class Cluster_preconditions_unmanaged_gr : public Cluster_preconditions {
  virtual void set_preconditions() {
    add_precondition_queries(&_queries, mysqlsh::dba::GroupReplication, {});
    add_precondition_queries(&_queries, mysqlsh::dba::GroupReplication, {});
    add_precondition_queries(&_queries, mysqlsh::dba::GroupReplication, {});
    add_precondition_queries(&_queries, mysqlsh::dba::GroupReplication, {});
    add_precondition_queries(&_queries, mysqlsh::dba::GroupReplication, {});
    add_precondition_queries(&_queries, mysqlsh::dba::GroupReplication, {});
    add_precondition_queries(&_queries, mysqlsh::dba::GroupReplication, {});
    add_precondition_queries(&_queries, mysqlsh::dba::GroupReplication, {});

    // Force quorum is enabled even if it's an unmanaged gr instance
    add_precondition_queries(&_queries, mysqlsh::dba::GroupReplication,
                           std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
                           {}, std::string("ONLINE"), 3, 0);
  }
};

TEST_F(Cluster_preconditions_unmanaged_gr, all_functions) {
  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("some@instance:4321"));
    EXPECT_THROW_LIKE(_cluster.add_instance(args),
                      shcore::Exception,
                      "Cluster.addInstance: This function is not available "
                      "through a session to an instance belonging to an "
                      "unmanaged replication group");
  }

  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("some@instance:4321"));
    EXPECT_THROW_LIKE(_cluster.rejoin_instance(args),
                      shcore::Exception,
                      "Cluster.rejoinInstance: This function is not available "
                      "through a session to an instance belonging to an "
                      "unmanaged replication group");
  }

  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("some@instance:4321"));
    EXPECT_THROW_LIKE(_cluster.remove_instance(args),
                      shcore::Exception,
                      "Cluster.removeInstance: This function is not available "
                      "through a session to an instance belonging to an "
                      "unmanaged replication group");
  }

  {
    EXPECT_THROW_LIKE(_cluster.describe({}),
                      shcore::Exception,
                      "Cluster.describe: This function is not available "
                      "through a session to an instance belonging to an "
                      "unmanaged replication group");
  }

  {
    EXPECT_THROW_LIKE(_cluster.status({}),
                      shcore::Exception,
                      "Cluster.status: This function is not available "
                      "through a session to an instance belonging to an "
                      "unmanaged replication group");
  }

  {
    EXPECT_THROW_LIKE(_cluster.dissolve({}),
                      shcore::Exception,
                      "Cluster.dissolve: This function is not available "
                      "through a session to an instance belonging to an "
                      "unmanaged replication group");
  }

  {
    EXPECT_THROW_LIKE(_cluster.rescan({}),
                      shcore::Exception,
                      "Cluster.rescan: This function is not available "
                      "through a session to an instance belonging to an "
                      "unmanaged replication group");
  }

  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("some@instance:4321"));
    EXPECT_THROW_LIKE(_cluster.check_instance_state(args),
                      shcore::Exception,
                      "Cluster.checkInstanceState: This function is not "
                      "available through a session to an instance belonging to "
                      "an unmanaged replication group");
  }

  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("some@instance:4321"));
    EXPECT_THROW_LIKE(_cluster.force_quorum_using_partition_of(args),
                      shcore::Exception,
                      "Cluster.forceQuorumUsingPartitionOf: ReplicaSet not "
                      "initialized.");
  }
}

TEST_F(Cluster_test, precondition_add_instance_through_RO) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b8"),
    std::string("ONLINE"), 3, 0);

  reset_cluster("dev");

  shcore::Argument_list args;
  args.push_back(shcore::Value("dev"));
  EXPECT_THROW_LIKE(_cluster.add_instance(args),
                    shcore::Exception,
                    "Cluster.addInstance: This function is not available "
                    "through a session to a read only instance");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_add_instance_through_quorum_less) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 2);

  reset_cluster("dev");

  shcore::Argument_list args;
  args.push_back(shcore::Value("dev"));
  EXPECT_THROW_LIKE(_cluster.add_instance(args),
                    shcore::Exception,
                    "Cluster.addInstance: There is no quorum to perform the "
                    "operation");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_add_instance_success) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 0);

  reset_cluster("dev");

  shcore::Argument_list args;
  args.push_back(shcore::Value("dev"));
  EXPECT_THROW_LIKE(_cluster.add_instance(args),
                    shcore::Exception,
                    "Cluster.addInstance: ReplicaSet not initialized.");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_remove_instance_through_RO) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b8"),
    std::string("ONLINE"), 3, 0);

  reset_cluster("dev");

  shcore::Argument_list args;
  args.push_back(shcore::Value("dev"));
  EXPECT_THROW_LIKE(_cluster.remove_instance(args),
                    shcore::Exception,
                    "Cluster.removeInstance: This function is not available "
                    "through a session to a read only instance");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_remove_instance_through_quorum_less) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 2);

  reset_cluster("dev");

  shcore::Argument_list args;
  args.push_back(shcore::Value("dev"));
  EXPECT_THROW_LIKE(_cluster.remove_instance(args),
                    shcore::Exception,
                    "Cluster.removeInstance: There is no quorum to perform the "
                    "operation");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_remove_instance_success) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 0);

  reset_cluster("dev");

  shcore::Argument_list args;
  args.push_back(shcore::Value("dev"));
  EXPECT_THROW_LIKE(_cluster.remove_instance(args),
                    shcore::Exception,
                    "Cluster.removeInstance: ReplicaSet not initialized.");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_rejoin_instance_through_RO) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b8"),
    std::string("ONLINE"), 3, 0);

  reset_cluster("dev");

  shcore::Argument_list args;
  args.push_back(shcore::Value("dev"));
  EXPECT_THROW_LIKE(_cluster.rejoin_instance(args),
                    shcore::Exception,
                    "Cluster.rejoinInstance: ReplicaSet not initialized.");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_rejoin_instance_through_quorum_less) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 2);

  reset_cluster("dev");

  shcore::Argument_list args;
  args.push_back(shcore::Value("dev"));
  EXPECT_THROW_LIKE(_cluster.rejoin_instance(args),
                    shcore::Exception,
                    "Cluster.rejoinInstance: There is no quorum to perform the "
                    "operation");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_rejoin_instance_success) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 0);

  reset_cluster("dev");

  shcore::Argument_list args;
  args.push_back(shcore::Value("dev"));
  EXPECT_THROW_LIKE(_cluster.rejoin_instance(args),
                    shcore::Exception,
                    "Cluster.rejoinInstance: ReplicaSet not initialized.");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_describe_success) {
  // RO Instance
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b8"),
    std::string("ONLINE"), 3, 0);

  // Quorum Less
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 2);

  // RW Instance
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 0);

  reset_cluster("dev");

  EXPECT_CALL(_mock_metadata, cluster_exists("dev")).WillRepeatedly(Return(false));

  EXPECT_THROW_LIKE(_cluster.describe({}),
                    shcore::Exception,
                    "Cluster.describe: The cluster 'dev' no longer "
                    "exists.");

  EXPECT_THROW_LIKE(_cluster.describe({}),
                    shcore::Exception,
                    "Cluster.describe: The cluster 'dev' no longer "
                    "exists.");

  EXPECT_THROW_LIKE(_cluster.describe({}),
                    shcore::Exception,
                    "Cluster.describe: The cluster 'dev' no longer "
                    "exists.");
  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_status_success) {
  // RO Instance
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b8"),
    std::string("ONLINE"), 3, 0);

  // Quorum Less
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 2);

  // RW Instance
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 0);

  reset_cluster("dev");

  auto ret_val_ro = _cluster.status({}).as_map();
  EXPECT_STREQ("dev", (*ret_val_ro)["clusterName"].as_string().c_str());

  auto ret_val_ql = _cluster.status({}).as_map();
  EXPECT_STREQ("dev", (*ret_val_ql)["clusterName"].as_string().c_str());

  auto ret_val_rw = _cluster.status({}).as_map();
  EXPECT_STREQ("dev", (*ret_val_rw)["clusterName"].as_string().c_str());

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_dissolve_through_RO) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b8"),
    std::string("ONLINE"), 3, 0);

  reset_cluster("dev");

  EXPECT_THROW_LIKE(_cluster.dissolve({}),
                    shcore::Exception,
                    "Cluster.dissolve: This function is not available "
                    "through a session to a read only instance");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_dissolve_through_quorum_less) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 2);

  reset_cluster("dev");

  EXPECT_THROW_LIKE(_cluster.dissolve({}),
                    shcore::Exception,
                    "Cluster.dissolve: There is no quorum to perform the "
                    "operation");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_dissolve_success) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 0);

  reset_cluster("dev");

  EXPECT_CALL(_mock_metadata, start_transaction());

  EXPECT_CALL(_mock_metadata, is_cluster_empty(_)).WillOnce(Return(false));

  EXPECT_CALL(_mock_metadata, rollback());

  EXPECT_THROW_LIKE(_cluster.dissolve({}),
                    shcore::Exception,
                    "Cluster.dissolve: Cannot drop cluster: The cluster is not "
                    "empty.");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_check_instance_state_through_RO) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b8"),
    std::string("ONLINE"), 3, 0);

  reset_cluster("dev");

  shcore::Argument_list args;
  args.push_back(shcore::Value("dev"));
  EXPECT_THROW_LIKE(_cluster.check_instance_state(args),
                    shcore::Exception,
                    "Cluster.checkInstanceState: ReplicaSet not initialized.");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_check_instance_state_through_quorum_less) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 2);

  reset_cluster("dev");

  shcore::Argument_list args;
  args.push_back(shcore::Value("dev"));
  EXPECT_THROW_LIKE(_cluster.check_instance_state(args),
                    shcore::Exception,
                    "Cluster.checkInstanceState: There is no quorum to perform the "
                    "operation");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_check_instance_state_success) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 0);

  reset_cluster("dev");

  shcore::Argument_list args;
  args.push_back(shcore::Value("dev"));
  EXPECT_THROW_LIKE(_cluster.check_instance_state(args),
                    shcore::Exception,
                    "Cluster.checkInstanceState: ReplicaSet not initialized.");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_rescan_through_RO) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b8"),
    std::string("ONLINE"), 3, 0);

  reset_cluster("dev");

  EXPECT_THROW_LIKE(_cluster.rescan({}),
                    shcore::Exception,
                    "Cluster.rescan: This function is not available "
                    "through a session to a read only instance");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_rescan_through_quorum_less) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 2);

  reset_cluster("dev");

  EXPECT_THROW_LIKE(_cluster.rescan({}),
                    shcore::Exception,
                    "Cluster.rescan: There is no quorum to perform the "
                    "operation");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_rescan_success) {
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 0);

  reset_cluster("dev");

  EXPECT_THROW_LIKE(_cluster.rescan({}),
                    shcore::Exception,
                    "Cluster.rescan: ReplicaSet not initialized.");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Cluster_test, precondition_force_quorum_using_partition_of_success) {
  // RO Instance
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b8"),
    std::string("ONLINE"), 3, 0);

  // Quorum Less
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 2);

  // RW Instance
  add_precondition_queries(&_queries, mysqlsh::dba::InnoDBCluster,
    std::string("851f0e89-5730-11e7-9e4f-b86b230042b9"),
    {}, std::string("ONLINE"), 3, 0);

  reset_cluster("dev");

  EXPECT_CALL(_mock_metadata, cluster_exists("dev")).WillRepeatedly(Return(false));

  shcore::Argument_list args;
  args.push_back(shcore::Value("some:@host:1234"));
  EXPECT_THROW_LIKE(_cluster.force_quorum_using_partition_of(args),
                    shcore::Exception,
                    "Cluster.forceQuorumUsingPartitionOf: ReplicaSet not "
                    "initialized.");

  EXPECT_THROW_LIKE(_cluster.force_quorum_using_partition_of(args),
                    shcore::Exception,
                    "Cluster.forceQuorumUsingPartitionOf: ReplicaSet not "
                    "initialized.");

  EXPECT_THROW_LIKE(_cluster.force_quorum_using_partition_of(args),
                    shcore::Exception,
                    "Cluster.forceQuorumUsingPartitionOf: ReplicaSet not "
                    "initialized.");

  _interactive_shell->shell_context()->get_dev_session()->close();
}

}  // namespace testing
