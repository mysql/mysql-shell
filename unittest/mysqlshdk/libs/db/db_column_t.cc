/* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2.0,
 as published by the Free Software Foundation.

 This program is also distributed with certain software (including
 but not limited to OpenSSL) that is licensed under separate terms, as
 designated in a particular file or component or in included license
 documentation.  The authors of MySQL hereby grant you an additional
 permission to link the program and your derivative works with the
 separately licensed software that they have included with MySQL.
 This program is distributed in the hope that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 the GNU General Public License, version 2.0, for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include "unittest/mysqlshdk/libs/db/db_common.h"

namespace mysqlshdk {
namespace db {

TEST_F(Db_tests, metadata_columns) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    ASSERT_NO_THROW(session->connect(Connection_options(uri())));

    {
      auto result = session->query("set @a = 1");
      EXPECT_TRUE(nullptr == result->fetch_one());
      EXPECT_EQ(0, result->get_metadata().size());
    }

    {
      auto result = session->query(
          "select 1 as one, 1+1 as two, -3, null, c1 as Col1 from "
          "xtest.t_tinyint");
      EXPECT_TRUE(nullptr != result->fetch_one());
      const std::vector<Column> &columns = result->get_metadata();
      EXPECT_EQ(5, columns.size());
      EXPECT_EQ(Type::Integer, columns[0].get_type());
      EXPECT_EQ("", columns[0].get_schema());
      EXPECT_EQ("", columns[0].get_table_name());
      EXPECT_EQ("", columns[0].get_table_label());
      EXPECT_EQ("", columns[0].get_column_name());
      EXPECT_EQ("one", columns[0].get_column_label());
      EXPECT_FALSE(columns[0].is_unsigned());
      EXPECT_FALSE(columns[0].is_zerofill());
      if (is_classic) {
        EXPECT_TRUE(columns[0].is_binary());
      } else {
        // Classic returns BINARY flag, X proto doesn't
        // Don't know if it matters, tho
      }

      EXPECT_EQ(Type::Integer, columns[1].get_type());
      EXPECT_EQ("", columns[1].get_schema());
      EXPECT_EQ("", columns[1].get_table_name());
      EXPECT_EQ("", columns[1].get_table_label());
      EXPECT_EQ("", columns[1].get_column_name());
      EXPECT_EQ("two", columns[1].get_column_label());
      EXPECT_FALSE(columns[1].is_unsigned());
      EXPECT_FALSE(columns[1].is_zerofill());
      if (is_classic) {
        EXPECT_TRUE(columns[1].is_binary());
      } else {
        // Classic returns BINARY flag, X proto doesn't
        // Don't know if it matters, tho
      }

      EXPECT_EQ(Type::Integer, columns[2].get_type());
      EXPECT_EQ("", columns[2].get_schema());
      EXPECT_EQ("", columns[2].get_table_name());
      EXPECT_EQ("", columns[2].get_table_label());
      EXPECT_EQ("", columns[2].get_column_name());
      EXPECT_EQ("-3", columns[2].get_column_label());
      EXPECT_FALSE(columns[2].is_unsigned());
      EXPECT_FALSE(columns[2].is_zerofill());
      if (is_classic) {
        EXPECT_TRUE(columns[2].is_binary());
      } else {
        // Classic returns BINARY flag, X proto doesn't
        // Don't know if it matters, tho
      }

      if (is_classic) {
        EXPECT_EQ(Type::Null, columns[3].get_type());
      } else {
        EXPECT_EQ(Type::Bytes, columns[3].get_type());
      }
      EXPECT_EQ("", columns[3].get_schema());
      EXPECT_EQ("", columns[3].get_table_name());
      EXPECT_EQ("", columns[3].get_table_label());
      EXPECT_EQ("", columns[3].get_column_name());
      EXPECT_EQ("NULL", columns[3].get_column_label());
      EXPECT_FALSE(columns[3].is_unsigned());
      EXPECT_FALSE(columns[3].is_zerofill());
      if (is_classic) {
        EXPECT_TRUE(columns[3].is_binary());
      } else {
        // Classic returns BINARY flag, X proto doesn't
        // Don't know if it matters, tho
      }

      EXPECT_EQ(Type::Integer, columns[4].get_type());
      EXPECT_EQ("xtest", columns[4].get_schema());
      EXPECT_EQ("t_tinyint", columns[4].get_table_name());
      EXPECT_EQ("t_tinyint", columns[4].get_table_label());
      EXPECT_EQ("c1", columns[4].get_column_name());
      EXPECT_EQ("Col1", columns[4].get_column_label());
      EXPECT_FALSE(columns[4].is_unsigned());
      EXPECT_FALSE(columns[4].is_zerofill());
      EXPECT_FALSE(columns[4].is_binary());
    }
  } while (switch_proto());
}

}  // namespace db
}  // namespace mysqlshdk
