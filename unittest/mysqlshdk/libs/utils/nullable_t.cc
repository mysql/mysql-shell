/* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

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

#include "gtest_clean.h"
#include "mysqlshdk/libs/utils/nullable.h"

namespace testing {

TEST(nullable, default_initialization) {
  mysqlshdk::utils::nullable<int> sample;
  EXPECT_TRUE(sample.is_null());
}

TEST(nullable, base_type_initialization_numeric) {
  mysqlshdk::utils::nullable<int> sample(50);
  EXPECT_FALSE(sample.is_null());
  EXPECT_EQ(50, *sample); // Testing the * operator
  int value = sample;
  EXPECT_EQ(50, value); // Testing implicit conversion
}

TEST(nullable, other_initialization_numeric) {
  mysqlshdk::utils::nullable<int> sample(50);
  mysqlshdk::utils::nullable<int> sample2(sample);
  EXPECT_FALSE(sample2.is_null());
  EXPECT_EQ(50, *sample2); // Testing the * operator
  int value = sample2;
  EXPECT_EQ(50, value); // Testing implicit conversion
}

TEST(nullable, base_type_assignment_numeric) {
  mysqlshdk::utils::nullable<int> sample;

  sample = 50;

  EXPECT_FALSE(sample.is_null());
  EXPECT_EQ(50, *sample); // Testing the * operator
  int value = sample;
  EXPECT_EQ(50, value); // Testing implicit conversion
}

TEST(nullable, nullable_assignment_numeric) {
  mysqlshdk::utils::nullable<int> sample(50);
  mysqlshdk::utils::nullable<int> sample2;

  sample2 = sample;

  EXPECT_FALSE(sample2.is_null());
  EXPECT_EQ(50, *sample2); // Testing the * operator
  int value = sample2;
  EXPECT_EQ(50, value); // Testing implicit conversion
}

TEST(nullable, base_type_initialization_string) {
  mysqlshdk::utils::nullable<std::string> sample("some value");
  EXPECT_FALSE(sample.is_null());
  EXPECT_EQ("some value", *sample); // Testing the * operator
  std::string value = sample;
  EXPECT_EQ("some value", value); // Testing implicit conversion
}

TEST(nullable, other_initializtion_string) {
  mysqlshdk::utils::nullable<std::string> sample("some value");
  mysqlshdk::utils::nullable<std::string> sample2(sample);
  EXPECT_FALSE(sample2.is_null());
  EXPECT_EQ("some value", *sample2); // Testing the * operator
  std::string value = sample2;
  EXPECT_EQ("some value", value); // Testing implicit conversion
}

TEST(nullable, base_type_assignment_string) {
  mysqlshdk::utils::nullable<std::string> sample;

  sample = "some value";

  EXPECT_FALSE(sample.is_null());
  EXPECT_EQ("some value", *sample); // Testing the * operator
  std::string value = sample;
  EXPECT_EQ("some value", value); // Testing implicit conversion
}

TEST(nullable, nullable_assignment_string) {
  mysqlshdk::utils::nullable<std::string> sample("some value");
  mysqlshdk::utils::nullable<std::string> sample2;

  sample2 = sample;

  EXPECT_FALSE(sample2.is_null());
  EXPECT_EQ("some value", *sample2); // Testing the * operator
  std::string value = sample2;
  EXPECT_EQ("some value", value); // Testing implicit conversion
}

}
