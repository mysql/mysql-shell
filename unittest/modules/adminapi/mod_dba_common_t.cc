/*
 * Copyright (c) 2017, 2021, Oracle and/or its affiliates.
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
#include "modules/mod_shell.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "scripting/types.h"
#include "unittest/gtest_clean.h"
#include "unittest/mysqlshdk/libs/mysql/user_privileges_t.h"
#include "unittest/test_utils/admin_api_test.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"
#include "unittest/test_utils/mod_testutils.h"
#include "unittest/test_utils/shell_test_wrapper.h"

using mysqlshdk::mysql::Instance;
using mysqlshdk::mysql::Var_qualifier;
using mysqlshdk::utils::Version;

namespace testing {

class Dba_common_test : public tests::Admin_api_test {
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

TEST_F(Dba_common_test, resolve_cluster_ssl_mode_on_instance_with_ssl) {
  testutil->deploy_sandbox(_mysql_sandbox_ports[0], "root");
  auto instance = create_session(_mysql_sandbox_ports[0]);
  mysqlshdk::null_string member_ssl_mode;

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     ""            ON
  instance->set_sysvar("require_secure_transport", true, Var_qualifier::GLOBAL);
  try {
    member_ssl_mode = mysqlshdk::null_string();
    mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode);
    EXPECT_STREQ("REQUIRED", member_ssl_mode.get_safe().c_str());
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
    member_ssl_mode = mysqlshdk::null_string("AUTO");
    mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode);
    EXPECT_STREQ("REQUIRED", member_ssl_mode.get_safe().c_str());
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
    member_ssl_mode = mysqlshdk::null_string("REQUIRED");
    mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode);
    EXPECT_STREQ("REQUIRED", member_ssl_mode.get_safe().c_str());
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
    member_ssl_mode = mysqlshdk::null_string("VERIFY_CA");
    mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode);
    EXPECT_STREQ("VERIFY_CA", member_ssl_mode.get_safe().c_str());
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
    member_ssl_mode = mysqlshdk::null_string("VERIFY_IDENTITY");
    mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode);
    EXPECT_STREQ("VERIFY_IDENTITY", member_ssl_mode.get_safe().c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at require_secure_transport=ON, "
        "memberSslMode=VERIFY_IDENTITY");
    ADD_FAILURE();
  }

  // NOTE: This test only applies to 8.0. In 5.7, ssl_ca and ssl_capath are
  // read-only so whenever the server is started with SSL support those are
  // unchangeable. If the server is changed to not start with SSL support, then
  // the command would fail with the error indicating the Cluster is using SSL
  // so the instance cannot be added - Different code-path.
  //
  // To avoid looking at the version to decide whether the test shall be
  // executed or not, we simply emplace the test in a try..catch block ensuring
  // the exception caught when running in 5.7 is about ssl_ca being read-only.
  try {
    auto current_ssl_ca = instance->get_sysvar_string("ssl_ca").get_safe();
    auto current_ssl_capath =
        instance->get_sysvar_string("ssl_capath").get_safe();

    // Unset SSL CA option: ssl_ca and ssl_capath
    instance->set_sysvar_default("ssl_ca");
    instance->set_sysvar_default("ssl_capath");

    // An exception should be raised when the CA options are not set and
    // VERIFY_CA is used as memberSslMode
    member_ssl_mode = mysqlshdk::null_string("VERIFY_CA");
    EXPECT_THROW_MSG(
        mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode),
        std::runtime_error,
        "memberSslMode 'VERIFY_CA' requires Certificate Authority (CA) "
        "certificates to be supplied.");

    MY_EXPECT_STDOUT_CONTAINS(
        "ERROR: CA certificates options not set. --ssl-ca or --ssl-capath are "
        "required, to supply a CA certificate that matches the one used by the "
        "server.");

    // An exception should be raised when the CA options are not set and
    // VERIFY_IDENTITY is used as memberSslMode
    member_ssl_mode = mysqlshdk::null_string("VERIFY_IDENTITY");
    EXPECT_THROW_MSG(
        mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode),
        std::runtime_error,
        "memberSslMode 'VERIFY_IDENTITY' requires Certificate Authority (CA) "
        "certificates to be supplied.");

    MY_EXPECT_STDOUT_CONTAINS(
        "ERROR: CA certificates options not set. --ssl-ca or --ssl-capath are "
        "required, to supply a CA certificate that matches the one used by the "
        "server.");

    // Set back the original values
    instance->set_sysvar("ssl_ca", current_ssl_ca);
    instance->set_sysvar("ssl_capath", current_ssl_capath);
  } catch (const mysqlshdk::db::Error &e) {
    // In 5.7 ssl_ca and ssl_capath are read_only variables
    EXPECT_STREQ("Variable 'ssl_ca' is a read only variable", e.what());
  }

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     "DISABLED"    ON
  member_ssl_mode = mysqlshdk::null_string("DISABLED");
  EXPECT_THROW_MSG(
      mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode),
      std::runtime_error,
      "The instance '" + instance->descr() +
          "' requires secure connections, to create the cluster either turn "
          "off require_secure_transport or use the memberSslMode option "
          "with 'REQUIRED', 'VERIFY_CA' or 'VERIFY_IDENTITY' value.");

  // InstanceSSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // enabled     ""            OFF
  instance->set_sysvar("require_secure_transport", false,
                       Var_qualifier::GLOBAL);
  try {
    member_ssl_mode = mysqlshdk::null_string();
    mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode);
    EXPECT_STREQ("REQUIRED", member_ssl_mode.get_safe().c_str());
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
    member_ssl_mode = mysqlshdk::null_string("AUTO");
    mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode);
    EXPECT_STREQ("REQUIRED", member_ssl_mode.get_safe().c_str());
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
    member_ssl_mode = mysqlshdk::null_string("REQUIRED");
    mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode);
    EXPECT_STREQ("REQUIRED", member_ssl_mode.get_safe().c_str());
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
    member_ssl_mode = mysqlshdk::null_string("VERIFY_CA");
    mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode);
    EXPECT_STREQ("VERIFY_CA", member_ssl_mode.get_safe().c_str());
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
    member_ssl_mode = mysqlshdk::null_string("VERIFY_IDENTITY");
    mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode);
    EXPECT_STREQ("VERIFY_IDENTITY", member_ssl_mode.get_safe().c_str());
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
    member_ssl_mode = mysqlshdk::null_string("DISABLED");
    mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode);
    EXPECT_STREQ("DISABLED", member_ssl_mode.get_safe().c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at require_secure_transport=OFF, "
        "memberSslMode=DISABLED");
    ADD_FAILURE();
  }

  testutil->destroy_sandbox(_mysql_sandbox_ports[0]);
}

TEST_F(Dba_common_test, resolve_cluster_ssl_mode_on_instance_without_ssl) {
  testutil->deploy_sandbox(_mysql_sandbox_ports[0], "root");
  disable_ssl_on_instance(_mysql_sandbox_ports[0], "unsecure");
  mysqlshdk::null_string member_ssl_mode;

  auto instance = create_session(_mysql_sandbox_ports[0], "unsecure");

  // InstanceSSL memberSslMode
  //----------- -------------
  // disabled    "REQUIRED"
  member_ssl_mode = mysqlshdk::null_string("REQUIRED");
  EXPECT_THROW_MSG(
      mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode),
      std::runtime_error,
      "The instance '" + instance->descr() +
          "' does not have SSL enabled, to create the cluster "
          "either use an instance with SSL enabled, remove the memberSslMode "
          "option or use it with any of 'AUTO' or 'DISABLED'.");

  // InstanceSSL memberSslMode
  //----------- -------------
  // disabled    ""
  try {
    member_ssl_mode = mysqlshdk::null_string();
    mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode);
    EXPECT_STREQ("DISABLED", member_ssl_mode.get_safe().c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_cluster_ssl_mode_008");
    ADD_FAILURE();
  }

  // InstanceSSL memberSslMode
  //----------- -------------
  // disabled    "AUTO"
  try {
    member_ssl_mode = mysqlshdk::null_string("AUTO");
    mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode);
    EXPECT_STREQ("DISABLED", member_ssl_mode.get_safe().c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_cluster_ssl_mode_010");
    ADD_FAILURE();
  }

  // InstanceSSL memberSslMode
  //----------- -------------
  // disabled    "DISABLED"

  try {
    member_ssl_mode = mysqlshdk::null_string("DISABLED");
    mysqlsh::dba::resolve_cluster_ssl_mode(*instance, &member_ssl_mode);
    EXPECT_STREQ("DISABLED", member_ssl_mode.get_safe().c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at resolve_cluster_ssl_mode_009");
    ADD_FAILURE();
  }

  testutil->destroy_sandbox(_mysql_sandbox_ports[0]);
}

TEST_F(Dba_common_test, resolve_instance_ssl_cluster_with_ssl_required) {
  shcore::Dictionary_t sandbox_opts = shcore::make_dict();
  mysqlshdk::null_string member_ssl_mode;
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

  // Cluster SSL memberSslMode
  //----------- -------------
  // REQUIRED    ""
  try {
    member_ssl_mode = mysqlshdk::null_string();
    mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer, &member_ssl_mode);
    EXPECT_STREQ("REQUIRED", member_ssl_mode.get_safe().c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure with memberSslMode='', instance with SSL");
    ADD_FAILURE();
  }

  // Cluster SSL memberSslMode Instance SSL
  //----------- ------------- ------------
  // REQUIRED    AUTO          enabled
  try {
    member_ssl_mode = mysqlshdk::null_string("AUTO");
    mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer, &member_ssl_mode);
    EXPECT_STREQ("REQUIRED", member_ssl_mode.get_safe().c_str());
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
    member_ssl_mode = mysqlshdk::null_string("REQUIRED");
    mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer, &member_ssl_mode);
    EXPECT_STREQ("REQUIRED", member_ssl_mode.get_safe().c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at memberSslMode='REQUIRED', instance "
        "with SSL");
    ADD_FAILURE();
  }

  // Cluster SSL memberSslMode Instance SSL
  //----------- ------------- ------------
  // REQUIRED    VERIFY_CA      enabled
  try {
    member_ssl_mode = mysqlshdk::null_string("VERIFY_CA");
    mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer, &member_ssl_mode);
    EXPECT_STREQ("VERIFY_CA", member_ssl_mode.get_safe().c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at memberSslMode='VERIFY_CA', instance "
        "with SSL");
    ADD_FAILURE();
  }

  // Cluster SSL memberSslMode    Instance SSL
  //----------- -------------     ------------
  // REQUIRED    VERIFY_IDENTITY  enabled
  try {
    member_ssl_mode = mysqlshdk::null_string("VERIFY_IDENTITY");
    mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer, &member_ssl_mode);
    EXPECT_STREQ("VERIFY_IDENTITY", member_ssl_mode.get_safe().c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at memberSslMode='VERIFY_IDENTITY', instance "
        "with SSL");
    ADD_FAILURE();
  }

  // NOTE: This test only applies to 8.0. In 5.7, ssl_ca and ssl_capath are
  // read-only so whenever the server is started with SSL support those are
  // unchangeable. If the server is changed to not start with SSL support, then
  // the command would fail with the error indicating the Cluster is using SSL
  // so the instance cannot be added - Different code-path.
  //
  // To avoid looking at the version to decide whether the test shall be
  // executed or not, we simply emplace the test in a try..catch block ensuring
  // the exception caught when running in 5.7 is about ssl_ca being read-only.
  try {
    auto current_ssl_ca = instance->get_sysvar_string("ssl_ca").get_safe();
    auto current_ssl_capath =
        instance->get_sysvar_string("ssl_capath").get_safe();

    instance->set_sysvar_default("ssl_ca");
    instance->set_sysvar_default("ssl_capath");

    // An exception should be raised when the CA options are not set and
    // VERIFY_CA is used as memberSslMode
    member_ssl_mode = mysqlshdk::null_string("VERIFY_CA");
    EXPECT_THROW_MSG(
        mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer,
                                                &member_ssl_mode),
        std::runtime_error,
        "memberSslMode 'VERIFY_CA' requires Certificate Authority (CA) "
        "certificates to be supplied.");

    MY_EXPECT_STDOUT_CONTAINS(
        "ERROR: CA certificates options not set. --ssl-ca or --ssl-capath are "
        "required, to supply a CA certificate that matches the one used by the "
        "server.");

    // An exception should be raised when the CA options are not set and
    // VERIFY_IDENTITY is used as memberSslMode
    member_ssl_mode = mysqlshdk::null_string("VERIFY_IDENTITY");
    EXPECT_THROW_MSG(
        mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer,
                                                &member_ssl_mode),
        std::runtime_error,
        "memberSslMode 'VERIFY_IDENTITY' requires Certificate Authority (CA) "
        "certificates to be supplied.");

    MY_EXPECT_STDOUT_CONTAINS(
        "ERROR: CA certificates options not set. --ssl-ca or --ssl-capath are "
        "required, to supply a CA certificate that matches the one used by the "
        "server.");

    // Set back the original values
    instance->set_sysvar("ssl_ca", current_ssl_ca);
    instance->set_sysvar("ssl_capath", current_ssl_capath);
  } catch (const mysqlshdk::db::Error &e) {
    // In 5.7 ssl_ca and ssl_capath are read_only variables
    EXPECT_STREQ("Variable 'ssl_ca' is a read only variable", e.what());
  }

  // Cluster SSL memberSslMode
  //----------- -------------
  // REQUIRED    DISABLED
  member_ssl_mode = mysqlshdk::null_string("DISABLED");
  EXPECT_THROW_MSG(mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer,
                                                           &member_ssl_mode),
                   std::runtime_error,
                   "The cluster has SSL (encryption) enabled. "
                   "To add the instance '" +
                       instance->descr() +
                       "' to the cluster either disable SSL on the cluster, "
                       "remove the memberSslMode option or use it with any of "
                       "'AUTO', 'REQUIRED', 'VERIFY_CA' or "
                       "'VERIFY_IDENTITY'.");

  // Cluster SSL  memberSslMode     Instance SSL
  //-----------   ---------------   ------------
  // VERIFY_CA    ""                enabled
  try {
    peer->execute("SET GLOBAL group_replication_ssl_mode=VERIFY_CA");
    member_ssl_mode = mysqlshdk::null_string();
    mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer, &member_ssl_mode);
    EXPECT_STREQ("VERIFY_CA", member_ssl_mode.get_safe().c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE(
        "Unexpected failure at memberSslMode='VERIFY_CA', instance "
        "with SSL");
    ADD_FAILURE();
  }

  // Cluster SSL  memberSslMode     Instance SSL
  //-----------   ---------------   ------------
  // VERIFY_CA    ""                enabled
  try {
    peer->execute("SET GLOBAL group_replication_ssl_mode=VERIFY_IDENTITY");
    member_ssl_mode = mysqlshdk::null_string();
    mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer, &member_ssl_mode);
    EXPECT_STREQ("VERIFY_IDENTITY", member_ssl_mode.get_safe().c_str());
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
  member_ssl_mode = mysqlshdk::null_string("AUTO");
  EXPECT_THROW_MSG(mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer,
                                                           &member_ssl_mode),
                   std::runtime_error,
                   "Instance '" + instance->descr() +
                       "' does not support SSL and cannot join a cluster with "
                       "SSL (encryption) enabled. Enable SSL support on the "
                       "instance and try again, otherwise it can only be added "
                       "to a cluster with SSL disabled.");

  // Cluster SSL memberSslMode Instance SSL
  //----------- ------------- ------------
  // REQUIRED    REQUIRED      disabled
  member_ssl_mode = mysqlshdk::null_string("REQUIRED");
  EXPECT_THROW_MSG(
      mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer,
                                              &member_ssl_mode),
      std::runtime_error,
      "Instance '" + instance->descr() +
          "' does not support SSL and cannot join a cluster with SSL "
          "(encryption) enabled. Enable SSL support on the instance and try "
          "again, otherwise it can only be added to a cluster with SSL "
          "disabled.");

  // Cluster SSL memberSslMode Instance SSL
  //----------- ------------- ------------
  // REQUIRED    VERIFY_CA      disabled
  member_ssl_mode = mysqlshdk::null_string("VERIFY_CA");
  EXPECT_THROW_MSG(
      mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer,
                                              &member_ssl_mode),
      std::runtime_error,
      "Instance '" + instance->descr() +
          "' does not support SSL and cannot join a cluster with SSL "
          "(encryption) enabled. Enable SSL support on the instance and try "
          "again, otherwise it can only be added to a cluster with SSL "
          "disabled.");

  // Cluster SSL memberSslMode    Instance SSL
  //----------- -------------     ------------
  // REQUIRED    VERIFY_IDENTITY   disabled
  member_ssl_mode = mysqlshdk::null_string("VERIFY_IDENTITY");
  EXPECT_THROW_MSG(
      mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer,
                                              &member_ssl_mode),
      std::runtime_error,
      "Instance '" + instance->descr() +
          "' does not support SSL and cannot join a cluster with SSL "
          "(encryption) enabled. Enable SSL support on the instance and try "
          "again, otherwise it can only be added to a cluster with SSL "
          "disabled.");

  testutil->destroy_sandbox(_mysql_sandbox_ports[0]);
  testutil->destroy_sandbox(_mysql_sandbox_ports[1]);
}

TEST_F(Dba_common_test, resolve_instance_ssl_cluster_with_ssl_disabled) {
  shcore::Dictionary_t sandbox_opts = shcore::make_dict();
  (*sandbox_opts)["report_host"] = shcore::Value(hostname());
  mysqlshdk::null_string member_ssl_mode;

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

  // Cluster SSL memberSslMode
  //----------- -------------
  // DISABLED    REQUIRED
  member_ssl_mode = mysqlshdk::null_string("REQUIRED");
  EXPECT_THROW_MSG(
      mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer,
                                              &member_ssl_mode),
      std::runtime_error,
      "The cluster has SSL (encryption) disabled. "
      "To add the instance '" +
          instance->descr() +
          "' to the cluster either enable SSL on the cluster, remove the "
          "memberSslMode option or use it with any of 'AUTO' or 'DISABLED'.");

  // Cluster SSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // DISABLED    ""            OFF
  try {
    member_ssl_mode = mysqlshdk::null_string();
    mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer, &member_ssl_mode);
    EXPECT_STREQ("DISABLED", member_ssl_mode.get_safe().c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure using memberSslMode=''");
    ADD_FAILURE();
  }

  // Cluster SSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // DISABLED    AUTO          OFF
  try {
    member_ssl_mode = mysqlshdk::null_string("AUTO");
    mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer, &member_ssl_mode);
    EXPECT_STREQ("DISABLED", member_ssl_mode.get_safe().c_str());
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure using memberSslMode=AUTO");
    ADD_FAILURE();
  }

  instance->set_sysvar("require_secure_transport", true, Var_qualifier::GLOBAL);

  // Cluster SSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // DISABLED    ""            ON
  member_ssl_mode = mysqlshdk::null_string("AUTO");
  EXPECT_THROW_MSG(
      mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer,
                                              &member_ssl_mode),
      std::runtime_error,
      "The instance '" + instance->descr() +
          "' is configured to require a secure transport but the cluster has "
          "SSL disabled. To add the instance to the cluster, either turn OFF "
          "the require_secure_transport option on the instance or enable SSL "
          "on the cluster.");

  // Cluster SSL memberSslMode require_secure_transport
  //----------- ------------- ------------------------
  // DISABLED    AUTO          ON
  member_ssl_mode = mysqlshdk::null_string("AUTO");
  EXPECT_THROW_MSG(
      mysqlsh::dba::resolve_instance_ssl_mode(*instance, *peer,
                                              &member_ssl_mode),
      std::runtime_error,
      "The instance '" + instance->descr() +
          "' is configured to require a secure transport but the cluster has "
          "SSL disabled. To add the instance to the cluster, either turn OFF "
          "the require_secure_transport option on the instance or enable SSL "
          "on the cluster.");

  disable_ssl_on_instance(_mysql_sandbox_ports[1], "unsecure");
  instance = create_session(_mysql_sandbox_ports[1], "unsecure");

  testutil->destroy_sandbox(_mysql_sandbox_ports[0]);
  testutil->destroy_sandbox(_mysql_sandbox_ports[1]);
}

TEST_F(Dba_common_test, check_admin_account_access_restrictions) {
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
  EXPECT_TRUE(check_admin_account_access_restrictions(instance, "admin",
                                                      "myhost", true));

  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"",
                     {"grantee"},
                     {Type::String},
                     {{"'admin'@'myhost'"}, {"'admin'@'otherhost'"}}}});
  EXPECT_TRUE(check_admin_account_access_restrictions(instance, "admin",
                                                      "myhost", false));

  // TEST: Only one account not using wildcards (%) available for the user:
  // - Interactive 'true': return false;
  // - Interactive 'false': throw exception;
  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"", {"grantee"}, {Type::String}, {{"'admin'@'myhost'"}}}});
  EXPECT_FALSE(check_admin_account_access_restrictions(instance, "admin",
                                                       "myhost", true));

  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"", {"grantee"}, {Type::String}, {{"'admin'@'myhost'"}}}});
  EXPECT_THROW_LIKE(check_admin_account_access_restrictions(instance, "admin",
                                                            "myhost", false),
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
  EXPECT_TRUE(
      check_admin_account_access_restrictions(instance, "admin", "%", true));

  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"", {"grantee"}, {Type::String}, {{"'admin'@'%'"}}}});
  EXPECT_TRUE(
      check_admin_account_access_restrictions(instance, "admin", "%", false));

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
  EXPECT_TRUE(check_admin_account_access_restrictions(instance, "admin",
                                                      "192.168.1.20", true));

  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"",
                     {"grantee"},
                     {Type::String},
                     {{"'admin'@'192.168.1.0/255.255.255.0'"}}}});
  EXPECT_TRUE(check_admin_account_access_restrictions(instance, "admin",
                                                      "192.168.1.20", false));

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
      instance, "admin", "2001:0db8:85a3:0000:0000:8a2e:0370", true));

  mock_session
      ->expect_query(
          "SELECT DISTINCT grantee "
          "FROM information_schema.user_privileges "
          "WHERE grantee like '\\'admin\\'@%'")
      .then_return({{"",
                     {"grantee"},
                     {Type::String},
                     {{"'admin'@'2001:0db8:85a3:0000:0000:8a2e:0370'"}}}});
  EXPECT_THROW_LIKE(
      check_admin_account_access_restrictions(
          instance, "admin", "2001:0db8:85a3:0000:0000:8a2e:0370", false),
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
        options.is_8_0 = false;

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
  EXPECT_TRUE(check_admin_account_access_restrictions(instance, "admin",
                                                      "localhost", true));

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
  EXPECT_TRUE(check_admin_account_access_restrictions(instance, "admin",
                                                      "localhost", false));
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

// If the information on the Metadata and the GR group
// P_S info is the same get_newly_discovered_instances()
// result return an empty list
TEST_F(Dba_common_cluster_functions, get_newly_discovered_instances) {
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
TEST_F(Dba_common_cluster_functions, get_unavailable_instances) {
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

TEST_F(Dba_common_cluster_functions, validate_instance_rejoinable_01) {
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

TEST_F(Dba_common_cluster_functions, validate_instance_rejoinable_02) {
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

TEST_F(Dba_common_cluster_functions, validate_instance_rejoinable_03) {
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

TEST_F(Dba_common_test, super_read_only_server_on_flag_true) {
  enable_replay();
  testutil->deploy_sandbox(_mysql_sandbox_ports[0], "root");
  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(
      testutil->sandbox_connection_options(_mysql_sandbox_ports[0], "root"));

  // super_read_only is ON, no active sessions
  session->query("set global super_read_only = 1");

  try {
    auto read_only = mysqlsh::dba::validate_super_read_only(
        mysqlshdk::mysql::Instance(session), true, false);
    EXPECT_TRUE(read_only);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at super_read_only_server_on_flag_true");
    ADD_FAILURE();
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_ports[0]);
}

TEST_F(Dba_common_test, super_read_only_server_on_flag_false_open_sessions) {
  enable_replay();
  testutil->deploy_sandbox(_mysql_sandbox_ports[0], "root");
  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(
      testutil->sandbox_connection_options(_mysql_sandbox_ports[0], "root"));

  auto extra_session = mysqlshdk::db::mysql::Session::create();
  extra_session->connect(
      testutil->sandbox_connection_options(_mysql_sandbox_ports[0], "root"));

  // super_read_only is ON, no active sessions
  session->query("set global super_read_only = 1");

  try {
    mysqlsh::dba::validate_super_read_only(mysqlshdk::mysql::Instance(session),
                                           false, false);
    SCOPED_TRACE("Unexpected success calling validate_super_read_only");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ("Server in SUPER_READ_ONLY mode", e.what());
  }

  session->close();
  extra_session->close();
  testutil->destroy_sandbox(_mysql_sandbox_ports[0]);
}

TEST_F(Dba_common_test, super_read_only_server_on_flag_false_no_open_sessions) {
  enable_replay();
  testutil->deploy_sandbox(_mysql_sandbox_ports[0], "root");
  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(
      testutil->sandbox_connection_options(_mysql_sandbox_ports[0], "root"));

  // super_read_only is ON, no active sessions
  session->query("set global super_read_only = 1");
  try {
    mysqlsh::dba::validate_super_read_only(mysqlshdk::mysql::Instance(session),
                                           false, false);
    SCOPED_TRACE("Unexpected success calling validate_super_read_only");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ("Server in SUPER_READ_ONLY mode", e.what());
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_ports[0]);
}

TEST_F(Dba_common_test, super_read_only_server_off_flag_true) {
  enable_replay();
  testutil->deploy_sandbox(_mysql_sandbox_ports[0], "root");
  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(
      testutil->sandbox_connection_options(_mysql_sandbox_ports[0], "root"));

  // super_read_only is OFF, no active sessions
  session->query("set global super_read_only = 0");

  try {
    auto read_only = mysqlsh::dba::validate_super_read_only(
        mysqlshdk::mysql::Instance(session), true, false);
    EXPECT_FALSE(read_only);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at super_read_only_server_on_flag_true");
    ADD_FAILURE();
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_ports[0]);
}

TEST_F(Dba_common_test, super_read_only_server_off_flag_false) {
  enable_replay();
  testutil->deploy_sandbox(_mysql_sandbox_ports[0], "root");
  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(
      testutil->sandbox_connection_options(_mysql_sandbox_ports[0], "root"));

  // super_read_only is OFF, no active sessions
  session->query("set global super_read_only = 0");

  try {
    auto read_only = mysqlsh::dba::validate_super_read_only(
        mysqlshdk::mysql::Instance(session), false, false);
    EXPECT_FALSE(read_only);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure at super_read_only_server_on_flag_true");
    ADD_FAILURE();
  }

  session->close();
  testutil->destroy_sandbox(_mysql_sandbox_ports[0]);
}

TEST(mod_dba_common, validate_ipwhitelist_option) {
  using mysqlsh::dba::Group_replication_options;

  Group_replication_options options;
  // NOTE: hostnames_supported = true if version >= 8.0.4, otherwise false.
  auto ver_8014 = mysqlshdk::utils::Version(8, 0, 14);
  auto ver_804 = mysqlshdk::utils::Version(8, 0, 4);
  auto ver_800 = mysqlshdk::utils::Version(8, 0, 0);

  // Error if the ipWhitelist is empty.
  options.ip_allowlist_option_name = "ipWhitelist";
  options.ip_allowlist = "";
  try {
    options.check_option_values(Version(8, 0, 4));
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ("Invalid value for ipWhitelist: string value cannot be empty.",
                 e.what());
  }

  // Error if the ipWhitelist string is empty (only whitespace).
  options.ip_allowlist = " ";
  try {
    options.check_option_values(Version(8, 0, 4));
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ("Invalid value for ipWhitelist: string value cannot be empty.",
                 e.what());
  }

  // Error if CIDR is used but has an invalid value (not in range [1,32])
  options.ip_allowlist = "192.168.1.1/0";
  try {
    options.check_option_values(Version(8, 0, 4));
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist '192.168.1.1/0': subnet value in CIDR "
        "notation is not valid.",
        e.what());
  }

  // Error if CIDR is used but has an invalid value (not in range [1,32])
  options.ip_allowlist = "192.168.1.1/33";
  try {
    options.check_option_values(Version(8, 0, 4));
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist '192.168.1.1/33': subnet value in CIDR "
        "notation is not supported (version >= 8.0.14 required for IPv6 "
        "support).",
        e.what());
  }

  // Error if CIDR is used but has an invalid value (not in range [1,32])
  options.ip_allowlist = "1/33";
  try {
    options.check_option_values(Version(8, 0, 4));
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist '1/33': subnet value in CIDR "
        "notation is not supported (version >= 8.0.14 required for IPv6 "
        "support).",
        e.what());
  }

  // Error if CIDR is used but has an invalid value (not in range [1,32])
  // And a list of values is used
  options.ip_allowlist = "192.168.1.1/0,192.168.1.1/33";
  try {
    options.check_option_values(Version(8, 0, 4));
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist '192.168.1.1/0': subnet value in CIDR "
        "notation is not valid.",
        e.what());
  }

  // Error if ipWhitelist is an IPv6 address and not supported by server
  options.ip_allowlist = "2001:0db8:85a3:0000:0000:8a2e:0370:7334";
  try {
    options.check_option_values(Version(8, 0, 4));
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist "
        "'2001:0db8:85a3:0000:0000:8a2e:0370:7334': IPv6 not supported "
        "(version >= 8.0.14 required for IPv6 support).",
        e.what());
  }

  // Error if ipWhitelist is not a valid IPv4 address (not supported, < 8.0.4)
  options.ip_allowlist = "256.255.255.255";
  try {
    options.check_option_values(Version(8, 0, 3));
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist '256.255.255.255': hostnames are not "
        "supported (version >= 8.0.4 required for hostname support).",
        e.what());
  }

  // Error if ipWhitelist is not a valid IPv4 address
  options.ip_allowlist = "256.255.255.255/16";
  try {
    options.check_option_values(Version(8, 0, 4));
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist '256.255.255.255/16': CIDR notation can "
        "only be used with IPv4 or IPv6 addresses.",
        e.what());
  }

  // Error if hostname is used and server version < 8.0.4
  options.ip_allowlist = "localhost";
  try {
    options.check_option_values(Version(8, 0, 3));
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist 'localhost': hostnames are not "
        "supported (version >= 8.0.4 required for hostname support).",
        e.what());
  }

  // Error if hostname with cidr
  options.ip_allowlist = "localhost/8";
  try {
    options.check_option_values(Version(8, 0, 4));
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist 'localhost/8': CIDR notation can only "
        "be used with IPv4 or IPv6 addresses.",
        e.what());
  }

  // Error if hostname with cidr
  options.ip_allowlist = "bogus/8";
  try {
    options.check_option_values(Version(8, 0, 4));
    SCOPED_TRACE("Unexpected success calling validate_ip_whitelist_option");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "Invalid value for ipWhitelist 'bogus/8': CIDR notation can only be "
        "used with IPv4 or IPv6 addresses.",
        e.what());
  }

  // No error if ipWhitelist is an IPv6 address and server does support it
  options.ip_allowlist = "2001:0db8:85a3:0000:0000:8a2e:0370:7334";
  {
    options.check_option_values(Version(8, 0, 14));
    SCOPED_TRACE(
        "No error if ipWhitelist is an IPv6 address and server does support "
        "it");
    EXPECT_NO_THROW(options.check_option_values(Version(8, 0, 14)));
  }

  options.ip_allowlist = "256.255.255.255";
  {
    // Error if ipWhitelist is not a valid IPv4 address and version < 8.0.4
    // since it is not a valid IPv4 it assumes it must be an hostname and
    // hostnames are not supported below 8.0.4
    SCOPED_TRACE(
        "Error if ipWhitelist is not a valid IPv4 address and version < 8.0.4");
    EXPECT_THROW_LIKE(
        options.check_option_values(Version(8, 0, 0)), shcore::Exception,
        "Invalid value for ipWhitelist '256.255.255.255': hostnames are not "
        "supported (version >= 8.0.4 required for hostname support).");
  }

  {
    // Error if hostname is used and server version < 8.0.4
    options.ip_allowlist = "localhost";
    SCOPED_TRACE("Error if hostname is used and server version < 8.0.4");
    EXPECT_THROW_LIKE(
        options.check_option_values(Version(8, 0, 0)), shcore::Exception,
        "Invalid value for ipWhitelist 'localhost': hostnames are not "
        "supported (version >= 8.0.4 required for hostname support).");
  }

  {
    // No error if hostname is used and server version >= 8.0.4 because
    // we don't do hostname resolution
    SCOPED_TRACE("No error if hostname is used and server version >= 8.0.4");
    options.ip_allowlist = "fake_hostnanme";
    EXPECT_NO_THROW(options.check_option_values(Version(8, 0, 4)));
  }

  // No error if the ipWhitelist is a valid IPv4 address
  options.ip_allowlist = "192.168.1.1";
  EXPECT_NO_THROW(options.check_option_values(Version(8, 0, 4)));

  // No error if the ipWhitelist is a valid IPv4 address with a valid CIDR value
  options.ip_allowlist = "192.168.1.1/15";
  EXPECT_NO_THROW(options.check_option_values(Version(8, 0, 4)));

  // No error if the ipWhitelist consist of several valid IPv4 addresses with a
  // valid CIDR value
  // NOTE: if the server version is > 8.0.4, hostnames are allowed too so we
  // must test it
  options.ip_allowlist = "192.168.1.1/15,192.169.1.1/1, localhost";
  EXPECT_NO_THROW(options.check_option_values(Version(8, 0, 4)));
  options.ip_allowlist = "192.168.1.1/15,192.169.1.1/1";
  EXPECT_NO_THROW(options.check_option_values(Version(8, 0, 3)));
  options.ip_allowlist =
      "2001:0db8:85a3:0000:0000:8a2e:0370:7334/16,192.168.1.1/15,192.169.1.1/1";
  EXPECT_NO_THROW(options.check_option_values(Version(8, 0, 14)));
}

TEST(mod_dba_common, validate_exit_state_action_supported) {
  using mysqlsh::dba::Group_replication_options;

  Group_replication_options options;
  options.exit_state_action = "1";

  // Error only if the target server version is >= 5.7.24 if 5.0, or >= 8.0.12
  // if 8.0.

  EXPECT_THROW_LIKE(options.check_option_values(Version(5, 7, 23)),
                    shcore::Exception,
                    "Option 'exitStateAction' not supported on target server "
                    "version:");

  EXPECT_NO_THROW(options.check_option_values(Version(5, 7, 24)));

  EXPECT_THROW_LIKE(options.check_option_values(Version(8, 0, 11)),
                    shcore::Exception,
                    "Option 'exitStateAction' not supported on target server "
                    "version:");

  EXPECT_NO_THROW(options.check_option_values(Version(8, 0, 12)));
}

TEST(mod_dba_common, validate_member_weight_supported) {
  using mysqlsh::dba::Group_replication_options;

  Group_replication_options options;
  options.member_weight = 1;

  // Error only if the target server version is < 5.7.20 if 5.0, or < 8.0.11
  // if 8.0.

  EXPECT_THROW_LIKE(options.check_option_values(Version(5, 7, 19)),
                    shcore::Exception,
                    "Option 'memberWeight' not supported on target server "
                    "version:");

  EXPECT_NO_THROW(options.check_option_values(Version(5, 7, 20)));

  EXPECT_THROW_LIKE(options.check_option_values(Version(8, 0, 10)),
                    shcore::Exception,
                    "Option 'memberWeight' not supported on target server "
                    "version:");

  EXPECT_NO_THROW(options.check_option_values(Version(8, 0, 11)));
}

TEST(mod_dba_common, validate_consistency_supported) {
  using mysqlsh::dba::Group_replication_options;

  Group_replication_options options;
  Version version(8, 0, 14);

  auto empty_fail_cons = mysqlshdk::utils::nullable<std::string>("  ");
  auto null_fail_cons = mysqlshdk::utils::nullable<std::string>();
  auto valid_fail_cons = mysqlshdk::utils::nullable<std::string>("1");

  options.consistency = null_fail_cons;
  // if a null value was provided, it is as if the option was not provided,
  // so no error should be thrown
  options.check_option_values(version);

  options.consistency = empty_fail_cons;
  // if an empty value was provided, an error should be thrown independently
  // of the server version
  EXPECT_THROW_LIKE(
      options.check_option_values(version), shcore::Exception,
      "Invalid value for consistency, string value cannot be empty.");

  // if a valid value (non empty) was provided, an error should only be thrown
  // in case the option is not supported by the server version.
  options.consistency = valid_fail_cons;

  EXPECT_THROW_LIKE(options.check_option_values(Version(8, 0, 13)),
                    std::runtime_error,
                    "Option 'consistency' not supported on target server "
                    "version:");

  EXPECT_NO_THROW(options.check_option_values(Version(8, 0, 14)));
}

TEST(mod_dba_common, validate_auto_rejoin_tries_supported) {
  using mysqlsh::dba::Group_replication_options;

  Group_replication_options options;
  options.auto_rejoin_tries = 1;

  // Error only if the target server version is < 8.0.16

  EXPECT_THROW_LIKE(options.check_option_values(Version(5, 7, 19)),
                    shcore::Exception,
                    "Option 'autoRejoinTries' not supported on target server "
                    "version:");

  EXPECT_THROW_LIKE(options.check_option_values(Version(8, 0, 15)),
                    shcore::Exception,
                    "Option 'autoRejoinTries' not supported on target server "
                    "version:");

  EXPECT_NO_THROW(options.check_option_values(Version(8, 0, 16)));
}

TEST(mod_dba_common, validate_expel_timeout_supported) {
  using mysqlsh::dba::Group_replication_options;

  Group_replication_options options;
  Version version(8, 0, 13);

  auto null_timeout = mysqlshdk::utils::nullable<int64_t>();
  auto valid_timeout = mysqlshdk::utils::nullable<std::int64_t>(3600);
  auto invalid_timeout1 = mysqlshdk::utils::nullable<std::int64_t>(3601);
  auto invalid_timeout2 = mysqlshdk::utils::nullable<std::int64_t>(-1);
  // if a null value was provided, it is as if the option was not provided,
  // so no error should be thrown
  options.expel_timeout = null_timeout;
  options.check_option_values(version);

  // if a value non in the allowed range value was provided, an error should be
  // thrown independently of the server version
  options.expel_timeout = invalid_timeout1;
  EXPECT_THROW_LIKE(
      options.check_option_values(version), shcore::Exception,
      "Invalid value for expelTimeout, integer value must be in the range: "
      "[0, 3600]");

  options.expel_timeout = invalid_timeout2;
  EXPECT_THROW_LIKE(
      options.check_option_values(version), shcore::Exception,
      "Invalid value for expelTimeout, integer value must be in the range: "
      "[0, 3600]");

  // if a valid value was provided, an error should only be thrown
  // in case the option is not supported by the server version.
  options.expel_timeout = valid_timeout;
  EXPECT_THROW_LIKE(options.check_option_values(Version(8, 0, 12)),
                    std::runtime_error,
                    "Option 'expelTimeout' not supported on target server "
                    "version:");

  options.expel_timeout = valid_timeout;
  EXPECT_NO_THROW(options.check_option_values(Version(8, 0, 13)));
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

  // Error if the localAddress is empty.
  options.local_address = "";
  EXPECT_THROW(options.check_option_values(version), shcore::Exception);

  // Error if the localAddress string is empty (only whitespace).
  options.local_address = "  ";
  EXPECT_THROW(options.check_option_values(version), shcore::Exception);

  // Error if the localAddress has ':' and no host nor port part is specified.
  options.local_address = " : ";
  EXPECT_THROW(options.check_option_values(version), shcore::Exception);

  // No error if the localAddress is a non-empty string.
  options.local_address = "myhost:1234";
  EXPECT_NO_THROW(options.check_option_values(version));
  options.local_address = "myhost:";
  EXPECT_NO_THROW(options.check_option_values(version));
  options.local_address = ":1234";
  EXPECT_NO_THROW(options.check_option_values(version));
  options.local_address = "myhost";
  EXPECT_NO_THROW(options.check_option_values(version));
  options.local_address = "1234";
  EXPECT_NO_THROW(options.check_option_values(version));
}

TEST(mod_dba_common, validate_group_seeds_option) {
  using mysqlsh::dba::Group_replication_options;

  Group_replication_options options;
  Version version(8, 0, 14);

  // Error if the groupSeeds is empty.
  options.group_seeds = "";
  EXPECT_THROW(options.check_option_values(version), shcore::Exception);

  // Error if the groupSeeds string is empty (only whitespace).
  options.group_seeds = "  ";
  EXPECT_THROW(options.check_option_values(version), shcore::Exception);

  // No error if the groupSeeds is a non-empty string.
  options.group_seeds = "host1:1234,host2:4321";
  EXPECT_NO_THROW(options.check_option_values(version));
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
      // Valid identifier, begins with valid synbols (alpha)
      t = "Valid1"; mysqlsh::dba::validate_cluster_name(
          t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION));
  EXPECT_NO_THROW(
      // Valid identifier, begins with valid synbols (_)
      t = "_Valid_"; mysqlsh::dba::validate_cluster_name(
          t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION));
  EXPECT_NO_THROW(
      // Valid identifier, contains valid synbols
      t = "Valid_3"; mysqlsh::dba::validate_cluster_name(
          t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION));

  EXPECT_ANY_THROW(t = "";
                   // Invalid empty identifier
                   mysqlsh::dba::validate_cluster_name(
                       t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION););
  EXPECT_ANY_THROW(
      // Invalid too long identifier (over 40 characteres)
      t = "over40chars_12345678901234567890123456789";
      mysqlsh::dba::validate_cluster_name(
          t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION););
  EXPECT_ANY_THROW(
      // Invalid identifier, begins with invalid synbol
      t = "#not_allowed"; mysqlsh::dba::validate_cluster_name(
          t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION););
  EXPECT_ANY_THROW(
      // Invalid identifier, contains invalid synbol
      t = "not_allowed?"; mysqlsh::dba::validate_cluster_name(
          t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION););
  EXPECT_ANY_THROW(
      // Invalid identifier, begins with invalid synbols (numeric)
      t = "2_not_Valid"; mysqlsh::dba::validate_cluster_name(
          t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION));
  EXPECT_ANY_THROW(
      // Invalid identifier, contains invalid synbol
      t = "(*)%?"; mysqlsh::dba::validate_cluster_name(
          t, mysqlsh::dba::Cluster_type::GROUP_REPLICATION););
}

TEST_F(Dba_common_test, resolve_gr_local_address) {
  mysqlshdk::utils::nullable<std::string> local_address;
  std::string raw_report_host = "127.0.0.1";
  int port;

  // Tests for empty localAddress

  // Valid port, local_address null
  {
    local_address = mysqlshdk::utils::nullable<std::string>();
    port = 3306;
    EXPECT_NO_THROW(mysqlsh::dba::resolve_gr_local_address(
        local_address, raw_report_host, port, true));
  }

  // Valid port, local_address empty
  {
    local_address = std::string("");
    port = 3306;
    EXPECT_NO_THROW(mysqlsh::dba::resolve_gr_local_address(
        local_address, raw_report_host, port, true));
  }

  // Invalid port, local_address null
  {
    local_address.reset();
    port = 13040;

    EXPECT_THROW_LIKE(
        mysqlsh::dba::resolve_gr_local_address(local_address, raw_report_host,
                                               port, true),
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
        mysqlsh::dba::resolve_gr_local_address(local_address, raw_report_host,
                                               port, true),
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
      mysqlsh::dba::resolve_gr_local_address(local_address, raw_report_host,
                                             port, true);
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
        local_address, raw_report_host, port, true));
  }

  // Invalid port, complete local_address
  {
    local_address = std::string("127.0.0.1:130400");

    EXPECT_THROW_LIKE(
        mysqlsh::dba::resolve_gr_local_address(local_address, raw_report_host,
                                               port, true),
        shcore::Exception,
        "Invalid port '130400' for localAddress option. The port must be an "
        "integer between 1 and 65535.");
  }

  // Invalid port (string), complete local_address
  {
    local_address = std::string("127.0.0.1:a");

    EXPECT_THROW_LIKE(
        mysqlsh::dba::resolve_gr_local_address(local_address, raw_report_host,
                                               port, true),
        shcore::Exception,
        "Invalid port 'a' for localAddress option. The port must be an "
        "integer between 1 and 65535.");
  }

  // Invalid port, local_address without port part
  {
    local_address = std::string("127.0.0.2");
    port = 13040;

    EXPECT_THROW_LIKE(
        mysqlsh::dba::resolve_gr_local_address(local_address, raw_report_host,
                                               port, true),
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
        mysqlsh::dba::resolve_gr_local_address(local_address, raw_report_host,
                                               port, true),
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
        local_address, raw_report_host, port, true));
  }

  // Invalid port, local_address without separator ':', assumed to be the port
  {
    local_address = std::string("130401");

    EXPECT_THROW_LIKE(
        mysqlsh::dba::resolve_gr_local_address(local_address, raw_report_host,
                                               port, true),
        shcore::Exception,
        "Invalid port '130401' for localAddress option. The port must be an "
        "integer between 1 and 65535.");
  }

  // Busy port
  {
    int used_port = _mysql_sandbox_ports[0] * 10 + 1;
    local_address = std::string("127.0.0.1:") + std::to_string(used_port);

    try {
      mysqlsh::dba::resolve_gr_local_address(local_address, raw_report_host,
                                             port, true);
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
}  // namespace testing
