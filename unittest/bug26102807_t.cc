/* Copyright (c) 2017, 2024, Oracle and/or its affiliates.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2.0,
 as published by the Free Software Foundation.

 This program is designed to work with certain software (including
 but not limited to OpenSSL) that is licensed under separate terms,
 as designated in a particular file or component or in included license
 documentation.  The authors of MySQL hereby grant you an additional
 permission to link the program and your derivative works with the
 separately licensed software that they have either included with
 the program or referenced in the documentation.

 This program is distributed in the hope that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 the GNU General Public License, version 2.0, for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include <string>
#include "unittest/test_utils/command_line_test.h"
#include "utils/utils_file.h"

namespace tests {

TEST_F(Command_line_test, bug26102807) {
  std::string sql =
      "drop database if exists bug26102807;\n"
      "create schema bug26102807;\n"
      "create table bug26102807.test(a varchar (32));\n";

  create_file("bug26102807.sql", sql);
  // Testing with a Node Session
  {
    std::string uri = "--uri=" + _uri;
    execute({_mysqlsh, uri.c_str(), "--sqlx", "--interactive=full", "-f",
             "bug26102807.sql", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session to '" +
                                  _uri_nopasswd + "'");
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "drop database if exists bug26102807;\nQuery OK");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("create schema bug26102807;\nQuery OK");
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "create table bug26102807.test(a varchar (32));\nQuery OK");

#ifdef HAVE_JS
    execute(
        {_mysqlsh, uri.c_str(), "--js", "--interactive=full", "-e",
         "session.sql('select * from bug26102807.test').execute().getColumns()",
         NULL});
#else
    execute({_mysqlsh, uri.c_str(), "--py", "--interactive=full", "-e",
             "session.sql('select * from "
             "bug26102807.test').execute().get_columns()",
             NULL});
#endif
    MY_EXPECT_CMD_OUTPUT_CONTAINS("[");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("<Column>");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("]");
  }

  // Testing with a Classic Session
  {
    std::string uri = "--uri=" + _mysql_uri;
    execute({_mysqlsh, uri.c_str(), "--sqlc", "--interactive=full", "-f",
             "bug26102807.sql", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic session to '" +
                                  _mysql_uri_nopasswd + "'");
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "drop database if exists bug26102807;\nQuery OK");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("create schema bug26102807;\nQuery OK");
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "create table bug26102807.test(a varchar (32));\nQuery OK");

#ifdef HAVE_JS
    execute({_mysqlsh, uri.c_str(), "--js", "--interactive=full", "-e",
             "session.runSql('select * from bug26102807.test');", NULL});
#else
    execute({_mysqlsh, uri.c_str(), "--py", "--interactive=full", "-e",
             "session.run_sql('select * from bug26102807.test');", NULL});
#endif
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Empty set");
  }

  shcore::delete_file("bug26102807.sql");
}
}  // namespace tests
