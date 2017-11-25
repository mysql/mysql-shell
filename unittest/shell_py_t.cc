/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "unittest/test_utils.h"

namespace mysqlsh {

class Shell_python : public Shell_core_test_wrapper {
 public:
  void set_options() override {
    _options->interactive = true;
    _options->wizards = true;
  }

  void SetUp() override {
    Shell_core_test_wrapper::SetUp();
    execute("\\py");
    output_handler.wipe_all();
  }
};

TEST_F(Shell_python, stdin_read) {
  execute("import sys");
  output_handler.wipe_all();

  output_handler.feed_to_prompt("TEST");

  execute("print sys.stdin.read(2).upper()");
  MY_EXPECT_STDOUT_CONTAINS("TE");
  output_handler.wipe_all();

  execute("print sys.stdin.read(3).upper()");
  MY_EXPECT_STDOUT_CONTAINS("ST\n");
  output_handler.wipe_all();

  output_handler.feed_to_prompt("TEST STRING");
  execute("print sys.stdin.readline().upper()");
  MY_EXPECT_STDOUT_CONTAINS("TEST STRING");
  output_handler.wipe_all();

  output_handler.feed_to_prompt("text");
  execute("print raw_input('Enter text: ').upper()");
  MY_EXPECT_STDOUT_CONTAINS("Enter text:");
  MY_EXPECT_STDOUT_CONTAINS("TEXT");
}

// help() doesn't work in Windows because Python 2.7 doens't like
// cp65001
#ifndef _WIN32
TEST_F(Shell_python, help) {
  // Regression test for Bug #24554329
  // CALLING HELP() IN MYSQLSH --PY RESULTS IN ATTRIBUTEERROR
  output_handler.feed_to_prompt("");
  execute("help()");
  EXPECT_EQ("", output_handler.std_err);
  MY_EXPECT_STDOUT_NOT_CONTAINS("AttributeError");
  output_handler.wipe_all();

  output_handler.feed_to_prompt("spam");
  output_handler.feed_to_prompt("");
  execute("help()");
  MY_EXPECT_STDOUT_CONTAINS("no Python documentation found for 'spam'");
  MY_EXPECT_STDOUT_CONTAINS(
      "You are now leaving help and returning to the Python interpreter");
}
#endif

}  // namespace mysqlsh
