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

#include "mysqlshdk/libs/utils/enumset.h"

#include <set>

#include "unittest/gtest_clean.h"

namespace mysqlshdk {
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
  EXPECT_TRUE(fruitset.values().empty());

  fruitset.set(Apple);
  EXPECT_TRUE(fruitset.matches_any(Fruit_set::any()));
  EXPECT_TRUE(Fruit_set::any().matches_any(fruitset));
  EXPECT_EQ((std::set<Fruit>{Apple}), fruitset.values());

  fruitset2.set(Banana).set(Cantaloupe);
  EXPECT_FALSE(fruitset == fruitset2);
  EXPECT_FALSE(fruitset2.is_set(Apple));
  EXPECT_TRUE(fruitset2.is_set(Banana));
  EXPECT_TRUE(fruitset2.is_set(Cantaloupe));
  EXPECT_EQ((std::set<Fruit>{Banana, Cantaloupe}), fruitset2.values());

  fruitset2.unset(Cantaloupe);
  EXPECT_FALSE(fruitset2.is_set(Cantaloupe));
  EXPECT_EQ((std::set<Fruit>{Banana}), fruitset2.values());

  // variadic ctor
  Fruit_set fruitset3;

  fruitset3 = Fruit_set{Fruit::Apple};
  EXPECT_EQ(fruitset3.values().size(), 1);
  EXPECT_TRUE(fruitset3.is_set(Fruit::Apple));

  fruitset3 = Fruit_set{Fruit::Apple, Fruit::Banana};
  EXPECT_EQ(fruitset3.values().size(), 2);
  EXPECT_TRUE(fruitset3.is_set(Fruit::Apple) &&
              fruitset3.is_set(Fruit::Banana));

  fruitset3 = Fruit_set{Fruit::Cantaloupe, Fruit::Banana};
  EXPECT_EQ(fruitset3.values().size(), 2);
  EXPECT_TRUE(fruitset3.is_set(Fruit::Cantaloupe) &&
              fruitset3.is_set(Fruit::Banana));

  fruitset3 = Fruit_set{Fruit::Cantaloupe, Fruit::Banana, Fruit::Apple};
  EXPECT_EQ(fruitset3.values().size(), 3);
  EXPECT_TRUE(Fruit_set::all() == fruitset3);
}

TEST(Enumset, operators) {
  enum class Attrib { Color, Width, Height };
  using Attribs = Enum_set<Attrib, Attrib::Height>;

  Attribs a;
  EXPECT_FALSE(a);

  a = Attrib::Color;
  EXPECT_TRUE(a.is_set(Attrib::Color));
  EXPECT_FALSE(a.is_set(Attrib::Width));
  EXPECT_FALSE(a.is_set(Attrib::Height));

  EXPECT_TRUE(a & Attrib::Color);
  EXPECT_FALSE(a & Attrib::Height);
  EXPECT_TRUE(a == Attrib::Color);
  EXPECT_FALSE(a == Attrib::Height);
  EXPECT_TRUE((a | Attrib::Width) != (a | Attrib::Height));
  EXPECT_TRUE((a | Attrib::Width | Attrib::Height) & Attrib::Height);
  EXPECT_TRUE((a | Attrib::Width | Attrib::Height) & Attrib::Color);
}

}  // namespace utils
}  // namespace mysqlshdk
