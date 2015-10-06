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

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "test_utils.h"
#include "base_session.h"

namespace shcore {
  class Shell_py_mysqlx_schema_tests : public Shell_core_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_Python, initilaized);

      exec_and_out_equals("import mysqlx");

      exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "')");

      exec_and_out_equals("session.sql('drop schema if exists py_shell_test;').execute()");
      exec_and_out_equals("session.createSchema('py_shell_test');");
      exec_and_out_equals("session.setCurrentSchema('py_shell_test');");
      exec_and_out_equals("result = session.sql('create table table1 (name varchar(50));').execute()");
      exec_and_out_equals("session.sql('create view view1 (my_name) as select name from table1;').execute()");

      exec_and_out_equals("session.py_shell_test.createCollection('collection1')");
    }
  };

  // Tests schema.getName()
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_get_name)
  {
    exec_and_out_equals("print(session.mysql.getName())", "mysql");
  }

  // Tests schema.name
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_name)
  {
    exec_and_out_equals("print(session.mysql.name)", "mysql");
  }

  // Tests schema.getSession()
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_get_session)
  {
    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("mschema = session.mysql");

    exec_and_out_equals("schema_session = mschema.getSession()");

    exec_and_out_equals("print(schema_session)", "<NodeSession:" + uri + ">");
  }

  // Tests schema.session
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_session)
  {
    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("mschema = session.mysql");

    exec_and_out_equals("print(mschema.session)", "<NodeSession:" + uri + ">");
  }

  // Tests schema.getSchema()
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_get_schema)
  {
    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("mschema = session.mysql");

    exec_and_out_equals("schema_schema = schema.getSchema()");

    exec_and_out_equals("print(schema_schema)", "None");
  }

  // Tests schema.schema
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_schema)
  {
    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("mschema = session.mysql");

    exec_and_out_equals("print(mschema.schema)", "None");
  }

  // Tests schema.getTables()
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_get_tables)
  {
    exec_and_out_equals("tables = session.py_shell_test.getTables()");

    exec_and_out_equals("print(tables.table1)", "<Table:table1>");
  }

  // Tests schema.tables
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_tables)
  {
    exec_and_out_equals("print(session.py_shell_test.tables.table1)", "<Table:table1>");
  }

  // Tests schema.getViews()
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_get_views)
  {
    exec_and_out_equals("views = session.py_shell_test.getViews()");

    exec_and_out_equals("print(views.view1)", "<View:view1>");
  }

  // Tests schema.views
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_views)
  {
    exec_and_out_equals("print(session.py_shell_test.views.view1)", "<View:view1>");
  }

  // Tests schema.getCollections()
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_get_collections)
  {
    exec_and_out_equals("collections = session.py_shell_test.getCollections()");

    exec_and_out_equals("print(collections.collection1)", "<Collection:collection1>");
  }

  // Tests schema.collections
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_collections)
  {
    exec_and_out_equals("print(session.py_shell_test.collections.collection1)", "<Collection:collection1>");
  }

  // Tests schema.getTable()
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_get_table)
  {
    exec_and_out_equals("table = session.py_shell_test.getTable('table1')");

    exec_and_out_equals("print(table)", "<Table:table1>");
  }

  // Tests schema.getView()
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_get_view)
  {
    exec_and_out_equals("view = session.py_shell_test.getView('view1')");

    exec_and_out_equals("print(view)", "<View:view1>");
  }

  // Tests schema.getCollection()
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_get_collection)
  {
    exec_and_out_equals("collection = session.py_shell_test.getCollection('collection1')");

    exec_and_out_equals("print(collection)", "<Collection:collection1>");
  }

  // Tests schema.getCollectionAsTable()
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_get_collection_as_table)
  {
    exec_and_out_equals("table = session.py_shell_test.getCollectionAsTable('collection1')");

    exec_and_out_equals("print(table)", "<Table:collection1>");
  }

  // Tests schema.<object>
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_object)
  {
    exec_and_out_equals("print(session.py_shell_test.table1)", "<Table:table1>");
    exec_and_out_equals("print(session.py_shell_test.view1)", "<View:view1>");
    exec_and_out_equals("print(session.py_shell_test.collection1)", "<Collection:collection1>");
  }

  // Tests schema.createCollection()
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_create_collection)
  {
    exec_and_out_equals("collection = session.py_shell_test.createCollection('test_collection')");

    exec_and_out_equals("print(collection)", "<Collection:test_collection>");
  }

  // Tests session.dropSchema() and schema.existInDatabase()
  TEST_F(Shell_py_mysqlx_schema_tests, mysqlx_schema_drop_exist_in_database)
  {
    exec_and_out_equals("schema = session.createSchema('my_sample_schema')");

    exec_and_out_equals("print(schema.existInDatabase())", "True");

    exec_and_out_equals("print(schema.name)", "my_sample_schema");

    exec_and_out_equals("session.dropSchema('my_sample_schema')");

    exec_and_out_equals("print(schema.existInDatabase())", "False");
  }
}