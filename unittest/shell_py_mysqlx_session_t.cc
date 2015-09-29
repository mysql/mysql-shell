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
  class Shell_py_mysqlx_session_tests : public Shell_core_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_Python, initilaized);
    }
  };

  TEST_F(Shell_py_mysqlx_session_tests, mysqlx_session_members)
  {
    exec_and_out_equals("import mysqlx");

    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("session = mysqlx.getSession('" + _uri + "')");

    // Ensures the right members exist
    exec_and_out_equals("members = dir(session)");
    exec_and_out_equals("index=members.index('close')");
    exec_and_out_equals("index=members.index('createSchema')");
    exec_and_out_equals("index=members.index('getDefaultSchema')");
    exec_and_out_equals("index=members.index('getSchema')");
    exec_and_out_equals("index=members.index('getSchemas')");
    exec_and_out_equals("index=members.index('getUri')");
    exec_and_out_equals("index=members.index('setFetchWarnings')");
    exec_and_out_equals("index=members.index('defaultSchema')");
    exec_and_out_equals("index=members.index('schemas')");
    exec_and_out_equals("index=members.index('uri')");

    exec_and_out_equals("session.close()");
  }

  TEST_F(Shell_py_mysqlx_session_tests, mysqlx_node_session_members)
  {
    exec_and_out_equals("import mysqlx");

    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "')");

    // Ensures the right members exist
    exec_and_out_equals("members = dir(session)");
    exec_and_out_equals("index=members.index('close')");
    exec_and_out_equals("index=members.index('createSchema')");
    exec_and_out_equals("index=members.index('getCurrentSchema')");
    exec_and_out_equals("index=members.index('getDefaultSchema')");
    exec_and_out_equals("index=members.index('getSchema')");
    exec_and_out_equals("index=members.index('getSchemas')");
    exec_and_out_equals("index=members.index('getUri')");
    exec_and_out_equals("index=members.index('setCurrentSchema')");
    exec_and_out_equals("index=members.index('setFetchWarnings')");
    exec_and_out_equals("index=members.index('sql')");
    exec_and_out_equals("index=members.index('defaultSchema')");
    exec_and_out_equals("index=members.index('schemas')");
    exec_and_out_equals("index=members.index('uri')");
    exec_and_out_equals("index=members.index('currentSchema')");

    exec_and_out_equals("session.close()");
  }

  // Tests session.getDefaultSchema()
  TEST_F(Shell_py_mysqlx_session_tests, mysqlx_base_session_get_uri)
  {
    exec_and_out_equals("import mysqlx");

    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("session = mysqlx.getSession('" + _uri + "')");

    exec_and_out_equals("print(session.getUri())", uri);

    exec_and_out_equals("session.close()");
  }

  // Tests session.getDefaultSchema()
  TEST_F(Shell_py_mysqlx_session_tests, mysqlx_base_session_uri)
  {
    exec_and_out_equals("import mysqlx");

    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("session = mysqlx.getSession('" + _uri + "')");

    exec_and_out_equals("print(session.uri)", uri);

    exec_and_out_equals("session.close()");
  }

  // Tests session.getDefaultSchema() and session.getCurrentSchema()
  TEST_F(Shell_py_mysqlx_session_tests, mysqlx_base_session_get_empty_default_schema)
  {
    {
      SCOPED_TRACE("retrieving the default schema for first time");
      exec_and_out_equals("import mysqlx");

      exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "')");

      // Attempts to get the default schema
      exec_and_out_equals("dschema = session.getDefaultSchema()");
      exec_and_out_equals("print(dschema)", "None");

      // Now the current schema
      exec_and_out_equals("cschema = session.getCurrentSchema()");
      exec_and_out_equals("print(cschema)", "None");
    };

    {
      SCOPED_TRACE("Setting/getting default schema.");
      exec_and_out_equals("session.setCurrentSchema('mysql')");

      // Default schema does not change
      exec_and_out_equals("dschema = session.getDefaultSchema()");
      exec_and_out_equals("print(dschema)", "None");

      // Now uses the formal method
      exec_and_out_equals("cschema = session.getCurrentSchema()");
      exec_and_out_equals("print(cschema)", "<Schema:mysql>");
    };

    {
      // Sets a different default database
      exec_and_out_equals("session.setCurrentSchema('information_schema')");

      // Default schema does not change
      exec_and_out_equals("dschema = session.getDefaultSchema()");
      exec_and_out_equals("print(dschema)", "None");

      // Now uses the formal method
      exec_and_out_equals("cschema = session.getCurrentSchema()");
      exec_and_out_equals("print(cschema)", "<Schema:information_schema>");
    };

    exec_and_out_equals("session.close()");
  }

  // Tests session.getDefaultSchema() and session.getCurrentSchema()
  TEST_F(Shell_py_mysqlx_session_tests, mysqlx_base_session_get_default_schema)
  {
    {
      SCOPED_TRACE("retrieving the default schema for first time");
      exec_and_out_equals("import mysqlx");

      exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "/mysql')");

      // Attempts to get the default schema
      exec_and_out_equals("dschema = session.getDefaultSchema()");
      exec_and_out_equals("print(dschema)", "<Schema:mysql>");

      // Now the current schema
      exec_and_out_equals("cschema = session.getCurrentSchema()");
      exec_and_out_equals("print(cschema)", "<Schema:mysql>");
    };

    {
      // Sets a different default database
      exec_and_out_equals("session.setCurrentSchema('information_schema')");

      // Default schema does not change
      exec_and_out_equals("dschema = session.getDefaultSchema()");
      exec_and_out_equals("print(dschema)", "<Schema:mysql>");

      // Now uses the formal method
      exec_and_out_equals("cschema = session.getCurrentSchema()");
      exec_and_out_equals("print(cschema)", "<Schema:information_schema>");
    };

    exec_and_out_equals("session.close()");
  }

  // Tests session.defaultSchema and session.currentSchema
  TEST_F(Shell_py_mysqlx_session_tests, mysqlx_base_session_empty_default_schema)
  {
    {
      SCOPED_TRACE("retrieving the default schema for first time");
      exec_and_out_equals("import mysqlx");

      exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "')");

      // Attempts to get the default schema
      exec_and_out_equals("print(session.defaultSchema)", "None");
      exec_and_out_equals("print(session.currentSchema)", "None");
    };

    {
      SCOPED_TRACE("Setting/getting default schema.");
      exec_and_out_equals("session.setCurrentSchema('mysql')");

      // Now uses the formal method
      exec_and_out_equals("print(session.defaultSchema)", "None");
      exec_and_out_equals("print(session.currentSchema)", "<Schema:mysql>");
    };

    {
      // Sets a different default database
      exec_and_out_equals("session.setCurrentSchema('information_schema')");

      // Now uses the formal method
      exec_and_out_equals("print(session.defaultSchema)", "None");
      exec_and_out_equals("print(session.currentSchema)", "<Schema:information_schema>");
    };

    exec_and_out_equals("session.close()");
  }

  // Tests session.defaultSchema and session.currentSchema
  TEST_F(Shell_py_mysqlx_session_tests, mysqlx_base_session_default_schema)
  {
    {
      SCOPED_TRACE("retrieving the default schema for first time");
      exec_and_out_equals("import mysqlx");

      exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "/mysql')");

      // Attempts to get the default schema
      exec_and_out_equals("print(session.defaultSchema)", "<Schema:mysql>");
      exec_and_out_equals("print(session.currentSchema)", "<Schema:mysql>");
    };

    {
      // Sets a different default database
      exec_and_out_equals("session.setCurrentSchema('information_schema')");

      // Now uses the formal method
      exec_and_out_equals("print(session.defaultSchema)", "<Schema:mysql>");
      exec_and_out_equals("print(session.currentSchema)", "<Schema:information_schema>");
    };

    exec_and_out_equals("session.close()");
  }

  // Tests session.getSchemas()
  TEST_F(Shell_py_mysqlx_session_tests, mysqlx_base_session_get_schemas)
  {
    exec_and_out_equals("import mysqlx");

    exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "')");

    // Triggers the schema load
    exec_and_out_equals("schemas = session.getSchemas()");

    // Now checks the objects can be accessed directly
    // because has been already set as a session attributes
    exec_and_out_equals("print(schemas.mysql)", "<Schema:mysql>");
    exec_and_out_equals("print(schemas.information_schema)", "<Schema:information_schema>");

    exec_and_out_equals("session.close()");
  }

  // Tests session.schemas
  TEST_F(Shell_py_mysqlx_session_tests, mysqlx_base_session_schemas)
  {
    exec_and_out_equals("import mysqlx");

    exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "')");

    // Ensures the schemas have not been loaded
    exec_and_out_equals("print(session.schemas.mysql)", "<Schema:mysql>");
    exec_and_out_equals("print(session.schemas.information_schema)", "<Schema:information_schema>");

    exec_and_out_equals("session.close()");
  }

  // Tests session.getSchema()
  TEST_F(Shell_py_mysqlx_session_tests, mysqlx_base_session_get_schema)
  {
    exec_and_out_equals("import mysqlx");

    exec_and_out_equals("session = mysqlx.getSession('" + _uri + "')");

    // Checks schema retrieval
    exec_and_out_equals("schema = session.getSchema('mysql')");
    exec_and_out_equals("print(schema)", "<Schema:mysql>");

    exec_and_out_equals("schema = session.getSchema('information_schema')");
    exec_and_out_equals("print(schema)", "<Schema:information_schema>");

    // Checks schema retrieval with invalid schema
    exec_and_out_contains("schema = session.getSchema('unexisting_schema')", "", "Unknown database 'unexisting_schema'");

    exec_and_out_equals("session.close()");
  }

  // Tests session.<schema>
  TEST_F(Shell_py_mysqlx_session_tests, mysqlx_base_session_schema)
  {
    exec_and_out_equals("import mysqlx");

    exec_and_out_equals("session = mysqlx.getSession('" + _uri + "')");

    // Now direct and indirect access
    exec_and_out_equals("print(session.mysql)", "<Schema:mysql>");

    exec_and_out_equals("print(session.information_schema)", "<Schema:information_schema>");

    // TODO: add test case with schema that is named as another property

    // Now direct and indirect access
    exec_and_out_contains("print(session.unexisting_schema);", "", "unknown attribute: unexisting_schema");

    exec_and_out_equals("session.close()");
  }

  // Tests session.<schema>
  TEST_F(Shell_py_mysqlx_session_tests, set_fetch_warnings)
  {
    exec_and_out_equals("import mysqlx");

    exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "')");

    // Now direct and indirect access
    exec_and_out_equals("session.setFetchWarnings(True)");

    exec_and_out_equals("result = session.sql('drop database if exists unexisting;').execute()");

    exec_and_out_equals("print(result.warningCount)", "1");

    exec_and_out_equals("print(len(result.warnings))", "1");

    exec_and_out_equals("print(len(result.getWarnings()))", "1");

    exec_and_out_contains("print(result.warnings[0].Message)", "Can't drop database 'unexisting'; database doesn't exist");

    exec_and_out_equals("session.setFetchWarnings(False)");

    exec_and_out_equals("result = session.sql('drop database if exists unexisting;').execute()");

    exec_and_out_equals("print(result.warningCount)", "0");

    exec_and_out_equals("print(len(result.warnings))", "0");

    exec_and_out_equals("print(len(result.getWarnings()))", "0");

    exec_and_out_equals("session.close()");
  }

  TEST_F(Shell_py_mysqlx_session_tests, create_schema)
  {
    exec_and_out_equals("import mysqlx");

    exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "')");

    // Cleans py_test_create_schema
    exec_and_out_equals("session.sql('drop database if exists py_test_create_schema').execute()");

    // Happy path
    exec_and_out_equals("s = session.createSchema('py_test_create_schema')");

    exec_and_out_equals("print(s)", "<Schema:py_test_create_schema>");

    // Error, existing schema
    exec_and_out_contains("s2 = session.createSchema('py_test_create_schema')", "", "Can't create database 'py_test_create_schema'; database exists");

    // Error, passing non string
    exec_and_out_contains("s2 = session.createSchema(45)", "", "Argument #1 is expected to be a string");

    // Drops the database
    exec_and_out_equals("session.sql('drop database py_test_create_schema').execute()");

    exec_and_out_equals("session.close()");
  }

  TEST_F(Shell_py_mysqlx_session_tests, transaction_handling)
  {
    exec_and_out_equals("import mysqlx");

    exec_and_out_equals("session = mysqlx.getNodeSession('" + _uri + "')");

    // Cleans py_test_create_schema
    exec_and_out_equals("session.sql('drop database if exists py_tx_schema').execute()");

    // Happy path
    exec_and_out_equals("s = session.createSchema('py_tx_schema')");

    exec_and_out_equals("collection = s.createCollection('sample')");

    // Tests the rollback
    exec_and_out_equals("session.startTransaction()");

    exec_and_out_equals("res1 = collection.add({'name':'john', 'age': 15}).execute()");
    exec_and_out_equals("res1 = collection.add({'name':'carol', 'age': 16}).execute()");
    exec_and_out_equals("res1 = collection.add({'name':'alma', 'age': 17}).execute()");

    exec_and_out_equals("session.rollback()");

    exec_and_out_equals("result = collection.find().execute()");
    exec_and_out_equals("print(len(result.all()))", "0", "");

    // Test the commit
    exec_and_out_equals("session.startTransaction()");

    exec_and_out_equals("res1 = collection.add({'name':'john', 'age': 15}).execute()");
    exec_and_out_equals("res1 = collection.add({'name':'carol', 'age': 16}).execute()");
    exec_and_out_equals("res1 = collection.add({'name':'alma', 'age': 17}).execute()");

    exec_and_out_equals("session.commit()");

    exec_and_out_equals("result = collection.find().execute()");
    exec_and_out_equals("print(len(result.all()))", "3", "");

    // Drops the database
    exec_and_out_equals("s.drop()");

    exec_and_out_equals("session.close()");
  }
}