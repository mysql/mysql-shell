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

class utils_file : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    s_test_folder = path::join_path(getenv("TMPDIR"), "test_utils_file");
  }

  void SetUp() override { create_directory(s_test_folder); }

  void TearDown() override { remove_directory(s_test_folder, false); }

  static std::string s_test_folder;
};

std::string utils_file::s_test_folder;

TEST_F(utils_file, exists) {
  {
    const auto directory = path::join_path(s_test_folder, "1");
    create_directory(directory);
    EXPECT_TRUE(path_exists(directory));
    EXPECT_TRUE(is_folder(directory));
    EXPECT_FALSE(is_file(directory));
    EXPECT_NO_THROW(remove_directory(directory, false));
    ASSERT_FALSE(path_exists(directory));
  }

  {
    const auto directory = s_test_folder + "\\zażółć_gęślą_jaźń";
    create_directory(directory);
    EXPECT_TRUE(path_exists(directory));
    EXPECT_TRUE(is_folder(directory));
    EXPECT_FALSE(is_file(directory));
    EXPECT_NO_THROW(remove_directory(directory, false));
    ASSERT_FALSE(path_exists(directory));
  }

  {
    const auto file = path::join_path(s_test_folder, "2.txt");
    EXPECT_TRUE(create_file(file, ""));
    EXPECT_TRUE(path_exists(file));
    EXPECT_FALSE(is_folder(file));
    EXPECT_TRUE(is_file(file));
    EXPECT_NO_THROW(delete_file(file, false));
    EXPECT_FALSE(path_exists(file));
  }

  {
    const auto file = path::join_path(s_test_folder, "zażółć_gęślą_jaźń.txt");
    EXPECT_TRUE(
        create_file(file, "Pchnąć w tę łódź jeża lub ośm skrzyń fig", true));
    EXPECT_TRUE(path_exists(file));
    EXPECT_FALSE(is_folder(file));
    EXPECT_TRUE(is_file(file));
    EXPECT_NO_THROW(delete_file(file, false));
    EXPECT_FALSE(path_exists(file));
  }

  {
    const auto file =
        path::join_path(s_test_folder, "ZAŻÓŁĆ_GĘŚLĄ_JAŹŃ_CAPITAL.txt");
    EXPECT_TRUE(create_file(file, "Pchnąć w tę łódź jeża lub ośm skrzyń fig"));
    EXPECT_TRUE(path_exists(file));
    EXPECT_FALSE(is_folder(file));
    EXPECT_TRUE(is_file(file));
    EXPECT_NO_THROW(delete_file(file, false));
    EXPECT_FALSE(path_exists(file));
  }

  {
    const auto missing = path::join_path(s_test_folder, "missing");
    EXPECT_FALSE(path_exists(missing));
    EXPECT_FALSE(is_folder(missing));
    EXPECT_FALSE(is_file(missing));
  }
}

TEST_F(utils_file, create_directory) {
  {
    const auto dir = path::join_path(s_test_folder, "foo");
    ASSERT_FALSE(path_exists(dir));
    EXPECT_NO_THROW(create_directory(path::join_path(dir, "bar/baz"), true));
    ASSERT_TRUE(path_exists(dir));
    EXPECT_NO_THROW(remove_directory(dir, true));
    ASSERT_FALSE(path_exists(dir));
  }

  {
    const auto dir = path::join_path(s_test_folder, "Zażółć");
    ASSERT_FALSE(path_exists(dir));
    EXPECT_NO_THROW(create_directory(
        path::join_path(dir, "gęślą/jaźń/ZAŻÓŁĆ/ÓŚM/SKRZYŃ/x/y/z/a/b/c"),
        true));
    ASSERT_TRUE(path_exists(dir));
    EXPECT_NO_THROW(remove_directory(dir, true));
    ASSERT_FALSE(path_exists(dir));
  }
}

}  // namespace test
}  // namespace shcore
