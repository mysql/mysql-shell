/* Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.

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

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/mysqlshdk/libs/db/db_common.h"
#include "unittest/test_utils.h"

namespace mysqlshdk {
namespace db {

std::shared_ptr<mysqlshdk::db::ISession> Db_tests::make_session() {
  if (is_classic)
    return mysqlshdk::db::mysql::Session::create();
  else
    return mysqlshdk::db::mysqlx::Session::create();
}

void Db_tests::TearDownTestCase() { run_script_classic({"drop schema xtest"}); }

void Db_tests::SetUp() {
  is_classic = true;
  Shell_test_env::SetUp();
  std::cout << "Testing classic\n";
  session = mysqlshdk::db::mysql::Session::create();

  if (first_test) {
    run_test_data_sql_file(_mysql_uri, "fieldtypes_all.sql");
    first_test = false;
  }
}

bool Db_tests::switch_proto() {
  if (is_classic) {
    is_classic = false;
    std::cout << "Testing X proto\n";
    session = mysqlshdk::db::mysqlx::Session::create();
    return true;
  }
  return false;
}

TEST_F(Db_tests, connect_uri) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    auto connection_options = shcore::get_connection_options(uri_nopass());
    connection_options.set_password("fake_pwd");

    // Connection failure
    EXPECT_THROW(session->connect(connection_options), std::exception);

    // Success connection, no schema selected
    {
      auto connection_options = shcore::get_connection_options(uri());

      ASSERT_NO_THROW(session->connect(connection_options));
      auto result = std::shared_ptr<mysqlshdk::db::IResult>(
          session->query("select database()"));
      auto row = result->fetch_one();
      EXPECT_TRUE(row->is_null(0));
      session->close();
    }

    // Success connection, default schema
    {
      auto connection_options =
          shcore::get_connection_options(uri() + "/mysql");
      ASSERT_NO_THROW(session->connect(connection_options));
      auto result = std::shared_ptr<mysqlshdk::db::IResult>(
          session->query("select database()"));
      auto row = result->fetch_one();
      EXPECT_FALSE(row->is_null(0));
      EXPECT_EQ("mysql", row->get_string(0));
      session->close();
    }
  } while (switch_proto());
}

TEST_F(Db_tests, execute) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    auto connection_options = shcore::get_connection_options(uri());
    EXPECT_NO_THROW(session->connect(connection_options));

    // Execute error, trying to use an unexisting schema
    EXPECT_THROW(session->execute("use some_weird_schema"), std::exception);

    // Execute success, use a valid schema
    EXPECT_NO_THROW(session->execute("use mysql"));

    session->close();
  } while (switch_proto());
}

TEST_F(Db_tests, query) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    auto connection_options = shcore::get_connection_options(uri());

    EXPECT_NO_THROW(session->connect(connection_options));

    // Query error, trying to retrieve an unexisting system variable
    EXPECT_THROW(session->query("select @@some_weird_variable"),
                 std::exception);

    // Query success, retrieving a valid system variable
    {
      auto result = std::shared_ptr<mysqlshdk::db::IResult>(
          session->query("select @@server_id"));
    }

    // Query success, retrieving a valid system variable, again
    {
      auto result = std::shared_ptr<mysqlshdk::db::IResult>(
          session->query("select @@server_id"));
    }

    // Query success, using buffered result
    {
      auto result = std::shared_ptr<mysqlshdk::db::IResult>(
          session->query(std::string("select @@server_id"), true));
    }

    // Required test database for the rest of this test
    EXPECT_NO_THROW(session->execute("drop schema if exists Db_sessions"));
    EXPECT_NO_THROW(session->execute("create schema Db_sessions"));
    EXPECT_NO_THROW(session->execute("use Db_sessions"));

    // Uses a stored procedure to test multi result behavior
    {
      std::string procedure =
          "create procedure test_query() begin select @@server_id; select "
          "@@server_uuid; end";
      EXPECT_NO_THROW(session->execute(procedure));
      auto result = session->query("call test_query()");
      EXPECT_TRUE(result->next_resultset());
      if (!is_classic) {
        EXPECT_TRUE(result->next_resultset());
      }
      EXPECT_FALSE(result->next_resultset());
    }

    // Pulls multiple results and ensures they are discarded
    // if a new execution occurs
    if (is_classic) {
      // TODO(alfredo) ensure common behaviour
      // at this layer, the pending resultset should not be discarded (or
      // cached) but rather throw an exception
      auto result = std::shared_ptr<mysqlshdk::db::IResult>(
          session->query("call test_query()"));
      result = std::shared_ptr<mysqlshdk::db::IResult>(
          session->query("select current_user()"));
      auto row = result->fetch_one();
      std::string user = _user + "@";
      EXPECT_EQ(row->get_string(0).find(user), 0);
    } else {
      PENDING_BUG_TEST(
          "Need to ensure proper behavior for non-consumed results");
    }

    EXPECT_NO_THROW(session->execute("drop schema Db_sessions"));

    session->close();
  } while (switch_proto());
}

TEST_F(Db_tests, auto_close) {
  do {
    SCOPED_TRACE(is_classic ? "mysql" : "mysqlx");
    ASSERT_NO_THROW(session->connect(Connection_options(uri())));

    auto result = session->query(
        "select count(*) from performance_schema.threads where "
        "PROCESSLIST_USER IS NOT NULL");
    int ocount = result->fetch_one()->get_int(0);

    {
      auto connection_options = shcore::get_connection_options(uri());
      auto mysql_session = make_session();
      EXPECT_NO_THROW(mysql_session->connect(connection_options));

      EXPECT_BECOMES_TRUE(
          3,
          (ocount + 1) ==
              session
                  ->query(
                      "select count(*) from performance_schema.threads where "
                      "PROCESSLIST_USER IS NOT NULL")
                  ->fetch_one_or_throw()
                  ->get_int(0));
    }

    bool ok = false;
    int count = 0;
    for (int i = 0; i < 10; i++) {
      count = session
                  ->query(
                      "select count(*) from performance_schema.threads where "
                      "PROCESSLIST_USER IS NOT NULL")
                  ->fetch_one()
                  ->get_int(0);
      if (count == ocount) {
        ok = true;
        break;
      }
      shcore::sleep_ms(1000);
    }
    if (!ok) {
      EXPECT_EQ(ocount, count);
    }
  } while (switch_proto());
}

TEST_F(Db_tests, connect_read_timeout) {
  auto connection_options = shcore::get_connection_options(uri());

  connection_options.set_unchecked("net-read-timeout", "1000");

  EXPECT_NO_THROW(session->connect(connection_options));

  EXPECT_NO_THROW(session->execute("SELECT SLEEP(0)"));

  EXPECT_THROW_LIKE(session->execute("SELECT SLEEP(2)"), mysqlshdk::db::Error,
                    "Lost connection to MySQL server during query");
}

}  // namespace db
}  // namespace mysqlshdk
