/* Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.

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

  User_privileges up{m_session, "notexist_user", "notexist_host"};
  EXPECT_FALSE(up.user_exists());

  auto test = [&up](const std::set<std::string> &tested_privileges,
                    const std::set<std::string> &expected_privileges) {
    SCOPED_TRACE(shcore::str_join(tested_privileges, ", "));

    auto sup = up.validate(tested_privileges);

    EXPECT_FALSE(sup.user_exists());
    EXPECT_EQ(expected_privileges, sup.get_missing_privileges());
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

  User_privileges up{m_session, "test_user", "test_host"};

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

  // Test subset of privileges for *.*, test_db.*, test_db2.*, mysql.*,
  // test_db.t1, test_db.t2, test_db.t3, test_db2.t1 and mysql.user
  std::set<std::string> test_priv = {"SELECT", "Insert", "UPDATE"};

  User_privileges up{m_session, "test_user", "test_host"};
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
    EXPECT_EQ(expected_privileges, sup.get_missing_privileges());
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

  test({"ALL"}, false);
  test({"ALTER",
        "ALTER ROUTINE",
        "CREATE",
        "CREATE ROUTINE",
        "CREATE TABLESPACE",
        "CREATE TEMPORARY TABLES",
        "CREATE USER",
        "CREATE VIEW",
        "DELETE",
        "DROP",
        "EVENT",
        "EXECUTE",
        "FILE",
        "INDEX",
        "LOCK TABLES",
        "PROCESS",
        "REFERENCES",
        "RELOAD",
        "REPLICATION CLIENT",
        "REPLICATION SLAVE",
        "SHOW DATABASES",
        "SHOW VIEW",
        "SHUTDOWN",
        "SUPER",
        "TRIGGER"},
       false, "test_db");
  test({"ALTER",
        "ALTER ROUTINE",
        "CREATE",
        "CREATE ROUTINE",
        "CREATE TABLESPACE",
        "CREATE TEMPORARY TABLES",
        "CREATE USER",
        "CREATE VIEW",
        "DELETE",
        "DROP",
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
        "SHOW DATABASES",
        "SHOW VIEW",
        "SHUTDOWN",
        "SUPER",
        "TRIGGER",
        "UPDATE"},
       true, "test_db2");
  test({"ALL"}, false, "mysql");
  test({"ALTER",
        "ALTER ROUTINE",
        "CREATE",
        "CREATE ROUTINE",
        "CREATE TABLESPACE",
        "CREATE TEMPORARY TABLES",
        "CREATE USER",
        "CREATE VIEW",
        "DROP",
        "EVENT",
        "EXECUTE",
        "FILE",
        "INDEX",
        "LOCK TABLES",
        "PROCESS",
        "REFERENCES",
        "RELOAD",
        "REPLICATION CLIENT",
        "REPLICATION SLAVE",
        "SHOW DATABASES",
        "SHOW VIEW",
        "SHUTDOWN",
        "SUPER",
        "TRIGGER"},
       false, "test_db", "t1");
  test({"ALTER ROUTINE",
        "CREATE",
        "CREATE ROUTINE",
        "CREATE TABLESPACE",
        "CREATE TEMPORARY TABLES",
        "CREATE USER",
        "CREATE VIEW",
        "DELETE",
        "EVENT",
        "EXECUTE",
        "FILE",
        "INDEX",
        "LOCK TABLES",
        "PROCESS",
        "REFERENCES",
        "RELOAD",
        "REPLICATION CLIENT",
        "REPLICATION SLAVE",
        "SHOW DATABASES",
        "SHOW VIEW",
        "SHUTDOWN",
        "SUPER",
        "TRIGGER"},
       true, "test_db", "t2");
  test({"ALTER",
        "ALTER ROUTINE",
        "CREATE",
        "CREATE ROUTINE",
        "CREATE TABLESPACE",
        "CREATE TEMPORARY TABLES",
        "CREATE USER",
        "CREATE VIEW",
        "DELETE",
        "DROP",
        "EVENT",
        "EXECUTE",
        "FILE",
        "INDEX",
        "LOCK TABLES",
        "PROCESS",
        "REFERENCES",
        "RELOAD",
        "REPLICATION CLIENT",
        "REPLICATION SLAVE",
        "SHOW DATABASES",
        "SHOW VIEW",
        "SHUTDOWN",
        "SUPER",
        "TRIGGER"},
       false, "test_db", "t3");
  test({"ALTER",
        "ALTER ROUTINE",
        "CREATE",
        "CREATE ROUTINE",
        "CREATE TABLESPACE",
        "CREATE TEMPORARY TABLES",
        "CREATE USER",
        "CREATE VIEW",
        "DELETE",
        "DROP",
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
        "SHOW DATABASES",
        "SHOW VIEW",
        "SHUTDOWN",
        "SUPER",
        "TRIGGER",
        "UPDATE"},
       true, "test_db2", "t1");
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
                      {"CREATE TABLESPACE", "YES"}}}});
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

  // Test ALL privileges for *.*, test_db.*, test_db2.*, mysql.*, test_db.t1,
  // test_db.t2, test_db.t3, test_db2.t1, and mysql.user
  std::set<std::string> test_priv = {"All"};

  User_privileges up{m_session, "dba_user", "dba_host"};
  EXPECT_TRUE(up.user_exists());

  using User_privileges = mysqlshdk::mysql::User_privileges;
  auto test = [&up, &test_priv](
                  const std::string &schema = User_privileges::k_wildcard,
                  const std::string &table = User_privileges::k_wildcard) {
    SCOPED_TRACE(shcore::str_join(test_priv, ", ") + " on " + schema + "." +
                 table);

    auto sup = up.validate(test_priv, schema, table);

    EXPECT_TRUE(sup.user_exists());
    EXPECT_EQ(std::set<std::string>{}, sup.get_missing_privileges());
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

}  // namespace testing
