/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/util/mod_util.h"
#include "modules/util/upgrade_check.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/test_utils.h"

using Version = mysqlshdk::utils::Version;

namespace mysqlsh {

class MySQL_upgrade_check_test : public Shell_core_test_wrapper {
 public:
  MySQL_upgrade_check_test()
      : opts{_target_server_version, Version(MYSH_VERSION), "", ""} {}

 protected:
  static void SetUpTestCase() {
    auto session = mysqlshdk::db::mysql::Session::create();
  }

  virtual void SetUp() {
    Shell_core_test_wrapper::SetUp();
    if (_target_server_version >= Version(5, 7, 0) ||
        _target_server_version < Version(8, 0, 0)) {
      session = mysqlshdk::db::mysql::Session::create();
      auto connection_options = shcore::get_connection_options(_mysql_uri);
      session->connect(connection_options);
    }
  }

  virtual void TearDown() {
    if (_target_server_version >= Version(5, 7, 0) ||
        _target_server_version < Version(8, 0, 0)) {
      if (!db.empty()) {
        session->execute("drop database if exists " + db);
        db.clear();
      }
      session->close();
    }
    Shell_core_test_wrapper::TearDown();
  }

  void PrepareTestDatabase(std::string name) {
    ASSERT_NO_THROW(session->execute("drop database if exists " + name));
    ASSERT_NO_THROW(
        session->execute("create database " + name + " CHARACTER SET utf8mb3"));
    db = name;
    ASSERT_NO_THROW(session->execute("use " + db));
  }

  void EXPECT_ISSUES(Upgrade_check *check, const int no = -1) {
    try {
      issues = check->run(session, opts);
    } catch (const std::exception &e) {
      puts("Exception runing check:");
      puts(e.what());
      ASSERT_TRUE(false);
    }
    if (no >= 0 && no != static_cast<int>(issues.size())) {
      for (const auto &issue : issues)
        puts(upgrade_issue_to_string(issue).c_str());
      ASSERT_EQ(no, issues.size());
    }
  }

  void EXPECT_NO_ISSUES(Upgrade_check *check) { EXPECT_ISSUES(check, 0); }

  Upgrade_check_options opts;
  std::shared_ptr<mysqlshdk::db::ISession> session;
  std::string db;
  std::vector<mysqlsh::Upgrade_issue> issues;
};

TEST_F(MySQL_upgrade_check_test, checklist_generation) {
  Version current(MYSH_VERSION);
  Version prev(current.get_major(), current.get_minor(),
               current.get_patch() - 1);
  EXPECT_THROW_LIKE(Upgrade_check::create_checklist(Upgrade_check_options{
                        Version("5.7"), Version("5.7"), "", ""}),
                    std::invalid_argument, "This tool supports checking");
  EXPECT_THROW_LIKE(Upgrade_check::create_checklist(Upgrade_check_options{
                        Version("5.6.11"), Version("8.0"), "", ""}),
                    std::invalid_argument, "at least at version 5.7");
  EXPECT_THROW_LIKE(Upgrade_check::create_checklist(Upgrade_check_options{
                        Version("5.7.19"), Version("8.1.0"), "", ""}),
                    std::invalid_argument, "This tool supports checking");
  EXPECT_THROW_LIKE(
      Upgrade_check::create_checklist(
          Upgrade_check_options{current, current, "", ""}),
      std::invalid_argument,
      "MySQL Shell cannot check MySQL server instances for upgrade if they are "
      "at a version the same as or higher than the MySQL Shell version.");
  EXPECT_THROW_LIKE(Upgrade_check::create_checklist(Upgrade_check_options{
                        Version("8.0.12"), Version("8.0.12"), "", ""}),
                    std::invalid_argument, "Target version must be greater");
  EXPECT_NO_THROW(Upgrade_check::create_checklist(
      Upgrade_check_options{Version("5.7.19"), current, "", ""}));
  EXPECT_NO_THROW(Upgrade_check::create_checklist(
      Upgrade_check_options{Version("5.7.17"), Version("8.0"), "", ""}));
  EXPECT_NO_THROW(Upgrade_check::create_checklist(
      Upgrade_check_options{Version("5.7"), Version("8.0.12"), "", ""}));

  std::vector<std::unique_ptr<Upgrade_check>> checks;
  EXPECT_NO_THROW(checks = Upgrade_check::create_checklist(
                      Upgrade_check_options{prev, current, "", ""}));
  // Check for table command is there for every valid version as a last check
  EXPECT_FALSE(checks.empty());
  EXPECT_EQ(0, strcmp("checkTableOutput", checks.back()->get_name()));
}

TEST_F(MySQL_upgrade_check_test, old_temporal) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_old_temporal_check();
  EXPECT_NE(nullptr, check->get_doc_link());
  EXPECT_NO_ISSUES(check.get());
  // No way to create test data in 5.7
}

TEST_F(MySQL_upgrade_check_test, reserved_keywords) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_reserved_keywords_check(opts);
  EXPECT_NE(nullptr, check->get_doc_link());
  EXPECT_NO_ISSUES(check.get());

  PrepareTestDatabase("grouping");
  ASSERT_NO_THROW(
      session->execute("create table System(JSON_TABLE integer, cube int);"));
  ASSERT_NO_THROW(
      session->execute("create trigger first_value AFTER INSERT on System FOR "
                       "EACH ROW delete from Clone where JSON_TABLE<0;"));
  ASSERT_NO_THROW(session->execute("create view LATERAL as select now();"));
  ASSERT_NO_THROW(
      session->execute("create view NTile as select * from System;"));
  ASSERT_NO_THROW(
      session->execute("CREATE FUNCTION Array (s CHAR(20)) RETURNS CHAR(50) "
                       "DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');"));
  ASSERT_NO_THROW(
      session->execute("CREATE FUNCTION rows (s CHAR(20)) RETURNS CHAR(50) "
                       "DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');"));
  ASSERT_NO_THROW(session->execute(
      "CREATE EVENT LEAD ON SCHEDULE AT CURRENT_TIMESTAMP + INTERVAL 1 "
      "HOUR DO UPDATE System SET JSON_TABLE = JSON_TABLE + 1;"));

  EXPECT_ISSUES(check.get(), 12);
  EXPECT_EQ("grouping", issues[0].schema);
  EXPECT_EQ(Upgrade_issue::WARNING, issues[0].level);
  EXPECT_STRCASEEQ("system", issues[1].table.c_str());
  EXPECT_EQ("JSON_TABLE", issues[2].column);
  EXPECT_EQ("cube", issues[3].column);
  // Views columns are also displayed
  EXPECT_EQ("JSON_TABLE", issues[4].column);
  EXPECT_EQ("cube", issues[5].column);
  EXPECT_EQ("first_value", issues[6].table);
  EXPECT_STRCASEEQ("LATERAL", issues[7].table.c_str());
  EXPECT_STRCASEEQ("NTile", issues[8].table.c_str());
  EXPECT_EQ("Array", issues[9].table);
  EXPECT_EQ("rows", issues[10].table);
  EXPECT_EQ("LEAD", issues[11].table);

  check = Sql_upgrade_check::get_reserved_keywords_check(
      Upgrade_check_options{Version(5, 7, 0), Version(8, 0, 11), "", ""});
  ASSERT_NO_THROW(issues = check->run(session, opts));
  ASSERT_EQ(10, issues.size());
}

TEST_F(MySQL_upgrade_check_test, utf8mb3) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("aaaaaaaaaaaaaaaa_utf8mb3");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_utf8mb3_check();
  EXPECT_NE(nullptr, check->get_doc_link());

  session->execute(
      "create table utf83 (s3 varchar(64) charset 'utf8mb3', s4 varchar(64) "
      "charset 'utf8mb4');");

  EXPECT_ISSUES(check.get());
  ASSERT_GE(issues.size(), 2);
  EXPECT_EQ("aaaaaaaaaaaaaaaa_utf8mb3", issues[0].schema);
  EXPECT_EQ("s3", issues[1].column);
  EXPECT_EQ(Upgrade_issue::WARNING, issues[0].level);
}

TEST_F(MySQL_upgrade_check_test, mysql_schema) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_mysql_schema_check();
  EXPECT_NO_ISSUES(check.get());
  EXPECT_NE(nullptr, check->get_doc_link());

  ASSERT_NO_THROW(session->execute("use mysql;"));
  EXPECT_NO_THROW(session->execute("create table Role_edges (i integer);"));
  EXPECT_NO_THROW(session->execute("create table triggers (i integer);"));
  EXPECT_ISSUES(check.get(), 2);
#ifdef _MSC_VER
  EXPECT_EQ("role_edges", issues[0].table);
#else
  EXPECT_EQ("Role_edges", issues[0].table);
#endif
  EXPECT_EQ("triggers", issues[1].table);
  EXPECT_EQ(Upgrade_issue::ERROR, issues[0].level);
  EXPECT_NO_THROW(session->execute("drop table triggers;"));
  EXPECT_NO_THROW(session->execute("drop table Role_edges;"));
}

TEST_F(MySQL_upgrade_check_test, innodb_rowformat) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("test_innodb_rowformat");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_innodb_rowformat_check();
  EXPECT_NO_ISSUES(check.get());

  ASSERT_NO_THROW(session->execute(
      "create table compact (i integer) row_format=compact engine=innodb;"));
  EXPECT_ISSUES(check.get(), 1);
  EXPECT_EQ("compact", issues[0].table);
  EXPECT_EQ(Upgrade_issue::WARNING, issues[0].level);
}

TEST_F(MySQL_upgrade_check_test, zerofill) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("aaa_test_zerofill_nondefaultwidth");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_zerofill_check();
  EXPECT_ISSUES(check.get());
  // some tables in mysql schema use () syntax
  size_t old_count = issues.size();

  ASSERT_NO_THROW(session->execute(
      "create table zero_fill (zf INT zerofill, ti TINYINT(3), tu tinyint(2) "
      "unsigned, si smallint(3), su smallint(3) unsigned, mi mediumint(5), mu "
      "mediumint(5) unsigned, ii INT(4), iu INT(4) unsigned, bi bigint(10), bu "
      "bigint(12) unsigned);"));

  EXPECT_ISSUES(check.get(), 11 + old_count);
  EXPECT_EQ(Upgrade_issue::NOTICE, issues[0].level);
  EXPECT_EQ("zf", issues[0].column);
  EXPECT_EQ("ti", issues[1].column);
  EXPECT_EQ("tu", issues[2].column);
  EXPECT_EQ("si", issues[3].column);
  EXPECT_EQ("su", issues[4].column);
  EXPECT_EQ("mi", issues[5].column);
  EXPECT_EQ("mu", issues[6].column);
  EXPECT_EQ("ii", issues[7].column);
  EXPECT_EQ("iu", issues[8].column);
  EXPECT_EQ("bi", issues[9].column);
  EXPECT_EQ("bu", issues[10].column);
}

TEST_F(MySQL_upgrade_check_test, foreign_key_length) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_foreign_key_length_check();
  EXPECT_NE(nullptr, check->get_doc_link());
  EXPECT_NO_ISSUES(check.get());
  // No way to prepare test data in 5.7
}

TEST_F(MySQL_upgrade_check_test, maxdb_sqlmode) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("aaa_test_maxdb_sql_mode");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_maxdb_sql_mode_flags_check();
  EXPECT_NE(nullptr, check->get_doc_link());
  EXPECT_NO_ISSUES(check.get());

  ASSERT_NO_THROW(
      session->execute("create table Clone(COMPONENT integer, cube int);"));

  auto issues_count = issues.size();
  ASSERT_NO_THROW(session->execute("SET SESSION sql_mode = 'MAXDB';"));
  ASSERT_NO_THROW(session->execute(
      "CREATE FUNCTION TEST_MAXDB (s CHAR(20)) RETURNS CHAR(50) "
      "DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');"));
  ASSERT_NO_THROW(issues = check->run(session, opts));
  ASSERT_GT(issues.size(), issues_count);
  issues_count = issues.size();
  issues.clear();
  ASSERT_NO_THROW(
      session->execute("create trigger TR_MAXDB AFTER INSERT on Clone FOR "
                       "EACH ROW delete from Clone where COMPONENT<0;"));
  ASSERT_NO_THROW(issues = check->run(session, opts));
  ASSERT_GT(issues.size(), issues_count);
  issues_count = issues.size();
  issues.clear();
  ASSERT_NO_THROW(
      session->execute("CREATE EVENT EV_MAXDB ON SCHEDULE AT CURRENT_TIMESTAMP "
                       "+ INTERVAL 1 HOUR "
                       "DO UPDATE Clone SET COMPONENT = COMPONENT + 1;"));
  ASSERT_NO_THROW(issues = check->run(session, opts));
  ASSERT_GT(issues.size(), issues_count);
}

TEST_F(MySQL_upgrade_check_test, obsolete_sqlmodes) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("aaa_test_obsolete_sql_modes");
  ASSERT_NO_THROW(
      session->execute("create table Clone(COMPONENT integer, cube int);"));
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_obsolete_sql_mode_flags_check();
  EXPECT_NE(nullptr, check->get_doc_link());
  // NO_AUTO_CREATE_USER is in default sql_mode in 5.7 so its expected that we
  // would get a lot of issues here
  EXPECT_ISSUES(check.get());

  std::vector<std::string> modes = {"DB2",
                                    "MSSQL",
                                    "MYSQL323",
                                    "MYSQL40",
                                    "NO_AUTO_CREATE_USER",
                                    "NO_FIELD_OPTIONS",
                                    "NO_KEY_OPTIONS",
                                    "NO_TABLE_OPTIONS",
                                    "ORACLE",
                                    "POSTGRESQL"};

  for (const std::string &mode : modes) {
    const auto issues_count = issues.size();
    ASSERT_NO_THROW(session->execute(
        shcore::str_format("SET SESSION sql_mode = '%s';", mode.c_str())));
    ASSERT_NO_THROW(session->execute(shcore::str_format(
        "CREATE FUNCTION TEST_%s (s CHAR(20)) RETURNS CHAR(50) "
        "DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');",
        mode.c_str())));
    ASSERT_NO_THROW(session->execute(
        shcore::str_format("create trigger TR_%s AFTER INSERT on Clone FOR "
                           "EACH ROW delete from Clone where COMPONENT<0;",
                           mode.c_str())));
    ASSERT_NO_THROW(session->execute(
        shcore::str_format("CREATE EVENT EV_%s ON SCHEDULE AT "
                           "CURRENT_TIMESTAMP + INTERVAL 1 HOUR "
                           "DO UPDATE Clone SET COMPONENT = COMPONENT + 1;",
                           mode.c_str())));
    ASSERT_NO_THROW(issues = check->run(session, opts));
    ASSERT_GE(issues.size(), issues_count + 3);
  }
}

TEST_F(MySQL_upgrade_check_test, enum_set_element_length) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("aaa_test_enum_set_element_length");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_enum_set_element_length_check();
  EXPECT_EQ(
      0,
      strcmp(
          "https://dev.mysql.com/doc/refman/8.0/en/string-type-overview.html",
          check->get_doc_link()));
  ASSERT_NO_THROW(issues = check->run(session, opts));
  std::size_t original = issues.size();

  ASSERT_NO_THROW(session->execute(
      "CREATE TABLE large_enum (e enum('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
      "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
      "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
      "eeeeee'));"));

  ASSERT_NO_THROW(session->execute(
      "CREATE TABLE not_large_enum (e enum('aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
      "ccccccccccccccccccccccccccccccccc','cccccccccccccccccccccccccccccccccccc"
      "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
      "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
      "eeeeee', \"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz\"));"));

  ASSERT_NO_THROW(session->execute(
      "CREATE TABLE large_set (s set('a', 'zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
      "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy"
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
      "wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
      "vvvvvvvvvv', 'b', 'c'));"));

  ASSERT_NO_THROW(session->execute(
      "CREATE TABLE not_so_large (s set('a', 'zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
      "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy"
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
      "vvvvvvvvvv', 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb', 'b', 'c'));"));

  EXPECT_ISSUES(check.get(), original + 2);
  EXPECT_EQ(issues[0].table, "large_enum");
  EXPECT_EQ(issues[1].table, "large_set");
}

TEST_F(MySQL_upgrade_check_test, partitioned_tables_in_shared_tablespaces) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 13))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("aaa_test_partitioned_in_shared");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_partitioned_tables_in_shared_tablespaces_check(
          opts);
  EXPECT_NO_ISSUES(check.get());
  EXPECT_EQ(0, strcmp("https://dev.mysql.com/doc/refman/8.0/en/"
                      "mysql-nutshell.html#mysql-nutshell-removals",
                      check->get_doc_link()));

  EXPECT_NO_THROW(session->execute(
      "CREATE TABLESPACE tpists ADD DATAFILE 'tpists.ibd' ENGINE=INNODB;"));
  EXPECT_NO_THROW(session->execute(
      "create table part(i integer) TABLESPACE tpists partition "
      "by range(i) (partition p0 values less than (1000), "
      "partition p1 values less than MAXVALUE);"));
  EXPECT_ISSUES(check.get(), 2);
  EXPECT_NO_THROW(session->execute("drop table part"));
  EXPECT_NO_THROW(session->execute("drop tablespace tpists"));
}

TEST_F(MySQL_upgrade_check_test, circular_directory_reference) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 17))
    SKIP_TEST(
        "This test requires running against MySQL server version 5.7-8.0.16");
  PrepareTestDatabase("aaa_circular_directory");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_circular_directory_check();
  EXPECT_EQ(0,
            strcmp("https://dev.mysql.com/doc/refman/8.0/en/"
                   "upgrading-from-previous-series.html#upgrade-innodb-changes",
                   check->get_doc_link()));
  EXPECT_NO_ISSUES(check.get());

  EXPECT_NO_THROW(
      session->execute("CREATE TABLESPACE circular ADD DATAFILE "
                       "'mysql/../circular.ibd' ENGINE=INNODB;"));
  EXPECT_ISSUES(check.get(), 1);
  EXPECT_EQ("circular", issues[0].schema);
  EXPECT_NO_THROW(session->execute("drop tablespace circular"));
}

TEST_F(MySQL_upgrade_check_test, removed_functions) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("aaa_test_removed_functions");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_removed_functions_check();
  EXPECT_NE(nullptr, check->get_doc_link());
  EXPECT_NO_ISSUES(check.get());

  ASSERT_NO_THROW(session->execute(
      "create table geotab1 (col1 int ,col2 geometry,col3 geometry, col4 int "
      "generated always as (contains(col2,col3)));"));

  ASSERT_NO_THROW(
      session->execute("create view touch_view as select *, "
                       "TOUCHES(`col2`,`col3`) from geotab1;"));

  ASSERT_NO_THROW(session->execute(
      "create trigger contr AFTER INSERT on geotab1 FOR EACH ROW delete from \n"
      "-- This is a test NUMGEOMETRIES ()\n"
      "# This is a test GLENGTH()\n"
      "geotab1 where TOUCHES(`col2`,`col3`);"));
  ASSERT_NO_THROW(session->execute(
      "create procedure contains_proc(p1 geometry,p2 geometry) begin select "
      "col1, 'Y()' from tab1 where col2=@p1 and col3=@p2 and contains(p1,p2) "
      "and TOUCHES(p1, p2);\n"
      "-- This is a test NUMGEOMETRIES ()\n"
      "# This is a test GLENGTH()\n"
      "/* just a comment X() */end;"));
  ASSERT_NO_THROW(session->execute(
      "create function test_astext() returns TEXT deterministic return "
      "PASSWORD(AsText('MULTIPOINT(1 1, 2 2, 3 3)'));"));
  ASSERT_NO_THROW(
      session->execute("create function test_enc() returns text deterministic "
                       "return encrypt('123');"));

  ASSERT_NO_THROW(
      session->execute("create event e_contains ON SCHEDULE AT "
                       "CURRENT_TIMESTAMP + INTERVAL 1 HOUR "
                       "DO select contains(col2,col3) from geotab1;"));
  // Unable to test generated columns and views as at least in 5.7.26 they are
  // automatically converted to supported functions
  EXPECT_ISSUES(check.get(), 5);
  EXPECT_NE(std::string::npos, issues[0].description.find("CONTAINS"));
  EXPECT_NE(std::string::npos,
            issues[0].description.find("consider using MBRCONTAINS"));
  EXPECT_NE(std::string::npos, issues[0].description.find("TOUCHES"));
  EXPECT_NE(std::string::npos, issues[0].description.find("PROCEDURE"));
  EXPECT_NE(std::string::npos,
            issues[0].description.find("ST_TOUCHES instead"));
  EXPECT_NE(std::string::npos, issues[1].description.find("PASSWORD"));
  EXPECT_NE(std::string::npos, issues[1].description.find("ASTEXT"));
  EXPECT_NE(std::string::npos, issues[1].description.find("ST_ASTEXT"));
  EXPECT_NE(std::string::npos, issues[1].description.find("FUNCTION"));
  EXPECT_NE(std::string::npos, issues[2].description.find("ENCRYPT"));
  EXPECT_NE(std::string::npos, issues[2].description.find("SHA2"));
  EXPECT_NE(std::string::npos, issues[2].description.find("FUNCTION"));
  EXPECT_NE(std::string::npos, issues[3].description.find("TOUCHES"));
  EXPECT_NE(std::string::npos, issues[3].description.find("ST_TOUCHES"));
  EXPECT_NE(std::string::npos, issues[4].description.find("CONTAINS"));
  EXPECT_NE(std::string::npos, issues[4].description.find("MBRCONTAINS"));
  EXPECT_NE(std::string::npos, issues[4].description.find("EVENT"));
}

TEST_F(MySQL_upgrade_check_test, groupby_asc_desc_syntax) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 13))
    SKIP_TEST(
        "This test requires running against MySQL server version 5.7-8.0.12");
  PrepareTestDatabase("aaa_test_group_by_asc");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_groupby_asc_syntax_check();
  EXPECT_EQ(0, strcmp("https://dev.mysql.com/doc/relnotes/mysql/8.0/en/"
                      "news-8-0-13.html#mysqld-8-0-13-sql-syntax",
                      check->get_doc_link()));
  EXPECT_NO_ISSUES(check.get());

  ASSERT_NO_THROW(
      session->execute("create table movies (title varchar(100), genre "
                       "varchar(100), year_produced Year);"));
  ASSERT_NO_THROW(
      session->execute("create table genre_summary (genre varchar(100), count "
                       "int, time timestamp);"));
  ASSERT_NO_THROW(session->execute(
      "create view genre_ob as select genre, count(*), year_produced from "
      "movies group by genre, year_produced order by year_produced desc;"));

  ASSERT_NO_THROW(session->execute(
      "create view genre_desc as select genre, count(*), year_produced from "
      "movies group\n/*comment*/by genre\ndesc;"));

  ASSERT_NO_THROW(session->execute(
      "create trigger genre_summary_asc AFTER INSERT on movies for each row "
      "INSERT INTO genre_summary (genre, count, time) select genre, count(*), "
      "now() from movies group/* psikus */by genre\nasc;"));
  ASSERT_NO_THROW(session->execute(
      "create trigger genre_summary_desc AFTER INSERT on movies for each row "
      "INSERT INTO genre_summary (genre, count, time) select genre, count(*), "
      "now() from movies group\nby genre# tralala\ndesc;"));
  ASSERT_NO_THROW(session->execute(
      "create trigger genre_summary_ob AFTER INSERT on movies for each row "
      "INSERT INTO genre_summary (genre, count, time) select genre, count(*), "
      "now() from movies group by genre order by genre asc;"));

  ASSERT_NO_THROW(
      session->execute("create procedure list_genres_asc() select genre, "
                       "count(*), 'group by desc' from movies group by genre\n"
                       "-- This is a test order ()\n"
                       "# This is a test order\n"
                       "/* just a comment order */asc;"));
  ASSERT_NO_THROW(
      session->execute("create procedure list_genres_desc() select genre, "
                       "\"group by asc\", "
                       "count(*) from movies group# psikus\nby genre\tdesc;"));
  ASSERT_NO_THROW(session->execute(
      "create procedure list_genres_ob() select genre, count(*) from movies "
      "group by genre order/* group */by genre desc;"));

  ASSERT_NO_THROW(session->execute(
      "create event mov_sec ON SCHEDULE AT CURRENT_TIMESTAMP + INTERVAL 1 HOUR "
      "DO select * from movies group by genre desc;"));

  EXPECT_ISSUES(check.get(), 6);
  EXPECT_EQ("genre_desc", issues[0].table);
  EXPECT_TRUE(shcore::str_beginswith(issues[0].description, "VIEW"));
  EXPECT_EQ("list_genres_asc", issues[1].table);
  EXPECT_TRUE(shcore::str_beginswith(issues[1].description, "PROCEDURE"));
  EXPECT_EQ("list_genres_desc", issues[2].table);
  EXPECT_TRUE(shcore::str_beginswith(issues[2].description, "PROCEDURE"));
  EXPECT_EQ("genre_summary_asc", issues[3].table);
  EXPECT_TRUE(shcore::str_beginswith(issues[3].description, "TRIGGER"));
  EXPECT_EQ("genre_summary_desc", issues[4].table);
  EXPECT_TRUE(shcore::str_beginswith(issues[4].description, "TRIGGER"));
  EXPECT_EQ("mov_sec", issues[5].table);
  EXPECT_TRUE(shcore::str_beginswith(issues[5].description, "EVENT"));
}

TEST_F(MySQL_upgrade_check_test, removed_sys_log_vars) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 13))
    SKIP_TEST(
        "This test requires running against MySQL server version 5.7-8.0.13");

  std::unique_ptr<Upgrade_check> check =
      Sql_upgrade_check::get_removed_sys_log_vars_check(opts);
  EXPECT_EQ(0, strcmp("https://dev.mysql.com/doc/relnotes/mysql/8.0/en/"
                      "news-8-0-13.html#mysqld-8-0-13-logging",
                      check->get_doc_link()));

  if (_target_server_version < Version(8, 0, 0)) {
    EXPECT_THROW_LIKE(
        check->run(session, opts), Upgrade_check::Check_configuration_error,
        "To run this check requires full path to MySQL server configuration "
        "file to be specified at 'configPath' key of options dictionary");
  } else {
    EXPECT_NO_ISSUES(check.get());
  }
}

extern "C" const char *g_test_home;

TEST_F(MySQL_upgrade_check_test, configuration_check) {
  Config_check defined("test",
                       {{"basedir", "homedir"},
                        {"option_to_drop_with_no_value", nullptr},
                        {"not_existing_var", nullptr},
                        {"again_not_there", "personalized msg"}},
                       Config_check::Mode::FLAG_DEFINED, Upgrade_issue::NOTICE,
                       "problem");
  ASSERT_THROW(defined.run(session, opts),
               Upgrade_check::Check_configuration_error);

  opts.config_path.assign(
      shcore::path::join_path(g_test_home, "data", "config", "my.cnf"));
  EXPECT_ISSUES(&defined, 2);
  EXPECT_EQ("option_to_drop_with_no_value", issues[0].schema);
  EXPECT_EQ(Upgrade_issue::NOTICE, issues[0].level);
  EXPECT_EQ("problem", issues[0].description);
  EXPECT_EQ("basedir", issues[1].schema);
  EXPECT_NE(std::string::npos, issues[1].description.find("homedir"));

  Config_check undefined("test",
                         {{"basedir", "homedir"},
                          {"option_to_drop_with_no_value", nullptr},
                          {"not_existing_var", nullptr},
                          {"again_not_there", "personalized msg"}},
                         Config_check::Mode::FLAG_UNDEFINED,
                         Upgrade_issue::WARNING, "undefined");
  EXPECT_ISSUES(&undefined, 2);
  EXPECT_EQ("again_not_there", issues[0].schema);
  EXPECT_NE(std::string::npos, issues[0].description.find("personalized msg"));
  EXPECT_EQ("not_existing_var", issues[1].schema);
  EXPECT_EQ(Upgrade_issue::WARNING, issues[1].level);
  EXPECT_EQ("undefined", issues[1].description);

  opts.config_path.clear();
}

TEST_F(MySQL_upgrade_check_test, removed_sys_vars) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 13))
    SKIP_TEST(
        "This test requires running against MySQL server version 5.7-8.0.13");

  std::unique_ptr<Upgrade_check> check =
      Sql_upgrade_check::get_removed_sys_vars_check(opts);
  EXPECT_EQ(0, strcmp("https://dev.mysql.com/doc/refman/8.0/en/"
                      "added-deprecated-removed.html#optvars-removed",
                      check->get_doc_link()));

  if (_target_server_version < Version(8, 0, 0)) {
    EXPECT_THROW_LIKE(
        check->run(session, opts), Upgrade_check::Check_configuration_error,
        "To run this check requires full path to MySQL server configuration "
        "file to be specified at 'configPath' key of options dictionary");
    opts.config_path.assign(
        shcore::path::join_path(g_test_home, "data", "config", "uc.cnf"));
    EXPECT_ISSUES(check.get(), 2);
    opts.config_path.clear();
    EXPECT_EQ("max_tmp_tables", issues[0].schema);
    EXPECT_EQ("query-cache-type", issues[1].schema);
  } else {
    EXPECT_NO_ISSUES(check.get());
  }
}

TEST_F(MySQL_upgrade_check_test, sys_vars_new_defaults) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");

  std::unique_ptr<Upgrade_check> check =
      Sql_upgrade_check::get_sys_vars_new_defaults_check();
  EXPECT_EQ(0, strcmp("https://mysqlserverteam.com/new-defaults-in-mysql-8-0/",
                      check->get_doc_link()));

  EXPECT_THROW_LIKE(
      check->run(session, opts), Upgrade_check::Check_configuration_error,
      "To run this check requires full path to MySQL server configuration "
      "file to be specified at 'configPath' key of options dictionary");
  opts.config_path.assign(
      shcore::path::join_path(g_test_home, "data", "config", "my.cnf"));
  EXPECT_ISSUES(check.get(), 25);
  EXPECT_EQ("back_log", issues[0].schema);
  EXPECT_EQ("transaction_write_set_extraction", issues.back().schema);
  opts.config_path.clear();
}

TEST_F(MySQL_upgrade_check_test, schema_inconsitencies) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");

  // Preparing data for this check requires manipulating datadir by hand, we
  // only check here that queries run fine
  auto check = Sql_upgrade_check::get_schema_inconsistency_check();

  // Make sure special characters like hyphen are handled well
  PrepareTestDatabase("schema_inconsitencies_test");
  EXPECT_NO_THROW(session->execute("create table `!@#$%&*-_.:?` (i integer);"));

  // Make sure partitioned tables do not get positively flagged by accident
  EXPECT_NO_THROW(session->execute(
      "create table t(a datetime(5) not null) engine=innodb default "
      "charset=latin1 row_format=dynamic partition by range columns(a) "
      "(partition p0 values less than ('2019-01-23 16:59:53'), partition p1 "
      "values less than ('2019-02-22 10:17:03'), partition p2 values less than "
      "(maxvalue));"));

  EXPECT_NO_ISSUES(check.get());
}

TEST_F(MySQL_upgrade_check_test, non_native_partitioning) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("mysql_non_native_partitioning");
  auto check = Sql_upgrade_check::get_nonnative_partitioning_check();
  EXPECT_EQ(0, strcmp("https://dev.mysql.com/doc/refman/8.0/en/"
                      "upgrading-from-previous-series.html#upgrade-"
                      "configuration-changes",
                      check->get_doc_link()));
  EXPECT_NO_ISSUES(check.get());

  ASSERT_NO_THROW(
      session->execute("create table part(i integer) engine=myisam partition "
                       "by range(i) (partition p0 values less than (1000), "
                       "partition p1 values less than MAXVALUE);"));
  EXPECT_ISSUES(check.get(), 1);
  EXPECT_EQ("part", issues[0].table);
}

TEST_F(MySQL_upgrade_check_test, fts_tablename_check) {
  const auto res = session->query("select UPPER(@@version_compile_os);");
  const auto compile_os = res->fetch_one()->get_string(0);
#if defined(WIN32)
  EXPECT_THROW(
      Sql_upgrade_check::get_fts_in_tablename_check(Upgrade_check_options{
          Version(5, 7, 25), Version(8, 0, 17), compile_os.c_str(), ""}),
      Upgrade_check::Check_not_needed);
#else
  PrepareTestDatabase("fts_tablename");
  EXPECT_THROW(Sql_upgrade_check::get_fts_in_tablename_check(opts),
               Upgrade_check::Check_not_needed);
  auto check =
      Sql_upgrade_check::get_fts_in_tablename_check(Upgrade_check_options{
          Version(5, 7, 25), Version(8, 0, 17), compile_os.c_str(), ""});
  EXPECT_NO_ISSUES(check.get());

  ASSERT_NO_THROW(session->execute(
      "CREATE TABLE `DP01_FTS_REGULATION_ACTION_HIST` ( `HJID` bigint(20) NOT "
      "NULL, `VERSION` bigint(20) NOT NULL, PRIMARY KEY (`HJID`,`VERSION`));"));
#ifndef __APPLE__
  ASSERT_NO_THROW(session->execute(
      "create table `DP01_FtS_REGULATION_ACTION_HIST` (i integer);"));
#endif

  EXPECT_ISSUES(check.get(), 1);
  EXPECT_EQ("DP01_FTS_REGULATION_ACTION_HIST", issues[0].table);
#endif
}

TEST_F(MySQL_upgrade_check_test, check_table_command) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("mysql_check_table_test");
  Check_table_command check;
  EXPECT_NO_ISSUES(&check);

  ASSERT_NO_THROW(
      session->execute("create table part(i integer) engine=myisam partition "
                       "by range(i) (partition p0 values less than (1000), "
                       "partition p1 values less than MAXVALUE);"));
  EXPECT_NO_ISSUES(&check);
}

TEST_F(MySQL_upgrade_check_test, zero_dates_check) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("mysql_zero_dates_check_test");
  auto check = Sql_upgrade_check::get_zero_dates_check();
  EXPECT_EQ(0, strcmp("https://lefred.be/content/mysql-8-0-and-wrong-dates/",
                      check->get_doc_link()));
  EXPECT_NO_ISSUES(check.get());

  ASSERT_NO_THROW(
      session->execute("set @@session.sql_mode = "
                       "'ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,ERROR_FOR_"
                       "DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION';"));
  ASSERT_NO_THROW(session->execute(
      "create table dt (i integer, dt datetime default '0000-00-00', ts "
      "timestamp default '0000-00-00', d date default '0000-00-00');"));
  EXPECT_ISSUES(check.get(), 4);
  EXPECT_NE(std::string::npos,
            issues[0].description.find("1 session(s) does not contain either"));
  EXPECT_EQ("dt", issues[1].column);
  EXPECT_EQ("ts", issues[2].column);
  EXPECT_EQ("d", issues[3].column);
}

TEST_F(MySQL_upgrade_check_test, engine_mixup_check) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  auto check = Sql_upgrade_check::get_engine_mixup_check();
  EXPECT_NE(nullptr, strstr(check->get_description(),
                            "Rename the MyISAM table to a temporary name"));
  EXPECT_NO_ISSUES(check.get());

  // positive test cases performed manually as it requires manual changes to the
  // datadir files
}

TEST_F(MySQL_upgrade_check_test, manual_checks) {
  auto manual = Upgrade_check::create_checklist(
      Upgrade_check_options{Version("5.7.0"), Version("8.0.11"), "", ""});
  manual.erase(std::remove_if(manual.begin(), manual.end(),
                              [](const std::unique_ptr<Upgrade_check> &c) {
                                return c->is_runnable();
                              }),
               manual.end());
  ASSERT_EQ(1, manual.size());

  auto auth = dynamic_cast<Manual_check *>(manual[0].get());
  ASSERT_NE(nullptr, auth);
  ASSERT_EQ(0, strcmp(auth->get_name(), "defaultAuthenticationPlugin"));
  ASSERT_EQ(0, strcmp(auth->get_title(),
                      "New default authentication plugin considerations"));
  ASSERT_EQ(Upgrade_issue::WARNING, auth->get_level());
  ASSERT_NE(nullptr, strstr(auth->get_doc_link(),
                            "https://dev.mysql.com/doc/refman/8.0/en/"
                            "upgrading-from-previous-series.html#upgrade-"
                            "caching-sha2-password-compatibility-issues"));
  ASSERT_NE(nullptr,
            strstr(auth->get_description(),
                   "Warning: The new default authentication plugin "
                   "'caching_sha2_password' offers more secure password "
                   "hashing than previously used 'mysql_native_password' (and "
                   "consequent improved client connection authentication)."));
}

TEST_F(MySQL_upgrade_check_test, corner_cases_of_upgrade_check) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  _interactive_shell->connect(
      mysqlsh::get_connection_options(shcore::Value(_mysql_uri)));
  Util util(_interactive_shell->shell_context().get());
  shcore::Argument_list args;

  // valid mysql 5.7 superuser
  args.push_back(shcore::Value(_mysql_uri));
  try {
    util.check_for_server_upgrade(args);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    EXPECT_TRUE(false);
  }
  args.clear();

  // valid mysql 5.7 superuser X protocol
  args.push_back(shcore::Value(_uri));
  EXPECT_NO_THROW(util.check_for_server_upgrade(args));
  args.clear();

  // new user with required privileges sans grant option and '%' in host
  EXPECT_NO_THROW(session->execute(
      "create user if not exists 'percent'@'%' identified by 'percent';"));
  std::string percent_uri = _mysql_uri;
  percent_uri.replace(0, 4, "percent:percent");
  args.push_back(shcore::Value(percent_uri));
  // No privileges function should throw
  EXPECT_THROW(util.check_for_server_upgrade(args), shcore::Exception);

  // Still not enough privileges
  EXPECT_NO_THROW(session->execute("grant SELECT on *.* to 'percent'@'%';"));
  EXPECT_THROW(util.check_for_server_upgrade(args), shcore::Exception);

  // Privileges check out we should succeed
  EXPECT_NO_THROW(
      session->execute("grant RELOAD, PROCESS on *.* to 'percent'@'%';"));
  EXPECT_NO_THROW(util.check_for_server_upgrade(args));

  EXPECT_NO_THROW(session->execute("drop user 'percent'@'%';"));

  // Using default session to run checks
  args.clear();
  EXPECT_NO_THROW(util.check_for_server_upgrade(args));

  // Using default session with options dictionary
  auto op = std::make_shared<shcore::Value::Map_type>();
  op->set("targetVersion", shcore::Value("8.0.18"));
  args.push_back(shcore::Value(op));
  EXPECT_NO_THROW(util.check_for_server_upgrade(args));

  // Wrong options
  op->set("blah", shcore::Value("whatever"));
  EXPECT_THROW(util.check_for_server_upgrade(args), shcore::Exception);

  // Wrong type of parameters
  args.clear();
  args.push_back(shcore::Value(_mysql_uri));
  args.push_back(shcore::Value("whatever"));
  EXPECT_THROW_LIKE(util.check_for_server_upgrade(args), shcore::Exception,
                    "Argument #2 is expected to be a map.");
}

TEST_F(MySQL_upgrade_check_test, JSON_output_format) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(MYSH_VERSION))
    SKIP_TEST(
        "This test requires running against MySQL server version 5.7 up to but "
        "not including " MYSH_VERSION);
  Util util(_interactive_shell->shell_context().get());
  shcore::Argument_list args;

  // valid mysql 5.7 superuser
  args.push_back(shcore::Value(_mysql_uri));
  const auto options = shcore::make_dict();
  options->set("outputFormat", shcore::Value("JSON"));
  args.push_back(shcore::Value(options));
  try {
    util.check_for_server_upgrade(args);
    rapidjson::Document d;
    d.Parse(output_handler.std_out.c_str());

    ASSERT_FALSE(d.HasParseError());
    ASSERT_TRUE(d.IsObject());
    ASSERT_TRUE(d.HasMember("serverAddress"));
    ASSERT_TRUE(d["serverAddress"].IsString());
    ASSERT_TRUE(d.HasMember("serverVersion"));
    ASSERT_TRUE(d["serverVersion"].IsString());
    ASSERT_TRUE(d.HasMember("targetVersion"));
    ASSERT_TRUE(d["targetVersion"].IsString());
    ASSERT_TRUE(d.HasMember("errorCount"));
    ASSERT_TRUE(d["errorCount"].IsInt());
    ASSERT_TRUE(d.HasMember("warningCount"));
    ASSERT_TRUE(d["warningCount"].IsInt());
    ASSERT_TRUE(d.HasMember("noticeCount"));
    ASSERT_TRUE(d["noticeCount"].IsInt());
    ASSERT_TRUE(d.HasMember("summary"));
    ASSERT_TRUE(d["summary"].IsString());
    ASSERT_TRUE(d.HasMember("checksPerformed"));
    ASSERT_TRUE(d["checksPerformed"].IsArray());
    auto checks = d["checksPerformed"].GetArray();
    ASSERT_GE(checks.Size(), 1);
    for (rapidjson::SizeType i = 0; i < checks.Size(); i++) {
      ASSERT_TRUE(checks[i].IsObject());
      ASSERT_TRUE(checks[i].HasMember("id"));
      ASSERT_TRUE(checks[i]["id"].IsString());
      ASSERT_TRUE(checks[i].HasMember("title"));
      ASSERT_TRUE(checks[i]["title"].IsString());
      ASSERT_TRUE(checks[i].HasMember("status"));
      ASSERT_TRUE(checks[i]["status"].IsString());
      if (checks[i].HasMember("documentationLink")) {
        ASSERT_TRUE(checks[i]["documentationLink"].IsString());
      }
      if (strcmp(checks[i]["status"].GetString(), "OK") == 0) {
        ASSERT_TRUE(checks[i].HasMember("detectedProblems"));
        ASSERT_TRUE(checks[i]["detectedProblems"].IsArray());
        auto problems = checks[i]["detectedProblems"].GetArray();
        if (problems.Size() == 0) {
          EXPECT_FALSE(checks[i].HasMember("documentationLink"));
          EXPECT_FALSE(checks[i].HasMember("description"));
        }
        for (rapidjson::SizeType j = 0; j < problems.Size(); j++) {
          ASSERT_TRUE(problems[j].IsObject());
          ASSERT_TRUE(problems[j].HasMember("level"));
          ASSERT_TRUE(problems[j]["level"].IsString());
          ASSERT_TRUE(problems[j].HasMember("dbObject"));
          ASSERT_TRUE(problems[j]["dbObject"].IsString());
        }
      } else {
        ASSERT_TRUE(checks[i].HasMember("description"));
        ASSERT_TRUE(checks[i]["description"].IsString());
      }
    }
    ASSERT_TRUE(d.HasMember("manualChecks"));
    ASSERT_TRUE(d["manualChecks"].IsArray());
    auto manual = d["manualChecks"].GetArray();
    if (_target_server_version < Version(8, 0, 0)) {
      ASSERT_GT(manual.Size(), 0);
    }
    for (rapidjson::SizeType i = 0; i < manual.Size(); i++) {
      ASSERT_TRUE(manual[i].IsObject());
      ASSERT_TRUE(manual[i].HasMember("id"));
      ASSERT_TRUE(manual[i]["id"].IsString());
      ASSERT_TRUE(manual[i].HasMember("title"));
      ASSERT_TRUE(manual[i]["title"].IsString());
      ASSERT_TRUE(manual[i].HasMember("description"));
      ASSERT_TRUE(manual[i]["description"].IsString());
      if (manual[i].HasMember("documentationLink")) {
        ASSERT_TRUE(manual[i]["documentationLink"].IsString());
      }
    }

    // Test if valid JSON is produced when JSON wrapping is enabled
    wipe_all();
    EXPECT_NE("json/raw", _options->wrap_json);
    auto old_wrap = _options->wrap_json;
    _options->wrap_json = "json/raw";
    args.pop_back();
    util.check_for_server_upgrade(args);
    _options->wrap_json = old_wrap;
    rapidjson::Document d1;
    d1.Parse(output_handler.std_out.c_str());
    ASSERT_FALSE(d.HasParseError());
    ASSERT_TRUE(d.IsObject());
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    EXPECT_TRUE(false);
  }
  args.clear();
}

TEST_F(MySQL_upgrade_check_test, server_version_not_supported) {
  Version shell_version(MYSH_VERSION);
  // session established with 8.0 server
  if (_target_server_version < Version(8, 0, 0) ||
      _target_server_version < shell_version)
    SKIP_TEST(
        "This test requires running against MySQL server version 8.0, equal "
        "or greater than the shell version");
  Util util(_interactive_shell->shell_context().get());
  shcore::Argument_list args;
  args.push_back(shcore::Value(_mysql_uri));
  EXPECT_THROW(util.check_for_server_upgrade(args), shcore::Exception);
}

TEST_F(MySQL_upgrade_check_test, password_prompted) {
  Util util(_interactive_shell->shell_context().get());
  shcore::Argument_list args;
  args.push_back(shcore::Value(_mysql_uri_nopasswd));

  output_handler.passwords.push_back(
      {"Please provide the password for "
       "'" +
           _mysql_uri_nopasswd + "': ",
       "WhAtEvEr"});
  EXPECT_THROW(util.check_for_server_upgrade(args), shcore::Exception);

  // Passwords are consumed if prompted, so verifying this indicates the
  // password was prompted as expected and consumed
  EXPECT_TRUE(output_handler.passwords.empty());
}

TEST_F(MySQL_upgrade_check_test, password_no_prompted) {
  Util util(_interactive_shell->shell_context().get());
  shcore::Argument_list args;
  args.push_back(shcore::Value(_mysql_uri));

  output_handler.passwords.push_back(
      {"If this was prompted it is an error", "WhAtEvEr"});

  try {
    util.check_for_server_upgrade(args);
  } catch (...) {
    // We don't really care for this test
  }

  // Passwords are consumed if prompted, so verifying this indicates the
  // password was NOT prompted as expected and so, NOT consumed
  EXPECT_FALSE(output_handler.passwords.empty());
  output_handler.passwords.clear();
}

TEST_F(MySQL_upgrade_check_test, password_no_promptable) {
  _options->wizards = false;
  reset_shell();
  Util util(_interactive_shell->shell_context().get());
  shcore::Argument_list args;
  args.push_back(shcore::Value(_mysql_uri_nopasswd));

  output_handler.passwords.push_back(
      {"If this was prompted it is an error", "WhAtEvEr"});

  try {
    util.check_for_server_upgrade(args);
  } catch (...) {
    // We don't really care for this test
  }

  // Passwords are consumed if prompted, so verifying this indicates the
  // password was NOT prompted as expected and so, NOT consumed
  EXPECT_FALSE(output_handler.passwords.empty());
  output_handler.passwords.clear();
}

TEST_F(MySQL_upgrade_check_test, GTID_EXECUTED_unchanged) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  int sb_port = 0;
  for (auto port : _mysql_sandbox_ports) {
    try {
      testutil->deploy_sandbox(port, "root");
      sb_port = port;
      break;
    } catch (const std::runtime_error &err) {
      std::cerr << err.what();
      testutil->destroy_sandbox(port, true);
    }
  }

  ASSERT_NE(0, sb_port);
  try {
    std::string uri = "root:root@localhost:" + std::to_string(sb_port);
    auto s = mysqlshdk::db::mysql::Session::create();
    s->connect(shcore::get_connection_options(uri));
    EXPECT_NO_THROW(s->execute("set @@global.sql_mode='MAXDB';"));
    auto gtid_executed = s->query("select @@global.GTID_EXECUTED;")
                             ->fetch_one()
                             ->get_as_string(0);

    testutil->call_mysqlsh_c(
        {"--", "util", "checkForServerUpgrade", uri.c_str()});
    MY_EXPECT_STDOUT_NOT_CONTAINS("Check failed");
    MY_EXPECT_STDERR_NOT_CONTAINS("Check failed");
    MY_EXPECT_STDOUT_CONTAINS(
        "global system variable sql_mode - defined using obsolete MAXDB");
    MY_EXPECT_STDOUT_CONTAINS("obsolete NO_KEY_OPTIONS");
    MY_EXPECT_STDOUT_CONTAINS(
        "global.sql_mode - does not contain either NO_ZERO_DATE");

    EXPECT_EQ(gtid_executed, s->query("select @@global.GTID_EXECUTED;")
                                 ->fetch_one()
                                 ->get_as_string(0));
  } catch (const std::exception &e) {
    std::cerr << e.what();
  }
  testutil->destroy_sandbox(sb_port, true);
}

}  // namespace mysqlsh
