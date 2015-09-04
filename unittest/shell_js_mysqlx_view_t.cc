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

      exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

      exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");

      exec_and_out_equals("session.sql('drop schema if exists js_shell_test;').execute();");
      exec_and_out_equals("session.sql('create schema js_shell_test;').execute();");
      exec_and_out_equals("session.sql('use js_shell_test;').execute();");
      exec_and_out_equals("session.sql('create table table1 (name varchar(50));').execute();");
      exec_and_out_equals("session.sql('create view view1 (my_name) as select name from table1;').execute();");
    }
  };

  // Tests view.getName()
  TEST_F(Shell_js_mysqlx_view_tests, mysqlx_view_get_name)
  {
    exec_and_out_equals("var view = session.js_shell_test.view1;");

    exec_and_out_equals("print(view.getName());", "view1");
  }

  // Tests view.name
  TEST_F(Shell_js_mysqlx_view_tests, mysqlx_view_name)
  {
    exec_and_out_equals("var view = session.js_shell_test.view1;");

    exec_and_out_equals("print(view.name);", "view1");
  }

  // Tests view.getSession()
  TEST_F(Shell_js_mysqlx_view_tests, mysqlx_view_get_session)
  {
    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var view = session.js_shell_test.view1;");

    exec_and_out_equals("var view_session = view.getSession();");

    exec_and_out_equals("print(view_session)", "<NodeSession:" + uri + ">");
  }

  // Tests view.session
  TEST_F(Shell_js_mysqlx_view_tests, mysqlx_view_session)
  {
    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var view = session.js_shell_test.view1;");

    exec_and_out_equals("print(view.session)", "<NodeSession:" + uri + ">");
  }

  // Tests view.getSchema()
  TEST_F(Shell_js_mysqlx_view_tests, mysqlx_view_get_schema)
  {
    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var view = session.js_shell_test.view1;");

    exec_and_out_equals("var view_schema = view.getSchema();");

    exec_and_out_equals("print(view_schema)", "<Schema:js_shell_test>");
  }

  // Tests view.schema
  TEST_F(Shell_js_mysqlx_view_tests, mysqlx_view_schema)
  {
    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var view = session.js_shell_test.view1;");

    exec_and_out_equals("print(view.schema)", "<Schema:js_shell_test>");
  }

  // Tests view.drop() and view.existInDatabase()
  TEST_F(Shell_js_mysqlx_view_tests, mysqlx_view_drop_exist_in_database)
  {
    exec_and_out_equals("var schema = session.createSchema('my_sample_schema');");

    exec_and_out_equals("session.sql('create table my_sample_schema.my_sample_table (name varchar(50));').execute();");

    exec_and_out_equals("session.sql('create view my_sample_schema.my_sample_view (my_name) as select name from my_sample_schema.my_sample_table;').execute();");

    exec_and_out_equals("var view = schema.my_sample_view;");

    exec_and_out_equals("print(view.existInDatabase());", "true");

    exec_and_out_equals("view.drop();");

    exec_and_out_equals("print(view.existInDatabase());", "false");

    exec_and_out_equals("schema.drop();");

    exec_and_out_equals("session.close();");
  }
}