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
  class Shell_py_mysql_resultset_tests : public Shell_core_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_Python, initilaized);
    }
  };

  TEST_F(Shell_py_mysql_resultset_tests, initialization)
  {
    exec_and_out_equals("import mysql");

    exec_and_out_equals("session = mysql.getClassicSession('" + _mysql_uri + "')");

    exec_and_out_equals("session.sql('drop schema if exists py_shell_test;')");
    exec_and_out_equals("session.createSchema('py_shell_test');");
    exec_and_out_equals("session.setCurrentSchema('py_shell_test');");
    exec_and_out_equals("session.sql('create table table1 (id int auto_increment primary key, name varchar(50));')");

    exec_and_out_equals("session.sql('insert into table1 (`name`) values(\"one\");')");
    exec_and_out_equals("session.sql('insert into table1 (`name`) values(\"two\");')");
    exec_and_out_equals("session.sql('insert into table1 (`name`) values(\"three\");')");

    exec_and_out_equals("session.close()");
  }

  // Tests resultset.hasData and resultset.getHasData()
  TEST_F(Shell_py_mysql_resultset_tests, mysql_resultset_has_data)
  {
    exec_and_out_equals("import mysql");

    exec_and_out_equals("session = mysql.getClassicSession('" + _mysql_uri + "')");

    exec_and_out_equals("result = session.sql('use py_shell_test;')");

    exec_and_out_equals("print(result.hasData)", "False");
    exec_and_out_equals("print(result.getHasData())", "False");

    exec_and_out_equals("result = session.sql('select * from table1;')");

    exec_and_out_equals("print(result.hasData)", "True");
    exec_and_out_equals("print(result.getHasData())", "True");

    exec_and_out_equals("session.close()");
  }

  // Tests resultset.columnMetadata and resultset.getColumnMetadata()
  TEST_F(Shell_py_mysql_resultset_tests, mysql_resultset_column_metadata)
  {
    exec_and_out_equals("import mysql");

    exec_and_out_equals("session = mysql.getClassicSession('" + _mysql_uri + "')");

    exec_and_out_equals("result = session.sql('select * from py_shell_test.table1;')");

    exec_and_out_equals("metadata = result.getColumnMetadata()");

    exec_and_out_equals("print(metadata[0].name)", "id");

    exec_and_out_equals("metadata = result.columnMetadata");

    exec_and_out_equals("print(metadata[0].name)", "id");

    exec_and_out_equals("session.close()");
  }

  // Tests resultset.lastInsertId and resultset.getLastInsertId()
  TEST_F(Shell_py_mysql_resultset_tests, DISABLED_mysql_resultset_last_insert_id)
  {
    exec_and_out_equals("import mysql");

    exec_and_out_equals("session = mysql.getClassicSession('" + _mysql_uri + "')");

    exec_and_out_equals("result = session.sql('insert into py_shell_test.table1 (`name`) values(\"four\");')");

    exec_and_out_equals("print(result.lastInsertId)", "4");
    exec_and_out_equals("print(result.getLastInsertId())", "4");

    exec_and_out_equals("session.close()");
  }
}