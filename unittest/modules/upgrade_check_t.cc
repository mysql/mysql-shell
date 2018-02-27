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

#include "modules/util/mod_util.h"
#include "modules/util/upgrade_check.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/test_utils.h"

using Version = mysqlshdk::utils::Version;

namespace mysqlsh {

class MySQL_upgrade_check_test : public Shell_core_test_wrapper {
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

  std::shared_ptr<mysqlshdk::db::ISession> session;
  std::string db;
};

TEST_F(MySQL_upgrade_check_test, checklist_generation) {
  EXPECT_THROW(Upgrade_check::create_checklist("5.7", "5.7"),
               std::invalid_argument);
  EXPECT_THROW(Upgrade_check::create_checklist("5.6.11", "8.0"),
               std::invalid_argument);
  EXPECT_THROW(Upgrade_check::create_checklist("5.7.19", "8.1.0"),
               std::invalid_argument);
  std::vector<std::unique_ptr<Upgrade_check> > checks;
  EXPECT_NO_THROW(checks = Upgrade_check::create_checklist("5.7.19", "8.0.3"));
  EXPECT_NO_THROW(checks = Upgrade_check::create_checklist("5.7.17", "8.0"));
  EXPECT_NO_THROW(checks = Upgrade_check::create_checklist("5.7", "8.0.4"));
}

TEST_F(MySQL_upgrade_check_test, old_temporal) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_old_temporal_check();
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session));
  EXPECT_TRUE(issues.empty());
  // No way to create test data in 5.7
}

TEST_F(MySQL_upgrade_check_test, reserved_keywords) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_reserved_keywords_check();
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session));
  for (auto& warning : issues)
    puts(to_string(warning).c_str());
  EXPECT_TRUE(issues.empty());

  PrepareTestDatabase("buckets");
  ASSERT_NO_THROW(
      session->execute("create table Clone(COMPONENT integer, cube int);"));
  ASSERT_NO_THROW(
      session->execute("create trigger first_value AFTER INSERT on Clone FOR "
                       "EACH ROW delete from Clone where COMPONENT<0;"));
  ASSERT_NO_THROW(
      session->execute("create view NTile as select * from Clone;"));
  ASSERT_NO_THROW(
      session->execute("CREATE FUNCTION others (s CHAR(20)) RETURNS CHAR(50) "
                       "DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');"));
  ASSERT_NO_THROW(session->execute(
      "CREATE EVENT UNBOUNDED ON SCHEDULE AT CURRENT_TIMESTAMP + INTERVAL 1 "
      "HOUR DO UPDATE Clone SET COMPONENT = COMPONENT + 1;"));

  ASSERT_NO_THROW(issues = check->run(session));
  ASSERT_EQ(10, issues.size());
  EXPECT_EQ("buckets", issues[0].schema);
  EXPECT_EQ(Upgrade_issue::WARNING, issues[0].level);
  EXPECT_STRCASEEQ("clone", issues[1].table.c_str());
  EXPECT_EQ("COMPONENT", issues[2].column);
  EXPECT_EQ("cube", issues[3].column);
  // Views columns are also displayed
  EXPECT_EQ("COMPONENT", issues[4].column);
  EXPECT_EQ("cube", issues[5].column);
  EXPECT_EQ("first_value", issues[6].table);
  EXPECT_STRCASEEQ("NTile", issues[7].table.c_str());
  EXPECT_EQ("others", issues[8].table);
  EXPECT_EQ("UNBOUNDED", issues[9].table);
}

TEST_F(MySQL_upgrade_check_test, utf8mb3) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("aaaaaaaaaaaaaaaa_utf8mb3");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_utf8mb3_check();
  std::vector<Upgrade_issue> issues;

  session->execute(
      "create table utf83 (s3 varchar(64) charset 'utf8mb3', s4 varchar(64) "
      "charset 'utf8mb4');");

  ASSERT_NO_THROW(issues = check->run(session));
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
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session));
  EXPECT_TRUE(issues.empty());

  ASSERT_NO_THROW(session->execute("use mysql;"));
  EXPECT_NO_THROW(session->execute("create table Role_edges (i integer);"));
  EXPECT_NO_THROW(session->execute("create table triggers (i integer);"));
  EXPECT_NO_THROW(issues = check->run(session));
  EXPECT_EQ(2, issues.size());
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
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session));
  EXPECT_TRUE(issues.empty());
  for (auto& warning : issues)
    puts(to_string(warning).c_str());

  ASSERT_NO_THROW(session->execute(
      "create table compact (i integer) row_format=compact engine=innodb;"));
  EXPECT_NO_THROW(issues = check->run(session));
  EXPECT_TRUE(issues.size() == 1 && issues[0].table == "compact");
  EXPECT_EQ(Upgrade_issue::WARNING, issues[0].level);
}

TEST_F(MySQL_upgrade_check_test, zerofill) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("aaa_test_zerofill_nondefaultwidth");
  std::unique_ptr<Sql_upgrade_check> check =
      Sql_upgrade_check::get_zerofill_check();
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session));
  // some tables in mysql schema use () syntax
  size_t old_count = issues.size();
  issues.clear();

  ASSERT_NO_THROW(session->execute(
      "create table zero_fill (zf INT zerofill, ti TINYINT(3), tu tinyint(2) "
      "unsigned, si smallint(3), su smallint(3) unsigned, mi mediumint(5), mu "
      "mediumint(5) unsigned, ii INT(4), iu INT(4) unsigned, bi bigint(10), bu "
      "bigint(12) unsigned);"));

  ASSERT_NO_THROW(issues = check->run(session));
  //  for (auto& warning : issues)
  //    puts(to_string(warning).c_str());
  ASSERT_EQ(11 + old_count, issues.size());
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
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check->run(session));
  EXPECT_TRUE(issues.empty());
  // No way to prepare test data in 5.7
}

TEST_F(MySQL_upgrade_check_test, check_table_command) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  PrepareTestDatabase("mysql_check_table_test");
  Check_table_command check;
  std::vector<Upgrade_issue> issues;
  ASSERT_NO_THROW(issues = check.run(session));
  EXPECT_TRUE(issues.empty());

  ASSERT_NO_THROW(
      session->execute("create table part(i integer) engine=myisam partition "
                       "by range(i) (partition p0 values less than (1000), "
                       "partition p1 values less than MAXVALUE);"));
  EXPECT_NO_THROW(issues = check.run(session));
  EXPECT_TRUE(issues.size() == 1 && issues[0].table == "part");
}

TEST_F(MySQL_upgrade_check_test, corner_cases_of_upgrade_check) {
  if (_target_server_version < Version(5, 7, 0) ||
      _target_server_version >= Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 5.7");
  Util util(_interactive_shell->shell_context().get(),
            _interactive_shell->get_console_handler(),
            _interactive_shell->options().wizards);
  shcore::Argument_list args;

  // valid mysql 5.7 superuser
  args.push_back(shcore::Value(_mysql_uri));
  try {
    util.check_for_server_upgrade(args);
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    EXPECT_TRUE(false);
  }
  args.clear();

  // new user with all privileges sans grant option and '%' in host
  EXPECT_NO_THROW(session->execute(
      "create user if not exists 'percent'@'%' identified by 'percent';"));
  std::string percent_uri = _mysql_uri;
  percent_uri.replace(0, 4, "percent:percent");
  args.push_back(shcore::Value(percent_uri));
  // No privileges function should throw
  EXPECT_THROW(util.check_for_server_upgrade(args), shcore::Exception);

  // Still not enough privileges
  EXPECT_NO_THROW(session->execute("grant SUPER on *.* to 'percent'@'%';"));
  EXPECT_THROW(util.check_for_server_upgrade(args), shcore::Exception);

  // Privileges check out we should succeed
  EXPECT_NO_THROW(session->execute("grant ALL on *.* to 'percent'@'%';"));
  EXPECT_NO_THROW(util.check_for_server_upgrade(args));

  EXPECT_NO_THROW(session->execute("drop user 'percent'@'%';"));
}

TEST_F(MySQL_upgrade_check_test, server_version_not_supported) {
  // session established with 8.0 server
  if (_target_server_version < Version(8, 0, 0))
    SKIP_TEST("This test requires running against MySQL server version 8.0");
  Util util(_interactive_shell->shell_context().get(),
            _interactive_shell->get_console_handler(),
            _interactive_shell->options().wizards);
  shcore::Argument_list args;
  args.push_back(shcore::Value(_mysql_uri));
  EXPECT_THROW(util.check_for_server_upgrade(args), shcore::Exception);
}


TEST_F(MySQL_upgrade_check_test, password_prompted) {
  Util util(_interactive_shell->shell_context().get(),
            _interactive_shell->get_console_handler(),
            _interactive_shell->options().wizards);
  shcore::Argument_list args;
  args.push_back(shcore::Value(_mysql_uri_nopasswd));

  output_handler.passwords.push_back({"Please provide the password for "
                            "'" + _mysql_uri_nopasswd + "': ", "WhAtEvEr"});
  EXPECT_THROW(util.check_for_server_upgrade(args), shcore::Exception);

  // Passwords are consumed if prompted, so verifying this indicates the
  // password was prompted as expected and consumed
  EXPECT_TRUE(output_handler.passwords.empty());
}

TEST_F(MySQL_upgrade_check_test, password_no_prompted) {
  Util util(_interactive_shell->shell_context().get(),
            _interactive_shell->get_console_handler(),
            _interactive_shell->options().wizards);
  shcore::Argument_list args;
  args.push_back(shcore::Value(_mysql_uri));

  output_handler.passwords.push_back({"If this was prompted it is an error",
                                      "WhAtEvEr"});

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
  Util util(_interactive_shell->shell_context().get(),
            _interactive_shell->get_console_handler(),
            _interactive_shell->options().wizards);
  shcore::Argument_list args;
  args.push_back(shcore::Value(_mysql_uri_nopasswd));

  EXPECT_THROW_LIKE(util.check_for_server_upgrade(args), shcore::Exception,
    "Util.checkForServerUpgrade: Missing password for "
    "'" + _mysql_uri_nopasswd + "'");
}
}  // namespace mysqlsh
