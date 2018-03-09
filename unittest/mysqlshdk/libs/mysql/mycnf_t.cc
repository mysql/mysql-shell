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

#include "mysqlshdk/libs/mysql/mycnf.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

namespace mysqlshdk {
namespace mysql {
namespace mycnf {

#define TEST_FILE "testmy.cnf"

TEST(Mycnf, update_options_update) {
  shcore::create_file(TEST_FILE, R"*([mysql]
user=foo
password=bar

[mysqld]
log_general=1
)*");

  update_options(TEST_FILE, "[mysqld]",
                 std::vector<Option>{{"user", {}},
                                     {"log_general", {"2"}},
                                     {"bin_log", {"/var/lib/mysql/binlog"}}});

  std::string file;
  shcore::load_text_file(TEST_FILE, file);

  static const char *expected = R"*([mysql]
user=foo
password=bar

[mysqld]
log_general=2
bin_log=/var/lib/mysql/binlog
)*";

  EXPECT_STREQ(expected, file.c_str());

  shcore::delete_file(TEST_FILE);
}

TEST(Mycnf, update_options_remove) {
  shcore::create_file(TEST_FILE, R"*([mysql]
user=foo
password=bar

[mysqld]
user=bla
)*");

  update_options(
      TEST_FILE, "[mysqld]",
      std::vector<Option>{{"user", {}}, {"password", {}}});

  std::string file;
  shcore::load_text_file(TEST_FILE, file);

  static const char *expected = R"*([mysql]
user=foo
password=bar

[mysqld]
)*";

  EXPECT_STREQ(expected, file.c_str());

  shcore::delete_file(TEST_FILE);
}

TEST(Mycnf, update_options_blank) {
  shcore::create_file(TEST_FILE, R"*([mysql]
user=foo
password=bar

[mysqld]
foo=
bar
)*");

  update_options(TEST_FILE, "[mysqld]",
                 std::vector<Option>{
                     {"foo", {"hello world"}}, {"bar", {"x"}}, {"baz", {""}}});

  std::string file;
  shcore::load_text_file(TEST_FILE, file);

  static const char *expected = R"*([mysql]
user=foo
password=bar

[mysqld]
foo=hello world
bar=x
baz=
)*";

  EXPECT_STREQ(expected, file.c_str());

  shcore::delete_file(TEST_FILE);
}


TEST(Mycnf, update_options_empty) {
  shcore::create_file(TEST_FILE, "");

  update_options(TEST_FILE, "[mysqld]",
                 std::vector<Option>{
                     {"foo", {"hello world"}}, {"bar", {"x"}}, {"baz", {""}}});

  std::string file;
  shcore::load_text_file(TEST_FILE, file);

  static const char *expected = R"*([mysqld]
foo=hello world
bar=x
baz=
)*";

  EXPECT_STREQ(expected, file.c_str());

  shcore::delete_file(TEST_FILE);
}

}  // namespace mycnf
}  // namespace mysql
}  // namespace mysqlshdk
