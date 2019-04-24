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

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"

namespace shcore {
namespace test {

class Utils_file_test : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    s_test_folder = path::join_path(getenv("TMPDIR"), "utils_file_test");
  }

  void SetUp() override { create_directory(s_test_folder); }

  void TearDown() override { remove_directory(s_test_folder, true); }

  static std::string s_test_folder;
};

std::string Utils_file_test::s_test_folder;

TEST_F(Utils_file_test, exists) {
  {
    const auto directory = path::join_path(s_test_folder, "1");
    create_directory(directory);
    EXPECT_TRUE(path_exists(directory));
    EXPECT_TRUE(is_folder(directory));
    EXPECT_FALSE(is_file(directory));
  }

  {
    const auto file = path::join_path(s_test_folder, "2.txt");
    EXPECT_TRUE(create_file(file, ""));
    EXPECT_TRUE(path_exists(file));
    EXPECT_FALSE(is_folder(file));
    EXPECT_TRUE(is_file(file));
  }

  {
    const auto missing = path::join_path(s_test_folder, "missing");
    EXPECT_FALSE(path_exists(missing));
    EXPECT_FALSE(is_folder(missing));
    EXPECT_FALSE(is_file(missing));
  }
}

TEST_F(Utils_file_test, create_directory) {
  const auto dir = path::join_path(s_test_folder, "foo");

  ASSERT_FALSE(path_exists(dir));
  EXPECT_NO_THROW(create_directory(path::join_path(dir, "bar/baz"), true));
  EXPECT_NO_THROW(remove_directory(dir, true));
  ASSERT_FALSE(path_exists(dir));
}

}  // namespace test
}  // namespace shcore
