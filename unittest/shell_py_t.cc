/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
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

  execute("print(sys.stdin.read(2).upper())");
  MY_EXPECT_STDOUT_CONTAINS("TE");
  output_handler.wipe_all();

  execute("print(sys.stdin.read(3).upper())");
  MY_EXPECT_STDOUT_CONTAINS("ST\n");
  output_handler.wipe_all();

  output_handler.feed_to_prompt("TEST STRING");
  execute("print(sys.stdin.readline().upper())");
  MY_EXPECT_STDOUT_CONTAINS("TEST STRING");
  output_handler.wipe_all();

  output_handler.feed_to_prompt("text");
  execute("print(raw_input('Enter text: ').upper())");
  MY_EXPECT_STDOUT_CONTAINS("Enter text:");
  MY_EXPECT_STDOUT_CONTAINS("TEXT");
}

// help() doesn't work in Windows because Python 2.7 doesn't like
// cp65001, it also does not work in linux when embedded (generic packages)
#if !defined(_WIN32) && HAVE_PYTHON == 1
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
  // "No Python ..." if PY_VERSION_HEX >= 0x03050000, else "no Python ..."
  MY_EXPECT_STDOUT_CONTAINS("o Python documentation found for 'spam'");
  MY_EXPECT_STDOUT_CONTAINS(
      "You are now leaving help and returning to the Python interpreter");
}
#endif

TEST_F(Shell_python, bug30138755) {
  // executing a loop where each iteration produces a value should output the
  // last value produced
  execute(
      "for i in range(0, 5):\n"
      "    i\n");
  EXPECT_EQ("4\n", output_handler.std_out);
  EXPECT_EQ("", output_handler.std_err);

  wipe_out();

  // executing a statement which does not produce a value should not output any
  // value - before this bug was fixed it would output the penultimate value
  // produced by the loop above
  execute("x = 1");
  EXPECT_EQ("", output_handler.std_out);
  EXPECT_EQ("", output_handler.std_err);
}

TEST_F(Shell_python, crash_with_threads) {
  // Checks that print/console handling works from a Python thread
  execute(R"*(
import threading

def testf():
    print("PRINT")
    import sys
    sys.stdout.write("STDOUT\n")
    sys.stderr.write("STDERR\n")
    raise Exception("exception")

t = threading.Thread(target=testf)
t.start()
t.join()
)*");
}

}  // namespace mysqlsh
