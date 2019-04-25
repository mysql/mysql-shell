/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "unittest/gtest_clean.h"

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/command_line_test.h"

// Misc tests via calling the executable

class Mysqlsh_misc : public tests::Command_line_test {};

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
  static const char *expected3 = R"*(length: 1
  flags: 16
})*";
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected3);
}

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
  MY_EXPECT_CMD_OUTPUT_CONTAINS("<module '__mysqlx__' (built-in)>");
  wipe_out();
  execute({_mysqlsh, "--py", "-e", "print(mysql)", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("<module '__mysql__' (built-in)>");
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
  // will exec using JS - OK
  execute({_mysqlsh, "-f", "js.foo", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("JS works!");
  wipe_out();

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
