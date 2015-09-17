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
  class Shell_js_mysqlx_resultset_tests : public Shell_core_test_wrapper
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

  TEST_F(Shell_js_mysqlx_resultset_tests, initialization)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");

    exec_and_out_equals("session.sql('drop schema if exists js_shell_test;').execute();");
    exec_and_out_equals("session.sql('create schema js_shell_test;').execute();");
    exec_and_out_equals("session.sql('use js_shell_test;').execute();");
    exec_and_out_equals("session.sql('create table table1 (id int auto_increment primary key, name varchar(50));').execute();");

    // TODO: should be enabled once collection crud is available
    //exec_and_out_equals("session.js_shell_test.createCollection('collection1')");

    exec_and_out_equals("session.sql('insert into table1 (`name`) values(\"one\");').execute();");
    exec_and_out_equals("session.sql('insert into table1 (`name`) values(\"two\");').execute();");
    exec_and_out_equals("session.sql('insert into table1 (`name`) values(\"three\");').execute();");

    exec_and_out_equals("session.close();");
  }

  // Tests resultset.hasData and resultset.getHasData()
  TEST_F(Shell_js_mysqlx_resultset_tests, mysqlx_resultset_has_data)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");

    exec_and_out_equals("var result = session.sql('use js_shell_test;').execute();");

    exec_and_out_equals("print(result.hasData);", "false");
    exec_and_out_equals("print(result.getHasData());", "false");

    exec_and_out_equals("var result = session.sql('select * from table1;').execute();");

    exec_and_out_equals("print(result.hasData);", "true");
    exec_and_out_equals("print(result.getHasData());", "true");

    exec_and_out_equals("session.close();");
  }

  // Tests resultset.columnMetadata and resultset.getColumnMetadata()
  TEST_F(Shell_js_mysqlx_resultset_tests, mysqlx_resultset_column_metadata)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");

    exec_and_out_equals("var result = session.sql('select * from js_shell_test.table1;').execute();");

    exec_and_out_equals("var metadata = result.getColumnMetadata()");

    exec_and_out_equals("print(metadata[0].name)", "id");

    exec_and_out_equals("var metadata = result.columnMetadata");

    exec_and_out_equals("print(metadata[0].name)", "id");

    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_resultset_tests, mysqlx_resultset_buffering)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");

    exec_and_out_equals("var schema = session.getSchema('js_shell_test')");
    exec_and_out_equals("session.sql('drop table if exists js_shell_test.buffer_table;').execute()");
    exec_and_out_equals("session.sql('create table js_shell_test.buffer_table (name varchar(50), age integer, gender varchar(20));').execute()");
    exec_and_out_equals("var table = schema.getTable('buffer_table')");

    exec_and_out_equals("result = table.insert({'name': 'jack', 'age': 17, 'gender': 'male'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'adam', 'age': 15, 'gender': 'male'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'brian', 'age': 14, 'gender': 'male'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'alma', 'age': 13, 'gender': 'female'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'carol', 'age': 14, 'gender': 'female'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'donna', 'age': 16, 'gender': 'female'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'angel', 'age': 14, 'gender': 'male'}).execute()");

    exec_and_out_equals("var result1 = session.sql('select name, age from js_shell_test.buffer_table where gender = \"male\" order by name').execute();");

    exec_and_out_equals("var result2 = session.sql('select name, gender from js_shell_test.buffer_table where age < 15 order by name').execute();");

    exec_and_out_equals("var metadata2 = result2.getColumnMetadata()");
    exec_and_out_equals("var metadata1 = result1.getColumnMetadata()");

    exec_and_out_equals("print(metadata2[0].name)", "name");
    exec_and_out_equals("print(metadata2[1].name)", "gender");

    exec_and_out_equals("print(metadata1[0].name)", "name");
    exec_and_out_equals("print(metadata1[1].name)", "age");

    exec_and_out_equals("var record2 = result2.next()");
    exec_and_out_equals("var record1 = result1.next()");

    exec_and_out_equals("print(record2.name)", "alma");
    exec_and_out_equals("print(record1.name)", "adam");

    exec_and_out_equals("var record2 = result2.next()");
    exec_and_out_equals("var record1 = result1.next()");

    exec_and_out_equals("print(record2.name)", "angel");
    exec_and_out_equals("print(record1.name)", "angel");

    exec_and_out_equals("var record2 = result2.next()");
    exec_and_out_equals("var record1 = result1.next()");

    exec_and_out_equals("print(record2.name)", "brian");
    exec_and_out_equals("print(record1.name)", "brian");

    exec_and_out_equals("var record2 = result2.next()");
    exec_and_out_equals("var record1 = result1.next()");

    exec_and_out_equals("print(record2.name)", "carol");
    exec_and_out_equals("print(record1.name)", "jack");

    exec_and_out_equals("session.close();");
  }

  TEST_F(Shell_js_mysqlx_resultset_tests, crud_buffering)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");

    exec_and_out_equals("schema = session.getSchema('js_shell_test')");
    exec_and_out_equals("session.sql('drop table if exists js_shell_test.buffer_table;').execute()");
    exec_and_out_equals("session.sql('create table js_shell_test.buffer_table (name varchar(50), age integer, gender varchar(20));').execute()");
    exec_and_out_equals("table = schema.getTable('buffer_table')");

    exec_and_out_equals("result = table.insert({'name': 'jack', 'age': 17, 'gender': 'male'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'adam', 'age': 15, 'gender': 'male'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'brian', 'age': 14, 'gender': 'male'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'alma', 'age': 13, 'gender': 'female'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'carol', 'age': 14, 'gender': 'female'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'donna', 'age': 16, 'gender': 'female'}).execute()");
    exec_and_out_equals("result = table.insert({'name': 'angel', 'age': 14, 'gender': 'male'}).execute()");

    exec_and_out_equals("var result1 = table.select(['name', 'age']).where('gender = :gender').orderBy(['name']).bind('gender','male').execute();");
    exec_and_out_equals("var result2 = table.select(['name', 'gender']).where('age < :age').orderBy(['name']).bind('age',15).execute();");

    exec_and_out_equals("var metadata2 = result2.getColumnMetadata()");
    exec_and_out_equals("var metadata1 = result1.getColumnMetadata()");

    exec_and_out_equals("print(metadata2[0].name)", "name");
    exec_and_out_equals("print(metadata2[1].name)", "gender");

    exec_and_out_equals("print(metadata1[0].name)", "name");
    exec_and_out_equals("print(metadata1[1].name)", "age");

    exec_and_out_equals("var record2 = result2.next()");
    exec_and_out_equals("var record1 = result1.next()");

    exec_and_out_equals("print(record2.name)", "alma");
    exec_and_out_equals("print(record1.name)", "adam");

    exec_and_out_equals("var record2 = result2.next()");
    exec_and_out_equals("var record1 = result1.next()");

    exec_and_out_equals("print(record2.name)", "angel");
    exec_and_out_equals("print(record1.name)", "angel");

    exec_and_out_equals("var record2 = result2.next()");
    exec_and_out_equals("var record1 = result1.next()");

    exec_and_out_equals("print(record2.name)", "brian");
    exec_and_out_equals("print(record1.name)", "brian");

    exec_and_out_equals("var record2 = result2.next()");
    exec_and_out_equals("var record1 = result1.next()");

    exec_and_out_equals("print(record2.name)", "carol");
    exec_and_out_equals("print(record1.name)", "jack");

    exec_and_out_equals("session.close();");
  }

  // Tests resultset.lastInsertId and resultset.getLastInsertId()
  TEST_F(Shell_js_mysqlx_resultset_tests, DISABLED_mysqlx_resultset_last_insert_id)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    exec_and_out_equals("var session = mysqlx.getNodeSession('" + _uri + "');");

    exec_and_out_equals("var result = session.sql('insert into js_shell_test.table1 (`name`) values(\"four\");').execute();");

    exec_and_out_equals("print(result.lastInsertId)", "4");
    exec_and_out_equals("print(result.getLastInsertId())", "4");

    exec_and_out_equals("session.close();");
  }
}