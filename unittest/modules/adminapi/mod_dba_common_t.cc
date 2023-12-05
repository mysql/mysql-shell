/*
 * Copyright (c) 2017, 2023, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/accounts.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "scripting/types.h"
#include "unittest/gtest_clean.h"
#include "unittest/mysqlshdk/libs/mysql/user_privileges_t.h"
#include "unittest/test_utils/admin_api_test.h"
#include "unittest/test_utils/command_line_test.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"

using mysqlshdk::mysql::Instance;
using mysqlshdk::mysql::Var_qualifier;
using mysqlshdk::utils::Version;

namespace mysqlsh {
namespace dba {
extern void validate_ip_allowlist_option(
    const mysqlshdk::utils::Version &version, std::string_view ip_allowlist);
}  // namespace dba
}  // namespace mysqlsh

namespace testing {

class Admin_api_common_test : public tests::Admin_api_test {
 public:
  virtual void SetUp() {
    Admin_api_test::SetUp();
    reset_replayable_shell(
        ::testing::UnitTest::GetInstance()->current_test_info()->name());
  }

  virtual void TearDown() { Admin_api_test::TearDown(); }

 protected:
  static std::shared_ptr<mysqlsh::dba::Instance> create_session(
      int port, std::string user = "root") {
    auto connection_options = shcore::get_connection_options(
        user + ":root@localhost:" + std::to_string(port), false);
    return mysqlsh::dba::Instance::connect(connection_options);
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
    auto instance = create_session(port);

    instance->query("create user " + unsecure_user +
                    "@'%' identified with "
                    "mysql_native_password by /*((*/ 'root' /*))*/");

    testutil->stop_sandbox(port);
    testutil->change_sandbox_conf(port, "ssl", "0", "mysqld");
    testutil->change_sandbox_conf(port, "default_authentication_plugin",
                                  "mysql_native_password", "mysqld");
    testutil->start_sandbox(port);
  }
};

class Admin_api_cmdline_test : public tests::Command_line_test {};

TEST_F(Admin_api_common_test, resolve_cluster_ssl_mode_on_instance_with_ssl) {
  testutil->deploy_sandbox(_mysql_sandbox_ports[0], "root");
  auto instance = create_session(_mysql_sandbox_ports[0]);
  mysqlsh::dba::Cluster_ssl_mode member_ssl_mode;

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     "AUTO"        ON
  instance->set_sysvar("require_secure_transport", true, Var_qualifier::GLOBAL);
  try {
    member_ssl_mode = mysqlsh::dba::Cluster_ssl_mode::AUTO;
    mysqlsh::dba::resolve_ssl_mode_option("memberSslMode", "", *instance,
                                          &member_ssl_mode);
    EXPECT_STREQ("REQUIRED", to_string(member_ssl_mode).c_str());
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
    member_ssl_mode = mysqlsh::dba::Cluster_ssl_mode::REQUIRED;
    mysqlsh::dba::resolve_ssl_mode_option("memberSslMode", "", *instance,
                                          &member_ssl_mode);
    EXPECT_STREQ("REQUIRED", to_string(member_ssl_mode).c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at require_secure_transport=ON, "
        "memberSslMode=REQUIRED");
    ADD_FAILURE();
  }

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     "VERIFY_CA"   ON
  try {
    member_ssl_mode = mysqlsh::dba::Cluster_ssl_mode::VERIFY_CA;
    mysqlsh::dba::resolve_ssl_mode_option("memberSslMode", "", *instance,
                                          &member_ssl_mode);
    EXPECT_STREQ("VERIFY_CA", to_string(member_ssl_mode).c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at require_secure_transport=ON, "
        "memberSslMode=VERIFY_CA");
    ADD_FAILURE();
  }

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     "VERIFY_IDENTITY"   ON
  try {
    member_ssl_mode = mysqlsh::dba::Cluster_ssl_mode::VERIFY_IDENTITY;
    mysqlsh::dba::resolve_ssl_mode_option("memberSslMode", "", *instance,
                                          &member_ssl_mode);
    EXPECT_STREQ("VERIFY_IDENTITY", to_string(member_ssl_mode).c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at require_secure_transport=ON, "
        "memberSslMode=VERIFY_IDENTITY");
    ADD_FAILURE();
  }

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     "AUTO"       OFF
  instance->set_sysvar("require_secure_transport", false,
                       Var_qualifier::GLOBAL);
  try {
    member_ssl_mode = mysqlsh::dba::Cluster_ssl_mode::AUTO;
    mysqlsh::dba::resolve_ssl_mode_option("memberSslMode", "", *instance,
                                          &member_ssl_mode);
    EXPECT_STREQ("REQUIRED", to_string(member_ssl_mode).c_str());
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
    member_ssl_mode = mysqlsh::dba::Cluster_ssl_mode::REQUIRED;
    mysqlsh::dba::resolve_ssl_mode_option("memberSslMode", "", *instance,
                                          &member_ssl_mode);
    EXPECT_STREQ("REQUIRED", to_string(member_ssl_mode).c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at require_secure_transport=OFF, "
        "memberSslMode=REQUIRED");
    ADD_FAILURE();
  }

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     "VERIFY_CA"   OFF
  try {
    member_ssl_mode = mysqlsh::dba::Cluster_ssl_mode::VERIFY_CA;
    mysqlsh::dba::resolve_ssl_mode_option("memberSslMode", "", *instance,
                                          &member_ssl_mode);
    EXPECT_STREQ("VERIFY_CA", to_string(member_ssl_mode).c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at require_secure_transport=OFF, "
        "memberSslMode=REQUIRED");
    ADD_FAILURE();
  }

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     "VERIFY_IDENTITY"   OFF
  try {
    member_ssl_mode = mysqlsh::dba::Cluster_ssl_mode::VERIFY_IDENTITY;
    mysqlsh::dba::resolve_ssl_mode_option("memberSslMode", "", *instance,
                                          &member_ssl_mode);
    EXPECT_STREQ("VERIFY_IDENTITY", to_string(member_ssl_mode).c_str());
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
    member_ssl_mode = mysqlsh::dba::Cluster_ssl_mode::DISABLED;
    mysqlsh::dba::resolve_ssl_mode_option("memberSslMode", "", *instance,
                                          &member_ssl_mode);
    EXPECT_STREQ("DISABLED", to_string(member_ssl_mode).c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at require_secure_transport=OFF, "
        "memberSslMode=DISABLED");
    ADD_FAILURE();
  }

  testutil->destroy_sandbox(_mysql_sandbox_ports[0]);
}

TEST_F(Admin_api_common_test,
       resolve_cluster_ssl_mode_on_instance_without_ssl) {
  testutil->deploy_sandbox(_mysql_sandbox_ports[0], "root");
  disable_ssl_on_instance(_mysql_sandbox_ports[0], "unsecure");
  mysqlsh::dba::Cluster_ssl_mode member_ssl_mode;

  auto instance = create_session(_mysql_sandbox_ports[0], "unsecure");

  // InstanceSSL memberSslMode
  //----------- -------------
  // disabled    "REQUIRED"
  // Tested in create_cluster_neg.js

  // InstanceSSL memberSslMode
  //----------- -------------
  // disabled    "AUTO"
  try {
    member_ssl_mode = mysqlsh::dba::Cluster_ssl_mode::AUTO;
    mysqlsh::dba::resolve_ssl_mode_option("memberSslMode", "", *instance,
                                          &member_ssl_mode);
    EXPECT_STREQ("DISABLED", to_string(member_ssl_mode).c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_cluster_ssl_mode_010");
    ADD_FAILURE();
  }

  // InstanceSSL memberSslMode
  //----------- -------------
  // disabled    "DISABLED"

  try {
    member_ssl_mode = mysqlsh::dba::Cluster_ssl_mode::DISABLED;
    mysqlsh::dba::resolve_ssl_mode_option("memberSslMode", "", *instance,
                                          &member_ssl_mode);
    EXPECT_STREQ("DISABLED", to_string(member_ssl_mode).c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_cluster_ssl_mode_009");
    ADD_FAILURE();
  }

  testutil->destroy_sandbox(_mysql_sandbox_ports[0]);
}

TEST_F(Admin_api_common_test, resolve_instance_ssl_cluster_with_ssl_required) {
  shcore::Dictionary_t sandbox_opts = shcore::make_dict();
  (*sandbox_opts)["report_host"] = shcore::Value(hostname());

  testutil->deploy_sandbox(_mysql_sandbox_ports[0], "root", sandbox_opts);
  testutil->deploy_sandbox(_mysql_sandbox_ports[1], "root", sandbox_opts);
  execute("shell.connect('root:root@localhost:" +
          std::to_string(_mysql_sandbox_ports[0]) + "')");

  testutil->expect_prompt(
      "Should the configuration be changed accordingly? [y/N]: ", "y");
#ifdef HAVE_V8
  execute(
      "var c = dba.createCluster('sample', {memberSslMode:'REQUIRED', "
      "gtidSetIsComplete: true})");
#else
  execute(
      "c = dba.create_cluster('sample', {'memberSslMode':'REQUIRED', "
      "gtidSetIsComplete: true})");
#endif
  execute("c.disconnect()");
  execute("session.close()");

  auto peer = create_session(_mysql_sandbox_ports[0]);
  auto instance = create_session(_mysql_sandbox_ports[1]);

  try {
    auto member_ssl_mode =
        mysqlsh::dba::resolve_instance_ssl_mode_option(*instance, *peer);
    EXPECT_STREQ("REQUIRED", to_string(member_ssl_mode).c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure with memberSslMode='AUTO', instance with "
        "SSL");
    ADD_FAILURE();
  }

  // Cluster SSL  memberSslMode     Instance SSL
  //-----------   ---------------   ------------
  // VERIFY_CA    "AUTO"            enabled
  try {
    peer->execute("SET GLOBAL group_replication_ssl_mode=VERIFY_CA");
    auto member_ssl_mode =
        mysqlsh::dba::resolve_instance_ssl_mode_option(*instance, *peer);
    EXPECT_STREQ("VERIFY_CA", to_string(member_ssl_mode).c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at memberSslMode='VERIFY_CA', instance "
        "with SSL");
    ADD_FAILURE();
  }

  // Cluster SSL  memberSslMode     Instance SSL
  //-----------   ---------------   ------------
  // VERIFY_CA    "AUTO"            enabled
  try {
    peer->execute("SET GLOBAL group_replication_ssl_mode=VERIFY_IDENTITY");
    auto member_ssl_mode =
        mysqlsh::dba::resolve_instance_ssl_mode_option(*instance, *peer);
    EXPECT_STREQ("VERIFY_IDENTITY", to_string(member_ssl_mode).c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at memberSslMode='VERIFY_IDENTITY', instance "
        "with SSL");
    ADD_FAILURE();
  }

  peer->execute("SET GLOBAL group_replication_ssl_mode=REQUIRED");
  disable_ssl_on_instance(_mysql_sandbox_ports[1], "unsecure");
  instance = create_session(_mysql_sandbox_ports[1], "unsecure");

  // Cluster SSL memberSslMode Instance SSL
  //----------- ------------- ------------
  // REQUIRED    AUTO          disabled
  EXPECT_THROW_MSG(
      mysqlsh::dba::resolve_instance_ssl_mode_option(*instance, *peer),
      std::runtime_error,
      "Instance '" + instance->descr() +
          "' does not support TLS and cannot join a cluster with "
          "TLS (encryption) enabled. Enable TLS support on the "
          "instance and try again, otherwise it can only be added "
          "to a cluster with TLS disabled.");

  testutil->destroy_sandbox(_mysql_sandbox_ports[0]);
  testutil->destroy_sandbox(_mysql_sandbox_ports[1]);
}

TEST_F(Admin_api_common_test, resolve_instance_ssl_cluster_with_ssl_disabled) {
  shcore::Dictionary_t sandbox_opts = shcore::make_dict();
  (*sandbox_opts)["report_host"] = shcore::Value(hostname());

  testutil->deploy_sandbox(_mysql_sandbox_ports[0], "root", sandbox_opts);
  testutil->deploy_sandbox(_mysql_sandbox_ports[1], "root", sandbox_opts);
  execute("shell.connect('root:root@localhost:" +
          std::to_string(_mysql_sandbox_ports[0]) + "')");

  testutil->expect_prompt(
      "Should the configuration be changed accordingly? [y/N]: ", "y");
#ifdef HAVE_V8
  execute(
      "var c = dba.createCluster('sample', {memberSslMode:'DISABLED', "
      "gtidSetIsComplete: true})");
#else
  execute(
      "c = dba.create_cluster('sample', {'memberSslMode':'DISABLED', "
      "gtidSetIsComplete: true})");
#endif
  execute("c.disconnect()");
  execute("session.close()");

  auto peer = create_session(_mysql_sandbox_ports[0]);
  auto instance = create_session(_mysql_sandbox_ports[1]);

  // Cluster SSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // DISABLED    AUTO          OFF
  try {
    auto member_ssl_mode =
        mysqlsh::dba::resolve_instance_ssl_mode_option(*instance, *peer);
    EXPECT_STREQ("DISABLED", to_string(member_ssl_mode).c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure using memberSslMode=AUTO");
    ADD_FAILURE();
  }

  instance->set_sysvar("require_secure_transport", true, Var_qualifier::GLOBAL);

  // Cluster SSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // DISABLED    ""            ON
  EXPECT_THROW_MSG(
      mysqlsh::dba::resolve_instance_ssl_mode_option(*instance, *peer),
      std::runtime_error,
      "The instance '" + instance->descr() +
          "' is configured to require a secure transport but the cluster has "
          "TLS disabled. To add the instance to the cluster, either turn OFF "
          "the require_secure_transport option on the instance or enable TLS "
          "on the cluster.");

  // Cluster SSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // DISABLED    AUTO          ON
  EXPECT_THROW_MSG(
      mysqlsh::dba::resolve_instance_ssl_mode_option(*instance, *peer),
      std::runtime_error,
      "The instance '" + instance->descr() +
          "' is configured to require a secure transport but the cluster has "
          "TLS disabled. To add the instance to the cluster, either turn OFF "
          "the require_secure_transport option on the instance or enable TLS "
          "on the cluster.");

  disable_ssl_on_instance(_mysql_sandbox_ports[1], "unsecure");
  instance = create_session(_mysql_sandbox_ports[1], "unsecure");

  testutil->destroy_sandbox(_mysql_sandbox_ports[0]);
  testutil->destroy_sandbox(_mysql_sandbox_ports[1]);
}

TEST_F(Admin_api_common_test, check_admin_account_access_restrictions) {
  using mysqlsh::dba::check_admin_account_access_restrictions;
  using mysqlshdk::db::Type;

  std::shared_ptr<Mock_session> mock_session = std::make_shared<Mock_session>();
  mysqlshdk::mysql::Instance instance{mock_session};

  // TEST: More than one account available for the user:
  // - Return true independently of the interactive mode.
  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"",
                     {"grantee"},
                     {Type::String},
                     {{"'admin'@'myhost'"}, {"'admin'@'otherhost'"}}}});
  EXPECT_TRUE(check_admin_account_access_restrictions(
      instance, "admin", "myhost", true,
      mysqlsh::dba::Cluster_type::GROUP_REPLICATION));

  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"",
                     {"grantee"},
                     {Type::String},
                     {{"'admin'@'myhost'"}, {"'admin'@'otherhost'"}}}});
  EXPECT_TRUE(check_admin_account_access_restrictions(
      instance, "admin", "myhost", false,
      mysqlsh::dba::Cluster_type::GROUP_REPLICATION));

  // TEST: Only one account not using wildcards (%) available for the user:
  // - Interactive 'true': return false;
  // - Interactive 'false': throw exception;
  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"", {"grantee"}, {Type::String}, {{"'admin'@'myhost'"}}}});
  EXPECT_FALSE(check_admin_account_access_restrictions(
      instance, "admin", "myhost", true,
      mysqlsh::dba::Cluster_type::GROUP_REPLICATION));

  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"", {"grantee"}, {Type::String}, {{"'admin'@'myhost'"}}}});
  EXPECT_THROW_LIKE(check_admin_account_access_restrictions(
                        instance, "admin", "myhost", false,
                        mysqlsh::dba::Cluster_type::GROUP_REPLICATION),
                    std::runtime_error,
                    "User 'admin' can only connect from 'myhost'.");

  // TEST: Only one account with wildcard (%) available which is the same
  // currently used (passed as parameter):
  // - Return true independently of the interactive mode.
  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"", {"grantee"}, {Type::String}, {{"'admin'@'%'"}}}});
  EXPECT_TRUE(check_admin_account_access_restrictions(
      instance, "admin", "%", true,
      mysqlsh::dba::Cluster_type::GROUP_REPLICATION));

  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"", {"grantee"}, {Type::String}, {{"'admin'@'%'"}}}});
  EXPECT_TRUE(check_admin_account_access_restrictions(
      instance, "admin", "%", false,
      mysqlsh::dba::Cluster_type::GROUP_REPLICATION));

  // TEST: Only one account with netmask which is the same
  // currently used (passed as parameter):
  // - Return true independently of the interactive mode.
  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"",
                     {"grantee"},
                     {Type::String},
                     {{"'admin'@'192.168.1.0/255.255.255.0'"}}}});
  EXPECT_TRUE(check_admin_account_access_restrictions(
      instance, "admin", "192.168.1.20", true,
      mysqlsh::dba::Cluster_type::GROUP_REPLICATION));

  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"",
                     {"grantee"},
                     {Type::String},
                     {{"'admin'@'192.168.1.0/255.255.255.0'"}}}});
  EXPECT_TRUE(check_admin_account_access_restrictions(
      instance, "admin", "192.168.1.20", false,
      mysqlsh::dba::Cluster_type::GROUP_REPLICATION));

  // TEST: Only one account with IPv6 which is the same
  // currently used (passed as parameter):
  // - Interactive 'true': return false;
  // - Interactive 'false': throw exception;
  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"",
                     {"grantee"},
                     {Type::String},
                     {{"'admin'@'2001:0db8:85a3:0000:0000:8a2e:0370'"}}}});
  EXPECT_FALSE(check_admin_account_access_restrictions(
      instance, "admin", "2001:0db8:85a3:0000:0000:8a2e:0370", true,
      mysqlsh::dba::Cluster_type::GROUP_REPLICATION));

  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"",
                     {"grantee"},
                     {Type::String},
                     {{"'admin'@'2001:0db8:85a3:0000:0000:8a2e:0370'"}}}});
  EXPECT_THROW_LIKE(check_admin_account_access_restrictions(
                        instance, "admin", "2001:0db8:85a3:0000:0000:8a2e:0370",
                        false, mysqlsh::dba::Cluster_type::GROUP_REPLICATION),
                    std::runtime_error,
                    "User 'admin' can only connect from "
                    "'2001:0db8:85a3:0000:0000:8a2e:0370'.");

  // TEST: Multiple accounts and one with wildcard (%) with the needed
  // privileges, which is not the one currently used (passed as parameter):
  // - Return true independently of the interactive mode.

  auto expect_all_privileges =
      [](const std::shared_ptr<Mock_session> &session) {
        testing::user_privileges::Setup_options options;

        options.user = "admin";
        options.host = "%";
        // Simulate version is always < 8.0.0 (5.7.0) to skip reading roles
        // data.
        options.version = Version(5, 7, 28);

        options.grants = {
            "GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, RELOAD, "
            "SHUTDOWN, PROCESS, FILE, REFERENCES, INDEX, ALTER, SHOW "
            "DATABASES, SUPER, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, "
            "REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, "
            "CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, "
            "CREATE TABLESPACE ON *.* TO 'admin'@'%' WITH GRANT OPTION",
        };

        testing::user_privileges::setup(options, session.get());
      };

  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"",
                     {"grantee"},
                     {Type::String},
                     {{"'admin'@'localhost'"}, {"'admin'@'%'"}}}});
  expect_all_privileges(mock_session);
  EXPECT_TRUE(check_admin_account_access_restrictions(
      instance, "admin", "localhost", true,
      mysqlsh::dba::Cluster_type::GROUP_REPLICATION));

  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"",
                     {"grantee"},
                     {Type::String},
                     {{"'admin'@'localhost'"}, {"'admin'@'%'"}}}});
  expect_all_privileges(mock_session);
  EXPECT_TRUE(check_admin_account_access_restrictions(
      instance, "admin", "localhost", false,
      mysqlsh::dba::Cluster_type::GROUP_REPLICATION));
}

class Admin_api_common_cluster_functions : public Admin_api_common_test {
 public:
  static void SetUpTestCase() {
    SetUpSampleCluster("Admin_api_common_cluster_functions/SetUpTestCase");
  }

  static void TearDownTestCase() {
    TearDownSampleCluster(
        "Admin_api_common_cluster_functions/TearDownTestCase");
  }
};

// If the information on the Metadata and the GR group
// P_S info is the same get_newly_discovered_instances()
// result return an empty list
TEST_F(Admin_api_common_cluster_functions, get_newly_discovered_instances) {
  auto md_instance = create_session(_mysql_sandbox_ports[0]);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_instance));

  try {
    auto newly_discovered_instances_list(get_newly_discovered_instances(
        *md_instance, metadata, _cluster->impl()->get_id()));

    EXPECT_TRUE(newly_discovered_instances_list.empty());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at get_instances_md");
    ADD_FAILURE();
  }
}

// If the information on the Metadata and the GR group
// P_S info is the same get_unavailable_instances()
// should return an empty list
TEST_F(Admin_api_common_cluster_functions, get_unavailable_instances) {
  auto md_instance = create_session(_mysql_sandbox_ports[0]);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_instance));

  try {
    auto unavailable_instances_list(get_unavailable_instances(
        *md_instance, metadata, _cluster->impl()->get_id()));

    EXPECT_TRUE(unavailable_instances_list.empty());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at get_unavailable_instances_001");
    ADD_FAILURE();
  }
}

TEST_F(Admin_api_common_cluster_functions, validate_instance_rejoinable_01) {
  // There are missing instances and the instance we are checking belongs to
  // the metadata list but does not belong to the GR list.

  auto md_instance = create_session(_mysql_sandbox_ports[0]);
  auto instance = create_session(_mysql_sandbox_ports[2]);

  auto cluster_id = _cluster->impl()->get_id();

  // Insert a fake record for the third instance on the metadata
  std::string query =
      "insert into mysql_innodb_cluster_metadata.instances "
      "(instance_id, cluster_id, mysql_server_uuid, instance_name, "
      "addresses, attributes, description)"
      "values (0, '" +
      cluster_id + "', '" + uuid_3 +
      "', 'localhost:<port>', "
      "'{\"mysqlX\": \"localhost:<port>0\", "
      "\"grLocal\": \"localhost:1<port>\", "
      "\"mysqlClassic\": \"localhost:<port>\"}', "
      "NULL, NULL)";

  query = shcore::str_replace(query, "<port>",
                              std::to_string(_mysql_sandbox_ports[2]));

  md_instance->query(query);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_instance));

  try {
    auto is_rejoinable(validate_instance_rejoinable(
        *instance, metadata, _cluster->impl()->get_id()));

    EXPECT_EQ(is_rejoinable, mysqlsh::dba::Instance_rejoinability::REJOINABLE);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at validate_instance_rejoinable_01");
    ADD_FAILURE();
  }

  md_instance->query(
      "delete from mysql_innodb_cluster_metadata.instances "
      " where mysql_server_uuid = '" +
      uuid_3 + "'");
}

TEST_F(Admin_api_common_cluster_functions, validate_instance_rejoinable_02) {
  // There are missing instances and the instance we are checking belongs
  // to neither the metadata nor GR lists.

  auto md_instance = create_session(_mysql_sandbox_ports[0]);
  auto instance = create_session(_mysql_sandbox_ports[2]);

  auto cluster_id = _cluster->impl()->get_id();

  // Insert a fake record for the third instance on the metadata
  std::string query =
      "insert into mysql_innodb_cluster_metadata.instances "
      "(instance_id, cluster_id, mysql_server_uuid, instance_name, "
      "addresses, attributes, description)"
      "values (0, '" +
      cluster_id +
      "', '11111111-2222-3333-4444-555555555555', 'localhost:<port>', "
      "'{\"mysqlX\": \"localhost:<port>0\", "
      "\"grLocal\": \"localhost:1<port>\", "
      "\"mysqlClassic\": \"localhost:<port>\"}', "
      "NULL, NULL)";

  query = shcore::str_replace(query, "<port>",
                              std::to_string(_mysql_sandbox_ports[2]));

  md_instance->query(query);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_instance));

  try {
    auto is_rejoinable(validate_instance_rejoinable(
        *instance, metadata, _cluster->impl()->get_id()));

    EXPECT_EQ(is_rejoinable, mysqlsh::dba::Instance_rejoinability::NOT_MEMBER);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at validate_instance_rejoinable_02");
    ADD_FAILURE();
  }

  md_instance->query(
      "delete from mysql_innodb_cluster_metadata.instances "
      " where mysql_server_uuid = '11111111-2222-3333-4444-"
      "555555555555'");
}

TEST_F(Admin_api_common_cluster_functions, validate_instance_rejoinable_03) {
  // There are no missing instances and the instance we are checking belongs
  // to both the metadata and GR lists.
  auto md_instance = create_session(_mysql_sandbox_ports[0]);
  auto instance = create_session(_mysql_sandbox_ports[1]);

  std::shared_ptr<mysqlsh::dba::MetadataStorage> metadata;
  metadata.reset(new mysqlsh::dba::MetadataStorage(md_instance));

  try {
    auto is_rejoinable(validate_instance_rejoinable(
        *instance, metadata, _cluster->impl()->get_id()));

    EXPECT_EQ(is_rejoinable, mysqlsh::dba::Instance_rejoinability::ONLINE);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at validate_instance_rejoinable_03");
    ADD_FAILURE();
  }
}

TEST_F(Admin_api_common_test, super_read_only_server_on_flag_true) {
  enable_replay();
  testutil->deploy_sandbox(_mysql_sandbox_ports[0], "root");
  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(
      testutil->sandbox_connection_options(_mysql_sandbox_ports[0], "root"));

  // super_read_only is ON, no active sessions
  session->query("set global super_read_only = 1");

  try {
    auto read_only = mysqlsh::dba::validate_super_read_only(
        mysqlshdk::mysql::Instance(session));
    EXPECT_TRUE(read_only);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at super_read_only_server_on_flag_true");
    ADD_FAILURE();
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_ports[0]);
}

TEST_F(Admin_api_common_test, super_read_only_server_off_flag_true) {
  enable_replay();
  testutil->deploy_sandbox(_mysql_sandbox_ports[0], "root");
  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(
      testutil->sandbox_connection_options(_mysql_sandbox_ports[0], "root"));

  // super_read_only is OFF, no active sessions
  session->query("set global super_read_only = 0");

  try {
    auto read_only = mysqlsh::dba::validate_super_read_only(
        mysqlshdk::mysql::Instance(session));
    EXPECT_FALSE(read_only);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at super_read_only_server_on_flag_true");
    ADD_FAILURE();
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_ports[0]);
}

TEST_F(Admin_api_common_test, super_read_only_server_off_flag_false) {
  enable_replay();
  testutil->deploy_sandbox(_mysql_sandbox_ports[0], "root");
  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(
      testutil->sandbox_connection_options(_mysql_sandbox_ports[0], "root"));

  // super_read_only is OFF, no active sessions
  session->query("set global super_read_only = 0");

  try {
    auto read_only = mysqlsh::dba::validate_super_read_only(
        mysqlshdk::mysql::Instance(session));
    EXPECT_FALSE(read_only);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at super_read_only_server_on_flag_true");
    ADD_FAILURE();
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_ports[0]);
}

TEST(mod_dba_common, validate_ipallowlist_option) {
  using mysqlsh::dba::Group_replication_options;

  Group_replication_options options;
  // NOTE: hostnames_supported = true if version >= 8.0.4, otherwise false.
  auto ver_8014 = mysqlshdk::utils::Version(8, 0, 14);
  auto ver_804 = mysqlshdk::utils::Version(8, 0, 4);
  auto ver_800 = mysqlshdk::utils::Version(8, 0, 0);
  int canonical_port = 3306;

  // Error if the ipAllowlist is empty.
  options.ip_allowlist = "";
  try {
    options.check_option_values(Version(8, 0, 4), canonical_port);
    SCOPED_TRACE("Unexpected success calling validate_ip_allowlist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ("Invalid value for ipAllowlist: string value cannot be empty.",
                 e.what());
  }

  // Error if the ipAllowlist string is empty (only whitespace).
  options.ip_allowlist = " ";
  try {
    options.check_option_values(Version(8, 0, 4), canonical_port);
    SCOPED_TRACE("Unexpected success calling validate_ip_allowlist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ("Invalid value for ipAllowlist: string value cannot be empty.",
                 e.what());
  }

  // Error if CIDR is used but has an invalid value (not in range [1,32])
  options.ip_allowlist = "192.168.1.1/0";
  try {
    options.check_option_values(Version(8, 0, 4), canonical_port);
    SCOPED_TRACE("Unexpected success calling validate_ip_allowlist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipAllowlist '192.168.1.1/0': subnet value in CIDR "
        "notation is not valid.",
        e.what());
  }

  // Error if CIDR is used but has an invalid value (not in range [1,32])
  options.ip_allowlist = "192.168.1.1/33";
  try {
    options.check_option_values(Version(8, 0, 4), canonical_port);
    SCOPED_TRACE("Unexpected success calling validate_ip_allowlist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipAllowlist '192.168.1.1/33': subnet value in CIDR "
        "notation is not supported (version >= 8.0.14 required for IPv6 "
        "support).",
        e.what());
  }

  // Error if CIDR is used but has an invalid value (not in range [1,32])
  options.ip_allowlist = "1/33";
  try {
    options.check_option_values(Version(8, 0, 4), canonical_port);
    SCOPED_TRACE("Unexpected success calling validate_ip_allowlist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipAllowlist '1/33': subnet value in CIDR "
        "notation is not supported (version >= 8.0.14 required for IPv6 "
        "support).",
        e.what());
  }

  // Error if CIDR is used but has an invalid value (not in range [1,32])
  // And a list of values is used
  options.ip_allowlist = "192.168.1.1/0,192.168.1.1/33";
  try {
    options.check_option_values(Version(8, 0, 4), canonical_port);
    SCOPED_TRACE("Unexpected success calling validate_ip_allowlist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipAllowlist '192.168.1.1/0': subnet value in CIDR "
        "notation is not valid.",
        e.what());
  }

  // Error if ipAllowlist is an IPv6 address and not supported by server
  options.ip_allowlist = "2001:0db8:85a3:0000:0000:8a2e:0370:7334";
  try {
    options.check_option_values(Version(8, 0, 4), canonical_port);
    SCOPED_TRACE("Unexpected success calling validate_ip_allowlist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipAllowlist "
        "'2001:0db8:85a3:0000:0000:8a2e:0370:7334': IPv6 not supported "
        "(version >= 8.0.14 required for IPv6 support).",
        e.what());
  }

  // Error if ipAllowlist is not a valid IPv4 address (not supported, < 8.0.4)
  options.ip_allowlist = "256.255.255.255";
  try {
    options.check_option_values(Version(8, 0, 3), canonical_port);
    SCOPED_TRACE("Unexpected success calling validate_ip_allowlist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipAllowlist '256.255.255.255': hostnames are not "
        "supported (version >= 8.0.4 required for hostname support).",
        e.what());
  }

  // Error if ipAllowlist is not a valid IPv4 address
  options.ip_allowlist = "256.255.255.255/16";
  try {
    options.check_option_values(Version(8, 0, 4), canonical_port);
    SCOPED_TRACE("Unexpected success calling validate_ip_allowlist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipAllowlist '256.255.255.255/16': CIDR notation can "
        "only be used with IPv4 or IPv6 addresses.",
        e.what());
  }

  // Error if hostname is used and server version < 8.0.4
  options.ip_allowlist = "localhost";
  try {
    options.check_option_values(Version(8, 0, 3), canonical_port);
    SCOPED_TRACE("Unexpected success calling validate_ip_allowlist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipAllowlist 'localhost': hostnames are not "
        "supported (version >= 8.0.4 required for hostname support).",
        e.what());
  }

  // Error if hostname with cidr
  options.ip_allowlist = "localhost/8";
  try {
    options.check_option_values(Version(8, 0, 4), canonical_port);
    SCOPED_TRACE("Unexpected success calling validate_ip_allowlist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipAllowlist 'localhost/8': CIDR notation can only "
        "be used with IPv4 or IPv6 addresses.",
        e.what());
  }

  // Error if hostname with cidr
  options.ip_allowlist = "bogus/8";
  try {
    options.check_option_values(Version(8, 0, 4), canonical_port);
    SCOPED_TRACE("Unexpected success calling validate_ip_allowlist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipAllowlist 'bogus/8': CIDR notation can only be "
        "used with IPv4 or IPv6 addresses.",
        e.what());
  }

  // No error if ipAllowlist is an IPv6 address and server does support it
  options.ip_allowlist = "2001:0db8:85a3:0000:0000:8a2e:0370:7334";
  {
    options.check_option_values(Version(8, 0, 14), canonical_port);
    SCOPED_TRACE(
        "No error if ipAllowlist is an IPv6 address and server does support "
        "it");
    EXPECT_NO_THROW(
        options.check_option_values(Version(8, 0, 14), canonical_port));
  }

  options.ip_allowlist = "256.255.255.255";
  {
    // Error if ipAllowlist is not a valid IPv4 address and version < 8.0.4
    // since it is not a valid IPv4 it assumes it must be an hostname and
    // hostnames are not supported below 8.0.4
    SCOPED_TRACE(
        "Error if ipAllowlist is not a valid IPv4 address and version < 8.0.4");
    EXPECT_THROW_LIKE(
        options.check_option_values(Version(8, 0, 0), canonical_port),
        shcore::Exception,
        "Invalid value for ipAllowlist '256.255.255.255': hostnames are not "
        "supported (version >= 8.0.4 required for hostname support).");
  }

  {
    // Error if hostname is used and server version < 8.0.4
    options.ip_allowlist = "localhost";
    SCOPED_TRACE("Error if hostname is used and server version < 8.0.4");
    EXPECT_THROW_LIKE(
        options.check_option_values(Version(8, 0, 0), canonical_port),
        shcore::Exception,
        "Invalid value for ipAllowlist 'localhost': hostnames are not "
        "supported (version >= 8.0.4 required for hostname support).");
  }

  {
    // No error if hostname is used and server version >= 8.0.4 because
    // we don't do hostname resolution
    SCOPED_TRACE("No error if hostname is used and server version >= 8.0.4");
    options.ip_allowlist = "fake_hostnanme";
    EXPECT_NO_THROW(
        options.check_option_values(Version(8, 0, 4), canonical_port));
  }

  // No error if the ipAllowlist is a valid IPv4 address
  options.ip_allowlist = "192.168.1.1";
  EXPECT_NO_THROW(
      options.check_option_values(Version(8, 0, 4), canonical_port));

  // No error if the ipAllowlist is a valid IPv4 address with a valid CIDR value
  options.ip_allowlist = "192.168.1.1/15";
  EXPECT_NO_THROW(
      options.check_option_values(Version(8, 0, 4), canonical_port));

  // No error if the ipAllowlist consist of several valid IPv4 addresses with a
  // valid CIDR value
  // NOTE: if the server version is > 8.0.4, hostnames are allowed too so we
  // must test it
  options.ip_allowlist = "192.168.1.1/15,192.169.1.1/1, localhost";
  EXPECT_NO_THROW(
      options.check_option_values(Version(8, 0, 4), canonical_port));
  options.ip_allowlist = "192.168.1.1/15,192.169.1.1/1";
  EXPECT_NO_THROW(
      options.check_option_values(Version(8, 0, 3), canonical_port));
  options.ip_allowlist =
      "2001:0db8:85a3:0000:0000:8a2e:0370:7334/16,192.168.1.1/15,192.169.1.1/1";
  EXPECT_NO_THROW(
      options.check_option_values(Version(8, 0, 14), canonical_port));
}

TEST(mod_dba_common, validate_exit_state_action_supported) {
  using mysqlsh::dba::Group_replication_options;

  Group_replication_options options;
  options.exit_state_action = "1";
  int canonical_port = 3306;

  // Error only if the target server version is >= 5.7.24 if 5.0, or >= 8.0.12
  // if 8.0.

  EXPECT_THROW_LIKE(
      options.check_option_values(Version(5, 7, 23), canonical_port),
      shcore::Exception,
      "Option 'exitStateAction' not supported on target server "
      "version:");

  EXPECT_NO_THROW(
      options.check_option_values(Version(5, 7, 24), canonical_port));

  EXPECT_THROW_LIKE(
      options.check_option_values(Version(8, 0, 11), canonical_port),
      shcore::Exception,
      "Option 'exitStateAction' not supported on target server "
      "version:");

  EXPECT_NO_THROW(
      options.check_option_values(Version(8, 0, 12), canonical_port));
}

TEST(mod_dba_common, validate_member_weight_supported) {
  using mysqlsh::dba::Group_replication_options;

  Group_replication_options options;
  options.member_weight = 1;
  int canonical_port = 3306;

  // Error only if the target server version is < 5.7.20 if 5.0, or < 8.0.11
  // if 8.0.

  EXPECT_THROW_LIKE(
      options.check_option_values(Version(5, 7, 19), canonical_port),
      shcore::Exception,
      "Option 'memberWeight' not supported on target server "
      "version:");

  EXPECT_NO_THROW(
      options.check_option_values(Version(5, 7, 20), canonical_port));

  EXPECT_THROW_LIKE(
      options.check_option_values(Version(8, 0, 10), canonical_port),
      shcore::Exception,
      "Option 'memberWeight' not supported on target server "
      "version:");

  EXPECT_NO_THROW(
      options.check_option_values(Version(8, 0, 11), canonical_port));
}

TEST(mod_dba_common, validate_consistency_supported) {
  using mysqlsh::dba::Group_replication_options;

  Group_replication_options options;
  Version version(8, 0, 14);
  int canonical_port = 3306;

  std::optional<std::string> empty_fail_cons{"  "};
  std::optional<std::string> valid_fail_cons{"1"};

  options.consistency = std::nullopt;
  // if a null value was provided, it is as if the option was not provided,
  // so no error should be thrown
  options.check_option_values(version, canonical_port);

  options.consistency = empty_fail_cons;
  // if an empty value was provided, an error should be thrown independently
  // of the server version
  EXPECT_THROW_LIKE(
      options.check_option_values(version, canonical_port), shcore::Exception,
      "Invalid value for consistency, string value cannot be empty.");

  // if a valid value (non empty) was provided, an error should only be thrown
  // in case the option is not supported by the server version.
  options.consistency = valid_fail_cons;

  EXPECT_THROW_LIKE(
      options.check_option_values(Version(8, 0, 13), canonical_port),
      std::runtime_error,
      "Option 'consistency' not supported on target server "
      "version:");

  EXPECT_NO_THROW(
      options.check_option_values(Version(8, 0, 14), canonical_port));
}

TEST(mod_dba_common, validate_auto_rejoin_tries_supported) {
  using mysqlsh::dba::Group_replication_options;

  Group_replication_options options;
  options.auto_rejoin_tries = 1;
  int canonical_port = 3306;

  // Error only if the target server version is < 8.0.16

  EXPECT_THROW_LIKE(
      options.check_option_values(Version(5, 7, 19), canonical_port),
      shcore::Exception,
      "Option 'autoRejoinTries' not supported on target server "
      "version:");

  EXPECT_THROW_LIKE(
      options.check_option_values(Version(8, 0, 15), canonical_port),
      shcore::Exception,
      "Option 'autoRejoinTries' not supported on target server "
      "version:");

  EXPECT_NO_THROW(
      options.check_option_values(Version(8, 0, 16), canonical_port));
}

TEST(mod_dba_common, validate_expel_timeout_supported) {
  using mysqlsh::dba::Group_replication_options;

  Group_replication_options options;
  Version version(8, 0, 13);
  int canonical_port = 3306;

  std::optional<std::int64_t> valid_timeout{3600};
  std::optional<std::int64_t> maybe_valid_timeout{3601};

  // if a null value was provided, it is as if the option was not provided,
  // so no error should be thrown
  options.expel_timeout = std::nullopt;
  options.check_option_values(version, canonical_port);

  // if a value non in the allowed range value was provided, an error should be
  // thrown independently of the server version
  // this value is "valid" only for versions > 8.0.13
  options.expel_timeout = maybe_valid_timeout;
  EXPECT_THROW_LIKE(
      options.check_option_values(Version(8, 0, 13), canonical_port),
      std::runtime_error,
      "Invalid value for 'expelTimeout': integer value must be "
      "in the range [0, 3600]");
  EXPECT_NO_THROW(
      options.check_option_values(Version(8, 0, 14), canonical_port));
  EXPECT_NO_THROW(
      options.check_option_values(Version(8, 0, 15), canonical_port));

  // if a valid value was provided, an error should only be thrown
  // in case the option is not supported by the server version.
  options.expel_timeout = valid_timeout;
  EXPECT_THROW_LIKE(
      options.check_option_values(Version(8, 0, 12), canonical_port),
      std::runtime_error,
      "Option 'expelTimeout' not supported on target server "
      "version:");

  options.expel_timeout = valid_timeout;
  EXPECT_NO_THROW(
      options.check_option_values(Version(8, 0, 13), canonical_port));
}

TEST(mod_dba_common, is_option_supported) {
  // if a non supported version is used, then we must throw an exception,
  // else just save the result for further testing
  EXPECT_THROW_LIKE(
      mysqlsh::dba::is_option_supported(
          Version(9, 0, 0), mysqlsh::dba::kExitStateAction,
          mysqlsh::dba::k_global_cluster_supported_options),
      std::runtime_error,
      "Unexpected version found for option support check: '9.0.0'.");

  // testing the result of exit-state action case since it has requirements for
  // both 5.7 and the 8.0 MySQL versions.
  EXPECT_FALSE(mysqlsh::dba::is_option_supported(
      Version(8, 0, 11), mysqlsh::dba::kExitStateAction,
      mysqlsh::dba::k_global_cluster_supported_options));
  EXPECT_TRUE(mysqlsh::dba::is_option_supported(
      Version(8, 0, 12), mysqlsh::dba::kExitStateAction,
      mysqlsh::dba::k_global_cluster_supported_options));
  EXPECT_FALSE(mysqlsh::dba::is_option_supported(
      Version(5, 7, 23), mysqlsh::dba::kExitStateAction,
      mysqlsh::dba::k_global_cluster_supported_options));
  EXPECT_TRUE(mysqlsh::dba::is_option_supported(
      Version(5, 7, 24), mysqlsh::dba::kExitStateAction,
      mysqlsh::dba::k_global_cluster_supported_options));

  // testing the result of autoRejoinRetries which is only supported on 8.0.16
  // onwards (BUG#29246657)
  EXPECT_FALSE(mysqlsh::dba::is_option_supported(
      Version(8, 0, 11), mysqlsh::dba::kAutoRejoinTries,
      mysqlsh::dba::k_global_cluster_supported_options));
  EXPECT_TRUE(mysqlsh::dba::is_option_supported(
      Version(8, 0, 16), mysqlsh::dba::kAutoRejoinTries,
      mysqlsh::dba::k_global_cluster_supported_options));
  EXPECT_FALSE(mysqlsh::dba::is_option_supported(
      Version(5, 7, 23), mysqlsh::dba::kAutoRejoinTries,
      mysqlsh::dba::k_global_cluster_supported_options));
}

TEST(mod_dba_common, validate_local_address_option) {
  using mysqlsh::dba::Group_replication_options;

  Group_replication_options options;
  Version version(8, 0, 14);
  int canonical_port = 3306;

  // Error if the localAddress is empty.
  options.local_address = "";
  EXPECT_THROW(options.check_option_values(version, canonical_port),
               shcore::Exception);

  // Error if the localAddress string is empty (only whitespace).
  options.local_address = "  ";
  EXPECT_THROW(options.check_option_values(version, canonical_port),
               shcore::Exception);

  // Error if the localAddress has ':' and no host nor port part is specified.
  options.local_address = " : ";
  EXPECT_THROW(options.check_option_values(version, canonical_port),
               shcore::Exception);

  // No error if the localAddress is a non-empty string.
  options.local_address = "myhost:1234";
  EXPECT_NO_THROW(options.check_option_values(version, canonical_port));
  options.local_address = "myhost:";
  EXPECT_NO_THROW(options.check_option_values(version, canonical_port));
  options.local_address = ":1234";
  EXPECT_NO_THROW(options.check_option_values(version, canonical_port));
  options.local_address = "myhost";
  EXPECT_NO_THROW(options.check_option_values(version, canonical_port));
  options.local_address = "1234";
  EXPECT_NO_THROW(options.check_option_values(version, canonical_port));
}

TEST(mod_dba_common, validate_local_address_option_mysql_comm_stack) {
  using mysqlsh::dba::Group_replication_options;

  Group_replication_options options;
  Version version(8, 0, 30);
  int canonical_port = 3306;
  options.communication_stack = mysqlsh::dba::kCommunicationStackMySQL;

  // Error if the localAddress is empty.
  options.local_address = "";
  EXPECT_THROW(options.check_option_values(version, canonical_port),
               shcore::Exception);

  // Error if the localAddress string is empty (only whitespace).
  options.local_address = "  ";
  EXPECT_THROW(options.check_option_values(version, canonical_port),
               shcore::Exception);

  // Error if the localAddress has ':' and no host nor port part is specified.
  options.local_address = " : ";
  EXPECT_THROW(options.check_option_values(version, canonical_port),
               shcore::Exception);

  // No Error if the port is not specified
  options.local_address = "myhost:";
  EXPECT_NO_THROW(options.check_option_values(version, canonical_port));
  options.local_address = "myhost";
  EXPECT_NO_THROW(options.check_option_values(version, canonical_port));

  // Error if the port is not the canonical port
  options.local_address = "myhost:3300";
  EXPECT_THROW(options.check_option_values(version, canonical_port),
               shcore::Exception);
  options.local_address = ":3300";
  EXPECT_THROW(options.check_option_values(version, canonical_port),
               shcore::Exception);
  options.local_address = "3300";
  EXPECT_THROW(options.check_option_values(version, canonical_port),
               shcore::Exception);

  // No error if the localAddress is using the canonical port
  options.local_address = "myhost:3306";
  EXPECT_NO_THROW(options.check_option_values(version, canonical_port));
  options.local_address = ":3306";
  EXPECT_NO_THROW(options.check_option_values(version, canonical_port));
  options.local_address = "3306";
  EXPECT_NO_THROW(options.check_option_values(version, canonical_port));
}

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
      // Valid identifier, begins with valid characters (alpha)
      t = "Valid1"; mysqlsh::dba::validate_cluster_name(
          t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION));
  EXPECT_NO_THROW(
      // Valid identifier, begins with valid characters (_)
      t = "_Valid_"; mysqlsh::dba::validate_cluster_name(
          t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION));
  EXPECT_NO_THROW(
      // Valid identifier, contains valid characters (_, -, .)
      t = "Valid_3.still-valid"; mysqlsh::dba::validate_cluster_name(
          t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION));

  EXPECT_ANY_THROW(t = "";
                   // Invalid empty identifier
                   mysqlsh::dba::validate_cluster_name(
                       t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION););
  EXPECT_ANY_THROW(
      // Invalid too long identifier (over 63 characters)
      t = "o-ver63ch.ars_12345678901234567890123456789013456789012345678901";
      mysqlsh::dba::validate_cluster_name(
          t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION););
  EXPECT_ANY_THROW(
      // Invalid identifier, begins with invalid character
      t = "#not_allowed"; mysqlsh::dba::validate_cluster_name(
          t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION););
  EXPECT_ANY_THROW(
      // Invalid identifier, contains invalid character
      t = "not_allowed?"; mysqlsh::dba::validate_cluster_name(
          t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION););
  EXPECT_ANY_THROW(
      // Invalid identifier, begins with invalid characters (numeric)
      t = "2_not_Valid"; mysqlsh::dba::validate_cluster_name(
          t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION));
  EXPECT_ANY_THROW(
      // Invalid identifier, contains invalid character
      t = "(*)%?"; mysqlsh::dba::validate_cluster_name(
          t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION););
}

TEST_F(Admin_api_common_test, resolve_gr_local_address) {
  std::optional<std::string> local_address;
  std::string raw_report_host = "127.0.0.1";
  std::optional<std::string> communication_stack;
  int port;

  // Tests for empty localAddress

  // Valid port, local_address null
  {
    local_address = std::nullopt;
    port = 3306;
    EXPECT_NO_THROW(mysqlsh::dba::resolve_gr_local_address(
        local_address, communication_stack, raw_report_host, port, true));
  }

  // Valid port, local_address empty
  {
    local_address = std::string("");
    port = 3306;
    EXPECT_NO_THROW(mysqlsh::dba::resolve_gr_local_address(
        local_address, communication_stack, raw_report_host, port, true));
  }

  // Invalid port, local_address null
  {
    local_address = std::nullopt;
    port = 13040;

    EXPECT_THROW_LIKE(
        mysqlsh::dba::resolve_gr_local_address(
            local_address, communication_stack, raw_report_host, port, true),
        shcore::Exception,
        "Automatically generated port for localAddress falls out of valid "
        "range. The port must be an integer between 1 and 65535. Please use "
        "the localAddress option to manually set a valid value.");
  }

  // Invalid port, local_address empty
  {
    local_address = std::string("");
    port = 13040;

    EXPECT_THROW_LIKE(
        mysqlsh::dba::resolve_gr_local_address(
            local_address, communication_stack, raw_report_host, port, true),
        shcore::Exception,
        "Automatically generated port for localAddress falls out of valid "
        "range. The port must be an integer between 1 and 65535. Please use "
        "the localAddress option to manually set a valid value.");
  }

  // Busy port
  {
    testutil->deploy_sandbox(_mysql_sandbox_ports[0] * 10 + 1, "root");

    local_address = std::string("");
    port = _mysql_sandbox_ports[0];
    try {
      mysqlsh::dba::resolve_gr_local_address(local_address, communication_stack,
                                             raw_report_host, port, true);
      testutil->destroy_sandbox(_mysql_sandbox_ports[0] * 10 + 1);
      SCOPED_TRACE("Unexpected success calling resolve_gr_local_address");
      ADD_FAILURE();
    } catch (const shcore::Exception &e) {
      std::string expected =
          "The port '" + std::to_string(port * 10 + 1) +
          "' for localAddress option is already in use. Specify an "
          "available port to be used with localAddress option or free port "
          "'" +
          std::to_string(port * 10 + 1) + "'.";

      EXPECT_STREQ(expected.c_str(), e.what());
    }
  }

  // Tests for non-empty localAddress

  // Valid port, non-empty local_address
  {
    local_address = std::string("127.0.0.1");
    port = 3306;
    EXPECT_NO_THROW(mysqlsh::dba::resolve_gr_local_address(
        local_address, communication_stack, raw_report_host, port, true));
  }

  // Invalid port, complete local_address
  {
    local_address = std::string("127.0.0.1:130400");

    EXPECT_THROW_LIKE(
        mysqlsh::dba::resolve_gr_local_address(
            local_address, communication_stack, raw_report_host, port, true),
        shcore::Exception,
        "Invalid port '130400' for localAddress option. The port must be an "
        "integer between 1 and 65535.");
  }

  // Invalid port (string), complete local_address
  {
    local_address = std::string("127.0.0.1:a");

    EXPECT_THROW_LIKE(
        mysqlsh::dba::resolve_gr_local_address(
            local_address, communication_stack, raw_report_host, port, true),
        shcore::Exception,
        "Invalid port 'a' for localAddress option. The port must be an "
        "integer between 1 and 65535.");
  }

  // Invalid port, local_address without port part
  {
    local_address = std::string("127.0.0.2");
    port = 13040;

    EXPECT_THROW_LIKE(
        mysqlsh::dba::resolve_gr_local_address(
            local_address, communication_stack, raw_report_host, port, true),
        shcore::Exception,
        "Automatically generated port for localAddress falls out of valid "
        "range. The port must be an integer between 1 and 65535. Please use "
        "the localAddress option to manually set a valid value.");
  }

  // Invalid port, local_address without port part, using ":"
  {
    local_address = std::string("127.0.0.2:");
    port = 13040;

    EXPECT_THROW_LIKE(
        mysqlsh::dba::resolve_gr_local_address(
            local_address, communication_stack, raw_report_host, port, true),
        shcore::Exception,
        "Automatically generated port for localAddress falls out of valid "
        "range. The port must be an integer between 1 and 65535. Please use "
        "the localAddress option to manually set a valid value.");
  }

  // Valid port, local_address without separator ':', assumed to be the port
  {
    int unused_port = _mysql_sandbox_ports[1];
    local_address = std::to_string(unused_port);

    EXPECT_NO_THROW(mysqlsh::dba::resolve_gr_local_address(
        local_address, communication_stack, raw_report_host, port, true));
  }

  // Invalid port, local_address without separator ':', assumed to be the port
  {
    local_address = std::string("130401");

    EXPECT_THROW_LIKE(
        mysqlsh::dba::resolve_gr_local_address(
            local_address, communication_stack, raw_report_host, port, true),
        shcore::Exception,
        "Invalid port '130401' for localAddress option. The port must be an "
        "integer between 1 and 65535.");
  }

  // Busy port
  {
    int used_port = _mysql_sandbox_ports[0] * 10 + 1;
    local_address = std::string("127.0.0.1:") + std::to_string(used_port);

    try {
      mysqlsh::dba::resolve_gr_local_address(local_address, communication_stack,
                                             raw_report_host, port, true);
      testutil->destroy_sandbox(_mysql_sandbox_ports[0] * 10 + 1);
      SCOPED_TRACE("Unexpected success calling resolve_gr_local_address");
      ADD_FAILURE();
    } catch (const shcore::Exception &e) {
      std::string expected =
          "The port '" + std::to_string(used_port) +
          "' for localAddress option is already in use. Specify an "
          "available port to be used with localAddress option or free port "
          "'" +
          std::to_string(used_port) + "'.";

      EXPECT_STREQ(expected.c_str(), e.what());
    }
  }

  testutil->destroy_sandbox(_mysql_sandbox_ports[0] * 10 + 1);
}

TEST_F(Admin_api_common_test, set_recovery_progress) {
  mysqlsh::dba::cluster::Add_instance_options options;

  options.set_recovery_progress(0);
  EXPECT_EQ(options.get_recovery_progress(),
            mysqlsh::dba::Recovery_progress_style::NOINFO);

  options.set_recovery_progress(1);
  EXPECT_EQ(options.get_recovery_progress(),
            mysqlsh::dba::Recovery_progress_style::TEXTUAL);

  options.set_recovery_progress(2);
  EXPECT_EQ(options.get_recovery_progress(),
            mysqlsh::dba::Recovery_progress_style::PROGRESSBAR);
}

TEST_F(Admin_api_common_test, ip_allowlist) {
  using V = const mysqlshdk::utils::Version;
  {
    std::string_view ip_allowlist{};
    EXPECT_THROW_LIKE(
        mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.17"}, ip_allowlist),
        shcore::Exception,
        "Invalid value for 'ipAllowlist': string value cannot be empty.");

    // Verify that the option name is presented as ipAllowlist when used
    EXPECT_THROW_LIKE(
        mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.22"}, ip_allowlist),
        shcore::Exception,
        "Invalid value for 'ipAllowlist': string value cannot be empty.");
  }
  {
    std::string_view ip_allowlist{
        "192.168.0.0/24,::ffff:10.2.0.3/128,::10.3.0.3/128,::ffff:10.2.0.3/16,"
        "::10.3.0.3/16,10.100.23.1/128"};
    EXPECT_THROW_LIKE(
        mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.14"}, ip_allowlist),
        shcore::Exception,
        "Invalid value for ipAllowlist '10.100.23.1/128': subnet value in CIDR "
        "notation is not supported for IPv4 addresses.");
  }
  {
    std::string_view ip_allowlist{
        "192.168.0.1/8,127.0.0.1, 10.123.4.11, ::1,mysql.cluster.example.com"};
    mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.17"}, ip_allowlist);
    mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.14"}, ip_allowlist);
    EXPECT_THROW_LIKE(
        mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.13"}, ip_allowlist),
        shcore::Exception,
        "Invalid value for ipAllowlist '::1': IPv6 not supported");
    EXPECT_THROW_LIKE(
        mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.10"}, ip_allowlist),
        shcore::Exception,
        "Invalid value for ipAllowlist '::1': IPv6 not supported");
    EXPECT_THROW_LIKE(
        mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.4"}, ip_allowlist),
        shcore::Exception,
        "Invalid value for ipAllowlist '::1': IPv6 not supported");
    EXPECT_THROW_LIKE(
        mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.3"}, ip_allowlist),
        shcore::Exception,
        "Invalid value for ipAllowlist '::1': IPv6 not supported");
    EXPECT_THROW_LIKE(
        mysqlsh::dba::validate_ip_allowlist_option(V{"5.7.27"}, ip_allowlist),
        shcore::Exception,
        "Invalid value for ipAllowlist '::1': IPv6 not supported");
  }
  {
    std::string_view ip_allowlist{
        "192.168.0.1/8,127.0.0.1,10.123.4.11, mysql.cluster.example.com"};
    mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.17"}, ip_allowlist);
    mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.14"}, ip_allowlist);
    mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.13"}, ip_allowlist);
    mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.10"}, ip_allowlist);
    mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.4"}, ip_allowlist);
    EXPECT_THROW_LIKE(
        mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.3"}, ip_allowlist),
        shcore::Exception,
        "Invalid value for ipAllowlist 'mysql.cluster.example.com': hostnames "
        "are not supported");
    EXPECT_THROW_LIKE(
        mysqlsh::dba::validate_ip_allowlist_option(V{"5.7.27"}, ip_allowlist),
        shcore::Exception,
        "Invalid value for ipAllowlist 'mysql.cluster.example.com': hostnames "
        "are not supported");
  }
  {
    std::string_view ip_allowlist{"192.168.0.1/8, 127.0.0.1,10.123.4.11"};
    mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.17"}, ip_allowlist);
    mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.14"}, ip_allowlist);
    mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.13"}, ip_allowlist);
    mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.10"}, ip_allowlist);
    mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.4"}, ip_allowlist);
    mysqlsh::dba::validate_ip_allowlist_option(V{"8.0.3"}, ip_allowlist);
    mysqlsh::dba::validate_ip_allowlist_option(V{"5.7.27"}, ip_allowlist);
  }
}

// This test used to be for ensuring socket connections are rejected for
// InnoDB cluster, but they are now allowed.
TEST_F(Admin_api_cmdline_test, bug26970629) {
  const std::string variable = "socket";
  const std::string host =
      "--host="
#ifdef _WIN32
      "."
#else   // !_WIN32
      "localhost"
#endif  // !_WIN32
      ;
  const std::string pwd = "--password=" + _pwd;
  std::string socket;

  mysqlshdk::db::Connection_options options;
  options.set_host(_host);
  options.set_port(std::stoi(_mysql_port));
  options.set_user(_user);
  options.set_password(_pwd);

  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(options);

  auto result = session->query("show variables like '" + variable + "'");
  auto row = result->fetch_one();

  if (row) {
    std::string socket_path = row->get_as_string(1);

#ifdef _WIN32
    socket = "--socket=" + socket_path;
#else   // !_WIN32
    if (shcore::path_exists(socket_path)) {
      socket = "--socket=" + socket_path;
    } else {
      result = session->query("show variables like 'datadir'");
      row = result->fetch_one();
      socket_path = shcore::path::normalize(
          shcore::path::join_path(row->get_as_string(1), socket_path));

      if (shcore::path_exists(socket_path)) {
        socket = "--socket=" + socket_path;
      }
    }
#endif  // !_WIN32
  }

#ifdef _WIN32
  result = session->query("show variables like 'named_pipe'");
  row = result->fetch_one();

  if (!row || row->get_as_string(1) != "ON") {
    socket.clear();
  }
#endif  // _WIN32

  session->close();

  if (socket.empty()) {
    SCOPED_TRACE("Socket/Pipe Connections are Disabled, they must be enabled.");
    FAIL();
  } else {
    std::string usr = "--user=" + _user;

#ifdef HAVE_V8
    Command_line_test::execute({_mysqlsh, "--js", usr.c_str(), pwd.c_str(),
                                host.c_str(), socket.c_str(), "-e",
                                "dba.createCluster('sample')", NULL});
#else
    Command_line_test::execute({_mysqlsh, "--py", usr.c_str(), pwd.c_str(),
                                host.c_str(), socket.c_str(), "-e",
                                "dba.create_cluster('sample')", NULL});
#endif
    SCOPED_TRACE(socket.c_str());
    MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS(
        "a MySQL session through TCP/IP is required to perform this operation");
  }
}

}  // namespace testing
