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

#include "../modules/mod_mysqlx_collection_remove.h"

namespace shcore {
  class Shell_py_mysqlx_collection_remove_tests : public Crud_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_Python, initilaized);

      // Sets the correct functions to be validated
      set_functions("remove, sort, limit, bind, execute, __shell_hook__");
    }
  };

  TEST_F(Shell_py_mysqlx_collection_remove_tests, initialization)
  {
    exec_and_out_equals("import mysqlx");

    exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "')");

    exec_and_out_equals("session.sql('drop schema if exists js_shell_test;').execute()");
    exec_and_out_equals("session.sql('create schema js_shell_test;').execute()");

    exec_and_out_equals("session.js_shell_test.createCollection('collection1')");

    exec_and_out_equals("session.close()");
  }

  TEST_F(Shell_py_mysqlx_collection_remove_tests, chain_combinations)
  {
    // NOTE: No data validation is done on this test, only tests
    //       different combinations of chained methods.

    exec_and_out_equals("import mysqlx");

    exec_and_out_equals("session = mysqlx.getSession('" + _uri + "')");

    exec_and_out_equals("collection = session.js_shell_test.getCollection('collection1')");

    // Creates the collection find object
    exec_and_out_equals("crud = collection.remove()");

    //-------- ----------------------------------------------------
    // Tests the happy path validating only the right functions
    // are available following the chained call
    // this is CollectionRemove.remove().limit(#).execute()
    //-------------------------------------------------------------
    // TODO: Function availability needs to be tested on after the rest of the functions
    //       once they are enabled
    {
      SCOPED_TRACE("Testing function availability after remove.");
      ensure_available_functions("sort, limit, bind, execute, __shell_hook__");
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
      exec_and_out_equals("crud = collection.remove('test = :data').bind('data', 5)");
      ensure_available_functions("bind, execute, __shell_hook__");
    }

    exec_and_out_equals("session.close()");
  }

  TEST_F(Shell_py_mysqlx_collection_remove_tests, remove_validations)
  {
    exec_and_out_equals("import mysqlx");
    exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "')");
    exec_and_out_equals("schema = session.getSchema('js_shell_test')");
    exec_and_out_equals("collection = schema.getCollection('collection1')");

    // Testing the remove function
    {
      SCOPED_TRACE("Testing parameter validation on remove");
      exec_and_out_equals("collection.remove()");
      exec_and_out_contains("collection.remove(5)", "", "CollectionRemove.remove: Argument #1 is expected to be a string");
      exec_and_out_contains("collection.remove('test = \"2')", "", "CollectionRemove.remove: Unterminated quoted string starting at 8");
      exec_and_out_equals("collection.remove('test = \"2\"')");
    }

    {
      SCOPED_TRACE("Testing parameter validation on sort");
      exec_and_out_contains("collection.remove().sort()", "", "Invalid number of arguments in CollectionRemove.sort, expected 1 but got 0");
      exec_and_out_contains("collection.remove().sort(5)", "", "CollectionRemove.sort: Argument #1 is expected to be an array");
      exec_and_out_contains("collection.remove().sort([])", "", "CollectionRemove.sort: Sort criteria can not be empty");
      exec_and_out_contains("collection.remove().sort(['name', 5])", "", "CollectionRemove.sort: Element #2 is expected to be a string");
      exec_and_out_contains("collection.remove().sort(['name'])", "", "");
    }

    {
      SCOPED_TRACE("Testing parameter validation on limit");
      exec_and_out_contains("collection.remove().limit()", "", "Invalid number of arguments in CollectionRemove.limit, expected 1 but got 0");
      exec_and_out_contains("collection.remove().limit('')", "", "CollectionRemove.limit: Argument #1 is expected to be an unsigned int");
      exec_and_out_equals("collection.remove().limit(5)");
    }

    {
      SCOPED_TRACE("Testing parameter validation on bind");
      exec_and_out_contains("collection.remove('name = :data and age > :years').bind()", "", "Invalid number of arguments in CollectionRemove.bind, expected 2 but got 0");
      exec_and_out_contains("collection.remove('name = :data and age > :years').bind(5, 5)", "", "CollectionRemove.bind: Argument #1 is expected to be a string");
      exec_and_out_contains("collection.remove('name = :data and age > :years').bind('another', 5)", "", "CollectionRemove.bind: Unable to bind value for unexisting placeholder: another");
    }

    {
      SCOPED_TRACE("Testing parameter validation on execute");
      exec_and_out_contains("collection.remove('name = :data and age > :years').execute()", "", "CollectionRemove.execute: Missing value bindings for the next placeholders: data, years");
      exec_and_out_contains("collection.remove('name = :data and age > :years').bind('years', 5).execute()", "", "CollectionRemove.execute: Missing value bindings for the next placeholders: data");
    }

    exec_and_out_equals("session.close()");
  }

  TEST_F(Shell_py_mysqlx_collection_remove_tests, remove_execution)
  {
    exec_and_out_equals("import mysqlx");
    exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "')");
    exec_and_out_equals("schema = session.getSchema('js_shell_test')");
    exec_and_out_equals("collection = schema.getCollection('collection1')");

    exec_and_out_equals("result = collection.add({'name': 'jack', 'age': 17, 'gender': 'male'}).execute()");
    exec_and_out_equals("result = collection.add({'name': 'adam', 'age': 15, 'gender': 'male'}).execute()");
    exec_and_out_equals("result = collection.add({'name': 'brian', 'age': 14, 'gender': 'male'}).execute()");
    exec_and_out_equals("result = collection.add({'name': 'alma', 'age': 13, 'gender': 'female'}).execute()");
    exec_and_out_equals("result = collection.add({'name': 'carol', 'age': 14, 'gender': 'female'}).execute()");
    exec_and_out_equals("result = collection.add({'name': 'donna', 'age': 16, 'gender': 'female'}).execute()");
    exec_and_out_equals("result = collection.add({'name': 'angel', 'age': 14, 'gender': 'male'}).execute()");

    {
      SCOPED_TRACE("Testing remove");
      exec_and_out_equals("result = collection.remove('age = 13').execute()");
      exec_and_out_equals("print(result.affectedRows)", "1");

      exec_and_out_equals("records = collection.find().execute().all()");
      exec_and_out_equals("print(len(records))", "6");
    }

    {
      SCOPED_TRACE("Testing remove with limits and bound values");
      exec_and_out_equals("result = collection.remove('gender = :heorshe').limit(2).bind('heorshe', 'male').execute()");
      exec_and_out_equals("print(result.affectedRows)", "2");

      exec_and_out_equals("records = collection.find().execute().all()");
      exec_and_out_equals("print(len(records))", "4");
    }

    {
      SCOPED_TRACE("Testing full remove");
      exec_and_out_equals("result = collection.remove().execute()");
      exec_and_out_equals("print(result.affectedRows)", "4");

      exec_and_out_equals("records = collection.find().execute().all()");
      exec_and_out_equals("print(len(records))", "0");
    }

    exec_and_out_equals("session.close();");
  }
}