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
#include "modules/adminapi/cluster/cluster_join.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/sql.h"
#include "modules/mod_shell.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "unittest/test_utils/admin_api_test.h"
#include "unittest/test_utils/mod_testutils.h"

using mysqlshdk::mysql::Instance;
using mysqlshdk::mysql::Var_qualifier;
namespace tests {

class Dba_cluster_test : public Admin_api_test {
 public:
  static std::shared_ptr<mysqlshdk::db::ISession> create_session(
      int port, std::string user = "root") {
    auto session = mysqlshdk::db::mysql::Session::create();

    auto connection_options = shcore::get_connection_options(
        user + ":root@localhost:" + std::to_string(port), false);
    session->connect(connection_options);

    return session;
  }
  virtual void SetUp() {
    Admin_api_test::SetUp();
    reset_replayable_shell(
        ::testing::UnitTest::GetInstance()->current_test_info()->name());

    execute("shell.connect('root:root@localhost:" +
            std::to_string(_mysql_sandbox_ports[0]) + "')");

    auto dba = _interactive_shell->shell_context()
                   ->get_global("dba")
                   .as_object<mysqlsh::dba::Dba>();
    m_cluster = dba->get_cluster({"sample"});

    execute("session.close()");
  }

  virtual void TearDown() {
    if (m_cluster) m_cluster->disconnect();
    Admin_api_test::TearDown();
  }

  static void SetUpTestCase() {
    SetUpSampleCluster("Dba_cluster_test/SetUpTestCase");
  }

  static void TearDownTestCase() {
    TearDownSampleCluster("Dba_cluster_test/TearDownTestCase");
  }

 protected:
  std::shared_ptr<mysqlshdk::db::ISession> create_base_session(int port) {
    mysqlshdk::db::Connection_options connection_options;

    connection_options.set_host("localhost");
    connection_options.set_port(port);
    connection_options.set_user("user");
    connection_options.set_password("");

    auto session = mysqlshdk::db::mysql::Session::create();

    session->connect(connection_options);

    return session;
  }

  std::shared_ptr<mysqlsh::dba::Cluster> m_cluster;
};

// Regression test for BUG#28219398
// INSTANCE ADDED TO CLUSTER STUCK IN RECOVERY PHASE DUE TO WRONG CHANGE MASTER
// CMD:
//
// Occasionally, when adding an instance to an existing cluster, the instance
// gets stuck in the distributed-recovery phase resulting in an immutable
// reported status of 'RECOVERY'.
//
// The issue happens every-time the generated password for the replication-user
// contains the following sequence of characters: '%s'.
TEST_F(Dba_cluster_test, bug28219398) {
  std::string replication_user, replication_pwd;

  replication_user = "mysql_innodb_cluster_r1711111111";

  // The password must contain the sequence of chars '%s';
  replication_pwd = "|t+-*2]1@+T?e%&H;*.%s)>)F/U}9,$?";

  // Create the replication_user on the cluster
  auto md_session = create_session(_mysql_sandbox_ports[0]);

  md_session->query("CREATE USER IF NOT EXISTS '" + replication_user +
                    "'@'localhost' IDENTIFIED BY /*((*/ '" + replication_pwd +
                    "' /*))*/ ");

  md_session->query("GRANT REPLICATION SLAVE ON *.* to /*(*/ '" +
                    replication_user + "'@'localhost' /*)*/");

  md_session->query("CREATE USER IF NOT EXISTS '" + replication_user +
                    "'@'%' IDENTIFIED BY /*((*/ '" + replication_pwd +
                    "' /*))*/ ");

  md_session->query("GRANT REPLICATION SLAVE ON *.* to /*(*/ '" +
                    replication_user + "'@'%' /*)*/");

  try {
    {
      mysqlsh::dba::Group_replication_options gr_opts;

      mysqlsh::dba::Instance_pool::Auth_options auth_opts;
      auth_opts.get(md_session->get_connection_options());
      mysqlsh::dba::Scoped_instance_pool ipool(false, auth_opts);

      gr_opts.recovery_credentials = mysqlshdk::mysql::Auth_options();
      gr_opts.recovery_credentials->user = replication_user;
      gr_opts.recovery_credentials->password = replication_pwd;

      auto target = std::make_unique<mysqlsh::dba::Instance>(
          create_session(_mysql_sandbox_ports[0]));

      // Create the add_instance command and execute it.
      mysqlsh::dba::cluster::Cluster_join joiner(
          m_cluster->impl().get(), target.get(),
          std::make_shared<mysqlsh::dba::Instance>(
              create_session(_mysql_sandbox_ports[2])),
          gr_opts, {}, false);

      // Prepare the add_instance command execution (validations).
      joiner.prepare_join({""});

      // Execute add_instance operations.
      joiner.join(mysqlsh::dba::Recovery_progress_style::NOWAIT);
    }

    // Set the Shell global session
    execute("shell.connect('root:root@localhost:" +
            std::to_string(_mysql_sandbox_ports[2]) + "')");

    testutil->wait_member_state(_mysql_sandbox_ports[2], "ONLINE", false);

    execute("session.close()");

    auto session = create_session(_mysql_sandbox_ports[2]);
    mysqlsh::dba::Instance instance(session);
    auto instance_type = mysqlsh::dba::get_gr_instance_type(instance);

    mysqlsh::dba::Cluster_check_info state =
        mysqlsh::dba::get_replication_group_state(instance, instance_type);

    instance.close_session();

    // The instance should be 'ONLINE' and not stuck in 'RECOVERING'
    EXPECT_EQ(state.source_state, mysqlsh::dba::ManagedInstance::OnlineRO);
  } catch (const shcore::Exception &e) {
    SCOPED_TRACE(e.what());
    SCOPED_TRACE("Unexpected failure adding instance.");
    ADD_FAILURE();
  }

  md_session->close();
}
}  // namespace tests
