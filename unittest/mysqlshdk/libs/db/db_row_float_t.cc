/* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

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

#include "unittest/mysqlshdk/libs/db/db_common.h"

namespace mysqlshdk {
namespace db {

TEST_F(Db_tests, row_float) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    ASSERT_NO_THROW(session->connect(Connection_options(uri())));
    {
      TABLE_ROW("t_float", 2);

      CHECK_FAIL_STRING(0);
      CHECK_FAIL_INT(0);
      CHECK_FAIL(0, get_bit);
      CHECK_NOT_NULL(0);
      CHECK_FAIL_STRING(1);
      CHECK_FAIL_INT(1);
      CHECK_FAIL(1, get_bit);
      CHECK_NOT_NULL(1);
      CHECK_FAIL_ALL(10);
      CHECK_FAIL(10, is_null);

      // expected values taken from mysql cli output
      if (is_classic) {
        CHECK_FLOAT_EQ(0, -1220220, get_float);
      } else {
        CHECK_FLOAT_EQ(0, -1220223, get_float);
      }
      CHECK_FLOAT_EQ(1, 0.0001, get_float);
      if (is_classic) {
        CHECK_DOUBLE_EQ(0, -1220220, get_double);
      } else {
        CHECK_DOUBLE_EQ(0, -1220223, get_double);
      }
      CHECK_FLOAT_EQ(1, 0.0001, get_double);
      NEXT_ROW();
      CHECK_FLOAT_EQ(0, -1.02323, get_float);
      CHECK_FLOAT_EQ(1, 1.2333, get_float);
      NEXT_ROW();
      if (is_classic) {
        CHECK_FLOAT_EQ(0, 123523, get_float);
        CHECK_FLOAT_EQ(1, 112353, get_float);
      } else {
        CHECK_FLOAT_EQ(0, 123522.67, get_float);
        CHECK_FLOAT_EQ(1, 112352.67, get_float);
      }
      NEXT_ROW();
      CHECK_NULL(0);
      CHECK_NULL(1);
      LAST_ROW();
    }

    {
      TABLE_ROW("t_double", 2);

      CHECK_FAIL_STRING(0);
      CHECK_FAIL_INT(0);
      CHECK_FAIL(0, get_bit);
      CHECK_NOT_NULL(0);
      CHECK_FAIL_STRING(1);
      CHECK_FAIL_INT(1);
      CHECK_FAIL(1, get_bit);
      CHECK_NOT_NULL(1);
      CHECK_FAIL_ALL(10);
      CHECK_FAIL(10, is_null);

      CHECK_DOUBLE_EQ(0, -122022323.0230221, get_double);
      CHECK_FLOAT_EQ(0, -122022323.0230221, get_float);
      CHECK_DOUBLE_EQ(1, 2320.0012301, get_double);
      CHECK_FLOAT_EQ(1, 2320.0012301, get_float);
      NEXT_ROW();
      CHECK_DOUBLE_EQ(0, -1.232023231, get_double);
      CHECK_DOUBLE_EQ(1, 1231231231.23331231, get_double);
      NEXT_ROW();
      CHECK_DOUBLE_EQ(0, 1235212322.6123123, get_double);
      CHECK_DOUBLE_EQ(1, 11235212312322.671, get_double);
      NEXT_ROW();
      CHECK_NULL(0);
      CHECK_NULL(1);
      LAST_ROW();
    }
  } while (switch_proto());
}

}  // namespace db
}  // namespace mysqlshdk
