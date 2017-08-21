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
