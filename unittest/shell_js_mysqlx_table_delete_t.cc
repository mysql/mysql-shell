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

#include "../modules/mod_mysqlx_table_delete.h"

namespace shcore {
  class Shell_js_mysqlx_table_delete_tests : public Crud_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_JScript, initilaized);

      // Sets the correct functions to be validated
      set_functions("delete, where, orderBy, limit, bind, execute, __shell_hook__");
    }
  };

  TEST_F(Shell_js_mysqlx_table_delete_tests, initialization)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");

    exec_and_out_equals("session.sql('drop schema if exists js_shell_test;').execute();");
    exec_and_out_equals("session.createSchema('js_shell_test');");
    exec_and_out_equals("session.setCurrentSchema('js_shell_test');");
    exec_and_out_equals("session.sql('create table table1 (name varchar(50), age integer, gender varchar(20));').execute();");

    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_table_delete_tests, chain_combinations)
  {
    // NOTE: No data validation is done on this test, only tests
    //       different combinations of chained methods.

    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.getSession('" + _uri + "');");

    exec_and_out_equals("var table = session.js_shell_test.getTable('table1');");

    // Creates the table delete object
    exec_and_out_equals("var crud = table.delete();");

    //-------- ----------------------------------------------------
    // Tests the happy path validating only the right functions
    // are available following the chained call
    // this is TableRemove.delete().where(condition).limit(#).execute()
    //-------------------------------------------------------------
    // TODO: Function availability needs to be tested on after the rest of the functions
    //       once they are enabled
    {
      SCOPED_TRACE("Testing function availability after delete.");
      ensure_available_functions("where, orderBy, limit, bind, execute, __shell_hook__");
    }

    // Now executes where
    {
      SCOPED_TRACE("Testing function availability after where.");
      exec_and_out_equals("crud.where(\"id < 100\")");
      ensure_available_functions("orderBy, limit, bind, execute, __shell_hook__");
    }

    // Now executes orderBy
    {
      SCOPED_TRACE("Testing function availability after orderBy.");
      exec_and_out_equals("crud.orderBy(['name'])");
      ensure_available_functions("limit, bind, execute, __shell_hook__");
    }

    // Now executes limit
    {
      SCOPED_TRACE("Testing function availability after limit.");
      exec_and_out_equals("crud.limit(1)");
      ensure_available_functions("bind, execute, __shell_hook__");
    }

    // Now executes bind
    {
      SCOPED_TRACE("Testing function availability after bind.");
      exec_and_out_equals("crud = table.delete().where('test = :data').bind('data', 5)");
      ensure_available_functions("bind, execute, __shell_hook__");
    }

    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_table_delete_tests, delete_validations)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");
    exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");
    exec_and_out_equals("var schema = session.getSchema('js_shell_test');");
    exec_and_out_equals("var table = schema.getTable('table1');");

    // Testing the delete function
    {
      SCOPED_TRACE("Testing parameter validation on delete");
      exec_and_out_equals("table.delete();");
      exec_and_out_contains("table.delete(5);", "", "Invalid number of arguments in TableDelete.delete, expected 0 but got 1");
    }

    {
      SCOPED_TRACE("Testing parameter validation on where");
      exec_and_out_contains("table.delete().where();", "", "Invalid number of arguments in TableDelete.where, expected 1 but got 0");
      exec_and_out_contains("table.delete().where(5);", "", "TableDelete.where: Argument #1 is expected to be a string");
      exec_and_out_contains("table.delete().where('test = \"2');", "", "TableDelete.where: Unterminated quoted string starting at 8");
      exec_and_out_equals("table.delete().where('test = \"2\"');");
    }

    {
      SCOPED_TRACE("Testing parameter validation on orderBy");
      exec_and_out_contains("table.delete().orderBy();", "", "Invalid number of arguments in TableDelete.orderBy, expected 1 but got 0");
      exec_and_out_contains("table.delete().orderBy(5);", "", "TableDelete.orderBy: Argument #1 is expected to be an array");
      exec_and_out_contains("table.delete().orderBy([]);", "", "TableDelete.orderBy: Order criteria can not be empty");
      exec_and_out_contains("table.delete().orderBy(['test', 5]);", "", "TableDelete.orderBy: Element #2 is expected to be a string");
      exec_and_out_contains("table.delete().orderBy(['test']);", "", "");
    }

    {
      SCOPED_TRACE("Testing parameter validation on limit");
      exec_and_out_contains("table.delete().limit();", "", "Invalid number of arguments in TableDelete.limit, expected 1 but got 0");
      exec_and_out_contains("table.delete().limit('');", "", "TableDelete.limit: Argument #1 is expected to be an unsigned int");
      exec_and_out_equals("table.delete().limit(5);");
    }

    {
      SCOPED_TRACE("Testing parameter validation on bind");
      exec_and_out_contains("table.delete().where('name = :data and age > :years').bind()", "", "Invalid number of arguments in TableDelete.bind, expected 2 but got 0");
      exec_and_out_contains("table.delete().where('name = :data and age > :years').bind(5, 5)", "", "TableDelete.bind: Argument #1 is expected to be a string");
      exec_and_out_contains("table.delete().where('name = :data and age > :years').bind('another', 5)", "", "TableDelete.bind: Unable to bind value for unexisting placeholder: another");
    }

    {
      SCOPED_TRACE("Testing parameter validation on execute");
      exec_and_out_contains("table.delete().where('name = :data and age > :years').execute()", "", "TableDelete.execute: Missing value bindings for the next placeholders: data, years");
      exec_and_out_contains("table.delete().where('name = :data and age > :years').bind('years', 5).execute()", "", "TableDelete.execute: Missing value bindings for the next placeholders: data");
    }

    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_table_delete_tests, delete_execution)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");
    exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");
    exec_and_out_equals("var schema = session.getSchema('js_shell_test');");
    exec_and_out_equals("var table = schema.getTable('table1');");

    exec_and_out_equals("var result = table.insert({name: 'jack', age: 17, gender: 'male'}).execute();");
    exec_and_out_equals("var result = table.insert({name: 'adam', age: 15, gender: 'male'}).execute();");
    exec_and_out_equals("var result = table.insert({name: 'brian', age: 14, gender: 'male'}).execute();");
    exec_and_out_equals("var result = table.insert({name: 'alma', age: 13, gender: 'female'}).execute();");
    exec_and_out_equals("var result = table.insert({name: 'carol', age: 14, gender: 'female'}).execute();");
    exec_and_out_equals("var result = table.insert({name: 'donna', age: 16, gender: 'female'}).execute();");
    exec_and_out_equals("var result = table.insert({name: 'angel', age: 14, gender: 'male'}).execute();");

    {
      SCOPED_TRACE("Testing delete");
      exec_and_out_equals("var result = table.delete().where('age = 13').execute();");
      exec_and_out_equals("print(result.affectedRows)", "1");

      exec_and_out_equals("var records = table.select().execute().all();");
      exec_and_out_equals("print(records.length);", "6");
    }

    {
      SCOPED_TRACE("Testing delete with limits and bound value");
      exec_and_out_equals("var result = table.delete().where('gender = :heorshe').limit(2).bind('heorshe', 'male').execute();");
      exec_and_out_equals("print(result.affectedRows)", "2");

      exec_and_out_equals("var records = table.select().execute().all();");
      exec_and_out_equals("print(records.length);", "4");
    }

    {
      SCOPED_TRACE("Testing full delete");
      exec_and_out_equals("var result = table.delete().execute();");
      exec_and_out_equals("print(result.affectedRows)", "4");

      exec_and_out_equals("var records = table.select().execute().all();");
      exec_and_out_equals("print(records.length);", "0");
    }

    {
      SCOPED_TRACE("Testing delete with limit");

      exec_and_out_equals("var result = table.insert({name: 'jack', age: 17, gender: 'male'}).execute();");
      exec_and_out_equals("var result = table.insert({name: 'adam', age: 15, gender: 'male'}).execute();");
      exec_and_out_equals("var result = table.insert({name: 'brian', age: 14, gender: 'male'}).execute();");
      exec_and_out_equals("var result = table.insert({name: 'alma', age: 13, gender: 'female'}).execute();");
      exec_and_out_equals("var result = table.insert({name: 'carol', age: 14, gender: 'female'}).execute();");

      exec_and_out_equals("var records = table.select().execute().all();");
      exec_and_out_equals("print(records.length);", "5");

      exec_and_out_equals("var result = table.delete().limit(2).execute();");
      exec_and_out_equals("print(result.affectedRows)", "2");

      exec_and_out_equals("var records = table.select().execute().all();");
      exec_and_out_equals("print(records.length);", "3");

      exec_and_out_equals("var result = table.delete().limit(2).execute();");
      exec_and_out_equals("print(result.affectedRows)", "2");

      exec_and_out_equals("var records = table.select().execute().all();");
      exec_and_out_equals("print(records.length);", "1");

      exec_and_out_equals("var result = table.delete().limit(2).execute();");
      exec_and_out_equals("print(result.affectedRows)", "1");

      exec_and_out_equals("var records = table.select().execute().all();");
      exec_and_out_equals("print(records.length);", "0");
    }

    exec_and_out_equals("session.close();");
  }
}