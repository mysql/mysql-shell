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
  class Shell_js_mysqlx_collection_remove_tests : public Crud_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_JScript, initilaized);

      // Sets the correct functions to be validated
      set_functions("remove, orderBy, limit, bind, execute");
    }
  };

  TEST_F(Shell_js_mysqlx_collection_remove_tests, initialization)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");

    exec_and_out_equals("session.executeSql('drop schema if exists js_shell_test;')");
    exec_and_out_equals("session.executeSql('create schema js_shell_test;')");
    exec_and_out_equals("session.executeSql('use js_shell_test;')");
    exec_and_out_equals("session.executeSql(\"create table `collection1`(`doc` JSON, `_id` VARBINARY(16) GENERATED ALWAYS AS(unhex(json_unquote(json_extract(doc, '$._id')))) stored PRIMARY KEY)\")");
  }

  TEST_F(Shell_js_mysqlx_collection_remove_tests, chain_combinations)
  {
    // NOTE: No data validation is done on this test, only tests
    //       different combinations of chained methods.

    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.openSession('" + _uri + "');");

    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    // Creates the collection find object
    exec_and_out_equals("var crud = collection.remove();");

    //-------- ----------------------------------------------------
    // Tests the happy path validating only the right functions
    // are available following the chained call
    // this is CollectionRemove.remove().limit(#).execute()
    //-------------------------------------------------------------
    // TODO: Function availability needs to be tested on after the rest of the functions
    //       once they are enabled
    {
      SCOPED_TRACE("Testing function availability after remove.");
      ensure_available_functions("orderBy, limit, bind, execute");
    }

    // Now executes limit
    {
      SCOPED_TRACE("Testing function availability after limit.");
      exec_and_out_equals("crud.limit(1)");
      ensure_available_functions("bind, execute");
    }
  }

  TEST_F(Shell_js_mysqlx_collection_remove_tests, remove_validations)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");
    exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");
    exec_and_out_equals("var schema = session.getSchema('js_shell_test');");
    exec_and_out_equals("var collection = schema.getCollection('collection1');");

    // Testing the remove function
    {
      SCOPED_TRACE("Testing parameter validation on remove");
      exec_and_out_equals("collection.remove();");
      exec_and_out_contains("collection.remove(5);", "", "CollectionRemove::remove: string parameter required.");
      exec_and_out_contains("collection.remove('test = \"2');", "", "CollectionRemove::remove: Unterminated quoted string starting at 8");
      exec_and_out_equals("collection.remove('test = \"2\"');");
    }

    {
      SCOPED_TRACE("Testing parameter validation on orderBy");
      exec_and_out_contains("collection.remove().orderBy();", "", "Invalid number of arguments in CollectionRemove::orderBy");
      exec_and_out_contains("collection.remove().orderBy(5);", "", "CollectionRemove::orderBy: string parameter required.");
    }

    {
      SCOPED_TRACE("Testing parameter validation on limit");
      exec_and_out_contains("collection.remove().limit();", "", "Invalid number of arguments in CollectionRemove::limit");
      exec_and_out_contains("collection.remove().limit('');", "", "CollectionRemove::limit: integer parameter required.");
      exec_and_out_equals("collection.remove().limit(5);");
    }

    exec_and_out_contains("collection.remove().bind();", "", "CollectionRemove::bind: not yet implemented.");
  }

  TEST_F(Shell_js_mysqlx_collection_remove_tests, remove_execution)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");
    exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");
    exec_and_out_equals("var schema = session.getSchema('js_shell_test');");
    exec_and_out_equals("var collection = schema.getCollection('collection1');");

    exec_and_out_equals("var result = collection.add({name: 'jack', age: 17, gender: 'male'}).execute();");
    exec_and_out_equals("var result = collection.add({name: 'adam', age: 15, gender: 'male'}).execute();");
    exec_and_out_equals("var result = collection.add({name: 'brian', age: 14, gender: 'male'}).execute();");
    exec_and_out_equals("var result = collection.add({name: 'alma', age: 13, gender: 'female'}).execute();");
    exec_and_out_equals("var result = collection.add({name: 'carol', age: 14, gender: 'female'}).execute();");
    exec_and_out_equals("var result = collection.add({name: 'donna', age: 16, gender: 'female'}).execute();");
    exec_and_out_equals("var result = collection.add({name: 'angel', age: 14, gender: 'male'}).execute();");

    {
      SCOPED_TRACE("Testing remove");
      exec_and_out_equals("var result = collection.remove('age = 13').execute();");
      exec_and_out_equals("print(result.affectedRows)", "1");

      exec_and_out_equals("var records = collection.find().execute().all();");
      exec_and_out_equals("print(records.length);", "6");
    }

    {
      SCOPED_TRACE("Testing remove with limits");
      exec_and_out_equals("var result = collection.remove('gender = \"male\"').limit(2).execute();");
      exec_and_out_equals("print(result.affectedRows)", "2");

      exec_and_out_equals("var records = collection.find().execute().all();");
      exec_and_out_equals("print(records.length);", "4");
    }

    {
      SCOPED_TRACE("Testing full remove");
      exec_and_out_equals("var result = collection.remove().execute();");
      exec_and_out_equals("print(result.affectedRows)", "4");

      exec_and_out_equals("var records = collection.find().execute().all();");
      exec_and_out_equals("print(records.length);", "0");
    }
  }
}