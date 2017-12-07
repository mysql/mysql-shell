/* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include <cctype>
#include <fstream>
#include <string>

#include "unittest/gtest_clean.h"
#include "mysqlshdk/libs/utils/version.h"

namespace ngcommon {
namespace tests {

using mysqlshdk::utils::Version;

TEST(Version, version_parsing) {
  // Full version
  {
    Version v("5.7.3-uno-dos");
    ASSERT_EQ(5, v.major());
    ASSERT_EQ(7, v.minor());
    ASSERT_EQ(3, v.patch());
    ASSERT_STREQ("uno-dos", v.extra().c_str());
    ASSERT_STREQ("5.7.3", v.base().c_str());
    ASSERT_STREQ("5.7.3-uno-dos", v.full().c_str());
  }

  // No patch
  {
    Version v("5.7-uno-dos");
    ASSERT_EQ(5, v.major());
    ASSERT_EQ(7, v.minor());
    ASSERT_EQ(0, v.patch());
    ASSERT_STREQ("uno-dos", v.extra().c_str());
    ASSERT_STREQ("5.7", v.base().c_str());
    ASSERT_STREQ("5.7-uno-dos", v.full().c_str());
  }

  // No patch and minor
  {
    Version v("5-uno-dos");
    ASSERT_EQ(5, v.major());
    ASSERT_EQ(0, v.minor());
    ASSERT_EQ(0, v.patch());
    ASSERT_STREQ("uno-dos", v.extra().c_str());
    ASSERT_STREQ("5", v.base().c_str());
    ASSERT_STREQ("5-uno-dos", v.full().c_str());
  }

  // No extra
  {
    Version v("5.7");
    ASSERT_EQ(5, v.major());
    ASSERT_EQ(7, v.minor());
    ASSERT_EQ(0, v.patch());
    ASSERT_STREQ("", v.extra().c_str());
    ASSERT_STREQ("5.7", v.base().c_str());
    ASSERT_STREQ("5.7", v.full().c_str());
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
