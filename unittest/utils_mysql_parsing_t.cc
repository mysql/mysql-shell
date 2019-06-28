/* Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.

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
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils/shell_test_env.h"
#include "utils/utils_mysql_parsing.h"

namespace mysqlshdk {
namespace utils {

// Legacy tests with adaptations
class TestMySQLSplitter : public ::testing::Test {
 protected:
  bool debug = false;
  std::vector<std::tuple<std::string, std::string, size_t>> results;
  std::string sql;
  std::string delim = ";";
  std::string context;
  std::string buffer;
  std::unique_ptr<Sql_splitter> splitter;
  std::string multiline;

  void SetUp() override {
    splitter.reset(new Sql_splitter(
        [this](const char *s, size_t len, bool bol,
               size_t lnum) -> std::pair<size_t, bool> {
          if (!bol) len = 2;
          if (s[1] != 'g' && s[1] != 'G') {
            results.emplace_back(std::string(len, 2), "", lnum);
            return std::make_pair(len, false);
          }
          return std::make_pair(2U, true);
        },
        [this](const std::string &err) { results.emplace_back(err, "", 0); }));
  }

  void send_sql(const std::string &data, bool dont_flush = false) {
    // Sends the received sql and sets the list of command results denoted
    // by their position in the original query and a delimiter.
    results.clear();
    splitter->reset();
    buffer.clear();
    delim = ";";
    sql = data;
    std::stringstream str(data);
    split_incremental(&str, data.size(), dont_flush);
  }

  void send_sql_incr(const std::string &data, bool dont_flush = true) {
    // Sends the received sql and sets the list of command results denoted
    // by their position in the original query and a delimiter.
    sql = data;
    std::stringstream str(data);
    if (debug) std::cout << "\n" << data << "\n";
    split_incremental(&str, data.size(), dont_flush);
  }

  void split_incremental(std::iostream *stream, size_t chunk_size,
                         bool dont_flush) {
    assert(chunk_size > 0);

    splitter->set_delimiter(delim);

    size_t offset = buffer.size();

    buffer.resize(offset + chunk_size);
    stream->read(&buffer[offset], chunk_size);
    buffer.resize(offset + stream->gcount());
    splitter->feed_chunk(&buffer[0], buffer.size());

    while (!splitter->eof()) {
      Sql_splitter::Range range;
      std::string delim;

      if (splitter->next_range(&range, &delim)) {
        if (debug)
          std::cout << buffer.substr(range.offset, range.length) << delim
                    << "\n";
        results.emplace_back(buffer.substr(range.offset, range.length), delim,
                             range.line_num);
      } else {
        splitter->pack_buffer(&buffer, range);

        size_t osize = buffer.size();
        buffer.resize(osize + chunk_size);
        stream->read(&buffer[osize], chunk_size);
        buffer.resize(osize + stream->gcount());

        if (static_cast<size_t>(stream->gcount()) < chunk_size && !dont_flush) {
          splitter->feed(&buffer[0], buffer.size());
        } else {
          if (stream->gcount() == 0) break;
          splitter->feed_chunk(&buffer[0], buffer.size());
        }
      }
    }

    delim = splitter->delimiter();
    context = to_string(splitter->context());
  }
};

TEST_F(TestMySQLSplitter, full_statement) {
  send_sql("show databases;");
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ("show databases", std::get<0>(results[0]));
}

TEST_F(TestMySQLSplitter, full_statement_misc_delimiter) {
  send_sql("show databases\\G");
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ("\\G", std::get<1>(results[0]));
  EXPECT_EQ("show databases", std::get<0>(results[0]));
}

TEST_F(TestMySQLSplitter, by_line_continued_statement) {
  send_sql_incr("show ");
  ASSERT_EQ(0, results.size());
  EXPECT_EQ("-", context);

  send_sql_incr("databases");
  ASSERT_EQ(0, results.size());
  EXPECT_EQ("-", context);

  send_sql_incr(";");
  ASSERT_EQ(1, results.size());
  EXPECT_EQ(";", std::get<1>(results[0]));
  EXPECT_EQ("", context);
  EXPECT_EQ("show databases", std::get<0>(results[0]));
}

TEST_F(TestMySQLSplitter, by_line_continued_statement_misc_delimiter) {
  send_sql_incr("show ");
  ASSERT_EQ(0, results.size());
  EXPECT_EQ("-", context);

  send_sql_incr("databases");
  ASSERT_EQ(0, results.size());
  EXPECT_EQ("-", context);

  send_sql_incr("\\G");
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("\\G", std::get<1>(results[0]));
  EXPECT_EQ("", context);
  EXPECT_EQ("show databases", std::get<0>(results[0]));
}

TEST_F(TestMySQLSplitter, script_continued_statement) {
  send_sql("show\ndatabases\n;\n");
  ASSERT_EQ(1, results.size());
  EXPECT_FALSE(std::get<1>(results[0]).empty());
  EXPECT_EQ("", context);
  EXPECT_EQ("show\ndatabases\n", std::get<0>(results[0]));
}

TEST_F(TestMySQLSplitter, script_continued_statement_misc_delimiter) {
  send_sql("show\ndatabases\n\\G\n");
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ("\\G", std::get<1>(results[0]));
  EXPECT_EQ("show\ndatabases\n", std::get<0>(results[0]));
}

TEST_F(TestMySQLSplitter, no_ignore_empty_statements) {
  send_sql("show databases;\n;");
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ("show databases", std::get<0>(results[0]));
  EXPECT_EQ("", std::get<0>(results[1]));

  send_sql(";");
  EXPECT_EQ(1, results.size());
  EXPECT_EQ("", context);

  send_sql("show databases\\G;\\G");
  EXPECT_EQ(3, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ("show databases", std::get<0>(results[0]));

  send_sql("\\G#");
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("", context);

  send_sql("\\G #");
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("", context);

  send_sql(";;");
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("", context);

  send_sql(";;;");
  EXPECT_EQ(3, results.size());
  EXPECT_EQ("", context);

  send_sql("show databases;;");
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ("show databases", std::get<0>(results[0]));

  send_sql("show databases;;;");
  EXPECT_EQ(3, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ("show databases", std::get<0>(results[0]));

  send_sql("show databases;\\G;");
  EXPECT_EQ(3, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ("show databases", std::get<0>(results[0]));

  send_sql(";#");
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("", context);

  send_sql("; #");
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("", context);
}

TEST_F(TestMySQLSplitter, single_line_comments) {
  sql =
      "/* shows the database list */\n"
      "show databases;";
  send_sql(sql);
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ(sql.substr(0, sql.length() - 1), std::get<0>(results[0]));

  sql =
      "-- shows the database list\n"
      "show databases;";
  send_sql(sql);
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ("-- shows the database list", std::get<0>(results[0]));
  EXPECT_EQ("show databases", std::get<0>(results[1]));

  sql =
      "--\tshows the database list\n"
      "show databases;";
  send_sql(sql);
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ("--\tshows the database list", std::get<0>(results[0]));
  EXPECT_EQ("show databases", std::get<0>(results[1]));

  sql =
      "--\n"
      "show databases;";
  send_sql(sql);
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ("--", std::get<0>(results[0]));
  EXPECT_EQ("show databases", std::get<0>(results[1]));

  sql = "--";
  send_sql(sql, true);
  ASSERT_EQ(0, results.size());
  EXPECT_EQ("-", context);  // -- comments are terminated by a newline

  sql =
      "--this is an invalid comment, should be considered part of statement\n"
      "show databases;";
  send_sql(sql);
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ(sql.substr(0, sql.length() - 1), std::get<0>(results[0]));

  sql =
      "#this is an valid comment\n"
      "show databases;";
  send_sql(sql);
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ("#this is an valid comment", std::get<0>(results[0]));
  EXPECT_EQ("show databases", std::get<0>(results[1]));

  sql = "show databases; #this is an valid comment\n";
  send_sql(sql);
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ("show databases", std::get<0>(results[0]));
  EXPECT_EQ("#this is an valid comment", std::get<0>(results[1]));

  sql = "show databases\\G #this is an valid comment\n";
  send_sql(sql);
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ("\\G", std::get<1>(results[0]));
  EXPECT_EQ("show databases", std::get<0>(results[0]));
  EXPECT_EQ("#this is an valid comment", std::get<0>(results[1]));

  sql = "show databases\\G -- this is an valid comment\n";
  send_sql(sql);
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ("\\G", std::get<1>(results[0]));
  EXPECT_EQ("show databases", std::get<0>(results[0]));
  EXPECT_EQ("-- this is an valid comment", std::get<0>(results[1]));

  sql =
      "--this is an invalid comment, state should indicate a continued "
      "statement";
  send_sql(sql, true);
  ASSERT_EQ(0, results.size());
  EXPECT_EQ("-", context);
}

TEST_F(TestMySQLSplitter, multi_line_comments_in_batch) {
  sql =
      "/*\n"
      "this is a comment\n"
      "and should be ignored\n"
      "and ends here */\n"
      "show databases;";
  send_sql(sql);
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ(";", std::get<1>(results[0]));
  EXPECT_EQ(sql.substr(0, sql.length() - 1), std::get<0>(results[0]));

  sql =
      "/*\n"
      "this is a comment;\n"
      "and should be ignored;\n"
      "* foo; bar\n"
      "and ends here */\n"
      "show databases;";
  send_sql(sql);
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ(";", std::get<1>(results[0]));
  EXPECT_EQ(sql.substr(0, sql.length() - 1), std::get<0>(results[0]));

  sql =
      "/*\n"
      "this is a comment\n"
      "and should be ignored\n"
      "and ends here */\n"
      "show databases\\G";
  send_sql(sql);
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("", context);
  EXPECT_EQ("\\G", std::get<1>(results[0]));
  EXPECT_EQ(sql.substr(0, sql.length() - 2), std::get<0>(results[0]));

  sql =
      "/*\n"
      "this is a comment\n"
      "and should be ignored\n"
      "and ends on next line\n"
      "*/\n"
      "show databases;";
  send_sql(sql);
  send_sql_incr("*/");
  ASSERT_EQ(1, results.size());
  // should continue until delimiter since we're not trimming comments
  // EXPECT_EQ("", context);  // Multiline comment flag is cleared

  send_sql_incr("show databases;");
  ASSERT_EQ(2, results.size());
  EXPECT_FALSE(std::get<1>(results[0]).empty());
  EXPECT_EQ("", context);
  EXPECT_EQ(
      "/*\nthis is a comment\nand should be ignored\nand ends on next "
      "line\n*/\nshow databases",
      std::get<0>(results[0]));

  sql =
      "/* comment */# comment\n"
      "show databases;";
  send_sql(sql);
  ASSERT_EQ(2, results.size());
  EXPECT_FALSE(std::get<1>(results[1]).empty());

  EXPECT_EQ("/* comment */# comment", std::get<0>(results[0]));
  EXPECT_EQ("show databases", std::get<0>(results[1]));

  sql =
      "/* comment */ # comment\n"
      "show databases;";
  send_sql(sql);
  ASSERT_EQ(2, results.size());
  EXPECT_FALSE(std::get<1>(results[1]).empty());

  EXPECT_EQ("/* comment */ # comment", std::get<0>(results[0]));
  EXPECT_EQ("show databases", std::get<0>(results[1]));
}

TEST_F(TestMySQLSplitter, continued_backtick_string) {
  send_sql_incr("select * from `t1");
  ASSERT_EQ(0, results.size());
  // EXPECT_TRUE(std::get<1>(results[0]).empty());
  EXPECT_EQ("`", context);  // Multiline backtick flag is set
  // EXPECT_EQ(sql, std::get<0>(results[0]));

  send_sql_incr("`;");
  ASSERT_EQ(1, results.size());
  EXPECT_FALSE(std::get<1>(results[0]).empty());
  EXPECT_EQ("", context);  // Multiline backtick flag is cleared
  EXPECT_EQ("select * from `t1`", std::get<0>(results[0]));
}

TEST_F(TestMySQLSplitter, continued_single_quote_string) {
  send_sql_incr("select * from t1 where name = 'this");
  ASSERT_EQ(0, results.size());
  // EXPECT_TRUE(std::get<1>(results[0]).empty());
  EXPECT_EQ("'", context);  // Multiline backtick flag is set
  // EXPECT_EQ(sql, std::get<0>(results[0]));

  send_sql_incr("thing';");
  ASSERT_EQ(1, results.size());
  EXPECT_FALSE(std::get<1>(results[0]).empty());
  EXPECT_EQ("", context);  // Multiline backtick flag is cleared
  EXPECT_EQ("select * from t1 where name = 'thisthing'",
            std::get<0>(results[0]));
}

TEST_F(TestMySQLSplitter, continued_double_quote_string) {
  send_sql_incr("select * from t1 where name = \"this");
  ASSERT_EQ(0, results.size());
  // EXPECT_TRUE(std::get<1>(results[0]).empty());
  EXPECT_EQ("\"", context);  // Multiline backtick flag is set
  // EXPECT_EQ(sql, std::get<0>(results[0]));

  send_sql_incr("\";");
  ASSERT_EQ(1, results.size());
  EXPECT_FALSE(std::get<1>(results[0]).empty());
  EXPECT_EQ("", context);  // Multiline backtick flag is cleared
  EXPECT_EQ("select * from t1 where name = \"this\"", std::get<0>(results[0]));
}

TEST_F(TestMySQLSplitter, delimiter_ignored_contexts) {
  sql = "#this is an valid comment, delimiter ; inside\n";
  send_sql(sql);
  ASSERT_EQ(1, results.size());

  sql = "-- this is an valid comment, delimiter ; inside\n";
  send_sql(sql);
  ASSERT_EQ(1, results.size());

  auto prev_context = context;
  sql = "/* this is an valid comment, delimiter ; inside */\n";
  send_sql(sql, true);
  ASSERT_EQ(0, results.size());
  EXPECT_EQ(prev_context, context);

  prev_context = context;
  sql = "/* this is an valid comment, delimiter ; inside */\n";
  send_sql(sql, false);
  ASSERT_EQ(1, results.size());
  EXPECT_EQ(prev_context, context);

  sql = "select ';' as a;";
  send_sql(sql);
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("select ';' as a", std::get<0>(results[0]));

  sql = "select `;` from x;";
  send_sql(sql);
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("select `;` from x", std::get<0>(results[0]));

  sql = "#this is an valid comment, delimiter \\G inside\n";
  send_sql(sql);
  ASSERT_EQ(1, results.size());

  sql = "-- this is an valid comment, delimiter \\G inside\n";
  send_sql(sql);
  ASSERT_EQ(1, results.size());

  sql = "/* this is an valid comment, delimiter \\G inside */\n";
  send_sql(sql, true);
  ASSERT_EQ(0, results.size());

  sql = "select '\\G' as a\\G";
  send_sql(sql);
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("select '\\G' as a", std::get<0>(results[0]));

  sql = "select `\\G` from x\\G";
  send_sql(sql);
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("select `\\G` from x", std::get<0>(results[0]));
}

TEST_F(TestMySQLSplitter, multiple_statements) {
  sql =
      "show databases;\n"
      "select * from whatever;\n"
      "drop database whatever;\n";
  send_sql(sql);
  EXPECT_EQ(3, results.size());
  for (int i = 0; i < 3; i++) EXPECT_FALSE(std::get<1>(results[i]).empty());
  EXPECT_EQ("", context);
  EXPECT_EQ("show databases", std::get<0>(results[0]));
  EXPECT_EQ("select * from whatever", std::get<0>(results[1]));
  EXPECT_EQ("drop database whatever", std::get<0>(results[2]));
}

TEST_F(TestMySQLSplitter, multiple_statements_misc_delimiter) {
  sql =
      "show databases\\G\n"
      "select * from whatever\\G\n"
      "drop database whatever\\G\n";
  send_sql(sql);
  EXPECT_EQ(3, results.size());
  for (int i = 0; i < 3; i++) EXPECT_EQ("\\G", std::get<1>(results[i]));
  EXPECT_EQ("", context);
  EXPECT_EQ("show databases", std::get<0>(results[0]));
  EXPECT_EQ("select * from whatever", std::get<0>(results[1]));
  EXPECT_EQ("drop database whatever", std::get<0>(results[2]));
}

TEST_F(TestMySQLSplitter, multiple_statements_one_line_misc_delimiter) {
  sql = "show databases\\Gselect * from whatever\\Gdrop database whatever\\G\n";
  send_sql(sql);
  EXPECT_EQ(3, results.size());
  for (int i = 0; i < 3; i++) EXPECT_EQ("\\G", std::get<1>(results[i]));
  EXPECT_EQ("", context);
  EXPECT_EQ("show databases", std::get<0>(results[0]));
  EXPECT_EQ("select * from whatever", std::get<0>(results[1]));
  EXPECT_EQ("drop database whatever", std::get<0>(results[2]));
}

TEST_F(TestMySQLSplitter, multiple_statements_mixed_delimiters) {
  sql =
      "show databases;\n"
      "select * from whatever\\G\n"
      "drop database whatever;\n";
  send_sql(sql);
  EXPECT_EQ(3, results.size());
  EXPECT_EQ(";", std::get<1>(results[0]));
  EXPECT_EQ("\\G", std::get<1>(results[1]));
  EXPECT_EQ(";", std::get<1>(results[2]));
  EXPECT_EQ("", context);
  EXPECT_EQ("show databases", std::get<0>(results[0]));
  EXPECT_EQ("select * from whatever", std::get<0>(results[1]));
  EXPECT_EQ("drop database whatever", std::get<0>(results[2]));
}

TEST_F(TestMySQLSplitter, delimiter_change) {
  send_sql("delimiter \\\n");
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("DELIMITER cannot contain a backslash character",
            std::get<0>(results[0]));
  EXPECT_EQ(";", delim);

  send_sql("delimiter ;");
  ASSERT_EQ(0, results.size());
  EXPECT_EQ(";", delim);

  sql =
      "delimiter $\n"
      "show databases;\n"
      "select * from whatever;\n"
      "drop database whatever;\n"
      "$\n"
      "delimiter $\n";

  std::string statement =
      "show databases;\n"
      "select * from whatever;\n"
      "drop database whatever;\n";

  send_sql(sql);
  ASSERT_EQ(1, results.size());
  EXPECT_FALSE(std::get<1>(results[0]).empty());
  EXPECT_EQ("", context);
  EXPECT_EQ(statement, std::get<0>(results[0]));
}

TEST_F(TestMySQLSplitter, delimiter_change_misc_delimiter) {
  send_sql("show databases;", true);
  ASSERT_EQ(1, results.size());
  EXPECT_EQ(";", std::get<1>(results[0]));
  EXPECT_EQ(";", delim);
  send_sql("show databases\\G", true);
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("\\G", std::get<1>(results[0]));
  EXPECT_EQ(";", delim);

  send_sql("delimiter //\n", true);
  ASSERT_EQ(0, results.size());
  EXPECT_EQ("//", delim);

  send_sql_incr("show databases//");
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("//", std::get<1>(results[0]));
  EXPECT_EQ("//", delim);
  results.clear();
  send_sql_incr("show databases\\G");
  ASSERT_EQ(1, results.size());
  EXPECT_EQ("//", delim);
  EXPECT_EQ("\\G", std::get<1>(results[0]));

  results.clear();
  send_sql_incr("show databases; select 11 as a\\G select 12 as b//");
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("show databases; select 11 as a", std::get<0>(results[0]));
  EXPECT_EQ("select 12 as b", std::get<0>(results[1]));
}

TEST_F(TestMySQLSplitter, single_line_mysql_extension) {
  sql =
      "/*! SET TIME_ZONE='+00:00' */;\n"
      "show databases;";
  send_sql(sql);
  EXPECT_EQ("", context);
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("/*! SET TIME_ZONE='+00:00' */", std::get<0>(results[0]));
  EXPECT_EQ("show databases", std::get<0>(results[1]));
}

TEST_F(TestMySQLSplitter, multi_line_mysql_extension_in_batch) {
  std::string view =
      "/*!50001 CREATE VIEW `ActorLimit10` AS SELECT "
      "1 AS `actor_id`,\n"
      "1 AS `first_name`,\n"
      "1 AS `last_name`,\n"
      "1 AS `last_update`*/";
  sql = view + ";\nshow databases;";

  send_sql(sql);
  EXPECT_EQ("", context);
  EXPECT_EQ(2, static_cast<int>(results.size()));
  EXPECT_EQ(view, std::get<0>(results[0]));
  EXPECT_EQ("show databases", std::get<0>(results[1]));
}

TEST_F(TestMySQLSplitter, multi_line_mysql_extension_line_by_line) {
  send_sql_incr("/*!50001 CREATE VIEW `ActorLimit10`");
  EXPECT_EQ("-", context);  // Multiline comment flag is set
  EXPECT_EQ(0, static_cast<int>(results.size()));
  std::string expected = "/*!50001 CREATE VIEW `ActorLimit10`";
  // EXPECT_EQ(expected, std::get<0>(results[0]));

  send_sql_incr("1 AS `actor_id`,");
  EXPECT_EQ("-", context);  // Multiline comment flag continues
  EXPECT_EQ(0, static_cast<int>(results.size()));
  expected += "1 AS `actor_id`,";
  // EXPECT_EQ(expected, std::get<0>(results[0]));

  send_sql_incr("1 AS `last_name`,");
  EXPECT_EQ("-", context);  // Multiline comment flag continues
  EXPECT_EQ(0, static_cast<int>(results.size()));
  expected += "1 AS `last_name`,";
  // EXPECT_EQ(expected, std::get<0>(results[0]));

  send_sql_incr("1 AS `last_update`*/;");
  EXPECT_EQ("", context);  // Multiline comment flag is done
  EXPECT_EQ(1, static_cast<int>(results.size()));
  expected += "1 AS `last_update`*/";
  EXPECT_EQ(expected, std::get<0>(results[0]));
}

TEST_F(TestMySQLSplitter, single_line_optimizer_hint) {
  sql =
      "/*+ NO_RANGE_OPTIMIZATION(t3 PRIMARY, f2_idx) */;\n"
      "show databases;";
  send_sql(sql);
  EXPECT_EQ("", context);
  EXPECT_EQ(2, results.size());
  EXPECT_EQ("/*+ NO_RANGE_OPTIMIZATION(t3 PRIMARY, f2_idx) */",
            std::get<0>(results[0]));
  EXPECT_EQ("show databases", std::get<0>(results[1]));
}

TEST_F(TestMySQLSplitter, multi_line_optimizer_hint_in_batch) {
  std::string optimizer =
      "/*+ BKA(t1) "
      "NO_BKA(t2)*/";
  sql = optimizer + ";\nshow databases;";

  send_sql(sql);
  EXPECT_EQ("", context);
  EXPECT_EQ(2, static_cast<int>(results.size()));
  EXPECT_EQ(optimizer, std::get<0>(results[0]));
  EXPECT_EQ("show databases", std::get<0>(results[1]));
}

TEST_F(TestMySQLSplitter, multi_line_optimizer_hint_line_by_line) {
  send_sql_incr("/*+ BKA(t1) ");
  EXPECT_EQ("-", context);  // Multiline comment flag is set
  EXPECT_EQ(0, static_cast<int>(results.size()));
  std::string expected = "/*+ BKA(t1) ";
  // EXPECT_EQ(expected, std::get<0>(results[0]));

  send_sql_incr("NO_BKA(t2)*/;");
  EXPECT_EQ("", context);  // Multiline comment flag is done
  EXPECT_EQ(1, static_cast<int>(results.size()));
  expected += "NO_BKA(t2)*/";
  EXPECT_EQ(expected, std::get<0>(results[0]));
}

TEST_F(TestMySQLSplitter, continued_stmt_multiline_comment) {
  send_sql_incr("SELECT 1 AS _one /*");
  EXPECT_EQ("-", context);  // Continued statement flag
  EXPECT_EQ(0, static_cast<int>(results.size()));

  // The first range is the incomplete statement
  std::string expected = "SELECT 1 AS _one /*";
  // EXPECT_EQ(expected, std::get<0>(results[0]));

  send_sql_incr("comment text */;");
  EXPECT_EQ("", context);
  EXPECT_EQ(1, static_cast<int>(results.size()));

  // The first range is the incomplete statement
  expected += "comment text */";
  EXPECT_EQ(expected, std::get<0>(results[0]));
}

TEST_F(TestMySQLSplitter, continued_stmt_dash_dash_comment) {
  send_sql_incr("select 1 as one -- sample text\n");
  EXPECT_EQ("-", context);  // Continued statement flag
  EXPECT_EQ(0, static_cast<int>(results.size()));

  // The first range is the incomplete statement
  std::string expected = "select 1 as one ";
  // EXPECT_EQ(expected, std::get<0>(results[0]));

  send_sql_incr(";select 2 as two;");
  EXPECT_EQ("", context);
  EXPECT_EQ(2, static_cast<int>(results.size()));

  EXPECT_EQ("select 1 as one -- sample text\n", std::get<0>(results[0]));

  // The second range is the second statement
  expected = "select 2 as two";
  EXPECT_EQ(expected, std::get<0>(results[1]));
}

TEST_F(TestMySQLSplitter, continued_stmt_dash_dash_comment_batch) {
  send_sql_incr("select 1 as one -- sample text\n;select 2 as two;");
  EXPECT_EQ("", context);
  EXPECT_EQ(2, static_cast<int>(results.size()));

  // The first range is the incomplete statement
  std::string expected = "select 1 as one -- sample text\n";
  EXPECT_EQ(expected, std::get<0>(results[0]));

  // The 2nd range is the second statement
  expected = "select 2 as two";
  EXPECT_EQ(expected, std::get<0>(results[1]));
}

TEST_F(TestMySQLSplitter, continued_stmt_hash_comment) {
  send_sql_incr("select 1 as one #sample text\n");
  EXPECT_EQ("-", context);  // Continued statement flag
  EXPECT_EQ(0, static_cast<int>(results.size()));

  // The first range is the incomplete statement
  std::string expected = "select 1 as one ";
  // EXPECT_EQ(expected, std::get<0>(results[0]));

  send_sql_incr(";select 2 as two;");
  EXPECT_EQ("", context);
  EXPECT_EQ(2, static_cast<int>(results.size()));

  EXPECT_EQ("select 1 as one #sample text\n", std::get<0>(results[0]));

  // The second range is the second statement
  expected = "select 2 as two";
  EXPECT_EQ(expected, std::get<0>(results[1]));
}

TEST_F(TestMySQLSplitter, continued_stmt_hash_comment_batch) {
  send_sql_incr("select 1 as one #sample text\n;select 2 as two;");
  EXPECT_EQ("", context);
  EXPECT_EQ(2, static_cast<int>(results.size()));

  // The first range is the incomplete statement
  std::string expected = "select 1 as one #sample text\n";
  EXPECT_EQ(expected, std::get<0>(results[0]));

  // The 2nd range is the second statement
  expected = "select 2 as two";
  EXPECT_EQ(expected, std::get<0>(results[1]));
}

TEST_F(TestMySQLSplitter, multiline_comment_bug) {
  send_sql("/* * */ select 1;");  // This loops forever before bugfix
  EXPECT_EQ(1, static_cast<int>(results.size()));
  EXPECT_EQ("/* * */ select 1", std::get<0>(results[0]));

  send_sql("select 0; /* / */ select 1;");
  EXPECT_EQ(2, static_cast<int>(results.size()));
  EXPECT_EQ("select 0", std::get<0>(results[0]));
  EXPECT_EQ("/* / */ select 1", std::get<0>(results[1]));

  send_sql("/*/ */ select 1;");
  EXPECT_EQ(1, static_cast<int>(results.size()));
  EXPECT_EQ("/*/ */ select 1", std::get<0>(results[0]));

  send_sql("/* /*/ select 1;");
  EXPECT_EQ(1, static_cast<int>(results.size()));
  EXPECT_EQ("/* /*/ select 1", std::get<0>(results[0]));

  send_sql("/**/ select 1;");
  EXPECT_EQ(1, static_cast<int>(results.size()));
  EXPECT_EQ("/**/ select 1", std::get<0>(results[0]));
}

#if 0
// this is not a bug, old client will only accept delimiter if its at
// the beginning of statement
TEST_F(TestMySQLSplitter, multiline_comment_bug_delim) {
  send_sql("select 1; /* * */ delimiter //\nselect 2//\nfoo;bar//\n");
  EXPECT_EQ(3, static_cast<int>(results.size()));
  EXPECT_EQ("select 1", std::get<0>(results[0]));
  EXPECT_EQ("select 2", std::get<0>(results[1]));
  EXPECT_EQ("foo;bar", std::get<0>(results[2]));
}
#endif

// New tests
class Statement_splitter : public ::testing::TestWithParam<int> {
 protected:
  std::vector<std::string> split_batch(const std::string &sql,
                                       bool ansi_quotes = false) {
    std::stringstream ss(sql);
    std::vector<std::tuple<std::string, std::string, size_t>> r;
    if (GetParam() == 0)
      r = split_sql_stream(
          &ss, sql.empty() ? 1 : sql.length(),
          [](const std::string &err) { throw std::runtime_error(err); },
          ansi_quotes, &delimiter);
    else
      r = split_sql_stream(
          &ss, GetParam(),
          [](const std::string &err) { throw std::runtime_error(err); },
          ansi_quotes, &delimiter);
    std::vector<std::string> stmts;
    for (const auto &i : r) {
      stmts.push_back(std::get<0>(i) + std::get<1>(i));
    }
    return stmts;
  }

  void check_lnum(const std::vector<std::string> &stmts) {
    std::string sql;
    for (const auto &s : stmts) {
      sql.append(s);
    }

    std::stringstream ss(sql);
    std::vector<std::tuple<std::string, std::string, size_t>> r;
    if (GetParam() == 0)
      r = split_sql_stream(
          &ss, sql.empty() ? 1 : sql.length(),
          [](const std::string &err) { throw std::runtime_error(err); }, false,
          &delimiter);
    else
      r = split_sql_stream(
          &ss, GetParam(),
          [](const std::string &err) { throw std::runtime_error(err); }, false,
          &delimiter);

    size_t lnum = 1;
    auto s = stmts.begin();
    for (const auto &i : r) {
      ASSERT_TRUE(s != stmts.end());
      if (s->compare(0, strlen("delimiter"), "delimiter") == 0) {
        // delimiter lines are skipped
        lnum += std::count(s->begin(), s->end(), '\n');
        ++s;
      }

      SCOPED_TRACE(std::to_string(lnum) + ") " + *s);

      EXPECT_EQ(lnum, std::get<2>(i));
      EXPECT_EQ(shcore::str_rstrip(*s, "\n"), std::get<0>(i) + std::get<1>(i));

      lnum += std::count(s->begin(), s->end(), '\n');
      ++s;
    }
  }

  std::string delimiter = ";";
};

using strv = std::vector<std::string>;

TEST_P(Statement_splitter, basic) {
  EXPECT_EQ(strv({}), split_batch(""));

  EXPECT_EQ(strv({";"}), split_batch(";"));

  EXPECT_EQ(strv({"'foo\\'b;a''r';"}), split_batch("'foo\\'b;a''r';"));

  EXPECT_EQ(strv({"select 1;", "select 2;"}),
            split_batch("select 1; select 2;"));

  EXPECT_EQ(strv({"select 1;", "select 2"}), split_batch("select 1; select 2"));

  EXPECT_EQ(strv({"select 1;", "select 2;", "select\n3;"}),
            split_batch(R"*(select 1;
select 2;select
3;)*"));
}

TEST_P(Statement_splitter, comments) {
  EXPECT_EQ(strv({"/* one\n*/"}), split_batch(R"*(/* one
*/)*"));

  EXPECT_EQ(strv({"/* one **/"}), split_batch(R"*(/* one **/)*"));

  EXPECT_EQ(strv({"/** one */"}), split_batch(R"*(/** one */)*"));

  EXPECT_EQ(strv({"/*/ one */"}), split_batch(R"*(/*/ one */)*"));

  EXPECT_EQ(strv({"/* one /*/"}), split_batch(R"*(/* one /*/)*"));

  EXPECT_EQ(strv({"/* one *"}), split_batch(R"*(/* one *)*"));

  EXPECT_EQ(strv({"/* one;\n--\ntwo */"}), split_batch(R"*(/* one;
--
two */)*"));

  EXPECT_EQ(strv({"/* one\n--\ntwo */"}), split_batch(R"*(/* one
--
two */)*"));

  EXPECT_EQ(strv({"/*! x */ select 1;", "select 2;", "select /* x */ 3;"}),
            split_batch(R"*(/*! x */ select 1; select 2; 
select /* x */ 3;)*"));

  EXPECT_EQ(strv({R"*(/* x
*/ select /* y 
z */)*"}),
            split_batch(R"*(/* x
*/ select /* y 
z */)*"));

  EXPECT_EQ(strv({"select 1# bla;\n;", "-- ;;;#", "select 2;"}),
            split_batch(R"*(select 1# bla;
;
-- ;;;#
select 2;)*"));
}

TEST_P(Statement_splitter, strings) {
  EXPECT_EQ(strv({"select ';';", "select 'delimiter ;';", "select 'foo\nbar';",
                  "'foo\\'b;a''r';"}),
            split_batch("select ';'; select 'delimiter ;'; select "
                        "'foo\nbar';'foo\\'b;a''r';"));

  EXPECT_EQ(
      strv({"select `foo`;", "select `Foo\nBar`;", "select `FOO;\\`,\n`BAR`;"}),
      split_batch(R"*(select `foo`;
select `Foo
Bar`;
select `FOO;\`,
`BAR`;)*"));

  EXPECT_EQ(strv({"select 'foo';", "select 'Foo\nBar';",
                  "select 'FOO\\';',\n'BAR';"}),
            split_batch(R"*(select 'foo';
select 'Foo
Bar'; select 'FOO\';',
'BAR';)*"));

  EXPECT_EQ(strv({"/* foo; bar\nbla;\nble */\nselect 1;",
                  "/* ; bla -- */ select 1, /* # */2, 3;"}),
            split_batch(R"*(/* foo; bar
bla;
ble */
select 1;
/* ; bla -- */ select 1, /* # */2, 3;
)*"));
}

TEST_P(Statement_splitter, ansi_quotes) {
  // if ansi_quotes then "" is handled the same way as ``
  EXPECT_EQ(strv({R"*("a"";b";)*", "'a'';b';", "`a``;b`;"}),
            split_batch(R"*("a"";b"; 'a'';b'; `a``;b`;)*", false));

  const auto s1 = R"*("a\";b"; 'a\';b'; `a\`;b`;)*";
  const auto expected_s1 =
      strv({R"*("a\";b";)*", R"*('a\';b';)*", R"*(`a\`;)*", R"*(b`;)*"});
  EXPECT_EQ(expected_s1, split_batch(s1, false));

  EXPECT_EQ(strv({R"*("a"";b";)*", "'a'';b';", "`a``;b`;"}),
            split_batch(R"*("a"";b"; 'a'';b'; `a``;b`;)*", true));

  const auto s2 = R"*("a\";b; 'a\';b'; `a\`;b;)*";
  const auto expected_s2 =
      strv({R"*("a\";)*", R"*(b;)*", R"*('a\';b';)*", R"*(`a\`;)*", R"*(b;)*"});
  EXPECT_EQ(expected_s2, split_batch(s2, true));
}

TEST_P(Statement_splitter, commands) {
  EXPECT_EQ(strv({"\\g", ";"}), split_batch(R"*(\g;)*"));

  EXPECT_EQ(strv({"select 1\\g", "select 2;", "\\w", "select 3,4\\G"}),
            split_batch(R"*(select 1\gselect 2;select 3\w,4\G)*"));

  EXPECT_EQ(strv({"\\w", "/* comment */\n\nselect 1;"}),
            split_batch(R"*(/* comment */
\w
select 1;)*"));

  EXPECT_EQ(strv({"\\W", "\\w", "select 1\\g"}),
            split_batch("select 1\\W\\w\\g"));
}

TEST_P(Statement_splitter, delimiter) {
  delimiter = ";";
  EXPECT_THROW(split_batch("delimiter;\n"), std::runtime_error);
  EXPECT_EQ(";", delimiter);

  EXPECT_THROW(split_batch("delimiter;"), std::runtime_error);
  EXPECT_EQ(";", delimiter);

  EXPECT_THROW(split_batch("  delimiter;"), std::runtime_error);
  EXPECT_EQ(";", delimiter);

  delimiter = ";";
  EXPECT_EQ(strv({}), split_batch("delimiter ;;\n"));
  EXPECT_EQ(";;", delimiter);

  delimiter = ";";
  EXPECT_EQ(strv({"x;"}), split_batch("x;delimiter ;;\n"));
  EXPECT_EQ(";;", delimiter);

  delimiter = ";";
  EXPECT_EQ(strv({}), split_batch("  delimiter ;;\n"));
  EXPECT_EQ(";;", delimiter);

  delimiter = ";";
  EXPECT_EQ(strv({"a;b$"}), split_batch("delimiter\t$\na;b$"));
  EXPECT_EQ("$", delimiter);

  delimiter = ";";
  EXPECT_EQ(strv({"a;b$"}), split_batch("delimiter\t $ \t;\na;b$"));
  EXPECT_EQ("$", delimiter);

  delimiter = ";";
  EXPECT_EQ(strv({"a;b$"}), split_batch("delimiter $ \t;\na;b$"));
  EXPECT_EQ("$", delimiter);

  delimiter = ";";
  EXPECT_EQ(strv({}), split_batch("delimiter ;select 1;"));
  EXPECT_EQ(";select", delimiter);

  delimiter = ";";
  EXPECT_EQ(strv({}), split_batch("delimiter ;select 1;\n"));
  EXPECT_EQ(";select", delimiter);

  delimiter = ";";
  EXPECT_EQ(strv({"select 1;\nselect 2$", "select $;"}),
            split_batch(R"*(delimiter $
select 1;
select 2$
delimiter ;
select $;)*"));
  EXPECT_EQ(";", delimiter);

  delimiter = "$";
  EXPECT_EQ(strv({"select 1;2$", "select 3"}),
            split_batch("select 1;2$select 3"));

  delimiter = ";;";
  EXPECT_EQ(strv({"select 1;2;;", "select 3"}),
            split_batch("select 1;2;;select 3"));

  delimiter = ";";
  EXPECT_EQ(strv({"create table delimiter (a int);", "select delimiter;"}),
            split_batch("create table delimiter (a int); select delimiter;"));
  EXPECT_EQ(";", delimiter);

  EXPECT_EQ(strv({"drop table\ndelimiter;"}),
            split_batch("drop table\ndelimiter;"));
  EXPECT_EQ(";", delimiter);

  EXPECT_EQ(strv({"drop table\ndelimiter ,\nfoobar;"}),
            split_batch("drop table\ndelimiter ,\nfoobar;"));
  EXPECT_EQ(";", delimiter);

  EXPECT_EQ(strv({"select 1//"}), split_batch("/* bla */\n"
                                              "delimiter //\n"
                                              "select 1//"));
  EXPECT_EQ("//", delimiter);

  EXPECT_EQ(strv({"-- bla", "select 1$$$"}), split_batch("-- bla\n"
                                                         "delimiter $$$\n"
                                                         "select 1$$$\n"));
  EXPECT_EQ("$$$", delimiter);

  EXPECT_EQ(strv({"select 1$$"}), split_batch("/* bla\n"
                                              "bla */\n"
                                              "delimiter $$\n"
                                              "select 1$$\n"));
  EXPECT_EQ("$$", delimiter);

  EXPECT_EQ(strv({"# comment", "select 1#"}), split_batch("# comment\n"
                                                          "delimiter #\n"
                                                          "select 1#\n"));
  EXPECT_EQ("#", delimiter);

  EXPECT_EQ(strv({"select 1!%#", "show databases$$"}),
            split_batch("\n"
                        "DELIMITER !%#\n"
                        "select 1!%#"
                        "DELIMITER $$\n"
                        "\n"
                        "show databases$$\n"));
  EXPECT_EQ("$$", delimiter);

  EXPECT_EQ(strv({"show databases$$"}), split_batch("\n"
                                                    "DELIMITER ;\n"
                                                    "\n"
                                                    "DELIMITER $$\n"
                                                    "\n"
                                                    "show databases$$\n"
                                                    "delimiter ;"));
  EXPECT_EQ(";", delimiter);

  // TODO(alfredo): \d as delimiter change command
  // select 1\d@ @
  // select 1;\d@ select 2@
  // select 1, \w\d @ 2, 3@
}

TEST_P(Statement_splitter, line_numbering) {
  // clang-format off
  {
    SCOPED_TRACE("");
    check_lnum({"1;\n",
                "2;\n",
                "3;"});
  }
  {
    SCOPED_TRACE("");
    check_lnum({"-- ;\n",  // single-line comments are always a separate stmt
                ";\n",
                "2;"});
  }
  {
    SCOPED_TRACE("");
    check_lnum({"# ;\n",  // single-line comments are always a separate stmt
                ";\n",
                "2;"});
  }
  {
    SCOPED_TRACE("");
    check_lnum({"/* ; */\n"
                ";\n",
                "2;"});
  }
  {
    SCOPED_TRACE("");
    check_lnum({"1;", "2;\n",
                "3;"});
  }
  {
    SCOPED_TRACE("");
    check_lnum({"'foo\n"
                "bar';", "x;"});
  }
  {
    SCOPED_TRACE("");
    check_lnum({"\"foo\n"
                "bar\";", "x;"});
  }
  {
    SCOPED_TRACE("");
    check_lnum({"`foo\n"
                "bar`;", "x;"});
  }
  {
    SCOPED_TRACE("");
    check_lnum({"/*foo\n"
                "bar*/;", "x;"});
  }
  {
    SCOPED_TRACE("");
    check_lnum({"delimiter $$\n",
                ";$$\n",
                "y$$\n",
                "x$$"});
    delimiter = ";";
  }
  {
    SCOPED_TRACE("");
    check_lnum({"delimiter $$\n",
                ";\n"
                ";$$\n",
                "y$$\n",
                "x$$"});
    delimiter = ";";
  }
  {
    SCOPED_TRACE("");
    check_lnum({"line 1;\n",
                "line 2,\n"
                "3\n"
                "and 4;\n",
                "-- line comment\n",
                "/* another comment */ line 6;\n",
                "/* line 7\n"
                "   line 8 */;\n",
                "# another comment\n",
                "'10 long string\n"
                "until line 11';",
                "line 11.1;",
                "line 11.2;"});
  }
  // clang-format on
}

// Runs tests processing script in 1 chunk
INSTANTIATE_TEST_CASE_P(Statement_splitter_full, Statement_splitter,
                        ::testing::Values(0));

// Runs tests processing script in small chunks with size incrementing by 1
INSTANTIATE_TEST_CASE_P(Statement_splitter_1, Statement_splitter,
                        ::testing::Range(1, 16, 1));

// Runs tests processing script in multiple chunks with various sizes
INSTANTIATE_TEST_CASE_P(Statement_splitter_chunks, Statement_splitter,
                        ::testing::Values(4, 8, 16, 32, 64));

}  // namespace utils
}  // namespace mysqlshdk
