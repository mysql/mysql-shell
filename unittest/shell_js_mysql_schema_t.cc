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
  class Shell_js_mysql_schema_tests : public Shell_core_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_JScript, initilaized);

      exec_and_out_equals("var mysql = require('mysql').mysql;");

      exec_and_out_equals("var session = mysql.openSession('" + _mysql_uri + "');");

      exec_and_out_equals("session.executeSql('drop schema if exists js_shell_test;')");
      exec_and_out_equals("session.executeSql('create schema js_shell_test;')");
      exec_and_out_equals("session.executeSql('use js_shell_test;')");
      exec_and_out_equals("var result = session.executeSql('create table table1 (name varchar(50));')");
      exec_and_out_equals("session.executeSql('create view view1 (my_name) as select name from table1;')");

    }
  };

  // Tests schema.getName()
  TEST_F(Shell_js_mysql_schema_tests, mysql_schema_get_name)
  {
    exec_and_out_equals("print(session.mysql.getName());", "mysql");
  }

  // Tests schema.name
  TEST_F(Shell_js_mysql_schema_tests, mysql_schema_name)
  {
    exec_and_out_equals("print(session.mysql.name);", "mysql");
  }

  // Tests schema.getSession()
  TEST_F(Shell_js_mysql_schema_tests, mysql_schema_get_session)
  {
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("var schema = session.mysql;");

    exec_and_out_equals("var schema_session = schema.getSession();");

    exec_and_out_equals("print(schema_session)", "<Session:" + uri + ">");
  }

  // Tests schema.session
  TEST_F(Shell_js_mysql_schema_tests, mysql_schema_session)
  {
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("var schema = session.mysql;");

    exec_and_out_equals("print(schema.session)", "<Session:" + uri + ">");
  }

  // Tests schema.getSchema()
  TEST_F(Shell_js_mysql_schema_tests, mysql_schema_get_schema)
  {
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("var schema = session.mysql;");

    exec_and_out_equals("var schema_schema = schema.getSchema();");

    exec_and_out_equals("print(schema_schema)", "null");
  }

  // Tests schema.schema
  TEST_F(Shell_js_mysql_schema_tests, mysql_schema_schema)
  {
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("var schema = session.mysql;");

    exec_and_out_equals("print(schema.schema)", "null");
  }

  // Tests schema.getTables()
  TEST_F(Shell_js_mysql_schema_tests, mysql_schema_get_tables)
  {
    exec_and_out_equals("var tables = session.js_shell_test.getTables();");

    exec_and_out_equals("print(tables.table1)", "<Table:table1>");
  }

  // Tests schema.tables
  TEST_F(Shell_js_mysql_schema_tests, mysql_schema_tables)
  {
    exec_and_out_equals("print(session.js_shell_test.tables.table1)", "<Table:table1>");
  }

  // Tests schema.getViews()
  TEST_F(Shell_js_mysql_schema_tests, mysql_schema_get_views)
  {
    exec_and_out_equals("var views = session.js_shell_test.getViews();");

    exec_and_out_equals("print(views.view1)", "<View:view1>");
  }

  // Tests schema.views
  TEST_F(Shell_js_mysql_schema_tests, mysql_schema_views)
  {
    exec_and_out_equals("print(session.js_shell_test.views.view1)", "<View:view1>");
  }

  // Tests schema.getTable()
  TEST_F(Shell_js_mysql_schema_tests, mysql_schema_get_table)
  {
    exec_and_out_equals("var table = session.js_shell_test.getTable('table1');");

    exec_and_out_equals("print(table)", "<Table:table1>");
  }

  // Tests schema.getView()
  TEST_F(Shell_js_mysql_schema_tests, mysql_schema_get_view)
  {
    exec_and_out_equals("var view = session.js_shell_test.getView('view1');");

    exec_and_out_equals("print(view)", "<View:view1>");
  }

  // Tests schema.<object>
  TEST_F(Shell_js_mysql_schema_tests, mysql_schema_object)
  {
    exec_and_out_equals("print(session.js_shell_test.table1)", "<Table:table1>");
    
    exec_and_out_equals("print(session.js_shell_test.view1)", "<View:view1>");
  }
}