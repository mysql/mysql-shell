/*
* Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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
#include <boost/shared_ptr.hpp>
#include <boost/pointer_cast.hpp>

#include "gtest/gtest.h"
#include "shellcore/types.h"
#include "shellcore/lang_base.h"
#include "shellcore/types_cpp.h"

#include "shellcore/shell_core.h"
#include "shellcore/shell_jscript.h"
#include "shell_script_tester.h"

#include "shellcore/server_registry.h"

namespace shcore
{
  namespace shell_adminapi_tests
  {
    class Shell_js_adminapi_tests : public Shell_core_test_wrapper
    {
    protected:
      virtual void set_options()
      {
        _options->interactive = true;
        _options->wizards = true;
        _options->initial_mode = IShell_core::Mode_JScript;
      };

      virtual void SetUp()
      {
        Shell_core_test_wrapper::SetUp();

        // Initializes the JS mode to point to the right module paths
        std::string js_modules_path = MYSQLX_SOURCE_HOME;
        js_modules_path += "/scripting/modules/js";
        _interactive_shell->process_line("shell.js.module_paths[shell.js.module_paths.length] = '" + js_modules_path + "'; ");
        output_handler.wipe_all();
      }
    };

    TEST_F(Shell_js_adminapi_tests, getAdminSession)
    {
      _interactive_shell->process_line("var mysqlx = require('mysqlx');");
      _interactive_shell->process_line("var admin = mysqlx.getAdminSession('" + _uri + "');");
      _interactive_shell->process_line("admin");
      MY_EXPECT_STDOUT_CONTAINS("<AdminSession:"+_uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("admin.close()");
    }

    TEST_F(Shell_js_adminapi_tests, createFarm)
    {
      _interactive_shell->process_line("var mysqlx = require('mysqlx');");
      _interactive_shell->process_line("var admin = mysqlx.getAdminSession('" + _uri + "');");

      _interactive_shell->process_line("farm = admin.createFarm('devFarm')");
      MY_EXPECT_STDOUT_CONTAINS("<Farm>");
      _interactive_shell->process_line("farm");
      MY_EXPECT_STDOUT_CONTAINS("<Farm>");
      output_handler.wipe_all();

      _interactive_shell->process_line("farm.name");
      MY_EXPECT_STDOUT_CONTAINS("devFarm");
      output_handler.wipe_all();

      _interactive_shell->process_line("farm.getName()");
      MY_EXPECT_STDOUT_CONTAINS("devFarm");

      _interactive_shell->process_line("farm = admin.createFarm()");
      MY_EXPECT_STDERR_CONTAINS("Invalid number of arguments in AdminSession.createFarm, expected 1 but got 0");
      output_handler.wipe_all();

      _interactive_shell->process_line("farm = admin.createFarm('')");
      MY_EXPECT_STDERR_CONTAINS("The Farm name cannot be empty");
      output_handler.wipe_all();

      // Clean up the created Farm
      _interactive_shell->process_line("admin.dropFarm('devFarm', {dropDefaultReplicaSet: true})");
      output_handler.wipe_all();

      _interactive_shell->process_line("admin.close()");
    }

    TEST_F(Shell_js_adminapi_tests, dropFarm)
    {
      _interactive_shell->process_line("var mysqlx = require('mysqlx');");
      _interactive_shell->process_line("var admin = mysqlx.getAdminSession('" + _uri + "');");

      _interactive_shell->process_line("admin.createFarm('devFarm')");
      output_handler.wipe_all();

      _interactive_shell->process_line("admin.dropFarm('devFarm', {dropDefaultReplicaSet: true})");

      _interactive_shell->process_line("farm");
      MY_EXPECT_STDERR_CONTAINS("farm is not defined at");
      output_handler.wipe_all();

      _interactive_shell->process_line("admin.dropFarm()");
      MY_EXPECT_STDERR_CONTAINS("Invalid number of arguments in AdminSession.dropFarm, expected 1 to 2 but got 0");
      output_handler.wipe_all();

      _interactive_shell->process_line("admin.dropFarm('')");
      MY_EXPECT_STDERR_CONTAINS("The Farm name cannot be empty");
      output_handler.wipe_all();

      _interactive_shell->process_line("admin.dropFarm('foobar')");
      MY_EXPECT_STDERR_CONTAINS("The farm with the name 'foobar' does not exist");
      output_handler.wipe_all();
    }

    TEST_F(Shell_js_adminapi_tests, addNodeFarm)
    {
      _interactive_shell->process_line("var mysqlx = require('mysqlx');");
      _interactive_shell->process_line("var admin = mysqlx.getAdminSession('" + _uri + "');");

      _interactive_shell->process_line("farm = admin.createFarm('devFarm')");
      _interactive_shell->process_line("farm.addNode('192.168.1.1:33060')");
      MY_EXPECT_STDOUT_CONTAINS("<Farm>");
      output_handler.wipe_all();

      _interactive_shell->process_line("farm.addNode({host: '192.168.1.1', port: 1234})");
      MY_EXPECT_STDOUT_CONTAINS("");
      output_handler.wipe_all();

      _interactive_shell->process_line("farm.addNode({host: '192.168.1.1', schema: 'abs'})");
      MY_EXPECT_STDERR_CONTAINS("Unexpected argument on connection data");
      output_handler.wipe_all();

      _interactive_shell->process_line("farm.addNode({host: '192.168.1.1', user: 'abs'})");
      MY_EXPECT_STDERR_CONTAINS("Unexpected argument on connection data");
      output_handler.wipe_all();

      _interactive_shell->process_line("farm.addNode({host: '192.168.1.1', password: 'abs'})");
      MY_EXPECT_STDERR_CONTAINS("Unexpected argument on connection data");
      output_handler.wipe_all();

      _interactive_shell->process_line("farm.addNode({host: '192.168.1.1', authMethod: 'abs'})");
      MY_EXPECT_STDERR_CONTAINS("Unexpected argument on connection data");
      output_handler.wipe_all();

      _interactive_shell->process_line("farm.addNode({port: 33060})");
      MY_EXPECT_STDERR_CONTAINS("Missing required value for hostname");
      output_handler.wipe_all();

      _interactive_shell->process_line("farm.addNode('')");
      MY_EXPECT_STDERR_CONTAINS("Connection data empty");
      output_handler.wipe_all();

      // Clean up the created Farm
      _interactive_shell->process_line("admin.dropFarm('devFarm', {dropDefaultReplicaSet: true})");
      output_handler.wipe_all();

      _interactive_shell->process_line("admin.close()");
    }

    TEST_F(Shell_js_adminapi_tests, defaultReplicaSet)
    {
       _interactive_shell->process_line("var mysqlx = require('mysqlx');");
      _interactive_shell->process_line("var admin = mysqlx.getAdminSession('" + _uri + "');");
      _interactive_shell->process_line("farm = admin.createFarm('devFarm')");
      output_handler.wipe_all();

      _interactive_shell->process_line("rs = farm.getReplicaSet('default')");
      MY_EXPECT_STDOUT_CONTAINS("<ReplicaSet>");
      output_handler.wipe_all();

      _interactive_shell->process_line("rs.name");
      MY_EXPECT_STDOUT_CONTAINS("default");
      output_handler.wipe_all();

      _interactive_shell->process_line("rs.getName()");
      MY_EXPECT_STDOUT_CONTAINS("default");
      output_handler.wipe_all();

      _interactive_shell->process_line("rs = farm.getReplicaSet()");
      MY_EXPECT_STDOUT_CONTAINS("<ReplicaSet>");
      output_handler.wipe_all();

      _interactive_shell->process_line("rs.name");
      MY_EXPECT_STDOUT_CONTAINS("default");
      output_handler.wipe_all();

      _interactive_shell->process_line("rs.getName()");
      MY_EXPECT_STDOUT_CONTAINS("default");
      output_handler.wipe_all();

      // Clean up the created Farm
      _interactive_shell->process_line("admin.dropFarm('devFarm', {dropDefaultReplicaSet: true})");
      output_handler.wipe_all();

      _interactive_shell->process_line("admin.close()");
    }
  }
}
