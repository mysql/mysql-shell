/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include <array>
#include <random>
#include <thread>
#include <vector>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

#include "mysqlshdk/libs/utils/version.h"

#include "mysqlshdk/libs/yparser/parser.h"

namespace mysqlshdk {
namespace yacc {
namespace {

using Version = utils::Version;

const Version k_mysql_5_7{5, 7, 44};
const Version k_mysql_8_0{8, 0, 36};
const Version k_mysql_current{LIBMYSQL_VERSION};

TEST(Parser_test, version) {
  {
    Parser parser{Version{5, 6, 51}};
    EXPECT_EQ(k_mysql_5_7, parser.version());
  }

  {
    Parser parser{Version{5, 7, 0}};
    EXPECT_EQ(k_mysql_5_7, parser.version());
  }

  {
    Parser parser{k_mysql_5_7};
    EXPECT_EQ(k_mysql_5_7, parser.version());
  }

  {
    Parser parser{Version{5, 7, 100}};
    EXPECT_EQ(k_mysql_5_7, parser.version());
  }

  {
    Parser parser{Version{8, 0, 0}};
    EXPECT_EQ(k_mysql_8_0, parser.version());
  }

  {
    Parser parser{k_mysql_8_0};
    EXPECT_EQ(k_mysql_8_0, parser.version());
  }

  {
    Parser parser{Version{8, 0, 100}};
    EXPECT_EQ(k_mysql_8_0, parser.version());
  }

  // these tests need to be adjusted once 8.4 parser is added
  {
    Parser parser{Version{8, 1, 0}};
    EXPECT_EQ(k_mysql_current, parser.version());
  }

  {
    Parser parser{Version{8, 2, 0}};
    EXPECT_EQ(k_mysql_current, parser.version());
  }

  {
    Parser parser{Version{8, 3, 0}};
    EXPECT_EQ(k_mysql_current, parser.version());
  }

  {
    Parser parser{Version{8, 4, 0}};
    EXPECT_EQ(k_mysql_current, parser.version());
  }

  {
    Parser parser{Version{8, 4, 100}};
    EXPECT_EQ(k_mysql_current, parser.version());
  }

  // in case of an unsupported version, latest available parser is used
  {
    Parser parser{Version{1000, 0, 0}};
    EXPECT_EQ(k_mysql_current, parser.version());
  }
}

TEST(Parser_test, sql_mode) {
  {
    Parser parser{k_mysql_5_7};

    EXPECT_THROW_LIKE(parser.set_sql_mode("INVALID"), Parser_error,
                      "Failed to set sql_mode: Unknown SQL mode: INVALID");

    EXPECT_THROW_LIKE(parser.set_sql_mode("TIME_TRUNCATE_FRACTIONAL"),
                      Parser_error,
                      "Failed to set sql_mode: Unsupported SQL mode: "
                      "TIME_TRUNCATE_FRACTIONAL");

    EXPECT_NO_THROW(parser.set_sql_mode("REAL_AS_FLOAT"));
    EXPECT_NO_THROW(parser.set_sql_mode("PIPES_AS_CONCAT"));
    EXPECT_NO_THROW(parser.set_sql_mode("ANSI_QUOTES"));
    EXPECT_NO_THROW(parser.set_sql_mode("IGNORE_SPACE"));
    EXPECT_NO_THROW(parser.set_sql_mode("NOT_USED"));
    EXPECT_NO_THROW(parser.set_sql_mode("ONLY_FULL_GROUP_BY"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_UNSIGNED_SUBTRACTION"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_DIR_IN_CREATE"));
    EXPECT_NO_THROW(parser.set_sql_mode("POSTGRESQL"));
    EXPECT_NO_THROW(parser.set_sql_mode("ORACLE"));
    EXPECT_NO_THROW(parser.set_sql_mode("MSSQL"));
    EXPECT_NO_THROW(parser.set_sql_mode("DB2"));
    EXPECT_NO_THROW(parser.set_sql_mode("MAXDB"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_KEY_OPTIONS"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_TABLE_OPTIONS"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_FIELD_OPTIONS"));
    EXPECT_NO_THROW(parser.set_sql_mode("MYSQL323"));
    EXPECT_NO_THROW(parser.set_sql_mode("MYSQL40"));
    EXPECT_NO_THROW(parser.set_sql_mode("ANSI"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_AUTO_VALUE_ON_ZERO"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_BACKSLASH_ESCAPES"));
    EXPECT_NO_THROW(parser.set_sql_mode("STRICT_TRANS_TABLES"));
    EXPECT_NO_THROW(parser.set_sql_mode("STRICT_ALL_TABLES"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_ZERO_IN_DATE"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_ZERO_DATE"));
    EXPECT_NO_THROW(parser.set_sql_mode("ALLOW_INVALID_DATES"));
    EXPECT_NO_THROW(parser.set_sql_mode("ERROR_FOR_DIVISION_BY_ZERO"));
    EXPECT_NO_THROW(parser.set_sql_mode("TRADITIONAL"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_AUTO_CREATE_USER"));
    EXPECT_NO_THROW(parser.set_sql_mode("HIGH_NOT_PRECEDENCE"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_ENGINE_SUBSTITUTION"));
    EXPECT_NO_THROW(parser.set_sql_mode("PAD_CHAR_TO_FULL_LENGTH"));
    EXPECT_THROW(parser.set_sql_mode("TIME_TRUNCATE_FRACTIONAL"), Parser_error);

    EXPECT_NO_THROW(parser.set_sql_mode("ANSI,NO_ENGINE_SUBSTITUTION"));
    EXPECT_NO_THROW(parser.set_sql_mode("ANSI,,NO_ENGINE_SUBSTITUTION"));
    EXPECT_NO_THROW(parser.set_sql_mode("ANSI,NO_ENGINE_SUBSTITUTION,"));
    EXPECT_NO_THROW(parser.set_sql_mode(",ANSI,NO_ENGINE_SUBSTITUTION"));
  }

  for (const auto &v : {k_mysql_8_0, k_mysql_current}) {
    SCOPED_TRACE("Version: " + v.get_base());
    Parser parser{v};

    EXPECT_THROW_LIKE(parser.set_sql_mode("INVALID"), Parser_error,
                      "Failed to set sql_mode: Unknown SQL mode: INVALID");

    EXPECT_THROW_LIKE(parser.set_sql_mode("ORACLE"), Parser_error,
                      "Failed to set sql_mode: Unsupported SQL mode: ORACLE");

    EXPECT_NO_THROW(parser.set_sql_mode("REAL_AS_FLOAT"));
    EXPECT_NO_THROW(parser.set_sql_mode("PIPES_AS_CONCAT"));
    EXPECT_NO_THROW(parser.set_sql_mode("ANSI_QUOTES"));
    EXPECT_NO_THROW(parser.set_sql_mode("IGNORE_SPACE"));
    EXPECT_NO_THROW(parser.set_sql_mode("NOT_USED"));
    EXPECT_NO_THROW(parser.set_sql_mode("ONLY_FULL_GROUP_BY"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_UNSIGNED_SUBTRACTION"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_DIR_IN_CREATE"));
    EXPECT_THROW(parser.set_sql_mode("POSTGRESQL"), Parser_error);
    EXPECT_THROW(parser.set_sql_mode("ORACLE"), Parser_error);
    EXPECT_THROW(parser.set_sql_mode("MSSQL"), Parser_error);
    EXPECT_THROW(parser.set_sql_mode("DB2"), Parser_error);
    EXPECT_THROW(parser.set_sql_mode("MAXDB"), Parser_error);
    EXPECT_THROW(parser.set_sql_mode("NO_KEY_OPTIONS"), Parser_error);
    EXPECT_THROW(parser.set_sql_mode("NO_TABLE_OPTIONS"), Parser_error);
    EXPECT_THROW(parser.set_sql_mode("NO_FIELD_OPTIONS"), Parser_error);
    EXPECT_THROW(parser.set_sql_mode("MYSQL323"), Parser_error);
    EXPECT_THROW(parser.set_sql_mode("MYSQL40"), Parser_error);
    EXPECT_NO_THROW(parser.set_sql_mode("ANSI"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_AUTO_VALUE_ON_ZERO"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_BACKSLASH_ESCAPES"));
    EXPECT_NO_THROW(parser.set_sql_mode("STRICT_TRANS_TABLES"));
    EXPECT_NO_THROW(parser.set_sql_mode("STRICT_ALL_TABLES"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_ZERO_IN_DATE"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_ZERO_DATE"));
    EXPECT_NO_THROW(parser.set_sql_mode("ALLOW_INVALID_DATES"));
    EXPECT_NO_THROW(parser.set_sql_mode("ERROR_FOR_DIVISION_BY_ZERO"));
    EXPECT_NO_THROW(parser.set_sql_mode("TRADITIONAL"));
    EXPECT_THROW(parser.set_sql_mode("NO_AUTO_CREATE_USER"), Parser_error);
    EXPECT_NO_THROW(parser.set_sql_mode("HIGH_NOT_PRECEDENCE"));
    EXPECT_NO_THROW(parser.set_sql_mode("NO_ENGINE_SUBSTITUTION"));
    EXPECT_NO_THROW(parser.set_sql_mode("PAD_CHAR_TO_FULL_LENGTH"));
    EXPECT_NO_THROW(parser.set_sql_mode("TIME_TRUNCATE_FRACTIONAL"));
  }

  {
    constexpr std::string_view statement = "SELECT * FROM \"mysql\".\"user\"";

    Parser parser{k_mysql_current};
    EXPECT_FALSE(parser.check_syntax(statement).empty());

    parser.set_sql_mode("ANSI_QUOTES");
    EXPECT_TRUE(parser.check_syntax(statement).empty());
  }

  {
    constexpr std::string_view statement = "SELECT 'abc\\'def'";

    Parser parser{k_mysql_current};
    EXPECT_TRUE(parser.check_syntax(statement).empty());

    parser.set_sql_mode("NO_BACKSLASH_ESCAPES");
    EXPECT_FALSE(parser.check_syntax(statement).empty());
  }
}

TEST(Parser_test, check_syntax) {
  {
    // parsers for different versions all present at once
    Parser parser_5_7{k_mysql_5_7};
    Parser parser_8_0{k_mysql_8_0};
    Parser parser_X_X{k_mysql_current};

    {
      constexpr std::string_view grant_with_resource_option =
          "GRANT ALL ON *.* TO user@host WITH MAX_QUERIES_PER_HOUR 1";
      EXPECT_TRUE(parser_5_7.check_syntax(grant_with_resource_option).empty());
      EXPECT_FALSE(parser_8_0.check_syntax(grant_with_resource_option).empty());
      EXPECT_FALSE(parser_X_X.check_syntax(grant_with_resource_option).empty());
    }

    {
      constexpr std::string_view create_role = "CREATE ROLE dev";
      EXPECT_FALSE(parser_5_7.check_syntax(create_role).empty());
      EXPECT_TRUE(parser_8_0.check_syntax(create_role).empty());
      EXPECT_TRUE(parser_X_X.check_syntax(create_role).empty());
    }

    {
      constexpr std::string_view show_parse_tree = "SHOW PARSE_TREE SELECT 1";
      EXPECT_FALSE(parser_5_7.check_syntax(show_parse_tree).empty());
      EXPECT_FALSE(parser_8_0.check_syntax(show_parse_tree).empty());
      EXPECT_TRUE(parser_X_X.check_syntax(show_parse_tree).empty());
    }
  }

  {
    Parser parser{k_mysql_current};

    // valid statement
    EXPECT_TRUE(parser.check_syntax("SELECT * FROM mysql.user").empty());

    // typo
    {
      const auto errors = parser.check_syntax("SELECT * FORM mysql.user");
      ASSERT_FALSE(errors.empty());

      const auto &error = errors.front();
      EXPECT_STREQ("syntax error: near 'FORM'", error.what());
      EXPECT_EQ(0, error.line());
      EXPECT_EQ(9, error.line_offset());
      EXPECT_EQ(9, error.token_offset());
      EXPECT_EQ(4, error.token_length());
      EXPECT_EQ("1:10: syntax error: near 'FORM'", error.format());
    }

    // valid multiline statement
    EXPECT_TRUE(parser.check_syntax("SELECT\n*\nFROM\nmysql.user").empty());

    // typo, multiline
    {
      const auto errors = parser.check_syntax("SELECT\n*\nFORM\nmysql.user");
      ASSERT_FALSE(errors.empty());

      const auto &error = errors.front();
      EXPECT_STREQ("syntax error: near 'FORM'", error.what());
      EXPECT_EQ(2, error.line());
      EXPECT_EQ(0, error.line_offset());
      EXPECT_EQ(9, error.token_offset());
      EXPECT_EQ(4, error.token_length());
      EXPECT_EQ("3:1: syntax error: near 'FORM'", error.format());
    }

    // multiline, multiple statements
    {
      const auto errors = parser.check_syntax(
          "SELECT * FROM\nmysql.user; SELECT * FROM\nmysql.user;");
      ASSERT_FALSE(errors.empty());

      const auto &error = errors.front();
      EXPECT_STREQ("syntax error: near 'SELECT'", error.what());
      EXPECT_EQ(1, error.line());
      EXPECT_EQ(12, error.line_offset());
      EXPECT_EQ(26, error.token_offset());
      EXPECT_EQ(6, error.token_length());
      EXPECT_EQ("2:13: syntax error: near 'SELECT'", error.format());
    }

    // unfinished comment
    {
      const auto errors = parser.check_syntax("SELECT /* this is a comment ");
      ASSERT_FALSE(errors.empty());

      const auto &error = errors.front();
      EXPECT_STREQ("syntax error: near '/* this is a comment '", error.what());
      EXPECT_EQ(0, error.line());
      EXPECT_EQ(7, error.line_offset());
      EXPECT_EQ(7, error.token_offset());
      EXPECT_EQ(21, error.token_length());
      EXPECT_EQ("1:8: syntax error: near '/* this is a comment '",
                error.format());
    }

    // unfinished string
    {
      const auto errors = parser.check_syntax("SELECT \"this is a string ");
      ASSERT_FALSE(errors.empty());

      const auto &error = errors.front();
      EXPECT_STREQ("syntax error: near '\"this is a string '", error.what());
      EXPECT_EQ(0, error.line());
      EXPECT_EQ(7, error.line_offset());
      EXPECT_EQ(7, error.token_offset());
      EXPECT_EQ(18, error.token_length());
      EXPECT_EQ("1:8: syntax error: near '\"this is a string '",
                error.format());
    }
  }
}

TEST(Parser_test, optimizer_hints) {
  {
    // parsers for different versions all present at once
    Parser parser_5_7{k_mysql_5_7};
    Parser parser_8_0{k_mysql_8_0};
    Parser parser_X_X{k_mysql_current};

    {
      constexpr std::string_view no_icp =
          "EXPLAIN SELECT /*+ NO_ICP(t1) */ * FROM t1";
      EXPECT_TRUE(parser_5_7.check_syntax(no_icp).empty());
      EXPECT_TRUE(parser_8_0.check_syntax(no_icp).empty());
      EXPECT_TRUE(parser_X_X.check_syntax(no_icp).empty());
    }

    {
      constexpr std::string_view set_var =
          "INSERT /*+ SET_VAR(foreign_key_checks=OFF) */ INTO t2 VALUES(2)";
      EXPECT_FALSE(parser_5_7.check_syntax(set_var).empty());
      EXPECT_TRUE(parser_8_0.check_syntax(set_var).empty());
      EXPECT_TRUE(parser_X_X.check_syntax(set_var).empty());
    }

    {
      constexpr std::string_view unknown_hint =
          "EXPLAIN SELECT /*+ UNKNOWN(t1) */ * FROM t1";
      EXPECT_FALSE(parser_5_7.check_syntax(unknown_hint).empty());
      EXPECT_FALSE(parser_8_0.check_syntax(unknown_hint).empty());
      EXPECT_FALSE(parser_X_X.check_syntax(unknown_hint).empty());
    }

    {
      constexpr std::string_view no_icp_dq =
          "EXPLAIN SELECT /*+ NO_ICP(\"t1\") */ * FROM t1";
      EXPECT_FALSE(parser_5_7.check_syntax(no_icp_dq).empty());
      EXPECT_FALSE(parser_8_0.check_syntax(no_icp_dq).empty());
      EXPECT_FALSE(parser_X_X.check_syntax(no_icp_dq).empty());

      parser_5_7.set_sql_mode("ANSI");
      parser_8_0.set_sql_mode("ANSI");
      parser_X_X.set_sql_mode("ANSI");

      EXPECT_TRUE(parser_5_7.check_syntax(no_icp_dq).empty());
      EXPECT_TRUE(parser_8_0.check_syntax(no_icp_dq).empty());
      EXPECT_TRUE(parser_X_X.check_syntax(no_icp_dq).empty());
    }
  }

  {
    Parser parser{k_mysql_current};

    // valid statement
    EXPECT_TRUE(parser.check_syntax("SELECT /*+ BKA(t1) */ * FROM t1").empty());

    // typo
    {
      const auto errors =
          parser.check_syntax("SELECT /*+ BAK(t1) */ * FROM t1");
      ASSERT_FALSE(errors.empty());

      const auto &error = errors.front();
      EXPECT_STREQ("syntax error (optimizer hints): near 'BAK'", error.what());
      EXPECT_EQ(0, error.line());
      EXPECT_EQ(11, error.line_offset());
      EXPECT_EQ(11, error.token_offset());
      EXPECT_EQ(3, error.token_length());
      EXPECT_EQ("1:12: syntax error (optimizer hints): near 'BAK'",
                error.format());
    }

    // valid multiline statement
    EXPECT_TRUE(
        parser.check_syntax("SELECT\n/*+\nBKA(t1)\n*/\n*\nFROM\nt1").empty());

    // typo, multiline
    {
      const auto errors =
          parser.check_syntax("SELECT\n/*+\nBAK(t1)\n*/\n*\nFROM\nt1");
      ASSERT_FALSE(errors.empty());

      const auto &error = errors.front();
      EXPECT_STREQ("syntax error (optimizer hints): near 'BAK'", error.what());
      EXPECT_EQ(2, error.line());
      EXPECT_EQ(0, error.line_offset());
      EXPECT_EQ(11, error.token_offset());
      EXPECT_EQ(3, error.token_length());
      EXPECT_EQ("3:1: syntax error (optimizer hints): near 'BAK'",
                error.format());
    }

    // unfinished comment, string, etc.
    // two errors are reported in this case, first by the hint parser, and
    // second by the SQL parser
    {
      const auto errors = parser.check_syntax("SELECT /*+ BKA(t1) ");
      ASSERT_FALSE(errors.empty());

      const auto &first = errors.front();
      EXPECT_STREQ("syntax error (optimizer hints): near '<EOF>'",
                   first.what());
      EXPECT_EQ(0, first.line());
      EXPECT_EQ(19, first.line_offset());
      EXPECT_EQ(19, first.token_offset());
      EXPECT_EQ(1, first.token_length());
      EXPECT_EQ("1:20: syntax error (optimizer hints): near '<EOF>'",
                first.format());

      const auto &second = errors.back();
      EXPECT_STREQ("syntax error: near '/*+ BKA(t1) '", second.what());
      EXPECT_EQ(0, second.line());
      EXPECT_EQ(7, second.line_offset());
      EXPECT_EQ(7, second.token_offset());
      EXPECT_EQ(12, second.token_length());
      EXPECT_EQ("1:8: syntax error: near '/*+ BKA(t1) '", second.format());
    }
  }
}

TEST(Parser_test, threads) {
  std::vector<Version> versions = {
      k_mysql_5_7,
      k_mysql_8_0,
      k_mysql_current,
  };
  constexpr std::array statements = {
      "SELECT * from \"mysql\".user",
      "INSERT INTO mysql.\"user\" VALUES (1)",
      "UNLOCK TABLES",
  };

  ASSERT_EQ(versions.size(), statements.size());

  const auto fun = [&versions, &statements](std::size_t id) {
    std::mt19937 gen{std::random_device()()};
    std::uniform_int_distribution<std::size_t> dist{0, versions.size() - 1};

    for (int i = 0; i < 1000; ++i) {
      try {
        Parser parser{versions[dist(gen)]};
        parser.set_sql_mode("ANSI");
        EXPECT_TRUE(parser.check_syntax(statements[dist(gen)]).empty());
      } catch (const std::exception &e) {
        EXPECT_FALSE(true) << id << ": Exception thrown (iteration " << i
                           << "): " << e.what();
      }
    }
  };

  const auto thread_count = versions.size();
  std::vector<std::thread> threads;
  threads.reserve(thread_count);

  for (std::size_t id = 1; id <= thread_count; ++id) {
    threads.emplace_back(fun, id);
  }

  fun(0);

  for (auto &t : threads) {
    t.join();
  }
}

}  // namespace
}  // namespace yacc
}  // namespace mysqlshdk
