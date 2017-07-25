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

// Tests for executing of script files in different modes
// General checks made:
//  - whether an error will stop processing the file
//  - whether --force will bypass the error in SQL but not elsewhere
//  - whether the whole thing keeps executing if --interactive

// File types: SQL, JS and Python
// Input methods: file (-f) or stream (e.g. mysqlsh < file or cat file|mysqlsh)
// Options: --force, --interactive

#include "mysqlshdk/libs/utils/utils_file.h"
#include "shellcore/shell_core_options.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

namespace shellcore {

class ShellRunScript : public Shell_core_test_wrapper {
 public:
  static void SetUpTestCase() {
    shcore::create_file("good.sql",
                        "select 1;\n"
                        "select 2\n"
                        ", 3;\n"
                        "select 5,\n"
                        "6,\n"
                        "7; select 'end';\n");
    shcore::create_file("bad.sql",
                        "select 1;\n"
                        "drop schema bogusdb;\n"
                        "select 'end';\n");

    shcore::create_file("good_int.py",
                        "print 1\n"
                        "print 2\n"
                        "if 1:\n"
                        "  print 3\n"
                        "\n"
                        "print 'end'\n");
    shcore::create_file("good.py",
                        "print 1\n"
                        "print 2\n"
                        "if 1:\n"
                        "  print 3\n"
                        "print 'end'\n");
    shcore::create_file("bad.py",
                        "print 1\n"
                        "raise Exception()\n"
                        "print 'end'\n");

    shcore::create_file("good.js",
                        "println(1);\n"
                        "println(2);\n"
                        "if (1)\n"
                        "  println(3);\n"
                        "println('end');\n");
    shcore::create_file("bad.js",
                        "println(1)\n"
                        "throw 'error'\n"
                        "println('end')\n");
  }

  static void TearDownTestCase() {
    shcore::delete_file("good.sql");
    shcore::delete_file("bad.sql");
    shcore::delete_file("good.js");
    shcore::delete_file("bad.js");
    shcore::delete_file("good.py");
    shcore::delete_file("good_int.py");
    shcore::delete_file("bad.py");
  }

  void reset(bool force, bool interactive, const std::string &mode) {
    _options->interactive = interactive;
    _options->force = force;

    reset_shell();
    execute("\\connect " + _uri);
    execute(mode);
    wipe_all();
  }

  void test_stream() {
    // _interactive_shell->process_stream(stream, "STDIN", {});
  }
};

#define RESET_BATCH(mode) \
  SCOPED_TRACE("batch");  \
  reset(false, false, "\\" mode)

#define RESET_INTER(mode)      \
  SCOPED_TRACE("interactive"); \
  reset(false, true, "\\" mode)

#define RESET_FORCE_BATCH(mode)  \
  SCOPED_TRACE("batch --force"); \
  reset(true, false, "\\" mode)

#define RESET_FORCE_INTER(mode)        \
  SCOPED_TRACE("interactive --force"); \
  reset(true, true, "\\" mode)

#define RUNFILE_TIL_END(file)                         \
  do {                                                \
    SCOPED_TRACE(file);                               \
    wipe_all();                                       \
    _interactive_shell->process_file(file, {"test"}); \
    MY_EXPECT_STDOUT_CONTAINS("end");                 \
  } while (0)

#define RUNFILE_ABORTS(file)                          \
  do {                                                \
    SCOPED_TRACE(file);                               \
    wipe_all();                                       \
    _interactive_shell->process_file(file, {"test"}); \
    MY_EXPECT_STDOUT_NOT_CONTAINS("end");             \
  } while (0)

#define RUNSTREAM_TIL_END(file)                               \
  do {                                                        \
    std::ifstream s;                                          \
    s.open(file);                                             \
    ASSERT_FALSE(s.fail());                                   \
    SCOPED_TRACE(file);                                       \
    wipe_all();                                               \
    _interactive_shell->process_stream(s, "STDIN", {"test"}); \
    MY_EXPECT_STDOUT_CONTAINS("end");                         \
  } while (0)

#define RUNSTREAM_ABORTS(file)                                \
  do {                                                        \
    std::ifstream s;                                          \
    s.open(file);                                             \
    ASSERT_FALSE(s.fail());                                   \
    SCOPED_TRACE(file);                                       \
    wipe_all();                                               \
    _interactive_shell->process_stream(s, "STDIN", {"test"}); \
    MY_EXPECT_STDOUT_NOT_CONTAINS("end");                     \
  } while (0)

TEST_F(ShellRunScript, sql_file) {
  {
    RESET_BATCH("sql");
    RUNFILE_TIL_END("good.sql");
    EXPECT_EQ("", output_handler.std_err);
    RUNFILE_ABORTS("bad.sql");
    MY_EXPECT_STDERR_CONTAINS(
        "Can't drop database 'bogusdb'; database doesn't exist");
  }
  {
    RESET_INTER("sql");
    RUNFILE_TIL_END("good.sql");
    EXPECT_EQ("", output_handler.std_err);
    RUNFILE_TIL_END("bad.sql");
    MY_EXPECT_STDERR_CONTAINS(
        "Can't drop database 'bogusdb'; database doesn't exist");
  }
  {
    RESET_FORCE_BATCH("sql");
    RUNFILE_TIL_END("good.sql");
    EXPECT_EQ("", output_handler.std_err);
    RUNFILE_TIL_END("bad.sql");
    MY_EXPECT_STDERR_CONTAINS(
        "Can't drop database 'bogusdb'; database doesn't exist");
  }
  {
    RESET_FORCE_INTER("sql");
    RUNFILE_TIL_END("good.sql");
    EXPECT_EQ("", output_handler.std_err);
    RUNFILE_TIL_END("bad.sql");
    MY_EXPECT_STDERR_CONTAINS(
        "Can't drop database 'bogusdb'; database doesn't exist");
  }
}

TEST_F(ShellRunScript, sql_stream) {
  {
    RESET_BATCH("sql");
    RUNSTREAM_TIL_END("good.sql");
    EXPECT_EQ("", output_handler.std_err);
    RUNSTREAM_ABORTS("bad.sql");
    MY_EXPECT_STDERR_CONTAINS(
        "Can't drop database 'bogusdb'; database doesn't exist");
  }
  {
    RESET_INTER("sql");
    RUNSTREAM_TIL_END("good.sql");
    EXPECT_EQ("", output_handler.std_err);
    RUNSTREAM_TIL_END("bad.sql");
    MY_EXPECT_STDERR_CONTAINS(
        "Can't drop database 'bogusdb'; database doesn't exist");
  }
  {
    RESET_FORCE_BATCH("sql");
    RUNSTREAM_TIL_END("good.sql");
    EXPECT_EQ("", output_handler.std_err);
    RUNSTREAM_TIL_END("bad.sql");
    MY_EXPECT_STDERR_CONTAINS(
        "Can't drop database 'bogusdb'; database doesn't exist");
  }
  {
    RESET_FORCE_INTER("sql");
    RUNSTREAM_TIL_END("good.sql");
    EXPECT_EQ("", output_handler.std_err);
    RUNSTREAM_TIL_END("bad.sql");
    MY_EXPECT_STDERR_CONTAINS(
        "Can't drop database 'bogusdb'; database doesn't exist");
  }
}

TEST_F(ShellRunScript, js_file) {
  {
    RESET_BATCH("js");
    RUNFILE_TIL_END("good.js");
    EXPECT_EQ("", output_handler.std_err);
    RUNFILE_ABORTS("bad.js");
    MY_EXPECT_STDERR_CONTAINS("error at bad.js");
  }
  {
    RESET_INTER("js");
    RUNFILE_TIL_END("good.js");
    EXPECT_EQ("", output_handler.std_err);
    RUNFILE_TIL_END("bad.js");
    MY_EXPECT_STDERR_CONTAINS("error");
  }
  {
    RESET_FORCE_BATCH("js");
    RUNFILE_TIL_END("good.js");
    EXPECT_EQ("", output_handler.std_err);
    RUNFILE_ABORTS("bad.js");
    MY_EXPECT_STDERR_CONTAINS("error at bad.js");
  }
  {
    RESET_FORCE_INTER("js");
    RUNFILE_TIL_END("good.js");
    EXPECT_EQ("", output_handler.std_err);
    RUNFILE_TIL_END("bad.js");
    MY_EXPECT_STDERR_CONTAINS("error");
  }
}

TEST_F(ShellRunScript, js_stream) {
  {
    RESET_BATCH("js");
    RUNSTREAM_TIL_END("good.js");
    EXPECT_EQ("", output_handler.std_err);
    RUNSTREAM_ABORTS("bad.js");
    MY_EXPECT_STDERR_CONTAINS("error at STDIN");
  }
  {
    RESET_INTER("js");
    RUNSTREAM_TIL_END("good.js");
    EXPECT_EQ("", output_handler.std_err);
    RUNSTREAM_TIL_END("bad.js");
    MY_EXPECT_STDERR_CONTAINS("error");
  }
  {
    RESET_FORCE_BATCH("js");
    RUNSTREAM_TIL_END("good.js");
    EXPECT_EQ("", output_handler.std_err);
    RUNSTREAM_ABORTS("bad.js");
    MY_EXPECT_STDERR_CONTAINS("error at STDIN");
  }
  {
    RESET_FORCE_INTER("js");
    RUNSTREAM_TIL_END("good.js");
    EXPECT_EQ("", output_handler.std_err);
    RUNSTREAM_TIL_END("bad.js");
    MY_EXPECT_STDERR_CONTAINS("error");
  }
}

TEST_F(ShellRunScript, py_file) {
  {
    RESET_BATCH("py");
    RUNFILE_TIL_END("good.py");
    EXPECT_EQ("", output_handler.std_err);
    RUNFILE_ABORTS("bad.py");
    MY_EXPECT_STDERR_CONTAINS("Exception");
  }
  {
    RESET_INTER("py");
    RUNFILE_TIL_END("good_int.py");
    EXPECT_EQ("", output_handler.std_err);
    RUNFILE_TIL_END("bad.py");
    MY_EXPECT_STDERR_CONTAINS("Exception");
  }
  {
    RESET_FORCE_BATCH("py");
    RUNFILE_TIL_END("good.py");
    EXPECT_EQ("", output_handler.std_err);
    RUNFILE_ABORTS("bad.py");
    MY_EXPECT_STDERR_CONTAINS("Exception");
  }
  {
    RESET_FORCE_INTER("py");
    RUNFILE_TIL_END("good_int.py");
    EXPECT_EQ("", output_handler.std_err);
    RUNFILE_TIL_END("bad.py");
    MY_EXPECT_STDERR_CONTAINS("Exception");
  }
}

TEST_F(ShellRunScript, py_stream) {
  {
    RESET_BATCH("py");
    RUNSTREAM_TIL_END("good.py");
    EXPECT_EQ("", output_handler.std_err);
    RUNSTREAM_ABORTS("bad.py");
    MY_EXPECT_STDERR_CONTAINS("Exception");
  }
  {
    RESET_INTER("py");
    RUNSTREAM_TIL_END("good_int.py");
    EXPECT_EQ("", output_handler.std_err);
    RUNSTREAM_TIL_END("bad.py");
    MY_EXPECT_STDERR_CONTAINS("Exception");
  }
  {
    RESET_FORCE_BATCH("py");
    RUNSTREAM_TIL_END("good.py");
    EXPECT_EQ("", output_handler.std_err);
    RUNSTREAM_ABORTS("bad.py");
    MY_EXPECT_STDERR_CONTAINS("Exception");
  }
  {
    RESET_FORCE_INTER("py");
    RUNSTREAM_TIL_END("good_int.py");
    EXPECT_EQ("", output_handler.std_err);
    RUNSTREAM_TIL_END("bad.py");
    MY_EXPECT_STDERR_CONTAINS("Exception");
  }
}

}  // namespace shellcore
