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
  class Shell_js_mysql_session_tests : public Shell_core_test_wrapper
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

  // Tests session.getDefaultSchema()
  TEST_F(Shell_js_mysql_session_tests, mysql_classic_session_members)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "');");

    // Ensures the right members exist
    exec_and_out_equals("var members = dir(session);");
    exec_and_out_equals("print(members.length >= 13)", "true");
    exec_and_out_equals("print(members.indexOf('close') != -1);", "true");
    exec_and_out_equals("print(members.indexOf('createSchema') != -1);", "true");
    exec_and_out_equals("print(members.indexOf('getCurrentSchema') != -1);", "true");
    exec_and_out_equals("print(members.indexOf('getDefaultSchema') != -1);", "true");
    exec_and_out_equals("print(members.indexOf('getSchema') != -1);", "true");
    exec_and_out_equals("print(members.indexOf('getSchemas') != -1);", "true");
    exec_and_out_equals("print(members.indexOf('getUri') != -1);", "true");
    exec_and_out_equals("print(members.indexOf('setCurrentSchema') != -1);", "true");
    exec_and_out_equals("print(members.indexOf('runSql') != -1);", "true");
    exec_and_out_equals("print(members.indexOf('defaultSchema') != -1);", "true");
    exec_and_out_equals("print(members.indexOf('schemas') != -1);", "true");
    exec_and_out_equals("print(members.indexOf('uri') != -1);", "true");
    exec_and_out_equals("print(members.indexOf('currentSchema') != -1);", "true");

    exec_and_out_equals("session.close();");
  }

  // Tests session.getDefaultSchema()
  TEST_F(Shell_js_mysql_session_tests, mysql_base_session_get_uri)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "');");

    exec_and_out_equals("print(session.getUri());", uri);

    exec_and_out_equals("session.close();");
  }

  // Tests session.getDefaultSchema()
  TEST_F(Shell_js_mysql_session_tests, mysql_base_session_uri)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "');");

    exec_and_out_equals("print(session.uri);", uri);

    exec_and_out_equals("session.close();");
  }

  // Tests session.getDefaultSchema() and session.getCurrentSchema()
  TEST_F(Shell_js_mysql_session_tests, mysql_base_session_get_empty_default_schema)
  {
    {
      SCOPED_TRACE("retrieving the default schema for first time");
      exec_and_out_equals("var mysql = require('mysql').mysql;");

      exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "');");

      // Attempts to get the default schema
      exec_and_out_equals("var dschema = session.getDefaultSchema();");
      exec_and_out_equals("print(dschema);", "null");

      // Attempts to get the default schema
      exec_and_out_equals("var cschema = session.getCurrentSchema();");
      exec_and_out_equals("print(cschema);", "null");
    };

    {
      SCOPED_TRACE("Setting/getting current schema.");
      exec_and_out_equals("session.setCurrentSchema('mysql');");

      // Default schema does not change
      exec_and_out_equals("var dschema = session.getDefaultSchema();");
      exec_and_out_equals("print(dschema);", "null");

      // Now uses the formal method
      exec_and_out_equals("var cschema = session.getCurrentSchema();");
      exec_and_out_equals("print(cschema);", "<ClassicSchema:mysql>");
    };

    {
      // Sets a different current database
      exec_and_out_equals("session.setCurrentSchema('information_schema');");

      // Default schema does not change
      exec_and_out_equals("var dschema = session.getDefaultSchema();");
      exec_and_out_equals("print(dschema);", "null");

      // Now uses the formal method
      exec_and_out_equals("var cschema = session.getCurrentSchema();");
      exec_and_out_equals("print(cschema);", "<ClassicSchema:information_schema>");
    };

    exec_and_out_equals("session.close();");
  }

  // Tests session.getDefaultSchema() and session.getCurrentSchema()
  TEST_F(Shell_js_mysql_session_tests, mysql_base_session_get_default_schema)
  {
    {
      SCOPED_TRACE("retrieving the default schema for first time");
      exec_and_out_equals("var mysql = require('mysql').mysql;");

      exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "/mysql');");

      // Attempts to get the default schema
      exec_and_out_equals("var dschema = session.getDefaultSchema();");
      exec_and_out_equals("print(dschema);", "<ClassicSchema:mysql>");

      // Attempts to get the default schema
      exec_and_out_equals("var cschema = session.getCurrentSchema();");
      exec_and_out_equals("print(cschema);", "<ClassicSchema:mysql>");
    };

    {
      // Sets a different current database
      exec_and_out_equals("session.setCurrentSchema('information_schema');");

      // Default schema does not change
      exec_and_out_equals("var dschema = session.getDefaultSchema();");
      exec_and_out_equals("print(dschema);", "<ClassicSchema:mysql>");

      // Now uses the formal method
      exec_and_out_equals("var cschema = session.getCurrentSchema();");
      exec_and_out_equals("print(cschema);", "<ClassicSchema:information_schema>");
    };

    exec_and_out_equals("session.close();");
  }

  // Tests session.defaultSchema and session.currentSchema
  TEST_F(Shell_js_mysql_session_tests, mysql_base_session_empty_default_schema)
  {
    {
      SCOPED_TRACE("retrieving the default schema for first time");
      exec_and_out_equals("var mysql = require('mysql').mysql;");

      exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "');");

      // Attempts to get the default schema
      exec_and_out_equals("print(session.defaultSchema);", "null");
      exec_and_out_equals("print(session.currentSchema);", "null");
    };

    {
      SCOPED_TRACE("Setting/getting default schema.");
      exec_and_out_equals("session.setCurrentSchema('mysql');");

      // Now uses the formal method
      exec_and_out_equals("print(session.defaultSchema);", "null");
      exec_and_out_equals("print(session.currentSchema);", "<ClassicSchema:mysql>");
    };

    {
      // Sets a different default database
      exec_and_out_equals("session.setCurrentSchema('information_schema');");

      // Now uses the formal method
      exec_and_out_equals("print(session.defaultSchema);", "null");
      exec_and_out_equals("print(session.currentSchema);", "<ClassicSchema:information_schema>");
    };

    exec_and_out_equals("session.close();");
  }

  // Tests session.defaultSchema and session.currentSchema
  TEST_F(Shell_js_mysql_session_tests, mysql_base_session_default_schema)
  {
    {
      SCOPED_TRACE("retrieving the default schema for first time");
      exec_and_out_equals("var mysql = require('mysql').mysql;");

      exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "/mysql');");

      // Attempts to get the default schema
      exec_and_out_equals("print(session.defaultSchema);", "<ClassicSchema:mysql>");
      exec_and_out_equals("print(session.currentSchema);", "<ClassicSchema:mysql>");
    };

    {
      // Sets a different default database
      exec_and_out_equals("session.setCurrentSchema('information_schema');");

      // Now uses the formal method
      exec_and_out_equals("print(session.defaultSchema);", "<ClassicSchema:mysql>");
      exec_and_out_equals("print(session.currentSchema);", "<ClassicSchema:information_schema>");
    };

    exec_and_out_equals("session.close();");
  }

  // Tests session.getSchemas()
  TEST_F(Shell_js_mysql_session_tests, mysql_base_session_get_schemas)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "');");

    // Triggers the schema load
    exec_and_out_equals("var schemas = session.getSchemas();");

    // Now checks the objects can be accessed directly
    // because has been already set as a session attributes
    exec_and_out_equals("print(schemas.mysql);", "<ClassicSchema:mysql>");
    exec_and_out_equals("print(schemas.information_schema);", "<ClassicSchema:information_schema>");

    exec_and_out_equals("session.close();");
  }

  // Tests session.schemas
  TEST_F(Shell_js_mysql_session_tests, mysql_base_session_schemas)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "');");

    // Ensures the schemas have not been loaded
    exec_and_out_equals("print(session.schemas.mysql);", "<ClassicSchema:mysql>");
    exec_and_out_equals("print(session.schemas.information_schema)", "<ClassicSchema:information_schema>");

    exec_and_out_equals("session.close();");
  }

  // Tests session.getSchema()
  TEST_F(Shell_js_mysql_session_tests, mysql_base_session_get_schema)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "');");

    // Checks schema retrieval
    exec_and_out_equals("var schema = session.getSchema('mysql');");
    exec_and_out_equals("print(schema);", "<ClassicSchema:mysql>");

    exec_and_out_equals("var schema = session.getSchema('information_schema');");
    exec_and_out_equals("print(schema);", "<ClassicSchema:information_schema>");

    // Checks schema retrieval with invalid schema
    exec_and_out_contains("var schema = session.getSchema('unexisting_schema');", "", "Unknown database 'unexisting_schema'");

    exec_and_out_equals("session.close();");
  }

  // Tests session.<schema>
  TEST_F(Shell_js_mysql_session_tests, mysql_base_session_schema)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "');");

    // Now direct and indirect access
    exec_and_out_equals("print(session.mysql);", "<ClassicSchema:mysql>");

    exec_and_out_equals("print(session.information_schema);", "<ClassicSchema:information_schema>");

    // TODO: add test case with schema that is named as another property

    // Now direct and indirect access
    exec_and_out_equals("print(session.unexisting_schema);", "undefined");

    exec_and_out_equals("session.close();");
  }

  // Tests session.<schema>
  TEST_F(Shell_js_mysql_session_tests, create_schema)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "');");

    // Cleans environment
    exec_and_out_equals("session.runSql('drop database if exists mysql_test_create_schema_1')");

    // Happy path
    exec_and_out_equals("var s = session.createSchema('mysql_test_create_schema_1');");

    exec_and_out_equals("print(s);", "<ClassicSchema:mysql_test_create_schema_1>");

    // Schema with spaces
    exec_and_out_equals("var s = session.createSchema('classic schema');");

    exec_and_out_equals("print(s);", "<ClassicSchema:classic schema>");

    exec_and_out_equals("session.dropSchema('classic schema')");

    // Error, existing schema
    exec_and_out_contains("var s2 = session.createSchema('mysql_test_create_schema_1');", "", "MySQLError (1007): Can't create database 'mysql_test_create_schema_1'; database exists");

    // Error, passing non string
    exec_and_out_contains("var s2 = session.createSchema(45);", "", "TypeError: Argument #1 is expected to be a string");

    // Drops the database
    exec_and_out_equals("session.runSql('drop database mysql_test_create_schema_1')");

    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysql_session_tests, transaction_handling)
  {
    exec_and_out_equals("var mysql = require('mysql').mysql;");

    exec_and_out_equals("var session = mysql.getClassicSession('" + _mysql_uri + "')");

    // Cleans py_test_create_schema
    exec_and_out_equals("session.runSql('drop database if exists py_tx_schema');");

    // Happy path
    exec_and_out_equals("var s = session.createSchema('py_tx_schema');");
    exec_and_out_equals("session.setCurrentSchema('py_tx_schema');");

    exec_and_out_equals("session.runSql('create table sample (name varchar(50))');");

    // Tests the rollback
    exec_and_out_equals("session.startTransaction();");

    exec_and_out_equals("session.runSql('insert into sample values (\"first\")');");
    exec_and_out_equals("session.runSql('insert into sample values (\"second\")');");
    exec_and_out_equals("session.runSql('insert into sample values (\"third\")');");

    exec_and_out_equals("session.rollback();");

    exec_and_out_equals("var result = session.runSql('select count(*) from sample');");
    exec_and_out_equals("print(result.fetchOne()[0])", "0", "");

    // Test the commit
    exec_and_out_equals("session.startTransaction();");

    exec_and_out_equals("session.runSql('insert into sample values (\"first\")');");
    exec_and_out_equals("session.runSql('insert into sample values (\"second\")');");
    exec_and_out_equals("session.runSql('insert into sample values (\"third\")');");

    exec_and_out_equals("session.commit();");

    exec_and_out_equals("result = session.runSql('select count(*) from sample');");
    exec_and_out_equals("print(result.fetchOne()[0])", "3", "");

    // Drops the database
    exec_and_out_equals("session.dropSchema('py_tx_schema');");

    exec_and_out_equals("session.close();");
  }
}