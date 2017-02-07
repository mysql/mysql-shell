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
#include "../utils/utils_general.h"

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
}
