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

TEST_F(Db_tests, row_getters_tinyint) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    ASSERT_NO_THROW(session->connect(Connection_options(uri())));

    {
      TABLE_ROW("t_tinyint", 2);

      CHECK_FAIL_STRING(0);
      CHECK_FAIL_FP(0);
      CHECK_FAIL(0, get_bit);
      CHECK_FAIL(0, get_uint);
      CHECK_NOT_NULL(0);
      CHECK_FAIL_STRING(1);
      CHECK_FAIL_FP(1);
      CHECK_FAIL(1, get_bit);
      CHECK_NOT_NULL(1);
      CHECK_FAIL_ALL(10);
      CHECK_FAIL(10, is_null);

      CHECK_EQ(0, -128, get_int);
      CHECK_EQ(1, 0, get_int);
      NEXT_ROW();
      CHECK_FAIL(0, get_uint);
      CHECK_EQ(0, -1, get_int);
      CHECK_EQ(1, 1, get_int);
      NEXT_ROW();
      CHECK_EQ(0, 0, get_int);
      CHECK_EQ(1, 127, get_int);
      NEXT_ROW();
      CHECK_EQ(0, 1, get_int);
      CHECK_EQ(1, 200, get_int);
      NEXT_ROW();
      CHECK_EQ(0, 127, get_int);
      CHECK_EQ(1, 255, get_int);
      NEXT_ROW();
      CHECK_NULL(0);
      CHECK_NULL(1);
      LAST_ROW();
    }
  } while (switch_proto());
}
}  // namespace db
}  // namespace mysqlshdk
