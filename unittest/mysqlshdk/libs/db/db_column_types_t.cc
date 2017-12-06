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

#define TABLE(table)                                                \
  auto result = session->query("select * from xtest." table); \
  const std::vector<Column> &columns = result->get_metadata();

#define CHECK(i, type, length, un, zf, bin)     \
  EXPECT_EQ(type, columns[i].get_type());       \
  if (length > 0) {                             \
    EXPECT_EQ(length, columns[i].get_length()); \
  }                                             \
  if (un) {                                     \
    EXPECT_TRUE(columns[i].is_unsigned());      \
  } else {                                      \
    EXPECT_FALSE(columns[i].is_unsigned());     \
  }                                             \
  if (zf) {                                     \
    EXPECT_TRUE(columns[i].is_zerofill());      \
  } else {                                      \
    EXPECT_FALSE(columns[i].is_zerofill());     \
  }                                             \
  if (bin) {                                    \
    EXPECT_TRUE(columns[i].is_binary());        \
  } else {                                      \
    EXPECT_FALSE(columns[i].is_binary());       \
  }

TEST_F(Db_tests, metadata_columns_alltypes) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    ASSERT_NO_THROW(session->connect(Connection_options(uri())));
    {
      TABLE("t_tinyint");
      ASSERT_EQ(2, columns.size());
      CHECK(0, Type::Integer, 4, false, false, false);
      CHECK(1, Type::UInteger, 3, true, false, false);
    }

    {
      TABLE("t_smallint");
      ASSERT_EQ(2, columns.size());
      CHECK(0, Type::Integer, 6, false, false, false);
      CHECK(1, Type::UInteger, 5, true, false, false);
    }

    {
      TABLE("t_mediumint");
      ASSERT_EQ(2, columns.size());
      CHECK(0, Type::Integer, 9, false, false, false);
      CHECK(1, Type::UInteger, 8, true, false, false);
    }

    {
      TABLE("t_int");
      ASSERT_EQ(2, columns.size());
      CHECK(0, Type::Integer, 11, false, false, false);
      CHECK(1, Type::UInteger, 10, true, false, false);
    }

    {
      TABLE("t_bigint");
      ASSERT_EQ(2, columns.size());
      CHECK(0, Type::Integer, 20, false, false, false);
      CHECK(1, Type::UInteger, 20, true, false, false);
    }

    {
      TABLE("t_decimal1");
      ASSERT_EQ(2, columns.size());
      CHECK(0, Type::Decimal, 4, false, false, false);
      EXPECT_EQ(1, columns[0].get_fractional());
      CHECK(1, Type::Decimal, 3, true, false, false);
      EXPECT_EQ(1, columns[1].get_fractional());
    }

    {
      TABLE("t_float");
      ASSERT_EQ(2, columns.size());
      CHECK(0, Type::Float, 12, false, false, false);
      EXPECT_EQ(31, columns[0].get_fractional());
      CHECK(1, Type::Float, 12, true, false, false);
      EXPECT_EQ(31, columns[1].get_fractional());
    }

    {
      TABLE("t_double");
      ASSERT_EQ(2, columns.size());
      CHECK(0, Type::Double, 22, false, false, false);
      EXPECT_EQ(31, columns[0].get_fractional());
      CHECK(1, Type::Double, 22, true, false, false);
      EXPECT_EQ(31, columns[1].get_fractional());
    }

    {
      TABLE("t_date");
      ASSERT_EQ(5, columns.size());
      CHECK(0, Type::Date, 10, false, false, true);
      CHECK(1, Type::Time, 10, false, false, true);
      CHECK(2, Type::DateTime, 19, false, false, true);
      CHECK(3, Type::DateTime, 19, false, false, true);
      if (is_classic) {
        CHECK(4, Type::UInteger, 4, true, true, false);
      } else {
        PENDING_BUG_TEST("YEAR is zerofill, but xplugin doesn't set it");
        CHECK(4, Type::UInteger, 4, true, false, false);
      }
    }

    {
      TABLE("t_lob");
      ASSERT_EQ(12, columns.size());
      CHECK(0, Type::Bytes, 255, false, false, true);
      CHECK(1, Type::Bytes, 65535, false, false, true);
    }

    {
      TABLE("t_char");
      ASSERT_EQ(5, columns.size());
      CHECK(0, Type::String, 128, false, false, false);
      CHECK(1, Type::String, 128, false, false, false);
      if (is_classic) {
        CHECK(2, Type::String, 32, false, false, true);
        CHECK(3, Type::String, 32, false, false, true);
      } else {
        CHECK(2, Type::Bytes, 32, false, false, true);
        CHECK(3, Type::Bytes, 32, false, false, true);
      }
      if (is_classic) {
        // In classic, the length is for the data in UTF8
        CHECK(4, Type::String, 128, false, false, false);
      } else {
        // In xproto, the length is for the raw latin1 data (for now, at least)
        CHECK(4, Type::String, 32, false, false, false);
      }
    }

    {
      TABLE("t_bit");
      ASSERT_EQ(2, columns.size());
      CHECK(0, Type::Bit, 1, true, false, false);
      CHECK(1, Type::Bit, 64, true, false, false);
    }

    {
      TABLE("t_enum");
      ASSERT_EQ(2, columns.size());
      if (is_classic) {
        CHECK(0, Type::Enum, 8, false, false, false);
      } else {
          // X Protocol: SQL Type ENUM doesn't have .length
        CHECK(0, Type::Enum, 0, false, false, false);
      }
      CHECK(1, Type::Enum, 0, false, false, false);
    }

    {
      TABLE("t_set");
      ASSERT_EQ(2, columns.size());
      if (is_classic) {
        CHECK(0, Type::Set, 32, false, false, false);
      } else {
        // X Protocol: SQL Type SET doesn't have .length
        CHECK(0, Type::Set, 0, false, false, false);
      }
      CHECK(1, Type::Set, 0, false, false, false);
    }
  } while (switch_proto());
#undef CHECK
#undef TABLE
}

}  // namespace db
}  // namespace mysqlshdk
