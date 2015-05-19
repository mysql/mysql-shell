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

#include "gtest/gtest.h"
#include "test_utils.h"

namespace shcore {
  class Shell_js_mysqlx_tests : public Shell_core_test_wrapper
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

  TEST_F(Shell_js_mysqlx_tests, mysqlx_exports)
  {
    execute("print(shell.js.module_paths);");
    wipe_all();
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");
    exec_and_out_equals("var exports = dir(mysqlx);");
    exec_and_out_equals("print(exports.length);", "8");

    exec_and_out_equals("print(typeof mysqlx.openSession);", "function");
    exec_and_out_equals("print(typeof mysqlx.openNodeSession);", "function");
    exec_and_out_equals("print(typeof mysqlx.Session);", "function");
    exec_and_out_equals("print(typeof mysqlx.NodeSession);", "function");
    exec_and_out_equals("print(typeof mysqlx.Schema);", "function");
    exec_and_out_equals("print(typeof mysqlx.Table);", "function");
    exec_and_out_equals("print(typeof mysqlx.Collection);", "function");
    exec_and_out_equals("print(typeof mysqlx.View);", "function");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_open_session)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    exec_and_out_equals("var session = mysqlx.openSession('" + _uri + "');");
    exec_and_out_equals("print(session instanceof mysqlx.Session);", "true");
    exec_and_out_equals("print(session instanceof mysqlx.NodeSession);", "false");

    // Ensures the right members exist
    exec_and_out_equals("var members = dir(session);");
    exec_and_out_equals("print(members.length)", "6");
    exec_and_out_equals("print(typeof session.getSchema);", "function");
    exec_and_out_equals("print(typeof session.getSchemas);", "function");
    exec_and_out_equals("print(typeof session.getDefaultSchema);", "function");

    exec_and_out_equals("print(typeof session.executeSql);", "undefined");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_open_node_session)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");
    exec_and_out_equals("print(session instanceof mysqlx.Session);", "false");
    exec_and_out_equals("print(session instanceof mysqlx.NodeSession);", "true");

    // Ensures the right members exist
    exec_and_out_equals("var members = dir(session);");
    exec_and_out_equals("print(members.length)", "7");
    exec_and_out_equals("print(typeof session.getSchema);", "function");
    exec_and_out_equals("print(typeof session.getSchemas);", "function");
    exec_and_out_equals("print(typeof session.getDefaultSchema);", "function");

    exec_and_out_equals("print(typeof session.executeSql);", "function");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_base_session_get_default_schema)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");

    // Attempts to get the default schema
    exec_and_out_equals("var schema = session.getDefaultSchema();");

    // TODO: This is returning a null object and is threated as object in JS
    //       should it be a null value??
    exec_and_out_equals("print(schema);", "null");

    // Sets a default database
    exec_and_out_equals("session.executeSql('use mysql;');");

    // Attempts accessing the default database by name
    exec_and_out_equals("print(session.mysql)", "undefined");

    // Now uses the formal method
    exec_and_out_equals("var schema = session.getDefaultSchema();");
    exec_and_out_equals("print(typeof schema);", "object");
    exec_and_out_equals("print(schema instanceof mysqlx.Schema);", "true");
    exec_and_out_equals("print(schema.name);", "mysql");

    // Now checks the object can be accessed directly
    // because has been already set as a session attribute
    exec_and_out_equals("print(typeof session.mysql);", "object");
    exec_and_out_equals("print(session.mysql instanceof mysqlx.Schema);", "true");
    exec_and_out_equals("print(session.mysql.name);", "mysql");

    // Sets a different default database
    exec_and_out_equals("session.executeSql('use information_schema;');");

    // Now uses the formal method
    exec_and_out_equals("var schema = session.getDefaultSchema();");
    exec_and_out_equals("print(typeof schema);", "object");
    exec_and_out_equals("print(schema instanceof mysqlx.Schema);", "true");
    exec_and_out_equals("print(schema.name);", "information_schema");

    // Now checks the object can be accessed directly
    // because has been already set as a session attribute
    exec_and_out_equals("print(typeof session.information_schema);", "object");
    exec_and_out_equals("print(session.information_schema instanceof mysqlx.Schema);", "true");
    exec_and_out_equals("print(session.information_schema.name);", "information_schema");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_base_session_default_schema)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");

    // Attempts to get the default schema
    exec_and_out_equals("var schema = session.default_schema;");

    // TODO: This is returning a null object and is threated as object in JS
    //       should it be a null value??
    exec_and_out_equals("print(schema);", "null");

    // Sets a default database
    exec_and_out_equals("session.executeSql('use mysql;');");

    // Attempts accessing the default database by name
    exec_and_out_equals("print(session.mysql)", "undefined");

    // Now uses the formal method
    exec_and_out_equals("var schema = session.default_schema;");
    exec_and_out_equals("print(typeof schema);", "object");
    exec_and_out_equals("print(schema instanceof mysqlx.Schema);", "true");
    exec_and_out_equals("print(schema.name);", "mysql");

    // Now checks the object can be accessed directly
    // because has been already set as a session attribute
    exec_and_out_equals("print(typeof session.mysql);", "object");
    exec_and_out_equals("print(session.mysql instanceof mysqlx.Schema);", "true");
    exec_and_out_equals("print(session.mysql.name);", "mysql");

    // Sets a different default database
    exec_and_out_equals("session.executeSql('use information_schema;');");

    // Now uses the formal method
    exec_and_out_equals("var schema = session.default_schema;");
    exec_and_out_equals("print(typeof schema);", "object");
    exec_and_out_equals("print(schema instanceof mysqlx.Schema);", "true");
    exec_and_out_equals("print(schema.name);", "information_schema");

    // Now checks the object can be accessed directly
    // because has been already set as a session attribute
    exec_and_out_equals("print(typeof session.information_schema);", "object");
    exec_and_out_equals("print(session.information_schema instanceof mysqlx.Schema);", "true");
    exec_and_out_equals("print(session.information_schema.name);", "information_schema");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_base_session_get_schema)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");

    // Attempts accessing a database directly as session attribute
    exec_and_out_equals("print(session.information_schema)", "undefined");

    // Now uses the formal method
    exec_and_out_equals("var schema = session.getSchema('information_schema');");
    exec_and_out_equals("print(typeof schema);", "object");
    exec_and_out_equals("print(schema instanceof mysqlx.Schema);", "true");
    exec_and_out_equals("print(schema.name);", "information_schema");

    // Now checks the object can be accessed directly
    // because has been already set as a session attribute
    exec_and_out_equals("print(typeof session.information_schema);", "object");
    exec_and_out_equals("print(session.information_schema instanceof mysqlx.Schema);", "true");
    exec_and_out_equals("print(session.information_schema.name);", "information_schema");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_base_session_get_schemas)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");

    // Ensures the schemas have not been loaded
    exec_and_out_equals("print(session._loaded);", "false");
    exec_and_out_equals("print(session.mysql)", "undefined");
    exec_and_out_equals("print(session.information_schema)", "undefined");

    // Triggers the schema load
    exec_and_out_equals("var schemas = session.getSchemas();");

    // Validates schemas were loaded
    exec_and_out_equals("print(session._loaded);", "true");

    // Now checks the objects can be accessed directly
    // because has been already set as a session attributes
    exec_and_out_equals("print(typeof session.mysql);", "object");
    exec_and_out_equals("print(session.mysql instanceof mysqlx.Schema);", "true");
    exec_and_out_equals("print(session.mysql.name);", "mysql");

    exec_and_out_equals("print(typeof session.information_schema);", "object");
    exec_and_out_equals("print(session.information_schema instanceof mysqlx.Schema);", "true");
    exec_and_out_equals("print(session.information_schema.name);", "information_schema");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_base_session_schemas)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");

    // Ensures the schemas have not been loaded
    exec_and_out_equals("print(session._loaded);", "false");
    exec_and_out_equals("print(session.mysql)", "undefined");
    exec_and_out_equals("print(session.information_schema)", "undefined");

    // Triggers the schema load
    exec_and_out_equals("var schemas = session.schemas;");

    // Validates schemas were loaded
    exec_and_out_equals("print(session._loaded);", "true");

    // Now checks the objects can be accessed directly
    // because has been already set as a session attributes
    exec_and_out_equals("print(typeof session.mysql);", "object");
    exec_and_out_equals("print(session.mysql instanceof mysqlx.Schema);", "true");
    exec_and_out_equals("print(session.mysql.name);", "mysql");

    exec_and_out_equals("print(typeof session.information_schema);", "object");
    exec_and_out_equals("print(session.information_schema instanceof mysqlx.Schema);", "true");
    exec_and_out_equals("print(session.information_schema.name);", "information_schema");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_session_object_structure)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    exec_and_out_equals("var session = mysqlx.openSession('" + _uri + "');");

    // Ensures the schemas have not been loaded
    exec_and_out_equals("var schema = session.getSchema('mysql')");
    exec_and_out_equals("var members = dir(schema);");
    exec_and_out_equals("print(members.length);", "17");

    // Review the DatabaseObject members are present
    exec_and_out_equals("print(typeof schema.session);", "object");
    exec_and_out_equals("print(typeof schema.schema);", "object");
    exec_and_out_equals("print(typeof schema.name);", "string");
    exec_and_out_equals("print(typeof schema.in_database);", "boolean");
    exec_and_out_equals("print(typeof schema.getSession);", "function");
    exec_and_out_equals("print(typeof schema.getSchema);", "function");
    exec_and_out_equals("print(typeof schema.getName);", "function");
    exec_and_out_equals("print(typeof schema.existsInDatabase);", "function");

    // Review the Schema members are present
    exec_and_out_equals("print(typeof schema.getTable);", "function");
    exec_and_out_equals("print(typeof schema.getView);", "function");
    exec_and_out_equals("print(typeof schema.getCollection);", "function");
    exec_and_out_equals("print(typeof schema.getTables);", "function");
    exec_and_out_equals("print(typeof schema.getViews);", "function");
    exec_and_out_equals("print(typeof schema.getCollections);", "function");

    exec_and_out_equals("print(schema.session instanceof mysqlx.Session)", "true");
    exec_and_out_equals("print(schema.schema instanceof mysqlx.Schema)", "true");
    exec_and_out_equals("print(schema.name)", "mysql");
    exec_and_out_equals("print(schema.in_database)", "true");

    // Ensures the properties are non writable
    exec_and_out_equals("schema.session = null;");
    exec_and_out_equals("schema.schema = null;");
    exec_and_out_equals("schema.name = 'fake'");

    exec_and_out_equals("print(schema.session instanceof mysqlx.Session)", "true");
    exec_and_out_equals("print(schema.schema instanceof mysqlx.Schema)", "true");
    exec_and_out_equals("print(schema.name)", "mysql");

    // Now compares the getFunctions on DatabaseObject against the attributes
    exec_and_out_equals("print(schema.session === schema.getSession())", "true");
    exec_and_out_equals("print(schema.schema === schema.getSchema())", "true");
    exec_and_out_equals("print(schema.name === schema.getName())", "true");
    exec_and_out_equals("print(schema.in_database=== schema.existsInDatabase())", "true");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_session_items)
  {
    // TODO: Include the code for the Collections
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");

    exec_and_out_equals("session.executeSql('drop schema if exists js_shell_test;')");
    exec_and_out_equals("session.executeSql('create schema js_shell_test;')");
    exec_and_out_equals("session.executeSql('use js_shell_test;')");
    exec_and_out_equals("session.executeSql('create table table1 (name varchar(50));')");
    exec_and_out_equals("session.executeSql('create view view1 (my_name) as select name from table1;')");

    exec_and_out_equals("var schema = session.getSchema('js_shell_test')");

    // Ensures the table object does not exists before loading
    exec_and_out_equals("print(typeof schema.table1)", "undefined");

    // Ensures the schemas have not been loaded
    exec_and_out_equals("var table = schema.getTable('table1');");
    exec_and_out_equals("print(table instanceof mysqlx.Table)", "true");

    // Ensures the table object now exists on the schema
    exec_and_out_equals("print(schema.table1 instanceof mysqlx.Table)", "true");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_session_full_loading)
  {
    // TODO: Include the code for the Collections
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    std::vector<std::string> full_load_triggers;
    full_load_triggers.push_back("var tables = schema.getTables()");
    full_load_triggers.push_back("var tables = schema.tables");
    full_load_triggers.push_back("var views = schema.getViews()");
    full_load_triggers.push_back("var views = schema.views");
    full_load_triggers.push_back("var views = schema.getCollections()");
    full_load_triggers.push_back("var views = schema.collections");

    for (size_t index = 0; index < full_load_triggers.size(); index++)
    {
      // Assuming _uri is in the format user:password@host
      exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");

      exec_and_out_equals("session.executeSql('drop schema if exists js_shell_test;')");
      exec_and_out_equals("session.executeSql('create schema js_shell_test;')");
      exec_and_out_equals("session.executeSql('use js_shell_test;')");
      exec_and_out_equals("session.executeSql('create table table1 (name varchar(50));')");
      exec_and_out_equals("session.executeSql('create view view1 (my_name) as select name from table1;')");

      exec_and_out_equals("var schema = session.getSchema('js_shell_test')");

      // Ensures the table object does not exists before loading
      exec_and_out_equals("print(schema._loaded)", "false");

      // Triggers all object loading
      exec_and_out_equals(full_load_triggers[index]);

      // Ensures all the objects have been loaded
      exec_and_out_equals("print(schema._loaded)", "true");
      exec_and_out_equals("print(schema.table1 instanceof mysqlx.Table)", "true");
      exec_and_out_equals("print(schema.view1 instanceof mysqlx.View)", "true");
    }
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_table_object_structure)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");

    exec_and_out_equals("session.executeSql('drop schema if exists js_shell_test;')");
    exec_and_out_equals("session.executeSql('create schema js_shell_test;')");
    exec_and_out_equals("session.executeSql('use js_shell_test;')");
    exec_and_out_equals("session.executeSql('create table table1 (name varchar(50));')");
    exec_and_out_equals("session.executeSql('create view view1 (my_name) as select name from table1;')");

    exec_and_out_equals("var schema = session.getSchema('js_shell_test')");

    // Ensures the schemas have not been loaded
    exec_and_out_equals("var table = schema.getTable('table1')");
    exec_and_out_equals("var members = dir(table);");
    exec_and_out_equals("print(members.length);", "9");

    // Review the DatabaseObject members are present
    exec_and_out_equals("print(typeof table.session);", "object");
    exec_and_out_equals("print(typeof table.schema);", "object");
    exec_and_out_equals("print(typeof table.name);", "string");
    exec_and_out_equals("print(typeof table.in_database);", "boolean");
    exec_and_out_equals("print(typeof table.getSession);", "function");
    exec_and_out_equals("print(typeof table.getSchema);", "function");
    exec_and_out_equals("print(typeof table.getName);", "function");
    exec_and_out_equals("print(typeof table.existsInDatabase);", "function");

    // TODO Review the Schema members are present

    // Ensures the properties are non writable
    exec_and_out_equals("table.session = null;");
    exec_and_out_equals("table.schema = null;");
    exec_and_out_equals("table.name = 'fake'");

    exec_and_out_equals("print(table.session instanceof mysqlx.NodeSession)", "true");
    exec_and_out_equals("print(table.schema instanceof mysqlx.Schema)", "true");
    exec_and_out_equals("print(table.name)", "table1");

    // Now compares the getFunctions on DatabaseObject against the attributes
    exec_and_out_equals("print(table.session === table.getSession())", "true");
    exec_and_out_equals("print(table.schema === table.getSchema())", "true");
    exec_and_out_equals("print(table.name === table.getName())", "true");
    exec_and_out_equals("print(table.in_database=== table.existsInDatabase())", "true");

    // Ensures the schemas have not been loaded
    exec_and_out_equals("var view = schema.getView('view1')");
    exec_and_out_equals("var members = dir(view);");
    exec_and_out_equals("print(members.length);", "8");

    // Review the DatabaseObject members are present
    exec_and_out_equals("print(typeof view.session);", "object");
    exec_and_out_equals("print(typeof view.schema);", "object");
    exec_and_out_equals("print(typeof view.name);", "string");
    exec_and_out_equals("print(typeof view.in_database);", "boolean");
    exec_and_out_equals("print(typeof view.getSession);", "function");
    exec_and_out_equals("print(typeof view.getSchema);", "function");
    exec_and_out_equals("print(typeof view.getName);", "function");
    exec_and_out_equals("print(typeof view.existsInDatabase);", "function");

    // TODO Review the Schema members are present

    // Ensures the properties are non writable
    exec_and_out_equals("view.session = null;");
    exec_and_out_equals("view.schema = null;");
    exec_and_out_equals("view.name = 'fake'");

    exec_and_out_equals("print(view.session instanceof mysqlx.NodeSession)", "true");
    exec_and_out_equals("print(view.schema instanceof mysqlx.Schema)", "true");
    exec_and_out_equals("print(view.name)", "view1");

    // Now compares the getFunctions on DatabaseObject against the attributes
    exec_and_out_equals("print(view.session === view.getSession())", "true");
    exec_and_out_equals("print(view.schema === view.getSchema())", "true");
    exec_and_out_equals("print(view.name === view.getName())", "true");
    exec_and_out_equals("print(view.in_database=== view.existsInDatabase())", "true");
  }
}