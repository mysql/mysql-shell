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
  class Shell_js_mysql_resultset_tests : public Shell_core_test_wrapper
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

  TEST_F(Shell_js_mysql_resultset_tests, initialization)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "');");

    exec_and_out_equals("session.sql('drop schema if exists js_shell_test;')");
    exec_and_out_equals("session.createSchema('js_shell_test');");
    exec_and_out_equals("session.setCurrentSchema('js_shell_test');");
    exec_and_out_equals("session.sql('create table table1 (id int auto_increment primary key, name varchar(50));')");

    exec_and_out_equals("session.sql('insert into table1 (`name`) values(\"one\");')");
    exec_and_out_equals("session.sql('insert into table1 (`name`) values(\"two\");')");
    exec_and_out_equals("session.sql('insert into table1 (`name`) values(\"three\");')");

    exec_and_out_equals("session.close();");
  }

  // Tests resultset.hasData and resultset.getHasData()
  TEST_F(Shell_js_mysql_resultset_tests, mysql_resultset_has_data)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "');");

    exec_and_out_equals("var result = session.sql('use js_shell_test;');");

    exec_and_out_equals("print(result.hasData);", "false");
    exec_and_out_equals("print(result.getHasData());", "false");

    exec_and_out_equals("var result = session.sql('select * from table1;');");

    exec_and_out_equals("print(result.hasData);", "true");
    exec_and_out_equals("print(result.getHasData());", "true");

    exec_and_out_equals("session.close();");
  }

  // Tests resultset.columnMetadata and resultset.getColumnMetadata()
  TEST_F(Shell_js_mysql_resultset_tests, mysql_resultset_column_metadata)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "');");

    exec_and_out_equals("var result = session.sql('select * from js_shell_test.table1;');");

    exec_and_out_equals("var metadata = result.getColumnMetadata()");

    exec_and_out_equals("print(metadata[0].name)", "id");

    exec_and_out_equals("var metadata = result.columnMetadata");

    exec_and_out_equals("print(metadata[0].name)", "id");

    exec_and_out_equals("session.close();");
  }

  // Tests resultset.lastInsertId and resultset.getLastInsertId()
  TEST_F(Shell_js_mysql_resultset_tests, DISABLED_mysql_resultset_last_insert_id)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "');");

    exec_and_out_equals("var result = session.sql('insert into js_shell_test.table1 (`name`) values(\"four\");');");

    exec_and_out_equals("print(result.lastInsertId)", "4");
    exec_and_out_equals("print(result.getLastInsertId())", "4");

    exec_and_out_equals("session.close();");
  }
}