
/* Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */


#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/pointer_cast.hpp>
#include <stack>

#include "gtest/gtest.h"
#include "../utils/utils_mysql_parsing.h"


namespace shcore {


  namespace sql_shell_tests {

    class TestMySQLSplitter : public ::testing::Test
    {
    protected:
      std::stack<std::string> multiline_flags;
      std::string delimiter;
      MySQL_splitter splitter;
      std::vector<std::pair<size_t, size_t> > ranges;
      std::string sql;

      virtual void SetUp()
      {
        delimiter = ";";
      }

      size_t send_sql(const std::string & data)
      {
        // Sends the received sql and returns the statement count considered by the splitter along with
        // status data in delimiter, ranges and multiline_flags
        sql = data;
        return splitter.determineStatementRanges(sql.c_str(), sql.length(), delimiter, ranges, "\n", multiline_flags);
      }
    };

    TEST_F(TestMySQLSplitter, full_statement)
    {
      EXPECT_EQ(1, send_sql("show databases;"));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ("show databases", sql.substr(ranges[0].first, ranges[0].second));
    }

    TEST_F(TestMySQLSplitter, by_line_continued_statement)
    {
      EXPECT_EQ(0, send_sql("show"));
      EXPECT_EQ("-", multiline_flags.top());
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ("show", sql.substr(ranges[0].first, ranges[0].second));

      EXPECT_EQ(0, send_sql("databases"));
      EXPECT_EQ("-", multiline_flags.top());
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ("databases", sql.substr(ranges[0].first, ranges[0].second));


      EXPECT_EQ(1, send_sql(";"));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(0, ranges.size());
    }

    TEST_F(TestMySQLSplitter, script_continued_statement)
    {
      EXPECT_EQ(1, send_sql("show\ndatabases\n;\n"));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ("show\ndatabases\n", sql.substr(ranges[0].first, ranges[0].second));
    }

    TEST_F(TestMySQLSplitter, ignore_empty_statements)
    {
      EXPECT_EQ(1, send_sql("show databases;\n;"));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ("show databases", sql.substr(ranges[0].first, ranges[0].second));

      EXPECT_EQ(0, send_sql(";"));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(0, ranges.size());
    }

    TEST_F(TestMySQLSplitter, single_line_comments)
    {
      sql = "-- shows the database list\n"
            "show databases;";
      EXPECT_EQ(1, send_sql(sql));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ("show databases", sql.substr(ranges[0].first, ranges[0].second));

      sql = "--\tshows the database list\n"
        "show databases;";
      EXPECT_EQ(1, send_sql(sql));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ("show databases", sql.substr(ranges[0].first, ranges[0].second));

      sql = "--\n"
            "show databases;";
      EXPECT_EQ(1, send_sql(sql));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ("show databases", sql.substr(ranges[0].first, ranges[0].second));

      sql = "--";
      EXPECT_EQ(0, send_sql(sql));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(0, ranges.size());

      sql = "--this is an invalid comment, should be considered part of statement\n"
            "show databases;";
      EXPECT_EQ(1, send_sql(sql));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ(sql.substr(0, sql.length()-1), sql.substr(ranges[0].first, ranges[0].second));

      sql = "#this is an valid comment\n"
            "show databases;";
      EXPECT_EQ(1, send_sql(sql));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ("show databases", sql.substr(ranges[0].first, ranges[0].second));

      sql = "show databases; #this is an valid comment\n";
      EXPECT_EQ(1, send_sql(sql));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ("show databases", sql.substr(ranges[0].first, ranges[0].second));

      sql = "--this is an invalid comment, state should indicate a continued statement";
      EXPECT_EQ(0, send_sql(sql));
      EXPECT_EQ("-", multiline_flags.top());
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ(sql, sql.substr(ranges[0].first, ranges[0].second));
    }

    TEST_F(TestMySQLSplitter, multi_line_comments_in_batch)
    {
      sql = "/*\n"
        "this is a comment\n"
        "and should be ignored\n"
        "and ends here */\n"
        "show databases;";
      EXPECT_EQ(1, send_sql(sql));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ("show databases", sql.substr(ranges[0].first, ranges[0].second));

      sql = "/*\n"
        "this is a comment\n"
        "and should be ignored\n"
        "and ends on next line\n"
        "*/\n"
        "show databases;";
      EXPECT_EQ(1, send_sql(sql));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ("show databases", sql.substr(ranges[0].first, ranges[0].second));
    }

    TEST_F(TestMySQLSplitter, multi_line_comments_line_by_line)
    {
      EXPECT_EQ(0, send_sql("/*"));
      EXPECT_EQ("/*", multiline_flags.top());  // Multiline comment flag is set
      EXPECT_EQ(0, ranges.size());

      EXPECT_EQ(0, send_sql("This is a comment"));
      EXPECT_EQ("/*", multiline_flags.top()); // Multiline comment flag continues
      EXPECT_EQ(0, ranges.size());

      EXPECT_EQ(0, send_sql("processed line by line"));
      EXPECT_EQ("/*", multiline_flags.top()); // Multiline comment flag continues
      EXPECT_EQ(0, ranges.size());

      EXPECT_EQ(0, send_sql("and finishes next line"));
      EXPECT_EQ("/*", multiline_flags.top()); // Multiline comment flag continues
      EXPECT_EQ(0, ranges.size());

      EXPECT_EQ(0, send_sql("*/"));
      EXPECT_TRUE(multiline_flags.empty()); // Multiline comment flag is cleared
      EXPECT_EQ(0, ranges.size());

      EXPECT_EQ(1, send_sql("show databases;"));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ("show databases", sql.substr(ranges[0].first, ranges[0].second));
    }

    TEST_F(TestMySQLSplitter, continued_backtick_string)
    {
      EXPECT_EQ(0, send_sql("select * from `t1"));
      EXPECT_EQ("`", multiline_flags.top());  // Multiline backtick flag is set
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ(sql, sql.substr(ranges[0].first, ranges[0].second));

      EXPECT_EQ(1, send_sql("`;"));
      EXPECT_TRUE(multiline_flags.empty());  // Multiline backtick flag is cleared
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ("`", sql.substr(ranges[0].first, ranges[0].second));
    }

    TEST_F(TestMySQLSplitter, continued_single_quote_string)
    {
      EXPECT_EQ(0, send_sql("select * from t1 where name = 'this"));
      EXPECT_EQ("'", multiline_flags.top());  // Multiline backtick flag is set
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ(sql, sql.substr(ranges[0].first, ranges[0].second));

      EXPECT_EQ(1, send_sql("thing';"));
      EXPECT_TRUE(multiline_flags.empty());  // Multiline backtick flag is cleared
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ("thing'", sql.substr(ranges[0].first, ranges[0].second));
    }

    TEST_F(TestMySQLSplitter, continued_double_quote_string)
    {
      EXPECT_EQ(0, send_sql("select * from t1 where name = 'this"));
      EXPECT_EQ("'", multiline_flags.top());  // Multiline backtick flag is set
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ(sql, sql.substr(ranges[0].first, ranges[0].second));

      EXPECT_EQ(1, send_sql("\";"));
      EXPECT_TRUE(multiline_flags.empty());  // Multiline backtick flag is cleared
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ("\"", sql.substr(ranges[0].first, ranges[0].second));
    }

    TEST_F(TestMySQLSplitter, multiple_statements)
    {
      sql = "show databases;\n"
            "select * from whatever;\n"
            "drop database whatever;\n";
      EXPECT_EQ(3, send_sql(sql));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(3, ranges.size());
      EXPECT_EQ("show databases", sql.substr(ranges[0].first, ranges[0].second));
      EXPECT_EQ("select * from whatever", sql.substr(ranges[1].first, ranges[1].second));
      EXPECT_EQ("drop database whatever", sql.substr(ranges[2].first, ranges[2].second));
    }

    TEST_F(TestMySQLSplitter, delimiter_change)
    {
      EXPECT_EQ(0, send_sql("delimiter \\"));
      EXPECT_EQ("\\", delimiter);
      EXPECT_EQ(0, ranges.size());

      EXPECT_EQ(0, send_sql("delimiter ;"));
      EXPECT_EQ(";", delimiter);
      EXPECT_EQ(0, ranges.size());

      sql = "delimiter \\\n"
            "show databases;\n"
            "select * from whatever;\n"
            "drop database whatever;\n"
            "\\\n"
            "delimiter \\\n";

      std::string statement = "show databases;\n"
                              "select * from whatever;\n"
                              "drop database whatever;\n";

      EXPECT_EQ(1, send_sql(sql));
      EXPECT_TRUE(multiline_flags.empty());
      EXPECT_EQ(1, ranges.size());
      EXPECT_EQ(statement, sql.substr(ranges[0].first, ranges[0].second));
    }
  }
}
