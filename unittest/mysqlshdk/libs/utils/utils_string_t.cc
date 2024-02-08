/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <bitset>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <stack>
#include <string>
#include <tuple>

#include "unittest/gtest_clean.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

namespace shcore {

using namespace std::literals;

TEST(utils_string, strip) {
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

TEST(utils_string, format) {
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

TEST(utils_string, upperlower) {
  EXPECT_EQ("", str_lower(""));
  EXPECT_EQ("abab", str_lower("ABAB"));
  EXPECT_EQ("abab", str_lower("AbaB"));
  EXPECT_EQ("abab", str_lower("AbaB"));

  EXPECT_EQ("ABAB", str_upper("abab"));
  EXPECT_EQ("ABAB", str_upper("aBab"));
  EXPECT_EQ("ABAB", str_upper("abAb"));
}

TEST(utils_string, caseeq) {
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

  EXPECT_TRUE(str_caseeq("a"sv, "a"sv));
  EXPECT_TRUE(str_caseeq("", ""sv));
  EXPECT_FALSE(str_caseeq("a"sv, "b"));
  EXPECT_FALSE(str_caseeq("b", "a"sv));
  EXPECT_FALSE(str_caseeq("a"sv, "bb"));
  EXPECT_FALSE(str_caseeq("b", "ab"sv));

  EXPECT_TRUE(str_caseeq("a"sv, "A"sv));
  EXPECT_FALSE(str_caseeq("A", "b"));
  EXPECT_FALSE(str_caseeq("B", "a"sv));
  EXPECT_FALSE(str_caseeq("a", "bB"sv));
  EXPECT_FALSE(str_caseeq("b"sv, "Ab"sv));
}

TEST(utils_string, casecmp) {
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

TEST(utils_string, beginswith) {
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

  EXPECT_TRUE(str_beginswith(""sv, ""));
  EXPECT_TRUE(str_beginswith("a", ""sv));
  EXPECT_TRUE(str_beginswith("a", "a"));
  EXPECT_TRUE(str_beginswith("ab"sv, "a"sv));
  EXPECT_FALSE(str_beginswith("", "a"sv));
  EXPECT_FALSE(str_beginswith("a"sv, "b"));
  EXPECT_FALSE(str_beginswith("a"sv, "ab"sv));

  EXPECT_TRUE(str_beginswith(""sv, ""));
  EXPECT_TRUE(str_beginswith("a"sv, ""));
  EXPECT_TRUE(str_beginswith("a"sv, "a"));
  EXPECT_TRUE(str_beginswith("ab"sv, "a"));
  EXPECT_FALSE(str_beginswith(""sv, "a"));
  EXPECT_FALSE(str_beginswith("a"sv, "b"));
  EXPECT_FALSE(str_beginswith("a"sv, "ab"));

  EXPECT_TRUE(str_beginswith(std::string(""), ""sv));
  EXPECT_TRUE(str_beginswith(std::string("a"), ""sv));
  EXPECT_TRUE(str_beginswith(std::string("a"), "a"sv));
  EXPECT_TRUE(str_beginswith(std::string("ab"), "a"sv));
  EXPECT_FALSE(str_beginswith(std::string(""), "a"sv));
  EXPECT_FALSE(str_beginswith(std::string("a"), "b"sv));
  EXPECT_FALSE(str_beginswith(std::string("a"), "ab"sv));

  EXPECT_TRUE(str_beginswith(""sv, ""sv));
  EXPECT_TRUE(str_beginswith("a"sv, ""sv));
  EXPECT_TRUE(str_beginswith("a"sv, "a"sv));
  EXPECT_TRUE(str_beginswith("ab"sv, "a"sv));
  EXPECT_FALSE(str_beginswith(""sv, "a"sv));
  EXPECT_FALSE(str_beginswith("a"sv, "b"sv));
  EXPECT_FALSE(str_beginswith("a"sv, "ab"sv));
}

TEST(utils_string, ibeginswith) {
  EXPECT_TRUE(str_ibeginswith("", ""));
  EXPECT_TRUE(str_ibeginswith("a", ""));
  EXPECT_TRUE(str_ibeginswith("a", "A"));
  EXPECT_TRUE(str_ibeginswith("ab", "A"));
  EXPECT_FALSE(str_ibeginswith("", "A"));
  EXPECT_FALSE(str_ibeginswith("a", "B"));
  EXPECT_FALSE(str_ibeginswith("a", "AB"));

  EXPECT_TRUE(str_ibeginswith(std::string(""), ""));
  EXPECT_TRUE(str_ibeginswith(std::string("a"), ""));
  EXPECT_TRUE(str_ibeginswith(std::string("a"), "A"));
  EXPECT_TRUE(str_ibeginswith(std::string("ab"), "A"));
  EXPECT_FALSE(str_ibeginswith(std::string(""), "A"));
  EXPECT_FALSE(str_ibeginswith(std::string("a"), "B"));
  EXPECT_FALSE(str_ibeginswith(std::string("a"), "AB"));

  EXPECT_TRUE(str_ibeginswith(std::string(""), std::string("")));
  EXPECT_TRUE(str_ibeginswith(std::string("a"), std::string("")));
  EXPECT_TRUE(str_ibeginswith(std::string("a"), std::string("A")));
  EXPECT_TRUE(str_ibeginswith(std::string("ab"), std::string("A")));
  EXPECT_FALSE(str_ibeginswith(std::string(""), std::string("A")));
  EXPECT_FALSE(str_ibeginswith(std::string("a"), std::string("B")));
  EXPECT_FALSE(str_ibeginswith(std::string("a"), std::string("AB")));

  EXPECT_TRUE(str_ibeginswith(""sv, ""));
  EXPECT_TRUE(str_ibeginswith("a", ""sv));
  EXPECT_TRUE(str_ibeginswith("a", "A"));
  EXPECT_TRUE(str_ibeginswith("ab"sv, "A"sv));
  EXPECT_FALSE(str_ibeginswith("", "A"sv));
  EXPECT_FALSE(str_ibeginswith("a"sv, "B"));
  EXPECT_FALSE(str_ibeginswith("a"sv, "AB"sv));

  EXPECT_TRUE(str_ibeginswith(""sv, ""));
  EXPECT_TRUE(str_ibeginswith("a"sv, ""));
  EXPECT_TRUE(str_ibeginswith("a"sv, "A"));
  EXPECT_TRUE(str_ibeginswith("ab"sv, "A"));
  EXPECT_FALSE(str_ibeginswith(""sv, "A"));
  EXPECT_FALSE(str_ibeginswith("a"sv, "B"));
  EXPECT_FALSE(str_ibeginswith("a"sv, "AB"));

  EXPECT_TRUE(str_ibeginswith(std::string(""), ""sv));
  EXPECT_TRUE(str_ibeginswith(std::string("a"), ""sv));
  EXPECT_TRUE(str_ibeginswith(std::string("a"), "A"sv));
  EXPECT_TRUE(str_ibeginswith(std::string("ab"), "A"sv));
  EXPECT_FALSE(str_ibeginswith(std::string(""), "A"sv));
  EXPECT_FALSE(str_ibeginswith(std::string("a"), "B"sv));
  EXPECT_FALSE(str_ibeginswith(std::string("a"), "AB"sv));

  EXPECT_TRUE(str_ibeginswith(""sv, ""sv));
  EXPECT_TRUE(str_ibeginswith("a"sv, ""sv));
  EXPECT_TRUE(str_ibeginswith("a"sv, "A"sv));
  EXPECT_TRUE(str_ibeginswith("ab"sv, "A"sv));
  EXPECT_FALSE(str_ibeginswith(""sv, "A"sv));
  EXPECT_FALSE(str_ibeginswith("a"sv, "B"sv));
  EXPECT_FALSE(str_ibeginswith("a"sv, "AB"sv));
}

TEST(utils_string, endswith) {
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

  EXPECT_TRUE(str_endswith("aba", "a", "b"));
  EXPECT_TRUE(str_endswith("aba", "b", "a"));
  EXPECT_FALSE(str_endswith("aba", "b", "c"));

  EXPECT_TRUE(str_endswith(std::string("aba"), "a", "b"));
  EXPECT_TRUE(str_endswith(std::string("aba"), std::string("b"), "a"));
  EXPECT_FALSE(
      str_endswith(std::string("aba"), std::string("b"), std::string("c")));

  EXPECT_TRUE(str_endswith("aba", std::string("a"), "b"));
  EXPECT_TRUE(str_endswith("aba", "b", std::string("a")));
}

TEST(utils_string, partition) {
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

TEST(utils_string, partition_after) {
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

TEST(utils_string, join) {
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

TEST(utils_string, join_f) {
  struct Info {
    std::string s;
  };
  std::vector<Info> l;
  l.push_back(Info{"foo"});
  l.push_back(Info{"bar"});
  l.push_back(Info{"baz"});

  EXPECT_EQ("foo,bar,baz", str_join(l, ",", [](const Info &i) { return i.s; }));
}

TEST(utils_string, replace) {
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

TEST(utils_string, bits_to_string) {
  // preliminary tests
  ASSERT_EQ(0, MAKE_BITS("0").to_ullong());
  ASSERT_EQ(0, MAKE_BITS("0000").to_ullong());
  ASSERT_EQ(1, MAKE_BITS("1").to_ullong());
  ASSERT_EQ(255, MAKE_BITS("11111111").to_ullong());
  ASSERT_EQ(510, MAKE_BITS("111111110").to_ullong());
  ASSERT_EQ(2817036, MAKE_BITS("01010101111110000001100").to_ullong());

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
  TEST_BIT("000000001");
  TEST_BIT("0000000011");
  TEST_BIT("00000000111");
  TEST_BIT("000000001111");
  TEST_BIT("0000000011111");
  TEST_BIT("00000000111111");
  TEST_BIT("000000001111111");
  TEST_BIT("0000000011111111");
  TEST_BIT("00000000111111111");
  TEST_BIT("000000001111111111");
  TEST_BIT("0000000011111111111");
  TEST_BIT("00000000111111111111");
  TEST_BIT("000000001111111111111");
  TEST_BIT("0000000011111111111111");
  TEST_BIT("00000000111111111111111");
  TEST_BIT("000000001111111111111111");
  TEST_BIT("0000000011111111111111111");
  TEST_BIT("00000000111111111111111111");
  TEST_BIT("000000001111111111111111111");
  TEST_BIT("0000000011111111111111111111");
  TEST_BIT("00000000111111111111111111111");
  TEST_BIT("000000001111111111111111111111");
  TEST_BIT("0000000011111111111111111111111");
  TEST_BIT("00000000111111111111111111111111");
  TEST_BIT("000000001111111111111111111111111");
  TEST_BIT("0000000011111111111111111111111111");
  TEST_BIT("1111111111111111111111111111111111111111111111111111111111111111");
  TEST_BIT("1010101010101010101010101010101010101010101010101010101010101010");
  TEST_BIT("0000111100001111000011110000111100001111000011110000111100001111");
  TEST_BIT("1111000011110000111100001111000011110000111100001111000011110000");
}

#define TEST_BIT_HEX(strhex, strbin)                                  \
  EXPECT_EQ(strhex, bits_to_string_hex(MAKE_BITS(strbin).to_ullong(), \
                                       sizeof(strbin) - 1));

TEST(utils_string, bits_to_string_hex) {
  TEST_BIT_HEX("0x00", "0");
  TEST_BIT_HEX("0x01", "1");
  TEST_BIT_HEX("0x00", "000");
  TEST_BIT_HEX("0x01", "0001");
  TEST_BIT_HEX("0x03", "00011");
  TEST_BIT_HEX("0xFF", "11111111");
  TEST_BIT_HEX("0x01FE", "111111110");
  TEST_BIT_HEX("0xFF00", "1111111100000000");
  TEST_BIT_HEX("0x01FE00", "11111111000000000");
  TEST_BIT_HEX("0x00000000015695",
               "00000000000000000000000000000000000000010101011010010101");
  TEST_BIT_HEX(
      "0x00000000002AFC0C",
      "0000000000000000000000000000000000000000001010101111110000001100");
  TEST_BIT_HEX(
      "0xFFFFFFFFFFFFFFFF",
      "1111111111111111111111111111111111111111111111111111111111111111");
  TEST_BIT_HEX(
      "0xAAAAAAAAAAAAAAAA",
      "1010101010101010101010101010101010101010101010101010101010101010");
  TEST_BIT_HEX(
      "0x0F0F0F0F0F0F0F0F",
      "0000111100001111000011110000111100001111000011110000111100001111");
  TEST_BIT_HEX(
      "0xF0F0F0F0F0F0F0F0",
      "1111000011110000111100001111000011110000111100001111000011110000");
}

#undef TEST_BIT
#define TEST_BIT(strvalue)                                                    \
  EXPECT_EQ(MAKE_BITS(strvalue).to_ullong(), string_to_bits(strvalue).first); \
  EXPECT_EQ(sizeof(strvalue) - 1, string_to_bits(strvalue).second)

TEST(utils_string, string_to_bits) {
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

TEST(utils_string, split) {
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

TEST(utils_string, ljust) {
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

TEST(utils_string, rjust) {
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

TEST(utils_string, str_break_into_lines) {
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

TEST(utils_string, quote_string) {
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

TEST(utils_string, unquote_string) {
  for (const auto quote : {'\'', '"'}) {
    for (const auto s :
         {"", "\"\"", "''", "\"string\"", "'string'", "some \\ string",
          "\"some \\ string\"", "'some \\ string'", "some \"\\ string\"",
          "some '\\ string'", "some \\\" string\"", "some \\' string'"}) {
      EXPECT_EQ(s, unquote_string(quote_string(s, quote), quote));
    }
  }
}

TEST(utils_string, str_span) {
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

TEST(utils_string, str_subvars) {
  EXPECT_EQ("", str_subvars(
                    "", [](std::string_view) { return ""; }, "$", ""));
  EXPECT_EQ("", str_subvars(
                    "$", [](std::string_view) { return ""; }, "$", ""));
  EXPECT_EQ(
      "boo",
      str_subvars(
          "$boo", [](std::string_view v) { return std::string{v}; }, "$", ""));
  EXPECT_EQ("booble",
            str_subvars(
                "$boo$ble", [](std::string_view v) { return std::string{v}; },
                "$", ""));
  EXPECT_EQ("boo.ble",
            str_subvars(
                "$boo.$ble", [](std::string_view v) { return std::string{v}; },
                "$", ""));
  EXPECT_EQ(".boo.ble.",
            str_subvars(
                ".$boo.$ble.",
                [](std::string_view v) { return std::string{v}; }, "$", ""));
  EXPECT_EQ("boo.ble",
            str_subvars(
                "boo.{ble}", [](std::string_view v) { return std::string{v}; },
                "{", "}"));
  EXPECT_EQ("blaboo",
            str_subvars(
                "{bla}boo", [](std::string_view v) { return std::string{v}; },
                "{", "}"));
  EXPECT_EQ("boo", str_subvars(
                       "boo", [](std::string_view v) { return std::string{v}; },
                       "{", "}"));
  EXPECT_EQ("blable",
            str_subvars(
                "${bla}${ble}",
                [](std::string_view v) { return std::string{v}; }, "${", "}"));
  EXPECT_EQ("blable",
            str_subvars({"\0bla\0\0ble\0", 10},
                        [](std::string_view v) { return std::string{v}; },
                        {"\0", 1}, {"\0", 1}));

  {
    shcore::Scoped_naming_style lower(shcore::LowerCamelCase);
    EXPECT_EQ("mySampleIdentifier", str_subvars("<<<mySampleIdentifier>>>"));
  }
  {
    shcore::Scoped_naming_style lower(shcore::LowerCaseUnderscores);
    EXPECT_EQ("my_sample_identifier", str_subvars("<<<mySampleIdentifier>>>"));
  }
}

#ifdef _WIN32
TEST(utils_string, wide_to_utf8) {
  {
    const std::wstring x{L"abcdef ghijk"};
    const std::string x_utf8_expected{"abcdef ghijk"};
    const auto x_utf8 = wide_to_utf8(x);
    EXPECT_EQ(x_utf8_expected, x_utf8);
    EXPECT_EQ(x, utf8_to_wide(x_utf8));
  }
  {
    const std::wstring x{
        L"za"
        L"\x017c"
        L"\x00f3"
        L"\x0142"
        L"\x0107"
        L" g\x0119\x015b"
        L"l\x0105 ja\x017a\x0144"};
    const std::string x_utf8_expected{"zażółć gęślą jaźń"};
    const std::string x_utf8_as_bytes{
        "\x7a\x61\xc5\xbc\xc3\xb3\xc5\x82\xc4\x87\x20\x67\xc4\x99\xc5\x9b\x6c"
        "\xc4\x85\x20\x6a\x61\xc5\xba\xc5\x84"};
    EXPECT_EQ(x_utf8_expected, x_utf8_as_bytes);
    const auto x_utf8 = wide_to_utf8(x);
    EXPECT_EQ(x_utf8_expected, x_utf8);
    EXPECT_EQ(x, utf8_to_wide(x_utf8));
  }
  {
    const std::wstring x{L"\x4eca\x65e5\x306f"};
    const std::string x_utf8_expected{"今日は"};
    const std::string x_utf8_as_bytes{"\xe4\xbb\x8a\xe6\x97\xa5\xe3\x81\xaf"};
    EXPECT_EQ(x_utf8_expected, x_utf8_as_bytes);
    const auto x_utf8 = wide_to_utf8(x);
    EXPECT_EQ(x_utf8_expected, x_utf8);
    EXPECT_EQ(x, utf8_to_wide(x_utf8));
  }
}

TEST(utils_string, utf8_to_wide) {
  {
    const std::string x{"abcdef ghijk"};
    const std::wstring x_wide_expected{L"abcdef ghijk"};
    const auto x_wide = utf8_to_wide(x);
    EXPECT_EQ(x_wide_expected, x_wide);
    EXPECT_EQ(x, wide_to_utf8(x_wide));
  }
  {
    const std::string x{"zażółć gęślą jaźń"};
    const std::wstring x_wide_expected{
        L"za"
        L"\x017c"
        L"\x00f3"
        L"\x0142"
        L"\x0107"
        L" g\x0119\x015b"
        L"l\x0105 ja\x017a\x0144"};
    const auto x_wide = utf8_to_wide(x);
    EXPECT_EQ(x_wide_expected, x_wide);
    EXPECT_EQ(x, wide_to_utf8(x_wide));
  }
  {
    const std::string x{"今日は"};  // \u4eca\u65e5\u306f
    const std::string x_utf8_as_bytes{"\xe4\xbb\x8a\xe6\x97\xa5\xe3\x81\xaf"};
    EXPECT_EQ(x, x_utf8_as_bytes);
    const std::wstring x_wide_expected{L"\x4eca\x65e5\x306f"};
    const auto x_wide = utf8_to_wide(x);
    EXPECT_EQ(x_wide_expected, x_wide);
    EXPECT_EQ(x, wide_to_utf8(x_wide));
  }
}
#endif

namespace {

constexpr char operator"" _c(unsigned long long n) {
  return static_cast<char>(n);
}

}  // namespace

TEST(utils_string, is_valid_utf8) {
  EXPECT_TRUE(is_valid_utf8(""));
  EXPECT_TRUE(is_valid_utf8(std::string("\x00", 1)));
  EXPECT_TRUE(is_valid_utf8("\x01"));
  EXPECT_TRUE(is_valid_utf8("$"));
  EXPECT_TRUE(is_valid_utf8("¢"));
  EXPECT_TRUE(is_valid_utf8("€"));
  EXPECT_TRUE(is_valid_utf8("𐍈"));
  EXPECT_TRUE(is_valid_utf8("$¢€𐍈"));
  EXPECT_TRUE(is_valid_utf8("𐍈€¢$"));

  // invalid character
  EXPECT_FALSE(is_valid_utf8("\xFF"));
  // invalid character after a null byte character
  EXPECT_FALSE(is_valid_utf8(std::string("\x00\xFF", 2)));

  // invalid multi-byte sequences
  EXPECT_FALSE(is_valid_utf8(std::string{0b11000000_c, 0b11000000_c}));
  EXPECT_FALSE(
      is_valid_utf8(std::string{0b11100000_c, 0b10000000_c, 0b11000000_c}));
  EXPECT_FALSE(
      is_valid_utf8(std::string{0b11100000_c, 0b11000000_c, 0b11000000_c}));
  EXPECT_FALSE(is_valid_utf8(
      std::string{0b11110000_c, 0b10000000_c, 0b10000000_c, 0b11000000_c}));
  EXPECT_FALSE(is_valid_utf8(
      std::string{0b11110000_c, 0b10000000_c, 0b11000000_c, 0b11000000_c}));
  EXPECT_FALSE(is_valid_utf8(
      std::string{0b11110000_c, 0b11000000_c, 0b11000000_c, 0b11000000_c}));

  // too short sequences
  // ¢ - missing one byte
  EXPECT_FALSE(is_valid_utf8("\xC2"));
  // € - missing one byte
  EXPECT_FALSE(is_valid_utf8("\xE2\x82"));
  // € - missing two bytes
  EXPECT_FALSE(is_valid_utf8("\xE2"));
  // 𐍈 - missing one byte
  EXPECT_FALSE(is_valid_utf8("\xF0\x90\x8D"));
  // 𐍈 - missing two bytes
  EXPECT_FALSE(is_valid_utf8("\xF0\x90"));
  // 𐍈 - missing three bytes
  EXPECT_FALSE(is_valid_utf8("\xF0"));

  // overlong encoding
  // $ - 2 bytes
  EXPECT_FALSE(is_valid_utf8(std::string{0b11000000_c, 0b10100100_c}));
  // $ - 3 bytes
  EXPECT_FALSE(
      is_valid_utf8(std::string{0b11100000_c, 0b10000000_c, 0b10100100_c}));
  // $ - 4 bytes
  EXPECT_FALSE(is_valid_utf8(
      std::string{0b11110000_c, 0b10000000_c, 0b10000000_c, 0b10100100_c}));
  // ¢ - 3 bytes
  EXPECT_FALSE(
      is_valid_utf8(std::string{0b11100000_c, 0b10000010_c, 0b10100010_c}));
  // ¢ - 4 bytes
  EXPECT_FALSE(is_valid_utf8(
      std::string{0b11110000_c, 0b10000000_c, 0b10000010_c, 0b10100010_c}));
  // € - 4 bytes
  EXPECT_FALSE(is_valid_utf8(
      std::string{0b11110000_c, 0b10000010_c, 0b10000010_c, 0b10101100_c}));
  // U+7F
  EXPECT_TRUE(is_valid_utf8(std::string{0b01111111_c}));
  // U+7F - 2 bytes
  EXPECT_FALSE(is_valid_utf8(std::string{0b11000001_c, 0b10111111_c}));
  // U+7F - 3 bytes
  EXPECT_FALSE(
      is_valid_utf8(std::string{0b11100000_c, 0b10000001_c, 0b10111111_c}));
  // U+7F - 4 bytes
  EXPECT_FALSE(is_valid_utf8(
      std::string{0b11110000_c, 0b10000000_c, 0b10000001_c, 0b10111111_c}));
  // U+80
  EXPECT_TRUE(is_valid_utf8(std::string{0b11000010_c, 0b10000000_c}));
  // U+80 - 3 bytes
  EXPECT_FALSE(
      is_valid_utf8(std::string{0b11100000_c, 0b10000010_c, 0b10000000_c}));
  // U+80 - 4 bytes
  EXPECT_FALSE(is_valid_utf8(
      std::string{0b11110000_c, 0b10000000_c, 0b10000010_c, 0b10000000_c}));
  // U+07FF
  EXPECT_TRUE(is_valid_utf8(std::string{0b11011111_c, 0b10111111_c}));
  // U+07FF - 3 bytes
  EXPECT_FALSE(
      is_valid_utf8(std::string{0b11100000_c, 0b10011111_c, 0b10111111_c}));
  // U+07FF - 4 bytes
  EXPECT_FALSE(is_valid_utf8(
      std::string{0b11110000_c, 0b10000000_c, 0b10011111_c, 0b10111111_c}));
  // U+0800
  EXPECT_TRUE(
      is_valid_utf8(std::string{0b11100000_c, 0b10100000_c, 0b10000000_c}));
  // U+0800 - 4 bytes
  EXPECT_FALSE(is_valid_utf8(
      std::string{0b11110000_c, 0b10000000_c, 0b10100000_c, 0b10000000_c}));
  // U+FFFF
  EXPECT_TRUE(
      is_valid_utf8(std::string{0b11101111_c, 0b10111111_c, 0b10111111_c}));
  // U+FFFF - 4 bytes
  EXPECT_FALSE(is_valid_utf8(
      std::string{0b11110000_c, 0b10001111_c, 0b10111111_c, 0b10111111_c}));
  // U+10000
  EXPECT_TRUE(is_valid_utf8(
      std::string{0b11110000_c, 0b10010000_c, 0b10000000_c, 0b10000000_c}));

  // UTF-16 surrogate halves
  EXPECT_TRUE(is_valid_utf8("\xED\x9F\xBF"));   // U+D7FF
  EXPECT_FALSE(is_valid_utf8("\xED\xA0\x80"));  // U+D800
  EXPECT_FALSE(is_valid_utf8("\xED\xAB\x9A"));  // U+DADA
  EXPECT_FALSE(is_valid_utf8("\xED\xBF\xBF"));  // U+DFFF
  EXPECT_TRUE(is_valid_utf8("\xEE\x80\x80"));   // U+E000

  // valid range
  // U+10FFFF
  EXPECT_TRUE(is_valid_utf8(
      std::string{0b11110100_c, 0b10001111_c, 0b10111111_c, 0b10111111_c}));
  // U+110000
  EXPECT_FALSE(is_valid_utf8(
      std::string{0b11110100_c, 0b10010000_c, 0b10000000_c, 0b10000000_c}));
}

TEST(utils_string, truncate) {
  EXPECT_EQ("", truncate("", 0));
  EXPECT_EQ("", truncate("zß水𝄋", 0));
  EXPECT_EQ("z", truncate("zß水𝄋", 1));
  EXPECT_EQ("zß", truncate("zß水𝄋", 2));
  EXPECT_EQ("zß水", truncate("zß水𝄋", 3));
  EXPECT_EQ("zß水𝄋", truncate("zß水𝄋", 4));
  EXPECT_EQ("zß水𝄋", truncate("zß水𝄋", 5));
  EXPECT_EQ("zß水𝄋", truncate("zß水𝄋zß水𝄋", 4));
  EXPECT_EQ("zß水𝄋z", truncate("zß水𝄋zß水𝄋", 5));
  EXPECT_EQ("zß水𝄋zß", truncate("zß水𝄋zß水𝄋", 6));
  EXPECT_EQ("zß水𝄋zß水", truncate("zß水𝄋zß水𝄋", 7));
  EXPECT_EQ("zß水𝄋zß水𝄋", truncate("zß水𝄋zß水𝄋", 8));
}

TEST(utils_string, pctencode) {
  EXPECT_EQ("", pctencode(""));
  EXPECT_EQ("%20", pctencode(" "));
  EXPECT_EQ("f", pctencode("f"));
  EXPECT_EQ("%2F", pctencode("/"));
  EXPECT_EQ("%2F%2F", pctencode("//"));
  EXPECT_EQ("%2F%20%2F", pctencode("/ /"));
  EXPECT_EQ("%2Fa%2F", pctencode("/a/"));
  EXPECT_EQ("%2Ftmp%2Ftmp.hS8tDOeTG0%2F3316%2Fdata%2Fmysqld.sock",
            pctencode("/tmp/tmp.hS8tDOeTG0/3316/data/mysqld.sock"));
  EXPECT_EQ("%C5%BC%C3%B3%C5%82w", pctencode("żółw"));
}

TEST(utils_string, pctdecode) {
  EXPECT_EQ("", pctdecode(""));
  EXPECT_EQ(" ", pctdecode("%20"));
  EXPECT_EQ("f", pctdecode("f"));
  EXPECT_EQ("/", pctdecode("%2F"));
  EXPECT_EQ("//", pctdecode("%2F%2F"));
  EXPECT_EQ("/ /", pctdecode("%2F%20%2F"));
  EXPECT_EQ("/a/", pctdecode("%2Fa%2F"));
  EXPECT_EQ("/tmp/tmp.hS8tDOeTG0/3316/data/mysqld.sock",
            pctdecode("%2Ftmp%2Ftmp.hS8tDOeTG0%2F3316%2Fdata%2Fmysqld.sock"));
  EXPECT_EQ("żółw", pctdecode("%C5%BC%C3%B3%C5%82w"));
}

}  // namespace shcore
