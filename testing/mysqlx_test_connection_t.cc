/* Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.

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

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <gtest/gtest.h>

#include "../mysqlxtest/mysqlx_test_connector.h"

namespace shcore
{
  namespace mysqlx_test_connector_tests
  {
    struct Environment
    {
      std::string protocol;
      std::string host;
      std::string port;
      std::string user;
      std::string password;
      std::string sock;
      std::string db;

      boost::shared_ptr<mysqlx::Mysqlx_test_connector> conn;
    } env;

    TEST(Mysqlx_test_connection, connection)
    {
      const char *uri = getenv("MYSQL_URI");
      const char *pwd = getenv("MYSQL_PWD");
      std::string x_uri;

      int port, pwd_found = 0;

      mysqlx::parse_mysql_connstring(uri, env.protocol, env.user, env.password, env.host, port, env.sock, env.db, pwd_found);
      if (pwd)
        env.password.assign(pwd);

      // Connection Error (using invalid port)
      EXPECT_THROW(env.conn.reset(new mysqlx::Mysqlx_test_connector(env.host, "45632")), boost::system::system_error);

      // Connection will now succeed.
      EXPECT_NO_THROW(env.conn.reset(new mysqlx::Mysqlx_test_connector(env.host, "33060")));
    }

    TEST(Mysqlx_test_connection, sample_authenticate_failure)
    {
      // Sent over the connection opened on the previous test.
      Mysqlx::Session::AuthenticateStart m;

      m.set_mech_name("plain");
      m.set_auth_data(env.user);
      m.set_initial_response("a_fake_password");
      env.conn->send_message(&m);

      int response_message_id;
      Message *response = env.conn->read_response(response_message_id);

      EXPECT_EQ(Mysqlx::ServerMessages_Type_SESS_AUTH_FAIL, response_message_id);
      if (response)
      {
        // AuthenticateFail as defined in mysqlx_session.proto:
        //
        // message AuthenticateFail{
        //   optional string msg = 1;
        // }
        Mysqlx::Session::AuthenticateFail *failure = dynamic_cast<Mysqlx::Session::AuthenticateFail *>(response);

        // the msg attribute in the protocol definition becomes a method on the message implementation
        EXPECT_EQ("Invalid user or password", failure->msg());
        delete response;
      }

      // At this moment the connection is closed.
    }

    TEST(Mysqlx_test_connection, sample_authenticate_success)
    {
      // Reconnects to attempt the authentication failure.
      EXPECT_NO_THROW(env.conn.reset(new mysqlx::Mysqlx_test_connector(env.host, "33060")));

      Mysqlx::Session::AuthenticateStart m;

      m.set_mech_name("plain");
      m.set_auth_data(env.user);
      m.set_initial_response(env.password);
      env.conn->send_message(&m);

      int response_message_id;
      Message *response = env.conn->read_response(response_message_id);

      EXPECT_EQ(Mysqlx::ServerMessages_Type_SESS_AUTH_OK, response_message_id);
      if (response)
      {
        // AuthenticateOk as defined in mysqlx_session.proto:
        //
        // message AuthenticateOk{
        //   optional bytes auth_data = 1; // authentication data
        // }
        Mysqlx::Session::AuthenticateOk *success = dynamic_cast<Mysqlx::Session::AuthenticateOk *>(response);

        // the auth_data attribute in the protocol definition becomes a method on the message implementation
        EXPECT_EQ("welcome!", success->auth_data());
        delete response;
      }
    }
  }
}