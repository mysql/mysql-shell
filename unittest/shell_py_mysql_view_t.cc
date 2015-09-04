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
  class Shell_py_mysql_view_tests : public Shell_core_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_Python, initilaized);

      exec_and_out_equals("import mysql");

      exec_and_out_equals("session = mysql.getClassicSession('" + _mysql_uri + "')");

      exec_and_out_equals("session.sql('drop schema if exists js_shell_test;')");
      exec_and_out_equals("session.sql('create schema js_shell_test;')");
      exec_and_out_equals("session.sql('use js_shell_test;')");
      exec_and_out_equals("session.sql('create table table1 (name varchar(50));')");
      exec_and_out_equals("session.sql('create view view1 (my_name) as select name from table1;')");
    }
  };

  // Tests view.getName()
  TEST_F(Shell_py_mysql_view_tests, mysql_view_get_name)
  {
    exec_and_out_equals("view = session.js_shell_test.view1");

    exec_and_out_equals("print(view.getName())", "view1");
  }

  // Tests view.name
  TEST_F(Shell_py_mysql_view_tests, mysql_view_name)
  {
    exec_and_out_equals("view = session.js_shell_test.view1");

    exec_and_out_equals("print(view.name)", "view1");
  }

  // Tests view.getSession()
  TEST_F(Shell_py_mysql_view_tests, mysql_view_get_session)
  {
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("view = session.js_shell_test.view1");

    exec_and_out_equals("view_session = view.getSession()");

    exec_and_out_equals("print(view_session)", "<ClassicSession:" + uri + ">");
  }

  // Tests view.session
  TEST_F(Shell_py_mysql_view_tests, mysql_view_session)
  {
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("view = session.js_shell_test.view1");

    exec_and_out_equals("print(view.session)", "<ClassicSession:" + uri + ">");
  }

  // Tests view.getSchema()
  TEST_F(Shell_py_mysql_view_tests, mysql_view_get_schema)
  {
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("view = session.js_shell_test.view1");

    exec_and_out_equals("view_schema = view.getSchema()");

    exec_and_out_equals("print(view_schema)", "<ClassicSchema:js_shell_test>");
  }

  // Tests view.schema
  TEST_F(Shell_py_mysql_view_tests, mysql_view_schema)
  {
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("view = session.js_shell_test.view1");

    exec_and_out_equals("print(view.schema)", "<ClassicSchema:js_shell_test>");
  }

  // Tests view.drop() and view.existInDatabase()
  TEST_F(Shell_py_mysql_view_tests, mysql_view_drop_exist_in_database)
  {
    exec_and_out_equals("schema = session.createSchema('my_sample_schema')");

    exec_and_out_equals("session.sql('create table my_sample_schema.my_sample_table (name varchar(50));')");

    exec_and_out_equals("session.sql('create view my_sample_schema.my_sample_view (my_name) as select name from my_sample_schema.my_sample_table;')");

    exec_and_out_equals("view = schema.my_sample_view");

    exec_and_out_equals("print(view.existInDatabase())", "True");

    exec_and_out_equals("view.drop()");

    exec_and_out_equals("print(view.existInDatabase())", "False");

    exec_and_out_equals("schema.drop()");

    exec_and_out_equals("session.close();");
  }
}