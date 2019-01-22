/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "unittest/gtest_clean.h"

#include "mysqlshdk/libs/rest/headers.h"

namespace mysqlshdk {
namespace rest {
namespace test {

TEST(Rest, header_comparator) {
  Header_comparator comp;

  EXPECT_FALSE(comp("a", "a"));
  EXPECT_FALSE(comp("a", "A"));
  EXPECT_FALSE(comp("A", "a"));
  EXPECT_FALSE(comp("A", "A"));

  EXPECT_TRUE(comp("a", "b"));
  EXPECT_FALSE(comp("b", "a"));
  EXPECT_TRUE(comp("a", "B"));
  EXPECT_FALSE(comp("B", "a"));
  EXPECT_TRUE(comp("A", "b"));
  EXPECT_FALSE(comp("b", "A"));
  EXPECT_TRUE(comp("A", "B"));
  EXPECT_FALSE(comp("B", "A"));

  const auto equiv = [&comp](const std::string &l, const std::string &r) {
    return !comp(l, r) && !comp(r, l);
  };

  EXPECT_TRUE(equiv("a", "a"));
  EXPECT_TRUE(equiv("a", "A"));
  EXPECT_TRUE(equiv("A", "a"));
  EXPECT_TRUE(equiv("A", "A"));

  EXPECT_FALSE(equiv("a", "b"));
  EXPECT_FALSE(equiv("a", "B"));
  EXPECT_FALSE(equiv("A", "b"));
  EXPECT_FALSE(equiv("A", "B"));
}

TEST(Rest, headers) {
  Headers h{{"Accept", "application/json"}};

  EXPECT_EQ("application/json", h["accept"]);
  EXPECT_EQ("application/json", h["ACCEPT"]);
}

}  // namespace test
}  // namespace rest
}  // namespace mysqlshdk
