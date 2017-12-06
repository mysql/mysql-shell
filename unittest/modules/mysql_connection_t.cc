/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "gtest_clean.h"
#include "unittest/test_utils/shell_base_test.h"
#include "mysqlshdk/libs/db/mysql/session.h"

namespace tests {

class Mysql_connection_test : public Shell_base_test {};

#ifdef _WIN32
TEST_F(Mysql_connection_test, connect_default_pipe) {
  mysqlshdk::db::Connection_options connection_options;
  connection_options.set_host(_host);
  connection_options.set_port(_mysql_port_number);
  connection_options.set_user(_user);
  connection_options.set_password(_pwd);
  auto connection = mysqlshdk::db::mysql::Session::create();

  connection->connect(connection_options);

  auto result = connection->query("show variables like 'named_pipe'");
  auto row = result->fetch_one();
  std::string named_pipe = row->get_as_string(1);

  // Named pipes must be enabled
  ASSERT_EQ(named_pipe, "ON");

  result = connection->query("show variables like 'socket'");
  row = result->fetch_one();
  named_pipe = row->get_as_string(1);

  connection->close();

  if (named_pipe.empty()) {
    SCOPED_TRACE("Named Pipe Connections are Disabled, they must be enabled.");
    FAIL();
  } else {
    try {
      // Test default named pipe connection using hostname = "."
      mysqlshdk::db::Connection_options connection_options;
      connection_options.set_host(".");
      connection_options.set_user(_user);
      connection_options.set_password(_pwd);
      auto pipe_conn = mysqlshdk::db::mysql::Session::create();
      pipe_conn->connect(connection_options);
      pipe_conn->close();
    } catch (const std::exception& e) {
      if (named_pipe != "MySQL") {
        MY_EXPECT_OUTPUT_CONTAINS(
            "Can't open named pipe to host: .  pipe: MySQL", e.what());
      } else {
        std::string error = "Failed default named pipe connection: ";
        error.append(e.what());
        SCOPED_TRACE(error);
        FAIL();
      }
    }
  }
}

TEST_F(Mysql_connection_test, connect_named_pipe) {
  mysqlshdk::db::Connection_options connection_options;
  connection_options.set_host(_host);
  connection_options.set_port(_mysql_port_number);
  connection_options.set_user(_user);
  connection_options.set_password(_pwd);

  auto connection = mysqlshdk::db::mysql::Session::create();
  connection->connect(connection_options);

  auto result = connection->query("show variables like 'named_pipe'");
  auto row = result->fetch_one();
  std::string named_pipe = row->get_as_string(1);

  // Named pipes must be enabled
  ASSERT_EQ(named_pipe, "ON");

  result = connection->query("show variables like 'socket'");
  row = result->fetch_one();
  named_pipe = row->get_as_string(1);

  connection->close();

  if (named_pipe.empty()) {
    SCOPED_TRACE("Named Pipe Connections are Disabled, they must be enabled.");
    FAIL();
  } else {
    try {
      // Test default named pipe connection using hostname = "."
      mysqlshdk::db::Connection_options connection_options;
      connection_options.set_socket(named_pipe);
      connection_options.set_user(_user);
      connection_options.set_password(_pwd);
      auto pipe_conn = mysqlshdk::db::mysql::Session::create();
      pipe_conn->connect(connection_options);
      pipe_conn->close();
    } catch (const std::exception& e) {
      std::string error = "Failed default named pipe connection: ";
      error.append(e.what());
      SCOPED_TRACE(error);
      FAIL();
    }
  }
}

#else
TEST_F(Mysql_connection_test, connect_socket) {
  if (_mysql_socket.empty()) {
    SCOPED_TRACE("Socket Connections are Disabled, they must be enabled.");
    FAIL();
  } else {
    try {
      mysqlshdk::db::Connection_options connection_options;
      connection_options.set_user(_user);
      connection_options.set_password(_pwd);
      connection_options.set_socket(_mysql_socket);
      auto socket_conn = mysqlshdk::db::mysql::Session::create();
      socket_conn->connect(connection_options);
      socket_conn->close();
    } catch (const std::exception& e) {
      std::string error = "Failed creating a socket connection using: '";
      error.append(_mysql_socket);
      error.append("' error: ");
      error.append(e.what());
      SCOPED_TRACE(error);
      FAIL();
    }
  }
}
#endif

}  // namespace tests
