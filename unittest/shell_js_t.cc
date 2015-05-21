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
  class Shell_js_test : public Shell_core_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_JScript, initilaized);
    }
  };

  TEST_F(Shell_js_test, require_failures)
  {
    // This module actually exists but will fail because
    // it parent folder has not been added to the module
    // directory list.
    execute("var mymodule = require('fail_wont_compile')");
    std::string expected = "Module fail_wont_compile was not found";
    EXPECT_NE(-1, int(output_handler.std_err.find(expected)));
    output_handler.wipe_all();

    // Adds a module directory
    execute("shell.js.module_paths[shell.js.module_paths.length] = './js'");
    EXPECT_TRUE(output_handler.std_out.empty());
    EXPECT_TRUE(output_handler.std_err.empty());

    execute("var mymodule = require('fail_wont_compile')");
    expected = "my_object is not defined";
    EXPECT_NE(-1, int(output_handler.std_err.find(expected)));
    output_handler.wipe_all();

    execute("var empty_module = require('fail_empty')");
    expected = "Module fail_empty is empty";
    EXPECT_NE(-1, int(output_handler.std_err.find(expected)));
    output_handler.wipe_all();
  }

  TEST_F(Shell_js_test, require_success)
  {
    std::string code;
    Interactive_input_state state;

    // Adds a module directory
    execute("shell.js.module_paths[shell.js.module_paths.length] = './js'");
    EXPECT_TRUE(output_handler.std_out.empty());
    EXPECT_TRUE(output_handler.std_err.empty());

    output_handler.wipe_all();
    execute("var my_module = require('success')");
    EXPECT_TRUE(output_handler.std_err.empty());
    EXPECT_TRUE(output_handler.std_out.empty());

    execute("print(Object.keys(my_module).length)");
    EXPECT_TRUE(output_handler.std_err.empty());
    EXPECT_EQ("1", output_handler.std_out);

    output_handler.wipe_out();
    execute("print(my_module.ok_message)");
    EXPECT_TRUE(output_handler.std_err.empty());
    EXPECT_EQ("module imported ok", output_handler.std_out);

    output_handler.wipe_out();
    execute("print(typeof unexisting)");
    EXPECT_TRUE(output_handler.std_err.empty());
    EXPECT_EQ("undefined", output_handler.std_out);

    output_handler.wipe_out();
    execute("print(in_global_context)");
    EXPECT_TRUE(output_handler.std_err.empty());
    EXPECT_EQ("This is here because of JS", output_handler.std_out);
  }

  TEST_F(Shell_js_test, crud_table_insert_chaining)
  {
    std::string xuri;
    const char *uri = getenv("MYSQL_URI");
    if (uri)
      xuri.assign(uri);

    // Creates a connection object
    exec_and_out_equals("var conn = _F.mysqlx.Connection('" + xuri + "');");

    // Creates the table insert object
    exec_and_out_equals("var ti = _F.mysqlx.TableInsert(conn, 'schema', 'table');");

    // Gets the available functions on creation
    exec_and_out_equals("var functs = Object.keys(ti);");

    // Tets the only available function is insert
    exec_and_out_equals("print(functs.length)", "1");
    exec_and_out_equals("print(functs.indexOf('insert') == 0)", "true");
    exec_and_out_equals("print(functs.indexOf('values') == -1)", "true");
    exec_and_out_equals("print(functs.indexOf('bind') == -1)", "true");
    exec_and_out_equals("print(functs.indexOf('run') == -1)", "true");

    // Tets only insert can be called
    exec_and_out_contains("ti.values('');", "", "Forbidden usage of values");
    exec_and_out_contains("ti.bind('');", "", "Forbidden usage of bind");
    exec_and_out_contains("ti.run('');", "", "Forbidden usage of run");
    exec_and_out_equals("ti.insert([])");

    // Once insert is called we go to the next 'state'
    exec_and_out_equals("functs = Object.keys(ti);");
    exec_and_out_equals("print(functs.length)", "3");
    exec_and_out_equals("print(functs.indexOf('insert') == -1)", "true");
    exec_and_out_equals("print(functs.indexOf('values') != -1)", "true");
    exec_and_out_equals("print(functs.indexOf('bind') != -1)", "true");
    exec_and_out_equals("print(functs.indexOf('run') != -1)", "true");

    // Tets insert can not longer be called
    exec_and_out_contains("ti.insert([]);", "", "Forbidden usage of insert");

    // Executes values and it will ensure the same methods still available since
    // values can be repeated
    exec_and_out_equals("ti.values([])");

    exec_and_out_equals("functs = Object.keys(ti);");
    exec_and_out_equals("print(functs.length)", "3");
    exec_and_out_equals("print(functs.indexOf('insert') == -1)", "true");
    exec_and_out_equals("print(functs.indexOf('values') != -1)", "true");
    exec_and_out_equals("print(functs.indexOf('bind') != -1)", "true");
    exec_and_out_equals("print(functs.indexOf('run') != -1)", "true");

    // Tets insert can not longer be called
    exec_and_out_contains("ti.insert([]);", "", "Forbidden usage of insert");

    // Now executes bind and the only available method will be run
    exec_and_out_equals("ti.bind([])");

    exec_and_out_equals("functs = Object.keys(ti);");
    exec_and_out_equals("print(functs.length)", "1");
    exec_and_out_equals("print(functs.indexOf('insert') == -1)", "true");
    exec_and_out_equals("print(functs.indexOf('values') == -1)", "true");
    exec_and_out_equals("print(functs.indexOf('bind') == -1)", "true");
    exec_and_out_equals("print(functs.indexOf('run') != -1)", "true");

    // Tets the other methods can't longer be called
    exec_and_out_contains("ti.insert([]);", "", "Forbidden usage of insert");
    exec_and_out_contains("ti.values([]);", "", "Forbidden usage of values");
    exec_and_out_contains("ti.bind([]);", "", "Forbidden usage of bind");

    // Creates the table insert object
    exec_and_out_equals("var ti = _F.mysqlx.TableInsert(conn, 'schema', 'table');");

    // Executes insert passing FieldsAndValues and the Values method should not
    // be available after
    exec_and_out_equals("ti.insert({})");

    // Once insert is called we go to the next 'state'
    exec_and_out_equals("functs = Object.keys(ti);");
    exec_and_out_equals("print(functs.length)", "2");
    exec_and_out_equals("print(functs.indexOf('insert') == -1)", "true");
    exec_and_out_equals("print(functs.indexOf('values') == -1)", "true");
    exec_and_out_equals("print(functs.indexOf('bind') != -1)", "true");
    exec_and_out_equals("print(functs.indexOf('run') != -1)", "true");

    exec_and_out_contains("ti.insert([]);", "", "Forbidden usage of insert");
    exec_and_out_contains("ti.values([]);", "", "Forbidden usage of values");

    // Creates the table insert object
    exec_and_out_equals("var ti = _F.mysqlx.TableInsert(conn, 'schema', 'table');");

    // Executes insert + bind, values  only run should be available after
    exec_and_out_equals("ti.insert([]).bind([])");

    // Once insert is called we go to the next 'state'
    exec_and_out_equals("functs = Object.keys(ti);");
    exec_and_out_equals("print(functs.length)", "1");
    exec_and_out_equals("print(functs.indexOf('insert') == -1)", "true");
    exec_and_out_equals("print(functs.indexOf('values') == -1)", "true");
    exec_and_out_equals("print(functs.indexOf('bind') == -1)", "true");
    exec_and_out_equals("print(functs.indexOf('run') != -1)", "true");

    exec_and_out_contains("ti.insert([]);", "", "Forbidden usage of insert");
    exec_and_out_contains("ti.values([]);", "", "Forbidden usage of values");
    exec_and_out_contains("ti.bind([]);", "", "Forbidden usage of bind");
  }
}