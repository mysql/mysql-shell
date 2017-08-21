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
#include <stack>
#include <string>
#include <tuple>

#include "gtest_clean.h"
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
  EXPECT_EQ("-1152921504606846976 9223372036854775808",
            shcore::str_format("%lli %llu", -1152921504606846976LL,
                               9223372036854775808ULL));

  EXPECT_EQ(
      "string with over 256charsstring with over 256charsstring with over "
      "256charsstring with over 256charsstring with over 256charsstring with "
      "over 256charsstring with over 256charsstring with over 256charsstring "
      "with over 256charsstring with over 256charsstring with over "
      "256charsstring with over 256charsstring with over 256charsstring with "
      "over 256charsstring with over 256charsstring with over 256chars!",
      shcore::str_format(
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
      shcore::str_format(
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
      shcore::str_format(
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
      shcore::str_format(
          "%s%s%s%s",
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
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

TEST(UtilsString, join) {
  EXPECT_EQ("", shcore::str_join(std::vector<std::string>{}, ""));
  EXPECT_EQ("", shcore::str_join(std::vector<std::string>{}, ","));
  EXPECT_EQ("a", shcore::str_join(std::vector<std::string>{"a"}, ","));
  EXPECT_EQ("a,b", shcore::str_join(std::vector<std::string>{"a", "b"}, ","));
  EXPECT_EQ(",b", shcore::str_join(std::vector<std::string>{"", "b"}, ","));
  EXPECT_EQ("a,", shcore::str_join(std::vector<std::string>{"a", ""}, ","));
  EXPECT_EQ(",", shcore::str_join(std::vector<std::string>{"", ""}, ","));
  EXPECT_EQ("a,b", shcore::str_join(std::set<std::string>({"a", "b"}), ","));

  const char *args[] = {"a", "b", nullptr};
  EXPECT_EQ("a,b", shcore::str_join(args, args + 2, ","));
  EXPECT_EQ("a", shcore::str_join(args, args + 1, ","));

  std::vector<std::string> v({"a", "b"});
  EXPECT_EQ("a,b", shcore::str_join(v.begin(), v.end(), ","));

  std::set<std::string> s({"a", "b"});
  EXPECT_EQ("a,b", shcore::str_join(s.begin(), s.end(), ","));
}

TEST(UtilsString, replace) {
  EXPECT_EQ("", shcore::str_replace("", "", ""));

  EXPECT_EQ("", shcore::str_replace("", "fo", ""));
  EXPECT_EQ("", shcore::str_replace("", "fo", "bar"));
  EXPECT_EQ("bar", shcore::str_replace("", "", "bar"));

  EXPECT_EQ("foo", shcore::str_replace("foo", "", ""));
  EXPECT_EQ("o", shcore::str_replace("foo", "fo", ""));
  EXPECT_EQ("baro", shcore::str_replace("foo", "fo", "bar"));
  EXPECT_EQ("barfbarobarobar", shcore::str_replace("foo", "", "bar"));

  EXPECT_EQ("fofoo", shcore::str_replace("fofoo", "", ""));
  EXPECT_EQ("o", shcore::str_replace("fofoo", "fo", ""));
  EXPECT_EQ("barbaro", shcore::str_replace("fofoo", "fo", "bar"));
  EXPECT_EQ("barfbarobarfbarobarobar", shcore::str_replace("fofoo", "", "bar"));

  EXPECT_EQ("barfoo", shcore::str_replace("barfoo", "", ""));
  EXPECT_EQ("baro", shcore::str_replace("barfoo", "fo", ""));
  EXPECT_EQ("barbaro", shcore::str_replace("barfoo", "fo", "bar"));
  EXPECT_EQ("barbbarabarrbarfbarobarobar",
            shcore::str_replace("barfoo", "", "bar"));

  EXPECT_EQ("barfo", shcore::str_replace("barfo", "", ""));
  EXPECT_EQ("bar", shcore::str_replace("barfo", "fo", ""));
  EXPECT_EQ("barbar", shcore::str_replace("barfo", "fo", "bar"));
  EXPECT_EQ("barbbarabarrbarfbarobar", shcore::str_replace("barfo", "", "bar"));

  EXPECT_EQ("fofo", shcore::str_replace("fofo", "", ""));
  EXPECT_EQ("", shcore::str_replace("fofo", "fo", ""));
  EXPECT_EQ("barbar", shcore::str_replace("fofo", "fo", "bar"));
  EXPECT_EQ("barfbarobarfbarobar", shcore::str_replace("fofo", "", "bar"));

  EXPECT_EQ("bar", shcore::str_replace("bar", "", ""));
  EXPECT_EQ("bar", shcore::str_replace("bar", "fo", ""));
  EXPECT_EQ("bar", shcore::str_replace("bar", "fo", "bar"));
  EXPECT_EQ("barbbarabarrbar", shcore::str_replace("bar", "", "bar"));
}

#include <bitset>

#define MAKE_BITS(strvalue) std::bitset<sizeof(strvalue) - 1>(strvalue)

#define TEST_BIT(strvalue)                                          \
  EXPECT_EQ(MAKE_BITS(strvalue).to_string(),                        \
            shcore::bits_to_string(MAKE_BITS(strvalue).to_ullong(), \
                                   sizeof(strvalue) - 1));

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
#define TEST_BIT(strvalue)                           \
  EXPECT_EQ(MAKE_BITS(strvalue).to_ullong(),         \
            shcore::string_to_bits(strvalue).first); \
  EXPECT_EQ(sizeof(strvalue) - 1, shcore::string_to_bits(strvalue).second)

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
