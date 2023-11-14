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
#include "unittest/gprod_clean.h"

#include "modules/util/mod_util.h"
#include "modules/util/upgrade_check.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"

#define SKIP_IF_NOT_5_7_UP_TO(version)                                      \
  do {                                                                      \
    if (_target_server_version < Version(5, 7, 0) ||                        \
        _target_server_version >= version) {                                \
      SKIP_TEST(                                                            \
          "This test requires running against MySQL server version [5.7-" + \
          version.get_full() + ")");                                        \
    }                                                                       \
  } while (false)

using Version = mysqlshdk::utils::Version;

namespace mysqlsh {
using mysqlshdk::db::Connection_options;

namespace {

Upgrade_check::Upgrade_info upgrade_info(const Version &server,
                                         const Version &target) {
  Upgrade_check::Upgrade_info si;

  si.server_version = server;
  si.target_version = target;

  return si;
}

}  // namespace

class MySQL_upgrade_check_test : public Shell_core_test_wrapper {
 public:
  MySQL_upgrade_check_test()
      : info(upgrade_info(_target_server_version, Version(MYSH_VERSION))) {}

 protected:
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

  bool is_plugin_loaded(const std::string &name) {
    auto result = session->query(
        "select 'OK' from information_schema.plugins where PLUGIN_NAME='" +
        name + "' and PLUGIN_STATUS='ACTIVE';");
    auto row = result->fetch_one();
    if (!row) return false;
    if (row->get_as_string(0) != "OK") return false;
    return true;
  }

  bool execute(const std::string &sql) {
    try {
      session->execute(sql);
    } catch (const mysqlshdk::db::Error &e) {
      return false;
    }
    return true;
  }

  bool install_plugin(const std::string &name) {
    std::string ext = "so";
#ifdef _WIN32
    ext = "dll";
#endif
    return execute(shcore::str_format("INSTALL PLUGIN %s SONAME \"%s.%s\"",
                                      name.c_str(), name.c_str(), ext.c_str()));
  }

  bool uninstall_plugin(const std::string &name) {
    return execute(shcore::str_format("UNINSTALL PLUGIN %s", name.c_str()));
  }

  void remove_issue_by_schema(const std::string &value) {
    auto it = std::find_if(issues.begin(), issues.end(), [&](const auto &item) {
      return item.schema == value;
    });
    if (it != issues.end()) issues.erase(it);
  }

  void remove_issues_by_schema(const std::vector<std::string> &list) {
    for (auto &item : list) {
      remove_issue_by_schema(item);
    }
  }

  void EXPECT_ISSUES(
      Upgrade_check *check, const int no = -1,
      std::function<void()> after_run_callback = std::function<void()>()) {
    try {
      issues = check->run(session, info);
      if (after_run_callback) after_run_callback();
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

  void EXPECT_ISSUE(
      const Upgrade_issue &actual, const std::string &schema = "",
      const std::string table = "", const std::string &column = "",
      Upgrade_issue::Level level = Upgrade_issue::Level::WARNING) {
    std::string scope{schema + "." + table + "." + column};
    scope.append("-").append(Upgrade_issue::level_to_string(level));
    SCOPED_TRACE(scope.c_str());
    EXPECT_STREQ(schema.c_str(), actual.schema.c_str());
    EXPECT_STRCASEEQ(table.c_str(), actual.table.c_str());
    EXPECT_STREQ(column.c_str(), actual.column.c_str());
    EXPECT_EQ(level, actual.level);
  }

  void EXPECT_NO_ISSUES(Upgrade_check *check) { EXPECT_ISSUES(check, 0); }

  Upgrade_check::Upgrade_info info;
  std::shared_ptr<mysqlshdk::db::ISession> session;
  std::string db;
  std::vector<mysqlsh::Upgrade_issue> issues;
};

TEST(Upgrade_check_options, set_target_version) {
  Upgrade_check_options options;
  options.set_target_version("8");
  EXPECT_EQ(Version(MYSH_VERSION), options.target_version);

  options.set_target_version("8.0");
  EXPECT_EQ(Version(LATEST_MYSH_80_VERSION), options.target_version);

  options.set_target_version("8.1");
  EXPECT_EQ(Version(8, 1, 0), options.target_version);

  options.set_target_version("8.2");
  EXPECT_EQ(Version(8, 2, 0), options.target_version);

  options.set_target_version("8.3");
  EXPECT_EQ(Version(8, 3, 0), options.target_version);

  options.set_target_version("8.0.34");
  EXPECT_EQ(Version(8, 0, 34), options.target_version);
}

TEST_F(MySQL_upgrade_check_test, checklist_generation) {
  Version current(MYSH_VERSION);
  Version prev(current.get_major(), current.get_minor(),
               current.get_patch() - 1);
  EXPECT_THROW_LIKE(Upgrade_check::create_checklist(
                        upgrade_info(Version("5.7"), Version("5.7"))),
                    std::invalid_argument, "This tool supports checking");
  EXPECT_THROW_LIKE(Upgrade_check::create_checklist(
                        upgrade_info(Version("5.6.11"), Version("8.0"))),
                    std::invalid_argument, "at least at version 5.7");
  EXPECT_THROW_LIKE(
      Upgrade_check::create_checklist(upgrade_info(
          Version("5.7.19"),
          Version(mysqlshdk::utils::k_shell_version.get_major(),
                  mysqlshdk::utils::k_shell_version.get_minor() + 1, 0))),
      std::invalid_argument, "This tool supports checking");
  EXPECT_THROW_LIKE(
      Upgrade_check::create_checklist(upgrade_info(current, current)),
      std::invalid_argument,
      "MySQL Shell cannot check MySQL server instances for upgrade if they are "
      "at a version the same as or higher than the MySQL Shell version.");
  EXPECT_THROW_LIKE(Upgrade_check::create_checklist(
                        upgrade_info(Version("8.0.12"), Version("8.0.12"))),
                    std::invalid_argument, "Target version must be greater");
  EXPECT_NO_THROW(Upgrade_check::create_checklist(
      upgrade_info(Version("5.7.19"), current)));
  EXPECT_NO_THROW(Upgrade_check::create_checklist(
      upgrade_info(Version("5.7.17"), Version("8.0"))));
  EXPECT_NO_THROW(Upgrade_check::create_checklist(
      upgrade_info(Version("5.7"), Version("8.0.12"))));

  std::vector<std::unique_ptr<Upgrade_check>> checks;
  EXPECT_NO_THROW(
      checks = Upgrade_check::create_checklist(upgrade_info(prev, current)));
  // Check for table command is there for every valid version as a last check
  EXPECT_FALSE(checks.empty());
  EXPECT_EQ(0, strcmp("checkTableOutput", checks.back()->get_name()));
}

TEST_F(MySQL_upgrade_check_test, old_temporal) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_old_temporal_check();
  EXPECT_NE(nullptr, check->get_doc_link());
  EXPECT_NO_ISSUES(check.get());
  // No way to create test data in 5.7
}

TEST_F(MySQL_upgrade_check_test, reserved_keywords) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_reserved_keywords_check(info);
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
  ASSERT_NO_THROW(
      session->execute("CREATE FUNCTION full (s CHAR(20)) RETURNS CHAR(50) "
                       "DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');"));
  ASSERT_NO_THROW(session->execute(
      "CREATE FUNCTION inteRsect (s CHAR(20)) RETURNS CHAR(50) "
      "DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');"));
  ASSERT_NO_THROW(session->execute(
      "CREATE EVENT LEAD ON SCHEDULE AT CURRENT_TIMESTAMP + INTERVAL 1 "
      "HOUR DO UPDATE System SET JSON_TABLE = JSON_TABLE + 1;"));

  EXPECT_ISSUES(check.get(), 14);
  EXPECT_ISSUE(issues[0], "grouping");
  EXPECT_ISSUE(issues[1], "grouping", "System");
  EXPECT_ISSUE(issues[2], "grouping", "NTile", "JSON_TABLE");
  EXPECT_ISSUE(issues[3], "grouping", "NTile", "cube");
  EXPECT_ISSUE(issues[4], "grouping", "System", "JSON_TABLE");
  EXPECT_ISSUE(issues[5], "grouping", "System", "cube");
  EXPECT_ISSUE(issues[6], "grouping", "first_value");
  EXPECT_ISSUE(issues[7], "grouping", "LATERAL");
  EXPECT_ISSUE(issues[8], "grouping", "NTile");
  EXPECT_ISSUE(issues[9], "grouping", "Array");
  EXPECT_ISSUE(issues[10], "grouping", "full");
  EXPECT_ISSUE(issues[11], "grouping", "inteRsect");
  EXPECT_ISSUE(issues[12], "grouping", "rows");
  EXPECT_ISSUE(issues[13], "grouping", "LEAD");

  check = Sql_upgrade_check::get_reserved_keywords_check(
      upgrade_info(Version(5, 7, 0), Version(8, 0, 30)));
  ASSERT_NO_THROW(issues = check->run(session, info));
  ASSERT_EQ(12, issues.size());
  EXPECT_ISSUE(issues[10], "grouping", "rows");
  EXPECT_ISSUE(issues[11], "grouping", "LEAD");

  check = Sql_upgrade_check::get_reserved_keywords_check(
      upgrade_info(Version(5, 7, 0), Version(8, 0, 11)));
  ASSERT_NO_THROW(issues = check->run(session, info));
  ASSERT_EQ(10, issues.size());
}

TEST_F(MySQL_upgrade_check_test, syntax_check) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));
  static const char *test_objects[] = {
      R"*(CREATE PROCEDURE testsp1()
BEGIN
  DECLARE rows INT DEFAULT 0;
  DECLARE ROW  INT DEFAULT 4;
END)*",
      R"*(CREATE PROCEDURE testsp1_ok()
BEGIN
  DECLARE `rows` INT DEFAULT 0;
  DECLARE `ROW`  INT DEFAULT 4;
END)*",
      R"*(CREATE FUNCTION testf1() RETURNS INT
BEGIN
  DECLARE rows INT DEFAULT 0;
  DECLARE ROW  INT DEFAULT 4;
  RETURN 0;
END)*",
      R"*(CREATE FUNCTION testf1_ok() RETURNS INT
BEGIN
  DECLARE `rows` INT DEFAULT 0;
  DECLARE `ROW`  INT DEFAULT 4;
  RETURN 0;
END)*",
      R"*(CREATE PROCEDURE testsp2()
BEGIN
  CREATE TABLE reservedTestTable (
      ROWS binary(16) NOT NULL,
      ROW binary(16) NOT NULL,
      C datetime NOT NULL,
      D varchar(32) NOT NULL,
      E longtext NOT NULL,
      PRIMARY KEY (ROWS,`C`),
      KEY `BD_idx` (ROW,`D`)
  ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
END)*",
      R"*(CREATE PROCEDURE testsp2_ok()
BEGIN
  CREATE TABLE reservedTestTable (
      `ROWS` binary(16) NOT NULL,
      `ROW` binary(16) NOT NULL,
      C datetime NOT NULL,
      D varchar(32) NOT NULL,
      E longtext NOT NULL,
      PRIMARY KEY (`ROWS`,`C`),
      KEY `BD_idx` (`ROW`,`D`)
  ) ENGINE=InnoDB DEFAULT CHARSET=utf8;
END)*",
      "CREATE TABLE testbl (a int primary key)",
      R"*(CREATE TRIGGER mytrigger BEFORE INSERT ON testbl FOR EACH ROW
       BEGIN
        DECLARE rows INT DEFAULT 0;
        DECLARE ROW  INT DEFAULT 4;
      END)*",
      R"*(CREATE TRIGGER mytrigger_ok BEFORE INSERT ON testbl
       FOR EACH ROW
      BEGIN 
        DECLARE `rows` INT DEFAULT 0;
        DECLARE `ROW` INT DEFAULT 4;
      END)*",
      R"*(CREATE EVENT myevent ON SCHEDULE AT '2000-01-01 00:00:00'
     ON COMPLETION PRESERVE DO
     begin
      DECLARE rows INT DEFAULT 0;
      DECLARE ROW INT DEFAULT 4;
     end)*",
      R"*(CREATE EVENT myevent_ok ON SCHEDULE AT '2000-01-01 00:00:00'
     ON COMPLETION PRESERVE DO
     begin
      DECLARE `rows` INT DEFAULT 0;
      DECLARE `ROW` INT DEFAULT 4;
     end)*"};

  auto check = Sql_upgrade_check::get_routine_syntax_check();
  EXPECT_NE(nullptr, check->get_doc_link());
  EXPECT_NO_ISSUES(check.get());

  PrepareTestDatabase("testdb");
  for (const char *sql : test_objects) {
    ASSERT_NO_THROW(session->execute(sql));
  }

  EXPECT_ISSUES(check.get(), 5);
  EXPECT_ISSUE(issues[0], "testdb", "testsp1", "", Upgrade_issue::ERROR);
  EXPECT_ISSUE(issues[1], "testdb", "testsp2", "", Upgrade_issue::ERROR);
  EXPECT_ISSUE(issues[2], "testdb", "testf1", "", Upgrade_issue::ERROR);
  EXPECT_ISSUE(issues[3], "testdb", "mytrigger", "", Upgrade_issue::ERROR);
  EXPECT_ISSUE(issues[4], "testdb", "myevent", "", Upgrade_issue::ERROR);
}

TEST_F(MySQL_upgrade_check_test, utf8mb3) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

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
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

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
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

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
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

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
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_foreign_key_length_check();
  EXPECT_NE(nullptr, check->get_doc_link());
  EXPECT_NO_ISSUES(check.get());
  // No way to prepare test data in 5.7
}

TEST_F(MySQL_upgrade_check_test, maxdb_sqlmode) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

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
  ASSERT_NO_THROW(issues = check->run(session, info));
  ASSERT_GT(issues.size(), issues_count);
  issues_count = issues.size();
  issues.clear();
  ASSERT_NO_THROW(
      session->execute("create trigger TR_MAXDB AFTER INSERT on Clone FOR "
                       "EACH ROW delete from Clone where COMPONENT<0;"));
  ASSERT_NO_THROW(issues = check->run(session, info));
  ASSERT_GT(issues.size(), issues_count);
  issues_count = issues.size();
  issues.clear();
  ASSERT_NO_THROW(
      session->execute("CREATE EVENT EV_MAXDB ON SCHEDULE AT CURRENT_TIMESTAMP "
                       "+ INTERVAL 1 HOUR "
                       "DO UPDATE Clone SET COMPONENT = COMPONENT + 1;"));
  ASSERT_NO_THROW(issues = check->run(session, info));
  ASSERT_GT(issues.size(), issues_count);
}

TEST_F(MySQL_upgrade_check_test, obsolete_sqlmodes) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

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
    ASSERT_NO_THROW(issues = check->run(session, info));
    ASSERT_GE(issues.size(), issues_count + 3);
  }
}

TEST_F(MySQL_upgrade_check_test, enum_set_element_length) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

  PrepareTestDatabase("aaa_test_enum_set_element_length");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_enum_set_element_length_check();
  EXPECT_EQ(
      0,
      strcmp(
          "https://dev.mysql.com/doc/refman/8.0/en/string-type-overview.html",
          check->get_doc_link()));
  ASSERT_NO_THROW(issues = check->run(session, info));
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
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 13));

  PrepareTestDatabase("aaa_test_partitioned_in_shared");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_partitioned_tables_in_shared_tablespaces_check(
          info);
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
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 17));

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
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

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
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 13));

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
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 13));

  std::unique_ptr<Upgrade_check> check =
      Sql_upgrade_check::get_removed_sys_log_vars_check(info);
  EXPECT_EQ(0, strcmp("https://dev.mysql.com/doc/relnotes/mysql/8.0/en/"
                      "news-8-0-13.html#mysqld-8-0-13-logging",
                      check->get_doc_link()));

  if (_target_server_version < Version(8, 0, 0)) {
    EXPECT_THROW_LIKE(
        check->run(session, info), Upgrade_check::Check_configuration_error,
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
  ASSERT_THROW(defined.run(session, info),
               Upgrade_check::Check_configuration_error);

  info.config_path.assign(
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

  info.config_path.clear();
}

TEST_F(MySQL_upgrade_check_test, removed_sys_vars) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 13));

  std::unique_ptr<Upgrade_check> check =
      Sql_upgrade_check::get_removed_sys_vars_check(info);
  EXPECT_EQ(0, strcmp("https://dev.mysql.com/doc/refman/8.0/en/"
                      "added-deprecated-removed.html#optvars-removed",
                      check->get_doc_link()));

  if (_target_server_version < Version(8, 0, 0)) {
    EXPECT_THROW_LIKE(
        check->run(session, info), Upgrade_check::Check_configuration_error,
        "To run this check requires full path to MySQL server configuration "
        "file to be specified at 'configPath' key of options dictionary");
    info.config_path.assign(
        shcore::path::join_path(g_test_home, "data", "config", "uc.cnf"));
    EXPECT_ISSUES(check.get(), 2);
    info.config_path.clear();
    EXPECT_EQ("max_tmp_tables", issues[0].schema);
    EXPECT_EQ("query-cache-type", issues[1].schema);
  } else {
    EXPECT_NO_ISSUES(check.get());
  }
}

TEST_F(MySQL_upgrade_check_test, sys_vars_new_defaults) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

  std::unique_ptr<Upgrade_check> check =
      Sql_upgrade_check::get_sys_vars_new_defaults_check();
  EXPECT_EQ(
      0, strcmp("https://dev.mysql.com/blog-archive/new-defaults-in-mysql-8-0/",
                check->get_doc_link()));

  EXPECT_THROW_LIKE(
      check->run(session, info), Upgrade_check::Check_configuration_error,
      "To run this check requires full path to MySQL server configuration "
      "file to be specified at 'configPath' key of options dictionary");
  info.config_path.assign(
      shcore::path::join_path(g_test_home, "data", "config", "my.cnf"));
  EXPECT_ISSUES(check.get(), 25);
  EXPECT_EQ("back_log", issues[0].schema);
  EXPECT_EQ("transaction_write_set_extraction", issues.back().schema);
  info.config_path.clear();
}

TEST_F(MySQL_upgrade_check_test, schema_inconsitencies) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

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

  // List of file names that cause problem on windows, when they are used in
  // schemas or tables, such names are are registered in i_s.innodb_sys_tables
  // by appending @@@ but not in i_s.tables
  std::vector<std::string> reserved_names = {
      "CLOCK$", "CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3",
      "COM4",   "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2",
      "LPT3",   "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};
  for (const auto &schema_name : reserved_names) {
    PrepareTestDatabase(schema_name);
    for (const auto &table_name : reserved_names) {
      EXPECT_NO_THROW(
          session->execute("create table `" + table_name + "` (i integer);"));
    }
  }

  // NOTE: Due to the impossibility of setting the state for some scenarios
  // (server crash required and inability to insert records into the
  // innodb_sy_table) the following test will exercise the query used on this
  // check on a test schema, to make sure it returns the correct table name in
  // the following scenarios (orphan tables in all cases):
  // - Normal Table
  // - Temporary Table
  // - Partitioned Table (Non Windows)
  // - Partitioned Table (Windows)

  // Creates the test schema with a copy of the required tables
  PrepareTestDatabase("orphan_table_query_test");
  EXPECT_NO_THROW(
      session->execute("create table innodb_sys_tables as select * from "
                       "information_schema.innodb_sys_tables"));
  EXPECT_NO_THROW(session->execute(
      "create table tables as select * from information_schema.tables"));

  // Inserts the orphan table records in the fake innodb_sys_tables
  EXPECT_NO_THROW(session->execute(
      "insert into innodb_sys_tables values"
      "(1, 'test1/normal', 1, 1, 1, 'Barracuda', 'Dynamic', 0, 'Single'),"
      "(2, 'test2/#sql2-65bb-2', 1, 1, 1, 'Barracuda', 'Dynamic', 0, 'Single'),"
      "(3, 'test3/partitioned#P#p1', 1, 1, 1, 'Barracuda', 'Dynamic', 0, "
      "'Single'),"
      "(4, 'test4/partitionedWin#p#p1', 1, 1, 1, 'Barracuda', 'Dynamic', 0, "
      "'Single')"));

  // Gets the query from the check, and tweaks is to use a test schema and adds
  // order by clause for test consistency
  std::string query = check->get_queries().at(0);
  query = shcore::str_replace(query, "information_schema",
                              "orphan_table_query_test");
  query.pop_back();
  query += " order by schema_name";
  auto result = session->query(query);

  // Validates the table names are retrieved as expected on each case
  std::vector<std::string> expected = {"normal", "#sql2-65bb-2", "partitioned",
                                       "partitionedWin"};

  for (const auto &table : expected) {
    auto row = result->fetch_one();
    EXPECT_STREQ(table.c_str(), row->get_string(1).c_str());
  }
}

TEST_F(MySQL_upgrade_check_test, non_native_partitioning) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

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
  auto si = upgrade_info(Version(5, 7, 25), Version(8, 0, 17));
  si.server_os = compile_os;
#if defined(WIN32)
  EXPECT_THROW(Sql_upgrade_check::get_fts_in_tablename_check(si),
               Upgrade_check::Check_not_needed);
#else
  PrepareTestDatabase("fts_tablename");
  EXPECT_THROW(Sql_upgrade_check::get_fts_in_tablename_check(info),
               Upgrade_check::Check_not_needed);
  auto check = Sql_upgrade_check::get_fts_in_tablename_check(si);
  EXPECT_NO_ISSUES(check.get());

  ASSERT_NO_THROW(
      session->execute("CREATE TABLE `DP01_FTS_REGULATION_ACTION_HIST` "
                       "( `HJID` bigint(20) NOT "
                       "NULL, `VERSION` bigint(20) NOT NULL, PRIMARY "
                       "KEY (`HJID`,`VERSION`));"));
#ifndef __APPLE__
  ASSERT_NO_THROW(session->execute(
      "create table `DP01_FtS_REGULATION_ACTION_HIST` (i integer);"));
#endif

  EXPECT_ISSUES(check.get(), 1);
  EXPECT_EQ("DP01_FTS_REGULATION_ACTION_HIST", issues[0].table);
#endif
}

TEST_F(MySQL_upgrade_check_test, check_table_command) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

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
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

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
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

  auto check = Sql_upgrade_check::get_engine_mixup_check();
  EXPECT_NE(nullptr, strstr(check->get_description(),
                            "Rename the MyISAM table to a temporary name"));
  EXPECT_NO_ISSUES(check.get());

  // positive test cases performed manually as it requires manual changes to the
  // datadir files
}

TEST_F(MySQL_upgrade_check_test, old_geometry_check) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

  const auto si23 = upgrade_info(_target_server_version, Version(8, 0, 23));
  auto check = Sql_upgrade_check::get_old_geometry_types_check(si23);
  EXPECT_NE(nullptr, strstr(check->get_description(),
                            "The following columns are spatial data columns "
                            "created in MySQL Server version 5.6"));

  // positive test cases performed manually as they required datadir created
  // in 5.6 and upgraded to 5.7
  EXPECT_NO_ISSUES(check.get());

  const auto si24 = upgrade_info(_target_server_version, Version(8, 0, 24));
  EXPECT_THROW(Sql_upgrade_check::get_old_geometry_types_check(si24),
               Upgrade_check::Check_not_needed);
}

TEST_F(MySQL_upgrade_check_test, manual_checks) {
  auto manual = Upgrade_check::create_checklist(
      upgrade_info(Version("5.7.0"), Version("8.0.11")));
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
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

  auto mysql_connection_options = shcore::get_connection_options(_mysql_uri);
  _interactive_shell->connect(mysql_connection_options);
  Util util(_interactive_shell->shell_context().get());

  // valid mysql 5.7 superuser
  try {
    util.check_for_server_upgrade(mysql_connection_options);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    EXPECT_TRUE(false);
  }

  // valid mysql 5.7 superuser X protocol
  EXPECT_NO_THROW(
      util.check_for_server_upgrade(shcore::get_connection_options(_uri)));

  // new user with required privileges sans grant option and '%' in host
  EXPECT_NO_THROW(session->execute(
      "create user if not exists 'percent'@'%' identified by 'percent';"));
  std::string percent_uri = _mysql_uri;
  percent_uri.replace(0, 4, "percent:percent");
  auto percent_connection_options = shcore::get_connection_options(percent_uri);
  // No privileges function should throw
  EXPECT_THROW(util.check_for_server_upgrade(percent_connection_options),
               std::runtime_error);

  // Still not enough privileges
  EXPECT_NO_THROW(session->execute("grant SELECT on *.* to 'percent'@'%';"));
  EXPECT_THROW(util.check_for_server_upgrade(percent_connection_options),
               std::runtime_error);

  // Privileges check out we should succeed
  EXPECT_NO_THROW(
      session->execute("grant RELOAD, PROCESS on *.* to 'percent'@'%';"));
  EXPECT_NO_THROW(util.check_for_server_upgrade(percent_connection_options));

  EXPECT_NO_THROW(session->execute("drop user 'percent'@'%';"));

  // Using default session to run checks
  EXPECT_NO_THROW(util.check_for_server_upgrade(Connection_options()));

  // Using default session with options dictionary
  shcore::Option_pack_ref<Upgrade_check_options> op;
  op->target_version = mysqlshdk::utils::Version(8, 0, 18);
  EXPECT_NO_THROW(util.check_for_server_upgrade(Connection_options(), op));
}

TEST_F(MySQL_upgrade_check_test, JSON_output_format) {
  SKIP_IF_NOT_5_7_UP_TO(Version(MYSH_VERSION));

  Util util(_interactive_shell->shell_context().get());

  // clear stdout/stderr garbage
  reset_shell();

  // valid mysql 5.7 superuser
  shcore::Option_pack_ref<Upgrade_check_options> options;
  options->output_format = "JSON";
  auto connection_options = shcore::get_connection_options(_mysql_uri);
  try {
    util.check_for_server_upgrade(connection_options, options);
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
    util.check_for_server_upgrade(connection_options);
    _options->wrap_json = old_wrap;
    rapidjson::Document d1;
    d1.Parse(output_handler.std_out.c_str());
    ASSERT_FALSE(d.HasParseError());
    ASSERT_TRUE(d.IsObject());
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    EXPECT_TRUE(false);
  }
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
  EXPECT_ANY_THROW(util.check_for_server_upgrade(
      shcore::get_connection_options(_mysql_uri)));
}

TEST_F(MySQL_upgrade_check_test, password_prompted) {
  Util util(_interactive_shell->shell_context().get());

  output_handler.passwords.push_back(
      {"Please provide the password for "
       "'" +
           _mysql_uri_nopasswd + "': ",
       "WhAtEvEr",
       {}});
  EXPECT_THROW(util.check_for_server_upgrade(
                   shcore::get_connection_options(_mysql_uri_nopasswd)),
               shcore::Exception);

  // Passwords are consumed if prompted, so verifying this indicates the
  // password was prompted as expected and consumed
  EXPECT_TRUE(output_handler.passwords.empty());
}

TEST_F(MySQL_upgrade_check_test, password_no_prompted) {
  Util util(_interactive_shell->shell_context().get());
  output_handler.passwords.push_back(
      {"If this was prompted it is an error", "WhAtEvEr", {}});

  try {
    util.check_for_server_upgrade(shcore::get_connection_options(_mysql_uri));
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

  output_handler.passwords.push_back(
      {"If this was prompted it is an error", "WhAtEvEr", {}});

  try {
    util.check_for_server_upgrade(
        shcore::get_connection_options(_mysql_uri_nopasswd));
  } catch (...) {
    // We don't really care for this test
  }

  // Passwords are consumed if prompted, so verifying this indicates the
  // password was NOT prompted as expected and so, NOT consumed
  EXPECT_FALSE(output_handler.passwords.empty());
  output_handler.passwords.clear();
}

TEST_F(MySQL_upgrade_check_test, GTID_EXECUTED_unchanged) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

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

TEST_F(MySQL_upgrade_check_test, convert_usage) {
  {
    // upgrade to < 8.0.28 needs no check
    ASSERT_THROW(
        Sql_upgrade_check::get_changed_functions_generated_columns_check(
            upgrade_info(Version(8, 0, 26), Version(8, 0, 27))),
        Upgrade_check::Check_not_needed);

    // upgrade between >= 8.0.28 needs no check
    ASSERT_THROW(
        Sql_upgrade_check::get_changed_functions_generated_columns_check(
            upgrade_info(Version(8, 0, 28), Version(8, 0, 29))),
        Upgrade_check::Check_not_needed);

    ASSERT_NO_THROW(
        Sql_upgrade_check::get_changed_functions_generated_columns_check(
            upgrade_info(Version(5, 7, 28), Version(8, 0, 28))));

    ASSERT_NO_THROW(
        Sql_upgrade_check::get_changed_functions_generated_columns_check(
            upgrade_info(Version(8, 0, 27), Version(8, 0, 29))));
  }

  auto options = upgrade_info(Version(8, 0, 11), Version(8, 0, 28));

  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_changed_functions_generated_columns_check(options);

  EXPECT_NE(nullptr, check->get_doc_link());

  PrepareTestDatabase("testdb");
  ASSERT_NO_THROW(session->execute(
      "create table if not exists testdb.genindexcast (a varchar(40), b "
      "varchar(40) generated always "
      "as (cast(a as char character set latin2)) stored key,"
      "`cast` int generated always as (1+1) stored);"));
  ASSERT_NO_THROW(session->execute(
      "create table if not exists testdb.genindexconv (a varchar(40), b "
      "varchar(40) generated always "
      "as (convert(a using 'latin1')) stored key,"
      "`convert` int generated always as (32+a) stored);"));

  ASSERT_NO_THROW(session->execute(
      "create table if not exists testdb.plainconv (a varchar(40), b "
      "varchar(40) generated always "
      "as (convert(a using 'latin1')) stored,"
      "x varchar(1) generated always as (4) virtual);"));
  ASSERT_NO_THROW(session->execute(
      "create table if not exists testdb.plaincast (a varchar(40), b "
      "varchar(40) generated always "
      "as (cast(a as char character set latin2)) stored,"
      "y int generated always as (42) virtual);"));

  EXPECT_ISSUES(check.get(), 2);
  EXPECT_EQ("testdb", issues[0].schema);
  EXPECT_EQ(Upgrade_issue::WARNING, issues[0].level);
  EXPECT_STRCASEEQ("genindexcast", issues[0].table.c_str());
  EXPECT_EQ("b", issues[0].column);

  EXPECT_EQ("testdb", issues[1].schema);
  EXPECT_EQ(Upgrade_issue::WARNING, issues[1].level);
  EXPECT_STRCASEEQ("genindexconv", issues[1].table.c_str());
  EXPECT_EQ("b", issues[1].column);
}

TEST_F(MySQL_upgrade_check_test, columns_which_cannot_have_defaults_check) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 12));

  PrepareTestDatabase("columns_which_cannot_have_defaults_check_test");

  const auto check =
      Sql_upgrade_check::get_columns_which_cannot_have_defaults_check();
  EXPECT_EQ(0, strcmp("https://dev.mysql.com/doc/refman/8.0/en/"
                      "data-type-defaults.html#data-type-defaults-explicit",
                      check->get_doc_link()));

  EXPECT_NO_ISSUES(check.get());

  // STRICT_TRANS_TABLES needs to be disabled in order for the columns with
  // types which cannot have defaults could be created
  ASSERT_NO_THROW(session->execute("set sql_mode='';"));
  ASSERT_NO_THROW(
      session->execute("create table t ("
                       "p point NOT NULL default '',"
                       "ls linestring NOT NULL default '',"
                       "poly polygon NOT NULL default '',"
                       "g geometry NOT NULL default '',"
                       "mp multipoint NOT NULL default '',"
                       "mls multilinestring NOT NULL default '',"
                       "mpoly multipolygon NOT NULL default '',"
                       "gc geometrycollection NOT NULL default '',"
                       "j json NOT NULL default '',"
                       "tb tinyblob NOT NULL default '',"
                       "b blob NOT NULL default '',"
                       "mb mediumblob NOT NULL default '',"
                       "lb longblob NOT NULL default '',"
                       "tt tinytext NOT NULL default '',"
                       "t text NOT NULL default '',"
                       "mt mediumtext NOT NULL default '',"
                       "lt longtext NOT NULL default ''"
                       ");"));

  // for some reason, creating a blob/text column with a default value of ''
  // warns that this column type cannot have a default, but COLUMN_DEFAULT in
  // INFORMATION_SCHEMA.COLUMNS still holds NULL, and SHOW CREATE TABLE does
  // not show a default value for such column, hence these columns are not
  // reported here

  EXPECT_ISSUES(check.get(), 9);

  EXPECT_EQ("p", issues[0].column);
  EXPECT_EQ("ls", issues[1].column);
  EXPECT_EQ("poly", issues[2].column);
  EXPECT_EQ("g", issues[3].column);
  EXPECT_EQ("mp", issues[4].column);
  EXPECT_EQ("mls", issues[5].column);
  EXPECT_EQ("mpoly", issues[6].column);
  EXPECT_EQ("gc", issues[7].column);
  EXPECT_EQ("j", issues[8].column);
}

TEST_F(MySQL_upgrade_check_test, orphaned_routines_check) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

  ASSERT_NO_THROW(session->execute("use mysql;"));

  const auto check = Sql_upgrade_check::get_orphaned_routines_check();

  EXPECT_NO_ISSUES(check.get());

  ASSERT_NO_THROW(session->execute(
      "INSERT INTO mysql.proc VALUES ("
      "'no_ex_db',"
      "'orphaned_procedure',"
      "'PROCEDURE',"
      "'orphaned_procedure',"
      "'SQL','CONTAINS_SQL','NO','DEFINER','','',"
      "_binary 'begin\n"
      "select count(*) from somedb.sometable;"
      "\nend',"
      "'root@localhost','2022-11-23 11:46:34','2022-11-23 11:46:34',"
      "'ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,"
      "NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION',"
      "'','utf8mb4','utf8mb4_general_ci','latin1_swedish_ci',"
      "_binary 'begin\n"
      "select count(*) from somedb.sometable;\n"
      "end');"));

  EXPECT_ISSUES(check.get(), 1);

  EXPECT_EQ("no_ex_db", issues[0].schema);
  EXPECT_EQ("orphaned_procedure", issues[0].table);

  ASSERT_NO_THROW(
      session->execute("DELETE FROM mysql.proc WHERE db='no_ex_db' AND "
                       "name='orphaned_procedure';"));
}

TEST_F(MySQL_upgrade_check_test, dollar_sign_name_check) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 32));

  ASSERT_NO_THROW(
      session->execute("DROP SCHEMA IF EXISTS $wrong_schema_dollar_name;"));

  PrepareTestDatabase("$dollar_sign_name_check$");

  ASSERT_NO_THROW(
      session->execute("CREATE TABLE $correct_table$ ($correct_column1$ INT, "
                       "$correct_column2$ INT);"));
  ASSERT_NO_THROW(
      session->execute("CREATE VIEW $correct_view$ AS SELECT $correct_column1$ "
                       "FROM $correct_table$;"));
  ASSERT_NO_THROW(session->execute(
      "CREATE INDEX $correct_index$ ON $correct_table$($correct_column2$);"));
  ASSERT_NO_THROW(
      session->execute("CREATE PROCEDURE $correct_procedure$()\nBEGIN\nSELECT "
                       "$correct_column1$ FROM $correct_table$;\nEND;"));
  ASSERT_NO_THROW(session->execute(
      "CREATE PROCEDURE $correct_procedure_second$()\nBEGIN\nSELECT "
      "'Some name with dollars $0.00';\nEND;"));

  const auto check = Sql_upgrade_check::get_dollar_sign_name_check();
  EXPECT_NO_ISSUES(check.get());

  ASSERT_NO_THROW(session->execute("CREATE SCHEMA $wrong_schema_dollar_name;"));
  ASSERT_NO_THROW(session->execute(
      "CREATE TABLE $wrong_table ($wrong_column1 INT, $wrong_column2 INT);"));
  ASSERT_NO_THROW(session->execute(
      "CREATE VIEW $wrong_view AS SELECT $wrong_column1 FROM $wrong_table;"));
  ASSERT_NO_THROW(session->execute(
      "CREATE INDEX $wrong_index ON $wrong_table($wrong_column2);"));
  ASSERT_NO_THROW(
      session->execute("CREATE PROCEDURE $wrong_procedure()\nBEGIN SELECT "
                       "$wrong_column1 FROM $wrong_table;\nEND;"));
  ASSERT_NO_THROW(session->execute(
      "CREATE PROCEDURE $wrong_procedure_body$()\nBEGIN\nSELECT "
      "$correct_column1$, $wrong_column1 FROM $correct_table$, "
      "$wrong_table;\nEND;"));

  EXPECT_ISSUES(check.get(), 10);

  EXPECT_EQ("$wrong_schema_dollar_name", issues[0].schema);
  EXPECT_EQ("$wrong_table", issues[1].table);
  EXPECT_EQ("$wrong_view", issues[2].table);
  EXPECT_EQ("$wrong_table", issues[3].table);
  EXPECT_EQ("$wrong_column1", issues[3].column);
  EXPECT_EQ("$wrong_table", issues[4].table);
  EXPECT_EQ("$wrong_column2", issues[4].column);
  EXPECT_EQ("$wrong_view", issues[5].table);
  EXPECT_EQ("$wrong_column1", issues[5].column);
  EXPECT_EQ("$wrong_table", issues[6].table);
  EXPECT_EQ("$wrong_index", issues[6].column);
  EXPECT_EQ("$wrong_procedure", issues[7].table);
  EXPECT_EQ("$wrong_procedure", issues[8].table);
  EXPECT_EQ("$wrong_procedure_body$", issues[9].table);

  ASSERT_NO_THROW(
      session->execute("DROP SCHEMA IF EXISTS $wrong_schema_dollar_name;"));
}

TEST_F(MySQL_upgrade_check_test, too_large_index_check) {
  SKIP_IF_NOT_5_7_UP_TO(Version(5, 7, 34));

  PrepareTestDatabase("index_test");

  ASSERT_NO_THROW(
      session->execute("set GLOBAL innodb_default_row_format='redundant';"));

  ASSERT_NO_THROW(session->execute(
      "CREATE TABLE `test123` (`id` int unsigned NOT NULL AUTO_INCREMENT, "
      "`comment` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci "
      "NOT NULL DEFAULT '', PRIMARY KEY(`id`)) ENGINE = InnoDB DEFAULT CHARSET "
      "= utf8;"));

  ASSERT_NO_THROW(session->execute(
      "insert into test123(comment) "
      "values('"
      "kncfosdncfvlknsadkvnalksdnvlkansdlkvcnalkdsncvlkasnfcklanskldnlaksndklas"
      "ndlkanslkdnaslkndlkasndklanslkdnkncfosdncfvlknsadkvnalksdnvlkansdlkvcnal"
      "kdsncvlkasnfcklanskldnlaksndklasndlkanslkdnaslkndlkasndklanslkdnkncfosdn"
      "cfvlknsadkvnalksdnvlkansdlkvcnalkdsncq');"));

  const auto check = Sql_upgrade_check::get_index_too_large_check();
  EXPECT_NO_ISSUES(check.get());

  EXPECT_ANY_THROW(
      session->execute("create index idx123 on test123 (`comment`);"));

  ASSERT_NO_THROW(
      session->execute("set GLOBAL innodb_default_row_format='dynamic';"));

  ASSERT_NO_THROW(
      session->execute("create index idx123 on test123 (`comment`);"));

  EXPECT_ISSUES(check.get(), 1);

  EXPECT_EQ("index_test", issues[0].schema);
  EXPECT_EQ("test123", issues[0].table);
  EXPECT_EQ("idx123", issues[0].column);

  ASSERT_NO_THROW(
      session->execute("set GLOBAL innodb_default_row_format=DEFAULT;"));

  ASSERT_NO_THROW(session->execute("DROP SCHEMA IF EXISTS index_test;"));
}

TEST_F(MySQL_upgrade_check_test, empty_dot_table_syntax_check) {
  SKIP_IF_NOT_5_7_UP_TO(Version(5, 7, 40));

  PrepareTestDatabase("dot_table_test");

  ASSERT_NO_THROW(session->execute("CREATE TABLE `dot_table` (`id` int);"));
  ASSERT_NO_THROW(session->execute(
      "CREATE TABLE `uni_table_\u1234\u0090\u4000` (`id` int);"));
  ASSERT_NO_THROW(session->execute(
      "CREATE TABLE `\u0020\u0170_uni_\u5033_table` (`id` int);"));

  ASSERT_NO_THROW(session->execute(
      "CREATE PROCEDURE correct_procedure()\nBEGIN\ndelete FROM "
      "dot_table;\nselect * from dot_table where 1=1;\nEND;"));

  ASSERT_NO_THROW(
      session->execute("CREATE EVENT correct_event ON SCHEDULE EVERY 1 HOUR DO "
                       "DELETE FROM dot_table;"));

  ASSERT_NO_THROW(
      session->execute("create TRIGGER correct_trigger BEFORE INSERT ON "
                       "dot_table FOR EACH ROW delete from dot_table;"));

  const auto check = Sql_upgrade_check::get_empty_dot_table_syntax_check();
  EXPECT_NO_ISSUES(check.get());

  ASSERT_NO_THROW(session->execute("set GLOBAL SQL_MODE=ANSI_QUOTES;"));

  ASSERT_NO_THROW(session->execute(
      "CREATE PROCEDURE incorrect_procedure()\nBEGIN\ndelete FROM "
      ".dot_table;\nselect * from .dot_table where 1=1;\nEND;"));

  ASSERT_NO_THROW(session->execute(
      "CREATE PROCEDURE incorrect_procedure2()\nBEGIN\ndelete FROM "
      ".dot_table;\nselect * from .\"dot_table\" where 1=1;\nEND;"));

  ASSERT_NO_THROW(session->execute(
      "CREATE PROCEDURE incorrect_procedure3()\nBEGIN\ndelete FROM "
      ".dot_table;\nselect * from .$dot_table$ where 1=1;\nEND;"));

  ASSERT_NO_THROW(session->execute(
      "CREATE PROCEDURE incorrect_uni_procedure()\nBEGIN\ndelete FROM "
      ".`uni_table_\u1234\u0090\u4000`;\nEND;"));

  ASSERT_NO_THROW(session->execute(
      "CREATE PROCEDURE incorrect_uni_procedure2()\nBEGIN\ndelete FROM "
      ".`\u0020\u0170_uni_\u5033_table`;\nEND;"));

  ASSERT_NO_THROW(session->execute(
      "CREATE EVENT incorrect_event ON SCHEDULE EVERY 1 HOUR DO "
      "DELETE FROM .dot_table;"));

  ASSERT_NO_THROW(
      session->execute("create TRIGGER incorrect_trigger BEFORE INSERT ON "
                       "dot_table FOR EACH ROW delete from .dot_table;"));

  EXPECT_ISSUES(check.get(), 7);

  EXPECT_EQ("dot_table_test", issues[0].schema);
  EXPECT_EQ("incorrect_procedure", issues[0].table);
  EXPECT_EQ("dot_table_test", issues[1].schema);
  EXPECT_EQ("incorrect_procedure2", issues[1].table);
  EXPECT_EQ("dot_table_test", issues[2].schema);
  EXPECT_EQ("incorrect_procedure3", issues[2].table);
  EXPECT_EQ("dot_table_test", issues[3].schema);
  EXPECT_EQ("incorrect_uni_procedure", issues[3].table);
  EXPECT_EQ("dot_table_test", issues[4].schema);
  EXPECT_EQ("incorrect_uni_procedure2", issues[4].table);
  EXPECT_EQ("dot_table_test", issues[5].schema);
  EXPECT_EQ("incorrect_event", issues[5].table);
  EXPECT_EQ("dot_table_test", issues[6].schema);
  EXPECT_EQ("incorrect_trigger", issues[6].table);

  ASSERT_NO_THROW(session->execute("DROP SCHEMA IF EXISTS dot_table_test;"));
}

TEST_F(MySQL_upgrade_check_test, invalid_engine_foreign_key_check) {
  SKIP_IF_NOT_5_7_UP_TO(Version(8, 0, 0));

  PrepareTestDatabase("inv_eng_db");

  ASSERT_NO_THROW(session->execute("SET SESSION foreign_key_checks = OFF;"));

  ASSERT_NO_THROW(session->execute(R"(
  CREATE TABLE `testtable` (
  `column_idColumn` int NOT NULL,
      KEY `fk_TestTable_SecondTable` (`column_idColumn`),
      CONSTRAINT `const_TestTable_SecondTable` FOREIGN
          KEY(`column_idColumn`)
              REFERENCES `secondtable` (`idColumn`) ON DELETE NO ACTION)
      ENGINE = InnoDB;)"));

  ASSERT_NO_THROW(session->execute(R"(CREATE TABLE `secondtable` (
  `idColumn` int NOT NULL, PRIMARY KEY(`idColumn`)) ENGINE = InnoDB;)"));

  const auto check = Sql_upgrade_check::get_invalid_engine_foreign_key_check();
  EXPECT_NO_ISSUES(check.get());

  ASSERT_NO_THROW(session->execute("DROP TABLE secondtable;"));

  ASSERT_NO_THROW(session->execute(R"(CREATE TABLE `secondtable` (
  `idColumn` int NOT NULL, PRIMARY KEY(`idColumn`)) ENGINE = MyISAM;)"));

  EXPECT_ISSUES(check.get(), 1);

  EXPECT_EQ("inv_eng_db", issues[0].schema);
  EXPECT_EQ("testtable", issues[0].table);
  EXPECT_EQ("column_idColumn", issues[0].column);
  EXPECT_EQ(
      "column has invalid foreign key to column 'idColumn' from table "
      "'inv_eng_db/secondtable' that is from a different database engine "
      "(MyISAM).",
      issues[0].description);

  ASSERT_NO_THROW(session->execute("SET SESSION foreign_key_checks = ON;"));
  ASSERT_NO_THROW(session->execute("DROP SCHEMA IF EXISTS inv_eng_db;"));
}

TEST_F(MySQL_upgrade_check_test, no_database_selected_corrupted_check) {
  SKIP_IF_NOT_5_7_UP_TO(Version(5, 7, 40));

  PrepareTestDatabase("test");

  ASSERT_NO_THROW(
      session->execute("create table test_table (test_column INT);"));
  ASSERT_NO_THROW(session->execute(
      "create view test_view as select `t`.`test_column` from (select "
      "`test`.`test_table`.`test_column` as `test_column` from "
      "`test`.`test_table` group by `test`.`test_table`.`test_column`) t;"));

  // clear current USE to null
  ASSERT_NO_THROW(session->execute("create database temp_db;"));
  ASSERT_NO_THROW(session->execute("use temp_db;"));
  ASSERT_NO_THROW(session->execute("drop database temp_db;"));

  const auto check = std::make_unique<Check_table_command>();
  EXPECT_ISSUES(check.get(), 2);
  EXPECT_EQ("test", issues[0].schema);
  EXPECT_EQ("test_view", issues[0].table);
  EXPECT_EQ("No database selected", issues[0].description);
  EXPECT_EQ("test", issues[1].schema);
  EXPECT_EQ("test_view", issues[1].table);
  EXPECT_EQ("Corrupt", issues[1].description);

  ASSERT_NO_THROW(session->execute("use mysql;"));

  EXPECT_NO_ISSUES(check.get());

  // clear current USE to null
  ASSERT_NO_THROW(session->execute("create database temp_db;"));
  ASSERT_NO_THROW(session->execute("use temp_db;"));
  ASSERT_NO_THROW(session->execute("drop database temp_db;"));

  auto cfg = Upgrade_check_config(Upgrade_check_options());
  cfg.set_user_privileges(nullptr);
  cfg.set_session(session);
  cfg.set_issue_filter([](const Upgrade_issue &issue) {
    if (issue.schema != "test") return false;
    if (issue.table != "test_view") return false;
    if (issue.description != "No database selected" &&
        issue.description != "Corrupt")
      return false;
    return true;
  });

  EXPECT_TRUE(check_for_upgrade(cfg));

  ASSERT_NO_THROW(session->execute("drop schema if exists test;"));
}

// WL#15973-TSFR_1_1
// WL#15973-TSFR_2
TEST_F(MySQL_upgrade_check_test, deprecated_auth_method_json_check) {
  SKIP_IF_NOT_5_7_UP_TO(Version(MYSH_VERSION));

  Util util(_interactive_shell->shell_context().get());

  // clear stdout/stderr garbage
  reset_shell();

  shcore::Option_pack_ref<Upgrade_check_options> options;
  options->output_format = "JSON";
  auto connection_options = shcore::get_connection_options(_mysql_uri);
  try {
    util.check_for_server_upgrade(connection_options, options);
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
    bool found_dep_auth_check_id = false;
    bool found_def_auth_check_id = false;
    for (auto &check : checks) {
      ASSERT_TRUE(check.IsObject());
      ASSERT_TRUE(check.HasMember("id"));
      ASSERT_TRUE(check["id"].IsString());
      if (std::string(check["id"].GetString()) == "deprecatedAuthMethod")
        found_dep_auth_check_id = true;
      if (std::string(check["id"].GetString()) == "deprecatedDefaultAuth")
        found_def_auth_check_id = true;
    }
    EXPECT_TRUE(found_dep_auth_check_id);
    EXPECT_TRUE(found_def_auth_check_id);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    EXPECT_TRUE(false);
  }
}

const std::vector<std::string> k_auth_method_false_positives = {
    "root@localhost", "mysql.session@localhost", "mysql.sys@localhost",
    "root@%"};

// WL#15973-TSFR_1_3_1
TEST_F(MySQL_upgrade_check_test, deprecated_auth_method_check) {
  PrepareTestDatabase("test");

  auto remove_false_positives = [&]() {
    remove_issues_by_schema(k_auth_method_false_positives);
    std::sort(issues.begin(), issues.end(),
              [](const auto &a, const auto &b) { return a.schema < b.schema; });
  };

  ASSERT_NO_THROW(
      session->execute("drop user if exists 'usr_sha_auth'@'localhost';"));
  ASSERT_NO_THROW(
      session->execute("drop user if exists 'usr_native_auth'@'localhost';"));
  ASSERT_NO_THROW(
      session->execute("drop user if exists 'usr_fido_auth'@'localhost';"));

  Upgrade_check::Upgrade_info temp_info;
  temp_info.target_version = Version(8, 4, 0);
  auto check = Sql_upgrade_check::get_deprecated_auth_method_check(temp_info);

  EXPECT_ISSUES(check.get(), 0, remove_false_positives);

  ASSERT_NO_THROW(
      session->execute("create user 'usr_sha_auth'@'localhost' identified with "
                       "'sha256_password';"));
  ASSERT_NO_THROW(session->execute(
      "create user 'usr_native_auth'@'localhost' identified with "
      "'mysql_native_password';"));

  // WL#15973-TSFR_1_2
  EXPECT_ISSUES(check.get(), 2, remove_false_positives);
  EXPECT_EQ(issues[0].schema, "usr_native_auth@localhost");
  EXPECT_EQ(issues[1].schema, "usr_sha_auth@localhost");

  EXPECT_EQ(issues[0].level, Upgrade_issue::Level::ERROR);
  EXPECT_EQ(issues[1].level, Upgrade_issue::Level::ERROR);

  EXPECT_EQ(issues[0].description,
            "mysql_native_password authentication method was removed and it "
            "must be corrected before upgrading to 8.4.0 release.");
  EXPECT_EQ(issues[1].description,
            "sha256_password authentication method was removed and it must be "
            "corrected before upgrading to 8.4.0 release.");

  // WL#15973-TSFR_1_3_2
  temp_info.target_version = Version(8, 3, 0);
  check = Sql_upgrade_check::get_deprecated_auth_method_check(temp_info);

  EXPECT_ISSUES(check.get(), 2, remove_false_positives);
  EXPECT_EQ(issues[0].schema, "usr_native_auth@localhost");
  EXPECT_EQ(issues[1].schema, "usr_sha_auth@localhost");

  EXPECT_EQ(issues[0].level, Upgrade_issue::Level::WARNING);
  EXPECT_EQ(issues[1].level, Upgrade_issue::Level::WARNING);

  EXPECT_EQ(issues[0].description,
            "mysql_native_password authentication method is deprecated and it "
            "should be considered to correct this before upgrading to 8.4.0 "
            "release.");
  EXPECT_EQ(issues[1].description,
            "sha256_password authentication method is deprecated and it should "
            "be considered to correct this before upgrading to 8.4.0 release.");

  // WL#15973-TSFR_1_3_3
  temp_info.target_version = Version(8, 2, 0);
  check = Sql_upgrade_check::get_deprecated_auth_method_check(temp_info);

  EXPECT_ISSUES(check.get(), 2, remove_false_positives);
  EXPECT_EQ(issues[0].schema, "usr_native_auth@localhost");
  EXPECT_EQ(issues[1].schema, "usr_sha_auth@localhost");

  EXPECT_EQ(issues[0].level, Upgrade_issue::Level::WARNING);
  EXPECT_EQ(issues[1].level, Upgrade_issue::Level::WARNING);

  EXPECT_EQ(issues[0].description,
            "mysql_native_password authentication method is deprecated and it "
            "should be considered to correct this before upgrading to 8.4.0 "
            "release.");
  EXPECT_EQ(issues[1].description,
            "sha256_password authentication method is deprecated and it should "
            "be considered to correct this before upgrading to 8.4.0 release.");

  // WL#15973-TSFR_1_3_4
  temp_info.target_version = Version(8, 1, 0);
  check = Sql_upgrade_check::get_deprecated_auth_method_check(temp_info);

  EXPECT_ISSUES(check.get(), 2, remove_false_positives);
  EXPECT_EQ(issues[0].schema, "usr_native_auth@localhost");
  EXPECT_EQ(issues[1].schema, "usr_sha_auth@localhost");

  EXPECT_EQ(issues[0].level, Upgrade_issue::Level::WARNING);
  EXPECT_EQ(issues[1].level, Upgrade_issue::Level::WARNING);

  EXPECT_EQ(issues[0].description,
            "mysql_native_password authentication method is deprecated and it "
            "should be considered to correct this before upgrading to 8.4.0 "
            "release.");
  EXPECT_EQ(issues[1].description,
            "sha256_password authentication method is deprecated and it should "
            "be considered to correct this before upgrading to 8.4.0 release.");

  temp_info.target_version = Version(8, 0, 27);
  check = Sql_upgrade_check::get_deprecated_auth_method_check(temp_info);

  EXPECT_ISSUES(check.get(), 2, remove_false_positives);
  EXPECT_EQ(issues[0].schema, "usr_native_auth@localhost");
  EXPECT_EQ(issues[1].schema, "usr_sha_auth@localhost");

  EXPECT_EQ(issues[0].level, Upgrade_issue::Level::WARNING);
  EXPECT_EQ(issues[1].level, Upgrade_issue::Level::WARNING);

  EXPECT_EQ(issues[0].description,
            "mysql_native_password authentication method is deprecated and it "
            "should be considered to correct this before upgrading to 8.4.0 "
            "release.");
  EXPECT_EQ(issues[1].description,
            "sha256_password authentication method is deprecated and it should "
            "be considered to correct this before upgrading to 8.4.0 release.");

  ASSERT_NO_THROW(session->execute("drop user 'usr_sha_auth'@'localhost';"));
  ASSERT_NO_THROW(session->execute("drop user 'usr_native_auth'@'localhost';"));
  ASSERT_NO_THROW(session->execute("drop schema if exists test;"));
}

TEST_F(MySQL_upgrade_check_test, deprecated_auth_method_check_fido) {
  bool authentication_fido_installed = is_plugin_loaded("authentication_fido");
  bool uninstall_authentication_fido = false;
  if (!authentication_fido_installed) {
    authentication_fido_installed = install_plugin("authentication_fido");
    if (!authentication_fido_installed) {
      SKIP_TEST(
          "This test requires loaded authentication_fido MySQL Server plugin.");
    } else {
      uninstall_authentication_fido = true;
    }
  }

  shcore::Scoped_callback cleanup([this, uninstall_authentication_fido]() {
    if (uninstall_authentication_fido) {
      uninstall_plugin("authentication_fido");
    }
  });

  auto remove_false_positives = [&]() {
    remove_issues_by_schema(k_auth_method_false_positives);
  };

  PrepareTestDatabase("test");

  ASSERT_NO_THROW(
      session->execute("drop user if exists 'usr_sha_auth'@'localhost';"));
  ASSERT_NO_THROW(
      session->execute("drop user if exists 'usr_native_auth'@'localhost';"));
  ASSERT_NO_THROW(
      session->execute("drop user if exists 'usr_fido_auth'@'localhost';"));

  Upgrade_check::Upgrade_info temp_info;
  temp_info.target_version = Version(8, 4, 0);
  auto check = Sql_upgrade_check::get_deprecated_auth_method_check(temp_info);

  EXPECT_ISSUES(check.get(), 0, remove_false_positives);

  ASSERT_NO_THROW(session->execute(
      "create user 'usr_fido_auth'@'localhost' identified with "
      "'authentication_fido';"));

  // WL#15973-TSFR_1_2
  EXPECT_ISSUES(check.get(), 1, remove_false_positives);
  EXPECT_EQ(issues[0].schema, "usr_fido_auth@localhost");

  EXPECT_EQ(issues[0].level, Upgrade_issue::Level::ERROR);  // WL#15973-TSFR_1_4

  EXPECT_EQ(issues[0].description,
            "authentication_fido was replaced by authentication_webauthn as of "
            "8.4.0 release - this must be corrected.");

  // WL#15973-TSFR_1_3_2
  temp_info.target_version = Version(8, 3, 0);
  check = Sql_upgrade_check::get_deprecated_auth_method_check(temp_info);

  EXPECT_ISSUES(check.get(), 1, remove_false_positives);
  EXPECT_EQ(issues[0].schema, "usr_fido_auth@localhost");

  EXPECT_EQ(issues[0].level, Upgrade_issue::Level::WARNING);

  EXPECT_EQ(
      issues[0].description,
      "authentication_fido is deprecated as of MySQL 8.2 and will be removed "
      "in a future release. Use authentication_webauthn instead.");

  // WL#15973-TSFR_1_3_3
  temp_info.target_version = Version(8, 2, 0);
  check = Sql_upgrade_check::get_deprecated_auth_method_check(temp_info);

  EXPECT_ISSUES(check.get(), 1, remove_false_positives);
  EXPECT_EQ(issues[0].schema, "usr_fido_auth@localhost");

  EXPECT_EQ(issues[0].level,
            Upgrade_issue::Level::WARNING);  // WL#15973-TSFR_1_5

  EXPECT_EQ(
      issues[0].description,
      "authentication_fido is deprecated as of MySQL 8.2 and will be removed "
      "in a future release. Use authentication_webauthn instead.");

  // WL#15973-TSFR_1_3_4
  temp_info.target_version = Version(8, 1, 0);
  check = Sql_upgrade_check::get_deprecated_auth_method_check(temp_info);

  EXPECT_ISSUES(check.get(), 1, remove_false_positives);
  EXPECT_EQ(issues[0].schema, "usr_fido_auth@localhost");

  EXPECT_EQ(issues[0].level, Upgrade_issue::Level::NOTICE);

  EXPECT_EQ(
      issues[0].description,
      "authentication_fido is deprecated as of MySQL 8.2 and will be removed "
      "in a future release. Consider using authentication_webauthn instead.");

  temp_info.target_version = Version(8, 0, 27);
  check = Sql_upgrade_check::get_deprecated_auth_method_check(temp_info);

  EXPECT_ISSUES(check.get(), 1, remove_false_positives);
  EXPECT_EQ(issues[0].schema, "usr_fido_auth@localhost");

  EXPECT_EQ(issues[0].level,
            Upgrade_issue::Level::NOTICE);  // WL#15973-TSFR_1_5

  EXPECT_EQ(
      issues[0].description,
      "authentication_fido is deprecated as of MySQL 8.2 and will be removed "
      "in a future release. Consider using authentication_webauthn instead.");

  ASSERT_NO_THROW(session->execute("drop user 'usr_fido_auth'@'localhost';"));
  ASSERT_NO_THROW(session->execute("drop schema if exists test;"));
}

namespace dep_def_auth_check {
void set_authentication_policy(std::shared_ptr<mysqlshdk::db::ISession> session,
                               const std::string &val) {
  ASSERT_NO_THROW(
      session->execute("SET GLOBAL authentication_policy='" + val + "';"));
}

std::unique_ptr<Sql_upgrade_check> make_check(const Version &ver) {
  Upgrade_check::Upgrade_info temp_info;
  temp_info.target_version = ver;
  return Deprecated_default_auth_check::get_deprecated_default_auth_check(
      temp_info);
}

}  // namespace dep_def_auth_check

TEST_F(MySQL_upgrade_check_test, deprecated_default_auth_check) {
  if (_target_server_version < Version(8, 0, 27)) {
    SKIP_TEST(
        "This test requires running against MySQL server version at least "
        "8.0.27.");
  }

  bool authentication_fido_installed = is_plugin_loaded("authentication_fido");
  bool uninstall_authentication_fido = false;
  if (!authentication_fido_installed) {
    authentication_fido_installed = install_plugin("authentication_fido");
    if (!authentication_fido_installed) {
      SKIP_TEST(
          "This test requires loaded authentication_fido MySQL Server plugin.");
    } else {
      uninstall_authentication_fido = true;
    }
  }

  shcore::on_leave_scope cleanup([this, uninstall_authentication_fido]() {
    dep_def_auth_check::set_authentication_policy(session, "*,,");
    if (uninstall_authentication_fido) {
      uninstall_plugin("authentication_fido");
    }
  });

  {
    dep_def_auth_check::set_authentication_policy(session,
                                                  "caching_sha2_password,*,");
    auto check = dep_def_auth_check::make_check(Version(8, 4, 0));
    EXPECT_NO_ISSUES(check.get());
  }

  auto run_check = [&](const std::string &auth_method) {
    const std::string k_test_var_pol_v = auth_method + ", authentication_fido,";

    dep_def_auth_check::set_authentication_policy(session, k_test_var_pol_v);

    auto check = dep_def_auth_check::make_check(Version(8, 5, 0));
    EXPECT_ISSUES(check.get(), 2);

    EXPECT_EQ(issues[0].schema, "authentication_policy");
    EXPECT_EQ(issues[1].schema, "authentication_policy");

    EXPECT_EQ(issues[0].level, Upgrade_issue::Level::ERROR);
    EXPECT_EQ(issues[1].level,
              Upgrade_issue::Level::ERROR);  // WL#15973-TSFR_2_4_2

    EXPECT_EQ(issues[0].description,
              auth_method +
                  " authentication method was removed and it must be corrected "
                  "before upgrading to 8.4.0 release.");
    EXPECT_EQ(
        issues[1].description,
        "authentication_fido was replaced by authentication_webauthn as of "
        "8.4.0 release - this must be corrected.");

    check = dep_def_auth_check::make_check(Version(8, 4, 0));
    EXPECT_ISSUES(check.get(), 2);

    EXPECT_EQ(issues[0].schema, "authentication_policy");
    EXPECT_EQ(issues[1].schema, "authentication_policy");

    EXPECT_EQ(issues[0].level,
              Upgrade_issue::Level::ERROR);  // WL#15973-TSFR_2_2_2
    EXPECT_EQ(issues[1].level,
              Upgrade_issue::Level::ERROR);  // WL#15973-TSFR_2_4_2

    EXPECT_EQ(issues[0].description,
              auth_method +
                  " authentication method was removed and it must be corrected "
                  "before upgrading to 8.4.0 release.");
    EXPECT_EQ(
        issues[1].description,
        "authentication_fido was replaced by authentication_webauthn as of "
        "8.4.0 release - this must be corrected.");

    check = dep_def_auth_check::make_check(Version(8, 3, 0));
    EXPECT_ISSUES(check.get(), 2);

    EXPECT_EQ(issues[0].schema, "authentication_policy");
    EXPECT_EQ(issues[1].schema, "authentication_policy");

    EXPECT_EQ(issues[0].level,
              Upgrade_issue::Level::WARNING);  // WL#15973-TSFR_2_3_3
    EXPECT_EQ(issues[1].level,
              Upgrade_issue::Level::WARNING);  // WL#15973-TSFR_2_5_2

    EXPECT_EQ(
        issues[0].description,
        auth_method +
            " authentication method is deprecated and it should be considered "
            "to correct this before upgrading to 8.4.0 release.");
    EXPECT_EQ(
        issues[1].description,
        "authentication_fido is deprecated as of MySQL 8.2 and will be removed "
        "in a future release. Use authentication_webauthn instead.");

    check = dep_def_auth_check::make_check(Version(8, 2, 0));
    EXPECT_ISSUES(check.get(), 2);

    EXPECT_EQ(issues[0].schema, "authentication_policy");
    EXPECT_EQ(issues[1].schema, "authentication_policy");

    EXPECT_EQ(issues[0].level, Upgrade_issue::Level::WARNING);
    EXPECT_EQ(issues[1].level,
              Upgrade_issue::Level::WARNING);  // WL#15973-TSFR_2_5_2

    EXPECT_EQ(
        issues[0].description,
        auth_method +
            " authentication method is deprecated and it should be considered "
            "to correct this before upgrading to 8.4.0 release.");
    EXPECT_EQ(
        issues[1].description,
        "authentication_fido is deprecated as of MySQL 8.2 and will be removed "
        "in a future release. Use authentication_webauthn instead.");

    check = dep_def_auth_check::make_check(Version(8, 1, 0));
    EXPECT_ISSUES(check.get(), 2);

    EXPECT_EQ(issues[0].schema, "authentication_policy");
    EXPECT_EQ(issues[1].schema, "authentication_policy");

    EXPECT_EQ(issues[0].level,
              Upgrade_issue::Level::WARNING);  // WL#15973-TSFR_2_3_4
    EXPECT_EQ(issues[1].level,
              Upgrade_issue::Level::NOTICE);  // WL#15973-TSFR_2_6_2

    EXPECT_EQ(
        issues[0].description,
        auth_method +
            " authentication method is deprecated and it should be considered "
            "to correct this before upgrading to 8.4.0 release.");
    EXPECT_EQ(
        issues[1].description,
        "authentication_fido is deprecated as of MySQL 8.2 and will be removed "
        "in a future release. Consider using authentication_webauthn instead.");

    check = dep_def_auth_check::make_check(Version(8, 0, 27));
    EXPECT_ISSUES(check.get(), 2);

    EXPECT_EQ(issues[0].schema, "authentication_policy");
    EXPECT_EQ(issues[1].schema, "authentication_policy");

    EXPECT_EQ(issues[0].level, Upgrade_issue::Level::WARNING);
    EXPECT_EQ(issues[1].level,
              Upgrade_issue::Level::NOTICE);  // WL#15973-TSFR_2_6_2

    EXPECT_EQ(
        issues[0].description,
        auth_method +
            " authentication method is deprecated and it should be considered "
            "to correct this before upgrading to 8.4.0 release.");
    EXPECT_EQ(
        issues[1].description,
        "authentication_fido is deprecated as of MySQL 8.2 and will be removed "
        "in a future release. Consider using authentication_webauthn instead.");
  };

  run_check("mysql_native_password");
  run_check("sha256_password");
}

TEST_F(MySQL_upgrade_check_test, deprecated_default_auth_parsing_check) {
  const std::string k_test_var_def = "default_authentication_plugin";
  const std::string k_test_var_def_v = "sha256_password";
  const std::string k_test_var_def_v_fido = "authentication_fido";

  std::vector<Upgrade_issue> temp_issues;

  auto check =
      std::make_unique<Deprecated_default_auth_check>(Version(8, 4, 0));
  check->parse_var(k_test_var_def, k_test_var_def_v_fido, &temp_issues);

  EXPECT_EQ(temp_issues.size(), 1);
  EXPECT_EQ(temp_issues[0].schema, "default_authentication_plugin");
  EXPECT_EQ(temp_issues[0].level,
            Upgrade_issue::Level::ERROR);  // WL#15973-TSFR_2_2_1

  check = std::make_unique<Deprecated_default_auth_check>(Version(8, 3, 0));
  temp_issues.clear();
  check->parse_var(k_test_var_def, k_test_var_def_v, &temp_issues);

  EXPECT_EQ(temp_issues.size(), 1);
  EXPECT_EQ(temp_issues[0].schema, "default_authentication_plugin");
  EXPECT_EQ(temp_issues[0].level,
            Upgrade_issue::Level::WARNING);  // WL#15973-TSFR_2_3_1

  check = std::make_unique<Deprecated_default_auth_check>(Version(8, 1, 0));
  temp_issues.clear();
  check->parse_var(k_test_var_def, k_test_var_def_v, &temp_issues);

  EXPECT_EQ(temp_issues.size(), 1);
  EXPECT_EQ(temp_issues[0].schema, "default_authentication_plugin");
  EXPECT_EQ(temp_issues[0].level,
            Upgrade_issue::Level::WARNING);  // WL#15973-TSFR_2_3_2

  check = std::make_unique<Deprecated_default_auth_check>(Version(8, 5, 0));
  temp_issues.clear();
  check->parse_var(k_test_var_def, k_test_var_def_v_fido, &temp_issues);

  EXPECT_EQ(temp_issues.size(), 1);
  EXPECT_EQ(temp_issues[0].schema, "default_authentication_plugin");
  EXPECT_EQ(temp_issues[0].level,
            Upgrade_issue::Level::ERROR);  // WL#15973-TSFR_2_4_1

  check = std::make_unique<Deprecated_default_auth_check>(Version(8, 4, 0));
  temp_issues.clear();
  check->parse_var(k_test_var_def, k_test_var_def_v_fido, &temp_issues);

  EXPECT_EQ(temp_issues.size(), 1);
  EXPECT_EQ(temp_issues[0].schema, "default_authentication_plugin");
  EXPECT_EQ(temp_issues[0].level,
            Upgrade_issue::Level::ERROR);  // WL#15973-TSFR_2_4_1

  check = std::make_unique<Deprecated_default_auth_check>(Version(8, 3, 0));
  temp_issues.clear();
  check->parse_var(k_test_var_def, k_test_var_def_v_fido, &temp_issues);

  EXPECT_EQ(temp_issues.size(), 1);
  EXPECT_EQ(temp_issues[0].schema, "default_authentication_plugin");
  EXPECT_EQ(temp_issues[0].level,
            Upgrade_issue::Level::WARNING);  // WL#15973-TSFR_2_5_1

  check = std::make_unique<Deprecated_default_auth_check>(Version(8, 2, 0));
  temp_issues.clear();
  check->parse_var(k_test_var_def, k_test_var_def_v_fido, &temp_issues);

  EXPECT_EQ(temp_issues.size(), 1);
  EXPECT_EQ(temp_issues[0].schema, "default_authentication_plugin");
  EXPECT_EQ(temp_issues[0].level,
            Upgrade_issue::Level::WARNING);  // WL#15973-TSFR_2_5_1

  check = std::make_unique<Deprecated_default_auth_check>(Version(8, 1, 0));
  temp_issues.clear();
  check->parse_var(k_test_var_def, k_test_var_def_v_fido, &temp_issues);

  EXPECT_EQ(temp_issues.size(), 1);
  EXPECT_EQ(temp_issues[0].schema, "default_authentication_plugin");
  EXPECT_EQ(temp_issues[0].level,
            Upgrade_issue::Level::NOTICE);  // WL#15973-TSFR_2_6_1

  check = std::make_unique<Deprecated_default_auth_check>(Version(8, 0, 27));
  temp_issues.clear();
  check->parse_var(k_test_var_def, k_test_var_def_v_fido, &temp_issues);

  EXPECT_EQ(temp_issues.size(), 1);
  EXPECT_EQ(temp_issues[0].schema, "default_authentication_plugin");
  EXPECT_EQ(temp_issues[0].level,
            Upgrade_issue::Level::NOTICE);  // WL#15973-TSFR_2_6_1
}

}  // namespace mysqlsh
