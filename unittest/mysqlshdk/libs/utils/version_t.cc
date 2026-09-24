/*
 * Copyright (c) 2017, 2026, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mysqlshdk/libs/utils/version.h"

#include <utility>
#include <vector>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

namespace mysqlshdk {
namespace utils {
namespace {

using testing::Contains;
using testing::ElementsAre;
using testing::Not;

TEST(Version, version_parsing) {
  // Full version
  {
    Version v("5.7.3-uno-dos");
    ASSERT_EQ(5, v.get_major());
    ASSERT_EQ(7, v.get_minor());
    ASSERT_EQ(3, v.get_patch());
    ASSERT_STREQ("uno-dos", v.get_extra().c_str());
    ASSERT_STREQ("5.7.3", v.get_base().c_str());
    ASSERT_STREQ("5.7.3-uno-dos", v.get_full().c_str());
  }

  // No patch
  {
    Version v("5.7-uno-dos");
    ASSERT_EQ(5, v.get_major());
    ASSERT_EQ(7, v.get_minor());
    ASSERT_EQ(0, v.get_patch());
    ASSERT_STREQ("uno-dos", v.get_extra().c_str());
    ASSERT_STREQ("5.7", v.get_base().c_str());
    ASSERT_STREQ("5.7-uno-dos", v.get_full().c_str());
  }

  // No patch and minor
  {
    Version v("5-uno-dos");
    ASSERT_EQ(5, v.get_major());
    ASSERT_EQ(0, v.get_minor());
    ASSERT_EQ(0, v.get_patch());
    ASSERT_STREQ("uno-dos", v.get_extra().c_str());
    ASSERT_STREQ("5", v.get_base().c_str());
    ASSERT_STREQ("5-uno-dos", v.get_full().c_str());
  }

  // No get_extra
  {
    Version v("5.7");
    ASSERT_EQ(5, v.get_major());
    ASSERT_EQ(7, v.get_minor());
    ASSERT_EQ(0, v.get_patch());
    ASSERT_STREQ("", v.get_extra().c_str());
    ASSERT_STREQ("5.7", v.get_base().c_str());
    ASSERT_STREQ("5.7", v.get_full().c_str());
  }

  // Comparisons
  {
    ASSERT_TRUE(Version("1.1.1") == Version("1.1.1"));
    ASSERT_TRUE(Version("1.1.0") == Version("1.1"));
    ASSERT_TRUE(Version("1.0.0") == Version("1"));

    ASSERT_FALSE(Version("1.0.0") == Version("1.0.1"));
    ASSERT_FALSE(Version("1.0.0") == Version("1.1.0"));
    ASSERT_FALSE(Version("1.0.0") == Version("2.0.0"));

    ASSERT_TRUE(Version("2.1.3") >= Version("2.1.3"));
    ASSERT_TRUE(Version("2.1.3") >= Version("2.1.2"));
    ASSERT_TRUE(Version("2.1.3") >= Version("2.1"));
    ASSERT_TRUE(Version("2.1.3") >= Version("2"));

    ASSERT_FALSE(Version("2.1.3") >= Version("2.1.4"));
    ASSERT_FALSE(Version("2.1.3") >= Version("2.2"));
    ASSERT_FALSE(Version("2.1.3") >= Version("3"));

    ASSERT_TRUE(Version("2.1.3") <= Version("2.1.3"));
    ASSERT_TRUE(Version("2.1.1") <= Version("2.1.2"));
    ASSERT_TRUE(Version("2.0") <= Version("2.1"));
    ASSERT_TRUE(Version("1.9.9") <= Version("2"));

    ASSERT_FALSE(Version("2.1.5") <= Version("2.1.4"));
    ASSERT_FALSE(Version("2.2.1") <= Version("2.2"));
    ASSERT_FALSE(Version("3.0.1") <= Version("3"));

    ASSERT_TRUE(Version("2.1.3") > Version("2.1.2"));
    ASSERT_TRUE(Version("2.1.3") > Version("2.1"));
    ASSERT_TRUE(Version("2.1.3") > Version("2"));

    ASSERT_FALSE(Version("2.1.3") > Version("2.1.3"));
    ASSERT_FALSE(Version("2.1.3") > Version("2.1.4"));
    ASSERT_FALSE(Version("2.1.3") > Version("2.2"));
    ASSERT_FALSE(Version("2.1.3") > Version("3"));

    ASSERT_TRUE(Version("2.1.1") < Version("2.1.2"));
    ASSERT_TRUE(Version("2.0") < Version("2.1"));
    ASSERT_TRUE(Version("1.9.9") < Version("2"));

    ASSERT_FALSE(Version("2.1.3") < Version("2.1.3"));
    ASSERT_FALSE(Version("2.1.5") < Version("2.1.4"));
    ASSERT_FALSE(Version("2.2.1") < Version("2.2"));
    ASSERT_FALSE(Version("3.0.1") < Version("3"));

    ASSERT_TRUE(Version("1.0.0") != Version("1.0.1"));
    ASSERT_TRUE(Version("1.0.0") != Version("1.1.0"));
    ASSERT_TRUE(Version("1.0.0") != Version("2.0.0"));

    ASSERT_FALSE(Version("1.1.1") != Version("1.1.1"));
    ASSERT_FALSE(Version("1.1.0") != Version("1.1"));
    ASSERT_FALSE(Version("1.0.0") != Version("1"));
  }

  ASSERT_ANY_THROW(Version("-uno-dos"));
  ASSERT_ANY_THROW(Version("5..2-uno-dos"));
  ASSERT_ANY_THROW(Version("5.2.-uno-dos"));
  ASSERT_ANY_THROW(Version("5.2.A-uno-dos"));

  EXPECT_EQ(80000, Version("8.0.0").numeric());
  EXPECT_EQ(10203, Version("1.2.3").numeric());
  EXPECT_EQ(12345, Version("1.23.45").numeric());

  EXPECT_STREQ("26.7", Version(26, 7, 0).get_short().c_str());
  EXPECT_STREQ("26.7.0", Version(26, 7, 0).get_base().c_str());
  EXPECT_STREQ("26.7.1-rc", Version("26.7.1-rc").get_full().c_str());
  EXPECT_STREQ("8.4.0", Version(8, 4, 0).get_base().c_str());
}

TEST(Version, update_parsing_and_formatting) {
  for (const auto *base : {"8.4.8", "9.7.0", "26.7.1"}) {
    for (const int update : {0, 1, 123}) {
      for (const auto *extra : {"", "-debug", "-commercial-cloud"}) {
        const auto full =
            std::string(base) + "." + std::to_string(update) + extra;
        SCOPED_TRACE(full);
        const Version v(full);
        EXPECT_TRUE(v.has_update());
        EXPECT_EQ(update, v.get_update());
        EXPECT_EQ(std::string(base) + "." + std::to_string(update),
                  v.get_base_with_update());
        EXPECT_EQ(full, v.get_full_with_update());
        EXPECT_EQ(base, v.get_base());
        EXPECT_EQ(std::string(base) + extra, v.get_full());
        EXPECT_EQ(Version(std::string(base) + extra).is_mds(), v.is_mds());
      }
    }
  }

  for (const auto &[full, base] :
       std::vector<std::pair<const char *, const char *>>{
           {"8", "8"},
           {"8.4", "8.4"},
           {"8.4.8", "8.4.8"},
           {"8.4.8-debug", "8.4.8"},
       }) {
    const Version v(full);
    EXPECT_FALSE(v.has_update());
    EXPECT_EQ(0, v.get_update());
    EXPECT_EQ(base, v.get_base_with_update());
    EXPECT_EQ(full, v.get_full_with_update());
  }

  for (const auto *invalid : {"9.7.0.", "9.7.0.x", "9.7.0.1x", "9.7.0.1.2",
                              "9.7.0.2147483648", "9.7.0.-debug"}) {
    SCOPED_TRACE(invalid);
    EXPECT_THROW(Version{invalid}, std::invalid_argument);
  }
}

TEST(Version, update_does_not_affect_compatibility) {
  for (const auto *base : {"8.4.8", "9.7.0", "26.7.1"}) {
    for (const auto *update : {"", ".0", ".1", ".123-debug"}) {
      const Version v(std::string(base) + update);
      for (const auto *other_update : {"", ".0", ".2", ".456-commercial"}) {
        const Version other(std::string(base) + other_update);
        EXPECT_TRUE(v == other);
        EXPECT_FALSE(v != other);
        EXPECT_TRUE(v <= other);
        EXPECT_TRUE(v >= other);
        EXPECT_FALSE(v < other);
        EXPECT_FALSE(v > other);
        EXPECT_EQ(other.numeric(), v.numeric());
        EXPECT_EQ(other.numeric_version_series(), v.numeric_version_series());
        EXPECT_EQ(version::is_lts(other), version::is_lts(v));
        EXPECT_EQ(0, version::major_difference(v, other));
      }
      EXPECT_LT(v, Version(v.get_major(), v.get_minor(), v.get_patch() + 1));
    }
  }
}

TEST(Version, corresponding_versions) {
  EXPECT_THAT(version::corresponding_versions(Version(8, 1, 0)),
              ElementsAre(Version(8, 0, 34), Version(8, 1, 0)));

  EXPECT_THAT(version::corresponding_versions(Version(8, 1, 1)),
              ElementsAre(Version(8, 0, 34), Version(8, 1, 1)));

  EXPECT_THAT(version::corresponding_versions(Version(8, 2, 0)),
              ElementsAre(Version(8, 0, 35), Version(8, 2, 0)));

  EXPECT_THAT(version::corresponding_versions(Version(8, 2, 1)),
              ElementsAre(Version(8, 0, 35), Version(8, 2, 1)));

  EXPECT_THAT(version::corresponding_versions(Version(8, 4, 0)),
              ElementsAre(Version(8, 0, 37), Version(8, 4, 0)));

  EXPECT_THAT(version::corresponding_versions(Version(8, 4, 1)),
              ElementsAre(Version(8, 0, 38), Version(8, 4, 1)));

  EXPECT_THAT(version::corresponding_versions(Version(8, 4, 2)),
              ElementsAre(Version(8, 0, 39), Version(8, 4, 2)));

  EXPECT_THAT(version::corresponding_versions(Version(8, 4, 3)),
              ElementsAre(Version(8, 0, 40), Version(8, 4, 3)));

  EXPECT_THAT(
      version::corresponding_versions(Version(9, 0, 0)),
      ElementsAre(Version(8, 0, 38), Version(8, 4, 1), Version(9, 0, 0)));

  EXPECT_THAT(
      version::corresponding_versions(Version(9, 0, 1)),
      ElementsAre(Version(8, 0, 39), Version(8, 4, 2), Version(9, 0, 1)));

  EXPECT_THAT(
      version::corresponding_versions(Version(9, 0, 2)),
      ElementsAre(Version(8, 0, 39), Version(8, 4, 2), Version(9, 0, 2)));

  EXPECT_THAT(
      version::corresponding_versions(Version(9, 1, 0)),
      ElementsAre(Version(8, 0, 40), Version(8, 4, 3), Version(9, 1, 0)));

  EXPECT_THAT(
      version::corresponding_versions(Version(9, 1, 1)),
      ElementsAre(Version(8, 0, 40), Version(8, 4, 3), Version(9, 1, 1)));

  EXPECT_THAT(
      version::corresponding_versions(Version(9, 7, 0)),
      ElementsAre(Version(8, 0, 46), Version(8, 4, 9), Version(9, 7, 0)));

  EXPECT_THAT(
      version::corresponding_versions(Version(9, 7, 1)),
      ElementsAre(Version(8, 0, 47), Version(8, 4, 10), Version(9, 7, 1)));

  EXPECT_THAT(version::corresponding_versions(Version(8, 4, 10)),
              ElementsAre(Version(8, 0, 47), Version(8, 4, 10)));

  EXPECT_THAT(version::corresponding_versions(Version(8, 4, 11)),
              ElementsAre(Version(8, 4, 11)));

  EXPECT_THAT(version::corresponding_versions(Version(9, 7, 2)),
              ElementsAre(Version(8, 4, 11), Version(9, 7, 2)));

  EXPECT_THAT(version::corresponding_versions(Version(9, 7, 10)),
              ElementsAre(Version(8, 4, 19), Version(9, 7, 10)));

  EXPECT_THAT(
      version::corresponding_versions(Version(26, 7, 0)),
      ElementsAre(Version(8, 4, 11), Version(9, 7, 2), Version(26, 7, 0)));

  EXPECT_THAT(
      version::corresponding_versions(Version(26, 7, 1)),
      ElementsAre(Version(8, 4, 12), Version(9, 7, 3), Version(26, 7, 1)));

  EXPECT_THAT(
      version::corresponding_versions(Version(26, 10, 0)),
      ElementsAre(Version(8, 4, 13), Version(9, 7, 4), Version(26, 10, 0)));

  EXPECT_THAT(
      version::corresponding_versions(Version(27, 1, 0)),
      ElementsAre(Version(8, 4, 14), Version(9, 7, 5), Version(27, 1, 0)));

  EXPECT_THAT(
      version::corresponding_versions(Version(28, 4, 0)),
      ElementsAre(Version(8, 4, 19), Version(9, 7, 10), Version(28, 4, 0)));

  EXPECT_THAT(
      version::corresponding_versions(Version(28, 4, 1)),
      ElementsAre(Version(8, 4, 20), Version(9, 7, 11), Version(28, 4, 1)));

  EXPECT_THAT(
      version::corresponding_versions(Version(28, 4, 5)),
      ElementsAre(Version(8, 4, 24), Version(9, 7, 15), Version(28, 4, 5)));

  EXPECT_THAT(version::corresponding_versions(Version(28, 7, 0)),
              ElementsAre(Version(8, 4, 20), Version(9, 7, 11),
                          Version(28, 4, 1), Version(28, 7, 0)));

  const auto overlap_versions =
      version::corresponding_versions(Version(63, 1, 1));
  EXPECT_EQ(Version(63, 1, 1), overlap_versions.back());
  EXPECT_THAT(overlap_versions, Contains(Version(62, 4, 4)));
  EXPECT_THAT(overlap_versions, Not(Contains(Version(62, 4, 5))));
}

TEST(Version, release_flavour_helpers) {
  EXPECT_TRUE(version::is_lts(Version(8, 0, 41)));
  EXPECT_TRUE(version::is_lts(Version(8, 4, 4)));
  EXPECT_TRUE(version::is_lts(Version(9, 7, 1)));

  EXPECT_FALSE(version::is_lts(Version(26, 7, 0)));
  EXPECT_FALSE(version::is_lts(Version(26, 7, 1)));
  EXPECT_TRUE(version::is_lts(Version(28, 4, 0)));
  EXPECT_TRUE(version::is_lts(Version(28, 4, 1)));
  EXPECT_FALSE(version::is_lts(Version(28, 7, 0)));

  EXPECT_FALSE(version::calendar::is_calendar_version(Version(26, 6, 99)));
  EXPECT_TRUE(version::calendar::is_calendar_version(Version(26, 7, 0)));
  EXPECT_TRUE(version::calendar::is_calendar_version(Version(26, 8, 0)));
  EXPECT_TRUE(version::calendar::is_calendar_version(Version(26, 10, 0)));
  EXPECT_TRUE(version::calendar::is_calendar_version(Version(26, 11, 0)));
  EXPECT_TRUE(version::calendar::is_calendar_version(Version(26, 12, 5)));
  EXPECT_FALSE(version::calendar::is_calendar_version(Version(27, 0, 0)));
  EXPECT_TRUE(version::calendar::is_calendar_version(Version(27, 1, 0)));
  EXPECT_TRUE(version::calendar::is_calendar_version(Version(27, 2, 0)));
  EXPECT_TRUE(version::calendar::is_calendar_version(Version(27, 4, 0)));
  EXPECT_TRUE(version::calendar::is_calendar_version(Version(27, 7, 0)));
  EXPECT_TRUE(version::calendar::is_calendar_version(Version(27, 10, 0)));
  EXPECT_TRUE(version::calendar::is_calendar_version(Version(28, 5, 0)));

  EXPECT_TRUE(version::calendar::is_release_cadence_version(Version(26, 7, 0)));
  EXPECT_TRUE(
      version::calendar::is_release_cadence_version(Version(26, 10, 0)));
  EXPECT_FALSE(
      version::calendar::is_release_cadence_version(Version(26, 11, 0)));
  EXPECT_FALSE(
      version::calendar::is_release_cadence_version(Version(28, 5, 0)));

  EXPECT_EQ(Version(8, 4, 0), version::first_lts(Version(8, 1, 0)));
  EXPECT_EQ(Version(9, 7, 0), version::first_lts(Version(9, 0, 0)));
  EXPECT_EQ(Version(28, 4, 0), version::first_lts(Version(26, 7, 0)));
  EXPECT_EQ(Version(28, 4, 0), version::first_lts(Version(28, 4, 1)));
  EXPECT_EQ(Version(30, 4, 0), version::first_lts(Version(28, 7, 0)));

  EXPECT_EQ(Version(8, 4, 0), version::next_lts(Version(8, 0, 41)));
  EXPECT_EQ(Version(8, 4, 0), version::next_lts(Version(8, 1, 0)));
  EXPECT_EQ(Version(9, 7, 0), version::next_lts(Version(8, 4, 4)));
  EXPECT_EQ(Version(9, 7, 0), version::next_lts(Version(9, 0, 0)));
  EXPECT_EQ(Version(28, 4, 0), version::next_lts(Version(9, 7, 1)));
  EXPECT_EQ(Version(28, 4, 0), version::next_lts(Version(26, 7, 0)));
  EXPECT_EQ(Version(30, 4, 0), version::next_lts(Version(28, 4, 1)));
  EXPECT_EQ(Version(30, 4, 0), version::next_lts(Version(28, 5, 0)));
  EXPECT_EQ(Version(30, 4, 0), version::next_lts(Version(28, 7, 0)));

  EXPECT_EQ(Version(8, 1, 0), version::first_innovation(Version(8, 4, 1)));
  EXPECT_EQ(Version(9, 0, 0), version::first_innovation(Version(9, 7, 1)));
  EXPECT_EQ(Version(26, 7, 0), version::first_innovation(Version(28, 4, 1)));
  EXPECT_EQ(Version(28, 7, 0), version::first_innovation(Version(28, 7, 0)));

  EXPECT_EQ(Version(26, 3, 0), version::calendar::to_legacy(Version(59, 4, 0)));
  EXPECT_EQ(Version(26, 7, 0), version::legacy::first_lts(Version(26, 3, 0)));
  EXPECT_FALSE(version::legacy::is_lts(Version(28, 4, 1)));
  EXPECT_TRUE(version::calendar::is_lts(Version(28, 4, 1)));
}

TEST(Version, major_difference_calendar_transition) {
  EXPECT_EQ(1, version::major_difference(Version(9, 7, 1), Version(26, 7, 0)));
  EXPECT_EQ(-1, version::major_difference(Version(26, 7, 0), Version(9, 7, 1)));
  EXPECT_EQ(0, version::major_difference(Version(26, 7, 0), Version(28, 4, 0)));
  EXPECT_EQ(1, version::major_difference(Version(28, 4, 0), Version(28, 5, 0)));
  EXPECT_EQ(1, version::major_difference(Version(28, 4, 0), Version(28, 7, 0)));
}

TEST(Version, supported_servers) {
  EXPECT_FALSE(version::is_supported_server(Version(5, 7, 44)));
  EXPECT_TRUE(version::is_supported_server(Version(8, 0, 0)));
  EXPECT_TRUE(version::is_supported_server(Version(9, 7, 1)));
  EXPECT_FALSE(version::is_supported_server(Version(10, 0, 0)));
  EXPECT_FALSE(version::is_supported_server(Version(26, 6, 99)));
  EXPECT_TRUE(version::is_supported_server(Version(26, 7, 0)));
  EXPECT_TRUE(version::is_supported_server(Version(26, 7, 99)));
  EXPECT_TRUE(version::is_supported_server(Version(26, 10, 0)));
  EXPECT_TRUE(version::is_supported_server(Version(26, 10, 99)));
  EXPECT_FALSE(version::is_supported_server(Version(27, 1, 0)));
}

}  // namespace
}  // namespace utils
}  // namespace mysqlshdk
