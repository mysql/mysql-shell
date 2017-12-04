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

using Version = mysqlshdk::utils::Version;

namespace mysqlshdk {
namespace db {

bool Db_tests::first_test = true;

TEST_F(Db_tests, row_getters_date) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    ASSERT_NO_THROW(session->connect(Connection_options(uri())));
    {
      TABLE_ROW("t_date", 5);

      for (int i = 0; i < 4; i++) {
        CHECK_FAIL_NUMBER(i);
        CHECK_FAIL(i, get_bit);
        CHECK_NOT_NULL(i);
      }
      CHECK_FAIL_FP(4);
      CHECK_FAIL_STRING(4);
      CHECK_FAIL(4, get_bit);
      CHECK_NOT_NULL(4);
      CHECK_FAIL_ALL(10);
      CHECK_FAIL(10, is_null);

      CHECK_EQ(0, "2015-07-23", get_string);
      CHECK_EQ(1, "16:34:12", get_string);
      if (_target_server_version < Version("8.0.4")) {
        PENDING_BUG_TEST("BUG#27169735 DATETIME libmysqlxclient regression");
      } else {
        CHECK_EQ(2, "2015-07-23 16:34:12", get_string);
        CHECK_EQ(3, "2015-07-23 16:34:12", get_string);
      }
      CHECK_EQ(4, 2015, get_int);
      NEXT_ROW();
      CHECK_EQ(0, "0000-01-01", get_string);
      CHECK_EQ(1, "-01:00:00", get_string);
      if (_target_server_version < Version("8.0.4")) {
        PENDING_BUG_TEST("BUG#27169735 DATETIME libmysqlxclient regression");
      } else {
        CHECK_EQ(2, "2000-01-01 00:00:02", get_string);
        CHECK_EQ(3, "0000-01-01 00:00:00", get_string);
      }
      CHECK_EQ(4, 1999, get_int);
      NEXT_ROW();
      CHECK_NULL(0);
      CHECK_NULL(1);
      if (_target_server_version >= Version("8.0")) {
        // TIMESTAMP field in MySQL Server 5.7.21 is not null
        CHECK_NULL(2);
      }
      CHECK_FAIL_NUMBER(2);
      CHECK_NULL(3);
      CHECK_NULL(4);
      LAST_ROW();
    }
  } while (switch_proto());
}

TEST_F(Db_tests, row_getters_str) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    ASSERT_NO_THROW(session->connect(Connection_options(uri())));
    {
      TABLE_ROW("t_char", 5);

      for (int i = 0; i < 5; i++) {
        CHECK_FAIL_NUMBER(i);
        CHECK_FAIL(i, get_bit);
        CHECK_NOT_NULL(i);
      }
      CHECK_FAIL_ALL(10);
      CHECK_FAIL(10, is_null);

      // clang-format off
      CHECK_STREQ(0, "char", get_string);
      CHECK_STREQ(1, "varchar", get_string);
      CHECK_STREQ(2, "binary\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", get_string);  // NOLINT
      CHECK_STREQ(3, "varbinary", get_string);
      CHECK_STREQ(4, "accent", get_string);
      NEXT_ROW();
      if (is_classic) {
        CHECK_STREQ(0, "chár™€", get_string);
        CHECK_STREQ(1, "varchár™€", get_string);
        CHECK_STREQ(2, "bínary™€\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", get_string);  // NOLINT
        CHECK_STREQ(3, "varbínary™€", get_string);
      } else {
          if (_target_server_version < Version("8.0")) {
            PENDING_BUG_TEST("missing client side charset conversion to utf8");
          } else {
            CHECK_STREQ(0, "chár™€", get_string);
            CHECK_STREQ(1, "varchár™€", get_string);
            CHECK_STREQ(2, "bínary™€\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", get_string);  // NOLINT
            CHECK_STREQ(3, "varbínary™€", get_string);
          }
      }

      if (is_classic) {
        CHECK_STREQ(4, "bíñáry", get_string);
      } else {
        PENDING_BUG_TEST("missing client side charset conversion to utf8");
      }

      NEXT_ROW();
      if (is_classic) {
        CHECK_STREQ(0, "chár™€", get_string);
        CHECK_STREQ(1, "varchár™€", get_string);
        CHECK_STREQ(2, "bínary™€\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", get_string);  // NOLINT
        CHECK_STREQ(3, "varbínary™€", get_string);
      } else {
          if (_target_server_version < Version("8.0")) {
            PENDING_BUG_TEST("missing client side charset conversion to utf8");
          } else {
              CHECK_STREQ(0, "chár™€", get_string);
              CHECK_STREQ(1, "varchár™€", get_string);
              CHECK_STREQ(2, "bínary™€\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", get_string);  // NOLINT
              CHECK_STREQ(3, "varbínary™€", get_string);
          }
      }

      if (is_classic) {
        if (_target_server_version < Version("8.0")) {
          // expect latin-1 encoding
          CHECK_STREQ(4, "b\xED\xF1\xE1ry", get_string);
        } else {
          // expect utf-8 encoding
          CHECK_STREQ(4, "bíñáry", get_string);
        }
      } else {
        PENDING_BUG_TEST("missing client side charset conversion to utf8");
      }
      NEXT_ROW();
      if (is_classic) {
        CHECK_STREQ(0, "⚽️", get_string);
        CHECK_STREQ(1, "🏀", get_string);
        CHECK_STREQ(2, "🏈\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", get_string);  // NOLINT
        CHECK_STREQ(3, "⚾️", get_string);
        CHECK_STREQ(4, "", get_string);
      } else {
          if (_target_server_version < Version("8.0")) {
            PENDING_BUG_TEST("missing client side charset conversion to utf8");
          } else {
              CHECK_STREQ(0, "⚽️", get_string);
              CHECK_STREQ(1, "🏀", get_string);
              CHECK_STREQ(2, "🏈\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", get_string);  // NOLINT
              CHECK_STREQ(3, "⚾️", get_string);
              CHECK_STREQ(4, "", get_string);
          }
      }

      NEXT_ROW();
      CHECK_STREQ(0, "", get_string);
      CHECK_STREQ(1, "", get_string);
      CHECK_STREQ(2, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", get_string);  // NOLINT
      CHECK_STREQ(3, "", get_string);
      CHECK_STREQ(4, "", get_string);
      // clang-format on
      NEXT_ROW();
      for (int i = 0; i < 5; i++) {
        CHECK_NULL(i);
      }
      LAST_ROW();
    }

    {
      TABLE_ROW("t_lob", 12);

      for (int i = 0; i < 12; i++) {
        CHECK_FAIL_NUMBER(i);
        CHECK_FAIL(i, get_bit);
        CHECK_NOT_NULL(i);
      }
      for (int i = 0; i < 12; i++) {
        CHECK_STREQ(i, "", get_string);
      }
      NEXT_ROW();
      CHECK_STREQ(0, "tinyblob-text readable", get_string);
      CHECK_STREQ(1, "blob-text readable", get_string);
      CHECK_STREQ(2, "mediumblob-text readable", get_string);
      CHECK_STREQ(3, "longblob-text readable", get_string);
      CHECK_STREQ(4, "tinytext", get_string);
      CHECK_STREQ(5, "text", get_string);
      CHECK_STREQ(6, "mediumtext", get_string);
      CHECK_STREQ(7, "longtext", get_string);
      CHECK_STREQ(8, "tinytext-binary\nnext line", get_string);
      CHECK_STREQ(9, "text-binary\nnext line", get_string);
      CHECK_STREQ(10, "mediumtext-binary\nnext line", get_string);
      CHECK_STREQ(11, "longtext-binary \nnext line", get_string);
      NEXT_ROW();
      CHECK_NULL(0);
      CHECK_NULL(1);
      LAST_ROW();
    }
  } while (switch_proto());
}

TEST_F(Db_tests, row_getters_bit) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    ASSERT_NO_THROW(session->connect(Connection_options(uri())));
    {
      TABLE_ROW("t_bit", 2);

      CHECK_FAIL_STRING(0);
      CHECK_FAIL_NUMBER(0);
      CHECK_NOT_NULL(0);
      CHECK_FAIL_STRING(1);
      CHECK_FAIL_NUMBER(1);
      CHECK_NOT_NULL(1);

      // clang-format off
      CHECK_BIT_EQ(0, 0, std::string("0"));
      CHECK_BIT_EQ(1, 0, std::string("0000000000000000000000000000000000000000000000000000000000000000"));  // NOLINT(whitespace/line_length)
      NEXT_ROW();
      CHECK_BIT_EQ(0, 1, std::string("1"));
      CHECK_BIT_EQ(1, 1, std::string("0000000000000000000000000000000000000000000000000000000000000001"));  // NOLINT(whitespace/line_length)
      NEXT_ROW();
      CHECK_BIT_EQ(0, 1, std::string("1"));
      CHECK_BIT_EQ(1, 0xFFFFFFFFFFFFFFFFULL, std::string("1111111111111111111111111111111111111111111111111111111111111111"));  // NOLINT(whitespace/line_length)
      NEXT_ROW();
      CHECK_BIT_EQ(0, 1, std::string("1"));
      CHECK_BIT_EQ(1, 0x5555555555555555ULL, std::string("0101010101010101010101010101010101010101010101010101010101010101"));  // NOLINT(whitespace/line_length)
      // clang-format on
      NEXT_ROW();
      CHECK_NULL(0);
      CHECK_NULL(1);
      LAST_ROW();
    }
  } while (switch_proto());
}

}  // namespace db
}  // namespace mysqlshdk
