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
  class Shell_js_mysqlx_view_tests : public Shell_core_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_JScript, initilaized);

/*      exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

      exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");

      exec_and_out_equals("session.executeSql('drop schema if exists js_shell_test;')");
      exec_and_out_equals("session.executeSql('create schema js_shell_test;')");
      exec_and_out_equals("session.executeSql('use js_shell_test;')");
      exec_and_out_equals("session.executeSql('create table table1 (name varchar(50));')");
      exec_and_out_equals("session.executeSql('create view view1 (my_name) as select name from table1;')");*/
    }
  };

  TEST_F(Shell_js_mysqlx_view_tests, initialization)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");

    exec_and_out_equals("session.executeSql('drop schema if exists js_shell_test;')");
    exec_and_out_equals("session.executeSql('create schema js_shell_test;')");
    exec_and_out_equals("session.executeSql('use js_shell_test;')");
    exec_and_out_equals("session.executeSql('create table table1 (name varchar(50));')");
    exec_and_out_equals("session.executeSql('create view view1 (my_name) as select name from table1;')");
  }

  // Tests view.getName()
  TEST_F(Shell_js_mysqlx_view_tests, mysqlx_view_get_name)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.openSession('" + _uri + "');");

    exec_and_out_equals("var view = session.js_shell_test.view1;");

    exec_and_out_equals("print(view.getName());", "view1");
  }

  // Tests view.name
  TEST_F(Shell_js_mysqlx_view_tests, mysqlx_view_name)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.openSession('" + _uri + "');");

    exec_and_out_equals("var view = session.js_shell_test.view1;");

    exec_and_out_equals("print(view.name);", "view1");
  }

  // Tests view.getSession()
  TEST_F(Shell_js_mysqlx_view_tests, mysqlx_view_get_session)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var session = mysqlx.openSession('" + _uri + "');");

    exec_and_out_equals("var view = session.js_shell_test.view1;");

    exec_and_out_equals("var view_session = view.getSession();");

    exec_and_out_equals("print(view_session)", "<Session:" + uri + ">");
  }

  // Tests view.session
  TEST_F(Shell_js_mysqlx_view_tests, mysqlx_view_session)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var session = mysqlx.openSession('" + _uri + "');");

    exec_and_out_equals("var view = session.js_shell_test.view1;");

    exec_and_out_equals("print(view.session)", "<Session:" + uri + ">");
  }

  // Tests view.getSchema()
  TEST_F(Shell_js_mysqlx_view_tests, mysqlx_view_get_schema)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    std::string uri = mysh::strip_password(_uri);

     exec_and_out_equals("var session = mysqlx.openSession('" + _uri + "');");

    exec_and_out_equals("var view = session.js_shell_test.view1;");

    exec_and_out_equals("var view_schema = view.getSchema();");

    exec_and_out_equals("print(view_schema)", "<Schema:js_shell_test>");
  }

  // Tests view.schema
  TEST_F(Shell_js_mysqlx_view_tests, mysqlx_view_schema)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var session = mysqlx.openSession('" + _uri + "');");

    exec_and_out_equals("var view = session.js_shell_test.view1;");

    exec_and_out_equals("print(view.schema)", "<Schema:js_shell_test>");
  }
}