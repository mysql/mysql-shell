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

#include "../modules/mod_mysqlx_table_update.h"

namespace shcore {
  class Shell_js_mysqlx_table_update_tests : public Crud_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_JScript, initilaized);

      // Sets the correct functions to be validated
      set_functions("update, set, where, orderBy, limit, bind, execute");
    }
  };

  TEST_F(Shell_js_mysqlx_table_update_tests, initialization)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");

    exec_and_out_equals("session.sql('drop schema if exists js_shell_test;').execute();");
    exec_and_out_equals("session.sql('create schema js_shell_test;').execute();");
    exec_and_out_equals("session.sql('use js_shell_test;').execute();");
    exec_and_out_equals("session.sql('create table table1 (name varchar(50), age integer, gender varchar(20));').execute();");

    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_table_update_tests, chain_combinations)
  {
    // NOTE: No data validation is done on this test, only tests
    //       different combinations of chained methods.

    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.getSession('" + _uri + "');");

    exec_and_out_equals("var table = session.js_shell_test.getTable('table1');");

    // Creates the table delete object
    exec_and_out_equals("var crud = table.update();");

    //-------- ----------------------------------------------------
    // Tests the happy path validating only the right functions
    // are available following the chained call
    // this is TableRemove.delete().where(condition).limit(#).execute()
    //-------------------------------------------------------------
    // TODO: Function availability needs to be tested on after the rest of the functions
    //       once they are enabled
    {
      SCOPED_TRACE("Testing function availability after update.");
      ensure_available_functions("set");
    }

    // Now executes set
    {
      SCOPED_TRACE("Testing function availability after where.");
      exec_and_out_equals("crud.set('name', 'Jack')");
      ensure_available_functions("set, where, orderBy, limit, bind, execute");
    }

    // Now executes where
    {
      SCOPED_TRACE("Testing function availability after where.");
      exec_and_out_equals("crud.where(\"age < 100\")");
      ensure_available_functions("orderBy, limit, bind, execute");
    }

    // Now executes orderBy
    {
      SCOPED_TRACE("Testing function availability after orderBy.");
      exec_and_out_equals("crud.orderBy(['name'])");
      ensure_available_functions("limit, bind, execute");
    }

    // Now executes limit
    {
      SCOPED_TRACE("Testing function availability after limit.");
      exec_and_out_equals("crud.limit(1)");
      ensure_available_functions("bind, execute");
    }

    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_table_update_tests, update_validations)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");
    exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");
    exec_and_out_equals("var schema = session.getSchema('js_shell_test');");
    exec_and_out_equals("var table = schema.getTable('table1');");

    {
      SCOPED_TRACE("Testing parameter validation on update");
      exec_and_out_equals("table.update();");
      exec_and_out_contains("table.update(5);", "", "Invalid number of arguments in TableUpdate.update, expected 0 but got 1");
    }

    {
      SCOPED_TRACE("Testing parameter validation on set");
      exec_and_out_contains("table.update().set();", "", "Invalid number of arguments in TableUpdate.set, expected 2 but got 0");
      exec_and_out_contains("table.update().set(5, 17);", "", "TableUpdate.set: Argument #1 is expected to be a string");
      exec_and_out_contains("table.update().set('name', session);", "", "TableUpdate.set: Unsupported value received for table update operation on field \"name\", received: <NodeSession:");
      exec_and_out_equals("table.update().set('age', 17);");
    }

    {
      SCOPED_TRACE("Testing parameter validation on where");
      exec_and_out_contains("table.update().set('age', 17).where();", "", "Invalid number of arguments in TableUpdate.where, expected 1 but got 0");
      exec_and_out_contains("table.update().set('age', 17).where(5);", "", "TableUpdate.where: Argument #1 is expected to be a string");
      exec_and_out_contains("table.update().set('age', 17).where('name = \"2');", "", "TableUpdate.where: Unterminated quoted string starting at 8");
      exec_and_out_contains("table.update().set('age', 17).where('name = \"2\"');");
    }

    {
      SCOPED_TRACE("Testing parameter validation on orderBy");
      exec_and_out_contains("table.update().set('age', 17).orderBy();", "", "Invalid number of arguments in TableUpdate.orderBy, expected 1 but got 0");
      exec_and_out_contains("table.update().set('age', 17).orderBy(5);", "", "TableUpdate.orderBy: Argument #1 is expected to be an array");
      exec_and_out_contains("table.update().set('age', 17).orderBy([]);", "", "TableUpdate.orderBy: Order criteria can not be empty");
      exec_and_out_contains("table.update().set('age', 17).orderBy(['test', 5]);", "", "TableUpdate.orderBy: Element #2 is expected to be a string");
      exec_and_out_contains("table.update().set('age', 17).orderBy(['test']);", "", "");
    }

    {
      SCOPED_TRACE("Testing parameter validation on limit");
      exec_and_out_contains("table.update().set('age', 17).limit();", "", "Invalid number of arguments in TableUpdate.limit, expected 1 but got 0");
      exec_and_out_contains("table.update().set('age', 17).limit('');", "", "TableUpdate.limit: Argument #1 is expected to be an unsigned int");
      exec_and_out_contains("table.update().set('age', 17).limit(5);");
    }

    exec_and_out_contains("table.update().set('age', 17).bind();", "", "TableUpdate.bind: not yet implemented.");

    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_table_update_tests, update_execution)
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
      SCOPED_TRACE("Testing update");
      exec_and_out_equals("var result = table.update().set('name', 'aline').where('age = 13').execute();");
      exec_and_out_equals("print(result.affectedRows)", "1");

      exec_and_out_equals("var records = table.select().where('name = \"aline\"').execute().all();");
      exec_and_out_equals("print(records.length);", "1");
    }

    {
      SCOPED_TRACE("Testing with expression");
      exec_and_out_equals("var result = table.update().set('age', expr('13+10')).where('age = 13').execute();");
      exec_and_out_equals("print(result.affectedRows)", "1");

      exec_and_out_equals("var records = table.select().where('age = 23').execute().all();");
      exec_and_out_equals("print(records.length);", "1");
    }

    {
      SCOPED_TRACE("Testing update with limits");
      exec_and_out_equals("var result = table.update().set('age', 15).where('age = 14').limit(2).execute();");
      exec_and_out_equals("print(result.affectedRows)", "2");

      exec_and_out_equals("var records = table.select().where('age = 15').execute().all();");
      exec_and_out_equals("print(records.length);", "3");

      exec_and_out_equals("var records = table.select().where('age = 14').execute().all();");
      exec_and_out_equals("print(records.length);", "1");
    }

    {
      SCOPED_TRACE("Testing full update");
      exec_and_out_equals("var result = table.update().set('gender', 'female').execute();");
      exec_and_out_equals("print(result.affectedRows)", "4");

      exec_and_out_equals("var records = table.select().where('gender = \"female\"').execute().all();");
      exec_and_out_equals("print(records.length);", "7");
    }

    exec_and_out_equals("session.close();");
  }
}