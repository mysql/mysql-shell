/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "unittest/mysqlshdk/libs/db/db_common.h"
#include "unittest/test_utils.h"

#include "mysqlshdk/libs/db/row_utils.h"

namespace mysqlshdk {
namespace db {

class Row_utils : public Db_tests {};

TEST_F(Row_utils, row_by_name) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    ASSERT_NO_THROW(session->connect(Connection_options(uri())));

    auto result = session->query("SELECT 1 as one, 'two' as two");
    auto row = result->fetch_one();
    {
      Row_ref_by_name rowb(result->field_names(), row);
      EXPECT_EQ(1, rowb.get_uint("one"));
      EXPECT_EQ("two", rowb.get_string("two"));
      EXPECT_THROW(rowb.get_string("three"), std::invalid_argument);
      EXPECT_THROW(rowb.get_string("one"), std::invalid_argument);
    }

    Row_by_name rowcopy(result->field_names(), *row);
    EXPECT_EQ(1, rowcopy.get_uint("one"));
    EXPECT_EQ("two", rowcopy.get_string("two"));
    EXPECT_THROW(rowcopy.get_string("three"), std::invalid_argument);

    result = session->query("SELECT 0 as foo");
    row = result->fetch_one();
    {
      Row_ref_by_name rowb(result->field_names(), row);
      EXPECT_EQ(0, rowb.get_uint("foo"));
      EXPECT_THROW(rowb.get_string("one"), std::invalid_argument);
    }

    EXPECT_EQ(1, rowcopy.get_uint("one"));
    EXPECT_EQ("two", rowcopy.get_string("two"));
    EXPECT_THROW(rowcopy.get_string("three"), std::invalid_argument);
  } while (switch_proto());
}

TEST_F(Row_utils, fetch_one_n) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    ASSERT_NO_THROW(session->connect(Connection_options(uri())));

    auto result = session->query("SELECT 1 as one, 'two' as two");
    auto rowb = result->fetch_one_named();
    {
      EXPECT_EQ(1, rowb.get_uint("one"));
      EXPECT_EQ("two", rowb.get_string("two"));
      EXPECT_THROW(rowb.get_string("three"), std::invalid_argument);
      EXPECT_THROW(rowb.get_string("one"), std::invalid_argument);
    }

    Row_by_name rowcopy(rowb);
    EXPECT_EQ(1, rowcopy.get_uint("one"));
    EXPECT_EQ("two", rowcopy.get_string("two"));
    EXPECT_THROW(rowcopy.get_string("three"), std::invalid_argument);

    result = session->query("SELECT 0 as foo");
    rowb = result->fetch_one_named();
    {
      EXPECT_EQ(0, rowb.get_uint("foo"));
      EXPECT_THROW(rowb.get_string("one"), std::invalid_argument);
    }

    EXPECT_EQ(1, rowcopy.get_uint("one"));
    EXPECT_EQ("two", rowcopy.get_string("two"));
    EXPECT_THROW(rowcopy.get_string("three"), std::invalid_argument);
  } while (switch_proto());
}

}  // namespace db
}  // namespace mysqlshdk
