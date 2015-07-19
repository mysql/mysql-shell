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

#include "../modules/mod_mysqlx_collection_modify.h"

namespace shcore {
  class Shell_js_mysqlx_collection_modify_tests : public Crud_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_JScript, initilaized);

      // Sets the correct functions to be validated
      set_functions("modify, set, change, unset, arrayInsert, arrayAppend, arrayDelete, sort, limit, bind, execute");
    }
  };

  TEST_F(Shell_js_mysqlx_collection_modify_tests, initialization)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");

    exec_and_out_equals("session.executeSql('drop schema if exists js_shell_test;')");
    exec_and_out_equals("session.executeSql('create schema js_shell_test;')");
    exec_and_out_equals("session.executeSql('use js_shell_test;')");
    exec_and_out_equals("session.executeSql(\"create table `collection1`(`doc` JSON, `_id` VARBINARY(16) GENERATED ALWAYS AS(unhex(json_unquote(json_extract(doc, '$._id')))) stored PRIMARY KEY)\")");

    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_collection_modify_tests, chain_combinations)
  {
    // NOTE: No data validation is done on this test, only tests
    //       different combinations of chained methods.

    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.getSession('" + _uri + "');");

    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    //-------- ----------------------------------------------------
    // Tests the happy path validating only the right functions
    // are available following the chained call
    // this is CollectionModify.modify().<operation>().sort().limit().bind().execute()
    //-------------------------------------------------------------
    // TODO: Function availability needs to be tested on after the rest of the functions
    //       once they are enabled
    {
      SCOPED_TRACE("Testing function availability after modify.");
      exec_and_out_equals("var crud = collection.modify();");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete");
    }

    {
      SCOPED_TRACE("Testing function availability after set.");
      exec_and_out_equals("var crud = collection.modify();");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete");

      // Empty set of values does not trigger a change
      exec_and_out_equals("crud.set({})");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete");

      // Empty set of values does not trigger a change
      exec_and_out_equals("crud.set({name:'dummy'})");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete, sort, limit, bind, execute");
    }

    {
      SCOPED_TRACE("Testing function availability after change.");
      exec_and_out_equals("var crud = collection.modify();");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete");

      exec_and_out_equals("crud.change({})");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete");

      exec_and_out_equals("crud.change({name:'another'})");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete, sort, limit, bind, execute");
    }

    {
      SCOPED_TRACE("Testing function availability after unset.");
      exec_and_out_equals("var crud = collection.modify();");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete");

      exec_and_out_equals("crud.unset([])");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete");

      exec_and_out_equals("crud.unset(['name','last_name'])");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete, sort, limit, bind, execute");

      // Tests passing multiple parameters
      exec_and_out_equals("var crud = collection.modify();");
      exec_and_out_equals("crud.unset('name','last_name')");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete, sort, limit, bind, execute");
    }

    {
      SCOPED_TRACE("Testing function availability after arrayInsert.");
      exec_and_out_equals("var crud = collection.modify();");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete");

      exec_and_out_equals("crud.arrayInsert('hobbies',3,'run')");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete, sort, limit, bind, execute");
    }

    {
      SCOPED_TRACE("Testing function availability after arrayAppend.");
      exec_and_out_equals("var crud = collection.modify();");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete");

      exec_and_out_equals("crud.arrayAppend('hobbies','skate')");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete, sort, limit, bind, execute");
    }

    {
      SCOPED_TRACE("Testing function availability after arrayDelete.");
      exec_and_out_equals("var crud = collection.modify();");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete");

      exec_and_out_equals("crud.arrayDelete('hobbies',5)");
      ensure_available_functions("set, change, unset, arrayInsert, arrayAppend, arrayDelete, sort, limit, bind, execute");
    }

    {
      SCOPED_TRACE("Testing function availability after sort.");
      exec_and_out_equals("crud.sort('')");
      ensure_available_functions("limit, bind, execute");
    }

    {
      SCOPED_TRACE("Testing function availability after limit.");
      exec_and_out_equals("crud.limit(2)");
      ensure_available_functions("bind, execute");
    }

    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_collection_modify_tests, modify_validations)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");
    exec_and_out_equals("var session = mysqlx.getSession('" + _uri + "');");
    exec_and_out_equals("var schema = session.getSchema('js_shell_test');");
    exec_and_out_equals("var collection = schema.getCollection('collection1');");

    {
      SCOPED_TRACE("Testing parameter validation on modify");
      exec_and_out_equals("collection.modify();");
      exec_and_out_contains("collection.modify(5);", "", "CollectionModify::modify: Argument #1 is expected to be a string");
      exec_and_out_contains("collection.modify('test = \"2');", "", "CollectionModify::modify: Unterminated quoted string starting at 8");
      exec_and_out_equals("collection.modify('test = \"2\"');");
    }

    {
      SCOPED_TRACE("Testing parameter validation on set");
      exec_and_out_contains("collection.modify().set();", "", "Invalid number of arguments in CollectionModify::set, expected 1 but got 0");
      exec_and_out_contains("collection.modify().set([]);", "", "CollectionModify::set: Argument #1 is expected to be a map");
      exec_and_out_contains("collection.modify().set({name:session});", "", "CollectionModify::set: Unsupported value received: <Session:");
      exec_and_out_contains("collection.modify().set({age:expr('13+25')});");
      exec_and_out_equals("collection.modify().set({name:'john'});");
    }

    {
      SCOPED_TRACE("Testing parameter validation on change");
      exec_and_out_contains("collection.modify().change();", "", "Invalid number of arguments in CollectionModify::change, expected 1 but got 0");
      exec_and_out_contains("collection.modify().change([]);", "", "CollectionModify::change: Argument #1 is expected to be a map");
      exec_and_out_contains("collection.modify().change({name:session});", "", "CollectionModify::change: Unsupported value received: <Session:");
      exec_and_out_contains("collection.modify().change({age:expr('13+25')});");
      exec_and_out_equals("collection.modify().change({name:'john'});");
    }

    {
      SCOPED_TRACE("Testing parameter validation on unset");
      exec_and_out_contains("collection.modify().unset();", "", "Invalid number of arguments in CollectionModify::unset, expected at least 1 but got 0");
      exec_and_out_contains("collection.modify().unset({});", "", "CollectionModify::unset: Argument #1 is expected to be either string or list of strings");
      exec_and_out_contains("collection.modify().unset({}, '');", "", "CollectionModify::unset: Argument #1 is expected to be a string");
      exec_and_out_contains("collection.modify().unset(['name', 1]);", "", "CollectionModify::unset: Element #2 is expected to be a string");
      exec_and_out_equals("collection.modify().unset(['name','age']);");
      exec_and_out_equals("collection.modify().unset('name','age');");
    }

    {
      SCOPED_TRACE("Testing parameter validation on arrayInsert");
      exec_and_out_contains("collection.modify().arrayInsert();", "", "Invalid number of arguments in CollectionModify::arrayInsert, expected 3 but got 0");
    }

    {
      SCOPED_TRACE("Testing parameter validation on arrayAppend");
      exec_and_out_contains("collection.modify().arrayAppend();", "", "Invalid number of arguments in CollectionModify::arrayAppend, expected 2 but got 0");
      exec_and_out_contains("collection.modify().arrayAppend({}, '');", "", "CollectionModify::arrayAppend: Argument #1 is expected to be a string");
      exec_and_out_contains("collection.modify().arrayAppend('data', session);", "", "CollectionModify::arrayAppend: Unsupported value received: <Session:");
      exec_and_out_contains("collection.modify().arrayAppend('data', 5);");
      exec_and_out_contains("collection.modify().arrayAppend('data', 'sample');");
      exec_and_out_contains("collection.modify().arrayAppend('data', expr(15+3));");
    }

    {
      SCOPED_TRACE("Testing parameter validation on arrayDelete");
      exec_and_out_contains("collection.modify().arrayDelete();", "", "Invalid number of arguments in CollectionModify::arrayDelete, expected 2 but got 0");
    }

    {
      SCOPED_TRACE("Testing parameter validation on sort");
      exec_and_out_contains("collection.modify().unset('name').sort();", "", "Invalid number of arguments in CollectionModify::sort, expected 1 but got 0");
      exec_and_out_contains("collection.modify().unset('name').sort(5);", "", "CollectionModify::sort: Argument #1 is expected to be a string");
    }

    {
      SCOPED_TRACE("Testing parameter validation on limit");
      exec_and_out_contains("collection.modify().unset('name').limit();", "", "Invalid number of arguments in CollectionModify::limit, expected 1 but got 0");
      exec_and_out_contains("collection.modify().unset('name').limit('');", "", "CollectionModify::limit: Argument #1 is expected to be an unsigned int");
      exec_and_out_equals("collection.remove().limit(5);");
    }

    exec_and_out_contains("collection.remove().bind();", "", "CollectionRemove::bind: not yet implemented.");

    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_collection_modify_tests, DISABLED_modify_execution)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");
    exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");
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
      exec_and_out_equals("var result = collection.modify('age = 13').unset('gender').execute();");
      exec_and_out_equals("print(result.affectedRows)", "1");

      exec_and_out_equals("var records = collection.find().execute().all();");
      exec_and_out_equals("print(records.length);", "6");
    }

    {
      SCOPED_TRACE("Testing set");
      exec_and_out_equals("var result = collection.modify('age = 13').set({gender:'male', age:expr('13+1')}).execute();");
      exec_and_out_equals("print(result.affectedRows)", "1");

      exec_and_out_equals("var records = collection.find().execute().all();");
      exec_and_out_equals("print(records.length);", "6");
    }

    {
      SCOPED_TRACE("Testing change");
      exec_and_out_equals("var result = collection.modify('age = 13').change({gender:'male', age:expr('13+1')}).execute();");
      exec_and_out_equals("print(result.affectedRows)", "1");

      exec_and_out_equals("var records = collection.find().execute().all();");
      exec_and_out_equals("print(records.length);", "6");
    }

    {
      SCOPED_TRACE("Testing array append");
      exec_and_out_equals("var result = collection.add({name: 'xavier', age: 14, gender: 'male', hobbies:['soccer', 'guitar']}).execute();");

      exec_and_out_equals("var result = collection.modify('name = \"xavier\"').arrayAppend('hobbies','reading').execute();");
      exec_and_out_equals("print(result.affectedRows)", "1");

      exec_and_out_equals("var records = collection.find().execute().all();");
      exec_and_out_equals("print(records.length);", "6");
    }

    /*
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
    }*/

    exec_and_out_equals("session.close();");
  }
}