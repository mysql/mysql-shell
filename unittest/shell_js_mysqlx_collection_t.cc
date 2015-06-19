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
    }
  };

  TEST_F(Shell_js_mysqlx_collection_tests, initialization)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");

    exec_and_out_equals("session.executeSql('drop schema if exists js_shell_test;')");
    exec_and_out_equals("session.executeSql('create schema js_shell_test;')");
    exec_and_out_equals("session.executeSql('use js_shell_test;')");
    exec_and_out_equals("session.executeSql(\"create table `collection1`(`doc` JSON, `_id` VARBINARY(16) GENERATED ALWAYS AS(unhex(json_unquote(json_extract(doc, '$._id')))) stored PRIMARY KEY)\")");
  }

  // Tests collection.getName()
  TEST_F(Shell_js_mysqlx_collection_tests, mysqlx_collection_get_name)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.openSession('" + _uri + "');");

    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    exec_and_out_equals("print(collection.getName());", "collection1");
  }

  // Tests collection.name
  TEST_F(Shell_js_mysqlx_collection_tests, mysqlx_collection_name)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.openSession('" + _uri + "');");

    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    exec_and_out_equals("print(collection.name);", "collection1");
  }

  // Tests collection.getSession()
  TEST_F(Shell_js_mysqlx_collection_tests, mysqlx_collection_get_session)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var session = mysqlx.openSession('" + _uri + "');");

    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    exec_and_out_equals("var collection_session = collection.getSession();");

    exec_and_out_equals("print(collection_session)", "<Session:" + uri + ">");
  }

  // Tests collection.session
  TEST_F(Shell_js_mysqlx_collection_tests, mysqlx_collection_session)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var session = mysqlx.openSession('" + _uri + "');");

    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    exec_and_out_equals("print(collection.session)", "<Session:" + uri + ">");
  }

  // Tests collection.getSchema()
  TEST_F(Shell_js_mysqlx_collection_tests, mysqlx_collection_get_schema)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var session = mysqlx.openSession('" + _uri + "');");

    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    exec_and_out_equals("var collection_schema = collection.getSchema();");

    exec_and_out_equals("print(collection_schema)", "<Schema:js_shell_test>");
  }

  // Tests collection.schema
  TEST_F(Shell_js_mysqlx_collection_tests, mysqlx_collection_schema)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var session = mysqlx.openSession('" + _uri + "');");

    exec_and_out_equals("var collection = session.js_shell_test.getCollection('collection1');");

    exec_and_out_equals("print(collection.schema)", "<Schema:js_shell_test>");
  }
}