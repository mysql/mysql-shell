/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates.
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
#include "modules/adminapi/cluster/add_instance.h"
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
    m_cluster->disconnect();
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
  mysqlshdk::db::Connection_options connection_options;

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

  // Set the connection options for _mysql_sandbox_ports[2]
  connection_options.set_host("localhost");
  connection_options.set_port(_mysql_sandbox_ports[2]);
  connection_options.set_user("root");
  connection_options.set_password("root");

  try {
    {
      // Create the add_instance command and execute it.
      mysqlsh::dba::cluster::Add_instance op_add_instance(
          connection_options, m_cluster->impl().get(), {}, {}, {}, false,
          mysqlsh::dba::Recovery_progress_style::NOWAIT, replication_user,
          replication_pwd);

      // Always execute finish when leaving scope.
      auto finally = shcore::on_leave_scope(
          [&op_add_instance]() { op_add_instance.finish(); });

      // Prepare the add_instance command execution (validations).
      op_add_instance.prepare();

      // Execute add_instance operations.
      op_add_instance.execute();
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
