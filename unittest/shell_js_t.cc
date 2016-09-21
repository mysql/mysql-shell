/* Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.

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
#include "shell_script_tester.h"

namespace shcore {
class Shell_js_tests : public Shell_js_script_tester {
  virtual void SetUp() {
    Shell_js_script_tester::SetUp();

    set_config_folder("js_devapi");
    //set_setup_script("setup.js");
  }
};

TEST_F(Shell_js_tests, built_ins) {
  std::string test_js_modules_path = MYSQLX_SOURCE_HOME;
  test_js_modules_path += "/unittest/test_modules/js";
  std::string code = "var __test_modules_path = '" + test_js_modules_path + "';";
  execute(code);

  validate_interactive("shell_builtins.js");
}
}
