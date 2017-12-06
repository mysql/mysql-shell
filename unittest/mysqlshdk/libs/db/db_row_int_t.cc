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

TEST_F(Db_tests, row_getters_integer) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    ASSERT_NO_THROW(session->connect(Connection_options(uri())));
    {
      TABLE_ROW("t_integer", 2);

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

      CHECK_EQ(0, -2147483648LL, get_int);
      CHECK_EQ(1, 0, get_int);
      NEXT_ROW();
      CHECK_FAIL(0, get_uint);
      CHECK_EQ(0, -1, get_int);
      CHECK_EQ(1, 1, get_int);
      NEXT_ROW();
      CHECK_EQ(0, 0, get_int);
      CHECK_EQ(1, 2147483647, get_int);
      NEXT_ROW();
      CHECK_EQ(0, 1, get_int);
      CHECK_EQ(1, 4294967294, get_int);
      NEXT_ROW();
      CHECK_EQ(0, 2147483647, get_int);
      CHECK_EQ(1, 4294967295, get_int);
      NEXT_ROW();
      CHECK_NULL(0);
      CHECK_NULL(1);
      LAST_ROW();
    }
  } while (switch_proto());
}

}  // namespace db
}  // namespace mysqlshdk
