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

#include "unittest/test_utils/shell_base_test.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include <mysql.h>

namespace testing {

class Mysql_session_test : public tests::Shell_base_test {
protected:
  mysqlshdk::db::mysql::Session mysql_session;
  mysqlshdk::db::ISession *session;

  void SetUp() {
    Shell_base_test::SetUp();
    session = &mysql_session;
  }
};


TEST_F(Mysql_session_test, connect_uri) {

  // Connection failure
  EXPECT_THROW(session->connect(_mysql_uri_nopasswd, "fake_pwd"), std::exception);

  // Success connection, no schema selected
  {
    ASSERT_NO_THROW(session->connect(_mysql_uri));
    auto result = std::unique_ptr<mysqlshdk::db::IResult>(session->query("select database()"));
    auto row = std::unique_ptr<mysqlshdk::db::IRow>(result->fetch_one());
    EXPECT_TRUE(row->is_null(0));
    session->close();
  }

  // Success connection, default schema
  {
    ASSERT_NO_THROW(session->connect(_mysql_uri + "/mysql"));
    auto result = std::unique_ptr<mysqlshdk::db::IResult>(session->query("select database()"));
    auto row = std::unique_ptr<mysqlshdk::db::IRow>(result->fetch_one());
    EXPECT_FALSE(row->is_null(0));
    EXPECT_EQ("mysql", row->get_string(0));
    session->close();
  }
}

TEST_F(Mysql_session_test, connect_params) {

  // Connect failure
  mysqlshdk::utils::Ssl_info ssl_info;
  EXPECT_THROW(session->connect(_host, _mysql_port_number, "", _user, "fake_pwd", "", ssl_info), std::exception);

  // Success connection, no schema selected
  {
    ASSERT_NO_THROW(session->connect(_host, _mysql_port_number, "", _user, _pwd,
                                     "", ssl_info));
    auto result = std::unique_ptr<mysqlshdk::db::IResult>(session->query("select database()"));
    auto row = std::unique_ptr<mysqlshdk::db::IRow>(result->fetch_one());
    EXPECT_TRUE(row->is_null(0));
    session->close();
  }

  // Success connection, default schema
  {
    ASSERT_NO_THROW(session->connect(_host, _mysql_port_number, "", _user, _pwd,
                                     "mysql", ssl_info));
    auto result = std::unique_ptr<mysqlshdk::db::IResult>(session->query("select database()"));
    auto row = std::unique_ptr<mysqlshdk::db::IRow>(result->fetch_one());
    EXPECT_FALSE(row->is_null(0));
    EXPECT_EQ("mysql", row->get_string(0));
    session->close();
  }
}

TEST_F(Mysql_session_test, execute) {

  EXPECT_NO_THROW(session->connect(_mysql_uri));

  // Execute error, trying to use an unexisting schema
  EXPECT_THROW(session->execute("use some_weird_schema"), std::exception);

  // Execute success, use a valid schema
  EXPECT_NO_THROW(session->execute("use mysql"));

  session->close();
}

TEST_F(Mysql_session_test, query) {

  EXPECT_NO_THROW(session->connect(_mysql_uri));

  // Query error, trying to retrieve an unexisting system variable
  EXPECT_THROW(session->query("select @@some_weird_variable"), std::exception);

  // Query success, retrieving a valid system variable
  {auto result = std::unique_ptr<mysqlshdk::db::IResult>(session->query("select @@server_id"));}

  // Query success, retrieving a valid system variable, again
  {auto result = std::unique_ptr<mysqlshdk::db::IResult>(session->query("select @@server_id"));}

  // Query success, using buffered result
  {auto result = std::unique_ptr<mysqlshdk::db::IResult>(session->query("select @@server_id", true));}

  // Required test database for the rest of this test
  EXPECT_NO_THROW(session->execute("drop schema if exists mysql_session_tests"));
  EXPECT_NO_THROW(session->execute("create schema mysql_session_tests"));
  EXPECT_NO_THROW(session->execute("use mysql_session_tests"));

  // Uses a stored procedure to test multi result behavior
  {
    std::string procedure = "create procedure test_query() begin select @@server_id; select @@server_uuid; end";
    EXPECT_NO_THROW(session->execute(procedure));
    auto result = session->query("call test_query()");
    EXPECT_TRUE(result->next_data_set());
    EXPECT_FALSE(result->next_data_set());
  }

  // Pulls multiple results and ensures they are discarded
  // if a new execution occurs
  {
    auto result = std::unique_ptr<mysqlshdk::db::IResult>(session->query("call test_query()"));
    result = std::unique_ptr<mysqlshdk::db::IResult>(session->query("select current_user()"));
    auto row = std::unique_ptr<mysqlshdk::db::IRow>(result->fetch_one());
    std::string user = _user + "@";
    EXPECT_EQ(row->get_string(0).find(user), 0);
  }

  EXPECT_NO_THROW(session->execute("drop schema mysql_session_tests"));

  session->close();
}

TEST_F(Mysql_session_test, commit) {
  EXPECT_NO_THROW(session->connect(_mysql_uri));

  // Required test database for the rest of this test
  EXPECT_NO_THROW(session->execute("drop schema if exists mysql_session_tests"));
  EXPECT_NO_THROW(session->execute("create schema mysql_session_tests"));
  EXPECT_NO_THROW(session->execute("use mysql_session_tests"));
  EXPECT_NO_THROW(session->execute("create table test_commit (id int, name varchar(30))"));

  session->start_transaction();
  EXPECT_NO_THROW(session->execute("insert into test_commit values (1, \"jhonny\")"));
  EXPECT_NO_THROW(session->execute("insert into test_commit values (2, \"walker\")"));
  session->commit();

  auto result = std::unique_ptr<mysqlshdk::db::IResult>(session->query("select * from test_commit"));
  auto row = std::unique_ptr<mysqlshdk::db::IRow>(result->fetch_one());
  EXPECT_EQ(1, row->get_int(0));
  EXPECT_EQ("jhonny", row->get_string(1));

  row = std::unique_ptr<mysqlshdk::db::IRow>(result->fetch_one());
  EXPECT_EQ(2, row->get_int(0));
  EXPECT_EQ("walker", row->get_string(1));

  EXPECT_EQ(nullptr, result->fetch_one());

  EXPECT_NO_THROW(session->execute("drop schema mysql_session_tests"));

  session->close();
}

TEST_F(Mysql_session_test, rollback) {
  EXPECT_NO_THROW(session->connect(_mysql_uri));

  // Required test database for the rest of this test
  EXPECT_NO_THROW(session->execute("drop schema if exists mysql_session_tests"));
  EXPECT_NO_THROW(session->execute("create schema mysql_session_tests"));
  EXPECT_NO_THROW(session->execute("use mysql_session_tests"));
  EXPECT_NO_THROW(session->execute("create table test_rollback (id int, name varchar(30))"));

  session->start_transaction();
  EXPECT_NO_THROW(session->execute("insert into test_rollback values (1, \"jhonny\")"));
  EXPECT_NO_THROW(session->execute("insert into test_rollback values (2, \"walker\")"));
  session->rollback();

  auto result = std::unique_ptr<mysqlshdk::db::IResult>(session->query("select * from test_rollback"));
  EXPECT_EQ(nullptr, result->fetch_one());

  EXPECT_NO_THROW(session->execute("drop schema mysql_session_tests"));

  session->close();
}

TEST_F(Mysql_session_test, auto_close) {
  {
    mysqlshdk::db::mysql::Session mysql_session;
    EXPECT_NO_THROW(session->connect(_mysql_uri));
  }
}

}
