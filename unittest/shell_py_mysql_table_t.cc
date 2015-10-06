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
  class Shell_py_mysql_table_tests : public Shell_core_test_wrapper
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

      exec_and_out_equals("session.runSql('drop schema if exists py_shell_test;')");
      exec_and_out_equals("session.createSchema('py_shell_test');");
      exec_and_out_equals("session.setCurrentSchema('py_shell_test');");
      exec_and_out_equals("session.runSql('create table table1 (name varchar(50));')");
    }
  };

  // Tests table.getName()
  TEST_F(Shell_py_mysql_table_tests, mysql_schema_get_name)
  {
    exec_and_out_equals("table = session.py_shell_test.table1");

    exec_and_out_equals("print(table.getName())", "table1");
  }

  // Tests table.name
  TEST_F(Shell_py_mysql_table_tests, mysql_schema_name)
  {
    exec_and_out_equals("table = session.py_shell_test.table1");

    exec_and_out_equals("print(table.name)", "table1");
  }

  // Tests table.getSession()
  TEST_F(Shell_py_mysql_table_tests, mysql_schema_get_session)
  {
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("table = session.py_shell_test.table1");

    exec_and_out_equals("table_session = table.getSession()");

    exec_and_out_equals("print(table_session)", "<ClassicSession:" + uri + ">");
  }

  // Tests table.session
  TEST_F(Shell_py_mysql_table_tests, mysql_schema_session)
  {
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("table = session.py_shell_test.table1");

    exec_and_out_equals("print(table.session)", "<ClassicSession:" + uri + ">");
  }

  // Tests table.getSchema()
  TEST_F(Shell_py_mysql_table_tests, mysql_schema_get_schema)
  {
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("table = session.py_shell_test.table1");

    exec_and_out_equals("table_schema = table.getSchema()");

    exec_and_out_equals("print(table_schema)", "<ClassicSchema:py_shell_test>");
  }

  // Tests table.schema
  TEST_F(Shell_py_mysql_table_tests, mysql_schema_schema)
  {
    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("table = session.py_shell_test.table1");

    exec_and_out_equals("print(table.schema)", "<ClassicSchema:py_shell_test>");
  }

  // Tests session.dropTable() and table.existInDatabase()
  TEST_F(Shell_py_mysql_table_tests, mysql_table_drop_exist_in_database)
  {
    exec_and_out_equals("schema = session.createSchema('my_sample_schema')");

    exec_and_out_equals("session.runSql('create table my_sample_schema.my_sample_table (name varchar(50));')");

    exec_and_out_equals("table = schema.my_sample_table");

    exec_and_out_equals("print(table.existInDatabase())", "True");

    exec_and_out_equals("session.dropTable('my_sample_schema', 'my_sample_table')");

    exec_and_out_equals("print(table.existInDatabase())", "False");

    exec_and_out_equals("session.dropSchema('my_sample_schema')");

    exec_and_out_equals("session.close();");
  }
}