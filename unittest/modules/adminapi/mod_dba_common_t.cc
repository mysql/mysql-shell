/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <string>
#include "modules/adminapi/mod_dba_common.h"
#include "modules/adminapi/mod_dba_metadata_storage.h"
#include "modules/mod_shell.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "scripting/types.h"
#include "src/interactive/interactive_global_dba.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils/admin_api_test.h"
#include "unittest/test_utils/mod_testutils.h"
#include "unittest/test_utils/shell_test_wrapper.h"

using mysqlshdk::mysql::Instance;
using mysqlshdk::mysql::Var_qualifier;
namespace tests {

class Dba_common_test : public Admin_api_test {
 public:
  virtual void SetUp() {
    Admin_api_test::SetUp();
    reset_replayable_shell();
  }

 protected:
  static std::shared_ptr<mysqlshdk::db::ISession> create_session(
      int port, std::string user = "root") {
    auto session = mysqlshdk::db::mysql::Session::create();

    auto connection_options = shcore::get_connection_options(
        user + ":root@localhost:" + std::to_string(port), false);
    session->connect(connection_options);

    return session;
  }
  std::shared_ptr<mysqlshdk::db::ISession> create_base_session(int port) {
    auto session = mysqlshdk::db::mysql::Session::create();

    mysqlshdk::db::Connection_options connection_options;
    connection_options.set_host("localhost");
    connection_options.set_port(port);
    connection_options.set_user("user");
    connection_options.set_password("");

    session->connect(connection_options);

    return session;
  }

  void disable_ssl_on_instance(int port, const std::string &unsecure_user) {
    auto session = create_session(port);
    session->query("create user " + unsecure_user +
                   "@'%' identified with "
                   "mysql_native_password by 'root'");
    session->close();

    testutil->stop_sandbox(port);
    testutil->change_sandbox_conf(port, "ssl", "0", "mysqld");
    testutil->change_sandbox_conf(port, "default_authentication_plugin",
                                  "mysql_native_password", "mysqld");
    testutil->start_sandbox(port);
  }
};

TEST_F(Dba_common_test, resolve_cluster_ssl_mode_on_instance_with_ssl) {
  testutil->deploy_sandbox(_mysql_sandbox_port1, "root");
  auto session = create_session(_mysql_sandbox_port1);
  Instance instance(session);

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     ""            ON
  instance.set_sysvar("require_secure_transport", true, Var_qualifier::GLOBAL);
  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session, "");
    EXPECT_STREQ("REQUIRED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at require_secure_transport=ON, "
        "memberSslMode=''");
    ADD_FAILURE();
  }

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     "AUTO"        ON
  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session, "AUTO");
    EXPECT_STREQ("REQUIRED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at require_secure_transport=ON, "
        "memberSslMode=AUTO");
    ADD_FAILURE();
  }

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     "REQUIRED"   ON
  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session, "REQUIRED");
    EXPECT_STREQ("REQUIRED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at require_secure_transport=ON, "
        "memberSslMode=REQUIRED");
    ADD_FAILURE();
  }

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     "DISABLED"    ON
  try {
    mysqlsh::dba::resolve_cluster_ssl_mode(session, "DISABLED");
    SCOPED_TRACE(
        "Unexpected success at require_secure_transport=ON, "
        "memberSslMode=DISABLED");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS(
        "The instance '" + session->uri() +
            "' requires "
            "secure connections, to create the cluster either turn off "
            "require_secure_transport or use the memberSslMode option "
            "with 'REQUIRED' value.",
        error);
  }

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     ""            OFF
  instance.set_sysvar("require_secure_transport", false, Var_qualifier::GLOBAL);
  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session, "");
    EXPECT_STREQ("REQUIRED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at require_secure_transport=OFF, "
        "memberSslMode=''");
    ADD_FAILURE();
  }

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     "AUTO"       OFF
  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session, "AUTO");
    EXPECT_STREQ("REQUIRED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at require_secure_transport=OFF, "
        "memberSslMode=AUTO");
    ADD_FAILURE();
  }

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     "REQUIRED"   OFF
  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session, "REQUIRED");
    EXPECT_STREQ("REQUIRED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at require_secure_transport=OFF, "
        "memberSslMode=REQUIRED");
    ADD_FAILURE();
  }

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     "DISABLED"    OFF
  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session, "DISABLED");
    EXPECT_STREQ("DISABLED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at require_secure_transport=OFF, "
        "memberSslMode=DISABLED");
    ADD_FAILURE();
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_port1);
}

TEST_F(Dba_common_test, resolve_cluster_ssl_mode_on_instance_without_ssl) {
  testutil->deploy_sandbox(_mysql_sandbox_port1, "root");
  disable_ssl_on_instance(_mysql_sandbox_port1, "unsecure");

  auto session = create_session(_mysql_sandbox_port1, "unsecure");

  Instance instance(session);

  // InstanceSSL memberSslMode
  //----------- -------------
  // disabled    "REQUIRED"
  try {
    mysqlsh::dba::resolve_cluster_ssl_mode(session, "REQUIRED");
    SCOPED_TRACE("Unexpected success at resolve_cluster_ssl_mode_007");
    ADD_FAILURE();

  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS("The instance '" + session->uri() +
                                  "' does not "
                                  "have SSL enabled, to create the cluster "
                                  "either use an instance with SSL "
                                  "enabled, remove the memberSslMode option or "
                                  "use it with any of 'AUTO' or "
                                  "'DISABLED'.",
                              error);
  }

  // InstanceSSL memberSslMode
  //----------- -------------
  // disabled    ""
  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session, "");
    EXPECT_STREQ("DISABLED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_cluster_ssl_mode_008");
    ADD_FAILURE();
  }

  // InstanceSSL memberSslMode
  //----------- -------------
  // disabled    "AUTO"
  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session, "AUTO");
    EXPECT_STREQ("DISABLED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_cluster_ssl_mode_010");
    ADD_FAILURE();
  }

  // InstanceSSL memberSslMode
  //----------- -------------
  // disabled    "DISABLED"

  try {
    auto ssl_mode = mysqlsh::dba::resolve_cluster_ssl_mode(session, "DISABLED");
    EXPECT_STREQ("DISABLED", ssl_mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_cluster_ssl_mode_009");
    ADD_FAILURE();
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_port1);
}

TEST_F(Dba_common_test, resolve_instance_ssl_cluster_with_ssl_required) {
  testutil->deploy_sandbox(_mysql_sandbox_port1, "root");
  testutil->deploy_sandbox(_mysql_sandbox_port2, "root");
  execute("shell.connect('root:root@localhost:" +
          std::to_string(_mysql_sandbox_port1) + "')");

  testutil->expect_prompt(
      "Should the configuration be changed accordingly? [y/N]: ", "y");
#ifdef HAVE_V8
  execute("var c = dba.createCluster('sample', {memberSslMode:'REQUIRED'})");
#else
  execute("c = dba.create_cluster('sample', {'memberSslMode':'REQUIRED'})");
#endif
  execute("c.disconnect()");

  auto peer_session = create_session(_mysql_sandbox_port1);
  auto instance_session = create_session(_mysql_sandbox_port2);

  // Cluster SSL memberSslMode
  //----------- -------------
  // REQUIRED    ""
  try {
    auto mode = mysqlsh::dba::resolve_instance_ssl_mode(instance_session,
                                                        peer_session, "");
    EXPECT_STREQ("REQUIRED", mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure with memberSslMode='', instance with SSL");
    ADD_FAILURE();
  }

  // Cluster SSL memberSslMode Instance SSL
  //----------- ------------- ------------
  // REQUIRED    AUTO          enabled
  try {
    auto mode = mysqlsh::dba::resolve_instance_ssl_mode(instance_session,
                                                        peer_session, "AUTO");
    EXPECT_STREQ("REQUIRED", mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure with memberSslMode='AUTO', instance with "
        "SSL");
    ADD_FAILURE();
  }

  // Cluster SSL memberSslMode Instance SSL
  //----------- ------------- ------------
  // REQUIRED    REQUIRED      enabled
  try {
    auto mode = mysqlsh::dba::resolve_instance_ssl_mode(
        instance_session, peer_session, "REQUIRED");
    EXPECT_STREQ("REQUIRED", mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at memberSslMode='REQUIRED', instance "
        "with SSL");
    ADD_FAILURE();
  }

  // Cluster SSL memberSslMode
  //----------- -------------
  // REQUIRED    DISABLED
  try {
    mysqlsh::dba::resolve_instance_ssl_mode(instance_session, peer_session,
                                            "DISABLED");
    SCOPED_TRACE("Unexpected success at memberSslMode='REQUIRED'");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS(
        "The cluster has SSL (encryption) enabled. "
        "To add the instance '" +
            instance_session->uri() +
            "' to the "
            "cluster either disable SSL on the cluster, remove the "
            "memberSslMode "
            "option or use it with any of 'AUTO' or 'REQUIRED'.",
        error);
  }

  instance_session->close();
  disable_ssl_on_instance(_mysql_sandbox_port2, "unsecure");
  instance_session = create_session(_mysql_sandbox_port2, "unsecure");

  // Cluster SSL memberSslMode Instance SSL
  //----------- ------------- ------------
  // REQUIRED    AUTO          disabled
  try {
    mysqlsh::dba::resolve_instance_ssl_mode(instance_session, peer_session,
                                            "AUTO");
    SCOPED_TRACE("Unexpected success at instance with no SSL");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS(
        "Instance '" + instance_session->uri() +
            "' does "
            "not support SSL and cannot join a cluster with SSL (encryption) "
            "enabled. Enable SSL support on the instance and try again, "
            "otherwise "
            "it can only be added to a cluster with SSL disabled.",
        error);
  }

  // Cluster SSL memberSslMode Instance SSL
  //----------- ------------- ------------
  // REQUIRED    REQUIRED      disabled
  try {
    mysqlsh::dba::resolve_instance_ssl_mode(instance_session, peer_session,
                                            "REQUIRED");
    SCOPED_TRACE("Unexpected success at instance with no SSL");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS(
        "Instance '" + instance_session->uri() +
            "' does "
            "not support SSL and cannot join a cluster with SSL (encryption) "
            "enabled. Enable SSL support on the instance and try again, "
            "otherwise "
            "it can only be added to a cluster with SSL disabled.",
        error);
  }

  peer_session->close();
  instance_session->close();
  testutil->destroy_sandbox(_mysql_sandbox_port1);
  testutil->destroy_sandbox(_mysql_sandbox_port2);
}

TEST_F(Dba_common_test, resolve_instance_ssl_cluster_with_ssl_disabled) {
  testutil->deploy_sandbox(_mysql_sandbox_port1, "root");
  testutil->deploy_sandbox(_mysql_sandbox_port2, "root");
  execute("shell.connect('root:root@localhost:" +
          std::to_string(_mysql_sandbox_port1) + "')");

  testutil->expect_prompt(
      "Should the configuration be changed accordingly? [y/N]: ", "y");
#ifdef HAVE_V8
  execute("var c = dba.createCluster('sample', {memberSslMode:'DISABLED'})");
#else
  execute("c = dba.create_cluster('sample', {'memberSslMode':'DISABLED'})");
#endif
  execute("c.disconnect()");

  auto peer_session = create_session(_mysql_sandbox_port1);
  auto instance_session = create_session(_mysql_sandbox_port2);

  // Cluster SSL memberSslMode
  //----------- -------------
  // DISABLED    REQUIRED
  try {
    mysqlsh::dba::resolve_instance_ssl_mode(instance_session, peer_session,
                                            "REQUIRED");
    SCOPED_TRACE("Unexpected success using memberSslMode=REQUIRED");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS(
        "The cluster has SSL (encryption) disabled. "
        "To add the instance '" +
            instance_session->uri() +
            "' to the "
            "cluster either enable SSL on the cluster, remove the "
            "memberSslMode "
            "option or use it with any of 'AUTO' or 'DISABLED'.",
        error);
  }

  // Cluster SSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // DISABLED    ""            OFF
  try {
    auto mode = mysqlsh::dba::resolve_instance_ssl_mode(instance_session,
                                                        peer_session, "");
    EXPECT_STREQ("DISABLED", mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure using memberSslMode=''");
    ADD_FAILURE();
  }

  // Cluster SSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // DISABLED    AUTO          OFF
  try {
    auto mode = mysqlsh::dba::resolve_instance_ssl_mode(instance_session,
                                                        peer_session, "AUTO");
    EXPECT_STREQ("DISABLED", mode.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure using memberSslMode=AUTO");
    ADD_FAILURE();
  }

  Instance instance(instance_session);
  instance.set_sysvar("require_secure_transport", true, Var_qualifier::GLOBAL);

  // Cluster SSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // DISABLED    ""            ON
  try {
    mysqlsh::dba::resolve_instance_ssl_mode(instance_session, peer_session, "");
    SCOPED_TRACE(
        "Unexpected success at instance with require_secure_transport"
        "=ON and memberSslMode=''");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS(
        "The instance '" + instance_session->uri() +
            "' "
            "is configured to require a secure transport but the cluster has "
            "SSL "
            "disabled. To add the instance to the cluster, either turn OFF the "
            "require_secure_transport option on the instance or enable SSL on "
            "the cluster.",
        error);
  }

  // Cluster SSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // DISABLED    AUTO          ON
  try {
    mysqlsh::dba::resolve_instance_ssl_mode(instance_session, peer_session,
                                            "AUTO");
    SCOPED_TRACE(
        "Unexpected success at instance with require_secure_transport"
        "=ON and memberSslMode=AUTO");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    std::string error = e.what();
    MY_EXPECT_OUTPUT_CONTAINS(
        "The instance '" + instance_session->uri() +
            "' "
            "is configured to require a secure transport but the cluster has "
            "SSL "
            "disabled. To add the instance to the cluster, either turn OFF the "
            "require_secure_transport option on the instance or enable SSL on "
            "the cluster.",
        error);
  }

  instance_session->close();
  disable_ssl_on_instance(_mysql_sandbox_port2, "unsecure");
  instance_session = create_session(_mysql_sandbox_port2, "unsecure");

  peer_session->close();
  instance_session->close();
  testutil->destroy_sandbox(_mysql_sandbox_port1);
  testutil->destroy_sandbox(_mysql_sandbox_port2);
}

class Dba_common_cluster_functions : public Dba_common_test {
 public:
  static void SetUpTestCase() {
    SetUpSampleCluster("Dba_common_cluster_functions/SetUpTestCase");
  }

  static void TearDownTestCase() {
    TearDownSampleCluster("Dba_common_cluster_functions/TearDownTestCase");
  }
};

TEST_F(Dba_common_cluster_functions, get_instances_gr) {
  auto md_session = create_session(_mysql_sandbox_port1);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  try {
    auto result = mysqlsh::dba::get_instances_gr(metadata);

    auto pos1 = std::find(result.begin(), result.end(), uuid_1);
    EXPECT_TRUE(pos1 != result.end());

    auto pos2 = std::find(result.begin(), result.end(), uuid_2);
    EXPECT_TRUE(pos2 != result.end());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at get_instances_gr");
    ADD_FAILURE();
  }

  md_session->close();
}

TEST_F(Dba_common_cluster_functions, get_instances_md) {
  auto md_session = create_session(_mysql_sandbox_port1);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  try {
    auto result = mysqlsh::dba::get_instances_md(metadata, 1);

    auto pos1 = std::find(result.begin(), result.end(), uuid_1);
    EXPECT_TRUE(pos1 != result.end());

    auto pos2 = std::find(result.begin(), result.end(), uuid_2);
    EXPECT_TRUE(pos2 != result.end());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at get_instances_md");
    ADD_FAILURE();
  }

  md_session->close();
}

// If the information on the Metadata and the GR group
// P_S info is the same get_newly_discovered_instances()
// result return an empty list
TEST_F(Dba_common_cluster_functions, get_newly_discovered_instances) {
  auto md_session = create_session(_mysql_sandbox_port1);

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

  md_session->close();
}

// If the information on the Metadata and the GR group
// P_S info is the same get_unavailable_instances()
// should return an empty list
TEST_F(Dba_common_cluster_functions, get_unavailable_instances) {
  auto md_session = create_session(_mysql_sandbox_port1);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  try {
    auto unavailable_instances_list(get_unavailable_instances(metadata, 1));

    EXPECT_TRUE(unavailable_instances_list.empty());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at get_unavailable_instances_001");
    ADD_FAILURE();
  }

  md_session->close();
}

TEST_F(Dba_common_cluster_functions, get_gr_replicaset_group_name) {
  auto session = create_session(_mysql_sandbox_port1);

  try {
    std::string result = mysqlsh::dba::get_gr_replicaset_group_name(session);

    EXPECT_STREQ(group_name.c_str(), result.c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at get_gr_replicaset_group_name");
    ADD_FAILURE();
  }

  session->close();
}

TEST_F(Dba_common_cluster_functions, validate_instance_rejoinable_01) {
  // There are missing instances and the instance we are checking belongs to
  // the metadata list but does not belong to the GR list.

  auto md_session = create_session(_mysql_sandbox_port1);
  auto instance_session = create_session(_mysql_sandbox_port3);

  // Insert a fake record for the third instance on the metadata
  std::string query =
      "insert into mysql_innodb_cluster_metadata.instances "
      "values (0, 1, " +
      std::to_string(_replicaset->get_id()) + ", '" + uuid_3 +
      "', 'localhost:<port>', "
      "'HA', NULL, '{\"mysqlX\": \"localhost:<port>0\", "
      "\"grLocal\": \"localhost:1<port>\", "
      "\"mysqlClassic\": \"localhost:<port>\"}', "
      "NULL, NULL, NULL)";

  query = shcore::str_replace(query, "<port>",
                              std::to_string(_mysql_sandbox_port3));

  md_session->query(query);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  try {
    bool is_rejoinable(
        validate_instance_rejoinable(instance_session, metadata, 1));

    EXPECT_TRUE(is_rejoinable);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at validate_instance_rejoinable_01");
    ADD_FAILURE();
  }

  md_session->query(
      "delete from mysql_innodb_cluster_metadata.instances "
      " where mysql_server_uuid = '" +
      uuid_3 + "'");

  md_session->close();
  instance_session->close();
}

TEST_F(Dba_common_cluster_functions, validate_instance_rejoinable_02) {
  // There are missing instances and the instance we are checking belongs
  // to neither the metadata nor GR lists.

  auto md_session = create_session(_mysql_sandbox_port1);
  auto instance_session = create_session(_mysql_sandbox_port3);

  // Insert a fake record for the third instance on the metadata
  std::string query =
      "insert into mysql_innodb_cluster_metadata.instances "
      "values (0, 1, " +
      std::to_string(_replicaset->get_id()) +
      ", '11111111-2222-3333-4444-555555555555', "
      "'localhost:<port>', 'HA', NULL, "
      "'{\"mysqlX\": \"localhost:<port>0\", "
      "\"grLocal\": \"localhost:1<port>\", "
      "\"mysqlClassic\": \"localhost:<port>\"}', "
      "NULL, NULL, NULL)";

  query = shcore::str_replace(query, "<port>",
                              std::to_string(_mysql_sandbox_port3));

  md_session->query(query);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  try {
    bool is_rejoinable(
        validate_instance_rejoinable(instance_session, metadata, 1));

    EXPECT_FALSE(is_rejoinable);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at validate_instance_rejoinable_02");
    ADD_FAILURE();
  }

  md_session->query(
      "delete from mysql_innodb_cluster_metadata.instances "
      " where mysql_server_uuid = '11111111-2222-3333-4444-"
      "555555555555'");

  md_session->close();
  instance_session->close();
}

TEST_F(Dba_common_cluster_functions, validate_instance_rejoinable_03) {
  // There are no missing instances and the instance we are checking belongs
  // to both the metadata and GR lists.
  auto md_session = create_session(_mysql_sandbox_port1);
  auto instance_session = create_session(_mysql_sandbox_port2);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_session));

  try {
    bool is_rejoinable(
        validate_instance_rejoinable(instance_session, metadata, 1));

    EXPECT_FALSE(is_rejoinable);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at validate_instance_rejoinable_03");
    ADD_FAILURE();
  }

  md_session->close();
  instance_session->close();
}

TEST_F(Dba_common_test, super_read_only_server_on_flag_true) {
  enable_replay();
  testutil->deploy_sandbox(_mysql_sandbox_port1, "root");
  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(
      testutil->sandbox_connection_options(_mysql_sandbox_port1, "root"));

  // super_read_only is ON, no active sessions
  session->query("set global super_read_only = 1");

  try {
    auto read_only = mysqlsh::dba::validate_super_read_only(session, true);
    EXPECT_TRUE(read_only);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at super_read_only_server_on_flag_true");
    ADD_FAILURE();
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_port1);
}

TEST_F(Dba_common_test, super_read_only_server_on_flag_false_open_sessions) {
  enable_replay();
  testutil->deploy_sandbox(_mysql_sandbox_port1, "root");
  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(
      testutil->sandbox_connection_options(_mysql_sandbox_port1, "root"));

  auto extra_session = mysqlshdk::db::mysql::Session::create();
  extra_session->connect(
      testutil->sandbox_connection_options(_mysql_sandbox_port1, "root"));

  // super_read_only is ON, no active sessions
  session->query("set global super_read_only = 1");

  try {
    mysqlsh::dba::validate_super_read_only(session, false);
    SCOPED_TRACE("Unexpected success calling validate_super_read_only");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ("Server in SUPER_READ_ONLY mode", e.what());
  }

  session->close();
  extra_session->close();
  testutil->destroy_sandbox(_mysql_sandbox_port1);
}

TEST_F(Dba_common_test, super_read_only_server_on_flag_false_no_open_sessions) {
  enable_replay();
  testutil->deploy_sandbox(_mysql_sandbox_port1, "root");
  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(
      testutil->sandbox_connection_options(_mysql_sandbox_port1, "root"));

  // super_read_only is ON, no active sessions
  session->query("set global super_read_only = 1");
  try {
    mysqlsh::dba::validate_super_read_only(session, false);
    SCOPED_TRACE("Unexpected success calling validate_super_read_only");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ("Server in SUPER_READ_ONLY mode", e.what());
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_port1);
}

TEST_F(Dba_common_test, super_read_only_server_off_flag_true) {
  enable_replay();
  testutil->deploy_sandbox(_mysql_sandbox_port1, "root");
  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(
      testutil->sandbox_connection_options(_mysql_sandbox_port1, "root"));

  // super_read_only is OFF, no active sessions
  session->query("set global super_read_only = 0");

  try {
    auto read_only = mysqlsh::dba::validate_super_read_only(session, true);
    EXPECT_FALSE(read_only);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at super_read_only_server_on_flag_true");
    ADD_FAILURE();
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_port1);
}

TEST_F(Dba_common_test, super_read_only_server_off_flag_false) {
  enable_replay();
  testutil->deploy_sandbox(_mysql_sandbox_port1, "root");
  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(
      testutil->sandbox_connection_options(_mysql_sandbox_port1, "root"));

  // super_read_only is OFF, no active sessions
  session->query("set global super_read_only = 0");

  try {
    auto read_only = mysqlsh::dba::validate_super_read_only(session, false);
    EXPECT_FALSE(read_only);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at super_read_only_server_on_flag_true");
    ADD_FAILURE();
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_port1);
}

TEST_F(Dba_common_test, validate_ipwhitelist_option) {
  testutil->deploy_sandbox(_mysql_sandbox_port1, "root");
  auto session = create_session(_mysql_sandbox_port1);
  bool hostnames_supported = false;
  std::string ip_whitelist;

  if (session->get_server_version() >= mysqlshdk::utils::Version(8, 0, 4))
    hostnames_supported = true;

  // Error if the ipWhitelist is empty.
  ip_whitelist = "";
  try {
    mysqlsh::dba::validate_ip_whitelist_option(ip_whitelist,
                                               hostnames_supported);
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ("Invalid value for ipWhitelist: string value cannot be empty.",
                 e.what());
  }

  // Error if the ipWhitelist string is empty (only whitespace).
  ip_whitelist = " ";
  try {
    mysqlsh::dba::validate_ip_whitelist_option(ip_whitelist,
                                               hostnames_supported);
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ("Invalid value for ipWhitelist: string value cannot be empty.",
                 e.what());
  }

  // Error if CIDR is used but has an invalid value (not in range [1,32])
  ip_whitelist = "192.168.1.1/0";
  try {
    mysqlsh::dba::validate_ip_whitelist_option(ip_whitelist,
                                               hostnames_supported);
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist '192.168.1.1/0': subnet value in CIDR "
        "notation is not valid.",
        e.what());
  }

  // Error if CIDR is used but has an invalid value (not in range [1,32])
  ip_whitelist = "192.168.1.1/33";
  try {
    mysqlsh::dba::validate_ip_whitelist_option(ip_whitelist,
                                               hostnames_supported);
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist '192.168.1.1/33': subnet value in CIDR "
        "notation is not valid.",
        e.what());
  }

  // Error if CIDR is used but has an invalid value (not in range [1,32])
  ip_whitelist = "1/33";
  try {
    mysqlsh::dba::validate_ip_whitelist_option(ip_whitelist,
                                               hostnames_supported);
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist '1/33': subnet value in CIDR "
        "notation is not valid.",
        e.what());
  }

  // Error if CIDR is used but has an invalid value (not in range [1,32])
  // And a list of values is used
  ip_whitelist = "192.168.1.1/0,192.168.1.1/33";
  try {
    mysqlsh::dba::validate_ip_whitelist_option(ip_whitelist,
                                               hostnames_supported);
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist '192.168.1.1/0': subnet value in CIDR "
        "notation is not valid.",
        e.what());
  }

  // Error if ipWhitelist is an IPv6 address
  ip_whitelist = "2001:0db8:85a3:0000:0000:8a2e:0370:7334";
  try {
    mysqlsh::dba::validate_ip_whitelist_option(ip_whitelist,
                                               hostnames_supported);
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist "
        "'2001:0db8:85a3:0000:0000:8a2e:0370:7334': IPv6 not "
        "supported.",
        e.what());
  }

  // Error if ipWhitelist is not a valid IPv4 address
  ip_whitelist = "256.255.255.255";
  try {
    mysqlsh::dba::validate_ip_whitelist_option(ip_whitelist,
                                               hostnames_supported);
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    if (!hostnames_supported) {
      EXPECT_STREQ(
          "Invalid value for ipWhitelist '256.255.255.255': string value is "
          "not a valid IPv4 address.",
          e.what());
    } else {
      EXPECT_STREQ(
          "Invalid value for ipWhitelist '256.255.255.255': address does not "
          "resolve to a valid IPv4 address.",
          e.what());
    }
  }

  // Error if ipWhitelist is not a valid IPv4 address
  ip_whitelist = "256.255.255.255/16";
  try {
    mysqlsh::dba::validate_ip_whitelist_option(ip_whitelist,
                                               hostnames_supported);
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist '256.255.255.255/16': CIDR notation "
        "can only be used with IPv4 addresses.",
        e.what());
  }

  // Error if hostname is used and server version < 8.0.4
  if (!hostnames_supported) {
    ip_whitelist = "localhost";
    try {
      mysqlsh::dba::validate_ip_whitelist_option(ip_whitelist,
                                                 hostnames_supported);
      SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
      ADD_FAILURE();
    } catch (const shcore::Exception &e) {
      EXPECT_STREQ(
          "Invalid value for ipWhitelist 'localhost': string value is not a "
          "valid IPv4 address.",
          e.what());
    }
  } else {
    ip_whitelist = "1invalid_hostname0";
    try {
      mysqlsh::dba::validate_ip_whitelist_option(ip_whitelist,
                                                 hostnames_supported);
      SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
      ADD_FAILURE();
    } catch (const shcore::Exception &e) {
      EXPECT_STREQ(
          "Invalid value for ipWhitelist '1invalid_hostname0': address does "
          "not resolve to a valid IPv4 address.",
          e.what());
    }
  }

  // Error if hostname with cidr
  ip_whitelist = "localhost/8";
  try {
    mysqlsh::dba::validate_ip_whitelist_option(ip_whitelist,
                                               hostnames_supported);
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist 'localhost/8': CIDR notation can only "
        "be used with IPv4 addresses.",
        e.what());
  }

  // Error if hostname with cidr
  ip_whitelist = "bogus/8";
  try {
    mysqlsh::dba::validate_ip_whitelist_option(ip_whitelist,
                                               hostnames_supported);
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist 'bogus/8': CIDR notation can only "
        "be used with IPv4 addresses.",
        e.what());
  }

  // No error if the ipWhitelist is a valid IPv4 address
  ip_whitelist = "192.168.1.1";
  EXPECT_NO_THROW(mysqlsh::dba::validate_ip_whitelist_option(
      ip_whitelist, hostnames_supported));

  // No error if the ipWhitelist is a valid IPv4 address with a valid CIDR value
  ip_whitelist = "192.168.1.1/15";
  EXPECT_NO_THROW(mysqlsh::dba::validate_ip_whitelist_option(
      ip_whitelist, hostnames_supported));

  // No error if the ipWhitelist consist of several valid IPv4 addresses with a
  // valid CIDR value
  // NOTE: if the server version is > 8.0.4, hostnames are allowed too so we
  // must test it
  if (hostnames_supported) {
    ip_whitelist = "192.168.1.1/15,192.169.1.1/1, localhost";
  } else {
    ip_whitelist = "192.168.1.1/15,192.169.1.1/1";
  }
  EXPECT_NO_THROW(mysqlsh::dba::validate_ip_whitelist_option(
      ip_whitelist, hostnames_supported));

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_port1);
}

TEST_F(Dba_common_test, validate_exit_state_action_supported) {
  testutil->deploy_sandbox(_mysql_sandbox_port1, "root");
  auto session = create_session(_mysql_sandbox_port1);
  shcore::Value::Map_type_ref options(new shcore::Value::Map_type);

  auto version = session->get_server_version();

  (*options)["exitStateAction"] = shcore::Value("1");

  // Error only if the target server version is >= 5.7.24 if 5.0, or >= 8.0.12
  // if 8.0.
  if (version < mysqlshdk::utils::Version(5, 7, 24) ||
      (version >= mysqlshdk::utils::Version(8, 0, 0) &&
       version < mysqlshdk::utils::Version(8, 0, 12))) {
    EXPECT_THROW_LIKE(
        mysqlsh::dba::validate_exit_state_action_supported(session),
        shcore::Exception,
        "Option 'memberWeight' not supported on target server "
        "version:");
  } else {
    EXPECT_NO_THROW(
        mysqlsh::dba::validate_exit_state_action_supported(session));
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_port1);
}

TEST_F(Dba_common_test, validate_member_weight_supported) {
  testutil->deploy_sandbox(_mysql_sandbox_port1, "root");
  auto session = create_session(_mysql_sandbox_port1);
  shcore::Value::Map_type_ref options(new shcore::Value::Map_type);

  auto version = session->get_server_version();

  (*options)["memberWeight"] = shcore::Value(1);

  // Error only if the target server version is < 5.7.20 if 5.0, or < 8.0.11
  // if 8.0.
  if (version < mysqlshdk::utils::Version(5, 7, 20) ||
      (version >= mysqlshdk::utils::Version(8, 0, 0) &&
       version < mysqlshdk::utils::Version(8, 0, 11))) {
    EXPECT_THROW_LIKE(mysqlsh::dba::validate_member_weight_supported(session),
                      shcore::Exception,
                      "Option 'memberWeight' not supported on target server "
                      "version:");
  } else {
    EXPECT_NO_THROW(mysqlsh::dba::validate_member_weight_supported(session));
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_port1);
}

TEST_F(Dba_common_test, validate_failover_consistency_supported) {
  testutil->deploy_sandbox(_mysql_sandbox_port1, "root");
  auto session = create_session(_mysql_sandbox_port1);
  auto version = session->get_server_version();
  auto empty_fail_cons = mysqlshdk::utils::nullable<std::string>("  ");
  auto null_fail_cons = mysqlshdk::utils::nullable<std::string>();
  auto valid_fail_cons = mysqlshdk::utils::nullable<std::string>("1");
  // if a null value was provided, it is as if the option was not provided,
  // so no error should be thrown
  mysqlsh::dba::validate_failover_consistency_supported(session,
                                                        null_fail_cons);
  // if an empty value was provided, an error should be thrown independently
  // of the server version
  EXPECT_THROW_LIKE(
      mysqlsh::dba::validate_failover_consistency_supported(session,
                                                            empty_fail_cons),
      shcore::Exception,
      "Invalid value for failoverConsistency, string value cannot be empty.");
  // if a valid value (non empty) was provided, an error should only be thrown
  // in case the option is not supported by the server version.
  if (version < mysqlshdk::utils::Version(8, 0, 14)) {
    EXPECT_THROW_LIKE(
        mysqlsh::dba::validate_failover_consistency_supported(session,
                                                              valid_fail_cons),
        std::runtime_error,
        "Option 'failoverConsistency' not supported on target server "
        "version:");
  } else {
    EXPECT_NO_THROW(mysqlsh::dba::validate_failover_consistency_supported(
        session, valid_fail_cons));
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_port1);
}

TEST_F(Dba_common_test, validate_expel_timeout_supported) {
  testutil->deploy_sandbox(_mysql_sandbox_port1, "root");
  auto session = create_session(_mysql_sandbox_port1);
  auto version = session->get_server_version();
  auto null_timeout = mysqlshdk::utils::nullable<int64_t>();
  auto valid_timeout = mysqlshdk::utils::nullable<std::int64_t>(3600);
  auto invalid_timeout1 = mysqlshdk::utils::nullable<std::int64_t>(3601);
  auto invalid_timeout2 = mysqlshdk::utils::nullable<std::int64_t>(-1);

  // if a null value was provided, it is as if the option was not provided,
  // so no error should be thrown
  mysqlsh::dba::validate_expel_timeout_supported(session, null_timeout);
  // if a value non in the allowed range value was provided, an error should be
  // thrown independently of the server version
  EXPECT_THROW_LIKE(
      mysqlsh::dba::validate_expel_timeout_supported(session, invalid_timeout1),
      shcore::Exception,
      "Invalid value for expelTimeout, integer value must be in the range: "
      "[0, 3600]");
  EXPECT_THROW_LIKE(
      mysqlsh::dba::validate_expel_timeout_supported(session, invalid_timeout2),
      shcore::Exception,
      "Invalid value for expelTimeout, integer value must be in the range: "
      "[0, 3600]");
  // if a valid value was provided, an error should only be thrown
  // in case the option is not supported by the server version.
  if (version < mysqlshdk::utils::Version(8, 0, 13)) {
    EXPECT_THROW_LIKE(
        mysqlsh::dba::validate_expel_timeout_supported(session, valid_timeout),
        std::runtime_error,
        "Option 'expelTimeout' not supported on target server "
        "version:");
  } else {
    EXPECT_NO_THROW(
        mysqlsh::dba::validate_expel_timeout_supported(session, valid_timeout));
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_port1);
}

TEST_F(Dba_common_test, is_group_replication_option_supported) {
  testutil->deploy_sandbox(_mysql_sandbox_port1, "root");
  auto session = create_session(_mysql_sandbox_port1);
  auto version = session->get_server_version();
  // if a non supported version is used, then we must throw an exception,
  // else just save the result for further testing
  if ((version.get_major() == 5 && version.get_minor() == 7) ||
      version.get_major() == 8) {
    mysqlsh::dba::is_group_replication_option_supported(
        session, mysqlsh::dba::kExitStateAction);
  } else {
    EXPECT_THROW_LIKE(mysqlsh::dba::is_group_replication_option_supported(
                          session, mysqlsh::dba::kExitStateAction),
                      std::runtime_error,
                      "Unexpected version found for GR option support check:");
  }
  // testing the result of exit-state action case since it has requirements for
  // both 5.7 and the 8.0 MySQL versions.
  if ((version < mysqlshdk::utils::Version(8, 0, 13) &&
       version >= mysqlshdk::utils::Version(8, 0, 0)) ||
      version < mysqlshdk::utils::Version(5, 7, 24)) {
    EXPECT_FALSE(mysqlsh::dba::is_group_replication_option_supported(
        session, mysqlsh::dba::kExitStateAction));
  } else if (version >= mysqlshdk::utils::Version(8, 0, 13) ||
             (version < mysqlshdk::utils::Version(8, 0, 0) &&
              version >= mysqlshdk::utils::Version(5, 7, 24))) {
    EXPECT_TRUE(mysqlsh::dba::is_group_replication_option_supported(
        session, mysqlsh::dba::kExitStateAction));
  }
  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_port1);
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
