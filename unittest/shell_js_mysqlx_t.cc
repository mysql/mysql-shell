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
    exec_and_out_equals("print(exports.length);", "2");

    exec_and_out_equals("print(typeof mysqlx.openSession);", "function");
    exec_and_out_equals("print(typeof mysqlx.openNodeSession);", "function");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_open_session)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    std::string uri = mysh::strip_password(_uri);

    exec_and_out_equals("var session = mysqlx.openSession('" + _uri + "');");
    exec_and_out_equals("print(session);", "<Session:" + uri + ">");

    // Ensures the right members exist
    exec_and_out_equals("var members = dir(session);");
    exec_and_out_equals("print(members.length >= 8)", "true");
    exec_and_out_equals("print(members[0] == 'getDefaultSchema');", "true");
    exec_and_out_equals("print(members[1] == 'getSchema');", "true");
    exec_and_out_equals("print(members[2] == 'getSchemas');", "true");
    exec_and_out_equals("print(members[3] == 'getUri');", "true");
    exec_and_out_equals("print(members[4] == 'setDefaultSchema');", "true");
    exec_and_out_equals("print(members[5] == 'defaultSchema');", "true");
    exec_and_out_equals("print(members[6] == 'schemas');", "true");
    exec_and_out_equals("print(members[7] == 'uri');", "true");
  }

  TEST_F(Shell_js_mysqlx_tests, mysqlx_open_node_session)
  {
    exec_and_out_equals("var mysqlx = require('mysqlx').mysqlx;");

    // Assuming _uri is in the format user:password@host
    std::string uri = mysh::strip_password(_uri);
    exec_and_out_equals("var session = mysqlx.openNodeSession('" + _uri + "');");
    exec_and_out_equals("print(session);", "<NodeSession:" + uri + ">");

    // Ensures the right members exist
    exec_and_out_equals("var members = dir(session);");
    exec_and_out_equals("print(members.length >= 9)", "true");
    exec_and_out_equals("print(members[0] == 'executeSql');", "true");
    exec_and_out_equals("print(members[1] == 'getDefaultSchema');", "true");
    exec_and_out_equals("print(members[2] == 'getSchema');", "true");
    exec_and_out_equals("print(members[3] == 'getSchemas');", "true");
    exec_and_out_equals("print(members[4] == 'getUri');", "true");
    exec_and_out_equals("print(members[5] == 'setDefaultSchema');", "true");
    exec_and_out_equals("print(members[6] == 'defaultSchema');", "true");
    exec_and_out_equals("print(members[7] == 'schemas');", "true");
    exec_and_out_equals("print(members[8] == 'uri');", "true");
  }
}