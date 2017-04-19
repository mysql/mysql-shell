/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
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
  EXPECT_EQ(6, span_quoted_string_dq(std::string("\"\0\0\0\0\"x", 10), 0));

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
  EXPECT_EQ(6, span_quoted_string_sq(std::string("'\0\0\0\0'x", 10), 0));

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

}  // namespace utils
}  // namespace mysqlshdk
