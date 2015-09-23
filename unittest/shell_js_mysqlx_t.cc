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
  class Shell_js_mysqlx_tests : public Shell_core_test_wrapper
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

  TEST_F(Shell_js_mysqlx_tests, mysqlx_exports)
  {
    execute("print(shell.js.module_paths);");
    wipe_all();
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");
    exec_and_out_equals("var exports = dir(mysqlx);");
    exec_and_out_equals("print(exports.length);", "2");

    exec_and_out_equals("print(typeof mysqlx.getSession);", "function");
    exec_and_out_equals("print(typeof mysqlx.getNodeSession);", "function");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_open_session_uri)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var session = mysqlx.getSession('" + _uri + "');");
    exec_and_out_equals("print(session);", "<XSession:" + uri + ">");
    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_open_session_uri_password)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    int port = 3306, pwd_found;
    std::string protocol, user, password, host, sock, schema, ssl_ca, ssl_cert, ssl_key;
    mysh::parse_mysql_connstring(_uri, protocol, user, password, host, port, sock, schema, pwd_found, ssl_ca, ssl_cert, ssl_key);

    std::string uri = mysh::strip_password(_uri);

    if (!_pwd.empty())
      password = _pwd;

    exec_and_out_equals("var session = mysqlx.getSession('" + _uri + "', '" + password + "');");
    exec_and_out_equals("print(session);", "<XSession:" + uri + ">");
    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_open_session_data)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    int port = 33060, pwd_found;
    std::string protocol, user, password, host, sock, schema, ssl_ca, ssl_cert, ssl_key;
    mysh::parse_mysql_connstring(_uri, protocol, user, password, host, port, sock, schema, pwd_found, ssl_ca, ssl_cert, ssl_key);

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

    exec_and_out_equals("var session = mysqlx.getSession(" + connection_data.str() + ");");
    exec_and_out_equals("print(session);", "<XSession:" + uri.str() + ">");
    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_open_session_data_password)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    int port = 33060, pwd_found;
    std::string protocol, user, password, host, sock, schema, ssl_ca, ssl_cert, ssl_key;
    mysh::parse_mysql_connstring(_uri, protocol, user, password, host, port, sock, schema, pwd_found, ssl_ca, ssl_cert, ssl_key);

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

    exec_and_out_equals("var session = mysqlx.getSession(" + connection_data.str() + ", '" + password + "');");
    exec_and_out_equals("print(session);", "<XSession:" + uri.str() + ">");
    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_open_node_session_uri)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    std::string uri = mysh::strip_password(_uri);
    exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");
    exec_and_out_equals("print(session);", "<NodeSession:" + uri + ">");
    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_open_node_session_uri_password)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    int port = 3306, pwd_found;
    std::string protocol, user, password, host, sock, schema, ssl_ca, ssl_cert, ssl_key;
    mysh::parse_mysql_connstring(_uri, protocol, user, password, host, port, sock, schema, pwd_found, ssl_ca, ssl_cert, ssl_key);

    std::string uri = mysh::strip_password(_uri);

    if (!_pwd.empty())
      password = _pwd;

    exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "', '" + password + "');");
    exec_and_out_equals("print(session);", "<NodeSession:" + uri + ">");
    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_open_node_session_data)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    int port = 33060, pwd_found;
    std::string protocol, user, password, host, sock, schema, ssl_ca, ssl_cert, ssl_key;
    mysh::parse_mysql_connstring(_uri, protocol, user, password, host, port, sock, schema, pwd_found, ssl_ca, ssl_cert, ssl_key);

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

    exec_and_out_equals("var session = mysqlx.getNodeSession(" + connection_data.str() + ");");
    exec_and_out_equals("print(session);", "<NodeSession:" + uri.str() + ">");
    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_open_node_session_data_password)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    int port = 33060, pwd_found;
    std::string protocol, user, password, host, sock, schema, ssl_ca, ssl_cert, ssl_key;
    mysh::parse_mysql_connstring(_uri, protocol, user, password, host, port, sock, schema, pwd_found, ssl_ca, ssl_cert, ssl_key);

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

    exec_and_out_equals("var session = mysqlx.getNodeSession(" + connection_data.str() + ", '" + password + "');");
    exec_and_out_equals("print(session);", "<NodeSession:" + uri.str() + ">");
    exec_and_out_equals("session.close();");
  }
}