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
  class Shell_js_mysql_table_tests : public Shell_core_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_JScript, initilaized);

      exec_and_out_equals("var mysql = require('mysql').mysql;");

      exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "');");

      exec_and_out_equals("session.executeSql('drop schema if exists js_shell_test;')");
      exec_and_out_equals("session.executeSql('create schema js_shell_test;')");
      exec_and_out_equals("session.executeSql('use js_shell_test;')");
      exec_and_out_equals("session.executeSql('create table table1 (name varchar(50));')");
    }
  };

  // Tests table.getName()
  TEST_F(Shell_js_mysql_table_tests, mysql_schema_get_name)
  {
    exec_and_out_equals("var table = session.js_shell_test.table1;");

    exec_and_out_equals("print(table.getName());", "table1");
  }

  // Tests table.name
  TEST_F(Shell_js_mysql_table_tests, mysql_schema_name)
  {
    exec_and_out_equals("var table = session.js_shell_test.table1;");

    exec_and_out_equals("print(table.name);", "table1");
  }

  // Tests table.getSession()
  TEST_F(Shell_js_mysql_table_tests, mysql_schema_get_session)
  {
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("var table = session.js_shell_test.table1;");

    exec_and_out_equals("var table_session = table.getSession();");

    exec_and_out_equals("print(table_session)", "<ClassicSession:" + uri + ">");
  }

  // Tests table.session
  TEST_F(Shell_js_mysql_table_tests, mysql_schema_session)
  {
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("var table = session.js_shell_test.table1;");

    exec_and_out_equals("print(table.session)", "<ClassicSession:" + uri + ">");
  }

  // Tests table.getSchema()
  TEST_F(Shell_js_mysql_table_tests, mysql_schema_get_schema)
  {
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("var table = session.js_shell_test.table1;");

    exec_and_out_equals("var table_schema = table.getSchema();");

    exec_and_out_equals("print(table_schema)", "<Schema:js_shell_test>");
  }

  // Tests table.schema
  TEST_F(Shell_js_mysql_table_tests, mysql_schema_schema)
  {
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("var table = session.js_shell_test.table1;");

    exec_and_out_equals("print(table.schema)", "<Schema:js_shell_test>");
  }
}