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

#include "../modules/mod_mysqlx_collection_add.h"

namespace shcore {
  class Shell_js_mysqlx_collection_add_tests : public Crud_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_JScript, initilaized);

      // Sets the correct functions to be validated
      set_functions("add execute __shell_hook__");
    }
  };

  TEST_F(Shell_js_mysqlx_collection_add_tests, initialization)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");

    exec_and_out_equals("session.sql('drop schema if exists js_shell_test;').execute();");
    exec_and_out_equals("session.createSchema('js_shell_test');");
    exec_and_out_equals("session.js_shell_test.createCollection('collection1')");

    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_collection_add_tests, chain_combinations)
  {
    // NOTE: No data validation is done on this test, only tests
    //       different combinations of chained methods.
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");
    exec_and_out_equals("var session = mysqlx.getSession('" + _uri + "');");
    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    // Creates the collection find object
    exec_and_out_equals("var crud = collection.add([]);");

    ensure_available_functions("add, execute, __shell_hook__");

    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_collection_add_tests, add_validations)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");
    exec_and_out_equals("var session = mysqlx.getSession('" + _uri + "');");
    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    // Test add attempt with no data
    exec_and_out_contains("collection.add().execute();", "", "Invalid number of arguments in CollectionAdd.add, expected 1 but got 0");

    // Test add attempt with non document
    exec_and_out_contains("collection.add(45).execute();", "", "CollectionAdd.add: Argument is expected to be either a document or a list of documents");

    // Test add collection with invalid document
    exec_and_out_contains("collection.add(['invalid data']).execute();", "", "CollectionAdd.add: Element #1 is expected to be a document");

    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_collection_add_tests, add_execution)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");
    exec_and_out_equals("var session = mysqlx.getSession('" + _uri + "');");
    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    // Test adding a single document
    exec_and_out_equals("var result = collection.add({name: 'my first', passed: 'document', count: 1}).execute();");
    exec_and_out_equals("print (result.affectedRows)", "1");

    // Test adding multiple documents
    exec_and_out_equals("var result = collection.add([{name: 'my second', passed: 'again', count: 2}, {name: 'my third', passed: 'once again', count: 3}]).execute();");
    exec_and_out_equals("print (result.affectedRows)", "2");

    // Test adding multiple documents with chained adds
    exec_and_out_equals("var result = collection.add({name: 'my fourth', passed: 'again', count: 4}).add({name: 'my fifth', passed: 'once again', count: 5}).execute();");
    exec_and_out_equals("print (result.affectedRows)", "2");

    exec_and_out_equals("session.close();");
  }
}