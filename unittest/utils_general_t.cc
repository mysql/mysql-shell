/*
* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <stack>

#include "gtest/gtest.h"
#include "utils/utils_general.h"

namespace shcore {
TEST(utils_general, split_account) {
  std::string a, b;

  // test no crash
  split_account("a@b", nullptr, nullptr);

  // test no crash
  split_account("a@b", &a, nullptr);

  struct Case {
    const char *account;
    const char *user;
    const char *host;
  };
  static std::vector<Case> good_cases{
    {"foo@bar", "foo", "bar"},
    {"'foo'@bar", "foo", "bar"},
    {"'foo'@'bar'", "foo", "bar"},
    {"\"foo\"@\"bar\"", "foo", "bar"},
    {"'fo\"o'@bar", "fo\"o", "bar"},
    {"'fo\\'o'@bar", "fo'o", "bar"},
    {"'fo''o'@bar", "fo'o", "bar"},
    {"\"foo\"\"bar\"@'bar'", "foo\"bar", "bar"},
    {"\"foo\\\"bar\"@'bar'", "foo\"bar", "bar"},
    {"'fo%o'@'bar%'", "fo%o", "bar%"},
    {"_foo@'bar'", "_foo", "bar"},
    {"$foo@'bar'", "$foo", "bar"},
    {"foo_@'bar'", "foo_", "bar"},
    {"foo123@'bar'", "foo123", "bar"},
    {"'foo@123'@'bar'", "foo@123", "bar"},
    {"'foo@123'", "foo@123", ""},
    {"'foo'@", "foo", ""},
    {"'fo\\no'@", "fo\no", ""},
    {"'foo'", "foo", ""},
    {"'foo'''", "foo'", ""},
    {"''''''''", "'''", ""},
    {"foo@", "foo", ""}
  };
  for (auto &t : good_cases) {
    a.clear();
    b.clear();
    SCOPED_TRACE(t.account);
    EXPECT_NO_THROW(split_account(t.account, &a, &b));
    EXPECT_EQ(t.user, a);
    EXPECT_EQ(t.host, b);
  }
  static std::vector<std::string> bad_cases{
    "'foo",
    "'foo'bar",
    "'foo'@@bar",
    "'foo@bar",
    "''foo",
    "123@bar",
    "\"foo'@bar",
    "@@@",
    "'foo'x",
    "'foo'bar",
    "'foo'@'bar'z",
    "foo@\"bar\".",
    "\"foo\"-",
    "''foo''@",
    "@",
    "''foo''@"
  };
  for (auto &t : bad_cases) {
    SCOPED_TRACE(t);
    EXPECT_THROW(split_account(t, nullptr, nullptr), std::runtime_error);
  }
}

TEST(utils_general, make_account) {
  struct Case {
    const char *account;
    const char *user;
    const char *host;
  };
  static std::vector<Case> good_cases{
    {"'foo'@'bar'", "foo", "bar"},
    {"'\\'foo\\''@'bar'", "'foo'", "bar"},
    {"'\\\"foo\\\"'@'bar%'", "\"foo\"", "bar%"},
    {"'a@b'@'bar'", "a@b", "bar"},
    {"'a@b'@'bar'", "a@b", "bar"},
    {"''@''", "", ""}
  };
  for (auto &t : good_cases) {
    SCOPED_TRACE(t.account);
    EXPECT_EQ(t.account, make_account(t.user, t.host));
  }
}

TEST(utils_general, split_string_chars) {
  std::vector<std::string> strs = split_string_chars("aaaa-vvvvv>bbbbb", "->", true);
  EXPECT_EQ("aaaa", strs[0]);
  EXPECT_EQ("vvvvv", strs[1]);
  EXPECT_EQ("bbbbb", strs[2]);

  std::vector<std::string> strs1 = split_string_chars("aaaa->bbbbb", "->", true);
  EXPECT_EQ("aaaa", strs1[0]);
  EXPECT_EQ("bbbbb", strs1[1]);

  std::vector<std::string> strs2 = split_string_chars("aaaa->bbbbb", "->", false);
  EXPECT_EQ("aaaa", strs2[0]);
  EXPECT_EQ("", strs2[1]);
  EXPECT_EQ("bbbbb", strs2[2]);

  std::vector<std::string> strs3 = split_string_chars(",aaaa-bbb-bb," , "-,", true);
  EXPECT_EQ("", strs3[0]);
  EXPECT_EQ("aaaa", strs3[1]);
  EXPECT_EQ("bbb", strs3[2]);
  EXPECT_EQ("bb", strs3[3]);
  EXPECT_EQ("", strs3[4]);

  std::vector<std::string> strs4 = split_string_chars("aa;a\\a-bbb-bb," , "\\-;", true);
  EXPECT_EQ("aa", strs4[0]);
  EXPECT_EQ("a", strs4[1]);
  EXPECT_EQ("a", strs4[2]);
  EXPECT_EQ("bbb", strs4[3]);
  EXPECT_EQ("bb,", strs4[4]);

  std::vector<std::string> strs5 = split_string_chars("!.", "!.", false);
  EXPECT_EQ("", strs5[0]);
  EXPECT_EQ("", strs5[1]);

  std::vector<std::string> strs6 = split_string_chars("aa.a.aaa.a", ".", true);
  EXPECT_EQ("aa", strs6[0]);
  EXPECT_EQ("a", strs6[1]);
  EXPECT_EQ("aaa", strs6[2]);
  EXPECT_EQ("a", strs6[3]);

  std::vector<std::string> strs7 = split_string_chars("", ".", false);
  EXPECT_EQ("", strs7[0]);

  std::vector<std::string> strs8 = split_string_chars(".", ".", false);
  EXPECT_EQ("", strs8[0]);
  EXPECT_EQ("", strs8[1]);

  std::vector<std::string> strs9 = split_string_chars("....", ".", false);
  EXPECT_EQ("", strs9[0]);
  EXPECT_EQ("", strs9[1]);
  EXPECT_EQ("", strs9[2]);
  EXPECT_EQ("", strs9[3]);
}
}
