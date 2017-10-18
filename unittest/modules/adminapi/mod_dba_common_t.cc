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

#include <string>

#include <gtest/gtest.h>
#include "unittest/test_utils/admin_api_test.h"
#include "modules/adminapi/mod_dba_common.h"
#include "modules/mod_mysql_session.h"
#include "modules/adminapi/mod_dba_metadata_storage.h"
#include "shellcore/types.h"

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
  std::shared_ptr<mysqlsh::ShellDevelopmentSession> create_dev_session(
        int port) {
    std::shared_ptr<mysqlsh::ShellDevelopmentSession> session;

    shcore::Argument_list session_args;
    shcore::Value::Map_type_ref instance_options(new shcore::Value::Map_type);
    (*instance_options)["host"] = shcore::Value("localhost");
    (*instance_options)["port"] = shcore::Value(port);
    (*instance_options)["password"] = shcore::Value("");
    (*instance_options)["user"] = shcore::Value("user");

    session_args.push_back(shcore::Value(instance_options));
    session = mysqlsh::connect_session(session_args,
                                       mysqlsh::SessionType::Classic);

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
          "To add the instance '" + instance_session->uri() + "' to the "
          "cluster either disable SSL on the cluster, remove the memberSslMode "
          "option or use it with any of 'AUTO' or 'REQUIRED'.",
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
          "cluster either enable SSL on the cluster, remove the memberSslMode "
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

TEST_F(Dba_common_test, get_instances_gr) {
// get_instances_gr():
//
// member_id
// ------------------------------------
// 851f0e89-5730-11e7-9e4f-b86b230042b9
// 8a8ae9ce-5730-11e7-a437-b86b230042b9
// 8fcb92c9-5730-11e7-aa60-b86b230042b9

  std::vector< tests::Fake_result_data > queries;
  std::vector<std::vector<std::string>> values;
  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"},
            {"8fcb92c9-5730-11e7-aa60-b86b230042b9"}};

  add_ps_gr_group_members_query(&queries, values);

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);

  auto md_session = create_dev_session(_mysql_sandbox_nport1);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  try {
    auto result = mysqlsh::dba::get_instances_gr(metadata);

    for (int i = 0; i < result.size(); i++)
      EXPECT_STREQ(values[i][0].c_str(), result[i].c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at get_instances_gr");
    ADD_FAILURE();
  }

  md_session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}

TEST_F(Dba_common_test, get_instances_md) {
// get_instances_md():
//
// member_id
// ------------------------------------
// 851f0e89-5730-11e7-9e4f-b86b230042b9
// 8fcb92c9-5730-11e7-aa60-b86b230042b9
// 8a8ae9ce-5730-11e7-a437-b86b230042b9

  std::vector< tests::Fake_result_data > queries;
  std::vector<std::vector<std::string>> values;
  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"},
            {"8fcb92c9-5730-11e7-aa60-b86b230042b9"}};

  add_md_group_members_query(&queries, values);

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);

  auto md_session = create_dev_session(_mysql_sandbox_nport1);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  try {
    auto result = mysqlsh::dba::get_instances_md(metadata, 1);

    for (int i = 0; i < result.size(); i++)
      EXPECT_STREQ(values[i][0].c_str(), result[i].c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at get_instances_md");
    ADD_FAILURE();
  }

  md_session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}

// If the information on the Metadata and the GR group
// P_S info is the same get_newly_discovered_instances()
// result return an empty list
TEST_F(Dba_common_test, get_newly_discovered_instances_001) {
// get_instances_gr() // get_instances_md():
//
// member_id
// ------------------------------------
// 851f0e89-5730-11e7-9e4f-b86b230042b9
// 8a8ae9ce-5730-11e7-a437-b86b230042b9
// 8fcb92c9-5730-11e7-aa60-b86b230042b9

  std::vector< tests::Fake_result_data > queries;

  std::vector<std::vector<std::string>> values;
  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"},
            {"8fcb92c9-5730-11e7-aa60-b86b230042b9"}};

  add_ps_gr_group_members_query(&queries, values);
  add_md_group_members_query(&queries, values);

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);

  auto md_session = create_dev_session(_mysql_sandbox_nport1);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  try {
    auto newly_discovered_instances_list(
      get_newly_discovered_instances(metadata, 1));

    EXPECT_TRUE(newly_discovered_instances_list.empty());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at get_instances_md");
    ADD_FAILURE();
  }

  md_session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}

// If the information on the Metadata and the GR group
// P_S info is the same but in different order,
// get_newly_discovered_instances() should return an empty list
//
// Regression test for BUG #25534693
TEST_F(Dba_common_test, get_newly_discovered_instances_002) {
// get_instances_gr():
//
// member_id
// ------------------------------------
// 851f0e89-5730-11e7-9e4f-b86b230042b9
// 8a8ae9ce-5730-11e7-a437-b86b230042b9
// 8fcb92c9-5730-11e7-aa60-b86b230042b9

  std::vector< tests::Fake_result_data > queries;

  std::vector<std::vector<std::string>> values;
  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"},
            {"8fcb92c9-5730-11e7-aa60-b86b230042b9"}};

  add_ps_gr_group_members_query(&queries, values);

// get_instances_md():
//
// member_id
// ------------------------------------
// 8fcb92c9-5730-11e7-aa60-b86b230042b9
// 851f0e89-5730-11e7-9e4f-b86b230042b9
// 8a8ae9ce-5730-11e7-a437-b86b230042b9

  values = {{"8fcb92c9-5730-11e7-aa60-b86b230042b9"},
            {"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"}};

  add_md_group_members_query(&queries, values);

  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9", "localhost", "3310"}};

  add_ps_gr_group_members_full_query(&queries,
      "851f0e89-5730-11e7-9e4f-b86b230042b9", values);

  values = {{"8a8ae9ce-5730-11e7-a437-b86b230042b9", "localhost", "3320"}};

  add_ps_gr_group_members_full_query(&queries,
      "8a8ae9ce-5730-11e7-a437-b86b230042b9", values);

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);

  auto md_session = create_dev_session(_mysql_sandbox_nport1);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  try {
    auto newly_discovered_instances_list(
      get_newly_discovered_instances(metadata, 1));

    EXPECT_TRUE(newly_discovered_instances_list.empty());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at get_instances_md");
    ADD_FAILURE();
  }

  md_session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}

// If the information on the Metadata and the GR group
// P_S info is the same get_unavailable_instances()
// should return an empty list
TEST_F(Dba_common_test, get_unavailable_instances_001) {
// get_instances_gr() // get_instances_md():
//
// member_id
// ------------------------------------
// 851f0e89-5730-11e7-9e4f-b86b230042b9
// 8a8ae9ce-5730-11e7-a437-b86b230042b9
// 8fcb92c9-5730-11e7-aa60-b86b230042b9

  std::vector< tests::Fake_result_data > queries;

  std::vector<std::vector<std::string>> values;
  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"},
            {"8fcb92c9-5730-11e7-aa60-b86b230042b9"}};

  add_ps_gr_group_members_query(&queries, values);
  add_md_group_members_query(&queries, values);

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);

  auto md_session = create_dev_session(_mysql_sandbox_nport1);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  try {
    auto unavailable_instances_list(
      get_unavailable_instances(metadata, 1));

    EXPECT_TRUE(unavailable_instances_list.empty());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at get_unavailable_instances_001");
    ADD_FAILURE();
  }

  md_session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}

// If the information on the Metadata and the GR group
// P_S info is the same but in different order,
// get_unavailable_instances() should return an empty list
//
// Regression test for BUG #25534693
TEST_F(Dba_common_test, get_unavailable_instances_002) {
// get_instances_gr():
//
// member_id
// ------------------------------------
// 851f0e89-5730-11e7-9e4f-b86b230042b9
// 8a8ae9ce-5730-11e7-a437-b86b230042b9
// 8fcb92c9-5730-11e7-aa60-b86b230042b9

  std::vector< tests::Fake_result_data > queries;

  std::vector<std::vector<std::string>> values;
  values = {{"8fcb92c9-5730-11e7-aa60-b86b230042b9"},
            {"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"}};

  add_ps_gr_group_members_query(&queries, values);

// get_instances_md():
//
// member_id
// ------------------------------------
// 8fcb92c9-5730-11e7-aa60-b86b230042b9
// 851f0e89-5730-11e7-9e4f-b86b230042b9
// 8a8ae9ce-5730-11e7-a437-b86b230042b9

  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"},
            {"8fcb92c9-5730-11e7-aa60-b86b230042b9"}};

  add_md_group_members_query(&queries, values);

  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9", "localhost:3330",
              "localhost:3330"}};

  add_md_group_members_full_query(&queries,
      "851f0e89-5730-11e7-9e4f-b86b230042b9", values);

  values = {{"8a8ae9ce-5730-11e7-a437-b86b230042b9", "localhost:3320",
              "localhost:3320"}};

  add_md_group_members_full_query(&queries,
      "8a8ae9ce-5730-11e7-a437-b86b230042b9", values);

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);

  auto md_session = create_dev_session(_mysql_sandbox_nport1);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  try {
    auto unavailable_instances_list(
      get_unavailable_instances(metadata, 1));

    EXPECT_TRUE(unavailable_instances_list.empty());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at get_unavailable_instances_002");
    ADD_FAILURE();
  }

  md_session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}

TEST_F(Dba_common_test, get_gr_replicaset_group_name) {
  // @@group_replication_group_name
  //-------------------------------------
  // fd4b70e8-5cb1-11e7-a68b-b86b230042b9
  //-------------------------------------

  std::vector<tests::Fake_result_data> queries;
  add_get_server_variable_query(
      &queries,
      "group_replication_group_name",
      tests::Type::String, "fd4b70e8-5cb1-11e7-a68b-b86b230042b9");

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);

  auto session = create_session(_mysql_sandbox_nport1);

  try {
    std::string result =
        mysqlsh::dba::get_gr_replicaset_group_name(session.get());

    EXPECT_STREQ("fd4b70e8-5cb1-11e7-a68b-b86b230042b9", result.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at get_gr_replicaset_group_name");
    ADD_FAILURE();
  }

  session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
}

TEST_F(Dba_common_test, validate_replicaset_group_name_001) {
  // @@group_replication_group_name (instance)
  //-------------------------------------
  // fd4b70e8-5cb1-11e7-a68b-b86b230042b9
  //-------------------------------------

  // group_replication_group_name (metadata)
  // ---------------------------------------
  // fd4b70e8-5cb1-11e7-a68b-b86b230042b9
  // ---------------------------------------

  std::vector<tests::Fake_result_data> queries_server1;
  add_get_server_variable_query(
      &queries_server1,
      "group_replication_group_name",
      tests::Type::String, "fd4b70e8-5cb1-11e7-a68b-b86b230042b9");

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries_server1);

  std::vector<tests::Fake_result_data> queries_server2;
  add_show_databases_query(&queries_server2, "mysql_innodb_cluster_metadata",
                           "mysql_innodb_cluster_metadata");

  add_md_group_name_query(&queries_server2,
      "fd4b70e8-5cb1-11e7-a68b-b86b230042b9");

  START_SERVER_MOCK(_mysql_sandbox_nport2, queries_server2);

  auto session = create_session(_mysql_sandbox_nport1);
  auto md_session = create_dev_session(_mysql_sandbox_nport2);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  try {
    EXPECT_TRUE(validate_replicaset_group_name(metadata, session.get(), 1));
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at validate_replicaset_group_name_001");
    ADD_FAILURE();
  }

  session->close(shcore::Argument_list());
  md_session->close(shcore::Argument_list());

  stop_server_mock(_mysql_sandbox_nport1);
  stop_server_mock(_mysql_sandbox_nport2);
}

TEST_F(Dba_common_test, validate_replicaset_group_name_002) {
  // @@group_replication_group_name (instance)
  //-------------------------------------
  // fd4b70e8-5cb1-11e7-a68b-b86b230042b9
  //-------------------------------------

  // group_replication_group_name (metadata)
  // ---------------------------------------
  // fd4b70e8-5cb1-11e7-a68b-b86b230042b0
  // ---------------------------------------

  std::vector<tests::Fake_result_data> queries_server1;
  add_get_server_variable_query(
      &queries_server1,
      "group_replication_group_name",
      tests::Type::String, "fd4b70e8-5cb1-11e7-a68b-b86b230042b9");

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries_server1);

  std::vector<tests::Fake_result_data> queries_server2;
  add_show_databases_query(&queries_server2, "mysql_innodb_cluster_metadata",
                           "mysql_innodb_cluster_metadata");

  add_md_group_name_query(&queries_server2,
      "fd4b70e8-5cb1-11e7-a68b-b86b230042b0");

  START_SERVER_MOCK(_mysql_sandbox_nport2, queries_server2);

  auto session = create_session(_mysql_sandbox_nport1);
  auto md_session = create_dev_session(_mysql_sandbox_nport2);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  try {
    EXPECT_FALSE(validate_replicaset_group_name(metadata, session.get(), 1));
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at validate_replicaset_group_name_001");
    ADD_FAILURE();
  }

  session->close(shcore::Argument_list());
  md_session->close(shcore::Argument_list());

  stop_server_mock(_mysql_sandbox_nport1);
  stop_server_mock(_mysql_sandbox_nport2);
}

TEST_F(Dba_common_test, validate_instance_rejoinable_01) {
  // There are missing instances and the instance we are checking belongs to
  // the metadata list but does not belong to the GR list.

  // get_instances_gr():
  //
  // member_id
  // ------------------------------------
  // 8fcb92c9-5730-11e7-aa60-b86b230042b9
  // 851f0e89-5730-11e7-9e4f-b86b230042b9

  std::vector<tests::Fake_result_data> queries;

  std::vector<std::vector<std::string>> values;
  values = {{"8fcb92c9-5730-11e7-aa60-b86b230042b9"},
            {"851f0e89-5730-11e7-9e4f-b86b230042b9"}};

  add_ps_gr_group_members_query(&queries, values);

  // get_instances_md():
  //
  // member_id
  // ------------------------------------
  // 851f0e89-5730-11e7-9e4f-b86b230042b9
  // 8a8ae9ce-5730-11e7-a437-b86b230042b9
  // 8fcb92c9-5730-11e7-aa60-b86b230042b9

  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"},
            {"8fcb92c9-5730-11e7-aa60-b86b230042b9"}};

  add_md_group_members_query(&queries, values);

  values = {{"8a8ae9ce-5730-11e7-a437-b86b230042b9", "localhost:3320",
             "localhost:3320"}};

  add_md_group_members_full_query(
      &queries, "8a8ae9ce-5730-11e7-a437-b86b230042b9", values);

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);

  auto md_session = create_dev_session(_mysql_sandbox_nport1);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  std::vector<tests::Fake_result_data> instance_queries;
  add_get_server_variable_query(&instance_queries, "server_uuid",
                                tests::Type::String,
                                "8a8ae9ce-5730-11e7-a437-b86b230042b9");
  START_SERVER_MOCK(_mysql_sandbox_nport2, instance_queries);
  auto instance_session = create_session(_mysql_sandbox_nport2);
  try {
    bool is_rejoinable(
        validate_instance_rejoinable(instance_session.get(), metadata, 1));

    EXPECT_TRUE(is_rejoinable);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at validate_instance_rejoinable_01");
    ADD_FAILURE();
  }

  md_session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
  instance_session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport2);
}

TEST_F(Dba_common_test, validate_instance_rejoinable_02) {
  // There are missing instances and the instance we are checking belongs
  // to neither the metadata nor GR lists.

  // get_instances_gr():
  //
  // member_id
  // ------------------------------------
  // 8fcb92c9-5730-11e7-aa60-b86b230042b9
  // 851f0e89-5730-11e7-9e4f-b86b230042b9

  std::vector<tests::Fake_result_data> queries;

  std::vector<std::vector<std::string>> values;
  values = {{"8fcb92c9-5730-11e7-aa60-b86b230042b9"},
            {"851f0e89-5730-11e7-9e4f-b86b230042b9"}};

  add_ps_gr_group_members_query(&queries, values);

  // get_instances_md():
  //
  // member_id
  // ------------------------------------
  // 851f0e89-5730-11e7-9e4f-b86b230042b9
  // 8a8ae9ce-5730-11e7-a437-b86b230042b9
  // 8fcb92c9-5730-11e7-aa60-b86b230042b9

  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"},
            {"8fcb92c9-5730-11e7-aa60-b86b230042b9"}};

  add_md_group_members_query(&queries, values);

  values = {{"8a8ae9ce-5730-11e7-a437-b86b230042b9", "localhost:3320",
             "localhost:3320"}};

  add_md_group_members_full_query(
      &queries, "8a8ae9ce-5730-11e7-a437-b86b230042b9", values);

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);

  auto md_session = create_dev_session(_mysql_sandbox_nport1);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  std::vector<tests::Fake_result_data> instance_queries;
  // Checking an instance that doesn't belong  to the metadata nor the GR
  // list.
  add_get_server_variable_query(&instance_queries, "server_uuid",
                                tests::Type::String,
                                "aaaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaa");
  START_SERVER_MOCK(_mysql_sandbox_nport2, instance_queries);
  auto instance_session = create_session(_mysql_sandbox_nport2);
  try {
    bool is_rejoinable(
        validate_instance_rejoinable(instance_session.get(), metadata, 1));

    EXPECT_FALSE(is_rejoinable);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at validate_instance_rejoinable_02");
    ADD_FAILURE();
  }

  md_session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
  instance_session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport2);
}

TEST_F(Dba_common_test, validate_instance_rejoinable_03) {
  // There are no missing instances and the instance we are checking belongs
  // to both the metadata and GR lists.

  // get_instances_gr():
  //
  // member_id
  // ------------------------------------
  // 851f0e89-5730-11e7-9e4f-b86b230042b9
  // 8a8ae9ce-5730-11e7-a437-b86b230042b9
  // 8fcb92c9-5730-11e7-aa60-b86b230042b9

  std::vector<tests::Fake_result_data> queries;

  std::vector<std::vector<std::string>> values;
  values = {{"851f0e89-5730-11e7-9e4f-b86b230042b9"},
            {"8a8ae9ce-5730-11e7-a437-b86b230042b9"},
            {"8fcb92c9-5730-11e7-aa60-b86b230042b9"}};

  add_ps_gr_group_members_query(&queries, values);

  // get_instances_md():
  //
  // member_id
  // ------------------------------------
  // 851f0e89-5730-11e7-9e4f-b86b230042b9
  // 8a8ae9ce-5730-11e7-a437-b86b230042b9
  // 8fcb92c9-5730-11e7-aa60-b86b230042b9

  add_md_group_members_query(&queries, values);

  START_SERVER_MOCK(_mysql_sandbox_nport1, queries);

  auto md_session = create_dev_session(_mysql_sandbox_nport1);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  std::vector<tests::Fake_result_data> instance_queries;
  add_get_server_variable_query(&instance_queries, "server_uuid",
                                tests::Type::String,
                                "8a8ae9ce-5730-11e7-a437-b86b230042b9");
  START_SERVER_MOCK(_mysql_sandbox_nport2, instance_queries);
  auto instance_session = create_session(_mysql_sandbox_nport2);
  try {
    bool is_rejoinable(
        validate_instance_rejoinable(instance_session.get(), metadata, 1));

    EXPECT_FALSE(is_rejoinable);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at validate_instance_rejoinable_03");
    ADD_FAILURE();
  }

  md_session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport1);
  instance_session->close(shcore::Argument_list());
  stop_server_mock(_mysql_sandbox_nport2);
}

}  // namespace tests

TEST(mod_dba_common, validate_label) {
  std::string t{};

  EXPECT_NO_THROW(
    // Valid label, begins with valid synbols (alpha)
    t = "Valid1"; mysqlsh::dba::validate_label(t.c_str()));
  EXPECT_NO_THROW(
    // Valid label, begins with valid synbols (_)
    t = "_Valid_"; mysqlsh::dba::validate_label(t.c_str()));
  EXPECT_NO_THROW(
    // Valid label, contains valid synbols
    t = "Valid_3"; mysqlsh::dba::validate_label(t.c_str()));
  EXPECT_NO_THROW(
    // Valid label, contains valid synbols (:.-)
    t = "Valid:.-4"; mysqlsh::dba::validate_label(t.c_str()));
  EXPECT_NO_THROW(
    // Valid label, begins with valid synbols (numeric)
    t = "2_Valid"; mysqlsh::dba::validate_label(t.c_str()));

  EXPECT_ANY_THROW(t = "";
                   // Invalid empty label
                   mysqlsh::dba::validate_label(t.c_str()););
  EXPECT_ANY_THROW(
      // Invalid label, contains invalid synbol
      t = "not_allowed?"; mysqlsh::dba::validate_label(t.c_str()));
  EXPECT_ANY_THROW(
      // Invalid label, contains invalid synbol
      t = "(not*valid)"; mysqlsh::dba::validate_label("(not_valid)"));
  EXPECT_ANY_THROW(
      // Invalid too long label (over 256 characteres)
      t = "over256chars_"
          "12345678901234567890123456789901234567890123456789012345678901234567"
          "89012345678901234567890123456789012345678901234567890123456789012345"
          "67890123456789012345678901234567890123456789012345678901234567890123"
          "4567890123456789012345678901234567890123";
      mysqlsh::dba::validate_label(t.c_str()););
  EXPECT_ANY_THROW(
      // Invalid label, begins with invalid synbol
      t = "#not_allowed"; mysqlsh::dba::validate_label(t.c_str()););
  EXPECT_ANY_THROW(
      // Invalid label, contains invalid synbol
      t = "_not-allowed?"; mysqlsh::dba::validate_label(t.c_str()););
  EXPECT_ANY_THROW(
      // Invalid label, contains invalid synbol
      t = "(*)%?"; mysqlsh::dba::validate_label(t.c_str()););
}

TEST(mod_dba_common, is_valid_identifier) {
  std::string t{};

  EXPECT_NO_THROW(
      // Valid identifier, begins with valid synbols (alpha)
      t = "Valid1"; mysqlsh::dba::validate_cluster_name(t));
  EXPECT_NO_THROW(
      // Valid identifier, begins with valid synbols (_)
      t = "_Valid_"; mysqlsh::dba::validate_cluster_name(t));
  EXPECT_NO_THROW(
      // Valid identifier, contains valid synbols
      t = "Valid_3"; mysqlsh::dba::validate_cluster_name(t));

  EXPECT_ANY_THROW(t = "";
                   // Invalid empty identifier
                   mysqlsh::dba::validate_cluster_name(t););
  EXPECT_ANY_THROW(
      // Invalid too long identifier (over 40 characteres)
      t = "over40chars_12345678901234567890123456789";
      mysqlsh::dba::validate_cluster_name(t););
  EXPECT_ANY_THROW(
      // Invalid identifier, begins with invalid synbol
      t = "#not_allowed"; mysqlsh::dba::validate_cluster_name(t););
  EXPECT_ANY_THROW(
      // Invalid identifier, contains invalid synbol
      t = "not_allowed?"; mysqlsh::dba::validate_cluster_name(t););
  EXPECT_ANY_THROW(
    // Invalid identifier, begins with invalid synbols (numeric)
    t = "2_not_Valid"; mysqlsh::dba::validate_cluster_name(t));
  EXPECT_ANY_THROW(
      // Invalid identifier, contains invalid synbol
      t = "(*)%?"; mysqlsh::dba::validate_cluster_name(t););
}

TEST(mod_dba_common, validate_group_name_option) {
  shcore::Value::Map_type_ref options(new shcore::Value::Map_type);

  // Error if the groupName is empty.
  (*options)["groupName"] = shcore::Value("");
  EXPECT_THROW(mysqlsh::dba::validate_group_name_option(options),
               shcore::Exception);

  // Error if the groupName string is empty (only whitespace).
  (*options)["groupName"] = shcore::Value("  ");
  EXPECT_THROW(mysqlsh::dba::validate_group_name_option(options),
               shcore::Exception);

  // No error if the groupName is a non-empty string.
  (*options)["groupName"] = shcore::Value("myname");
  EXPECT_NO_THROW(mysqlsh::dba::validate_group_name_option(options));
}

TEST(mod_dba_common, validate_local_address_option) {
  shcore::Value::Map_type_ref options(new shcore::Value::Map_type);

  // Error if the localAddress is empty.
  (*options)["localAddress"] = shcore::Value("");
  EXPECT_THROW(mysqlsh::dba::validate_local_address_option(options),
               shcore::Exception);

  // Error if the localAddress string is empty (only whitespace).
  (*options)["localAddress"] = shcore::Value("  ");
  EXPECT_THROW(mysqlsh::dba::validate_local_address_option(options),
               shcore::Exception);

  // Error if the localAddress has ':' and no host nor port part is specified.
  (*options)["localAddress"] = shcore::Value(" : ");
  EXPECT_THROW(mysqlsh::dba::validate_local_address_option(options),
               shcore::Exception);

  // No error if the localAddress is a non-empty string.
  (*options)["localAddress"] = shcore::Value("myhost:1234");
  EXPECT_NO_THROW(mysqlsh::dba::validate_local_address_option(options));
  (*options)["localAddress"] = shcore::Value("myhost:");
  EXPECT_NO_THROW(mysqlsh::dba::validate_local_address_option(options));
  (*options)["localAddress"] = shcore::Value(":1234");
  EXPECT_NO_THROW(mysqlsh::dba::validate_local_address_option(options));
  (*options)["localAddress"] = shcore::Value("myhost");
  EXPECT_NO_THROW(mysqlsh::dba::validate_local_address_option(options));
  (*options)["localAddress"] = shcore::Value("1234");
  EXPECT_NO_THROW(mysqlsh::dba::validate_local_address_option(options));
}

TEST(mod_dba_common, validate_group_seeds_option) {
  shcore::Value::Map_type_ref options(new shcore::Value::Map_type);

  // Error if the groupSeeds is empty.
  (*options)["groupSeeds"] = shcore::Value("");
  EXPECT_THROW(mysqlsh::dba::validate_group_seeds_option(options),
               shcore::Exception);

  // Error if the groupSeeds string is empty (only whitespace).
  (*options)["groupSeeds"] = shcore::Value("  ");
  EXPECT_THROW(mysqlsh::dba::validate_group_seeds_option(options),
               shcore::Exception);

  // No error if the groupSeeds is a non-empty string.
  (*options)["groupSeeds"] = shcore::Value("host1:1234,host2:4321");
  EXPECT_NO_THROW(mysqlsh::dba::validate_group_seeds_option(options));
}
