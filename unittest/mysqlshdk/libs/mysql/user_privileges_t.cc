/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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

#include "unittest/mysqlshdk/libs/mysql/user_privileges_t.h"

#include <memory>
#include <set>

// required for FRIEND_TEST
#include "unittest/gprod_clean.h"

#include "mysqlshdk/libs/mysql/user_privileges.h"
#include "mysqlshdk/libs/utils/utils_general.h"

#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_result.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"
#include "unittest/test_utils/shell_base_test.h"

using mysqlshdk::db::Type;
using mysqlshdk::mysql::Instance;
using mysqlshdk::utils::Version;
using testing::Mock_session;
using testing::Return;

namespace {

std::set<std::string> k_all_privileges = {
    "Alter",
    "Alter routine",
    "Create",
    "Create routine",
    "Create role",
    "Create temporary tables",
    "Create view",
    "Create user",
    "Delete",
    "Drop",
    "Drop role",
    "Event",
    "Execute",
    "File",
    "Grant option",
    "Index",
    "Insert",
    "Lock tables",
    "Process",
    "Proxy",
    "References",
    "Reload",
    "Replication client",
    "Replication slave",
    "Select",
    "Show databases",
    "Show view",
    "Shutdown",
    "Super",
    "Trigger",
    "Create tablespace",
    "Update",
    "Usage",
    "XA_RECOVER_ADMIN",
    "SHOW_ROUTINE",
    "SET_USER_ID",
    "SESSION_VARIABLES_ADMIN",
    "RESOURCE_GROUP_USER",
    "SYSTEM_VARIABLES_ADMIN",
    "REPLICATION_SLAVE_ADMIN",
    "REPLICATION_APPLIER",
    "BINLOG_ENCRYPTION_ADMIN",
    "RESOURCE_GROUP_ADMIN",
    "INNODB_REDO_LOG_ARCHIVE",
    "BINLOG_ADMIN",
    "PERSIST_RO_VARIABLES_ADMIN",
    "TABLE_ENCRYPTION_ADMIN",
    "SERVICE_CONNECTION_ADMIN",
    "AUDIT_ADMIN",
    "SYSTEM_USER",
    "APPLICATION_PASSWORD_ADMIN",
    "ROLE_ADMIN",
    "BACKUP_ADMIN",
    "CONNECTION_ADMIN",
    "ENCRYPTION_KEY_ADMIN",
    "CLONE_ADMIN",
    "FLUSH_OPTIMIZER_COSTS",
    "FLUSH_STATUS",
    "FLUSH_TABLES",
    "FLUSH_USER_RESOURCES",
    "GROUP_REPLICATION_ADMIN",
    "INNODB_REDO_LOG_ENABLE",
};

}  // namespace

namespace testing {
namespace user_privileges {

void setup(const Setup_options &options, Mock_session *session) {
  const auto account = shcore::make_account(options.user, options.host);

  {
    std::vector<std::vector<std::string>> user_exists;

    if (options.user_exists) {
      user_exists.emplace_back(std::vector<std::string>{"USAGE"});
    }

    session
        ->expect_query(shcore::sqlformat(
            "SELECT PRIVILEGE_TYPE FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
            "WHERE GRANTEE=? LIMIT 1",
            account))
        .then_return({{"", {"PRIVILEGE_TYPE"}, {Type::String}, user_exists}});
  }

  {
    std::vector<std::vector<std::string>> all_privileges;

    all_privileges.reserve(k_all_privileges.size());

    for (const auto &privilege : k_all_privileges) {
      all_privileges.emplace_back(std::vector<std::string>{privilege});
    }

    session->expect_query("SHOW PRIVILEGES")
        .then_return({{"", {"Privilege"}, {Type::String}, all_privileges}});
  }

  if (options.user_exists) {
    EXPECT_CALL(*session, get_server_version())
        .WillRepeatedly(
            Return(options.is_8_0 ? Version(8, 0, 0) : Version(5, 7, 28)));

    std::set<std::string> all_roles;

    if (options.is_8_0) {
      {
        std::vector<std::string> activate_all;

        activate_all.emplace_back(options.activate_all_roles_on_login ? "1"
                                                                      : "0");

        session->expect_query("SELECT @@GLOBAL.activate_all_roles_on_login")
            .then_return({{"",
                           {"@@GLOBAL.activate_all_roles_on_login"},
                           {Type::String},
                           {activate_all}}});
      }

      std::string query;

      if (options.activate_all_roles_on_login) {
        for (const auto &role : options.mandatory_roles) {
          std::string user;
          std::string host;

          shcore::split_account(role, &user, &host, false);

          if (host.empty()) {
            host = '%';
          }

          all_roles.emplace(shcore::make_account(user, host));
        }

        session->expect_query("SHOW GLOBAL VARIABLES LIKE 'mandatory_roles'")
            .then_return(
                {{"",
                  {"Variable_name", "Value"},
                  {Type::String, Type::String},
                  {{"mandatory_roles",
                    shcore::str_join(options.mandatory_roles, ",")}}}});

        query =
            "SELECT from_user, from_host FROM mysql.role_edges WHERE "
            "to_user=? AND to_host=?";
      } else {
        query =
            "SELECT default_role_user, default_role_host FROM "
            "mysql.default_roles WHERE user=? AND host=?";
      }

      std::vector<std::vector<std::string>> active_roles;

      for (const auto &role : options.active_roles) {
        active_roles.emplace_back(
            std::vector<std::string>{role.user, role.host});
        all_roles.emplace(shcore::make_account(role));
      }

      session
          ->expect_query(shcore::sqlformat(query, options.user, options.host))
          .then_return({{"",
                         {"user", "host"},
                         {Type::String, Type::String},
                         active_roles}});
    } else {
      session->expect_query("SELECT @@GLOBAL.activate_all_roles_on_login")
          .then_throw("Unknown system variable 'activate_all_roles_on_login'",
                      ER_UNKNOWN_SYSTEM_VARIABLE, "HY000");
    }

    std::vector<std::vector<std::string>> grants;

    for (const auto &grant : options.grants) {
      grants.emplace_back(std::vector<std::string>{grant});
    }

    std::string query = "SHOW GRANTS FOR " + account;

    if (!all_roles.empty()) {
      query += " USING " + shcore::str_join(all_roles, ",");
    }

    session->expect_query(query).then_return(
        {{"", {"grant"}, {Type::String}, grants}});
  }
}

std::set<std::string> all_privileges() {
  std::set<std::string> result;

  for (const auto &privilege : k_all_privileges) {
    result.emplace(shcore::str_upper(privilege));
  }

  result.erase("GRANT OPTION");
  result.erase("PROXY");

  return result;
}

}  // namespace user_privileges
}  // namespace testing

namespace mysqlshdk {
namespace mysql {

using testing::user_privileges::all_privileges;
using testing::user_privileges::setup;
using testing::user_privileges::Setup_options;

class User_privileges_test : public tests::Shell_base_test {
 protected:
  void SetUp() override {
    tests::Shell_base_test::SetUp();

    m_session = std::make_shared<Mock_session>();

    const auto connection_options = shcore::get_connection_options(_mysql_uri);

    EXPECT_CALL(*m_session, connect(connection_options));
    m_session->connect(connection_options);
  }

  void TearDown() override {
    EXPECT_CALL(*m_session, close());
    m_session->close();
  }

  User_privileges setup_test(const Setup_options &options) {
    setup(options, m_session.get());

    return {Instance(m_session), options.user, options.host};
  }

 private:
  std::shared_ptr<Mock_session> m_session;
};

TEST_F(User_privileges_test, validate_user_does_not_exist) {
  // Check non existing user.
  Setup_options setup;

  setup.user = "notexist_user";
  setup.host = "notexist_host";
  setup.user_exists = false;
  // Simulate a 5.7 version is used (not relevant for this test).
  setup.is_8_0 = false;

  const auto up = setup_test(setup);

  EXPECT_FALSE(up.user_exists());

  auto test = [&up](const std::set<std::string> &tested_privileges,
                    const std::set<std::string> &expected_privileges) {
    SCOPED_TRACE(shcore::str_join(tested_privileges, ", "));

    auto sup = up.validate(tested_privileges);

    EXPECT_FALSE(sup.user_exists());
    EXPECT_EQ(expected_privileges, sup.missing_privileges());
    EXPECT_TRUE(sup.has_missing_privileges());
    EXPECT_FALSE(sup.has_grant_option());
  };

  test({}, {});
  EXPECT_THROW_LIKE(up.validate({"all"}), std::runtime_error,
                    "Invalid privilege in the privileges list: ALL.");
  test({"select", "Insert", "UPDATE"}, {"SELECT", "INSERT", "UPDATE"});
}

TEST_F(User_privileges_test, validate_invalid_privileges) {
  // user with some privileges
  Setup_options setup;

  setup.user = "test_user";
  setup.host = "test_host";
  // Simulate 8.0.0 version is always used.
  setup.is_8_0 = true;

  setup.grants = {
      "GRANT USAGE ON *.* TO u@h",
      "GRANT INSERT, SELECT, UPDATE ON `test_db`.* TO u@h",
      "GRANT SELECT ON `test_db2`.* TO u@h WITH GRANT OPTION",
      "GRANT DELETE ON `test_db`.`t1` TO u@h",
      "GRANT DROP, ALTER ON `test_db`.`t2` TO u@h WITH GRANT OPTION",
  };

  const auto up = setup_test(setup);

  EXPECT_TRUE(up.user_exists());

  // Use of invalid privileges list.
  for (auto &privileges : std::vector<std::set<std::string>>{
           {"SELECT", "NO_PRIV", "INSERT"},
           {"SELECT", "ALL", "INSERT"},
           {"SELECT", "ALL PRIVILEGES", "INSERT"},
           {"SELECT", "All", "INSERT"}}) {
    SCOPED_TRACE(shcore::str_join(privileges, ", "));
    EXPECT_THROW(up.validate(privileges), std::runtime_error);
  }
}

TEST_F(User_privileges_test, validate_specific_privileges) {
  // Verify privileges for user with SELECT,INSERT,UPDATE on test_db.*,
  // DELETE on test_db.t1, ALTER,DROP on test_db.t2, and SELECT on test_db2.*.
  Setup_options setup;

  setup.user = "test_user";
  setup.host = "test_host";
  // Simulate 8.0.0 version is always used.
  setup.is_8_0 = true;

  setup.grants = {
      "GRANT USAGE ON *.* TO u@h",
      "GRANT INSERT, SELECT, UPDATE ON `test_db`.* TO u@h",
      "GRANT SELECT ON `test_db2`.* TO u@h WITH GRANT OPTION",
      "GRANT DELETE ON `test_db`.`t1` TO u@h",
      "GRANT DROP, ALTER ON `test_db`.`t2` TO u@h WITH GRANT OPTION",
  };

  const auto up = setup_test(setup);

  EXPECT_TRUE(up.user_exists());

  // Test subset of privileges for *.*, test_db.*, test_db2.*, mysql.*,
  // test_db.t1, test_db.t2, test_db.t3, test_db2.t1 and mysql.user
  std::set<std::string> test_priv = {"SELECT", "Insert", "UPDATE"};

  auto test = [&up, &test_priv](
                  const std::set<std::string> &missing_privileges,
                  bool expected_has_grant_option,
                  const std::string &schema = User_privileges::k_wildcard,
                  const std::string &table = User_privileges::k_wildcard) {
    SCOPED_TRACE(shcore::str_join(test_priv, ", ") + " on " + schema + "." +
                 table);

    auto sup = up.validate(test_priv, schema, table);

    EXPECT_TRUE(sup.user_exists());
    EXPECT_EQ(missing_privileges, sup.missing_privileges());
    EXPECT_EQ(missing_privileges.size() != 0, sup.has_missing_privileges());
    EXPECT_EQ(expected_has_grant_option, sup.has_grant_option());
  };

  test({"SELECT", "INSERT", "UPDATE"}, false);
  test({}, false, "test_db");
  test({"INSERT", "UPDATE"}, false, "test_db2");
  test({"SELECT", "INSERT", "UPDATE"}, false, "mysql");
  test({}, false, "test_db", "t1");
  test({}, false, "test_db", "t2");
  test({}, false, "test_db", "t3");
  test({"INSERT", "UPDATE"}, false, "test_db2", "t1");
  test({"SELECT", "INSERT", "UPDATE"}, false, "mysql", "user");

  // Test all given privileges for *.*, test_db.*, test_db2.*, mysql.*,
  // test_db.t1, test_db.t2, test_db.t3, test_db2.t1, and mysql.user
  test_priv = {"SELECT", "INSERT", "UPDATE", "Delete"};

  test({"SELECT", "INSERT", "UPDATE", "DELETE"}, false);
  test({"DELETE"}, false, "test_db");
  test({"INSERT", "UPDATE", "DELETE"}, false, "test_db2");
  test({"SELECT", "INSERT", "UPDATE", "DELETE"}, false, "mysql");
  test({}, false, "test_db", "t1");
  test({"DELETE"}, false, "test_db", "t2");
  test({"DELETE"}, false, "test_db", "t3");
  test({"INSERT", "UPDATE", "DELETE"}, false, "test_db2", "t1");
  test({"SELECT", "INSERT", "UPDATE", "DELETE"}, false, "mysql", "user");
}

TEST_F(User_privileges_test, validate_all_privileges) {
  // Verify privileges for dba_user with ALL on *.* WITH GRANT OPTION.
  Setup_options setup;

  setup.user = "dba_user";
  setup.host = "dba_host";
  // Simulate 8.0.0 version is always used.
  setup.is_8_0 = true;

  setup.grants = {
      "GRANT ALL PRIVILEGES ON *.* TO u@h WITH GRANT OPTION",
  };

  const auto up = setup_test(setup);

  // Test a subset of privileges for *.*, test_db.*, test_db2.*, mysql.*,
  // test_db.t1, test_db.t2, test_db.t3, test_db2.t1 and mysql.user
  std::set<std::string> test_priv = {"Select", "INSERT", "UPDATE", "DELETE"};

  EXPECT_TRUE(up.user_exists());

  auto test = [&up, &test_priv](
                  const std::string &schema = User_privileges::k_wildcard,
                  const std::string &table = User_privileges::k_wildcard) {
    SCOPED_TRACE(shcore::str_join(test_priv, ", ") + " on " + schema + "." +
                 table);

    auto sup = up.validate(test_priv, schema, table);

    EXPECT_TRUE(sup.user_exists());
    EXPECT_EQ(std::set<std::string>{}, sup.missing_privileges());
    EXPECT_FALSE(sup.has_missing_privileges());
    EXPECT_TRUE(sup.has_grant_option());
  };

  test();
  test("test_db");
  test("test_db2");
  test("mysql");
  test("test_db", "t1");
  test("test_db", "t2");
  test("test_db", "t3");
  test("test_db2", "t1");
  test("mysql", "user");
}

TEST_F(User_privileges_test, get_user_roles) {
  // Test retrieval of user roles.

  // Server does not support roles.
  {
    SCOPED_TRACE("Server does not support roles.");

    Setup_options setup;

    setup.user = "dba_user";
    setup.host = "dba_host";
    // 5.7 server, does not support roles
    setup.is_8_0 = false;

    setup.grants = {
        "GRANT USAGE *.* TO u@h",
    };

    const auto up = setup_test(setup);

    EXPECT_TRUE(up.get_user_roles().empty());
  }

  // User with no roles.
  {
    SCOPED_TRACE("User with no roles.");

    Setup_options setup;

    setup.user = "dba_user";
    setup.host = "dba_host";
    // Simulate 8.0.0 version is always used.
    setup.is_8_0 = true;

    setup.grants = {
        "GRANT USAGE *.* TO u@h",
    };

    const auto up = setup_test(setup);

    EXPECT_TRUE(up.get_user_roles().empty());
  }

  // User with roles (no mandatory roles).
  {
    SCOPED_TRACE("User with roles but no mandatory roles.");

    Setup_options setup;

    setup.user = "dba_user";
    setup.host = "dba_host";
    // Simulate 8.0.0 version is always used.
    setup.is_8_0 = true;

    setup.grants = {
        "GRANT USAGE *.* TO u@h",
    };

    setup.active_roles = {
        {"admin_role", "dba_host"},
    };

    const auto up = setup_test(setup);

    EXPECT_EQ((std::set<std::string>{"'admin_role'@'dba_host'"}),
              up.get_user_roles());
  }

  // User with roles (both granted and mandatory roles).
  {
    SCOPED_TRACE("User with both granted and mandatory roles.");

    Setup_options setup;

    setup.user = "dba_user";
    setup.host = "dba_host";
    // Simulate 8.0.0 version is always used.
    setup.is_8_0 = true;

    setup.grants = {
        "GRANT USAGE *.* TO u@h",
    };

    setup.activate_all_roles_on_login = true;
    setup.active_roles = {
        {"admin_role", "dba_host"},
        {"read_role", "dba_host"},
    };
    setup.mandatory_roles = {
        "m_role@dba_host",
        "read_role@dba_host",
        "write_role@dba_host",
    };

    const auto up = setup_test(setup);

    EXPECT_EQ((std::set<std::string>{
                  "'admin_role'@'dba_host'", "'read_role'@'dba_host'",
                  "'m_role'@'dba_host'", "'write_role'@'dba_host'"}),
              up.get_user_roles());
  }

  // User only with mandatory roles and activate all roles.
  {
    SCOPED_TRACE(
        "User only with mandatory roles and activate_all_roles_on_login=ON.");

    Setup_options setup;

    setup.user = "dba_user";
    setup.host = "dba_host";
    // Simulate 8.0.0 version is always used.
    setup.is_8_0 = true;

    setup.grants = {
        "GRANT USAGE *.* TO u@h",
    };

    setup.activate_all_roles_on_login = true;
    setup.mandatory_roles = {
        "`role1`@`%`",         "`role2`",        "'role3'", "role4@localhost",
        "role5@%.example.com", "\"weird,role\"",
    };

    const auto up = setup_test(setup);

    EXPECT_EQ(
        (std::set<std::string>{"'role1'@'%'", "'role2'@'%'", "'role3'@'%'",
                               "'role4'@'localhost'", "'role5'@'%.example.com'",
                               "'weird,role'@'%'"}),
        up.get_user_roles());
  }
}

namespace {

void validate_role_privileges(const User_privileges &up) {
  // Test test_user has all expected privileges inherited from its roles:
  // SELECT, INSERT, UPDATE; DELETE, CREATE, ALTER ON *.* (no grant option)
  std::set<std::string> test_priv = {"Select", "INSERT", "UPDATE",
                                     "DELETE", "create", "ALTER"};

  EXPECT_TRUE(up.user_exists());
  auto upr = up.validate(test_priv, "*", "*");
  EXPECT_TRUE(upr.user_exists());
  EXPECT_EQ(std::set<std::string>{}, upr.missing_privileges());
  EXPECT_FALSE(upr.has_missing_privileges());
  EXPECT_FALSE(upr.has_grant_option());

  upr = up.validate(test_priv, "test_db", "test_tbl");
  EXPECT_TRUE(upr.user_exists());
  EXPECT_EQ(std::set<std::string>{}, upr.missing_privileges());
  EXPECT_FALSE(upr.has_missing_privileges());
  EXPECT_FALSE(upr.has_grant_option());

  // Test missing privilege (DROP)
  test_priv = {"Select", "INSERT", "UPDATE", "DELETE",
               "create", "ALTER",  "DROP"};
  upr = up.validate(test_priv, "*", "*");
  EXPECT_TRUE(upr.user_exists());
  EXPECT_EQ(std::set<std::string>{"DROP"}, upr.missing_privileges());
  EXPECT_TRUE(upr.has_missing_privileges());
  EXPECT_FALSE(upr.has_grant_option());
}

}  // namespace

TEST_F(User_privileges_test, validate_role_privileges) {
  // Verify privileges for test_user with privileges associated to roles.
  // Individual privileges by user/role (without considering granted roles):
  // test_user -> USAGE ON *.* (no privileges)
  // create_role -> CREATE, ALTER ON *.*
  // write_role -> INSERT, UPDATE, DELETE ON *.*
  // read_role -> SELECT ON *.*
  Setup_options setup;

  setup.user = "test_user";
  setup.host = "%";
  // Simulate 8.0.0 version is always used.
  setup.is_8_0 = true;

  setup.grants = {"GRANT USAGE ON *.* TO u@h",
                  "GRANT CREATE, ALTER ON *.* TO u@h",
                  "GRANT SELECT ON *.* TO u@h",
                  "GRANT INSERT, UPDATE, DELETE ON *.* TO u@h",
                  "GRANT `create_role`@`%` TO u@h"};

  setup.active_roles = {
      {"create_role", "%"},
  };

  validate_role_privileges(setup_test(setup));
}

TEST_F(User_privileges_test, validate_role_privileges_direct) {
  if (_target_server_version < Version(8, 0, 0)) {
    SKIP_TEST("This test requires running against MySQL server version 8.0");
  }

  // Verify privileges for test_user with privileges associated to roles.

  // setup
  const auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(shcore::get_connection_options(_mysql_uri));

  session->execute("DROP USER IF EXISTS test_user");
  session->execute("DROP ROLE IF EXISTS create_role");
  session->execute("DROP ROLE IF EXISTS write_role");
  session->execute("DROP ROLE IF EXISTS read_role");

  // Individual privileges by user/role (without considering granted roles):
  // test_user -> USAGE ON *.* (no privileges)
  // create_role -> CREATE, ALTER ON *.*
  // write_role -> INSERT, UPDATE, DELETE ON *.*
  // read_role -> SELECT ON *.*
  session->execute("CREATE USER test_user");

  session->execute("CREATE ROLE create_role");
  session->execute("GRANT CREATE, ALTER ON *.* TO create_role");

  session->execute("CREATE ROLE write_role");
  session->execute("GRANT INSERT, UPDATE, DELETE ON *.* TO write_role");

  session->execute("CREATE ROLE read_role");
  session->execute("GRANT SELECT ON *.* TO read_role");

  // All granted roles:
  //  - read_role -> test_user
  //  - create_role -> test_user
  //  - read_role -> write_role
  //  - write_role -> create_role
  session->execute("GRANT read_role TO test_user");
  session->execute("GRANT create_role TO test_user");
  session->execute("SET DEFAULT ROLE create_role TO test_user");
  session->execute("GRANT read_role TO write_role");
  session->execute("GRANT write_role TO create_role");

  // test
  validate_role_privileges(
      User_privileges{Instance{session}, "test_user", "%"});

  // cleanup
  session->execute("DROP USER test_user");
  session->execute("DROP ROLE create_role");
  session->execute("DROP ROLE write_role");
  session->execute("DROP ROLE read_role");
}

TEST_F(User_privileges_test, partial_revokes) {
  if (_target_server_version < Version(8, 0, 16)) {
    SKIP_TEST("This test requires running against MySQL server version 8.0.16");
  }

  // Verify privileges for test_user with privileges associated to roles.

  // setup
  const auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(shcore::get_connection_options(_mysql_uri));

  session->execute("DROP USER IF EXISTS test_user");
  session->execute("DROP ROLE IF EXISTS app_dev");
  session->execute("DROP ROLE IF EXISTS app_user");

  session->execute("SET @saved_partial_revokes = @@GLOBAL.partial_revokes");
  session->execute("SET @@GLOBAL.partial_revokes = ON");

  // Individual privileges by user/role (without considering granted roles):
  // test_user -> USAGE ON *.* (no privileges)
  // app_dev -> SELECT ON *.*
  // app_user -> SELECT ON *.*, REVOKE SELECT ON `mysql`.*
  session->execute("CREATE USER test_user");

  session->execute("CREATE ROLE app_dev");
  session->execute("GRANT SELECT ON *.* TO app_dev");

  session->execute("CREATE ROLE app_user");
  session->execute("GRANT SELECT ON *.* TO app_user");
  session->execute("REVOKE SELECT ON `mysql`.* FROM app_user");

  // All granted roles:
  //  - app_dev -> test_user
  //  - app_user -> test_user
  session->execute("GRANT app_dev, app_user TO test_user");

  const auto test = [&session](bool has_global_select, bool has_mysql_select) {
    const auto up = User_privileges{Instance{session}, "test_user", "%"};

    EXPECT_EQ(has_global_select,
              !up.validate({"SELECT"}).has_missing_privileges());
    EXPECT_EQ(has_global_select,
              !up.validate({"SELECT"}, "sys").has_missing_privileges());
    EXPECT_EQ(has_mysql_select,
              !up.validate({"SELECT"}, "mysql").has_missing_privileges());
  };

  // no default roles, user has just USAGE
  test(false, false);

  // app_user is default, user has SELECT, but not on `mysql` schema
  session->execute("SET DEFAULT ROLE app_user TO test_user");
  test(true, false);

  // app_dev is default, user has SELECT ON *.*
  session->execute("SET DEFAULT ROLE app_dev TO test_user");
  test(true, true);

  // both are default, user has SELECT ON *.* (partial revoke is cancelled out)
  session->execute("SET DEFAULT ROLE app_dev, app_user TO test_user");
  test(true, true);

  // cleanup
  session->execute("DROP USER test_user");
  session->execute("DROP ROLE app_dev");
  session->execute("DROP ROLE app_user");

  session->execute("SET @@GLOBAL.partial_revokes = @saved_partial_revokes");
}

TEST_F(User_privileges_test, parse_grants) {
  Setup_options setup;

  setup.grants = {
      "REVOKE SELECT ON `1`.* FROM u@h",
      "REVOKE CREATE TEMPORARY TABLES ON `1`.* FROM u@h",
      "REVOKE CREATE TEMPORARY TABLES, CREATE VIEW, SELECT ON `2`.* FROM u@h",
      "REVOKE SELECT, CREATE VIEW, CREATE TEMPORARY TABLES ON `3`.* FROM u@h",
      "GRANT SELECT, INSERT (`id`) ON `4`.`4` TO u@h WITH GRANT OPTION",
      "GRANT INSERT (`id`), SELECT ON `5`.`5` TO u@h WITH GRANT OPTION",
      "GRANT INSERT (`id`, data), SELECT (x) ON `6`.`6` TO u@h WITH GRANT "
      "OPTION",
      "GRANT ALL PRIVILEGES ON *.* TO u@h REQUIRE NONE WITH GRANT OPTION",
      "GRANT ALL PRIVILEGES ON `7`.* TO u@h WITH GRANT OPTION",
      "GRANT ALL PRIVILEGES ON `8`.`8` TO u@h WITH GRANT OPTION",
      "GRANT r1, `r2`@`h`, \"r3\", 'r4'@'hh', 'r5,x' TO u@h WITH ADMIN OPTION",
      "REVOKE r1, `r2`@`h`, \"r3\", 'r4'@'hh', 'r5,x' FROM u@h",
      "GRANT EXECUTE ON FUNCTION `9`.`9` TO u@h",
      "GRANT EXECUTE ON PROCEDURE `10`.`10` TO u@h",
      "GRANT INSERT ON TABLE `11`.`11` TO u@h",
      "GRANT SELECT, SHOW DATABASES, INSERT, UPDATE, CREATE ROUTINE ON `12`.* "
      "TO u@h IDENTIFIED BY PASSWORD 'hash' WITH GRANT OPTION "
      "MAX_USER_CONNECTIONS 100",
  };

  const auto up = setup_test(setup);

  ASSERT_EQ(3, up.m_revoked_privileges.size());
  EXPECT_EQ((std::set<std::string>{"CREATE TEMPORARY TABLES", "SELECT"}),
            up.m_revoked_privileges.at("`1`.*"));
  EXPECT_EQ((std::set<std::string>{"CREATE TEMPORARY TABLES", "CREATE VIEW",
                                   "SELECT"}),
            up.m_revoked_privileges.at("`2`.*"));
  EXPECT_EQ((std::set<std::string>{"CREATE TEMPORARY TABLES", "CREATE VIEW",
                                   "SELECT"}),
            up.m_revoked_privileges.at("`3`.*"));

  ASSERT_EQ(6, up.m_grantable_privileges.size());
  EXPECT_EQ((std::set<std::string>{"SELECT"}),
            up.m_grantable_privileges.at("`4`.`4`"));
  EXPECT_EQ((std::set<std::string>{"SELECT"}),
            up.m_grantable_privileges.at("`5`.`5`"));
  EXPECT_EQ(all_privileges(), up.m_grantable_privileges.at("*.*"));
  EXPECT_EQ((std::set<std::string>{
                "ALTER",
                "ALTER ROUTINE",
                "CREATE",
                "CREATE ROUTINE",
                "CREATE TEMPORARY TABLES",
                "CREATE VIEW",
                "DELETE",
                "DROP",
                "EVENT",
                "EXECUTE",
                "INDEX",
                "INSERT",
                "LOCK TABLES",
                "REFERENCES",
                "SELECT",
                "SHOW VIEW",
                "TRIGGER",
                "UPDATE",
            }),
            up.m_grantable_privileges.at("`7`.*"));
  EXPECT_EQ((std::set<std::string>{
                "ALTER",
                "CREATE",
                "CREATE VIEW",
                "DELETE",
                "DROP",
                "INDEX",
                "INSERT",
                "REFERENCES",
                "SELECT",
                "SHOW VIEW",
                "TRIGGER",
                "UPDATE",
            }),
            up.m_grantable_privileges.at("`8`.`8`"));
  EXPECT_EQ((std::set<std::string>{
                "SELECT",
                "SHOW DATABASES",
                "INSERT",
                "UPDATE",
                "CREATE ROUTINE",
            }),
            up.m_grantable_privileges.at("`12`.*"));

  ASSERT_EQ(1, up.m_privileges.size());
  EXPECT_EQ((std::set<std::string>{"INSERT"}), up.m_privileges.at("`11`.`11`"));
}

}  // namespace mysql
}  // namespace mysqlshdk
