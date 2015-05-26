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

#include "../modules/mod_crud_collection_add.h"

namespace shcore {
  class Shell_js_crud_collection_add_tests : public Crud_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_JScript, initilaized);

      // Sets the correct functions to be validated
      set_functions("add bind execute");
    }
  };

  TEST_F(Shell_js_crud_collection_add_tests, chain_combinations)
  {
    // NOTE: No data validation is done on this test, only tests
    //       different combinations of chained methods.

    // Creates a connection object
    exec_and_out_equals("var conn = _F.mysqlx.Connection('" + _uri + "');");

    // Creates the collection add object
    exec_and_out_equals("var crud = _F.mysqlx.CollectionAdd(conn, 'schema', 'collection');");

    //-------- ---------------------Test 1------------------------//
    // Initial validation, any new CollectionAdd object only has
    // the add function available upon creation
    //-------------------------------------------------------------
    ensure_available_functions("add");

    //-------- ---------------------Test 2-------------------------
    // Tests the happy path validating only the right functions
    // are available following the chained call
    // this is ColumnAdd.add([]).bind([]).execute()
    //-------------------------------------------------------------
    exec_and_out_equals("crud.add([])");
    ensure_available_functions("bind execute");

    // Now executes bind and the only available method will be execute
    exec_and_out_equals("crud.bind([])");
    ensure_available_functions("execute");

    //-------- ---------------------Test 3-------------------------
    // Tests an alternative where add is called with no parameters
    // on this case bind is not optional but mandatory so after add
    // execute should not be available
    //-------------------------------------------------------------

    // Creates the collection add object
    exec_and_out_equals("var crud = _F.mysqlx.CollectionAdd(conn, 'schema', 'collection');");

    // Executes insert + bind, values  only execute should be available after
    exec_and_out_equals("crud.add()");
    ensure_available_functions("bind");
  }

  TEST_F(Shell_js_crud_collection_add_tests, add_validations)
  {
    exec_and_out_equals("var mysqlx = require('myx').mysqlx;");
    exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");
    exec_and_out_equals("session.executeSql('drop schema if exists js_shell_test;');");
    exec_and_out_equals("session.executeSql('create schema js_shell_test;');");
    exec_and_out_equals("session.executeSql('use js_shell_test;');");
    exec_and_out_equals("session.executeSql(\"create table `mycoll`(`doc` JSON, `_id` VARBINARY(16) GENERATED ALWAYS AS(unhex(jsn_unquote(jsn_extract(doc, '$._id')))) stored PRIMARY KEY)\")");

    exec_and_out_equals("var schema = session.getSchema('js_shell_test');");
    exec_and_out_equals("var collection = schema.getCollection('mycoll');");

    // Test add attempt with no data
    exec_and_out_contains("collection.add().bind().execute();", "", "No documents specified on add operation.");

    // Test add attempt with non document
    exec_and_out_contains("collection.add(45).bind().execute();", "", "Invalid document specified on add operation.");

    // Test add collection with invalid document
    exec_and_out_contains("collection.add(['invalid data']).bind().execute();", "", "Invalid document specified on list for add operation.");

    // Test adding a single document
    exec_and_out_equals("var result = collection.add({name: 'my first', passed: 'document', count: 1}).bind().execute();");
    exec_and_out_equals("print (result.affected_rows)", "1");

    // Test adding multiple documents
    exec_and_out_equals("var result = collection.add([{name: 'my second', passed: 'again', count: 2}, {name: 'my third', passed: 'once again', cound: 4}]).bind().execute();");
    exec_and_out_equals("print (result.affected_rows)", "2");

    exec_and_out_equals("session.executeSql('drop schema js_shell_test;');");
  }
}