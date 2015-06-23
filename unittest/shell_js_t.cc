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
#include "test_utils.h"

namespace shcore {
  class Shell_js_test : public Shell_core_test_wrapper
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

  TEST_F(Shell_js_test, require_failures)
  {
    // This module actually exists but will fail because
    // it parent folder has not been added to the module
    // directory list.
    execute("var mymodule = require('fail_wont_compile')");
    std::string expected = "Module fail_wont_compile was not found";
    EXPECT_NE(-1, int(output_handler.std_err.find(expected)));
    output_handler.wipe_all();

    // Adds a module directory
    execute("shell.js.module_paths[shell.js.module_paths.length] = os.get_binary_folder() + '/js'");
    EXPECT_TRUE(output_handler.std_out.empty());
    EXPECT_TRUE(output_handler.std_err.empty());

    execute("var mymodule = require('fail_wont_compile')");
    expected = "my_object is not defined";
    EXPECT_NE(-1, int(output_handler.std_err.find(expected)));
    output_handler.wipe_all();

    execute("var empty_module = require('fail_empty')");
    expected = "Module fail_empty is empty";
    EXPECT_NE(-1, int(output_handler.std_err.find(expected)));
    output_handler.wipe_all();
  }

  TEST_F(Shell_js_test, require_success)
  {
    std::string code;

    // Adds a module directory
    execute("shell.js.module_paths[shell.js.module_paths.length] = os.get_binary_folder() + '/js'");
    EXPECT_TRUE(output_handler.std_out.empty());
    EXPECT_TRUE(output_handler.std_err.empty());

    output_handler.wipe_all();
    execute("var my_module = require('success')");
    EXPECT_TRUE(output_handler.std_err.empty());
    EXPECT_TRUE(output_handler.std_out.empty());

    execute("print(Object.keys(my_module).length)");
    EXPECT_TRUE(output_handler.std_err.empty());
    EXPECT_EQ("1", output_handler.std_out);

    output_handler.wipe_out();
    execute("print(my_module.ok_message)");
    EXPECT_TRUE(output_handler.std_err.empty());
    EXPECT_EQ("module imported ok", output_handler.std_out);

    output_handler.wipe_out();
    execute("print(typeof unexisting)");
    EXPECT_TRUE(output_handler.std_err.empty());
    EXPECT_EQ("undefined", output_handler.std_out);

    output_handler.wipe_out();
    execute("print(in_global_context)");
    EXPECT_TRUE(output_handler.std_err.empty());
    EXPECT_EQ("This is here because of JS", output_handler.std_out);
  }
}