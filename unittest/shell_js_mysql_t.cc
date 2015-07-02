/*
* Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "test_utils.h"
#include "base_session.h"

namespace shcore {
  class Shell_js_mysql_tests : public Shell_core_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_JScript, initilaized);
    }
  };

  TEST_F(Shell_js_mysql_tests, mysql_exports)
  {
    execute("print(shell.js.module_paths);");
    wipe_all();
    exec_and_out_equals("var mysql = require('mysql').mysql;");
    exec_and_out_equals("var exports = dir(mysql);");
    exec_and_out_equals("print(exports.length);", "1");

    exec_and_out_equals("print(typeof mysql.openSession);", "function");
  }

  TEST_F(Shell_js_mysql_tests, mysql_open_session_uri)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    // Assuming _uri is in the format user:password@host
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("var session = mysql.openSession('" + _mysql_uri + "');");
    exec_and_out_equals("print(session);", "<Session:" + uri + ">");

    // Ensures the right members exist
    exec_and_out_equals("var members = dir(session);");
    exec_and_out_equals("print(members.length >= 9)", "true");
    exec_and_out_equals("print(members[0] == 'executeSql');", "true");
    exec_and_out_equals("print(members[1] == 'getDefaultSchema');", "true");
    exec_and_out_equals("print(members[2] == 'getSchema');", "true");
    exec_and_out_equals("print(members[3] == 'getSchemas');", "true");
    exec_and_out_equals("print(members[4] == 'getUri');", "true");
    exec_and_out_equals("print(members[5] == 'setDefaultSchema');", "true");
    exec_and_out_equals("print(members[6] == 'defaultSchema');", "true");
    exec_and_out_equals("print(members[7] == 'schemas');", "true");
    exec_and_out_equals("print(members[8] == 'uri');", "true");
  }

  TEST_F(Shell_js_mysql_tests, mysql_open_session_uri_password)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    // Assuming _uri is in the format user:password@host
    int port = 3306, pwd_found;
    std::string protocol, user, password, host, sock, schema;
    mysh::parse_mysql_connstring(_mysql_uri, protocol, user, password, host, port, sock, schema, pwd_found);

    std::string uri = mysh::strip_password(_mysql_uri);

    if (!_pwd.empty())
      password = _pwd;

    exec_and_out_equals("var session = mysql.openSession('" + _mysql_uri + "', '" + password + "');");
    exec_and_out_equals("print(session);", "<Session:" + uri + ">");

    // Ensures the right members exist
    exec_and_out_equals("var members = dir(session);");
    exec_and_out_equals("print(members.length >= 9)", "true");
    exec_and_out_equals("print(members[0] == 'executeSql');", "true");
    exec_and_out_equals("print(members[1] == 'getDefaultSchema');", "true");
    exec_and_out_equals("print(members[2] == 'getSchema');", "true");
    exec_and_out_equals("print(members[3] == 'getSchemas');", "true");
    exec_and_out_equals("print(members[4] == 'getUri');", "true");
    exec_and_out_equals("print(members[5] == 'setDefaultSchema');", "true");
    exec_and_out_equals("print(members[6] == 'defaultSchema');", "true");
    exec_and_out_equals("print(members[7] == 'schemas');", "true");
    exec_and_out_equals("print(members[8] == 'uri');", "true");
  }

  TEST_F(Shell_js_mysql_tests, mysql_open_session_data)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    // Assuming _uri is in the format user:password@host
    int port = 3306, pwd_found;
    std::string protocol, user, password, host, sock, schema;
    mysh::parse_mysql_connstring(_mysql_uri, protocol, user, password, host, port, sock, schema, pwd_found);

    if (!_pwd.empty())
      password = _pwd;

    std::stringstream connection_data;
    connection_data << "{";
    connection_data << "\"host\": '" << host << "',";
    connection_data << "\"port\": " << port << ",";
    connection_data << "\"schema\": '" << schema << "',";
    connection_data << "\"dbUser\": '" << user << "',";
    connection_data << "\"dbPassword\": '" << password << "'";
    connection_data << "}";

    std::stringstream uri;
    uri << user << "@" << host << ":" << port;

    exec_and_out_equals("var session = mysql.openSession(" + connection_data.str() + ");");
    exec_and_out_equals("print(session);", "<Session:" + uri.str() + ">");

    // Ensures the right members exist
    exec_and_out_equals("var members = dir(session);");
    exec_and_out_equals("print(members.length >= 9)", "true");
    exec_and_out_equals("print(members[0] == 'executeSql');", "true");
    exec_and_out_equals("print(members[1] == 'getDefaultSchema');", "true");
    exec_and_out_equals("print(members[2] == 'getSchema');", "true");
    exec_and_out_equals("print(members[3] == 'getSchemas');", "true");
    exec_and_out_equals("print(members[4] == 'getUri');", "true");
    exec_and_out_equals("print(members[5] == 'setDefaultSchema');", "true");
    exec_and_out_equals("print(members[6] == 'defaultSchema');", "true");
    exec_and_out_equals("print(members[7] == 'schemas');", "true");
    exec_and_out_equals("print(members[8] == 'uri');", "true");
  }

  TEST_F(Shell_js_mysql_tests, mysql_open_session_data_password)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    // Assuming _uri is in the format user:password@host
    int port = 3306, pwd_found;
    std::string protocol, user, password, host, sock, schema;
    mysh::parse_mysql_connstring(_mysql_uri, protocol, user, password, host, port, sock, schema, pwd_found);

    if (!_pwd.empty())
      password = _pwd;

    std::stringstream connection_data;
    connection_data << "{";
    connection_data << "\"host\": '" << host << "',";
    connection_data << "\"port\": " << port << ",";
    connection_data << "\"schema\": '" << schema << "',";
    connection_data << "\"dbUser\": '" << user << "'";
    connection_data << "}";

    std::stringstream uri;
    uri << user << "@" << host << ":" << port;

    exec_and_out_equals("var session = mysql.openSession(" + connection_data.str() + ", '" + password + "');");
    exec_and_out_equals("print(session);", "<Session:" + uri.str() + ">");

    // Ensures the right members exist
    exec_and_out_equals("var members = dir(session);");
    exec_and_out_equals("print(members.length >= 9)", "true");
    exec_and_out_equals("print(members[0] == 'executeSql');", "true");
    exec_and_out_equals("print(members[1] == 'getDefaultSchema');", "true");
    exec_and_out_equals("print(members[2] == 'getSchema');", "true");
    exec_and_out_equals("print(members[3] == 'getSchemas');", "true");
    exec_and_out_equals("print(members[4] == 'getUri');", "true");
    exec_and_out_equals("print(members[5] == 'setDefaultSchema');", "true");
    exec_and_out_equals("print(members[6] == 'defaultSchema');", "true");
    exec_and_out_equals("print(members[7] == 'schemas');", "true");
    exec_and_out_equals("print(members[8] == 'uri');", "true");
  }
}