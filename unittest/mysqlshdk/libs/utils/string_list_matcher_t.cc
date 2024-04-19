/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
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

#include "unittest/gtest_clean.h"

#include "mysqlshdk/libs/utils/string_list_matcher.h"
#include "unittest/test_utils/shell_test_env.h"

using STRSET = std::set<std::string>;

TEST(String_prefix_matcher, test_registration) {
  // Tests the string normalization
  mysqlshdk::String_prefix_matcher m(
      {"drop Rest Schema ", "  create rest  service  ", "cReatE rest schema ",
       "   cReatE rest  view "});

  EXPECT_EQ(STRSET({"CREATE REST SCHEMA ", "CREATE REST SERVICE ",
                    "CREATE REST VIEW ", "DROP REST SCHEMA "}),
            m.list_strings());

  // Tests the automatic simplification
  // NOTE: Smaller patterm matching existing patterns will cause they to be
  // discarded
  m.add_string("   cReatE rest  ");
  EXPECT_EQ(STRSET({"CREATE REST ", "DROP REST SCHEMA "}), m.list_strings());

  // Automatic simplication does not apply if tags are different
  EXPECT_THROW_LIKE(m.add_string("   cReatE  ", "other_tag"),
                    mysqlshdk::Prefix_overlap_error,
                    "The prefix '   cReatE  ' will shadow an existing prefix");

  // Tests ignoring overcomplication
  // Big patterns matched by already existing patterns will cause them to be
  // ignored
  m.add_string("CREATE REST PROCEDURE");
  EXPECT_EQ(STRSET({"CREATE REST ", "DROP REST SCHEMA "}), m.list_strings());

  // Automatic ignoring overcomplication does not apply if tags are different
  EXPECT_THROW_LIKE(
      m.add_string("CREATE REST PROCEDURE", "other_tag"),
      mysqlshdk::Prefix_overlap_error,
      "The prefix 'CREATE REST PROCEDURE' is shadowed by an existing prefix");

  // Tests acceptance of new patterns
  m.add_string("Drop rest service ");
  EXPECT_EQ(STRSET({"CREATE REST ", "DROP REST SCHEMA ", "DROP REST SERVICE "}),
            m.list_strings());
}

TEST(String_prefix_matcher, test_matching) {
  // Tests the string normalization
  mysqlshdk::String_prefix_matcher m({"create samPle ", "  delete sample  "});

  // Case insensitiveness..
  EXPECT_TRUE(m.matches("create   Sample   table"));
  EXPECT_TRUE(m.matches("Create\t\t\tSAMPLE\n\ntable"));
  EXPECT_TRUE(m.matches("   create   sample     table"));
  EXPECT_TRUE(m.matches("\n\tdelete   sample  view"));
}

TEST(String_prefix_matcher, test_check_overlapping) {
  {
    mysqlshdk::String_prefix_matcher m;
    m.add_string("create table", "one");
    try {
      m.check_for_overlaps("create tablespace");
      SCOPED_TRACE("Unexpected success checking for overlapping prefix");
      FAIL();
    } catch (const mysqlshdk::Prefix_overlap_error &error) {
      EXPECT_STREQ(
          "The prefix 'create tablespace' is shadowed by an existing prefix",
          error.what());
      EXPECT_STREQ("one", error.tag().data());
    }
  }
  {
    mysqlshdk::String_prefix_matcher m;
    m.add_string("create tablespace", "one");
    try {
      m.check_for_overlaps("create table");
      SCOPED_TRACE("Unexpected success checking for overlapping prefix");
      FAIL();
    } catch (const mysqlshdk::Prefix_overlap_error &error) {
      EXPECT_STREQ("The prefix 'create table' will shadow an existing prefix",
                   error.what());
      EXPECT_STREQ("one", error.tag().data());
    }
  }
}
