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

#include "mysqlshdk/libs/utils/enumset.h"
#include "unittest/gtest_clean.h"

namespace shcore {
namespace utils {

TEST(Enumset, test) {
  enum Fruit { Apple, Banana, Cantaloupe };
  typedef Enum_set<Fruit, Cantaloupe> Fruit_set;
  Fruit_set fruitset;
  Fruit_set fruitset2;
  Fruit_set empty;

  EXPECT_TRUE(fruitset.empty());
  EXPECT_FALSE(fruitset.is_set(Apple));
  EXPECT_FALSE(fruitset.is_set(Banana));
  EXPECT_FALSE(fruitset.matches_any(Fruit_set::any()));
  EXPECT_TRUE(fruitset == empty);

  fruitset.set(Apple);
  EXPECT_TRUE(fruitset.matches_any(Fruit_set::any()));
  EXPECT_TRUE(Fruit_set::any().matches_any(fruitset));

  fruitset2.set(Banana).set(Cantaloupe);
  EXPECT_FALSE(fruitset == fruitset2);
  EXPECT_FALSE(fruitset2.is_set(Apple));
  EXPECT_TRUE(fruitset2.is_set(Banana));
  EXPECT_TRUE(fruitset2.is_set(Cantaloupe));
  fruitset2.unset(Cantaloupe);
  EXPECT_FALSE(fruitset2.is_set(Cantaloupe));
}
}  // namespace utils
}  // namespace shcore
