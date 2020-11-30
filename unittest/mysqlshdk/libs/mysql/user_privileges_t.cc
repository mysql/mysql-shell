/* Copyright (c) 2018, 2020, Oracle and/or its affiliates.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2.0,
 as published by the Free Software Foundation.

 This program is also distributed with certain software (including
 but not limited to OpenSSL) that is licensed under separate terms, as
 designated in a particular file or component or in included license
 documentation.  The authors of MySQL hereby grant you an additional
 permission to link the program and your derivative works with the
 separately licensed software that they have included with MySQL.
 This program is distributed in the hope that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 the GNU General Public License, version 2.0, for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include "mysqlshdk/libs/mysql/user_privileges.h"

#include <memory>
#include <set>

#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_result.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"
#include "unittest/test_utils/shell_base_test.h"

#include "mysqlshdk/libs/utils/utils_general.h"

using mysqlshdk::db::Type;
using mysqlshdk::mysql::User_privileges;

namespace testing {

class User_privileges_test : public tests::Shell_base_test {
 protected:
  void SetUp() override {
    tests::Shell_base_test::SetUp();

    m_mock_session = std::make_shared<Mock_session>();
    m_session = m_mock_session;

    m_connection_options = shcore::get_connection_options(_mysql_uri);

    EXPECT_CALL(*m_mock_session, connect(m_connection_options));
    m_session->connect(m_connection_options);
  }

  void TearDown() override {
    EXPECT_CALL(*m_mock_session, close());
    m_session->close();
  }

  std::shared_ptr<Mock_session> m_mock_session;

  // Required for User_privileges
  std::shared_ptr<mysqlshdk::db::ISession> m_session;

 private:
  mysqlshdk::db::Connection_options m_connection_options;
};

void set_all_privileges_queries(std::shared_ptr<Mock_session> &mock_session) {
  std::vector<std::string> plugins = {"audit_log", "mysql_firewall",
                                      "version_tokens"};
  for (const auto &plugin_name : plugins) {
    std::string query =
        "SELECT plugin_status FROM information_schema.plugins "
        "WHERE plugin_name = '" +
        plugin_name + "'";
    mock_session->expect_query(query).then_return({{
        "", {"plugin_status"}, {Type::String}, {}  // No record
    }});
  }

  mock_session
      ->expect_query(
          "SELECT engine FROM information_schema.engines "
          "WHERE engine = 'NDBCLUSTER'")
      .then_return({{
          "", {"engine"}, {Type::String}, {}  // No record
      }});
}

TEST_F(User_privileges_test, validate_user_does_not_exist) {
  // Check non existing user.
  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
          "WHERE GRANTEE = '\\'notexist_user\\'@\\'notexist_host\\''")
      .then_return({{
          "",
          {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
          {Type::String, Type::String},
          {}  // No Records
      }});

  // Simulate a 5.7 version is used (not relevant for this test).
  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillRepeatedly(Return(mysqlshdk::utils::Version(5, 7, 28)));

  User_privileges up{mysqlshdk::mysql::Instance(m_session), "notexist_user",
                     "notexist_host"};
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
  test({"all"}, {"ALL"});
  test({"select", "Insert", "UPDATE"}, {"SELECT", "INSERT", "UPDATE"});
}

TEST_F(User_privileges_test, validate_invalid_privileges) {
  // user with some privileges
  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
          "WHERE GRANTEE = '\\'test_user\\'@\\'test_host\\''")
      .then_return({{"",
                     {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});

  set_all_privileges_queries(m_mock_session);

  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA "
          "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
          "WHERE GRANTEE = '\\'test_user\\'@\\'test_host\\'' "
          "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
                     {"PRIVILEGE_TYPE", "IS_GRANTABLE", "TABLE_SCHEMA"},
                     {Type::String, Type::String, Type::String},
                     {{"INSERT", "NO", "test_db"},
                      {"SELECT", "NO", "test_db"},
                      {"UPDATE", "NO", "test_db"},
                      {"SELECT", "YES", "test_db2"}}}});
  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA, TABLE_NAME "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = '\\'test_user\\'@\\'test_host\\'' "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"PRIVILEGE_TYPE", "IS_GRANTABLE", "TABLE_SCHEMA", "TABLE_NAME"},
            {Type::String, Type::String, Type::String, Type::String},
            {{"DELETE", "NO", "test_db", "t1"},
             {"DROP", "YES", "test_db", "t2"},
             {"ALTER", "YES", "test_db", "t2"}}}});

  // Simulate 8.0.0 version is always used.
  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillRepeatedly(Return(mysqlshdk::utils::Version(8, 0, 0)));
  m_mock_session
      ->expect_query("SHOW GLOBAL VARIABLES LIKE 'activate_all_roles_on_login'")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String},
                     {{"activate_all_roles_on_login", "OFF"}}}});
  m_mock_session
      ->expect_query(
          "SELECT default_role_user, default_role_host "
          "FROM mysql.default_roles "
          "WHERE user = 'test_user' AND host = 'test_host'")
      .then_return({{
          "",
          {"default_role_user", "default_role_host"},
          {Type::String, Type::String},
          {}  // No Records.
      }});
  m_mock_session->expect_query("SHOW GLOBAL VARIABLES LIKE 'mandatory_roles'")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String},
                     {{"mandatory_roles", ""}}}});

  User_privileges up{mysqlshdk::mysql::Instance(m_session), "test_user",
                     "test_host"};

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
  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
          "WHERE GRANTEE = '\\'test_user\\'@\\'test_host\\''")
      .then_return({{"",
                     {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});

  set_all_privileges_queries(m_mock_session);

  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA "
          "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
          "WHERE GRANTEE = '\\'test_user\\'@\\'test_host\\'' "
          "ORDER BY TABLE_SCHEMA")
      .then_return({{"",
                     {"PRIVILEGE_TYPE", "IS_GRANTABLE", "TABLE_SCHEMA"},
                     {Type::String, Type::String, Type::String},
                     {{"INSERT", "NO", "test_db"},
                      {"SELECT", "NO", "test_db"},
                      {"UPDATE", "NO", "test_db"},
                      {"SELECT", "YES", "test_db2"}}}});
  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA, TABLE_NAME "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = '\\'test_user\\'@\\'test_host\\'' "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return(
          {{"",
            {"PRIVILEGE_TYPE", "IS_GRANTABLE", "TABLE_SCHEMA", "TABLE_NAME"},
            {Type::String, Type::String, Type::String, Type::String},
            {{"DELETE", "NO", "test_db", "t1"},
             {"DROP", "YES", "test_db", "t2"},
             {"ALTER", "YES", "test_db", "t2"}}}});

  // Simulate 8.0.0 version is always used.
  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillRepeatedly(Return(mysqlshdk::utils::Version(8, 0, 0)));
  m_mock_session
      ->expect_query("SHOW GLOBAL VARIABLES LIKE 'activate_all_roles_on_login'")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String},
                     {{"activate_all_roles_on_login", "OFF"}}}});
  m_mock_session
      ->expect_query(
          "SELECT default_role_user, default_role_host "
          "FROM mysql.default_roles "
          "WHERE user = 'test_user' AND host = 'test_host'")
      .then_return({{
          "",
          {"default_role_user", "default_role_host"},
          {Type::String, Type::String},
          {}  // No Records.
      }});
  m_mock_session->expect_query("SHOW GLOBAL VARIABLES LIKE 'mandatory_roles'")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String},
                     {{"mandatory_roles", ""}}}});

  // Test subset of privileges for *.*, test_db.*, test_db2.*, mysql.*,
  // test_db.t1, test_db.t2, test_db.t3, test_db2.t1 and mysql.user
  std::set<std::string> test_priv = {"SELECT", "Insert", "UPDATE"};

  User_privileges up{mysqlshdk::mysql::Instance(m_session), "test_user",
                     "test_host"};
  EXPECT_TRUE(up.user_exists());

  auto test = [&up, &test_priv](
                  const std::set<std::string> &expected_privileges,
                  bool expected_has_grant_option,
                  const std::string &schema = User_privileges::k_wildcard,
                  const std::string &table = User_privileges::k_wildcard) {
    SCOPED_TRACE(shcore::str_join(test_priv, ", ") + " on " + schema + "." +
                 table);

    auto sup = up.validate(test_priv, schema, table);

    EXPECT_TRUE(sup.user_exists());
    EXPECT_EQ(expected_privileges, sup.missing_privileges());
    EXPECT_EQ(expected_privileges.size() != 0, sup.has_missing_privileges());
    EXPECT_EQ(expected_has_grant_option, sup.has_grant_option());
  };

  test({"SELECT", "INSERT", "UPDATE"}, false);
  test({}, false, "test_db");
  test({"INSERT", "UPDATE"}, true, "test_db2");
  test({"SELECT", "INSERT", "UPDATE"}, false, "mysql");
  test({}, false, "test_db", "t1");
  test({}, true, "test_db", "t2");
  test({}, false, "test_db", "t3");
  test({"INSERT", "UPDATE"}, true, "test_db2", "t1");
  test({"SELECT", "INSERT", "UPDATE"}, false, "mysql", "user");

  // Test all given privileges for *.*, test_db.*, test_db2.*, mysql.*,
  // test_db.t1, test_db.t2, test_db.t3, test_db2.t1, and mysql.user
  test_priv = {"SELECT", "INSERT", "UPDATE", "Delete"};

  test({"SELECT", "INSERT", "UPDATE", "DELETE"}, false);
  test({"DELETE"}, false, "test_db");
  test({"INSERT", "UPDATE", "DELETE"}, true, "test_db2");
  test({"SELECT", "INSERT", "UPDATE", "DELETE"}, false, "mysql");
  test({}, false, "test_db", "t1");
  test({"DELETE"}, true, "test_db", "t2");
  test({"DELETE"}, false, "test_db", "t3");
  test({"INSERT", "UPDATE", "DELETE"}, true, "test_db2", "t1");
  test({"SELECT", "INSERT", "UPDATE", "DELETE"}, false, "mysql", "user");

  // Test ALL privileges for *.*, test_db.*, test_db2.*, mysql.*, test_db.t1,
  // test_db.t2, test_db.t3, test_db2.t1 and mysql.user
  test_priv = {"ALL"};

  const std::set<std::string> all_expected_priv{"ALTER",
                                                "ALTER ROUTINE",
                                                "CREATE",
                                                "CREATE ROLE",
                                                "CREATE ROUTINE",
                                                "CREATE TABLESPACE",
                                                "CREATE TEMPORARY TABLES",
                                                "CREATE USER",
                                                "CREATE VIEW",
                                                "DELETE",
                                                "DROP",
                                                "DROP ROLE",
                                                "EVENT",
                                                "EXECUTE",
                                                "FILE",
                                                "INDEX",
                                                "INSERT",
                                                "LOCK TABLES",
                                                "PROCESS",
                                                "REFERENCES",
                                                "RELOAD",
                                                "REPLICATION CLIENT",
                                                "REPLICATION SLAVE",
                                                "SELECT",
                                                "SHOW DATABASES",
                                                "SHOW VIEW",
                                                "SHUTDOWN",
                                                "SUPER",
                                                "TRIGGER",
                                                "UPDATE",
                                                "APPLICATION_PASSWORD_ADMIN",
                                                "BINLOG_ADMIN",
                                                "BINLOG_ENCRYPTION_ADMIN",
                                                "CONNECTION_ADMIN",
                                                "ENCRYPTION_KEY_ADMIN",
                                                "GROUP_REPLICATION_ADMIN",
                                                "INNODB_REDO_LOG_ARCHIVE",
                                                "PERSIST_RO_VARIABLES_ADMIN",
                                                "REPLICATION_SLAVE_ADMIN",
                                                "RESOURCE_GROUP_ADMIN",
                                                "RESOURCE_GROUP_USER",
                                                "ROLE_ADMIN",
                                                "SESSION_VARIABLES_ADMIN",
                                                "SET_USER_ID",
                                                "SYSTEM_USER",
                                                "SYSTEM_VARIABLES_ADMIN",
                                                "TABLE_ENCRYPTION_ADMIN",
                                                "XA_RECOVER_ADMIN"};
  // Note: This list doesn't include BACKUP_ADMIN, CLONE_ADMIN or
  // REPLICATION_APPLIER because we are simulating a 8.0.0 server when we mock
  // the get_server_version() call Instead of updating the server version on the
  // mock, we decided to leave it as is, since on user_privileges.cc the list
  // starts with all privileges and we drop the ones that are not supported by a
  // given server_version. So, by using mocking an 8.0 server we are testing if
  // the privileges are indeed being dropped from the list of ALL privileges.

  test({"ALL"}, false);
  // User with SELECT,INSERT,UPDATE on test_db.*.
  auto expected_priv = all_expected_priv;
  expected_priv.erase("INSERT");
  expected_priv.erase("SELECT");
  expected_priv.erase("UPDATE");
  test(expected_priv, false, "test_db");
  // User with SELECT on test_db2.*.
  expected_priv = all_expected_priv;
  expected_priv.erase("SELECT");
  test(expected_priv, true, "test_db2");
  test({"ALL"}, false, "mysql");
  // User with SELECT,INSERT,UPDATE on test_db.* and DELETE on test_db.t1.
  expected_priv = all_expected_priv;
  expected_priv.erase("INSERT");
  expected_priv.erase("SELECT");
  expected_priv.erase("UPDATE");
  expected_priv.erase("DELETE");
  test(expected_priv, false, "test_db", "t1");
  // User with SELECT,INSERT,UPDATE on test_db.* and ALTER, DROP on test_db.t2.
  expected_priv = all_expected_priv;
  expected_priv.erase("INSERT");
  expected_priv.erase("SELECT");
  expected_priv.erase("UPDATE");
  expected_priv.erase("ALTER");
  expected_priv.erase("DROP");
  test(expected_priv, true, "test_db", "t2");
  // User with SELECT,INSERT,UPDATE on test_db.*.
  expected_priv = all_expected_priv;
  expected_priv.erase("INSERT");
  expected_priv.erase("SELECT");
  expected_priv.erase("UPDATE");
  test(expected_priv, false, "test_db", "t3");
  // User with SELECT on test_db2.*.
  expected_priv = all_expected_priv;
  expected_priv.erase("SELECT");
  test(expected_priv, true, "test_db2", "t1");
  test({"ALL"}, false, "mysql", "user");
}  // NOLINT(readability/fn_size)

TEST_F(User_privileges_test, validate_all_privileges) {
  // Verify privileges for dba_user with ALL on *.* WITH GRANT OPTION.
  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
          "WHERE GRANTEE = '\\'dba_user\\'@\\'dba_host\\''")
      .then_return({{"",
                     {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "YES"},
                      {"INSERT", "YES"},
                      {"UPDATE", "YES"},
                      {"DELETE", "YES"},
                      {"CREATE", "YES"},
                      {"DROP", "YES"},
                      {"RELOAD", "YES"},
                      {"SHUTDOWN", "YES"},
                      {"PROCESS", "YES"},
                      {"FILE", "YES"},
                      {"REFERENCES", "YES"},
                      {"INDEX", "YES"},
                      {"ALTER", "YES"},
                      {"SHOW DATABASES", "YES"},
                      {"SUPER", "YES"},
                      {"CREATE TEMPORARY TABLES", "YES"},
                      {"LOCK TABLES", "YES"},
                      {"EXECUTE", "YES"},
                      {"REPLICATION SLAVE", "YES"},
                      {"REPLICATION CLIENT", "YES"},
                      {"CREATE VIEW", "YES"},
                      {"SHOW VIEW", "YES"},
                      {"CREATE ROUTINE", "YES"},
                      {"ALTER ROUTINE", "YES"},
                      {"CREATE USER", "YES"},
                      {"EVENT", "YES"},
                      {"TRIGGER", "YES"},
                      {"CREATE TABLESPACE", "YES"},
                      {"CREATE ROLE", "YES"},
                      {"DROP ROLE", "YES"},
                      {"APPLICATION_PASSWORD_ADMIN", "YES"},
                      {"BACKUP_ADMIN", "YES"},
                      {"BINLOG_ADMIN", "YES"},
                      {"BINLOG_ENCRYPTION_ADMIN", "YES"},
                      {"CLONE_ADMIN", "YES"},
                      {"CONNECTION_ADMIN", "YES"},
                      {"ENCRYPTION_KEY_ADMIN", "YES"},
                      {"GROUP_REPLICATION_ADMIN", "YES"},
                      {"INNODB_REDO_LOG_ARCHIVE", "YES"},
                      {"PERSIST_RO_VARIABLES_ADMIN", "YES"},
                      {"REPLICATION_APPLIER", "YES"},
                      {"REPLICATION_SLAVE_ADMIN", "YES"},
                      {"RESOURCE_GROUP_ADMIN", "YES"},
                      {"RESOURCE_GROUP_USER", "YES"},
                      {"ROLE_ADMIN", "YES"},
                      {"SESSION_VARIABLES_ADMIN", "YES"},
                      {"SET_USER_ID", "YES"},
                      {"SYSTEM_USER", "YES"},
                      {"SYSTEM_VARIABLES_ADMIN", "YES"},
                      {"TABLE_ENCRYPTION_ADMIN", "YES"},
                      {"XA_RECOVER_ADMIN", "YES"}}}});

  set_all_privileges_queries(m_mock_session);

  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA "
          "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
          "WHERE GRANTEE = '\\'dba_user\\'@\\'dba_host\\'' "
          "ORDER BY TABLE_SCHEMA")
      .then_return({{
          "",
          {"PRIVILEGE_TYPE", "IS_GRANTABLE", "TABLE_SCHEMA"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA, TABLE_NAME "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = '\\'dba_user\\'@\\'dba_host\\'' "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{
          "",
          {"PRIVILEGE_TYPE", "IS_GRANTABLE", "TABLE_SCHEMA", "TABLE_NAME"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});

  // Simulate 8.0.0 version is always used.
  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillRepeatedly(Return(mysqlshdk::utils::Version(8, 0, 0)));
  m_mock_session
      ->expect_query("SHOW GLOBAL VARIABLES LIKE 'activate_all_roles_on_login'")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String},
                     {{"activate_all_roles_on_login", "OFF"}}}});
  m_mock_session
      ->expect_query(
          "SELECT default_role_user, default_role_host "
          "FROM mysql.default_roles "
          "WHERE user = 'dba_user' AND host = 'dba_host'")
      .then_return({{
          "",
          {"default_role_user", "default_role_host"},
          {Type::String, Type::String},
          {}  // No Records.
      }});
  m_mock_session->expect_query("SHOW GLOBAL VARIABLES LIKE 'mandatory_roles'")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String},
                     {{"mandatory_roles", ""}}}});

  // Test ALL privileges for *.*, test_db.*, test_db2.*, mysql.*, test_db.t1,
  // test_db.t2, test_db.t3, test_db2.t1, and mysql.user
  std::set<std::string> test_priv = {"All"};

  User_privileges up{mysqlshdk::mysql::Instance(m_session), "dba_user",
                     "dba_host"};
  EXPECT_TRUE(up.user_exists());

  using User_privileges = mysqlshdk::mysql::User_privileges;
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

  // Test a subset of privileges for *.*, test_db.*, test_db2.*, mysql.*,
  // test_db.t1, test_db.t2, test_db.t3, test_db2.t1 and mysql.user
  test_priv = {"Select", "INSERT", "UPDATE", "DELETE"};

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
  auto expect_no_privileges = [](std::shared_ptr<Mock_session> &mock_session,
                                 const std::string &user_account,
                                 bool is_role = false) {
    mock_session
        ->expect_query(
            "SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
            "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
            "WHERE GRANTEE = '" +
            user_account + "'")
        .then_return({{"",
                       {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                       {Type::String, Type::String},
                       {{"USAGE", "NO"}}}});

    if (!is_role) {
      set_all_privileges_queries(mock_session);
    }

    mock_session
        ->expect_query(
            "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA "
            "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
            "WHERE GRANTEE = '" +
            user_account +
            "' "
            "ORDER BY TABLE_SCHEMA")
        .then_return({{
            "",
            {"PRIVILEGE_TYPE", "IS_GRANTABLE", "TABLE_SCHEMA"},
            {Type::String, Type::String, Type::String},
            {}  // No Records.
        }});
    mock_session
        ->expect_query(
            "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA, TABLE_NAME "
            "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
            "WHERE GRANTEE = '" +
            user_account +
            "' "
            "ORDER BY TABLE_SCHEMA, TABLE_NAME")
        .then_return({{
            "",
            {"PRIVILEGE_TYPE", "IS_GRANTABLE", "TABLE_SCHEMA", "TABLE_NAME"},
            {Type::String, Type::String, Type::String, Type::String},
            {}  // No Records.
        }});
  };

  // User with no roles.
  {
    SCOPED_TRACE("User with no roles.");
    expect_no_privileges(m_mock_session, "\\'dba_user\\'@\\'dba_host\\'");

    // Simulate 8.0.0 version is always used.
    EXPECT_CALL(*m_mock_session, get_server_version())
        .WillRepeatedly(Return(mysqlshdk::utils::Version(8, 0, 0)));

    m_mock_session
        ->expect_query(
            "SHOW GLOBAL VARIABLES LIKE 'activate_all_roles_on_login'")
        .then_return({{"",
                       {"Variable_name", "Value"},
                       {Type::String},
                       {{"activate_all_roles_on_login", "OFF"}}}});
    m_mock_session
        ->expect_query(
            "SELECT default_role_user, default_role_host "
            "FROM mysql.default_roles "
            "WHERE user = 'dba_user' AND host = 'dba_host'")
        .then_return({{
            "",
            {"default_role_user", "default_role_host"},
            {Type::String, Type::String},
            {}  // No Records.
        }});

    User_privileges up_no_roles{mysqlshdk::mysql::Instance(m_session),
                                "dba_user", "dba_host"};
    std::set<std::string> res = up_no_roles.get_user_roles();
    EXPECT_TRUE(res.empty());
  }

  // User with roles (no mandatory roles).
  {
    SCOPED_TRACE("User with roles but no mandatory roles.");
    expect_no_privileges(m_mock_session, "\\'dba_user\\'@\\'dba_host\\'");

    // Simulate 8.0.0 version is always used.
    EXPECT_CALL(*m_mock_session, get_server_version())
        .WillRepeatedly(Return(mysqlshdk::utils::Version(8, 0, 0)));

    m_mock_session
        ->expect_query(
            "SHOW GLOBAL VARIABLES LIKE 'activate_all_roles_on_login'")
        .then_return({{"",
                       {"Variable_name", "Value"},
                       {Type::String},
                       {{"activate_all_roles_on_login", "OFF"}}}});
    m_mock_session
        ->expect_query(
            "SELECT default_role_user, default_role_host "
            "FROM mysql.default_roles "
            "WHERE user = 'dba_user' AND host = 'dba_host'")
        .then_return({{"",
                       {"default_role_user", "default_role_host"},
                       {Type::String, Type::String},
                       {{"admin_role", "dba_host"}}}});
    m_mock_session
        ->expect_query(
            "SELECT from_user, from_host, to_user, to_host "
            "FROM mysql.role_edges")
        .then_return({{"",
                       {"from_user", "from_host", "to_user", "to_host"},
                       {Type::String, Type::String, Type::String, Type::String},
                       {{"dba_user", "dba_host", "admin_role", "dba_host"},
                        {"admin_role", "dba_host", "dba_user", "dba_host"},
                        {"root", "dba_host", "admin_role", "dba_host"}}}});
    m_mock_session->expect_query("SHOW GLOBAL VARIABLES LIKE 'mandatory_roles'")
        .then_return({{"",
                       {"Variable_name", "Value"},
                       {Type::String},
                       {{"mandatory_roles", ""}}}});

    expect_no_privileges(m_mock_session, "\\'admin_role\\'@\\'dba_host\\'",
                         true);
    expect_no_privileges(m_mock_session, "\\'root\\'@\\'dba_host\\'", true);

    User_privileges up_roles{mysqlshdk::mysql::Instance(m_session), "dba_user",
                             "dba_host"};
    std::set<std::string> res = up_roles.get_user_roles();
    EXPECT_EQ(res.size(), 2);
    EXPECT_THAT(res, UnorderedElementsAre("'admin_role'@'dba_host'",
                                          "'root'@'dba_host'"));
  }

  // User with roles (both granted and mandatory roles).
  {
    SCOPED_TRACE("User with both granted and mandatory roles.");
    expect_no_privileges(m_mock_session, "\\'dba_user\\'@\\'dba_host\\'");

    // Simulate 8.0.0 version is always used.
    EXPECT_CALL(*m_mock_session, get_server_version())
        .WillRepeatedly(Return(mysqlshdk::utils::Version(8, 0, 0)));

    m_mock_session
        ->expect_query(
            "SHOW GLOBAL VARIABLES LIKE 'activate_all_roles_on_login'")
        .then_return({{"",
                       {"Variable_name", "Value"},
                       {Type::String},
                       {{"activate_all_roles_on_login", "OFF"}}}});
    m_mock_session
        ->expect_query(
            "SELECT default_role_user, default_role_host "
            "FROM mysql.default_roles "
            "WHERE user = 'dba_user' AND host = 'dba_host'")
        .then_return({{"",
                       {"default_role_user", "default_role_host"},
                       {Type::String, Type::String},
                       {{"admin_role", "dba_host"}}}});
    m_mock_session
        ->expect_query(
            "SELECT from_user, from_host, to_user, to_host "
            "FROM mysql.role_edges")
        .then_return({{"",
                       {"from_user", "from_host", "to_user", "to_host"},
                       {Type::String, Type::String, Type::String, Type::String},
                       {{"dba_user", "dba_host", "admin_role", "dba_host"},
                        {"admin_role", "dba_host", "dba_user", "dba_host"},
                        {"read_role", "dba_host", "dba_user", "dba_host"},
                        {"write_role", "dba_host", "admin_role", "dba_host"},
                        {"root", "dba_host", "admin_role", "dba_host"}}}});
    m_mock_session->expect_query("SHOW GLOBAL VARIABLES LIKE 'mandatory_roles'")
        .then_return(
            {{"",
              {"Variable_name", "Value"},
              {Type::String},
              {{"mandatory_roles",
                "m_role@dba_host,read_role@dba_host,write_role@dba_host"}}}});

    expect_no_privileges(m_mock_session, "\\'admin_role\\'@\\'dba_host\\'",
                         true);
    expect_no_privileges(m_mock_session, "\\'root\\'@\\'dba_host\\'", true);
    expect_no_privileges(m_mock_session, "\\'write_role\\'@\\'dba_host\\'",
                         true);

    User_privileges up_all_roles{mysqlshdk::mysql::Instance(m_session),
                                 "dba_user", "dba_host"};
    std::set<std::string> res = up_all_roles.get_user_roles();
    EXPECT_EQ(res.size(), 3);
    EXPECT_THAT(res, UnorderedElementsAre("'admin_role'@'dba_host'",
                                          "'write_role'@'dba_host'",
                                          "'root'@'dba_host'"));
  }

  // User only with mandatory roles and activate all roles.
  {
    SCOPED_TRACE(
        "User only with mandatory roles and activate_all_roles_on_login=ON.");
    expect_no_privileges(m_mock_session, "\\'dba_user\\'@\\'dba_host\\'");

    // Simulate 8.0.0 version is always used.
    EXPECT_CALL(*m_mock_session, get_server_version())
        .WillRepeatedly(Return(mysqlshdk::utils::Version(8, 0, 0)));

    m_mock_session
        ->expect_query(
            "SHOW GLOBAL VARIABLES LIKE 'activate_all_roles_on_login'")
        .then_return({{"",
                       {"Variable_name", "Value"},
                       {Type::String},
                       {{"activate_all_roles_on_login", "ON"}}}});
    m_mock_session
        ->expect_query(
            "SELECT from_user, from_host, to_user, to_host "
            "FROM mysql.role_edges")
        .then_return({{"",
                       {"from_user", "from_host", "to_user", "to_host"},
                       {Type::String, Type::String, Type::String, Type::String},
                       {}}});  // No record
    m_mock_session->expect_query("SHOW GLOBAL VARIABLES LIKE 'mandatory_roles'")
        .then_return({{"",
                       {"Variable_name", "Value"},
                       {Type::String},
                       {{"mandatory_roles",
                         "`role1`@`%`,`role2`,role3,role4@localhost"}}}});

    expect_no_privileges(m_mock_session, "\\'role1\\'@\\'%\\'", true);
    expect_no_privileges(m_mock_session, "\\'role2\\'@\\'%\\'", true);
    expect_no_privileges(m_mock_session, "\\'role3\\'@\\'%\\'", true);
    expect_no_privileges(m_mock_session, "\\'role4\\'@\\'localhost\\'", true);

    User_privileges up_mr_roles{mysqlshdk::mysql::Instance(m_session),
                                "dba_user", "dba_host"};
    std::set<std::string> res = up_mr_roles.get_user_roles();
    EXPECT_EQ(res.size(), 4);
    EXPECT_THAT(
        res, UnorderedElementsAre("'role1'@'%'", "'role2'@'%'", "'role3'@'%'",
                                  "'role4'@'localhost'"));
  }
}

TEST_F(User_privileges_test, validate_role_privileges) {
  // Verify privileges for test_user with privileges associated to roles.
  // Individual privileges by user/role (without considering granted roles):
  // test_user -> USAGE ON *.* (no privileges)
  // create_role -> CREATE, ALTER ON *.*
  // write_role -> INSERT, UPDATE, DELETE ON *.*
  // read_role -> SELECT ON *.*
  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
          "WHERE GRANTEE = '\\'test_user\\'@\\'%\\''")
      .then_return({{"",
                     {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"USAGE", "NO"}}}});

  set_all_privileges_queries(m_mock_session);

  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA "
          "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
          "WHERE GRANTEE = '\\'test_user\\'@\\'%\\'' "
          "ORDER BY TABLE_SCHEMA")
      .then_return({{
          "",
          {"PRIVILEGE_TYPE", "IS_GRANTABLE", "TABLE_SCHEMA"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA, TABLE_NAME "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = '\\'test_user\\'@\\'%\\'' "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{
          "",
          {"PRIVILEGE_TYPE", "IS_GRANTABLE", "TABLE_SCHEMA", "TABLE_NAME"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});

  // Simulate 8.0.0 version is always used.
  EXPECT_CALL(*m_mock_session, get_server_version())
      .WillRepeatedly(Return(mysqlshdk::utils::Version(8, 0, 0)));

  // Active role for test_user: create_role
  m_mock_session
      ->expect_query("SHOW GLOBAL VARIABLES LIKE 'activate_all_roles_on_login'")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String},
                     {{"activate_all_roles_on_login", "OFF"}}}});
  m_mock_session
      ->expect_query(
          "SELECT default_role_user, default_role_host "
          "FROM mysql.default_roles "
          "WHERE user = 'test_user' AND host = '%'")
      .then_return({{"",
                     {"default_role_user", "default_role_host"},
                     {Type::String, Type::String},
                     {{"create_role", "%"}}}});
  // All granted roles:
  //  - read_role -> test_user
  //  - create_role -> test_user
  //  - read_role -> write_role
  //  - write_role -> create_role
  m_mock_session
      ->expect_query(
          "SELECT from_user, from_host, to_user, to_host "
          "FROM mysql.role_edges")
      .then_return({{"",
                     {"from_user", "from_host", "to_user", "to_host"},
                     {Type::String, Type::String, Type::String, Type::String},
                     {{"create_role", "%", "test_user", "%"},
                      {"read_role", "%", "test_user", "%"},
                      {"read_role", "%", "write_role", "%"},
                      {"write_role", "%", "create_role", "%"}}}});
  m_mock_session->expect_query("SHOW GLOBAL VARIABLES LIKE 'mandatory_roles'")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String},
                     {{"mandatory_roles", ""}}}});

  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
          "WHERE GRANTEE = '\\'create_role\\'@\\'%\\''")
      .then_return({{"",
                     {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"CREATE", "NO"}, {"ALTER", "NO"}}}});
  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA "
          "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
          "WHERE GRANTEE = '\\'create_role\\'@\\'%\\'' "
          "ORDER BY TABLE_SCHEMA")
      .then_return({{
          "",
          {"PRIVILEGE_TYPE", "IS_GRANTABLE", "TABLE_SCHEMA"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA, TABLE_NAME "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = '\\'create_role\\'@\\'%\\'' "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{
          "",
          {"PRIVILEGE_TYPE", "IS_GRANTABLE", "TABLE_SCHEMA", "TABLE_NAME"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});

  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
          "WHERE GRANTEE = '\\'read_role\\'@\\'%\\''")
      .then_return({{"",
                     {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"SELECT", "NO"}}}});
  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA "
          "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
          "WHERE GRANTEE = '\\'read_role\\'@\\'%\\'' "
          "ORDER BY TABLE_SCHEMA")
      .then_return({{
          "",
          {"PRIVILEGE_TYPE", "IS_GRANTABLE", "TABLE_SCHEMA"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA, TABLE_NAME "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = '\\'read_role\\'@\\'%\\'' "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{
          "",
          {"PRIVILEGE_TYPE", "IS_GRANTABLE", "TABLE_SCHEMA", "TABLE_NAME"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});

  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE "
          "FROM INFORMATION_SCHEMA.USER_PRIVILEGES "
          "WHERE GRANTEE = '\\'write_role\\'@\\'%\\''")
      .then_return({{"",
                     {"PRIVILEGE_TYPE", "IS_GRANTABLE"},
                     {Type::String, Type::String},
                     {{"INSERT", "NO"}, {"UPDATE", "NO"}, {"DELETE", "NO"}}}});
  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA "
          "FROM INFORMATION_SCHEMA.SCHEMA_PRIVILEGES "
          "WHERE GRANTEE = '\\'write_role\\'@\\'%\\'' "
          "ORDER BY TABLE_SCHEMA")
      .then_return({{
          "",
          {"PRIVILEGE_TYPE", "IS_GRANTABLE", "TABLE_SCHEMA"},
          {Type::String, Type::String, Type::String},
          {}  // No Records.
      }});
  m_mock_session
      ->expect_query(
          "SELECT PRIVILEGE_TYPE, IS_GRANTABLE, TABLE_SCHEMA, TABLE_NAME "
          "FROM INFORMATION_SCHEMA.TABLE_PRIVILEGES "
          "WHERE GRANTEE = '\\'write_role\\'@\\'%\\'' "
          "ORDER BY TABLE_SCHEMA, TABLE_NAME")
      .then_return({{
          "",
          {"PRIVILEGE_TYPE", "IS_GRANTABLE", "TABLE_SCHEMA", "TABLE_NAME"},
          {Type::String, Type::String, Type::String, Type::String},
          {}  // No Records.
      }});

  // Test test_user has all expected privileges inherited from its roles:
  // SELECT, INSERT, UPDATE; DELETE, CREATE, ALTER ON *.* (no grant option)
  User_privileges up{mysqlshdk::mysql::Instance(m_session), "test_user", "%"};
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

}  // namespace testing
