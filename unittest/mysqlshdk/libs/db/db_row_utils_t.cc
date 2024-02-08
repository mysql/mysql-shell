/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

    auto result =
        session->query("SELECT 1 as one, 'two' as two, NULL as three");
    auto row = result->fetch_one();
    {
      Row_ref_by_name rowb(result->field_names(), row);
      EXPECT_EQ(1, rowb.get_uint("one"));
      EXPECT_EQ("two", rowb.get_string("two"));
      EXPECT_THROW(rowb.get_string("three"), std::invalid_argument);
      EXPECT_THROW(rowb.get_string("one"), std::invalid_argument);

      EXPECT_TRUE(rowb.is_null("three"));
      EXPECT_FALSE(rowb.is_null("two"));
      EXPECT_TRUE(rowb.has_field("three"));
      EXPECT_FALSE(rowb.has_field("bogus"));

      EXPECT_THROW(rowb.is_null("bogus"), std::invalid_argument);
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

    EXPECT_TRUE(rowcopy.is_null("three"));
    EXPECT_FALSE(rowcopy.is_null("two"));
    EXPECT_TRUE(rowcopy.has_field("three"));
    EXPECT_FALSE(rowcopy.has_field("bogus"));

    EXPECT_THROW(rowcopy.is_null("bogus"), std::invalid_argument);
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

TEST_F(Row_utils, fetch_by_name_multi_results) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    ASSERT_NO_THROW(session->connect(Connection_options(uri())));

    session->execute("drop schema if exists bug29451154");
    session->execute("create schema bug29451154");
    session->execute("use bug29451154");
    session->execute(
        "CREATE PROCEDURE samplesp() BEGIN "
        "SELECT 'name','last_name', 1 as number; SELECT 1 as one, 2 as two; "
        "END;");
    auto result = session->query("call samplesp()");
    auto rowa = result->fetch_one_named();
    {
      EXPECT_EQ("name", rowa.get_string("name"));
      EXPECT_EQ("last_name", rowa.get_string("last_name"));
      EXPECT_EQ(1, rowa.get_uint("number"));
    }

    result->next_resultset();
    auto rowb = result->fetch_one_named();
    {
      EXPECT_EQ(1, rowb.get_uint("one"));
      EXPECT_EQ(2, rowb.get_uint("two"));
    }
    EXPECT_TRUE(result->next_resultset());
    session->execute("drop schema bug29451154");
  } while (switch_proto());
}

}  // namespace db
}  // namespace mysqlshdk
