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

#include "ext/linenoise-ng/include/linenoise.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/version.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/command_line_test.h"

// Misc tests via calling the executable

class Mysqlsh_misc : public tests::Command_line_test {};

#ifdef USE_MYSQLX_FULL_PROTO

TEST_F(Mysqlsh_misc, trace_proto) {
  execute({_mysqlsh, _uri.c_str(), "--trace-proto", "--sql", "-e", "select 1",
           nullptr});
  static const char *expected1 = R"*(>>>> SEND Mysqlx.Sql.StmtExecute {
  stmt: "select 1"
})*";

  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected1);

  static const char *expected2 =
      R"*(<<<< RECEIVE Mysqlx.Resultset.ColumnMetaData {
  type: SINT
  name: "1"
  original_name: ""
  table: ""
  original_table: ""
  schema: ""
  catalog: "def")*";
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected2);

  // TODO(kg): optional fields are no longer send by X protocol. expected
  // string split for test backward compatibility. Merge when testing with
  // MySQL Server 8.0.3 will be dropped completely.
  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 25)) {
    static const char *expected3 = R"*(length: 1
  flags: 16
})*";
    MY_EXPECT_CMD_OUTPUT_CONTAINS(expected3);
  } else {
    // Introduced by MySQL Server commit 0077f1a68fdc82a702932
    // Bug#31348202: Wrong result is returned with UNION syntax
    //
    // Change in integer and decimal precision calculation caused length change
    // in column metadata information.
    static const char *expected3 = R"*(length: 2
  flags: 16
})*";
    MY_EXPECT_CMD_OUTPUT_CONTAINS(expected3);
  }
}

#endif

TEST_F(Mysqlsh_misc, load_builtin_modules) {
// Regression test for Bug #26174373
// Built-in modules should auto-load in non-interactive sessions too
#ifdef HAVE_V8
  wipe_out();
  execute({_mysqlsh, "--js", "-e", "println(mysqlx)", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("<mysqlx>");
  wipe_out();
  execute({_mysqlsh, "--js", "-e", "println(mysql)", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("<mysql>");
#endif

  wipe_out();
  execute({_mysqlsh, "--py", "-e", "print(mysqlx)", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("<module 'mysqlsh.mysqlx' (built-in)>");
  wipe_out();
  execute({_mysqlsh, "--py", "-e", "print(mysql)", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("<module 'mysqlsh.mysql' (built-in)>");
}

#ifdef HAVE_V8
TEST_F(Mysqlsh_misc, connection_attribute) {
  // Regression test for Bug #24735491
  // MYSQL SHELL DOESN'T SET CONNECTION ATTRIBUTES
  execute({_mysqlsh, "--js", _mysql_uri.c_str(), "-e",
           "println(session.runSql('select concat(attr_name, \\'=\\', "
           "attr_value) from performance_schema.session_connect_attrs where "
           "attr_name=\\'program_name\\'').fetchOne()[0])",
           nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("program_name=mysqlsh");
}
#endif

TEST_F(Mysqlsh_misc, warning_insecure_password) {
#if 0  // passing password through stdin doesn't work so this will freeze
  // Test secure call passing uri with no password (will be prompted)
  execute({_mysqlsh, "root@localhost", nullptr}, "whatever");

  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS(
      "mysqlsh: [Warning] Using a password on the command line interface can "
      "be insecure.");
  wipe_out();
#endif
  // Test non secure call passing uri and password with cmd line params
  execute({_mysqlsh, _mysql_uri_nopasswd.c_str(), "-pwhatever", nullptr},
          nullptr, nullptr, {"MYSQLSH_TERM_COLOR_MODE=nocolor"});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "WARNING: Using a password on the command line interface can be "
      "insecure.");
  wipe_out();

  execute(
      {_mysqlsh, _mysql_uri_nopasswd.c_str(), "--password=whatever", nullptr},
      nullptr, nullptr, {"MYSQLSH_TERM_COLOR_MODE=nocolor"});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "WARNING: Using a password on the command line interface can be "
      "insecure.");
  wipe_out();

  // Test non secure call passing uri with empty password
  std::string tmp = "root:@localhost:" + _mysql_port;
  execute({_mysqlsh, tmp.c_str(), "-e1", nullptr}, nullptr, nullptr,
          {"MYSQLSH_TERM_COLOR_MODE=nocolor"});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "WARNING: Using a password on the command line interface can be "
      "insecure.");
  wipe_out();

  // Test non secure call passing uri with password
  tmp = "root:whatever@localhost:" + _mysql_port;
  execute({_mysqlsh, tmp.c_str(), nullptr}, nullptr, nullptr,
          {"MYSQLSH_TERM_COLOR_MODE=nocolor"});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "WARNING: Using a password on the command line interface can be "
      "insecure.");
  wipe_out();
}

#ifdef HAVE_V8
TEST_F(Mysqlsh_misc, autocompletion_options) {
  execute({_mysqlsh, "--js", "-e",
           "println(shell.options['devapi.dbObjectHandles'], "
           "        shell.options['autocomplete.nameCache'])",
           nullptr});
  // disable autocomplete by default, if not running on tty or not interactive
  MY_EXPECT_CMD_OUTPUT_CONTAINS("true false");
  wipe_out();

  execute({_mysqlsh, "-A", "--js", "-e",
           "println(shell.options['devapi.dbObjectHandles'], "
           "        shell.options['autocomplete.nameCache'])",
           nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("false false");
  wipe_out();

  execute({_mysqlsh, "--name-cache", "--js", "-e",
           "println(shell.options['devapi.dbObjectHandles'], "
           "        shell.options['autocomplete.nameCache'])",
           nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("true true");
  wipe_out();

  execute({_mysqlsh, "--no-name-cache", "--js", "-e",
           "println(shell.options['devapi.dbObjectHandles'], "
           "        shell.options['autocomplete.nameCache'])",
           nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("false false");
  wipe_out();
}
#endif

TEST_F(Mysqlsh_misc, autodetect_script_type) {
  shcore::create_file("bla.js", "println('JavaScript works');\n");
  shcore::create_file("bla.py", "print('Python works')\n");
  shcore::create_file("py.foo", "print('Python works!')\n");
  shcore::create_file("js.foo", "println('JS works!')\n");
  shcore::create_file("bla.sql", "select 'SQL works';\n");

  try {
    shcore::delete_file(shcore::path::join_path(shcore::get_user_config_path(),
                                                "options.json"));
  } catch (...) {
  }

#ifdef HAVE_V8
  execute({_mysqlsh, "-f", "bla.js", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("JavaScript works");
  wipe_out();
#endif

  execute({_mysqlsh, "-f", "bla.py", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Python works");
  wipe_out();

  execute({_mysqlsh, "-f", "bla.sql", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Not connected");
  wipe_out();

  // will exec using JS - error
  execute({_mysqlsh, "-f", "py.foo", nullptr});
  wipe_out();

  execute({_mysqlsh, "--py", "-f", "py.foo", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Python works!");
  wipe_out();

#ifdef HAVE_V8
  // will exec using SQL (default) - ERROR
  execute({_mysqlsh, "-f", "js.foo", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Not connected");
  wipe_out();

  // will exec using JS - OK
  execute({_mysqlsh, "--js", "-f", "js.foo", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("JS works!");
  wipe_out();
#endif

  // actual file type always overrides --js switch,
  // although if -e is combined with -f, the mode given in cmdline should be
  // used for executing -e (when combining both is supported)
  execute({_mysqlsh, "--py", "-f", "bla.sql", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Not connected");
  wipe_out();

  // filetype doesn't type language, so the cmdline option overrides it
  execute({_mysqlsh, "--py", "-f", "py.foo", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Python works!");
  wipe_out();

#ifdef HAVE_V8
  // now change the default startup mode and persist it
  // Bug#27861407  SHELL DOESN'T EXECUTE SCRIPTS USING CORRECT LANGUAGE
  execute({_mysqlsh, "--js", "-e",
           "shell.options.setPersist('defaultMode', 'js')", nullptr});

  execute({_mysqlsh, "-f", "bla.js", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("JavaScript works");
  wipe_out();
#endif

  execute({_mysqlsh, "-f", "bla.py", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Python works");
  wipe_out();

#ifdef HAVE_V8
  execute({_mysqlsh, "-f", "js.foo", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("JS works!");
  wipe_out();
#endif

  execute({_mysqlsh, "-f", "py.foo", nullptr});

  wipe_out();

  execute({_mysqlsh, "-f", "bla.sql", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Not connected");
  wipe_out();

#ifdef HAVE_V8
  // again with Python as default
  execute({_mysqlsh, "--js", "-e",
           "shell.options.setPersist('defaultMode', 'py')", nullptr});
  execute({_mysqlsh, "-f", "bla.js", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("JavaScript works");
  wipe_out();
#endif

  execute({_mysqlsh, "-f", "bla.py", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Python works");
  wipe_out();

#ifdef HAVE_V8
  execute({_mysqlsh, "-f", "js.foo", nullptr});
  wipe_out();
#endif

  execute({_mysqlsh, "-f", "py.foo", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Python works!");
  wipe_out();

  shcore::delete_file(
      shcore::path::join_path(shcore::get_user_config_path(), "options.json"));
  shcore::delete_file("bla.js");
  shcore::delete_file("bla.py");
  shcore::delete_file("bla.sql");
}

TEST_F(Mysqlsh_misc, run_python_module) {
  execute({_mysqlsh, "--pym", "unittest", "-h", nullptr});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("run tests from test_module");

  wipe_out();
}

TEST_F(Mysqlsh_misc, js_sys_argv) {
  shcore::create_file("sysargv.js", "println(sys.argv);\n");

  // NOTE: Uses --json because in JS the array is printed in pretty format
  // (multiline) so we avoid dealing with line ending differences on this test
  execute({_mysqlsh, "--json=raw", "-f", "sysargv.js", "1", "2", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("\\\"sysargv.js\\\"");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("\\\"sysargv.js\\\"");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("\\\"1\\\"");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("\\\"2\\\"");
  wipe_out();

  execute({_mysqlsh, "--interactive", "--json=raw", "-f", "sysargv.js", "3",
           "4", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("\\\"sysargv.js\\\"");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("\\\"3\\\"");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("\\\"4\\\"");
  wipe_out();

  try {
    shcore::delete_file("sysargv.js");
  } catch (...) {
  }
}

TEST_F(Mysqlsh_misc, py_sys_argv) {
  shcore::create_file("sysargv.py", R"(
import sys
print(sys.argv)
)");

  execute({_mysqlsh, "--py", "-f", "sysargv.py", "1", "2", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("['sysargv.py', '1', '2']");
  wipe_out();

  execute({_mysqlsh, "--interactive", "--py", "-f", "sysargv.py", "3", "4",
           nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("['sysargv.py', '3', '4']");
  wipe_out();

  try {
    shcore::delete_file("sysargv.py");
  } catch (...) {
  }
}

TEST_F(Mysqlsh_misc, invalid_unicode_linoise_custom_command) {
  linenoiseRegisterCustomCommand(
      "\x18\x18",  // CTRL-X, CTRL-X
      [](const char *, void *, char **) { return -1; }, nullptr);

  int specialValue = 1048;    // 0000 0100 0001 1000, russian unicode shift+B,
  int sepcialValue2 = 65304;  // 1111 1111 0001 1000, japanese unicode '8'
  int correctValue = 0x18;    // 0000 0000 0001 1000, CTRL-X

  EXPECT_FALSE(linonoiseTestExtendedCharacter(specialValue));
  EXPECT_FALSE(linonoiseTestExtendedCharacter(correctValue, specialValue));

  EXPECT_FALSE(linonoiseTestExtendedCharacter(sepcialValue2));
  EXPECT_FALSE(linonoiseTestExtendedCharacter(correctValue, sepcialValue2));
}
