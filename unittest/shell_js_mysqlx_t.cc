/*
* Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of the
* License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301  USA
*/

#include "shell_script_tester.h"
#include "utils/utils_general.h"

namespace shcore
{
  class Shell_js_mysqlx_tests : public Shell_js_script_tester
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_js_script_tester::SetUp();

      int port = 33060, pwd_found;
      std::string protocol, user, password, host, sock, schema, ssl_ca, ssl_cert, ssl_key;
      shcore::parse_mysql_connstring(_uri, protocol, user, password, host, port, sock, schema, pwd_found, ssl_ca, ssl_cert, ssl_key);

      std::string code = "var __user = '" + user + "';";
      exec_and_out_equals(code);
      code = "var __pwd = '" + password + "';";
      exec_and_out_equals(code);
      code = "var __host = '" + host + "';";
      exec_and_out_equals(code);
      code = "var __port = 33060;";
      exec_and_out_equals(code);
      code = "var __schema = 'mysql';";
      exec_and_out_equals(code);
      code = "var __uri = '" + user + "@" + host + ":33060';";
      exec_and_out_equals(code);
      code = "var __uripwd = '" + user + ":" + password + "@" + host + ":33060';";
      exec_and_out_equals(code);
      code = "var __displayuri = '" + user + "@" + host + ":33060';";
      exec_and_out_equals(code);
      code = "var __displayuridb = '" + user + "@" + host + ":33060/mysql';";
      exec_and_out_equals(code);

      // All of the test cases share the same config folder
      // and setup script
      set_config_folder("js_devapi");
      set_setup_script("setup.js");
    }
  };

  TEST_F(Shell_js_mysqlx_tests, mysqlx_module)
  {
    validate_interactive("mysqlx_module.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_session)
  {
    validate_interactive("mysqlx_session.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_schema)
  {
    validate_interactive("mysqlx_schema.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_table)
  {
    validate_interactive("mysqlx_table.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_table_insert)
  {
    validate_interactive("mysqlx_table_insert.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_table_select)
  {
    validate_interactive("mysqlx_table_select.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_table_update)
  {
    validate_interactive("mysqlx_table_update.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_table_delete)
  {
    validate_interactive("mysqlx_table_delete.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_collection)
  {
    validate_interactive("mysqlx_collection.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_collection_add)
  {
    validate_interactive("mysqlx_collection_add.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_collection_find)
  {
    validate_interactive("mysqlx_collection_find.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_collection_modify)
  {
    validate_interactive("mysqlx_collection_modify.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_collection_remove)
  {
    validate_interactive("mysqlx_collection_remove.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_collection_create_index)
  {
    validate_interactive("mysqlx_collection_create_index.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_view)
  {
    validate_interactive("mysqlx_view.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_resultset)
  {
    validate_interactive("mysqlx_resultset.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_column_metadata)
  {
    validate_interactive("mysqlx_column_metadata.js");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_admin_session)
  {
    validate_interactive("mysqlx_admin_session.js");
  }
}