/*
* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include <gtest/gtest.h>
#include "unittest/test_utils/admin_api_test.h"
#include "modules/adminapi/mod_dba_common.h"
#include "modules/mod_mysql_session.h"

namespace tests {
class Dba_common_test: public Admin_api_test {
 protected:
  std::shared_ptr<mysqlsh::mysql::ClassicSession> create_session(int port) {
    auto session = std::shared_ptr<mysqlsh::mysql::ClassicSession>
                  (new mysqlsh::mysql::ClassicSession());

    shcore::Argument_list args;
    args.push_back(shcore::Value("user:@localhost:" + std::to_string(port)));
    session->connect(args);

    return session;
  }
};

TEST_F(Dba_common_test, resolve_cluster_ssl_mode_001) {
//InstanceSSL memberSslMode require_secure_transport
//----------- ------------- ------------------------
//enabled     ""            ON
  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.have_ssl",
                                tests::Type::String, "YES");
  add_get_server_variable_query(&queries,
                                "GLOBAL.require_secure_transport",
                                tests::Type::LongLong, "1");

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);

  auto session = create_session(_mysql_sandbox_nport1);

  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session.get(), "");
    EXPECT_STREQ("REQUIRED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_cluster_ssl_mode_001");
    ADD_FAILURE();
  }

  session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}

TEST_F(Dba_common_test, resolve_cluster_ssl_mode_002) {
//InstanceSSL memberSslMode require_secure_transport
//----------- ------------- ------------------------
//enabled     ""            OFF
  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.have_ssl",
                                tests::Type::String, "YES");
  add_get_server_variable_query(&queries,
                                "GLOBAL.require_secure_transport",
                                tests::Type::LongLong, "0");

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);

  auto session = create_session(_mysql_sandbox_nport1);

  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session.get(), "");
    EXPECT_STREQ("REQUIRED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_cluster_ssl_mode_002");
    ADD_FAILURE();
  }

  session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}

TEST_F(Dba_common_test, resolve_cluster_ssl_mode_003) {
//InstanceSSL memberSslMode require_secure_transport
//----------- ------------- ------------------------
//enabled     "DISABLED"    ON
  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.have_ssl",
                                tests::Type::String, "YES");
  add_get_server_variable_query(&queries,
                                "GLOBAL.require_secure_transport",
                                tests::Type::LongLong, "1");

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
  auto session = create_session(_mysql_sandbox_nport1);

  try {
    mysqlsh::dba::resolve_cluster_ssl_mode(session.get(), "DISABLED");
    SCOPED_TRACE("Unexpected success at resolve_cluster_ssl_mode_003");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS("The instance '" + session->uri() + "' requires "
      "secure connections, to create the cluster either turn off "
      "require_secure_transport or use the memberSslMode option "
      "with 'REQUIRED' value.",
      error);
  }

  session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}

TEST_F(Dba_common_test, resolve_cluster_ssl_mode_004) {
//InstanceSSL memberSslMode require_secure_transport
//----------- ------------- ------------------------
//enabled     "DISABLED"    OFF
  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.have_ssl",
                                tests::Type::String, "YES");
  add_get_server_variable_query(&queries,
                                "GLOBAL.require_secure_transport",
                                tests::Type::LongLong, "0");

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
  auto session = create_session(_mysql_sandbox_nport1);

  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session.get(),
                                                           "DISABLED");
    EXPECT_STREQ("DISABLED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected success at resolve_cluster_ssl_mode_004");
    ADD_FAILURE();
  }

  session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}

TEST_F(Dba_common_test, resolve_cluster_ssl_mode_005) {
//InstanceSSL memberSslMode
//----------- -------------
//enabled     "REQUIRED"
  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.have_ssl",
                                tests::Type::String, "YES");

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
  auto session = create_session(_mysql_sandbox_nport1);

  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session.get(),
                                                           "REQUIRED");
    EXPECT_STREQ("REQUIRED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected success at resolve_cluster_ssl_mode_005");
    ADD_FAILURE();
  }

  session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}

TEST_F(Dba_common_test, resolve_cluster_ssl_mode_006) {
//InstanceSSL memberSslMode
//----------- -------------
//enabled     "AUTO"
  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.have_ssl",
                                tests::Type::String, "YES");

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
  auto session = create_session(_mysql_sandbox_nport1);

  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session.get(),
                                                           "AUTO");
    EXPECT_STREQ("REQUIRED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected success at resolve_cluster_ssl_mode_006");
    ADD_FAILURE();
  }

  session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}

TEST_F(Dba_common_test, resolve_cluster_ssl_mode_007) {
//InstanceSSL memberSslMode
//----------- -------------
//disabled    "REQUIRED"
  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.have_ssl",
                                tests::Type::String, "NO");
  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
  auto session = create_session(_mysql_sandbox_nport1);

  try {
    mysqlsh::dba::resolve_cluster_ssl_mode(session.get(), "REQUIRED");
    SCOPED_TRACE("Unexpected success at resolve_cluster_ssl_mode_007");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS("The instance '" + session->uri() + "' does not "
    "have SSL enabled, to create the cluster either use an instance with SSL "
    "enabled, remove the memberSslMode option or use it with any of 'AUTO' or "
    "'DISABLED'.",
    error);
  }

  session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}

TEST_F(Dba_common_test, resolve_cluster_ssl_mode_008) {
//InstanceSSL memberSslMode
//----------- -------------
//disabled    ""
  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.have_ssl",
                                tests::Type::String, "NO");
  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
  auto session = create_session(_mysql_sandbox_nport1);

  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session.get(),
                                                           "");
    EXPECT_STREQ("DISABLED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_cluster_ssl_mode_008");
    ADD_FAILURE();
  }

  session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}

TEST_F(Dba_common_test, resolve_cluster_ssl_mode_009) {
//InstanceSSL memberSslMode
//----------- -------------
//disabled    "DISABLED"
  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.have_ssl",
                                tests::Type::String, "NO");
  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
  auto session = create_session(_mysql_sandbox_nport1);

  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session.get(),
                                                           "DISABLED");
    EXPECT_STREQ("DISABLED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_cluster_ssl_mode_009");
    ADD_FAILURE();
  }

  session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}

TEST_F(Dba_common_test, resolve_cluster_ssl_mode_010) {
//InstanceSSL memberSslMode
//----------- -------------
//disabled    "AUTO"
  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.have_ssl",
                                tests::Type::String, "NO");
  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);
  auto session = create_session(_mysql_sandbox_nport1);

  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session.get(),
                                                           "AUTO");
    EXPECT_STREQ("DISABLED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_cluster_ssl_mode_010");
    ADD_FAILURE();
  }

  session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}




TEST_F(Dba_common_test, resolve_instance_ssl_mode_001) {
//Cluster SSL memberSslMode
//----------- -------------
//REQUIRED    ""

  std::vector< tests::Fake_result_data > peer_queries;
  add_get_server_variable_query(&peer_queries,
                                "global.group_replication_ssl_mode",
                                tests::Type::String, "REQUIRED");
  START_SERVER_MOCK(_mysql_sandbox_nport1, peer_queries);
  auto peer_session = create_session(_mysql_sandbox_nport1);

  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.have_ssl",
                                tests::Type::String, "YES");
  START_SERVER_MOCK(_mysql_sandbox_nport2, queries);
  auto instance_session = create_session(_mysql_sandbox_nport2);


  try {
    auto mode = mysqlsh::dba::resolve_instance_ssl_mode(instance_session.get(),
                                                        peer_session.get(),
                                                        "");
    EXPECT_STREQ("REQUIRED", mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_instance_ssl_mode_001");
    ADD_FAILURE();
  }

  peer_session->close(shcore::Argument_list());
  instance_session->close(shcore::Argument_list());

  stop_server_mock(_mysql_sandbox_nport1);
  stop_server_mock(_mysql_sandbox_nport2);
}

TEST_F(Dba_common_test, resolve_instance_ssl_mode_002) {
//Cluster SSL memberSslMode
//----------- -------------
//REQUIRED    DISABLED

  std::vector< tests::Fake_result_data > peer_queries;
  add_get_server_variable_query(&peer_queries,
                                "global.group_replication_ssl_mode",
                                tests::Type::String, "REQUIRED");
  START_SERVER_MOCK(_mysql_sandbox_nport1, peer_queries);
  auto peer_session = create_session(_mysql_sandbox_nport1);

  std::vector< tests::Fake_result_data > queries;
  START_SERVER_MOCK(_mysql_sandbox_nport2, queries);
  auto instance_session = create_session(_mysql_sandbox_nport2);


  try {
    mysqlsh::dba::resolve_instance_ssl_mode(instance_session.get(),
                                            peer_session.get(),
                                            "DISABLED");
    SCOPED_TRACE("Unexpected success at resolve_instance_ssl_mode_002");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS("The cluster has SSL (encryption) enabled. "
          "To add the instance '" + instance_session->uri() + "to the cluster "
          "either disable SSL on the cluster or use the memberSslMode option "
          "with any of 'AUTO' or 'REQUIRED'.",
          error);
  }

  peer_session->close(shcore::Argument_list());
  instance_session->close(shcore::Argument_list());

  stop_server_mock(_mysql_sandbox_nport1);
  stop_server_mock(_mysql_sandbox_nport2);
}

TEST_F(Dba_common_test, resolve_instance_ssl_mode_003) {
//Cluster SSL memberSslMode Instance SSL
//----------- ------------- ------------
//REQUIRED    AUTO          enabled

  std::vector< tests::Fake_result_data > peer_queries;
  add_get_server_variable_query(&peer_queries,
                                "global.group_replication_ssl_mode",
                                tests::Type::String, "REQUIRED");
  START_SERVER_MOCK(_mysql_sandbox_nport1, peer_queries);
  auto peer_session = create_session(_mysql_sandbox_nport1);

  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.have_ssl",
                                tests::Type::String, "YES");
  START_SERVER_MOCK(_mysql_sandbox_nport2, queries);
  auto instance_session = create_session(_mysql_sandbox_nport2);


  try {
    auto mode = mysqlsh::dba::resolve_instance_ssl_mode(instance_session.get(),
                    peer_session.get(),
                    "AUTO");
    EXPECT_STREQ("REQUIRED", mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_instance_ssl_mode_003");
    ADD_FAILURE();
  }

  peer_session->close(shcore::Argument_list());
  instance_session->close(shcore::Argument_list());

  stop_server_mock(_mysql_sandbox_nport1);
  stop_server_mock(_mysql_sandbox_nport2);
}

TEST_F(Dba_common_test, resolve_instance_ssl_mode_004) {
//Cluster SSL memberSslMode Instance SSL
//----------- ------------- ------------
//REQUIRED    AUTO          disabled

  std::vector< tests::Fake_result_data > peer_queries;
  add_get_server_variable_query(&peer_queries,
                                "global.group_replication_ssl_mode",
                                tests::Type::String, "REQUIRED");
  START_SERVER_MOCK(_mysql_sandbox_nport1, peer_queries);
  auto peer_session = create_session(_mysql_sandbox_nport1);

  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.have_ssl",
                                tests::Type::String, "NO");
  START_SERVER_MOCK(_mysql_sandbox_nport2, queries);
  auto instance_session = create_session(_mysql_sandbox_nport2);


  try {
    mysqlsh::dba::resolve_instance_ssl_mode(instance_session.get(),
                                            peer_session.get(),
                                            "AUTO");
    SCOPED_TRACE("Unexpected success at resolve_instance_ssl_mode_004");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS("Instance '" + instance_session->uri() + "' does "
        "not support SSL and cannot join a cluster with SSL (encryption) "
        "enabled. Enable SSL support on the instance and try again, otherwise "
        "it can only be added to a cluster with SSL disabled.",
         error);
  }

  peer_session->close(shcore::Argument_list());
  instance_session->close(shcore::Argument_list());

  stop_server_mock(_mysql_sandbox_nport1);
  stop_server_mock(_mysql_sandbox_nport2);
}

TEST_F(Dba_common_test, resolve_instance_ssl_mode_005) {
//Cluster SSL memberSslMode Instance SSL
//----------- ------------- ------------
//REQUIRED    REQUIRED      enabled

  std::vector< tests::Fake_result_data > peer_queries;
  add_get_server_variable_query(&peer_queries,
                                "global.group_replication_ssl_mode",
                                tests::Type::String, "REQUIRED");
  START_SERVER_MOCK(_mysql_sandbox_nport1, peer_queries);
  auto peer_session = create_session(_mysql_sandbox_nport1);

  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.have_ssl",
                                tests::Type::String, "YES");
  START_SERVER_MOCK(_mysql_sandbox_nport2, queries);
  auto instance_session = create_session(_mysql_sandbox_nport2);


  try {
    auto mode = mysqlsh::dba::resolve_instance_ssl_mode(instance_session.get(),
                    peer_session.get(),
                    "REQUIRED");
    EXPECT_STREQ("REQUIRED", mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_instance_ssl_mode_005");
    ADD_FAILURE();
  }

  peer_session->close(shcore::Argument_list());
  instance_session->close(shcore::Argument_list());

  stop_server_mock(_mysql_sandbox_nport1);
  stop_server_mock(_mysql_sandbox_nport2);
}

TEST_F(Dba_common_test, resolve_instance_ssl_mode_006) {
//Cluster SSL memberSslMode Instance SSL
//----------- ------------- ------------
//REQUIRED    REQUIRED      disabled

  std::vector< tests::Fake_result_data > peer_queries;
  add_get_server_variable_query(&peer_queries,
                                "global.group_replication_ssl_mode",
                                tests::Type::String, "REQUIRED");
  START_SERVER_MOCK(_mysql_sandbox_nport1, peer_queries);
  auto peer_session = create_session(_mysql_sandbox_nport1);

  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.have_ssl",
                                tests::Type::String, "NO");
  START_SERVER_MOCK(_mysql_sandbox_nport2, queries);
  auto instance_session = create_session(_mysql_sandbox_nport2);


  try {
    mysqlsh::dba::resolve_instance_ssl_mode(instance_session.get(),
                                            peer_session.get(),
                                            "REQUIRED");
    SCOPED_TRACE("Unexpected success at resolve_instance_ssl_mode_006");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS("Instance '" + instance_session->uri() + "' does "
        "not support SSL and cannot join a cluster with SSL (encryption) "
        "enabled. Enable SSL support on the instance and try again, otherwise "
        "it can only be added to a cluster with SSL disabled.",
         error);
  }

  peer_session->close(shcore::Argument_list());
  instance_session->close(shcore::Argument_list());

  stop_server_mock(_mysql_sandbox_nport1);
  stop_server_mock(_mysql_sandbox_nport2);
}

TEST_F(Dba_common_test, resolve_instance_ssl_mode_007) {
//Cluster SSL memberSslMode
//----------- -------------
//DISABLED    REQUIRED

  std::vector< tests::Fake_result_data > peer_queries;
  add_get_server_variable_query(&peer_queries,
                                "global.group_replication_ssl_mode",
                                tests::Type::String, "DISABLED");
  START_SERVER_MOCK(_mysql_sandbox_nport1, peer_queries);
  auto peer_session = create_session(_mysql_sandbox_nport1);

  std::vector< tests::Fake_result_data > queries;
  START_SERVER_MOCK(_mysql_sandbox_nport2, queries);
  auto instance_session = create_session(_mysql_sandbox_nport2);


  try {
    mysqlsh::dba::resolve_instance_ssl_mode(instance_session.get(),
                                            peer_session.get(),
                                            "REQUIRED");
    SCOPED_TRACE("Unexpected success at resolve_instance_ssl_mode_007");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS("The cluster has SSL (encryption) disabled. "
          "To add the instance '" + instance_session->uri() + "' to the "
          "cluster either disable SSL on the cluster, remove the memberSslMode "
          "option or use it with any of 'AUTO' or 'DISABLED'.",
          error);
  }

  peer_session->close(shcore::Argument_list());
  instance_session->close(shcore::Argument_list());

  stop_server_mock(_mysql_sandbox_nport1);
  stop_server_mock(_mysql_sandbox_nport2);
}

TEST_F(Dba_common_test, resolve_instance_ssl_mode_008) {
//Cluster SSL memberSslMode require_secure_transport
//----------- ------------- ------------------------
//DISABLED    ""            ON

  std::vector< tests::Fake_result_data > peer_queries;
  add_get_server_variable_query(&peer_queries,
                                "global.group_replication_ssl_mode",
                                tests::Type::String, "DISABLED");
  START_SERVER_MOCK(_mysql_sandbox_nport1, peer_queries);
  auto peer_session = create_session(_mysql_sandbox_nport1);

  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.require_secure_transport",
                                tests::Type::LongLong, "1");
  START_SERVER_MOCK(_mysql_sandbox_nport2, queries);
  auto instance_session = create_session(_mysql_sandbox_nport2);


  try {
    mysqlsh::dba::resolve_instance_ssl_mode(instance_session.get(),
                                            peer_session.get(),
                                            "");
    SCOPED_TRACE("Unexpected success at resolve_instance_ssl_mode_008");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS("The instance '" + instance_session->uri() + "' "
        "is configured to require a secure transport but the cluster has SSL "
        "disabled. To add the instance to the cluster, either turn OFF the "
        "require_secure_transport option on the instance or enable SSL on "
        "the cluster.",
         error);
  }

  peer_session->close(shcore::Argument_list());
  instance_session->close(shcore::Argument_list());

  stop_server_mock(_mysql_sandbox_nport1);
  stop_server_mock(_mysql_sandbox_nport2);
}

TEST_F(Dba_common_test, resolve_instance_ssl_mode_009) {
//Cluster SSL memberSslMode require_secure_transport
//----------- ------------- ------------------------
//DISABLED    ""            OFF

  std::vector< tests::Fake_result_data > peer_queries;
  add_get_server_variable_query(&peer_queries,
                                "global.group_replication_ssl_mode",
                                tests::Type::String, "DISABLED");
  START_SERVER_MOCK(_mysql_sandbox_nport1, peer_queries);
  auto peer_session = create_session(_mysql_sandbox_nport1);

  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.require_secure_transport",
                                tests::Type::LongLong, "0");
  START_SERVER_MOCK(_mysql_sandbox_nport2, queries);
  auto instance_session = create_session(_mysql_sandbox_nport2);


  try {
    auto mode = mysqlsh::dba::resolve_instance_ssl_mode(instance_session.get(),
                                                        peer_session.get(),
                                                        "");
    EXPECT_STREQ("DISABLED", mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_instance_ssl_mode_009");
    ADD_FAILURE();
  }

  peer_session->close(shcore::Argument_list());
  instance_session->close(shcore::Argument_list());

  stop_server_mock(_mysql_sandbox_nport1);
  stop_server_mock(_mysql_sandbox_nport2);
}

TEST_F(Dba_common_test, resolve_instance_ssl_mode_010) {
//Cluster SSL memberSslMode require_secure_transport
//----------- ------------- ------------------------
//DISABLED    AUTO          ON

  std::vector< tests::Fake_result_data > peer_queries;
  add_get_server_variable_query(&peer_queries,
                                "global.group_replication_ssl_mode",
                                tests::Type::String, "DISABLED");
  START_SERVER_MOCK(_mysql_sandbox_nport1, peer_queries);
  auto peer_session = create_session(_mysql_sandbox_nport1);

  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.require_secure_transport",
                                tests::Type::LongLong, "1");
  START_SERVER_MOCK(_mysql_sandbox_nport2, queries);
  auto instance_session = create_session(_mysql_sandbox_nport2);


  try {
    mysqlsh::dba::resolve_instance_ssl_mode(instance_session.get(),
                                            peer_session.get(),
                                            "AUTO");
    SCOPED_TRACE("Unexpected success at resolve_instance_ssl_mode_010");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS("The instance '" + instance_session->uri() + "' "
        "is configured to require a secure transport but the cluster has SSL "
        "disabled. To add the instance to the cluster, either turn OFF the "
        "require_secure_transport option on the instance or enable SSL on "
        "the cluster.",
         error);
  }

  peer_session->close(shcore::Argument_list());
  instance_session->close(shcore::Argument_list());

  stop_server_mock(_mysql_sandbox_nport1);
  stop_server_mock(_mysql_sandbox_nport2);
}

TEST_F(Dba_common_test, resolve_instance_ssl_mode_011) {
//Cluster SSL memberSslMode require_secure_transport
//----------- ------------- ------------------------
//DISABLED    AUTO          OFF

  std::vector< tests::Fake_result_data > peer_queries;
  add_get_server_variable_query(&peer_queries,
                                "global.group_replication_ssl_mode",
                                tests::Type::String, "DISABLED");
  START_SERVER_MOCK(_mysql_sandbox_nport1, peer_queries);
  auto peer_session = create_session(_mysql_sandbox_nport1);

  std::vector< tests::Fake_result_data > queries;
  add_get_server_variable_query(&queries,
                                "global.require_secure_transport",
                                tests::Type::LongLong, "0");
  START_SERVER_MOCK(_mysql_sandbox_nport2, queries);
  auto instance_session = create_session(_mysql_sandbox_nport2);


  try {
    auto mode = mysqlsh::dba::resolve_instance_ssl_mode(instance_session.get(),
                                                        peer_session.get(),
                                                        "AUTO");
    EXPECT_STREQ("DISABLED", mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_instance_ssl_mode_011");
    ADD_FAILURE();
  }

  peer_session->close(shcore::Argument_list());
  instance_session->close(shcore::Argument_list());

  stop_server_mock(_mysql_sandbox_nport1);
  stop_server_mock(_mysql_sandbox_nport2);
}

TEST_F(Dba_common_test, resolve_instance_ssl_mode_012) {
//Cluster SSL
//-----------
//ANY_OTHER

  std::vector< tests::Fake_result_data > peer_queries;
  add_get_server_variable_query(&peer_queries,
                                "global.group_replication_ssl_mode",
                                tests::Type::String, "ANY_OTHER");
  START_SERVER_MOCK(_mysql_sandbox_nport1, peer_queries);
  auto peer_session = create_session(_mysql_sandbox_nport1);

  std::vector< tests::Fake_result_data > queries;
  START_SERVER_MOCK(_mysql_sandbox_nport2, queries);
  auto instance_session = create_session(_mysql_sandbox_nport2);


  try {
    mysqlsh::dba::resolve_instance_ssl_mode(instance_session.get(),
                                            peer_session.get(),
                                            "AUTO");
    SCOPED_TRACE("Unexpected success at resolve_instance_ssl_mode_012");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS("Unsupported Group Replication SSL Mode for the "
      "cluster: 'ANY_OTHER'. If the cluster was created using adoptFromGR:true "
      "make sure the group_replication_ssl_mode variable is set with a "
      "supported value (DISABLED or REQUIRED) for all cluster members.",
      error);
  }

  peer_session->close(shcore::Argument_list());
  instance_session->close(shcore::Argument_list());

  stop_server_mock(_mysql_sandbox_nport1);
  stop_server_mock(_mysql_sandbox_nport2);
}
}  // namespace tests
