/*
* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of the
* License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301  USA
*/


#include "gtest_clean.h"
#include "unittest/test_utils/shell_base_test.h"
#include "modules/mysql_connection.h"
#include "mysqlshdk/libs/db/ssl_info.h"

namespace tests {
class Mysql_connection_test : public Shell_base_test {
};

#ifdef _WIN32
TEST_F(Mysql_connection_test, connect_default_pipe){
  shcore::SslInfo info;
  std::shared_ptr<mysqlsh::mysql::Connection> connection
    (new mysqlsh::mysql::Connection(_host, _mysql_port_number,
                                    "", _user, _pwd, "", info));

  auto result = connection->run_sql("show variables like 'named_pipe'");
  auto row = result->fetch_one();
  std::string named_pipe = row->get_value_as_string(1);

  // Named pipes must be enabled
  ASSERT_EQ(named_pipe, "ON");

  result = connection->run_sql("show variables like 'socket'");
  row = result->fetch_one();
  named_pipe = row->get_value_as_string(1);

  connection->close();

  if (named_pipe.empty()) {
    SCOPED_TRACE("Named Pipe Connections are Disabled, they must be enabled.");
    FAIL();
  }
  else {
    try {
      // Test default named pipe connection using hostname = "."
      mysqlshdk::utils::Ssl_info info;
      mysqlsh::mysql::Connection pipe_conn(".", 0, "", _user, _pwd, "", info);
      pipe_conn.close();
    }
    catch (const std::exception& e) {
      if (named_pipe != "MySQL") {
        MY_EXPECT_OUTPUT_CONTAINS("Can't open named pipe to host: .  pipe: MySQL", e.what());
      } else {
        std::string error = "Failed default named pipe connection: ";
        error.append(e.what());
        SCOPED_TRACE(error);
        FAIL();
      }
    }
  }
}

TEST_F(Mysql_connection_test, connect_named_pipe){
  shcore::SslInfo info;
  std::shared_ptr<mysqlsh::mysql::Connection> connection
    (new mysqlsh::mysql::Connection(_host, _mysql_port_number,
    "", _user, _pwd, "", info));

  auto result = connection->run_sql("show variables like 'named_pipe'");
  auto row = result->fetch_one();
  std::string named_pipe = row->get_value_as_string(1);

  // Named pipes must be enabled
  ASSERT_EQ(named_pipe, "ON");

  result = connection->run_sql("show variables like 'socket'");
  row = result->fetch_one();
  named_pipe = row->get_value_as_string(1);

  connection->close();

  if (named_pipe.empty()) {
    SCOPED_TRACE("Named Pipe Connections are Disabled, they must be enabled.");
    FAIL();
  }
  else {
    try {
      // Test default named pipe connection using hostname = "."
      mysqlshdk::utils::Ssl_info info;
      mysqlsh::mysql::Connection pipe_conn("", 0, named_pipe, _user, _pwd, "",
                                           info);
      pipe_conn.close();
    }
    catch (const std::exception& e) {
      std::string error = "Failed default named pipe connection: ";
      error.append(e.what());
      SCOPED_TRACE(error);
      FAIL();
    }
  }
}

#else
TEST_F(Mysql_connection_test, connect_socket){
  mysqlshdk::utils::Ssl_info info;
  std::shared_ptr<mysqlsh::mysql::Connection> connection
      (new mysqlsh::mysql::Connection(_host,
                                      _mysql_port_number,
                                      "",
                                      _user,
                                      _pwd,
                                      "",
                                      info));

  auto result = connection->run_sql("show variables like 'socket'");

  auto row = result->fetch_one();

  std::string socket = row->get_value_as_string(1);

  connection->close();

  if (socket.empty()) {
    SCOPED_TRACE("Socket Connections are Disabled, they must be enabled.");
    FAIL();
  } else {
    try {
      mysqlshdk::utils::Ssl_info info;
      mysqlsh::mysql::Connection socket_conn("localhost", 0, socket, _user,
                                             _pwd, "", info);
      socket_conn.close();
    } catch (const std::exception& e) {
      std::string error = "Failed creating a socket connection using: '";
      error.append(socket);
      error.append("' error: ");
      error.append(e.what());
      SCOPED_TRACE(error);
      FAIL();
    }
  }
}
#endif

}
