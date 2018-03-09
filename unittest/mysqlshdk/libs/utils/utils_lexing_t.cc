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

#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

namespace mysqlshdk {
namespace utils {

TEST(Utils_lexing, span_quoted_string_dq) {
  EXPECT_EQ(2, span_quoted_string_dq("\"\"", 0));
  EXPECT_EQ(3, span_quoted_string_dq("\"a\"", 0));
  EXPECT_EQ(3, span_quoted_string_dq("\"a\"a", 0));
  EXPECT_EQ(3, span_quoted_string_dq("\"a\"aa", 0));
  EXPECT_EQ(4, span_quoted_string_dq("\"aa\"aa", 0));
  EXPECT_EQ(4, span_quoted_string_dq("\"aa\"a", 0));

  EXPECT_EQ(2, span_quoted_string_dq("\"\"\"", 0));
  EXPECT_EQ(3, span_quoted_string_dq("\"a\"\"", 0));

  EXPECT_EQ(1 + 2, span_quoted_string_dq("\"\"\"", 1));
  EXPECT_EQ(1 + 3, span_quoted_string_dq("\"\"a\"", 1));
  EXPECT_EQ(1 + 3, span_quoted_string_dq("\"\"a\"a", 1));
  EXPECT_EQ(1 + 3, span_quoted_string_dq("\"\"a\"aa", 1));
  EXPECT_EQ(1 + 4, span_quoted_string_dq("\"\"aa\"aa", 1));
  EXPECT_EQ(1 + 4, span_quoted_string_dq("\"\"aa\"a", 1));

  EXPECT_EQ(10, span_quoted_string_dq("\"foo 'bar\"x", 0));
  EXPECT_EQ(11, span_quoted_string_dq("\"foo \\'bar\"x", 0));
  EXPECT_EQ(11, span_quoted_string_dq("\"foo \\\"bar\"x", 0));
  EXPECT_EQ(12, span_quoted_string_dq("\"foo \\\"\\bar\"x", 0));
  EXPECT_EQ(12, span_quoted_string_dq("\"foo \\\"\\bar\"x", 0));

  EXPECT_EQ(6, span_quoted_string_dq("\"foo \"\\bar\"x", 0));

  EXPECT_EQ(9, span_quoted_string_dq(std::string("\"foo\0bar\"x", 10), 0));
  EXPECT_EQ(6, span_quoted_string_dq(std::string("\"\0\0\0\0\"x", 7), 0));

  EXPECT_EQ(std::string::npos, span_quoted_string_dq("\"foo", 0));
  EXPECT_EQ(std::string::npos, span_quoted_string_dq("\"foo\\", 0));

  EXPECT_EQ(std::string::npos, span_quoted_string_dq("\"", 0));
  EXPECT_EQ(std::string::npos, span_quoted_string_dq("\"\\", 0));
  EXPECT_EQ(std::string::npos, span_quoted_string_dq("\"\\\"", 0));
}

TEST(Utils_lexing, span_quoted_string_sq) {
  EXPECT_EQ(2, span_quoted_string_sq("''", 0));
  EXPECT_EQ(3, span_quoted_string_sq("'a'", 0));
  EXPECT_EQ(3, span_quoted_string_sq("'a'a", 0));
  EXPECT_EQ(3, span_quoted_string_sq("'a'aa", 0));
  EXPECT_EQ(4, span_quoted_string_sq("'aa'aa", 0));
  EXPECT_EQ(4, span_quoted_string_sq("'aa'a", 0));

  EXPECT_EQ(2, span_quoted_string_sq("'''", 0));
  EXPECT_EQ(3, span_quoted_string_sq("'a''", 0));

  EXPECT_EQ(1 + 2, span_quoted_string_sq("'''", 1));
  EXPECT_EQ(1 + 3, span_quoted_string_sq("''a'", 1));
  EXPECT_EQ(1 + 3, span_quoted_string_sq("''a'a", 1));
  EXPECT_EQ(1 + 3, span_quoted_string_sq("''a'aa", 1));
  EXPECT_EQ(1 + 4, span_quoted_string_sq("''aa'aa", 1));
  EXPECT_EQ(1 + 4, span_quoted_string_sq("''aa'a", 1));

  EXPECT_EQ(10, span_quoted_string_sq("'foo \"bar'x", 0));
  EXPECT_EQ(11, span_quoted_string_sq("'foo \\'bar'x", 0));
  EXPECT_EQ(11, span_quoted_string_sq("'foo \\'bar'x", 0));
  EXPECT_EQ(12, span_quoted_string_sq("'foo \\'\\bar'x", 0));
  EXPECT_EQ(12, span_quoted_string_sq("'foo \\'\\bar'x", 0));

  EXPECT_EQ(6, span_quoted_string_sq("'foo '\\bar'x", 0));

  EXPECT_EQ(9, span_quoted_string_sq(std::string("'foo\0bar'x", 10), 0));
  EXPECT_EQ(6, span_quoted_string_sq(std::string("'\0\0\0\0'x", 7), 0));

  EXPECT_EQ(std::string::npos, span_quoted_string_sq("'foo", 0));
  EXPECT_EQ(std::string::npos, span_quoted_string_sq("'foo\\", 0));

  EXPECT_EQ(std::string::npos, span_quoted_string_sq("'", 0));
  EXPECT_EQ(std::string::npos, span_quoted_string_sq("'\\", 0));
  EXPECT_EQ(std::string::npos, span_quoted_string_sq("'\\'", 0));
}

TEST(Utils_lexing, span_cstyle_comment) {
  EXPECT_EQ(9, span_cstyle_comment("/* foo */", 0));
  EXPECT_EQ(10, span_cstyle_comment("*/* foo */", 1));
  EXPECT_EQ(7, span_cstyle_comment("/*foo*/", 0));
  EXPECT_EQ(10, span_cstyle_comment("/* foo* */", 0));
  EXPECT_EQ(9, span_cstyle_comment("/* foo */*", 0));
  EXPECT_EQ(10, span_cstyle_comment("/* foo/ */*", 0));
  EXPECT_EQ(4, span_cstyle_comment("/**/", 0));

  EXPECT_EQ(std::string::npos, span_cstyle_comment("/* foo", 0));
  EXPECT_EQ(std::string::npos, span_cstyle_comment("/*/", 0));
}

TEST(Utils_lexing, span_sql_identifier) {
  EXPECT_EQ(5, span_quoted_sql_identifier_bt("`foo` bar", 0));
  EXPECT_EQ(2, span_quoted_sql_identifier_bt("`` bar", 0));
  EXPECT_EQ(6, span_quoted_sql_identifier_bt("`f``o` bar", 0));
  EXPECT_EQ(5, span_quoted_sql_identifier_bt("`f``` bar", 0));
  EXPECT_EQ(5, span_quoted_sql_identifier_bt("```f` bar", 0));
  EXPECT_EQ(4, span_quoted_sql_identifier_bt("```` bar", 0));

  EXPECT_EQ(std::string::npos, span_quoted_sql_identifier_bt("`foo", 0));
}

TEST(Utils_lexing, SQL_string_iterator) {
  std::string ts("# foo * \ns'* '/* foo*  */s\"* \"s-- * \ns");
  SQL_string_iterator it(ts);
  EXPECT_EQ('s', *(it++));
  EXPECT_EQ('s', *it);
  EXPECT_EQ('s', *(++it));
  EXPECT_EQ('s', *(++it));
  EXPECT_EQ(ts.length(), ++it);
  EXPECT_THROW(++it, std::out_of_range);

  std::string ts1(
      "# foo * \nselect '* '/* foo*  */select \"* \" from -- * \n *");
  SQL_string_iterator it1(ts1);
  for (std::size_t i = 0; i < 17; ++i)
    EXPECT_NO_THROW(++it1);
  EXPECT_THROW(++it, std::out_of_range);

  std::string ts2("# test");
  SQL_string_iterator it2(ts2);
  EXPECT_EQ(ts2.length(), it2);
  EXPECT_THROW(++it2, std::out_of_range);
}

}  // namespace utils
}  // namespace mysqlshdk
