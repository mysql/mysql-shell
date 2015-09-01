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
  class Shell_js_mysqlx_collection_tests : public Shell_core_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_JScript, initilaized);

      exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

      exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");

      exec_and_out_equals("session.sql('drop schema if exists js_shell_test;').execute();");
      exec_and_out_equals("session.sql('create schema js_shell_test;').execute()");

      exec_and_out_equals("session.js_shell_test.createCollection('collection1')");
    }
  };

  // Tests collection.getName()
  TEST_F(Shell_js_mysqlx_collection_tests, mysqlx_collection_get_name)
  {
    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    exec_and_out_equals("print(collection.getName());", "collection1");
  }

  // Tests collection.name
  TEST_F(Shell_js_mysqlx_collection_tests, mysqlx_collection_name)
  {
    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    exec_and_out_equals("print(collection.name);", "collection1");
  }

  // Tests collection.getSession()
  TEST_F(Shell_js_mysqlx_collection_tests, mysqlx_collection_get_session)
  {
    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    exec_and_out_equals("var collection_session = collection.getSession();");

    exec_and_out_equals("print(collection_session)", "<NodeSession:" + uri + ">");
  }

  // Tests collection.session
  TEST_F(Shell_js_mysqlx_collection_tests, mysqlx_collection_session)
  {
    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    exec_and_out_equals("print(collection.session)", "<NodeSession:" + uri + ">");
  }

  // Tests collection.getSchema()
  TEST_F(Shell_js_mysqlx_collection_tests, mysqlx_collection_get_schema)
  {
    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    exec_and_out_equals("var collection_schema = collection.getSchema();");

    exec_and_out_equals("print(collection_schema)", "<Schema:js_shell_test>");
  }

  // Tests collection.schema
  TEST_F(Shell_js_mysqlx_collection_tests, mysqlx_collection_schema)
  {
    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    exec_and_out_equals("print(collection.schema)", "<Schema:js_shell_test>");
  }

  // Tests collection.drop() and collection.existInDatabase()
  TEST_F(Shell_js_mysqlx_collection_tests, mysqlx_collection_drop_exist_in_database)
  {
    exec_and_out_equals("var schema = session.createSchema('my_sample_schema');");

    exec_and_out_equals("var collection = schema.createCollection('my_sample_collection');");

    exec_and_out_equals("print(collection.existInDatabase());", "true");

    exec_and_out_equals("collection.drop();");

    exec_and_out_equals("print(collection.existInDatabase());", "false");

    exec_and_out_equals("schema.drop();");
  }
}