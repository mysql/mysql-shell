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
  class DISABLED_Shell_js_crud_table_insert_tests : public Crud_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_JScript, initilaized);

      set_functions("insert,values,bind,execute");
    }
  };

  TEST_F(DISABLED_Shell_js_crud_table_insert_tests, chain_combinations)
  {
    // Creates a connection object
    exec_and_out_equals("var conn = _F.mysqlx.Connection('" + _uri + "');");

    // Creates the table insert object
    exec_and_out_equals("var crud = _F.mysqlx.TableInsert(conn, 'schema', 'table');");

    //-------- ---------------------Test 1------------------------//
    // Initial validation, any new TableInsert object only has
    // the insert function available upon creation
    //-------------------------------------------------------------
    ensure_available_functions("insert");

    //-------- ---------------------Test 2-------------------------
    // Tests the happy path validating only the right functions
    // are available following the chained call
    // this is TableInsert.insert([]).values([]).bind([]).execute()
    //-------------------------------------------------------------
    exec_and_out_equals("crud.insert([])");
    ensure_available_functions("values,bind,execute");

    // Executes values and it will ensure the same methods still available since
    // values can be repeated
    exec_and_out_equals("crud.values([])");
    ensure_available_functions("values,bind,execute");

    // Now executes bind and the only available method will be execute
    exec_and_out_equals("crud.bind([])");
    ensure_available_functions("execute");

    //-------- ---------------------Test 3-------------------------
    // Test the case when insert is called without parameters, on such case
    // execute should not be enabled until values or bind is called
    //-------------------------------------------------------------
    // Creates a new table insert object
    exec_and_out_equals("var crud = _F.mysqlx.TableInsert(conn, 'schema', 'table');");
    exec_and_out_equals("crud.insert()");
    ensure_available_functions("values,bind");

    //-------- ---------------------Test 4-------------------------
    // Test the case when insert is called FieldsAndValues, on such case
    // values should be disabled
    //-------------------------------------------------------------
    // Creates a new table insert object
    exec_and_out_equals("var crud = _F.mysqlx.TableInsert(conn, 'schema', 'table');");
    exec_and_out_equals("crud.insert({})");
    ensure_available_functions("bind,execute");

    //-------- ---------------------Test 5-------------------------
    // Test the case when insert and bind are called, ensures methods
    // behind bind are no longer enabled
    //-------------------------------------------------------------
    // Creates a new table insert object
    exec_and_out_equals("var crud = _F.mysqlx.TableInsert(conn, 'schema', 'table');");
    exec_and_out_equals("crud.insert([]).bind([])");
    ensure_available_functions("execute");
  }
}