/* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

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
#include <stack>
#include <tuple>

#include "gtest/gtest.h"
#include "utils/utils_string.h"

TEST(UtilsString, strip) {
  EXPECT_EQ("", shcore::str_strip(""));
  EXPECT_EQ("", shcore::str_strip(" \r\n\t"));
  EXPECT_EQ("x", shcore::str_strip("   x\t"));
  EXPECT_EQ("xxx\t\txxx", shcore::str_strip("   xxx\t\txxx\r\r\r"));
  EXPECT_EQ("xxx", shcore::str_strip("xxx\t\t\t"));
  EXPECT_EQ("\txxx\t", shcore::str_strip(" \txxx\t ", " "));
  EXPECT_EQ("xxx", shcore::str_strip("\r\r\rxxx"));

  EXPECT_EQ("", shcore::str_lstrip(""));
  EXPECT_EQ("", shcore::str_lstrip(" \r\n\t"));
  EXPECT_EQ("x\t", shcore::str_lstrip("   x\t"));
  EXPECT_EQ("xxx\t\txxx\r\r\r", shcore::str_lstrip("   xxx\t\txxx\r\r\r"));
  EXPECT_EQ("xxx\t\t\t", shcore::str_lstrip("xxx\t\t\t"));
  EXPECT_EQ("\txxx\t ", shcore::str_lstrip(" \txxx\t ", " "));
  EXPECT_EQ("xxx", shcore::str_lstrip("\r\r\rxxx"));

  EXPECT_EQ("", shcore::str_rstrip(""));
  EXPECT_EQ("", shcore::str_rstrip(" \r\n\t"));
  EXPECT_EQ("   x", shcore::str_rstrip("   x\t"));
  EXPECT_EQ("   xxx\t\txxx", shcore::str_rstrip("   xxx\t\txxx\r\r\r"));
  EXPECT_EQ("xxx", shcore::str_rstrip("xxx\t\t\t"));
  EXPECT_EQ(" \txxx\t", shcore::str_rstrip(" \txxx\t ", " "));
  EXPECT_EQ("\r\r\rxxx", shcore::str_rstrip("\r\r\rxxx"));
}

TEST(UtilsString, format) {
  EXPECT_EQ("1", shcore::str_format("%i", 1));
  EXPECT_EQ("!", shcore::str_format("%c", '!'));
  EXPECT_EQ("1\n2", shcore::str_format("%i\n%u", 1, 2));
  EXPECT_EQ("abab\t%", shcore::str_format("%s\t%%", "abab"));
  EXPECT_EQ("afoo  bar", shcore::str_format("%c%s%5s", 'a', "foo", "bar"));
  EXPECT_EQ("a b", shcore::str_format("%s %s", "a", "b"));
  EXPECT_EQ("-1152921504606846976 9223372036854775808", shcore::str_format("%lli %llu", -1152921504606846976LL, 9223372036854775808ULL));

  EXPECT_EQ("string with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256chars!", shcore::str_format("string with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256chars%c", '!'));
  EXPECT_EQ("string with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256chars", shcore::str_format("%s", "string with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256charsstring with over 256chars"));

  EXPECT_EQ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
      shcore::str_format("%s%s%s%s", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"));
  EXPECT_EQ("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
      shcore::str_format("%s%s%s%s", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
        "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
        "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
        "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"));
}

TEST(UtilsString, upperlower) {
  EXPECT_EQ("", shcore::str_lower(""));
  EXPECT_EQ("abab", shcore::str_lower("ABAB"));
  EXPECT_EQ("abab", shcore::str_lower("AbaB"));
  EXPECT_EQ("abab", shcore::str_lower("AbaB"));

  EXPECT_EQ("ABAB", shcore::str_upper("abab"));
  EXPECT_EQ("ABAB", shcore::str_upper("aBab"));
  EXPECT_EQ("ABAB", shcore::str_upper("abAb"));
}

TEST(UtilsString, caseeq) {
  EXPECT_TRUE(shcore::str_caseeq("a", "a"));
  EXPECT_TRUE(shcore::str_caseeq("", ""));
  EXPECT_FALSE(shcore::str_caseeq("a", "b"));
  EXPECT_FALSE(shcore::str_caseeq("b", "a"));
  EXPECT_FALSE(shcore::str_caseeq("a", "bb"));
  EXPECT_FALSE(shcore::str_caseeq("b", "ab"));

  EXPECT_TRUE(shcore::str_caseeq("a", "A"));
  EXPECT_FALSE(shcore::str_caseeq("A", "b"));
  EXPECT_FALSE(shcore::str_caseeq("B", "a"));
  EXPECT_FALSE(shcore::str_caseeq("a", "bB"));
  EXPECT_FALSE(shcore::str_caseeq("b", "Ab"));
}

TEST(UtilsString, casecmp) {
  EXPECT_EQ(0, shcore::str_casecmp("a", "a"));
  EXPECT_EQ(0, shcore::str_casecmp("", ""));
  EXPECT_EQ(-1, shcore::str_casecmp("a", "b"));
  EXPECT_EQ(1, shcore::str_casecmp("b", "a"));
  EXPECT_EQ(-1, shcore::str_casecmp("a", "bb"));
  EXPECT_EQ(1, shcore::str_casecmp("b", "ab"));

  EXPECT_EQ(0, shcore::str_casecmp("a", "A"));
  EXPECT_EQ(-1, shcore::str_casecmp("A", "b"));
  EXPECT_EQ(1, shcore::str_casecmp("B", "a"));
  EXPECT_EQ(-1, shcore::str_casecmp("a", "bB"));
  EXPECT_EQ(1, shcore::str_casecmp("b", "Ab"));
}

TEST(UtilsString, beginswith) {
  EXPECT_TRUE(shcore::str_beginswith("", ""));
  EXPECT_TRUE(shcore::str_beginswith("a", ""));
  EXPECT_TRUE(shcore::str_beginswith("a", "a"));
  EXPECT_TRUE(shcore::str_beginswith("ab", "a"));
  EXPECT_FALSE(shcore::str_beginswith("", "a"));
  EXPECT_FALSE(shcore::str_beginswith("a", "b"));
  EXPECT_FALSE(shcore::str_beginswith("a", "ab"));

  EXPECT_TRUE(shcore::str_beginswith(std::string(""), ""));
  EXPECT_TRUE(shcore::str_beginswith(std::string("a"), ""));
  EXPECT_TRUE(shcore::str_beginswith(std::string("a"), "a"));
  EXPECT_TRUE(shcore::str_beginswith(std::string("ab"), "a"));
  EXPECT_FALSE(shcore::str_beginswith(std::string(""), "a"));
  EXPECT_FALSE(shcore::str_beginswith(std::string("a"), "b"));
  EXPECT_FALSE(shcore::str_beginswith(std::string("a"), "ab"));

  EXPECT_TRUE(shcore::str_beginswith(std::string(""), std::string("")));
  EXPECT_TRUE(shcore::str_beginswith(std::string("a"), std::string("")));
  EXPECT_TRUE(shcore::str_beginswith(std::string("a"), std::string("a")));
  EXPECT_TRUE(shcore::str_beginswith(std::string("ab"), std::string("a")));
  EXPECT_FALSE(shcore::str_beginswith(std::string(""), std::string("a")));
  EXPECT_FALSE(shcore::str_beginswith(std::string("a"), std::string("b")));
  EXPECT_FALSE(shcore::str_beginswith(std::string("a"), std::string("ab")));
}

TEST(UtilsString, endswith) {
  EXPECT_TRUE(shcore::str_endswith("", ""));
  EXPECT_TRUE(shcore::str_endswith("a", ""));
  EXPECT_TRUE(shcore::str_endswith("a", "a"));
  EXPECT_TRUE(shcore::str_endswith("aba", "a"));
  EXPECT_FALSE(shcore::str_endswith("ab", "a"));
  EXPECT_FALSE(shcore::str_endswith("", "a"));
  EXPECT_FALSE(shcore::str_endswith("a", "b"));
  EXPECT_FALSE(shcore::str_endswith("a", "ab"));

  EXPECT_TRUE(shcore::str_endswith(std::string(""), ""));
  EXPECT_TRUE(shcore::str_endswith(std::string("a"), ""));
  EXPECT_TRUE(shcore::str_endswith(std::string("a"), "a"));
  EXPECT_TRUE(shcore::str_endswith(std::string("aba"), "a"));
  EXPECT_FALSE(shcore::str_endswith(std::string("ab"), "a"));
  EXPECT_FALSE(shcore::str_endswith(std::string(""), "a"));
  EXPECT_FALSE(shcore::str_endswith(std::string("a"), "b"));
  EXPECT_FALSE(shcore::str_endswith(std::string("a"), "ab"));

  EXPECT_TRUE(shcore::str_endswith(std::string(""), std::string("")));
  EXPECT_TRUE(shcore::str_endswith(std::string("a"), std::string("")));
  EXPECT_TRUE(shcore::str_endswith(std::string("a"), std::string("a")));
  EXPECT_TRUE(shcore::str_endswith(std::string("aba"), std::string("a")));
  EXPECT_FALSE(shcore::str_endswith(std::string("ab"), std::string("a")));
  EXPECT_FALSE(shcore::str_endswith(std::string(""), std::string("a")));
  EXPECT_FALSE(shcore::str_endswith(std::string("a"), std::string("b")));
  EXPECT_FALSE(shcore::str_endswith(std::string("a"), std::string("ab")));
}

TEST(UtilsString, partition) {
  std::string left, right;

  std::tie(left, right) = shcore::str_partition("", "");
  EXPECT_EQ("", left);
  EXPECT_EQ("", right);
  std::tie(left, right) = shcore::str_partition("", "/");
  EXPECT_EQ("", left);
  EXPECT_EQ("", right);
  std::tie(left, right) = shcore::str_partition("ab", "/");
  EXPECT_EQ("ab", left);
  EXPECT_EQ("", right);
  std::tie(left, right) = shcore::str_partition("ab/", "/");
  EXPECT_EQ("ab", left);
  EXPECT_EQ("", right);
  std::tie(left, right) = shcore::str_partition("ab//", "/");
  EXPECT_EQ("ab", left);
  EXPECT_EQ("/", right);
  std::tie(left, right) = shcore::str_partition("ab/ab", "/");
  EXPECT_EQ("ab", left);
  EXPECT_EQ("ab", right);
  std::tie(left, right) = shcore::str_partition("ab/ab/ab", "/");
  EXPECT_EQ("ab", left);
  EXPECT_EQ("ab/ab", right);
  std::tie(left, right) = shcore::str_partition("/ab", "/");
  EXPECT_EQ("", left);
  EXPECT_EQ("ab", right);
  std::tie(left, right) = shcore::str_partition("/ab/#ab", "/#");
  EXPECT_EQ("/ab", left);
  EXPECT_EQ("ab", right);
  std::tie(left, right) = shcore::str_partition("#/ab/#ab", "/#");
  EXPECT_EQ("#/ab", left);
  EXPECT_EQ("ab", right);
}
