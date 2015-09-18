/* Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/pointer_cast.hpp>

#include "gtest/gtest.h"
#include "shellcore/types.h"
#include "shellcore/lang_base.h"
#include "shellcore/types_cpp.h"

#include "shellcore/shell_core.h"
#include "shellcore/shell_jscript.h"
#include "test_utils.h"

namespace shcore {
  class Shell_py_mysqlx_table_insert_tests : public Crud_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_Python, initilaized);

      set_functions("insert,values,execute,__shell_hook__");
    }
  };

  TEST_F(Shell_py_mysqlx_table_insert_tests, initialization)
  {
    exec_and_out_equals("import mysqlx");

    exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "')");

    exec_and_out_equals("session.sql('drop schema if exists py_shell_test;').execute()");
    exec_and_out_equals("session.sql('create schema py_shell_test;').execute()");
    exec_and_out_equals("session.sql('use py_shell_test;').execute()");
    exec_and_out_equals("session.sql('create table table1 (name varchar(50), age integer, gender varchar(20));').execute()");
  }

  TEST_F(Shell_py_mysqlx_table_insert_tests, chain_combinations)
  {
    // NOTE: No data validation is done on this test, only tests
    //       different combinations of chained methods.
    exec_and_out_equals("import mysqlx");
    exec_and_out_equals("session = mysqlx.getSession('" + _uri + "')");
    exec_and_out_equals("table = session.py_shell_test.getTable('table1')");

    //-------- ---------------------Test 1-------------------------
    // Tests the happy path table.insert().values().execute()
    //-------------------------------------------------------------
    exec_and_out_equals("crud = table.insert()");
    ensure_available_functions("values");

    exec_and_out_equals("crud.values(1,2,3,4,5)");
    ensure_available_functions("values,execute,__shell_hook__");

    exec_and_out_equals("crud.values(6,7,8,9,10)");
    ensure_available_functions("values,execute,__shell_hook__");

    //-------- ---------------------Test 2-------------------------
    // Tests the happy path table.insert([column names]).values().execute()
    //-------------------------------------------------------------
    exec_and_out_equals("crud = table.insert(['id', 'name'])");
    ensure_available_functions("values");

    exec_and_out_equals("crud.values(1,2,3,4,5)");
    ensure_available_functions("values,execute,__shell_hook__");

    exec_and_out_equals("crud.values(6,7,8,9,10)");
    ensure_available_functions("values,execute,__shell_hook__");

    //-------- ---------------------Test 3-------------------------
    // Tests the happy path table.insert({columns:values}).values().execute()
    //-------------------------------------------------------------
    exec_and_out_equals("crud = table.insert({'id':3, 'name':'whatever'})");
    ensure_available_functions("execute,__shell_hook__");

    exec_and_out_equals("session.close()");
  }

  TEST_F(Shell_py_mysqlx_table_insert_tests, insert_validations)
  {
    exec_and_out_equals("import mysqlx");
    exec_and_out_equals("session = mysqlx.getSession('" + _uri + "')");
    exec_and_out_equals("table = session.py_shell_test.getTable('table1')");

    // Tests insert with invalid parameter
    exec_and_out_contains("table.insert(28).execute()", "", "TableInsert.insert: Argument #1 is expected to be either string, a list of strings or a map with fields and values");

    exec_and_out_contains("table.insert('name', 28).execute()", "", "TableInsert.insert: Argument #2 is expected to be a string");

    // Test add attempt with no data
    exec_and_out_contains("table.insert(['id', 45]).execute()", "", "TableInsert.insert: Element #2 is expected to be a string");

    // Test add attempt with column list but invalid values
    exec_and_out_contains("table.insert(['id','name']).values([5]).execute()", "", "Unsupported value received: [5]");

    // Test add attempt with column list but unsupported values
    exec_and_out_contains("table.insert(['id','name']).values(1, session).execute()", "", "Unsupported value received: <XSession");

    // Test add attempt with invalid column name
    exec_and_out_contains("table.insert(['id', 'name', 'gender']).values(15, 'walter', 'male').execute()", "", "Unknown column 'id' in 'field list'");

    exec_and_out_equals("session.close()");
  }

  TEST_F(Shell_py_mysqlx_table_insert_tests, insert_execution)
  {
    exec_and_out_equals("import mysqlx");
    exec_and_out_equals("session = mysqlx.getSession('" + _uri + "')");
    exec_and_out_equals("table = session.py_shell_test.getTable('table1')");

    // Insert without columns
    {
      SCOPED_TRACE("Testing insert without columns.");
      exec_and_out_equals("result = table.insert().values('jack', 17, 'male').execute()");
      exec_and_out_equals("print (result.affectedRows)", "1");
    }

    // Insert with columns
    {
      SCOPED_TRACE("Testing insert without columns.");
      exec_and_out_equals("result = table.insert(['age', 'name', 'gender']).values(21, 'john', 'male').execute()");
      exec_and_out_equals("print (result.affectedRows)", "1");
    }

    // Inserting multiple records
    {
      SCOPED_TRACE("Testing insert without columns.");
      exec_and_out_equals("insert = table.insert('name', 'age', 'gender')");
      exec_and_out_equals("insert.values('clark', 22,'male')");
      exec_and_out_equals("insert.values('mary', 13,'female')");
      exec_and_out_equals("result = insert.execute()");
      exec_and_out_equals("print (result.affectedRows)", "2");
    }

    // Inserting document
    {
      exec_and_out_equals("result = table.insert({'age':14, 'name':'jackie', 'gender': 'female'}).execute()");
      exec_and_out_equals("print (result.affectedRows)", "1");
    }

    exec_and_out_equals("session.close()");
  }
}