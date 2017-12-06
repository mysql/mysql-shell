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

TEST_F(Db_tests, row_decimal) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    ASSERT_NO_THROW(session->connect(Connection_options(uri())));
    {
      TABLE_ROW("t_decimal1", 2);

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

      CHECK_EQ(0, -1.1, get_double);
      CHECK_FLOAT_EQ(0, -1.1, get_float);
      CHECK_EQ(1, 0.0, get_double);
      CHECK_EQ(1, 0.0, get_float);
      NEXT_ROW();
      CHECK_FAIL(0, get_uint);
      CHECK_EQ(0, -9.9, get_double);
      CHECK_EQ(1, 9.8, get_double);
      NEXT_ROW();
      CHECK_EQ(0, 9.9, get_double);
      CHECK_EQ(1, 9.9, get_double);
      NEXT_ROW();
      CHECK_NULL(0);
      CHECK_NULL(1);
      LAST_ROW();
    }

    {
      TABLE_ROW("t_decimal2", 2);

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

      // clang-format off
      CHECK_DOUBLE_EQ(0, -1234567890123456789012345678901234.567890123456789012345678901234, get_double);  // NOLINT
      CHECK_EQ(0, "-1234567890123456789012345678901234.567890123456789012345678901234", get_as_string);  // NOLINT
      CHECK_DOUBLE_EQ(1, 1234567890123456789012345678901234.567890123456789012345678901234, get_double);  // NOLINT
      CHECK_EQ(1, "1234567890123456789012345678901234.567890123456789012345678901234", get_as_string);  // NOLINT
      NEXT_ROW();
      CHECK_DOUBLE_EQ(0, 9234567890123456789012345678901234.567890123456789012345678901234, get_double);  // NOLINT
      CHECK_DOUBLE_EQ(1, 9234567890123456789012345678901234.567890123456789012345678901234, get_double);  // NOLINT
      // clang-format on
      NEXT_ROW();
      CHECK_DOUBLE_EQ(0, 0, get_double);
      CHECK_DOUBLE_EQ(1, 0, get_double);
      LAST_ROW();
    }
  } while (switch_proto());
}

}  // namespace db
}  // namespace mysqlshdk
