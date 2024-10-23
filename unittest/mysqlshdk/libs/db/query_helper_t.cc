/* Copyright (c) 2017, 2024, Oracle and/or its affiliates.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2.0,
 as published by the Free Software Foundation.

 This program is designed to work with certain software (including
 but not limited to OpenSSL) that is licensed under separate terms,
 as designated in a particular file or component or in included license
 documentation.  The authors of MySQL hereby grant you an additional
 permission to link the program and your derivative works with the
 separately licensed software that they have either included with
 the program or referenced in the documentation.

 This program is distributed in the hope that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 the GNU General Public License, version 2.0, for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include <ranges>

#include "mysqlshdk/libs/db/query_helper.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/mysqlshdk/libs/db/db_common.h"

namespace mysqlshdk {
namespace db {

TEST_F(Db_tests, query_helper_bug37207914) {
  ASSERT_NO_THROW(session->connect(Connection_options(uri())));

  ASSERT_NO_THROW(session->execute("create schema if not exists testdb"));
  ASSERT_NO_THROW(session->execute("use testdb;"));
  auto cleanup =
      shcore::on_leave_scope([&]() { session->execute("drop schema testdb"); });

  ASSERT_NO_THROW(session->execute(
      "create table test_table(test_column_1 integer, test_column_2 int);"));
  ASSERT_NO_THROW(session->execute(
      "create table test_table_2(test_column_1 integer, test_column_2 int);"));
  ASSERT_NO_THROW(session->execute(
      "create trigger test_trigger_1 AFTER INSERT on test_table_2 FOR "
      "EACH ROW delete from Clone where test_column_1<0;"));
  ASSERT_NO_THROW(session->execute("create view test_view as select now();"));
  ASSERT_NO_THROW(session->execute(
      "create view test_view_2 as select * from test_table_2;"));
  ASSERT_NO_THROW(session->execute(
      "CREATE FUNCTION test_func (s CHAR(20)) RETURNS CHAR(50) "
      "DETERMINISTIC RETURN CONCAT('Hello, ',s,'!');"));
  ASSERT_NO_THROW(session->execute(
      "CREATE EVENT test_event ON SCHEDULE AT CURRENT_TIMESTAMP + INTERVAL 1 "
      "HOUR DO UPDATE test_table_2 SET test_column_1 = test_column_1 + 1;"));

  auto filters = Filtering_options();

  EXPECT_NO_THROW(filters.schemas().include("testdb"));
  EXPECT_NO_THROW(filters.tables().include("testdb.test_table_2"));
  EXPECT_NO_THROW(filters.tables().include("testdb.test_view"));
  EXPECT_NO_THROW(
      filters.triggers().include("testdb.test_table_2.test_trigger_1"));
  EXPECT_NO_THROW(filters.routines().include("testdb.test_func"));
  EXPECT_NO_THROW(filters.events().include("testdb.test_event"));

  {
    auto qh = Query_helper(filters);

    EXPECT_NO_THROW(session->execute(
        "SELECT ROUTINE_SCHEMA, ROUTINE_NAME"
        " FROM information_schema.routines WHERE ROUTINE_TYPE = 'PROCEDURE'"
        " AND " +
        qh.schema_and_routine_filter()));
    EXPECT_NO_THROW(session->execute(
        "SELECT ROUTINE_SCHEMA, ROUTINE_NAME"
        " FROM information_schema.routines WHERE ROUTINE_TYPE = 'FUNCTION'"
        " AND " +
        qh.schema_and_routine_filter()));
    EXPECT_NO_THROW(
        session->execute("SELECT TRIGGER_SCHEMA, TRIGGER_NAME"
                         " FROM information_schema.triggers"
                         " WHERE " +
                         qh.schema_and_trigger_filter()));
    EXPECT_NO_THROW(
        session->execute("SELECT EVENT_SCHEMA, EVENT_NAME"
                         " FROM information_schema.events"
                         " WHERE " +
                         qh.schema_and_event_filter()));
  }

  filters = Filtering_options();

  EXPECT_NO_THROW(filters.schemas().exclude("testdb"));
  EXPECT_NO_THROW(filters.tables().exclude("testdb.test_table_2"));
  EXPECT_NO_THROW(filters.tables().exclude("testdb.test_view"));
  EXPECT_NO_THROW(
      filters.triggers().exclude("testdb.test_table_2.test_trigger_1"));
  EXPECT_NO_THROW(filters.routines().exclude("testdb.test_func"));
  EXPECT_NO_THROW(filters.events().exclude("testdb.test_event"));

  {
    auto qh = Query_helper(filters);

    EXPECT_NO_THROW(session->execute(
        "SELECT ROUTINE_SCHEMA, ROUTINE_NAME"
        " FROM information_schema.routines WHERE ROUTINE_TYPE = 'PROCEDURE'"
        " AND " +
        qh.schema_and_routine_filter()));
    EXPECT_NO_THROW(session->execute(
        "SELECT ROUTINE_SCHEMA, ROUTINE_NAME"
        " FROM information_schema.routines WHERE ROUTINE_TYPE = 'FUNCTION'"
        " AND " +
        qh.schema_and_routine_filter()));
    EXPECT_NO_THROW(
        session->execute("SELECT TRIGGER_SCHEMA, TRIGGER_NAME"
                         " FROM information_schema.triggers"
                         " WHERE " +
                         qh.schema_and_trigger_filter()));
    EXPECT_NO_THROW(
        session->execute("SELECT EVENT_SCHEMA, EVENT_NAME"
                         " FROM information_schema.events"
                         " WHERE " +
                         qh.schema_and_event_filter()));
  }
}
}  // namespace db
}  // namespace mysqlshdk
