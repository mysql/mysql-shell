/* Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.

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

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <stack>

#include "gtest_clean.h"
#include "utils/utils_mysql_parsing.h"

namespace shcore {
namespace sql_shell_tests {
class TestMySQLSplitter : public ::testing::Test {
protected:
  std::stack<std::string> multiline_flags;
  mysql::splitter::Delimiters delimiters;
  std::vector<mysql::splitter::Statement_range> ranges;
  std::string sql;

  virtual void SetUp() {
    delimiters = mysql::splitter::Delimiters({";", "\\G", "\\g"});
  }

  void send_sql(const std::string &data) {
    // Sends the received sql and sets the list of command ranges denoted
    // by their position in the original query and a delimiter.
    sql = data;

    ranges = shcore::mysql::splitter::determineStatementRanges(sql.data(),
        sql.length(), delimiters, "\n", multiline_flags);
  }
};

TEST_F(TestMySQLSplitter, full_statement) {
  send_sql("show databases;");
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));
}

TEST_F(TestMySQLSplitter, full_statement_misc_delimiter) {
  send_sql("show databases\\G");
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("\\G", ranges[0].get_delimiter());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));
}

TEST_F(TestMySQLSplitter, by_line_continued_statement) {
  send_sql("show");
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(ranges[0].get_delimiter().empty());
  EXPECT_EQ("-", multiline_flags.top());
  EXPECT_EQ("show", sql.substr(ranges[0].offset(), ranges[0].length()));

  send_sql("databases");
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(ranges[0].get_delimiter().empty());
  EXPECT_EQ("-", multiline_flags.top());
  EXPECT_EQ("databases", sql.substr(ranges[0].offset(), ranges[0].length()));

  send_sql(";");
  EXPECT_EQ(1, ranges.size());
  EXPECT_EQ(";", ranges[0].get_delimiter());
  EXPECT_TRUE(multiline_flags.empty());
}

TEST_F(TestMySQLSplitter, by_line_continued_statement_misc_delimiter) {
  send_sql("show");
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(ranges[0].get_delimiter().empty());
  EXPECT_EQ("-", multiline_flags.top());
  EXPECT_EQ("show", sql.substr(ranges[0].offset(), ranges[0].length()));

  send_sql("databases");
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(ranges[0].get_delimiter().empty());
  EXPECT_EQ("-", multiline_flags.top());
  EXPECT_EQ("databases", sql.substr(ranges[0].offset(), ranges[0].length()));

  send_sql("\\G");
  EXPECT_EQ(1, ranges.size());
  EXPECT_EQ("\\G", ranges[0].get_delimiter());
  EXPECT_TRUE(multiline_flags.empty());
}

TEST_F(TestMySQLSplitter, script_continued_statement) {
  send_sql("show\ndatabases\n;\n");
  EXPECT_EQ(1, ranges.size());
  EXPECT_FALSE(ranges[0].get_delimiter().empty());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("show\ndatabases\n", sql.substr(ranges[0].offset(), ranges[0].length()));
}

TEST_F(TestMySQLSplitter, script_continued_statement_misc_delimiter) {
  send_sql("show\ndatabases\n\\G\n");
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("\\G", ranges[0].get_delimiter());
  EXPECT_EQ("show\ndatabases\n", sql.substr(ranges[0].offset(), ranges[0].length()));
}

TEST_F(TestMySQLSplitter, ignore_empty_statements) {
  send_sql("show databases;\n;");
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));

  send_sql(";");
  EXPECT_EQ(0, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
}

TEST_F(TestMySQLSplitter, ignore_empty_statements_misc_delimiter) {
  send_sql("show databases\\G\n\\G");
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));

  send_sql("\\G");
  EXPECT_EQ(0, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
}

TEST_F(TestMySQLSplitter, single_line_comments) {
  sql = "/* shows the database list */\n"
        "show databases;";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));

  sql = "-- shows the database list\n"
        "show databases;";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));

  sql = "--\tshows the database list\n"
    "show databases;";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));

  sql = "--\n"
        "show databases;";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));

  sql = "--";
  send_sql(sql);
  EXPECT_EQ(0, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());

  sql = "--this is an invalid comment, should be considered part of statement\n"
        "show databases;";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ(sql.substr(0, sql.length() - 1), sql.substr(ranges[0].offset(), ranges[0].length()));

  sql = "#this is an valid comment\n"
        "show databases;";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));

  sql = "show databases; #this is an valid comment\n";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));

  sql = "show databases\\G #this is an valid comment\n";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("\\G", ranges[0].get_delimiter());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));

  sql = "show databases\\G -- this is an valid comment\n";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("\\G", ranges[0].get_delimiter());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));

  sql = "--this is an invalid comment, state should indicate a continued statement";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(ranges[0].get_delimiter().empty());
  EXPECT_EQ("-", multiline_flags.top());
  EXPECT_EQ(sql, sql.substr(ranges[0].offset(), ranges[0].length()));
}

TEST_F(TestMySQLSplitter, multi_line_comments_in_batch) {
  sql = "/*\n"
    "this is a comment\n"
    "and should be ignored\n"
    "and ends here */\n"
    "show databases;";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ(";", ranges[0].get_delimiter());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));

  sql = "/*\n"
    "this is a comment\n"
    "and should be ignored\n"
    "and ends here */\n"
    "show databases\\G";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("\\G", ranges[0].get_delimiter());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));

  sql = "/*\n"
    "this is a comment\n"
    "and should be ignored\n"
    "and ends on next line\n"
    "*/\n"
    "show databases;";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_FALSE(ranges[0].get_delimiter().empty());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));
}

TEST_F(TestMySQLSplitter, multi_line_comments_line_by_line) {
  send_sql("/*");
  EXPECT_EQ(0, ranges.size());
  EXPECT_EQ("/*", multiline_flags.top());  // Multiline comment flag is set

  send_sql("This is a comment");
  EXPECT_EQ(0, ranges.size());
  EXPECT_EQ("/*", multiline_flags.top()); // Multiline comment flag continues

  send_sql("processed line by line");
  EXPECT_EQ(0, ranges.size());
  EXPECT_EQ("/*", multiline_flags.top()); // Multiline comment flag continues

  send_sql("and finishes next line");
  EXPECT_EQ(0, ranges.size());
  EXPECT_EQ("/*", multiline_flags.top()); // Multiline comment flag continues

  send_sql("*/");
  EXPECT_EQ(0, ranges.size());
  EXPECT_TRUE(multiline_flags.empty()); // Multiline comment flag is cleared

  send_sql("show databases;");
  EXPECT_EQ(1, ranges.size());
  EXPECT_FALSE(ranges[0].get_delimiter().empty());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));
}

TEST_F(TestMySQLSplitter, continued_backtick_string) {
  send_sql("select * from `t1");
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(ranges[0].get_delimiter().empty());
  EXPECT_EQ("`", multiline_flags.top());  // Multiline backtick flag is set
  EXPECT_EQ(sql, sql.substr(ranges[0].offset(), ranges[0].length()));

  send_sql("`;");
  EXPECT_EQ(1, ranges.size());
  EXPECT_FALSE(ranges[0].get_delimiter().empty());
  EXPECT_TRUE(multiline_flags.empty());  // Multiline backtick flag is cleared
  EXPECT_EQ("`", sql.substr(ranges[0].offset(), ranges[0].length()));
}

TEST_F(TestMySQLSplitter, continued_single_quote_string) {
  send_sql("select * from t1 where name = 'this");
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(ranges[0].get_delimiter().empty());
  EXPECT_EQ("'", multiline_flags.top());  // Multiline backtick flag is set
  EXPECT_EQ(sql, sql.substr(ranges[0].offset(), ranges[0].length()));

  send_sql("thing';");
  EXPECT_EQ(1, ranges.size());
  EXPECT_FALSE(ranges[0].get_delimiter().empty());
  EXPECT_TRUE(multiline_flags.empty());  // Multiline backtick flag is cleared
  EXPECT_EQ("thing'", sql.substr(ranges[0].offset(), ranges[0].length()));
}

TEST_F(TestMySQLSplitter, continued_double_quote_string) {
  send_sql("select * from t1 where name = 'this");
  EXPECT_EQ(1, ranges.size());
  EXPECT_TRUE(ranges[0].get_delimiter().empty());
  EXPECT_EQ("'", multiline_flags.top());  // Multiline backtick flag is set
  EXPECT_EQ(sql, sql.substr(ranges[0].offset(), ranges[0].length()));

  send_sql("\";");
  EXPECT_EQ(1, ranges.size());
  EXPECT_FALSE(ranges[0].get_delimiter().empty());
  EXPECT_TRUE(multiline_flags.empty());  // Multiline backtick flag is cleared
  EXPECT_EQ("\"", sql.substr(ranges[0].offset(), ranges[0].length()));
}

TEST_F(TestMySQLSplitter, delimiter_ignored_contexts) {
  sql = "#this is an valid comment, delimiter ; inside\n";
  send_sql(sql);
  EXPECT_EQ(0, ranges.size());

  sql = "-- this is an valid comment, delimiter ; inside\n";
  send_sql(sql);
  EXPECT_EQ(0, ranges.size());

  sql = "/* this is an valid comment, delimiter ; inside */\n";
  send_sql(sql);
  EXPECT_EQ(0, ranges.size());

  sql = "select ';' as a;";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_EQ("select ';' as a", sql.substr(ranges[0].offset(), ranges[0].length()));

  sql = "select `;` from x;";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_EQ("select `;` from x", sql.substr(ranges[0].offset(), ranges[0].length()));

  sql = "#this is an valid comment, delimiter \\G inside\n";
  send_sql(sql);
  EXPECT_EQ(0, ranges.size());

  sql = "-- this is an valid comment, delimiter \\G inside\n";
  send_sql(sql);
  EXPECT_EQ(0, ranges.size());

  sql = "/* this is an valid comment, delimiter \\G inside */\n";
  send_sql(sql);
  EXPECT_EQ(0, ranges.size());

  sql = "select '\\G' as a\\G";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_EQ("select '\\G' as a", sql.substr(ranges[0].offset(), ranges[0].length()));

  sql = "select `\\G` from x\\G";
  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_EQ("select `\\G` from x", sql.substr(ranges[0].offset(), ranges[0].length()));
}

TEST_F(TestMySQLSplitter, multiple_statements) {
  sql = "show databases;\n"
        "select * from whatever;\n"
        "drop database whatever;\n";
  send_sql(sql);
  EXPECT_EQ(3, ranges.size());
  for (int i = 0; i < 3; i++)
    EXPECT_FALSE(ranges[i].get_delimiter().empty());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));
  EXPECT_EQ("select * from whatever", sql.substr(ranges[1].offset(), ranges[1].length()));
  EXPECT_EQ("drop database whatever", sql.substr(ranges[2].offset(), ranges[2].length()));
}

TEST_F(TestMySQLSplitter, multiple_statements_misc_delimiter) {
  sql = "show databases\\G\n"
        "select * from whatever\\G\n"
        "drop database whatever\\G\n";
  send_sql(sql);
  EXPECT_EQ(3, ranges.size());
  for (int i = 0; i < 3; i++)
    EXPECT_EQ("\\G", ranges[i].get_delimiter());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));
  EXPECT_EQ("select * from whatever", sql.substr(ranges[1].offset(), ranges[1].length()));
  EXPECT_EQ("drop database whatever", sql.substr(ranges[2].offset(), ranges[2].length()));
}

TEST_F(TestMySQLSplitter, multiple_statements_one_line_misc_delimiter) {
  sql = "show databases\\Gselect * from whatever\\Gdrop database whatever\\G\n";
  send_sql(sql);
  EXPECT_EQ(3, ranges.size());
  for (int i = 0; i < 3; i++)
    EXPECT_EQ("\\G", ranges[i].get_delimiter());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));
  EXPECT_EQ("select * from whatever", sql.substr(ranges[1].offset(), ranges[1].length()));
  EXPECT_EQ("drop database whatever", sql.substr(ranges[2].offset(), ranges[2].length()));
}

TEST_F(TestMySQLSplitter, multiple_statements_mixed_delimiters) {
  sql = "show databases;\n"
        "select * from whatever\\G\n"
        "drop database whatever;\n";
  send_sql(sql);
  EXPECT_EQ(3, ranges.size());
  EXPECT_EQ(";", ranges[0].get_delimiter());
  EXPECT_EQ("\\G", ranges[1].get_delimiter());
  EXPECT_EQ(";", ranges[2].get_delimiter());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ("show databases", sql.substr(ranges[0].offset(), ranges[0].length()));
  EXPECT_EQ("select * from whatever", sql.substr(ranges[1].offset(), ranges[1].length()));
  EXPECT_EQ("drop database whatever", sql.substr(ranges[2].offset(), ranges[2].length()));
}

TEST_F(TestMySQLSplitter, delimiter_change) {
  send_sql("delimiter \\");
  EXPECT_EQ(0, ranges.size());
  EXPECT_EQ("\\", delimiters.get_main_delimiter());

  send_sql("delimiter ;");
  EXPECT_EQ(0, ranges.size());
  EXPECT_EQ(";", delimiters.get_main_delimiter());

  sql = "delimiter \\\n"
        "show databases;\n"
        "select * from whatever;\n"
        "drop database whatever;\n"
        "\\\n"
        "delimiter \\\n";

  std::string statement = "show databases;\n"
                          "select * from whatever;\n"
                          "drop database whatever;\n";

  send_sql(sql);
  EXPECT_EQ(1, ranges.size());
  EXPECT_FALSE(ranges[0].get_delimiter().empty());
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ(statement, sql.substr(ranges[0].offset(), ranges[0].length()));
}

TEST_F(TestMySQLSplitter, delimiter_change_misc_delimiter) {
  send_sql("show databases;");
  EXPECT_EQ(1, ranges.size());
  EXPECT_EQ(";", ranges[0].get_delimiter());
  EXPECT_EQ(";", delimiters.get_main_delimiter());
  send_sql("show databases\\G");
  EXPECT_EQ(1, ranges.size());
  EXPECT_EQ("\\G", ranges[0].get_delimiter());
  EXPECT_EQ(";", delimiters.get_main_delimiter());

  send_sql("delimiter //");
  EXPECT_EQ(0, ranges.size());
  EXPECT_EQ("//", delimiters.get_main_delimiter());

  send_sql("show databases//");
  EXPECT_EQ(1, ranges.size());
  EXPECT_EQ("//", ranges[0].get_delimiter());
  EXPECT_EQ("//", delimiters.get_main_delimiter());
  send_sql("show databases\\G");
  EXPECT_EQ(1, ranges.size());
  EXPECT_EQ("//", delimiters.get_main_delimiter());
  EXPECT_EQ("\\G", ranges[0].get_delimiter());

  send_sql("show databases; select 11 as a\\G select 12 as b//");
  EXPECT_EQ(2, ranges.size());
  EXPECT_EQ("show databases; select 11 as a",
      sql.substr(ranges[0].offset(), ranges[0].length()));
  EXPECT_EQ("select 12 as b",
      sql.substr(ranges[1].offset(), ranges[1].length()));
}

TEST_F(TestMySQLSplitter, single_line_mysql_extension)
{
  sql = "/*! SET TIME_ZONE='+00:00' */;\n"
        "show databases;";
  send_sql(sql);
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ(2, ranges.size());
  EXPECT_EQ("/*! SET TIME_ZONE='+00:00' */", sql.substr(ranges[0].offset(),
            ranges[0].length()));
  EXPECT_EQ("show databases", sql.substr(ranges[1].offset(),
            ranges[1].length()));
}

TEST_F(TestMySQLSplitter, multi_line_mysql_extension_in_batch) {
  std::string view = "/*!50001 CREATE VIEW `ActorLimit10` AS SELECT "\
    "1 AS `actor_id`,\n"\
    "1 AS `first_name`,\n"\
    "1 AS `last_name`,\n"\
    "1 AS `last_update`*/";
  sql = view + ";\nshow databases;";

  send_sql(sql);
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ(2, static_cast<int>(ranges.size()));
  EXPECT_EQ(view, sql.substr(ranges[0].offset(), ranges[0].length()));
  EXPECT_EQ("show databases", sql.substr(ranges[1].offset(), ranges[1].length()));
}

TEST_F(TestMySQLSplitter, multi_line_mysql_extension_line_by_line) {
  send_sql("/*!50001 CREATE VIEW `ActorLimit10`");
  EXPECT_EQ("/*!", multiline_flags.top());  // Multiline comment flag is set
  EXPECT_EQ(1, static_cast<int>(ranges.size()));
  std::string expected = "/*!50001 CREATE VIEW `ActorLimit10`";
  EXPECT_EQ(expected, sql.substr(ranges[0].offset(), ranges[0].length()));

  send_sql("1 AS `actor_id`,");
  EXPECT_EQ("/*!", multiline_flags.top()); // Multiline comment flag continues
  EXPECT_EQ(1, static_cast<int>(ranges.size()));
  expected = "1 AS `actor_id`,";
  EXPECT_EQ(expected, sql.substr(ranges[0].offset(), ranges[0].length()));

  send_sql("1 AS `last_name`,");
  EXPECT_EQ("/*!", multiline_flags.top()); // Multiline comment flag continues
  EXPECT_EQ(1, static_cast<int>(ranges.size()));
  expected = "1 AS `last_name`,";
  EXPECT_EQ(expected, sql.substr(ranges[0].offset(), ranges[0].length()));

  send_sql("1 AS `last_update`*/;");
  EXPECT_TRUE(multiline_flags.empty()); // Multiline comment flag is done
  EXPECT_EQ(1, static_cast<int>(ranges.size()));
  expected = "1 AS `last_update`*/";
  EXPECT_EQ(expected, sql.substr(ranges[0].offset(), ranges[0].length()));
}

TEST_F(TestMySQLSplitter, single_line_optimizer_hint)
{
  sql = "/*+ NO_RANGE_OPTIMIZATION(t3 PRIMARY, f2_idx) */;\n"
        "show databases;";
  send_sql(sql);
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ(2, ranges.size());
  EXPECT_EQ("/*+ NO_RANGE_OPTIMIZATION(t3 PRIMARY, f2_idx) */",
            sql.substr(ranges[0].offset(),ranges[0].length()));
  EXPECT_EQ("show databases", sql.substr(ranges[1].offset(),
            ranges[1].length()));
}

TEST_F(TestMySQLSplitter, multi_line_optimizer_hint_in_batch) {
  std::string optimizer = "/*+ BKA(t1) "\
    "NO_BKA(t2)*/";
  sql = optimizer + ";\nshow databases;";

  send_sql(sql);
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ(2, static_cast<int>(ranges.size()));
  EXPECT_EQ(optimizer, sql.substr(ranges[0].offset(), ranges[0].length()));
  EXPECT_EQ("show databases", sql.substr(ranges[1].offset(), ranges[1].length()));
}

TEST_F(TestMySQLSplitter, multi_line_optimizer_hint_line_by_line) {
  send_sql("/*+ BKA(t1) ");
  EXPECT_EQ("/*+", multiline_flags.top());  // Multiline comment flag is set
  EXPECT_EQ(1, static_cast<int>(ranges.size()));
  std::string expected = "/*+ BKA(t1) ";
  EXPECT_EQ(expected, sql.substr(ranges[0].offset(), ranges[0].length()));

  send_sql("NO_BKA(t2)*/;");
  EXPECT_TRUE(multiline_flags.empty()); // Multiline comment flag is done
  EXPECT_EQ(1, static_cast<int>(ranges.size()));
  expected = "NO_BKA(t2)*/";
  EXPECT_EQ(expected, sql.substr(ranges[0].offset(), ranges[0].length()));
}

TEST_F(TestMySQLSplitter, continued_stmt_multiline_comment) {

  send_sql("SELECT 1 AS _one /*");
  EXPECT_EQ("-", multiline_flags.top());  // Continued statement flag
  EXPECT_EQ(1, static_cast<int>(ranges.size()));

  // The first range is the incomplete statement
  std::string expected = "SELECT 1 AS _one /*";
  EXPECT_EQ(expected, sql.substr(ranges[0].offset(), ranges[0].length()));

  send_sql("comment text */;");
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ(1, static_cast<int>(ranges.size()));

  // The first range is the incomplete statement
  expected = "comment text */";
  EXPECT_EQ(expected, sql.substr(ranges[0].offset(), ranges[0].length()));
}


TEST_F(TestMySQLSplitter, continued_stmt_dash_dash_comment) {
  send_sql("select 1 as one -- sample text");
  EXPECT_EQ("-", multiline_flags.top()); // Continued statement flag
  EXPECT_EQ(1, static_cast<int>(ranges.size()));

  // The first range is the incomplete statement
  std::string expected = "select 1 as one ";
  EXPECT_EQ(expected, sql.substr(ranges[0].offset(), ranges[0].length()));

  send_sql(";select 2 as two;");
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ(2, static_cast<int>(ranges.size()));

  // The first range is an empty statement indicating the previous one is done
  EXPECT_EQ("", sql.substr(ranges[0].offset(), ranges[0].length()));

  // The second range is the second statement
  expected = "select 2 as two";
  EXPECT_EQ(expected, sql.substr(ranges[1].offset(), ranges[1].length()));
}

TEST_F(TestMySQLSplitter, continued_stmt_dash_dash_comment_batch) {
  send_sql("select 1 as one -- sample text\n;select 2 as two;");
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ(3, static_cast<int>(ranges.size()));

  // The first range is the incomplete statement
  std::string expected = "select 1 as one ";
  EXPECT_EQ(expected, sql.substr(ranges[0].offset(), ranges[0].length()));

  // The second range is the empty range for the delimiter on the second line
  EXPECT_EQ("", sql.substr(ranges[1].offset(), ranges[1].length()));

  // The third range is the second statement
  expected = "select 2 as two";
  EXPECT_EQ(expected, sql.substr(ranges[2].offset(), ranges[2].length()));
}

TEST_F(TestMySQLSplitter, continued_stmt_hash_comment) {
  send_sql("select 1 as one #sample text");
  EXPECT_EQ("-", multiline_flags.top()); // Continued statement flag
  EXPECT_EQ(1, static_cast<int>(ranges.size()));

  // The first range is the incomplete statement
  std::string expected = "select 1 as one ";
  EXPECT_EQ(expected, sql.substr(ranges[0].offset(), ranges[0].length()));

  send_sql(";select 2 as two;");
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ(2, static_cast<int>(ranges.size()));

  // The first range is an empty statement indicating the previous one is done
  EXPECT_EQ("", sql.substr(ranges[0].offset(), ranges[0].length()));

  // The second range is the second statement
  expected = "select 2 as two";
  EXPECT_EQ(expected, sql.substr(ranges[1].offset(), ranges[1].length()));
}

TEST_F(TestMySQLSplitter, continued_stmt_hash_comment_batch) {
  send_sql("select 1 as one #sample text\n;select 2 as two;");
  EXPECT_TRUE(multiline_flags.empty());
  EXPECT_EQ(3, static_cast<int>(ranges.size()));

  // The first range is the incomplete statement
  std::string expected = "select 1 as one ";
  EXPECT_EQ(expected, sql.substr(ranges[0].offset(), ranges[0].length()));

  // The second range is the empty range for the delimiter on the second line
  EXPECT_EQ("", sql.substr(ranges[1].offset(), ranges[1].length()));

  // The third range is the second statement
  expected = "select 2 as two";
  EXPECT_EQ(expected, sql.substr(ranges[2].offset(), ranges[2].length()));
}

}
}
