/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

namespace mysqlsh {

class Shell_error_printing : public Shell_core_test_wrapper {};

TEST_F(Shell_error_printing, print_error) {
  reset_shell();
  wipe_all();

  current_console()->print_error("test");
  EXPECT_EQ("ERROR: test\n", output_handler.std_out);
  wipe_all();
}

TEST_F(Shell_error_printing, print_diag) {
  reset_shell();
  wipe_all();
  current_console()->print_diag("test");
  EXPECT_EQ("test", output_handler.std_err);
  wipe_all();
  current_console()->print_diag("test\n");
  EXPECT_EQ("test\n", output_handler.std_err);
}

TEST_F(Shell_error_printing, python_stack) {
  reset_shell();
  execute("\\py");
  execute("import sys");
  wipe_all();

  execute("sys.stderr.write('hello world\\n')");
  execute("sys.stderr.write('foo')");
  EXPECT_EQ("hello world\nfoo", output_handler.std_err);

  wipe_all();
  execute("1/0");
  EXPECT_EQ(
      "Traceback (most recent call last):\n"
      "  File \"<string>\", line 1, in <module>\n"
      "ZeroDivisionError: division by zero\n",
      output_handler.std_err);

  wipe_all();
  execute("raise KeyboardInterrupt()");
  EXPECT_EQ(
      "Traceback (most recent call last):\n"
      "  File \"<string>\", line 1, in <module>\n"
      "KeyboardInterrupt\n",
      output_handler.std_err);

  wipe_all();
  execute("raise BaseException()");
  EXPECT_EQ(
      "Traceback (most recent call last):\n"
      "  File \"<string>\", line 1, in <module>\n"
      "BaseException\n",
      output_handler.std_err);

  wipe_all();
  execute("dba.deploy_sandbox_instance(-1, {'password':''})");
  EXPECT_EQ(
      "Traceback (most recent call last):\n"
      "  File \"<string>\", line 1, in <module>\n"
      "ValueError: Dba.deploy_sandbox_instance: "
      "Invalid value for 'port': Please use a valid TCP port number >= 1024 "
      "and <= 65535\n\n",
      output_handler.std_err);
}

#ifdef HAVE_V8
TEST_F(Shell_error_printing, js_stack) {
  _opts->set_interactive(false);
  reset_shell();
  execute("\\js");
  wipe_all();
  execute("foo.bar");
  EXPECT_EQ(
      R"(ReferenceError: foo is not defined at (shell):1:1
in foo.bar
   ^
)",
      output_handler.std_err);

  wipe_all();
  execute("throw 'SomethingWrong'");
  EXPECT_EQ(
      R"(SomethingWrong at (shell):1:1
in throw 'SomethingWrong'
   ^
)",
      output_handler.std_err);

  wipe_all();
  execute("dba.deploySandboxInstance(-1, {'password':''})");
  EXPECT_EQ(
      R"(Dba.deploySandboxInstance: Invalid value for 'port': Please use a valid TCP port number >= 1024 and <= 65535 (ArgumentError)
 at (shell):1:5
in dba.deploySandboxInstance(-1, {'password':''})
       ^
)",
      output_handler.std_err);
}
#endif

TEST_F(Shell_error_printing, sql_error) {
  execute("\\sql");
  execute("\\connect " + _uri);
  wipe_all();

  execute("garbage;");
  EXPECT_EQ(
      "ERROR: 1064: You have an error in your SQL syntax; check the manual "
      "that corresponds to your MySQL server version for the right syntax to "
      "use near 'garbage' at line 1\n",
      output_handler.std_err);
  execute("\\js");
  execute("session.close();");
}

}  // namespace mysqlsh
