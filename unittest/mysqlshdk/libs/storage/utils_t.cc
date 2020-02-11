/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "unittest/gprod_clean.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils/shell_test_env.h"

#include "mysqlshdk/libs/storage/utils.h"

namespace mysqlshdk {
namespace storage {
namespace utils {
namespace tests {

TEST(storage_utils, get_scheme) {
  EXPECT_EQ(get_scheme(""), "");
  EXPECT_EQ(get_scheme("/file/path"), "");
  EXPECT_EQ(get_scheme("file:///file/path"), "file");
  EXPECT_EQ(get_scheme("file:///invalid:///path"), "file");
  EXPECT_EQ(get_scheme("file://"), "file");
  EXPECT_EQ(get_scheme("file:/"), "");

  EXPECT_THROW_LIKE(get_scheme("://"), std::invalid_argument,
                    "URI :// has an empty scheme.");
  EXPECT_THROW_LIKE(get_scheme(":///file/path"), std::invalid_argument,
                    "URI :///file/path has an empty scheme.");
}

TEST(storage_utils, strip_scheme) {
  EXPECT_EQ(strip_scheme(""), "");
  EXPECT_EQ(strip_scheme("", "file"), "");

  EXPECT_EQ(strip_scheme("file://"), "");
  EXPECT_EQ(strip_scheme("file://", "file"), "");
  EXPECT_EQ(strip_scheme("FILE://", "file"), "");
  EXPECT_THROW_LIKE(strip_scheme("file://", "http"), std::invalid_argument,
                    "URI file:// has an invalid scheme, expected: http.");

  EXPECT_EQ(strip_scheme("/file/path"), "/file/path");
  EXPECT_EQ(strip_scheme("/file/path", "file"), "/file/path");
  EXPECT_EQ(strip_scheme("/file/path", "FILE"), "/file/path");
  EXPECT_EQ(strip_scheme("/file/path", "http"), "/file/path");

  EXPECT_EQ(strip_scheme("file:///file/path"), "/file/path");
  EXPECT_EQ(strip_scheme("file:///file/path", "file"), "/file/path");
  EXPECT_EQ(strip_scheme("FILE:///file/path", "file"), "/file/path");
  EXPECT_THROW_LIKE(
      strip_scheme("file:///file/path", "http"), std::invalid_argument,
      "URI file:///file/path has an invalid scheme, expected: http.");

  EXPECT_THROW_LIKE(strip_scheme("://"), std::invalid_argument,
                    "URI :// has an empty scheme.");
  EXPECT_THROW_LIKE(strip_scheme(":///file/path"), std::invalid_argument,
                    "URI :///file/path has an empty scheme.");
  EXPECT_THROW_LIKE(strip_scheme("://", "file"), std::invalid_argument,
                    "URI :// has an empty scheme.");
  EXPECT_THROW_LIKE(strip_scheme(":///file/path", "file"),
                    std::invalid_argument,
                    "URI :///file/path has an empty scheme.");
}

TEST(storage_utils, scheme_matches) {
  EXPECT_TRUE(scheme_matches("", ""));

  EXPECT_FALSE(scheme_matches("file", ""));
  EXPECT_FALSE(scheme_matches("", "file"));

  EXPECT_TRUE(scheme_matches("file", "file"));
  EXPECT_TRUE(scheme_matches("FILE", "file"));
  EXPECT_TRUE(scheme_matches("file", "FILE"));
  EXPECT_TRUE(scheme_matches("FILE", "FILE"));

  EXPECT_FALSE(scheme_matches("http", "file"));
  EXPECT_FALSE(scheme_matches("HTTP", "file"));
  EXPECT_FALSE(scheme_matches("http", "FILE"));
  EXPECT_FALSE(scheme_matches("HTTP", "FILE"));
  EXPECT_FALSE(scheme_matches("https", "file"));
  EXPECT_FALSE(scheme_matches("HTTPS", "file"));
  EXPECT_FALSE(scheme_matches("https", "FILE"));
  EXPECT_FALSE(scheme_matches("HTTPS", "FILE"));
}

}  // namespace tests
}  // namespace utils
}  // namespace storage
}  // namespace mysqlshdk
