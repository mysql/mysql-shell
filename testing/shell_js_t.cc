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
#include <boost/shared_ptr.hpp>
#include <boost/pointer_cast.hpp>

#include "gtest/gtest.h"
#include "shellcore/types.h"
#include "shellcore/lang_base.h"
#include "shellcore/types_cpp.h"

#include "shellcore/shell_core.h"
#include "shellcore/shell_jscript.h"

namespace shcore {
  class Shell_js_test : public ::testing::Test
  {
  protected:
    // Per-test-case set-up.
    // Called before the first test in this test case.
    // Can be omitted if not needed.
    static void SetUpTestCase()
    {
    }

    // Per-test-case tear-down.
    // Called after the last test in this test case.
    // Can be omitted if not needed.
    static void TearDownTestCase()
    {
    }

    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      deleg.user_data = this;
      deleg.print = &Shell_js_test::deleg_print;
      deleg.print_error = &Shell_js_test::deleg_print_error;
      shell_core.reset(new Shell_core(&deleg));
      bool initialized(false);

      shell_js.reset(new Shell_javascript(shell_core.get()));
    }

    virtual void TearDown()
    {
    }

    static void deleg_print(void *user_data, const char *text)
    {
      Shell_js_test* target = (Shell_js_test*)(user_data);
      target->std_out.assign(text);
    }

    static void deleg_print_error(void *user_data, const char *text)
    {
      Shell_js_test* target = (Shell_js_test*)(user_data);
      target->std_err.assign(text);
    }

    std::string std_err;
    std::string std_out;
    Interpreter_delegate deleg;
    boost::shared_ptr<Shell_core> shell_core;
    boost::shared_ptr<Shell_javascript> shell_js;
  };

  TEST_F(Shell_js_test, require_failures)
  {
    std::string code;
    Interactive_input_state state;

    // This module actually exists but will fail because
    // it parent folder has not been added to the module
    // directory list.
    code = "var mymodule = require('fail_wont_compile')";
    shell_js->handle_input(code, state, true);

    std::string expected = "Module fail_wont_compile was not found";
    EXPECT_NE(-1, int(std_err.find(expected)));
    std_out.clear();
    std_err.clear();

    // Adds a module directory
    code = "shell.js.module_paths[shell.js.module_paths.length] = './js'";
    shell_js->handle_input(code, state, true);
    EXPECT_TRUE(std_out.empty());
    EXPECT_TRUE(std_err.empty());

    code = "var mymodule = require('fail_wont_compile')";
    shell_js->handle_input(code, state, true);
    expected = "my_object is not defined";
    EXPECT_NE(-1, int(std_err.find(expected)));
    std_out.clear();
    std_err.clear();

    code = "var empty_module = require('fail_empty')";
    shell_js->handle_input(code, state, true);
    expected = "Module fail_empty is empty";
    EXPECT_NE(-1, int(std_err.find(expected)));
    std_out.clear();
    std_err.clear();
  }

  TEST_F(Shell_js_test, require_success)
  {
    std::string code;
    Interactive_input_state state;

    // Adds a module directory
    code = "shell.js.module_paths[shell.js.module_paths.length] = './js'";
    shell_js->handle_input(code, state, true);
    EXPECT_TRUE(std_out.empty());
    EXPECT_TRUE(std_err.empty());

    code = "var my_module = require('success')";
    std_err = "";
    std_out = "";
    shell_js->handle_input(code, state, true);
    EXPECT_TRUE(std_err.empty());
    EXPECT_TRUE(std_out.empty());

    code = "print(Object.keys(my_module).length)";
    shell_js->handle_input(code, state, true);
    EXPECT_TRUE(std_err.empty());
    EXPECT_EQ("1", std_out);

    std_out = "";
    code = "print(my_module.ok_message)";
    shell_js->handle_input(code, state, true);
    EXPECT_TRUE(std_err.empty());
    EXPECT_EQ("module imported ok", std_out);

    std_out = "";
    code = "print(typeof unexisting)";
    shell_js->handle_input(code, state, true);
    EXPECT_TRUE(std_err.empty());
    EXPECT_EQ("undefined", std_out);

    std_out = "";
    code = "print(in_global_context)";
    shell_js->handle_input(code, state, true);
    EXPECT_TRUE(std_err.empty());
    EXPECT_EQ("This is here because of JS", std_out);
  }
}