/* Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.

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

#include <bitset>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <stack>
#include <string>
#include <tuple>

#include "unittest/gtest_clean.h"
#include "utils/utils_string.h"

namespace shcore {

TEST(UtilsString, strip) {
  EXPECT_EQ("", str_strip(""));
  EXPECT_EQ("", str_strip(" \r\n\t"));
  EXPECT_EQ("x", str_strip("   x\t"));
  EXPECT_EQ("xxx\t\txxx", str_strip("   xxx\t\txxx\r\r\r"));
  EXPECT_EQ("xxx", str_strip("xxx\t\t\t"));
  EXPECT_EQ("\txxx\t", str_strip(" \txxx\t ", " "));
  EXPECT_EQ("xxx", str_strip("\r\r\rxxx"));

  EXPECT_EQ("", str_lstrip(""));
  EXPECT_EQ("", str_lstrip(" \r\n\t"));
  EXPECT_EQ("x\t", str_lstrip("   x\t"));
  EXPECT_EQ("xxx\t\txxx\r\r\r", str_lstrip("   xxx\t\txxx\r\r\r"));
  EXPECT_EQ("xxx\t\t\t", str_lstrip("xxx\t\t\t"));
  EXPECT_EQ("\txxx\t ", str_lstrip(" \txxx\t ", " "));
  EXPECT_EQ("xxx", str_lstrip("\r\r\rxxx"));

  EXPECT_EQ("", str_rstrip(""));
  EXPECT_EQ("", str_rstrip(" \r\n\t"));
  EXPECT_EQ("   x", str_rstrip("   x\t"));
  EXPECT_EQ("   xxx\t\txxx", str_rstrip("   xxx\t\txxx\r\r\r"));
  EXPECT_EQ("xxx", str_rstrip("xxx\t\t\t"));
  EXPECT_EQ(" \txxx\t", str_rstrip(" \txxx\t ", " "));
  EXPECT_EQ("\r\r\rxxx", str_rstrip("\r\r\rxxx"));
}

TEST(UtilsString, format) {
  EXPECT_EQ("1", str_format("%i", 1));
  EXPECT_EQ("!", str_format("%c", '!'));
  EXPECT_EQ("1\n2", str_format("%i\n%u", 1, 2));
  EXPECT_EQ("abab\t%", str_format("%s\t%%", "abab"));
  EXPECT_EQ("afoo  bar", str_format("%c%s%5s", 'a', "foo", "bar"));
  EXPECT_EQ("a b", str_format("%s %s", "a", "b"));
  EXPECT_EQ(
      "-1152921504606846976 9223372036854775808",
      str_format("%lli %llu", -1152921504606846976LL, 9223372036854775808ULL));

  EXPECT_EQ(
      "string with over 256charsstring with over 256charsstring with over "
      "256charsstring with over 256charsstring with over 256charsstring with "
      "over 256charsstring with over 256charsstring with over 256charsstring "
      "with over 256charsstring with over 256charsstring with over "
      "256charsstring with over 256charsstring with over 256charsstring with "
      "over 256charsstring with over 256charsstring with over 256chars!",
      str_format(
          "string with over 256charsstring with over 256charsstring with over "
          "256charsstring with over 256charsstring with over 256charsstring "
          "with over 256charsstring with over 256charsstring with over "
          "256charsstring with over 256charsstring with over 256charsstring "
          "with over 256charsstring with over 256charsstring with over "
          "256charsstring with over 256charsstring with over 256charsstring "
          "with over 256chars%c",
          '!'));
  EXPECT_EQ(
      "string with over 256charsstring with over 256charsstring with over "
      "256charsstring with over 256charsstring with over 256charsstring with "
      "over 256charsstring with over 256charsstring with over 256charsstring "
      "with over 256charsstring with over 256charsstring with over "
      "256charsstring with over 256charsstring with over 256charsstring with "
      "over 256charsstring with over 256charsstring with over 256chars",
      str_format(
          "%s",
          "string with over 256charsstring with over 256charsstring with over "
          "256charsstring with over 256charsstring with over 256charsstring "
          "with over 256charsstring with over 256charsstring with over "
          "256charsstring with over 256charsstring with over 256charsstring "
          "with over 256charsstring with over 256charsstring with over "
          "256charsstring with over 256charsstring with over 256charsstring "
          "with over 256chars"));

  EXPECT_EQ(
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
      str_format(
          "%s%s%s%s",
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"));
  EXPECT_EQ(
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
      str_format(
          "%s%s%s%s",
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"));
}

TEST(UtilsString, upperlower) {
  EXPECT_EQ("", str_lower(""));
  EXPECT_EQ("abab", str_lower("ABAB"));
  EXPECT_EQ("abab", str_lower("AbaB"));
  EXPECT_EQ("abab", str_lower("AbaB"));

  EXPECT_EQ("ABAB", str_upper("abab"));
  EXPECT_EQ("ABAB", str_upper("aBab"));
  EXPECT_EQ("ABAB", str_upper("abAb"));
}

TEST(UtilsString, caseeq) {
  EXPECT_TRUE(str_caseeq("a", "a"));
  EXPECT_TRUE(str_caseeq("", ""));
  EXPECT_FALSE(str_caseeq("a", "b"));
  EXPECT_FALSE(str_caseeq("b", "a"));
  EXPECT_FALSE(str_caseeq("a", "bb"));
  EXPECT_FALSE(str_caseeq("b", "ab"));

  EXPECT_TRUE(str_caseeq("a", "A"));
  EXPECT_FALSE(str_caseeq("A", "b"));
  EXPECT_FALSE(str_caseeq("B", "a"));
  EXPECT_FALSE(str_caseeq("a", "bB"));
  EXPECT_FALSE(str_caseeq("b", "Ab"));
}

TEST(UtilsString, casecmp) {
  EXPECT_EQ(0, str_casecmp("a", "a"));
  EXPECT_EQ(0, str_casecmp("", ""));
  EXPECT_EQ(-1, str_casecmp("a", "b"));
  EXPECT_EQ(1, str_casecmp("b", "a"));
  EXPECT_EQ(-1, str_casecmp("a", "bb"));
  EXPECT_EQ(1, str_casecmp("b", "ab"));

  EXPECT_EQ(0, str_casecmp("a", "A"));
  EXPECT_EQ(-1, str_casecmp("A", "b"));
  EXPECT_EQ(1, str_casecmp("B", "a"));
  EXPECT_EQ(-1, str_casecmp("a", "bB"));
  EXPECT_EQ(1, str_casecmp("b", "Ab"));
}

TEST(UtilsString, beginswith) {
  EXPECT_TRUE(str_beginswith("", ""));
  EXPECT_TRUE(str_beginswith("a", ""));
  EXPECT_TRUE(str_beginswith("a", "a"));
  EXPECT_TRUE(str_beginswith("ab", "a"));
  EXPECT_FALSE(str_beginswith("", "a"));
  EXPECT_FALSE(str_beginswith("a", "b"));
  EXPECT_FALSE(str_beginswith("a", "ab"));

  EXPECT_TRUE(str_beginswith(std::string(""), ""));
  EXPECT_TRUE(str_beginswith(std::string("a"), ""));
  EXPECT_TRUE(str_beginswith(std::string("a"), "a"));
  EXPECT_TRUE(str_beginswith(std::string("ab"), "a"));
  EXPECT_FALSE(str_beginswith(std::string(""), "a"));
  EXPECT_FALSE(str_beginswith(std::string("a"), "b"));
  EXPECT_FALSE(str_beginswith(std::string("a"), "ab"));

  EXPECT_TRUE(str_beginswith(std::string(""), std::string("")));
  EXPECT_TRUE(str_beginswith(std::string("a"), std::string("")));
  EXPECT_TRUE(str_beginswith(std::string("a"), std::string("a")));
  EXPECT_TRUE(str_beginswith(std::string("ab"), std::string("a")));
  EXPECT_FALSE(str_beginswith(std::string(""), std::string("a")));
  EXPECT_FALSE(str_beginswith(std::string("a"), std::string("b")));
  EXPECT_FALSE(str_beginswith(std::string("a"), std::string("ab")));
}

TEST(UtilsString, endswith) {
  EXPECT_TRUE(str_endswith("", ""));
  EXPECT_TRUE(str_endswith("a", ""));
  EXPECT_TRUE(str_endswith("a", "a"));
  EXPECT_TRUE(str_endswith("aba", "a"));
  EXPECT_FALSE(str_endswith("ab", "a"));
  EXPECT_FALSE(str_endswith("", "a"));
  EXPECT_FALSE(str_endswith("a", "b"));
  EXPECT_FALSE(str_endswith("a", "ab"));

  EXPECT_TRUE(str_endswith(std::string(""), ""));
  EXPECT_TRUE(str_endswith(std::string("a"), ""));
  EXPECT_TRUE(str_endswith(std::string("a"), "a"));
  EXPECT_TRUE(str_endswith(std::string("aba"), "a"));
  EXPECT_FALSE(str_endswith(std::string("ab"), "a"));
  EXPECT_FALSE(str_endswith(std::string(""), "a"));
  EXPECT_FALSE(str_endswith(std::string("a"), "b"));
  EXPECT_FALSE(str_endswith(std::string("a"), "ab"));

  EXPECT_TRUE(str_endswith(std::string(""), std::string("")));
  EXPECT_TRUE(str_endswith(std::string("a"), std::string("")));
  EXPECT_TRUE(str_endswith(std::string("a"), std::string("a")));
  EXPECT_TRUE(str_endswith(std::string("aba"), std::string("a")));
  EXPECT_FALSE(str_endswith(std::string("ab"), std::string("a")));
  EXPECT_FALSE(str_endswith(std::string(""), std::string("a")));
  EXPECT_FALSE(str_endswith(std::string("a"), std::string("b")));
  EXPECT_FALSE(str_endswith(std::string("a"), std::string("ab")));
}

TEST(UtilsString, partition) {
  std::string left, right;

  std::tie(left, right) = str_partition("", "");
  EXPECT_EQ("", left);
  EXPECT_EQ("", right);
  std::tie(left, right) = str_partition("", "/");
  EXPECT_EQ("", left);
  EXPECT_EQ("", right);
  std::tie(left, right) = str_partition("ab", "/");
  EXPECT_EQ("ab", left);
  EXPECT_EQ("", right);
  std::tie(left, right) = str_partition("ab/", "/");
  EXPECT_EQ("ab", left);
  EXPECT_EQ("", right);
  std::tie(left, right) = str_partition("ab//", "/");
  EXPECT_EQ("ab", left);
  EXPECT_EQ("/", right);
  std::tie(left, right) = str_partition("ab/ab", "/");
  EXPECT_EQ("ab", left);
  EXPECT_EQ("ab", right);
  std::tie(left, right) = str_partition("ab/ab/ab", "/");
  EXPECT_EQ("ab", left);
  EXPECT_EQ("ab/ab", right);
  std::tie(left, right) = str_partition("/ab", "/");
  EXPECT_EQ("", left);
  EXPECT_EQ("ab", right);
  std::tie(left, right) = str_partition("/ab/#ab", "/#");
  EXPECT_EQ("/ab", left);
  EXPECT_EQ("ab", right);
  std::tie(left, right) = str_partition("#/ab/#ab", "/#");
  EXPECT_EQ("#/ab", left);
  EXPECT_EQ("ab", right);
}

TEST(UtilsString, partition_after) {
  std::string left, right;

  std::tie(left, right) = str_partition_after("", "");
  EXPECT_EQ("", left);
  EXPECT_EQ("", right);
  std::tie(left, right) = str_partition_after("", "/");
  EXPECT_EQ("", left);
  EXPECT_EQ("", right);
  std::tie(left, right) = str_partition_after("ab", "/");
  EXPECT_EQ("ab", left);
  EXPECT_EQ("", right);
  std::tie(left, right) = str_partition_after("ab/", "/");
  EXPECT_EQ("ab/", left);
  EXPECT_EQ("", right);
  std::tie(left, right) = str_partition_after("ab//", "/");
  EXPECT_EQ("ab/", left);
  EXPECT_EQ("/", right);
  std::tie(left, right) = str_partition_after("ab/ab", "/");
  EXPECT_EQ("ab/", left);
  EXPECT_EQ("ab", right);
  std::tie(left, right) = str_partition_after("ab/ab/ab", "/");
  EXPECT_EQ("ab/", left);
  EXPECT_EQ("ab/ab", right);
  std::tie(left, right) = str_partition_after("/ab", "/");
  EXPECT_EQ("/", left);
  EXPECT_EQ("ab", right);
  std::tie(left, right) = str_partition_after("/ab/#ab", "/#");
  EXPECT_EQ("/ab/#", left);
  EXPECT_EQ("ab", right);
  std::tie(left, right) = str_partition_after("#/ab/#ab", "/#");
  EXPECT_EQ("#/ab/#", left);
  EXPECT_EQ("ab", right);
}

TEST(UtilsString, join) {
  EXPECT_EQ("", str_join(std::vector<std::string>{}, ""));
  EXPECT_EQ("", str_join(std::vector<std::string>{}, ","));
  EXPECT_EQ("a", str_join(std::vector<std::string>{"a"}, ","));
  EXPECT_EQ("a,b", str_join(std::vector<std::string>{"a", "b"}, ","));
  EXPECT_EQ(",b", str_join(std::vector<std::string>{"", "b"}, ","));
  EXPECT_EQ("a,", str_join(std::vector<std::string>{"a", ""}, ","));
  EXPECT_EQ(",", str_join(std::vector<std::string>{"", ""}, ","));
  EXPECT_EQ("a,b", str_join(std::set<std::string>({"a", "b"}), ","));

  const char *args[] = {"a", "b", nullptr};
  EXPECT_EQ("a,b", str_join(args, args + 2, ","));
  EXPECT_EQ("a", str_join(args, args + 1, ","));

  std::vector<std::string> v({"a", "b"});
  EXPECT_EQ("a,b", str_join(v.begin(), v.end(), ","));

  std::set<std::string> s({"a", "b"});
  EXPECT_EQ("a,b", str_join(s.begin(), s.end(), ","));
}

TEST(UtilsString, replace) {
  EXPECT_EQ("", str_replace("", "", ""));

  EXPECT_EQ("", str_replace("", "fo", ""));
  EXPECT_EQ("", str_replace("", "fo", "bar"));
  EXPECT_EQ("bar", str_replace("", "", "bar"));

  EXPECT_EQ("foo", str_replace("foo", "", ""));
  EXPECT_EQ("o", str_replace("foo", "fo", ""));
  EXPECT_EQ("baro", str_replace("foo", "fo", "bar"));
  EXPECT_EQ("barfbarobarobar", str_replace("foo", "", "bar"));

  EXPECT_EQ("fofoo", str_replace("fofoo", "", ""));
  EXPECT_EQ("o", str_replace("fofoo", "fo", ""));
  EXPECT_EQ("barbaro", str_replace("fofoo", "fo", "bar"));
  EXPECT_EQ("barfbarobarfbarobarobar", str_replace("fofoo", "", "bar"));

  EXPECT_EQ("barfoo", str_replace("barfoo", "", ""));
  EXPECT_EQ("baro", str_replace("barfoo", "fo", ""));
  EXPECT_EQ("barbaro", str_replace("barfoo", "fo", "bar"));
  EXPECT_EQ("barbbarabarrbarfbarobarobar", str_replace("barfoo", "", "bar"));

  EXPECT_EQ("barfo", str_replace("barfo", "", ""));
  EXPECT_EQ("bar", str_replace("barfo", "fo", ""));
  EXPECT_EQ("barbar", str_replace("barfo", "fo", "bar"));
  EXPECT_EQ("barbbarabarrbarfbarobar", str_replace("barfo", "", "bar"));

  EXPECT_EQ("fofo", str_replace("fofo", "", ""));
  EXPECT_EQ("", str_replace("fofo", "fo", ""));
  EXPECT_EQ("barbar", str_replace("fofo", "fo", "bar"));
  EXPECT_EQ("barfbarobarfbarobar", str_replace("fofo", "", "bar"));

  EXPECT_EQ("bar", str_replace("bar", "", ""));
  EXPECT_EQ("bar", str_replace("bar", "fo", ""));
  EXPECT_EQ("bar", str_replace("bar", "fo", "bar"));
  EXPECT_EQ("barbbarabarrbar", str_replace("bar", "", "bar"));
}

#define MAKE_BITS(strvalue) std::bitset<sizeof(strvalue) - 1>(strvalue)

#define TEST_BIT(strvalue)             \
  EXPECT_EQ(                           \
      MAKE_BITS(strvalue).to_string(), \
      bits_to_string(MAKE_BITS(strvalue).to_ullong(), sizeof(strvalue) - 1));

TEST(UtilsString, bits_to_string) {
  // preliminary tests
  ASSERT_EQ(0, MAKE_BITS("0").to_ullong());
  ASSERT_EQ(0, MAKE_BITS("0000").to_ullong());
  ASSERT_EQ(1, MAKE_BITS("1").to_ullong());
  ASSERT_EQ(255, MAKE_BITS("11111111").to_ullong());
  ASSERT_EQ(510, MAKE_BITS("111111110").to_ullong());

  TEST_BIT("0");
  TEST_BIT("1");
  TEST_BIT("000");
  TEST_BIT("0001");
  TEST_BIT("00011");
  TEST_BIT("11111111");
  TEST_BIT("111111110");
  TEST_BIT("1111111100000000");
  TEST_BIT("11111111000000000");
  TEST_BIT("00000000");
  TEST_BIT("000000000");
  TEST_BIT("0000000011");
  TEST_BIT("000000001");
  TEST_BIT("1111111111111111111111111111111111111111111111111111111111111111");
  TEST_BIT("1010101010101010101010101010101010101010101010101010101010101010");
  TEST_BIT("0000111100001111000011110000111100001111000011110000111100001111");
  TEST_BIT("1111000011110000111100001111000011110000111100001111000011110000");
}

#undef TEST_BIT
#define TEST_BIT(strvalue)                                                    \
  EXPECT_EQ(MAKE_BITS(strvalue).to_ullong(), string_to_bits(strvalue).first); \
  EXPECT_EQ(sizeof(strvalue) - 1, string_to_bits(strvalue).second)

TEST(UtilsString, string_to_bits) {
  TEST_BIT("0");
  TEST_BIT("1");
  TEST_BIT("000");
  TEST_BIT("0001");
  TEST_BIT("00011");
  TEST_BIT("11111111");
  TEST_BIT("111111110");
  TEST_BIT("00000000");
  TEST_BIT("000000000");
  TEST_BIT("0000000011");
  TEST_BIT("000000001");
  TEST_BIT("1111111111111111111111111111111111111111111111111111111111111111");
  TEST_BIT("1010101010101010101010101010101010101010101010101010101010101010");
  TEST_BIT("0000111100001111000011110000111100001111000011110000111100001111");
  TEST_BIT("1111000011110000111100001111000011110000111100001111000011110000");
}

TEST(UtilsString, split) {
  {
    const auto s = str_split("one/two/three/", "/");
    const std::vector<std::string> expect = {"one", "two", "three", ""};
    EXPECT_EQ(expect, s);
  }
  {
    const auto s = str_split("", "/");
    const std::vector<std::string> expect = {};
    EXPECT_EQ(expect, s);
  }
  {
    const auto s = str_split(" ", "/");
    const std::vector<std::string> expect = {" "};
    EXPECT_EQ(expect, s);
  }
  {
    const auto s = str_split("/", "/");
    const std::vector<std::string> expect = {"", ""};
    EXPECT_EQ(expect, s);
  }
  {
    const auto s = str_split("//", "/");
    const std::vector<std::string> expect = {"", "", ""};
    EXPECT_EQ(expect, s);
  }
  {
    const auto s = str_split("//", "/", -1, true);
    const std::vector<std::string> expect = {"", ""};
    EXPECT_EQ(expect, s);
  }
  {
    const auto s = str_split("//a", "/");
    const std::vector<std::string> expect = {"", "", "a"};
    EXPECT_EQ(expect, s);
  }
  {
    const auto s = str_split("a.b.c.d.e.f", ".", 3);
    const std::vector<std::string> expect = {"a", "b", "c", "d.e.f"};
    EXPECT_EQ(expect, s);
  }
}

TEST(UtilsString, ljust) {
  EXPECT_EQ("        ", str_ljust("", 8));
  EXPECT_EQ("x       ", str_ljust("x", 8));
  EXPECT_EQ("xxxy    ", str_ljust("xxxy", 8));
  EXPECT_EQ("xxxxxxxy", str_ljust("xxxxxxxy", 8));
  EXPECT_EQ("xxxxxxxxy", str_ljust("xxxxxxxxy", 8));
  EXPECT_EQ("xxxy", str_ljust("xxxy", 0));

  EXPECT_EQ("........", str_ljust("", 8, '.'));
  EXPECT_EQ("x.......", str_ljust("x", 8, '.'));
  EXPECT_EQ("xxxy....", str_ljust("xxxy", 8, '.'));
}

TEST(UtilsString, rjust) {
  EXPECT_EQ("        ", str_rjust("", 8));
  EXPECT_EQ("       x", str_rjust("x", 8));
  EXPECT_EQ("    xxxy", str_rjust("xxxy", 8));
  EXPECT_EQ("xxxxxxxy", str_rjust("xxxxxxxy", 8));
  EXPECT_EQ("xxxxxxxxy", str_rjust("xxxxxxxxy", 8));
  EXPECT_EQ("xxxy", str_rjust("xxxy", 0));

  EXPECT_EQ("........", str_rjust("", 8, '.'));
  EXPECT_EQ(".......x", str_rjust("x", 8, '.'));
  EXPECT_EQ("....xxxy", str_rjust("xxxy", 8, '.'));
}

TEST(UtilsString, str_break_into_lines) {
  const char *text1 = "1234 678 910 1234 1234 12345\t1111111111111111111";
  auto res = str_break_into_lines(text1, 14);
  ASSERT_EQ(4, res.size());
  EXPECT_EQ("1234 678 910", res[0]);
  EXPECT_EQ("1234 1234", res[1]);
  EXPECT_EQ("12345", res[2]);
  EXPECT_EQ("1111111111111111111", res[3]);

  const char *text2 = "1234 678 910 \n\n1234 1234 12345\t1111111111111111111";
  res = str_break_into_lines(text2, 14);
  ASSERT_EQ(5, res.size());
  EXPECT_EQ("1234 678 910 ", res[0]);
  EXPECT_EQ("", res[1]);
  EXPECT_EQ("1234 1234", res[2]);
  EXPECT_EQ("12345", res[3]);
  EXPECT_EQ("1111111111111111111", res[4]);

  const char *text3 = "12346789 12345 12346789";
  res = str_break_into_lines(text3, 7);
  ASSERT_EQ(3, res.size());
  EXPECT_EQ("12346789", res[0]);
  EXPECT_EQ("12345", res[1]);
  EXPECT_EQ("12346789", res[2]);

  const char *text4 = "\n\n\n";
  res = str_break_into_lines(text4, 7);
  ASSERT_EQ(3, res.size());
  EXPECT_EQ("", res[0]);
  EXPECT_EQ("", res[1]);
  EXPECT_EQ("", res[2]);
}

TEST(UtilsString, quote_string) {
  EXPECT_EQ("\"\"", quote_string("", '"'));
  EXPECT_EQ("''", quote_string("", '\''));
  EXPECT_EQ("\"\\\"\\\"\"", quote_string("\"\"", '"'));
  EXPECT_EQ("'\"\"'", quote_string("\"\"", '\''));
  EXPECT_EQ("\"\\\"string\\\"\"", quote_string("\"string\"", '"'));
  EXPECT_EQ("'\"string\"'", quote_string("\"string\"", '\''));
  EXPECT_EQ("\"some \\\\ string\"", quote_string("some \\ string", '"'));
  EXPECT_EQ("'some \\\\ string'", quote_string("some \\ string", '\''));
  EXPECT_EQ("\"\\\"some \\\\ string\\\"\"",
            quote_string("\"some \\ string\"", '"'));
  EXPECT_EQ("'\"some \\\\ string\"'", quote_string("\"some \\ string\"", '\''));
  EXPECT_EQ("\"some \\\"\\\\ string\\\"\"",
            quote_string("some \"\\ string\"", '"'));
  EXPECT_EQ("'some \"\\\\ string\"'", quote_string("some \"\\ string\"", '\''));
  EXPECT_EQ("\"some \\\\\\\" string\\\"\"",
            quote_string("some \\\" string\"", '"'));
  EXPECT_EQ("'some \\\\\" string\"'", quote_string("some \\\" string\"", '\''));
}

TEST(UtilsString, unquote_string) {
  for (const auto quote : {'\'', '"'}) {
    for (const auto s :
         {"", "\"\"", "''", "\"string\"", "'string'", "some \\ string",
          "\"some \\ string\"", "'some \\ string'", "some \"\\ string\"",
          "some '\\ string'", "some \\\" string\"", "some \\' string'"}) {
      EXPECT_EQ(s, unquote_string(quote_string(s, quote), quote));
    }
  }
}

TEST(UtilsString, str_span) {
  EXPECT_EQ(std::string::npos, str_span("", ""));
  EXPECT_EQ(0, str_span("", "1"));
  EXPECT_EQ(0, str_span("1", ""));
  EXPECT_EQ(0, str_span("x", "1"));
  EXPECT_EQ(0, str_span("1", "x"));
  EXPECT_EQ(1, str_span("x", "x1"));
  EXPECT_EQ(std::string::npos, str_span("abc", "abc"));
  EXPECT_EQ(3, str_span("abc", "abcd"));
  EXPECT_EQ(2, str_span("abbd", "abcd"));
}

}  // namespace shcore
