/* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include <string>
#include "unittest/test_utils/command_line_test.h"
#include "utils/utils_file.h"

namespace tests {

TEST_F(Command_line_test, bug26102807) {

  std::string sql = "drop database if exists bug26102807;\n"
                    "create schema bug26102807;\n"
                    "create table bug26102807.test(a varchar (32));\n";

  create_file("bug26102807.sql", sql);
  // Testing with a Node Session
  {
    std::string uri = "--uri=" + _uri;
    execute({_mysqlsh, uri.c_str(), "--sqlx", "--interactive=full", "-f", "bug26102807.sql", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session to '" + _uri_nopasswd + "'");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("drop database if exists bug26102807;\nQuery OK");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("create schema bug26102807;\nQuery OK");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("create table bug26102807.test(a varchar (32));\nQuery OK");

    execute({_mysqlsh, uri.c_str(), "--interactive=full", "-e", "session.sql('select * from bug26102807.test').execute().getColumns()", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("[");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("<Column>");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("]");
  }

  // Testing with a Classic Session
  {
    std::string uri = "--uri=" + _mysql_uri;
    execute({_mysqlsh, uri.c_str(), "--sqlc", "--interactive=full", "-f", "bug26102807.sql", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic session to '" + _mysql_uri_nopasswd + "'");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("drop database if exists bug26102807;\nQuery OK");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("create schema bug26102807;\nQuery OK");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("create table bug26102807.test(a varchar (32));\nQuery OK");

    execute({_mysqlsh, uri.c_str(), "--interactive=full", "-e", "session.runSql('select * from bug26102807.test');", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Empty set");
  }

  shcore::delete_file("bug26102807.sql");
};
}
