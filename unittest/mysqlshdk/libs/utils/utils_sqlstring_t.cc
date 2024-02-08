/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

#include <set>
#include <string>

#include "unittest/gtest_clean.h"

#include "mysqlshdk/libs/utils/utils_sqlstring.h"

namespace shcore {

TEST(utils_sqlstring, escape_wildcards) {
  EXPECT_EQ("", escape_wildcards(""));
  EXPECT_EQ("alphabet", escape_wildcards("alphabet"));
  EXPECT_EQ(R"(\_)", escape_wildcards("_"));
  EXPECT_EQ(R"(\%)", escape_wildcards("%"));
  EXPECT_EQ(R"(\%\_)", escape_wildcards("%_"));
  EXPECT_EQ(R"(\_\%)", escape_wildcards("_%"));
  EXPECT_EQ(R"(\%\%\%\%)", escape_wildcards("%%%%"));
  EXPECT_EQ(R"(\_\_\_\_)", escape_wildcards("____"));
  EXPECT_EQ(R"(\_\%\_\%)", escape_wildcards("_%_%"));
  EXPECT_EQ(R"(\%\_\%\_)", escape_wildcards("%_%_"));
  EXPECT_EQ(R"(\_al\%phab\_et\%)", escape_wildcards("_al%phab_et%"));
  EXPECT_EQ(R"(\%al\%phab\_et\_)", escape_wildcards("%al%phab_et_"));
}

TEST(utils_sqlstring, double_values) {
  EXPECT_EQ("3.14567890123456", (sqlstring("?", 0) << 3.14567890123456).str());
}

TEST(utils_sqlstring, boolean) {
  EXPECT_EQ("1", (sqlstring("?", 0) << true).str());
  EXPECT_EQ("0", (sqlstring("?", 0) << false).str());
}

TEST(utils_sqlstring, user_defined_literal) {
  EXPECT_EQ(("sql ?"_sql << 10).str(), (sqlstring("sql ?", 0) << 10).str());
}

TEST(utils_sqlstring, has_sql_wildcard) {
  EXPECT_EQ(false, has_sql_wildcard(""));

  EXPECT_EQ(true, has_sql_wildcard("%"));
  EXPECT_EQ(true, has_sql_wildcard("_"));

  EXPECT_EQ(true, has_sql_wildcard("\\%"));
  EXPECT_EQ(true, has_sql_wildcard("\\_"));

  EXPECT_EQ(true, has_sql_wildcard("\\\\%"));
  EXPECT_EQ(true, has_sql_wildcard("\\\\_"));

  EXPECT_EQ(true, has_sql_wildcard("a%"));
  EXPECT_EQ(true, has_sql_wildcard("a_"));
  EXPECT_EQ(true, has_sql_wildcard("%a"));
  EXPECT_EQ(true, has_sql_wildcard("_a"));
  EXPECT_EQ(true, has_sql_wildcard("a%b"));
  EXPECT_EQ(true, has_sql_wildcard("a_b"));

  EXPECT_EQ(false, has_sql_wildcard("a"));
  EXPECT_EQ(false, has_sql_wildcard("ab"));
  EXPECT_EQ(false, has_sql_wildcard("abc"));
}

TEST(utils_sqlstring, has_unescaped_sql_wildcard) {
  EXPECT_EQ(false, has_unescaped_sql_wildcard(""));

  EXPECT_EQ(true, has_unescaped_sql_wildcard("%"));
  EXPECT_EQ(true, has_unescaped_sql_wildcard("_"));

  EXPECT_EQ(false, has_unescaped_sql_wildcard("\\%"));
  EXPECT_EQ(false, has_unescaped_sql_wildcard("\\_"));

  EXPECT_EQ(true, has_unescaped_sql_wildcard("\\\\%"));
  EXPECT_EQ(true, has_unescaped_sql_wildcard("\\\\_"));

  EXPECT_EQ(true, has_unescaped_sql_wildcard("a%"));
  EXPECT_EQ(true, has_unescaped_sql_wildcard("a_"));
  EXPECT_EQ(true, has_unescaped_sql_wildcard("%a"));
  EXPECT_EQ(true, has_unescaped_sql_wildcard("_a"));
  EXPECT_EQ(true, has_unescaped_sql_wildcard("a%b"));
  EXPECT_EQ(true, has_unescaped_sql_wildcard("a_b"));

  EXPECT_EQ(false, has_unescaped_sql_wildcard("a"));
  EXPECT_EQ(false, has_unescaped_sql_wildcard("ab"));
  EXPECT_EQ(false, has_unescaped_sql_wildcard("abc"));
}

TEST(utils_sqlstring, match_sql_wild) {
  EXPECT_EQ(true, match_sql_wild("", ""));

  EXPECT_EQ(true, match_sql_wild("", "%"));
  EXPECT_EQ(false, match_sql_wild("", "_"));

  EXPECT_EQ(true, match_sql_wild("_", "_"));
  EXPECT_EQ(true, match_sql_wild("-", "_"));

  EXPECT_EQ(true, match_sql_wild("_", "\\_"));
  EXPECT_EQ(false, match_sql_wild("-", "\\_"));

  EXPECT_EQ(true, match_sql_wild("%", "\\%"));
  EXPECT_EQ(false, match_sql_wild("-", "\\%"));

  EXPECT_EQ(true, match_sql_wild("%", "%"));
  EXPECT_EQ(true, match_sql_wild("-", "%"));

  EXPECT_EQ(true, match_sql_wild("xyzABCqwe", "%ABC%"));
  EXPECT_EQ(false, match_sql_wild("xyzABCqwe", "%abc%"));
}

TEST(utils_sqlstring, SQL_wild_compare) {
  std::multiset<std::string, SQL_wild_compare> schemas;
  schemas.emplace("");
  schemas.emplace("%");
  schemas.emplace("a_");
  schemas.emplace("ab%");
  schemas.emplace("abc");

  ASSERT_EQ(5, schemas.size());
  auto it = schemas.begin();
  EXPECT_EQ("abc", *it++);
  EXPECT_EQ("ab%", *it++);
  EXPECT_EQ("a_", *it++);
  EXPECT_EQ("%", *it++);
  EXPECT_EQ("", *it++);
}

}  // namespace shcore
