/* Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.

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

#ifndef UNITTEST_MYSQLSHDK_LIBS_DB_DB_COMMON_H_
#define UNITTEST_MYSQLSHDK_LIBS_DB_DB_COMMON_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/test_utils/shell_test_env.h"

namespace mysqlshdk {
namespace db {

class Db_tests : public tests::Shell_test_env {
 public:
  static bool first_test;
  bool is_classic = true;

  std::shared_ptr<mysqlshdk::db::ISession> session;

  std::shared_ptr<mysqlshdk::db::ISession> make_session();

  static void SetUpTestCase() {
    first_test = true;
  }

  static void TearDownTestCase();

  void SetUp();

  bool switch_proto();

  const std::string &uri() {
    return is_classic ? _mysql_uri : _uri;
  }

  const std::string &uri_nopass() {
    return is_classic ? _mysql_uri_nopasswd : _uri_nopasswd;
  }
};

#define TABLE_ROW(table, fields)                              \
  auto result = session->query("select * from xtest." table); \
  auto rowptr = result->fetch_one();                          \
  ASSERT_TRUE(rowptr);                                        \
  EXPECT_EQ(fields, rowptr->num_fields());                    \
  Row_copy rowcp(*rowptr);                                    \
  EXPECT_EQ(fields, rowcp.num_fields());

#define NEXT_ROW()              \
  rowptr = result->fetch_one(); \
  ASSERT_TRUE(rowptr);          \
  rowcp = Row_copy(*rowptr);

#define LAST_ROW()              \
  rowptr = result->fetch_one(); \
  ASSERT_FALSE(rowptr);

#define CHECK_EQ(f, value, getter)                       \
  EXPECT_FALSE(rowptr->is_null(f));                      \
  EXPECT_FALSE(rowcp.is_null(f));                        \
  EXPECT_EQ(value, rowptr->getter(f));                   \
  EXPECT_EQ(value, rowcp.getter(f));                     \
  EXPECT_EQ(shcore::str_strip(STRINGIFY(value), "UL\""), \
            rowptr->get_as_string(f));                   \
  EXPECT_EQ(shcore::str_strip(STRINGIFY(value), "UL\""), \
            rowcp.get_as_string(f));

#define CHECK_BIT_EQ(f, value, svalue)         \
  EXPECT_FALSE(rowptr->is_null(f));            \
  EXPECT_FALSE(rowcp.is_null(f));              \
  EXPECT_EQ(value, rowptr->get_bit(f));        \
  EXPECT_EQ(value, rowcp.get_bit(f));          \
  EXPECT_EQ(svalue, rowptr->get_as_string(f)); \
  EXPECT_EQ(svalue, rowcp.get_as_string(f));

#define CHECK_DOUBLE_EQ(f, value, getter)     \
  EXPECT_FALSE(rowptr->is_null(f));           \
  EXPECT_FALSE(rowcp.is_null(f));             \
  EXPECT_DOUBLE_EQ(value, rowptr->getter(f)); \
  EXPECT_DOUBLE_EQ(value, rowcp.getter(f));

#define CHECK_FLOAT_EQ(f, value, getter)     \
  EXPECT_FALSE(rowptr->is_null(f));          \
  EXPECT_FALSE(rowcp.is_null(f));            \
  EXPECT_FLOAT_EQ(value, rowptr->getter(f)); \
  EXPECT_FLOAT_EQ(value, rowcp.getter(f));

#define CHECK_STREQ(f, value, getter)                                         \
  EXPECT_FALSE(rowptr->is_null(f));                                           \
  EXPECT_EQ(std::string(value, sizeof(value) - 1), rowptr->getter(f));        \
  EXPECT_EQ(std::string(value, sizeof(value) - 1), rowptr->get_as_string(f)); \
  EXPECT_FALSE(rowcp.is_null(f));                                             \
  EXPECT_EQ(std::string(value, sizeof(value) - 1), rowcp.getter(f));          \
  EXPECT_EQ(std::string(value, sizeof(value) - 1), rowcp.get_as_string(f));   \
  {                                                                           \
    const char *data;                                                         \
    size_t length;                                                            \
    std::tie(data, length) = rowptr->get_string_data(f);                      \
    EXPECT_EQ(std::string(value, sizeof(value) - 1),                          \
              std::string(data, length));                                     \
    std::tie(data, length) = rowcp.get_string_data(f);                        \
    EXPECT_EQ(std::string(value, sizeof(value) - 1),                          \
              std::string(data, length));                                     \
  }

#define CHECK_FAIL(f, getter)                                   \
  {                                                             \
    SCOPED_TRACE("Row " STRINGIFY(getter));                     \
    EXPECT_THROW_LIKE(rowptr->getter(f), std::invalid_argument, \
                      STRINGIFY(getter));                       \
  }                                                             \
  {                                                             \
    SCOPED_TRACE("Row_copy " STRINGIFY(getter));                \
    EXPECT_THROW_LIKE(rowcp.getter(f), std::invalid_argument,   \
                      STRINGIFY(getter));                       \
  }

#define CHECK_FAIL_STRING(f)                                             \
  {                                                                      \
    SCOPED_TRACE("Row");                                                 \
    EXPECT_THROW_LIKE(rowptr->get_string(f), std::invalid_argument,      \
                      "get_string");                                     \
    EXPECT_THROW_LIKE(rowptr->get_string_data(f), std::invalid_argument, \
                      "get_string_data");                                \
  }                                                                      \
  {                                                                      \
    SCOPED_TRACE("Row_copy");                                            \
    EXPECT_THROW_LIKE(rowcp.get_string(f), std::invalid_argument,        \
                      "get_string");                                     \
    EXPECT_THROW_LIKE(rowcp.get_string_data(f), std::invalid_argument,   \
                      "get_string_data");                                \
  }

#define CHECK_FAIL_NUMBER(f)                                                   \
  {                                                                            \
    SCOPED_TRACE("Row");                                                       \
    EXPECT_THROW_LIKE(rowptr->get_int(f), std::invalid_argument, "get_int");   \
    EXPECT_THROW_LIKE(rowptr->get_uint(f), std::invalid_argument, "get_uint"); \
    EXPECT_THROW_LIKE(rowptr->get_float(f), std::invalid_argument,             \
                      "get_float");                                            \
    EXPECT_THROW_LIKE(rowptr->get_double(f), std::invalid_argument,            \
                      "get_double");                                           \
  }                                                                            \
  {                                                                            \
    SCOPED_TRACE("Row_copy");                                                  \
    EXPECT_THROW_LIKE(rowcp.get_int(f), std::invalid_argument, "get_int");     \
    EXPECT_THROW_LIKE(rowcp.get_uint(f), std::invalid_argument, "get_uint");   \
    EXPECT_THROW_LIKE(rowcp.get_float(f), std::invalid_argument, "get_float"); \
    EXPECT_THROW_LIKE(rowcp.get_double(f), std::invalid_argument,              \
                      "get_double");                                           \
  }

#define CHECK_FAIL_INT(f)                                                      \
  {                                                                            \
    SCOPED_TRACE("Row");                                                       \
    EXPECT_THROW_LIKE(rowptr->get_int(f), std::invalid_argument, "get_int");   \
    EXPECT_THROW_LIKE(rowptr->get_uint(f), std::invalid_argument, "get_uint"); \
  }                                                                            \
  {                                                                            \
    SCOPED_TRACE("Row_copy");                                                  \
    EXPECT_THROW_LIKE(rowcp.get_int(f), std::invalid_argument, "get_int");     \
    EXPECT_THROW_LIKE(rowcp.get_uint(f), std::invalid_argument, "get_uint");   \
  }

#define CHECK_FAIL_FP(f)                                                       \
  {                                                                            \
    SCOPED_TRACE("Row");                                                       \
    EXPECT_THROW_LIKE(rowptr->get_float(f), std::invalid_argument,             \
                      "get_float");                                            \
    EXPECT_THROW_LIKE(rowptr->get_double(f), std::invalid_argument,            \
                      "get_double");                                           \
  }                                                                            \
  {                                                                            \
    SCOPED_TRACE("Row_copy");                                                  \
    EXPECT_THROW_LIKE(rowcp.get_float(f), std::invalid_argument, "get_float"); \
    EXPECT_THROW_LIKE(rowcp.get_double(f), std::invalid_argument,              \
                      "get_double");                                           \
  }

#define CHECK_FAIL_ALL(f)                                                      \
  {                                                                            \
    SCOPED_TRACE("Row");                                                       \
    EXPECT_THROW_LIKE(rowptr->get_string(f), std::invalid_argument,            \
                      "get_string");                                           \
    EXPECT_THROW_LIKE(rowptr->get_string_data(f), std::invalid_argument,       \
                      "get_string_data");                                      \
    EXPECT_THROW_LIKE(rowptr->get_int(f), std::invalid_argument, "get_int");   \
    EXPECT_THROW_LIKE(rowptr->get_uint(f), std::invalid_argument, "get_uint"); \
    EXPECT_THROW_LIKE(rowptr->get_float(f), std::invalid_argument,             \
                      "get_float");                                            \
    EXPECT_THROW_LIKE(rowptr->get_double(f), std::invalid_argument,            \
                      "get_double");                                           \
    EXPECT_THROW_LIKE(rowptr->get_bit(f), std::invalid_argument, "get_bit");   \
  }                                                                            \
  {                                                                            \
    SCOPED_TRACE("Row_copy");                                                  \
    EXPECT_THROW_LIKE(rowcp.get_string(f), std::invalid_argument,              \
                      "get_string");                                           \
    EXPECT_THROW_LIKE(rowcp.get_string_data(f), std::invalid_argument,         \
                      "get_string_data");                                      \
    EXPECT_THROW_LIKE(rowcp.get_int(f), std::invalid_argument, "get_int");     \
    EXPECT_THROW_LIKE(rowcp.get_uint(f), std::invalid_argument, "get_uint");   \
    EXPECT_THROW_LIKE(rowcp.get_float(f), std::invalid_argument, "get_float"); \
    EXPECT_THROW_LIKE(rowcp.get_double(f), std::invalid_argument,              \
                      "get_double");                                           \
    EXPECT_THROW_LIKE(rowcp.get_bit(f), std::invalid_argument, "get_bit");     \
  }

#define CHECK_NULL(f)              \
  EXPECT_TRUE(rowptr->is_null(f)); \
  EXPECT_TRUE(rowcp.is_null(f));   \
  CHECK_FAIL_ALL(f)

#define CHECK_NOT_NULL(f)           \
  EXPECT_FALSE(rowptr->is_null(f)); \
  EXPECT_FALSE(rowcp.is_null(f));

}  // namespace db
}  // namespace mysqlshdk

#endif  // UNITTEST_MYSQLSHDK_LIBS_DB_DB_COMMON_H_
