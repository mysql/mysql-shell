/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include <cctype>
#include <fstream>
#include <string>

#include "mysqlshdk/libs/utils/version.h"
#include "unittest/gtest_clean.h"

namespace ngcommon {
namespace tests {

using mysqlshdk::utils::Version;

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

  ASSERT_ANY_THROW(mysqlshdk::utils::Version("-uno-dos"));
  ASSERT_ANY_THROW(mysqlshdk::utils::Version("5..2-uno-dos"));
  ASSERT_ANY_THROW(mysqlshdk::utils::Version("5.2.-uno-dos"));
  ASSERT_ANY_THROW(mysqlshdk::utils::Version("5.2.A-uno-dos"));
}

}  // namespace tests
}  // namespace ngcommon
