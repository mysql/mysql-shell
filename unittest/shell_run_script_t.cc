/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

// Tests for executing of script files in different modes
// General checks made:
//  - whether an error will stop processing the file
//  - whether --force will bypass the error in SQL but not elsewhere
//  - whether the whole thing keeps executing if --interactive

// File types: SQL, JS and Python
// Input methods: file (-f) or stream (e.g. mysqlsh < file or cat file|mysqlsh)
// Options: --force, --interactive

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/command_line_test.h"

extern mysqlshdk::utils::Version g_target_server_version;

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
    shcore::create_file("badsyn.py",
                        "   if:\n"
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
    shcore::create_file("badsyn.js",
                        "){}\n"
                        "println('end')\n");
  }

  static void TearDownTestCase() {
    shcore::delete_file("good.sql");
    shcore::delete_file("bad.sql");
    shcore::delete_file("good.js");
    shcore::delete_file("bad.js");
    shcore::delete_file("badsyn.js");
    shcore::delete_file("good.py");
    shcore::delete_file("good_int.py");
    shcore::delete_file("bad.py");
    shcore::delete_file("badsyn.py");
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

class ShellExeRunScript : public tests::Command_line_test {
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
    shcore::create_file("badsyn.py",
                        "   if:\n"
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
    shcore::create_file("badsyn.js",
                        "){}\n"
                        "println('end')\n");
    shcore::create_file("reconnect_mysql.js",
                        "session.runSql('select 1');\n"
                        "try {session.runSql('kill ?', "
                        "[session.runSql('select connection_id()')"
                        ".fetchOne()[0]])} catch(e){};\n"
                        "session.runSql('select 1');\n"
                        "session.runSql('select 1');\n");
    shcore::create_file("reconnect_mysqlx.js",
                        "session.sql('select 1').execute();\n"
                        "try {session.sql('kill ?').bind("
                        "[session.sql('select connection_id()').execute()"
                        ".fetchOne()[0]]).execute(); } catch(e){};\n"
                        "session.sql('select 1').execute();\n"
                        "session.sql('select 1').execute();\n");
    // disable prompt theme, so we can check output along with prompt
    putenv(const_cast<char *>("MYSQLSH_PROMPT_THEME=invalid"));
  }

  static void TearDownTestCase() {
    shcore::delete_file("good.sql");
    shcore::delete_file("bad.sql");
    shcore::delete_file("good.js");
    shcore::delete_file("bad.js");
    shcore::delete_file("badsyn.js");
    shcore::delete_file("good.py");
    shcore::delete_file("good_int.py");
    shcore::delete_file("bad.py");
    shcore::delete_file("badsyn.py");
    shcore::delete_file("reconnect_mysql.js");
    shcore::delete_file("reconnect_mysqlx.js");
    putenv(const_cast<char *>("MYSQLSH_PROMPT_THEME="));
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

TEST_F(ShellExeRunScript, sql_file) {
  int rc;
  wipe_out();
  rc = execute({_mysqlsh, _uri.c_str(), "--sql", "-f", "good.sql", nullptr});
  EXPECT_EQ(0, rc);
  static const char *result1 = R"(1
1
2\t3
2\t3
5\t6\t7
5\t6\t7
end
end)";
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("Switching");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(shcore::str_replace(result1, "\\t", "\t"));

  wipe_out();
  rc = execute({_mysqlsh, _uri.c_str(), "--sql", "-f", "bad.sql", nullptr});
  EXPECT_EQ(1, rc);
  static const char *result2 = R"(1
1
ERROR: 1008: Can't drop database 'bogusdb'; database doesn't exist)";
  MY_EXPECT_CMD_OUTPUT_CONTAINS(result2);
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

TEST_F(ShellExeRunScript, js_file) {
  int rc;
  wipe_out();
  rc = execute({_mysqlsh, _uri.c_str(), "--js", "-f", "good.js", nullptr});
  // no error, exit code 0
  EXPECT_EQ(0, rc);
  static const char *result1 = R"(1
2
3
end)";
  MY_EXPECT_CMD_OUTPUT_CONTAINS(result1);

  // Ensures switching message doesn't appear for default/initial mode
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("Switching");

  wipe_out();
  rc = execute({_mysqlsh, _uri.c_str(), "--js", "-f", "bad.js", nullptr});
  // error, exit code not-0
  EXPECT_EQ(1, rc);
  static const char *result2 = R"(1
error at bad.js:2:0
in throw 'error'
   ^)";
  MY_EXPECT_CMD_OUTPUT_CONTAINS(result2);
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("end");

  wipe_out();
  rc = execute({_mysqlsh, _uri.c_str(), "--js", "-f", "badsyn.js", nullptr});
  // error, exit code not-0
  EXPECT_EQ(1, rc);
  static const char *result3 =
      R"(SyntaxError: Unexpected token ) at badsyn.js:1:0
in ){}
   ^)";
  MY_EXPECT_CMD_OUTPUT_CONTAINS(result3);
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("end");

  wipe_out();
  rc = execute({_mysqlsh, "--js", "-f", "good.js", nullptr});
  EXPECT_EQ(0, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(result1);

  wipe_out();
  rc = execute({_mysqlsh, "--js", "-f", "bad.js", nullptr});
  EXPECT_EQ(1, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(result2);
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("end");
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

TEST_F(ShellExeRunScript, py_file) {
  int rc;
  wipe_out();
  rc = execute({_mysqlsh, _uri.c_str(), "--py", "-f", "good.py", nullptr});
  // no error, exit code 0
  EXPECT_EQ(0, rc);
  static const char *result1 = R"(1
2
3
end)";
  MY_EXPECT_CMD_OUTPUT_CONTAINS(result1);

  // Ensures switching message doesn't appear for default/initial mode
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("Switching");

  wipe_out();
  rc = execute({_mysqlsh, _uri.c_str(), "--py", "-f", "bad.py", nullptr});
  // error, exit code not-0
  EXPECT_EQ(1, rc);
  static const char *result2 = R"(1
Traceback (most recent call last):
  File "<string>", line 2, in <module>
Exception)";
  MY_EXPECT_CMD_OUTPUT_CONTAINS(result2);
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("end");

  wipe_out();
  rc = execute({_mysqlsh, _uri.c_str(), "--py", "-f", "badsyn.py", nullptr});
  // error, exit code not-0
  EXPECT_EQ(1, rc);
  static const char *result3 = R"(File "<string>", line 1
    if:
    ^
IndentationError: unexpected indent)";
  MY_EXPECT_CMD_OUTPUT_CONTAINS(result3);
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("end");

  wipe_out();
  rc = execute({_mysqlsh, "--py", "-f", "good.py", nullptr});
  EXPECT_EQ(0, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(result1);

  wipe_out();
  rc = execute({_mysqlsh, "--py", "-f", "bad.py", nullptr});
  EXPECT_EQ(1, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(result2);
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("end");
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

TEST_F(ShellExeRunScript, file_from_stdin) {
  static const char *good_js_output = R"(1
2
3
end)";

  {
    wipe_out();
    int rc =
        execute({_mysqlsh, _uri.c_str(), "--js", nullptr}, nullptr, "good.js");
    // no error, exit code 0
    EXPECT_EQ(0, rc);
    MY_EXPECT_CMD_OUTPUT_CONTAINS(good_js_output);
  }
}

#ifndef _WIN32
// writing the password to terminal is not reliable on Windows, if someone
// is using the keybord while the tests are running, it may mess up the input

TEST_F(ShellExeRunScript, file_prompt_password) {
  static const char *good_js_output = R"(1
2
3
end)";

  {
    wipe_out();
    int rc = execute(
        {_mysqlsh, _uri_nopasswd.c_str(), "--js", "-f", "good.js", nullptr},
        _pwd.c_str());
    // no error, exit code 0
    EXPECT_EQ(0, rc);
    MY_EXPECT_CMD_OUTPUT_CONTAINS(good_js_output);
  }

  {
    wipe_out();
    std::string password_followed_by_whitespace = _pwd + " ";
    int rc = execute(
        {_mysqlsh, _uri_nopasswd.c_str(), "--js", "-f", "good.js", nullptr},
        password_followed_by_whitespace.c_str());
    // wrong password, exit code 1
    EXPECT_EQ(1, rc);
    if (g_target_server_version >= mysqlshdk::utils::Version(8, 0, 12)) {
      MY_EXPECT_CMD_OUTPUT_CONTAINS(
          "Access denied for user 'root'@'localhost'");
    } else {
      MY_EXPECT_CMD_OUTPUT_CONTAINS("Invalid user or password");
    }
  }

  {
    wipe_out();
    std::string wrong_password = _pwd + "zaq12WSX";
    int rc = execute(
        {_mysqlsh, _uri_nopasswd.c_str(), "--js", "-f", "good.js", nullptr},
        wrong_password.c_str());
    // wrong password, exit code 1
    EXPECT_EQ(1, rc);
    if (g_target_server_version >= mysqlshdk::utils::Version(8, 0, 12)) {
      MY_EXPECT_CMD_OUTPUT_CONTAINS(
          "Access denied for user "
          "'root'@'localhost'");
    } else {
      MY_EXPECT_CMD_OUTPUT_CONTAINS("Invalid user or password");
    }
  }
}

TEST_F(ShellExeRunScript, file_prompt_password_cancel) {
  wipe_out();
  int rc = execute(
      {_mysqlsh, _uri_nopasswd.c_str(), "--js", "-f", "good.js", nullptr},
      "pass\x03");
  // cancelled, exit code 1
  EXPECT_EQ(1, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Cancelled");
}

TEST_F(ShellExeRunScript, file_from_stdin_prompt_password) {
  static const char *good_js_output = R"(1
2
3
end)";

  {
    wipe_out();
    int rc = execute({_mysqlsh, _uri_nopasswd.c_str(), "--js", nullptr},
                     _pwd.c_str(), "good.js");
    // no error, exit code 0
    EXPECT_EQ(0, rc);
    MY_EXPECT_CMD_OUTPUT_CONTAINS(good_js_output);
  }
}

#endif  // ! _WIN32

TEST_F(ShellExeRunScript, reconnect_mysql_session) {
  static constexpr auto first_execution =
      R"(No default schema selected; type \use <schema> to set one.
NOTE: MYSQLSH_PROMPT_THEME prompt theme file 'invalid' does not exist.
mysql-js []> session.runSql('select 1');
+---+
| 1 |
+---+
| 1 |
+---+
1 row in set)";
  static constexpr auto second_execution =
      R"(mysql-js []> session.runSql('select 1');
ClassicSession.runSql: Lost connection to MySQL server during query (MySQL Error 2013)
The global session got disconnected..
Attempting to reconnect to 'mysql://)";
  static constexpr auto third_execution =
      R"(The global session was successfully reconnected.
mysql-js []> session.runSql('select 1');
+---+
| 1 |
+---+
| 1 |
+---+
1 row in set)";

  wipe_out();
  int rc = execute(
      {_mysqlsh, _mysql_uri.c_str(), "--js", "--interactive=full", nullptr},
      nullptr, "reconnect_mysql.js");
  // no error, exit code 0
  EXPECT_EQ(0, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(first_execution);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(second_execution);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(third_execution);
}

TEST_F(ShellExeRunScript, reconnect_mysqlx_session) {
  static constexpr auto first_execution =
      R"(No default schema selected; type \use <schema> to set one.
NOTE: MYSQLSH_PROMPT_THEME prompt theme file 'invalid' does not exist.
mysql-js []> session.sql('select 1').execute();
+---+
| 1 |
+---+
| 1 |
+---+
1 row in set)";
  static constexpr auto second_execution =
      R"(mysql-js []> session.sql('select 1').execute();
MySQL server has gone away (MySQL Error 2006)
The global session got disconnected..
Attempting to reconnect to 'mysqlx://)";
  static constexpr auto third_execution =
      R"(The global session was successfully reconnected.
mysql-js []> session.sql('select 1').execute();
+---+
| 1 |
+---+
| 1 |
+---+
1 row in set)";
  wipe_out();
  int rc =
      execute({_mysqlsh, _uri.c_str(), "--js", "--interactive=full", nullptr},
              nullptr, "reconnect_mysqlx.js");
  // no error, exit code 0
  EXPECT_EQ(0, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(first_execution);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(second_execution);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(third_execution);
}

}  // namespace shellcore
