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

#include "../modules/mod_mysqlx_table_select.h"

namespace shcore {
  class Shell_py_mysqlx_table_select_tests : public Crud_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_Python, initilaized);

      // Sets the correct functions to be validated
      set_functions("select, where, groupBy, having, orderBy, offset, limit, bind, execute, __shell_hook__");
    }
  };

  TEST_F(Shell_py_mysqlx_table_select_tests, initialization)
  {
    exec_and_out_equals("import mysqlx");

    exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "')");

    exec_and_out_equals("session.sql('drop schema if exists js_shell_test;').execute()");
    exec_and_out_equals("session.sql('create schema js_shell_test;').execute()");
    exec_and_out_equals("session.sql('use js_shell_test;').execute()");
    exec_and_out_equals("session.sql('create table table1 (name varchar(50), age integer, gender varchar(20));').execute()");
  }

  TEST_F(Shell_py_mysqlx_table_select_tests, chain_combinations)
  {
    // NOTE: No data validation is done on this test, only tests
    //       different combinations of chained methods.
    exec_and_out_equals("import mysqlx");

    exec_and_out_equals("session = mysqlx.getSession('" + _uri + "')");

    exec_and_out_equals("table = session.js_shell_test.getTable('table1')");

    // Creates the table select object
    exec_and_out_equals("crud = table.select()");

    //-------- ----------------------------------------------------
    // Tests the happy path validating only the right functions
    // are available following the chained call
    // this is TableSelect.select().offset(#).limit(#).execute()
    //-------------------------------------------------------------
    // TODO: Function availability needs to be tested on after the rest of the functions
    //       once they are enabled
    {
      SCOPED_TRACE("Testing function availability after select.");
      ensure_available_functions("where, groupBy, orderBy, limit, bind, execute, __shell_hook__");
    }

    // Now executes where
    {
      SCOPED_TRACE("Testing function availability after where.");
      exec_and_out_equals("crud.where('age > 13')");
      ensure_available_functions("groupBy, orderBy, limit, bind, execute, __shell_hook__");
    }

    // Now executes groupBy
    {
      SCOPED_TRACE("Testing function availability after groupBy.");
      exec_and_out_equals("crud.groupBy(['age'])");
      ensure_available_functions("having, orderBy, limit, bind, execute, __shell_hook__");
    }

    // Now executes having
    {
      SCOPED_TRACE("Testing function availability after groupBy.");
      exec_and_out_equals("crud.having('age < 13')");
      ensure_available_functions("orderBy, limit, bind, execute, __shell_hook__");
    }

    // Now executes having
    {
      SCOPED_TRACE("Testing function availability after groupBy.");
      exec_and_out_equals("crud.orderBy(['name ASC'])");
      ensure_available_functions("limit, bind, execute, __shell_hook__");
    }

    // Now executes limit
    {
      SCOPED_TRACE("Testing function availability after limit.");
      exec_and_out_equals("crud.limit(1)");
      ensure_available_functions("offset, bind, execute, __shell_hook__");
    }

    // Now executes offset
    {
      SCOPED_TRACE("Testing function availability after offset.");
      exec_and_out_equals("crud.offset(1)");
      ensure_available_functions("bind, execute, __shell_hook__");
    }

    // Now executes bind
    {
      SCOPED_TRACE("Testing function availability after bind.");
      exec_and_out_equals("crud = table.select().where('test = :data').bind('data', 5)");
      ensure_available_functions("bind, execute, __shell_hook__");
    }

    exec_and_out_equals("session.close()");
  }

  TEST_F(Shell_py_mysqlx_table_select_tests, select_validations)
  {
    exec_and_out_equals("import mysqlx");

    exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "')");

    exec_and_out_equals("schema = session.getSchema('js_shell_test')");

    exec_and_out_equals("table = schema.getTable('table1')");

    // Testing the select function
    {
      SCOPED_TRACE("Testing parameter validation on select");
      exec_and_out_equals("table.select()");
      exec_and_out_contains("table.select(5)", "", "TableSelect.select: Argument #1 is expected to be an array");
      exec_and_out_contains("table.select([])", "", "TableSelect.select: Field selection criteria can not be empty");
      exec_and_out_contains("table.select(['name as alias', 5])", "", "TableSelect.select: Element #2 is expected to be a string");
      exec_and_out_contains("table.select(['name'])");
    }

    {
      SCOPED_TRACE("Testing parameter validation on fields");
      exec_and_out_contains("table.select().where()", "", "Invalid number of arguments in TableSelect.where, expected 1 but got 0");
      exec_and_out_contains("table.select().where(5)", "", "TableSelect.where: Argument #1 is expected to be a string");
      exec_and_out_contains("table.select().where('name = \"whatever')", "", "");
      exec_and_out_contains("table.select().where('name = \"whatever\"')", "", "");
    }

    {
      SCOPED_TRACE("Testing parameter validation on groupBy");
      exec_and_out_contains("table.select().groupBy()", "", "Invalid number of arguments in TableSelect.groupBy, expected 1 but got 0");
      exec_and_out_contains("table.select().groupBy(5)", "", "TableSelect.groupBy: Argument #1 is expected to be an array");
      exec_and_out_contains("table.select().groupBy([])", "", "TableSelect.groupBy: Grouping criteria can not be empty");
      exec_and_out_contains("table.select().groupBy(['name', 5])", "", "TableSelect.groupBy: Element #2 is expected to be a string");
      exec_and_out_contains("table.select().groupBy(['name'])", "", "");
    }

    {
      SCOPED_TRACE("Testing parameter validation on having");
      exec_and_out_contains("table.select().groupBy(['name']).having()", "", "Invalid number of arguments in TableSelect.having, expected 1 but got 0");
      exec_and_out_contains("table.select().groupBy(['name']).having(5)", "", "TableSelect.having: Argument #1 is expected to be a string");
      exec_and_out_contains("table.select().groupBy(['name']).having('age > 5')", "", "");
    }

    {
      SCOPED_TRACE("Testing parameter validation on orderBy");
      exec_and_out_contains("table.select().orderBy()", "", "Invalid number of arguments in TableSelect.orderBy, expected 1 but got 0");
      exec_and_out_contains("table.select().orderBy(5)", "", "TableSelect.orderBy: Argument #1 is expected to be an array");
      exec_and_out_contains("table.select().orderBy([])", "", "TableSelect.orderBy: Order criteria can not be empty");
      exec_and_out_contains("table.select().orderBy(['test', 5])", "", "TableSelect.orderBy: Element #2 is expected to be a string");
      exec_and_out_contains("table.select().orderBy(['test'])", "", "");
    }

    {
      SCOPED_TRACE("Testing parameter validation on limit");
      exec_and_out_contains("table.select().limit()", "", "Invalid number of arguments in TableSelect.limit, expected 1 but got 0");
      exec_and_out_contains("table.select().limit('')", "", "TableSelect.limit: Argument #1 is expected to be an unsigned int");
      exec_and_out_equals("table.select().limit(5)");
    }

    {
      SCOPED_TRACE("Testing parameter validation on offset");
      exec_and_out_contains("table.select().limit(1).offset()", "", "Invalid number of arguments in TableSelect.offset, expected 1 but got 0");
      exec_and_out_contains("table.select().limit(1).offset('')", "", "TableSelect.offset: Argument #1 is expected to be an unsigned int");
      exec_and_out_equals("table.select().limit(1).offset(5)");
    }

    {
      SCOPED_TRACE("Testing parameter validation on bind");
      exec_and_out_contains("table.select().where('name = :data and age > :years').bind()", "", "Invalid number of arguments in TableSelect.bind, expected 2 but got 0");
      exec_and_out_contains("table.select().where('name = :data and age > :years').bind(5, 5)", "", "TableSelect.bind: Argument #1 is expected to be a string");
      exec_and_out_contains("table.select().where('name = :data and age > :years').bind('another', 5)", "", "TableSelect.bind: Unable to bind value for unexisting placeholder: another");
    }

    {
      SCOPED_TRACE("Testing parameter validation on execute");
      exec_and_out_contains("table.select().where('name = :data and age > :years').execute()", "", "TableSelect.execute: Missing value bindings for the next placeholders: data, years");
      exec_and_out_contains("table.select().where('name = :data and age > :years').bind('years', 5).execute()", "", "TableSelect.execute: Missing value bindings for the next placeholders: data");
    }

    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_py_mysqlx_table_select_tests, select_execution)
  {
    exec_and_out_equals("import mysqlx");
    exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "')");
    exec_and_out_equals("schema = session.getSchema('js_shell_test')");
    exec_and_out_equals("table = schema.getTable('table1')");

    exec_and_out_equals("result = table.insert({'name': 'jack', 'age': 17, 'gender': 'male'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'adam', 'age': 15, 'gender': 'male'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'brian', 'age': 14, 'gender': 'male'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'alma', 'age': 13, 'gender': 'female'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'carol', 'age': 14, 'gender': 'female'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'donna', 'age': 16, 'gender': 'female'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'angel', 'age': 14, 'gender': 'male'}).execute()");

    // Testing the select function
    {
      SCOPED_TRACE("Testing full select");
      exec_and_out_equals("records = table.select().execute().all()");
      exec_and_out_equals("print(len(records))", "7");
    }

    // Testing column filtering
    {
      SCOPED_TRACE("Testing select with column filtering");
      exec_and_out_equals("result = table.select(['name', 'age']).execute()");
      exec_and_out_equals("record = result.next()");
      exec_and_out_equals("print(record.getLength())", "2");
      exec_and_out_equals("print(record.name != '')", "True");
      exec_and_out_equals("print(record.age != '')", "True");
      exec_and_out_contains("print(record.gender != '')", "", "unknown attribute: gender");
      exec_and_out_equals("result.all()"); // Temporal hack: flushes the rest of the data

      exec_and_out_equals("result = table.select(['name']).execute()");
      exec_and_out_equals("record = result.next()");
      exec_and_out_equals("print(record.getLength())", "1");
      exec_and_out_equals("print(record.name != '')", "True");
      exec_and_out_contains("print(record.age != '')", "", "unknown attribute: age");
      exec_and_out_contains("print(record.gender != '')", "", "unknown attribute: gender");
      exec_and_out_equals("result.all()"); // Temporal hack: flushes the rest of the data
    }

    // Testing the select function with some criteria
    {
      SCOPED_TRACE("Testing full select with different filtering criteria");
      exec_and_out_equals("records = table.select().where('gender = \"male\"').execute().all()");
      exec_and_out_equals("print(len(records))", "4");

      exec_and_out_equals("records = table.select().where('gender = \"female\"').execute().all()");
      exec_and_out_equals("print(len(records))", "3");

      exec_and_out_equals("records = table.select().where('age = 13').execute().all()");
      exec_and_out_equals("print(len(records))", "1");

      exec_and_out_equals("records = table.select().where('age = 14').execute().all()");
      exec_and_out_equals("print(len(records))", "3");

      exec_and_out_equals("records = table.select().where('age < 17').execute().all()");
      exec_and_out_equals("print(len(records))", "6");

      exec_and_out_equals("records = table.select().where('name like \"a%\"').execute().all()");
      exec_and_out_equals("print(len(records))", "3");

      exec_and_out_equals("records = table.select().where('name like \"a%\" and age < 15').execute().all()");
      exec_and_out_equals("print(len(records))", "2");
    }

    {
      SCOPED_TRACE("Testing orderBy");
      exec_and_out_equals("records = table.select().orderBy(['name']).execute().all()");
      exec_and_out_equals("print(records[0].name)", "adam");
      exec_and_out_equals("print(records[1].name)", "alma");
      exec_and_out_equals("print(records[2].name)", "angel");
      exec_and_out_equals("print(records[3].name)", "brian");
      exec_and_out_equals("print(records[4].name)", "carol");
      exec_and_out_equals("print(records[5].name)", "donna");
      exec_and_out_equals("print(records[6].name)", "jack");

      exec_and_out_equals("records = table.select().orderBy(['name desc']).execute().all()");
      exec_and_out_equals("print(records[0].name)", "jack");
      exec_and_out_equals("print(records[1].name)", "donna");
      exec_and_out_equals("print(records[2].name)", "carol");
      exec_and_out_equals("print(records[3].name)", "brian");
      exec_and_out_equals("print(records[4].name)", "angel");
      exec_and_out_equals("print(records[5].name)", "alma");
      exec_and_out_equals("print(records[6].name)", "adam");
    }

    {
      SCOPED_TRACE("Testing limit and offset");
      exec_and_out_equals("records = table.select().limit(4).execute().all()");
      exec_and_out_equals("print(len(records))", "4");

      SCOPED_TRACE("Testing limit and offset");
      exec_and_out_equals("records = table.select().limit(4).offset(1).execute().all()");
      exec_and_out_equals("print(len(records))", "4");

      SCOPED_TRACE("Testing limit and offset");
      exec_and_out_equals("records = table.select().limit(4).offset(2).execute().all()");
      exec_and_out_equals("print(len(records))", "4");

      SCOPED_TRACE("Testing limit and offset");
      exec_and_out_equals("records = table.select().limit(4).offset(3).execute().all()");
      exec_and_out_equals("print(len(records))", "4");

      SCOPED_TRACE("Testing limit and offset");
      exec_and_out_equals("records = table.select().limit(4).offset(4).execute().all()");
      exec_and_out_equals("print(len(records))", "3");

      SCOPED_TRACE("Testing limit and offset");
      exec_and_out_equals("records = table.select().limit(4).offset(5).execute().all()");
      exec_and_out_equals("print(len(records))", "2");

      SCOPED_TRACE("Testing limit and offset");
      exec_and_out_equals("records = table.select().limit(4).offset(6).execute().all()");
      exec_and_out_equals("print(len(records))", "1");

      SCOPED_TRACE("Testing limit and offset");
      exec_and_out_equals("records = table.select().limit(4).offset(7).execute().all()");
      exec_and_out_equals("print(len(records))", "0");
    }

    {
      SCOPED_TRACE("Testing bind");
      exec_and_out_equals("records = table.select().where('age = :years and gender = :heorshe').bind('years', 13).bind('heorshe','female').execute().all();");
      exec_and_out_equals("print(len(records));", "1");
      exec_and_out_equals("print(records[0].name);", "alma");
    }

    exec_and_out_equals("session.close();");
  }
}