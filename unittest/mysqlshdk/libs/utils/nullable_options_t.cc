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

#include "unittest/gtest_clean.h"
#include "mysqlshdk/libs/utils/nullable_options.h"

#ifndef MY_EXPECT_THROW
#define MY_EXPECT_THROW(e, m, c)         \
  EXPECT_THROW(                          \
      {                                  \
        try {                            \
          c;                             \
        } catch (const e& error) {       \
          EXPECT_STREQ(m, error.what()); \
          throw;                         \
        }                                \
      },                                 \
      e)
#endif

using mysqlshdk::utils::nullable_options::Set_mode;
using mysqlshdk::utils::nullable_options::Comparison_mode;
using mysqlshdk::utils::Nullable_options;
namespace testing {

// This tests ensures variables are handled in a
// case sensitive way
TEST(Nullable_options, case_sensitive) {
  Nullable_options options(Comparison_mode::CASE_SENSITIVE);

  // Test adding option
  EXPECT_NO_THROW(options.set("sample1", "value1"));
  EXPECT_EQ(1, options.size());
  EXPECT_NO_THROW(options.set("Sample1", "value2"));
  EXPECT_EQ(2, options.size());
  EXPECT_NO_THROW(options.set("SAMPLE1"));
  EXPECT_EQ(3, options.size());

  // Test has
  EXPECT_TRUE(options.has("sample1"));
  EXPECT_TRUE(options.has("Sample1"));
  EXPECT_TRUE(options.has("SAMPLE1"));

  // Test has_value
  EXPECT_TRUE(options.has_value("sample1"));
  EXPECT_TRUE(options.has_value("Sample1"));
  EXPECT_FALSE(options.has_value("SAMPLE1"));

  // Test get_value
  EXPECT_STREQ("value1", options.get_value("sample1").c_str());
  EXPECT_STREQ("value2", options.get_value("Sample1").c_str());
  MY_EXPECT_THROW(std::invalid_argument, "The option 'SAMPLE1' has no value.",
                  options.get_value("SAMPLE1"));

  // Test clear_value and ensures other are not affected
  EXPECT_NO_THROW(options.clear_value("sample1"));
  EXPECT_TRUE(options.has("sample1"));
  EXPECT_FALSE(options.has_value("sample1"));
  MY_EXPECT_THROW(std::invalid_argument, "The option 'sample1' has no value.",
                  options.get_value("sample1"));

  EXPECT_TRUE(options.has("Sample1"));
  EXPECT_TRUE(options.has_value("Sample1"));
  EXPECT_STREQ("value2", options.get_value("Sample1").c_str());

  EXPECT_TRUE(options.has("SAMPLE1"));
  EXPECT_FALSE(options.has_value("SAMPLE1"));
  MY_EXPECT_THROW(std::invalid_argument, "The option 'SAMPLE1' has no value.",
                  options.get_value("SAMPLE1"));

  // Test remove and ensures others are not affected
  EXPECT_NO_THROW(options.remove("Sample1"));
  EXPECT_EQ(2, options.size());
  EXPECT_TRUE(options.has("sample1"));
  EXPECT_FALSE(options.has_value("sample1"));
  MY_EXPECT_THROW(std::invalid_argument, "The option 'sample1' has no value.",
                  options.get_value("sample1"));

  EXPECT_FALSE(options.has("Sample1"));
  MY_EXPECT_THROW(std::invalid_argument, "Invalid option 'Sample1'.",
                  options.has_value("Sample1"));
  MY_EXPECT_THROW(std::invalid_argument, "Invalid option 'Sample1'.",
                  options.get_value("Sample1"));

  EXPECT_TRUE(options.has("SAMPLE1"));
  EXPECT_FALSE(options.has_value("SAMPLE1"));
  MY_EXPECT_THROW(std::invalid_argument, "The option 'SAMPLE1' has no value.",
                  options.get_value("SAMPLE1"));

  // Attempt to remove unexisting option
  MY_EXPECT_THROW(std::invalid_argument, "Invalid option 'Sample1'.",
                  options.remove("Sample1"));

  // Attempt to clear unexisting option
  MY_EXPECT_THROW(std::invalid_argument, "Invalid option 'Sample1'.",
                  options.clear_value("Sample1"));
}

TEST(Nullable_options, case_insensitive) {
  Nullable_options options;

  // Test adding option
  EXPECT_NO_THROW(options.set("sample1", "value1"));
  EXPECT_EQ(1, options.size());
  EXPECT_NO_THROW(options.set("Sample1", "value2"));
  EXPECT_EQ(1, options.size());

  // Test has
  EXPECT_TRUE(options.has("sample1"));
  EXPECT_TRUE(options.has("Sample1"));
  EXPECT_TRUE(options.has("SAMPLE1"));

  // Test has_value
  EXPECT_TRUE(options.has_value("sample1"));
  EXPECT_TRUE(options.has_value("Sample1"));
  EXPECT_TRUE(options.has_value("SAMPLE1"));

  // Test get_value
  EXPECT_STREQ("value2", options.get_value("sample1").c_str());
  EXPECT_STREQ("value2", options.get_value("Sample1").c_str());
  EXPECT_STREQ("value2", options.get_value("SAMPLE1").c_str());

  // Test clear_value and ensures others use the same data
  EXPECT_NO_THROW(options.clear_value("sample1"));
  EXPECT_TRUE(options.has("sample1"));
  EXPECT_FALSE(options.has_value("sample1"));
  MY_EXPECT_THROW(std::invalid_argument, "The option 'sample1' has no value.",
                  options.get_value("sample1"));

  EXPECT_TRUE(options.has("Sample1"));
  EXPECT_FALSE(options.has_value("Sample1"));
  MY_EXPECT_THROW(std::invalid_argument, "The option 'Sample1' has no value.",
                  options.get_value("Sample1"));

  EXPECT_TRUE(options.has("SAMPLE1"));
  EXPECT_FALSE(options.has_value("SAMPLE1"));
  MY_EXPECT_THROW(std::invalid_argument, "The option 'SAMPLE1' has no value.",
                  options.get_value("SAMPLE1"));

  // Test remove and ensures others are gone too
  EXPECT_NO_THROW(options.remove("Sample1"));
  EXPECT_EQ(0, options.size());
  EXPECT_FALSE(options.has("sample1"));
  MY_EXPECT_THROW(std::invalid_argument, "Invalid option 'sample1'.",
                  options.has_value("sample1"));
  MY_EXPECT_THROW(std::invalid_argument, "Invalid option 'sample1'.",
                  options.get_value("sample1"));

  EXPECT_FALSE(options.has("Sample1"));
  MY_EXPECT_THROW(std::invalid_argument, "Invalid option 'Sample1'.",
                  options.has_value("Sample1"));
  MY_EXPECT_THROW(std::invalid_argument, "Invalid option 'Sample1'.",
                  options.get_value("Sample1"));

  EXPECT_FALSE(options.has("SAMPLE1"));
  MY_EXPECT_THROW(std::invalid_argument, "Invalid option 'SAMPLE1'.",
                  options.has_value("SAMPLE1"));
  MY_EXPECT_THROW(std::invalid_argument, "Invalid option 'SAMPLE1'.",
                  options.get_value("SAMPLE1"));

  // Attempt to remove unexisting option
  MY_EXPECT_THROW(std::invalid_argument, "Invalid option 'Sample1'.",
                  options.remove("Sample1"));

  // Attempt to clear unexisting option
  MY_EXPECT_THROW(std::invalid_argument, "Invalid option 'Sample1'.",
                  options.clear_value("Sample1"));
}

TEST(Nullable_options, context) {
  {
    Nullable_options options(Comparison_mode::CASE_SENSITIVE, "Sample");

    options.set("sample1", nullptr);

    MY_EXPECT_THROW(std::invalid_argument,
                    "The Sample option 'sample1' has no value.",
                    options.get_value("sample1"));

    MY_EXPECT_THROW(std::invalid_argument, "Invalid Sample option 'sample2'.",
                    options.set("sample2", nullptr, Set_mode::UPDATE_ONLY));
  }

  {
    Nullable_options options(Comparison_mode::CASE_SENSITIVE, "Category ");

    options.set("sample1", nullptr);

    MY_EXPECT_THROW(std::invalid_argument,
                    "The Category option 'sample1' has no value.",
                    options.get_value("sample1"));

    MY_EXPECT_THROW(std::invalid_argument, "Invalid Category option 'sample2'.",
                    options.set("sample2", nullptr, Set_mode::UPDATE_ONLY));
  }
}

TEST(Nullable_options, operator_compare) {
  // Case sensitiveness
  {
    Nullable_options opt1(Comparison_mode::CASE_SENSITIVE);
    Nullable_options opt2(Comparison_mode::CASE_SENSITIVE);
    Nullable_options opt3;

    ASSERT_TRUE(opt1 == opt2);
    ASSERT_FALSE(opt1 != opt2);
    ASSERT_FALSE(opt2 == opt3);
    ASSERT_TRUE(opt2 != opt3);
  }

  // Differences in context
  {
    Nullable_options opt1(Comparison_mode::CASE_SENSITIVE);
    Nullable_options opt2(Comparison_mode::CASE_SENSITIVE);
    Nullable_options opt3(Comparison_mode::CASE_SENSITIVE, "Sample");

    ASSERT_TRUE(opt1 == opt2);
    ASSERT_FALSE(opt1 != opt2);
    ASSERT_FALSE(opt2 == opt3);
    ASSERT_TRUE(opt2 != opt3);
  }

  // Number of options
  {
    Nullable_options opt1(Comparison_mode::CASE_SENSITIVE);
    Nullable_options opt2(Comparison_mode::CASE_SENSITIVE);
    Nullable_options opt3(Comparison_mode::CASE_SENSITIVE);

    opt1.set("sample", "value");
    opt2.set("sample", "value");

    ASSERT_TRUE(opt1 == opt2);
    ASSERT_FALSE(opt1 != opt2);
    ASSERT_FALSE(opt2 == opt3);
    ASSERT_TRUE(opt2 != opt3);
  }

  // Which options case sensitive
  {
    Nullable_options opt1(Comparison_mode::CASE_SENSITIVE);
    Nullable_options opt2(Comparison_mode::CASE_SENSITIVE);
    Nullable_options opt3(Comparison_mode::CASE_SENSITIVE);

    opt1.set("sample", "value");
    opt2.set("sample", "value");
    opt3.set("Sample", "value");

    ASSERT_TRUE(opt1 == opt2);
    ASSERT_FALSE(opt1 != opt2);
    ASSERT_FALSE(opt2 == opt3);
    ASSERT_TRUE(opt2 != opt3);
  }

  // Option values case sensitive
  {
    Nullable_options opt1(Comparison_mode::CASE_SENSITIVE);
    Nullable_options opt2(Comparison_mode::CASE_SENSITIVE);
    Nullable_options opt3(Comparison_mode::CASE_SENSITIVE);

    opt1.set("sample", "value");
    opt2.set("sample", "value");
    opt3.set("sample", "value");

    ASSERT_TRUE(opt1 == opt2);
    ASSERT_FALSE(opt1 != opt2);
    ASSERT_TRUE(opt2 == opt3);
    ASSERT_FALSE(opt2 != opt3);

    opt3.set("sample", "value2");
    ASSERT_TRUE(opt1 == opt2);
    ASSERT_FALSE(opt1 != opt2);
    ASSERT_FALSE(opt2 == opt3);
    ASSERT_TRUE(opt2 != opt3);
  }

  // Which options case insensitive
  {
    Nullable_options opt1(Comparison_mode::CASE_INSENSITIVE);
    Nullable_options opt2(Comparison_mode::CASE_INSENSITIVE);
    Nullable_options opt3(Comparison_mode::CASE_INSENSITIVE);

    opt1.set("sample", "value");
    opt2.set("sample", "value");
    opt3.set("Sample", "value");

    ASSERT_TRUE(opt1 == opt2);
    ASSERT_FALSE(opt1 != opt2);
    ASSERT_TRUE(opt2 == opt3);
    ASSERT_FALSE(opt2 != opt3);
  }

  // Option values case insensitive
  {
    Nullable_options opt1(Comparison_mode::CASE_INSENSITIVE);
    Nullable_options opt2(Comparison_mode::CASE_INSENSITIVE);
    Nullable_options opt3(Comparison_mode::CASE_INSENSITIVE);

    opt1.set("sample", "value");
    opt2.set("sample", "value");
    opt3.set("Sample", "value");

    ASSERT_TRUE(opt1 == opt2);
    ASSERT_FALSE(opt1 != opt2);
    ASSERT_TRUE(opt2 == opt3);
    ASSERT_FALSE(opt2 != opt3);

    opt3.set("Sample", "Value");
    ASSERT_TRUE(opt1 == opt2);
    ASSERT_FALSE(opt1 != opt2);
    ASSERT_FALSE(opt2 == opt3);
    ASSERT_TRUE(opt2 != opt3);
  }
}

TEST(Nullable_options, compare_function) {
  // Case sensitiveness
  {
    Nullable_options options(Comparison_mode::CASE_SENSITIVE);
    EXPECT_EQ(0, options.compare("Sample", "Sample"));
    EXPECT_NE(0, options.compare("Sample", "SAMPLE"));
  }
  // Case insensitiveness
  {
    Nullable_options options(Comparison_mode::CASE_INSENSITIVE);
    EXPECT_EQ(0, options.compare("Sample", "Sample"));
    EXPECT_EQ(0, options.compare("Sample", "SAMPLE"));
  }
}

TEST(Nullable_options, case_sensitive_set_modes) {
  Nullable_options options(Comparison_mode::CASE_SENSITIVE);

  EXPECT_NO_THROW(options.set("sample1", "val1", Set_mode::CREATE));
  EXPECT_STREQ("val1", options.get_value("sample1").c_str());
  // Testing option set modes(only creation allowed)
  MY_EXPECT_THROW(std::invalid_argument,
                  "The option 'sample1' is already defined as 'val1'.",
                  options.set("sample1", "value2", Set_mode::CREATE));

  EXPECT_NO_THROW(options.set("sample2", "val2", Set_mode::CREATE_AND_UPDATE));
  EXPECT_STREQ("val2", options.get_value("sample2").c_str());
  EXPECT_NO_THROW(options.set("sample2", "val3", Set_mode::CREATE_AND_UPDATE));
  EXPECT_STREQ("val3", options.get_value("sample2").c_str());

  EXPECT_NO_THROW(options.set("sample2", "val4", Set_mode::UPDATE_ONLY));
  EXPECT_STREQ("val4", options.get_value("sample2").c_str());
  // Testing option set modes(only creation allowed)
  MY_EXPECT_THROW(std::invalid_argument, "Invalid option 'sample3'.",
                  options.set("sample3", "value3", Set_mode::UPDATE_ONLY));

  EXPECT_NO_THROW(options.set("sample3", nullptr, Set_mode::CREATE));
  EXPECT_NO_THROW(options.set("sample3", "val3", Set_mode::UPDATE_NULL));
  EXPECT_STREQ("val3", options.get_value("sample3").c_str());
  MY_EXPECT_THROW(std::invalid_argument,
                  "The option 'sample3' is already defined as 'val3'.",
                  options.set("sample3", "value3", Set_mode::UPDATE_NULL));
}

}  // namespace testing
