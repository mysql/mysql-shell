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

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/command_line_test.h"

// Misc tests via calling the executable

class Mysqlsh_misc : public tests::Command_line_test {};

TEST_F(Mysqlsh_misc, trace_proto) {
  execute({_mysqlsh, _uri.c_str(), "--trace-proto", "--sql", "-e", "select 1",
           nullptr});
  static const char *expected1 = R"*(>>>> SEND Mysqlx.Sql.StmtExecute {
  stmt: "select 1\n"
  namespace: "sql"
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
  catalog: "def"
  collation: 0
  fractional_digits: 0
  length: 1
  flags: 16
})*";
  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected2);
}

TEST_F(Mysqlsh_misc, load_builtin_modules) {
  // Regression test for Bug #26174373
  // Built-in modules should auto-load in non-interactive sessions too
  wipe_out();
  execute({_mysqlsh, "--js", "-e", "println(mysqlx)", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("<mysqlx>");
  wipe_out();
  execute({_mysqlsh, "--js", "-e", "println(mysql)", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("<mysql>");

  wipe_out();
  execute({_mysqlsh, "--py", "-e", "print(mysqlx)", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("<module '__mysqlx__' (built-in)>");
  wipe_out();
  execute({_mysqlsh, "--py", "-e", "print(mysql)", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("<module '__mysql__' (built-in)>");
}

TEST_F(Mysqlsh_misc, connection_attribute) {
  // Regression test for Bug #24735491
  // MYSQL SHELL DOESN'T SET CONNECTION ATTRIBUTES
  execute({_mysqlsh, "--js", _mysql_uri.c_str(), "-e",
           "println(session.runSql('select concat(attr_name, \\'=\\', "
           "attr_value) from performance_schema.session_connect_attrs where "
           "attr_name=\\'program_name\\'').fetchOne()[0])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("program_name=mysqlsh");
}

TEST_F(Mysqlsh_misc, warning_insecure_password) {
#if 0  // passing password through stdin doesn't work so this will freeze
  // Test secure call passing uri with no password (will be prompted)
  execute({_mysqlsh, "root@localhost", nullptr}, "whatever");

  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS(
      "mysqlx: [Warning] Using a password on the command line interface can be "
      "insecure.");
  wipe_out();
#endif
  // Test non secure call passing uri and password with cmd line params
  execute({_mysqlsh, "root@localhost", "-pwhatever", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "mysqlx: [Warning] Using a password on the command line interface can be "
      "insecure.");
  wipe_out();

  execute({_mysqlsh, "root@localhost", "--password=whatever", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "mysqlx: [Warning] Using a password on the command line interface can be "
      "insecure.");
  wipe_out();

  // Test non secure call passing uri with empty password
  execute({_mysqlsh, "root:@localhost", "-e1", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "mysqlx: [Warning] Using a password on the command line interface can be "
      "insecure.");
  wipe_out();

  // Test non secure call passing uri with password
  execute({_mysqlsh, "root:whatever@localhost", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "mysqlx: [Warning] Using a password on the command line interface can be "
      "insecure.");
  wipe_out();
}

TEST_F(Mysqlsh_misc, autocompletion_options) {
  execute({_mysqlsh, "--js", "-e",
      "println(shell.options['devapi.dbObjectHandles'], "
      "        shell.options['autocomplete.nameCache'])", nullptr});
  // disable autocomplete by default, if not running on tty or not interactive
  MY_EXPECT_CMD_OUTPUT_CONTAINS("true false");
  wipe_out();

  execute({_mysqlsh, "-A", "--js", "-e",
      "println(shell.options['devapi.dbObjectHandles'], "
      "        shell.options['autocomplete.nameCache'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("false false");
  wipe_out();

  execute({_mysqlsh, "--name-cache", "--js", "-e",
      "println(shell.options['devapi.dbObjectHandles'], "
      "        shell.options['autocomplete.nameCache'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("true true");
  wipe_out();

  execute({_mysqlsh, "--no-name-cache", "--js", "-e",
      "println(shell.options['devapi.dbObjectHandles'], "
      "        shell.options['autocomplete.nameCache'])", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("false false");
  wipe_out();
}
