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

namespace tests {

TEST_F(Command_line_test, bug24911173) {
  // Test all the --socket(-S)/--port(-P) posibilities
  {
    std::string error("Conflicting options: port and socket can not be used together.");
    execute({_mysqlsh, "--socket=/some/path", "--port=3306", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--socket", "/some/path", "--port=3306", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "-S/some/path", "--port=3306", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "-S", "/some/path", "--port=3306", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--socket=/some/path", "--port", "3306", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--socket", "/some/path", "--port", "3306", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "-S/some/path", "--port", "3306", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "-S", "/some/path", "--port", "3306", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--socket=/some/path", "-P3306", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--socket", "/some/path", "-P3306", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "-S/some/path", "-P3306", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "-S", "/some/path", "-P3306", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--socket=/some/path", "-P", "3306", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--socket", "/some/path", "-P", "3306", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "-S/some/path", "-P", "3306", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "-S", "/some/path", "-P", "3306", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);
  }

  // Tests URI + socket
  {
    std::string error("Conflicting options: socket can not be used if the URI contains a port.");
    execute({_mysqlsh, "--uri=root@localhost:3306", "--socket=/some/path", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=root@localhost:3306", "--socket", "/some/path", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=root@localhost:3306", "-S/some/path", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=root@localhost:3306", "-S", "/some/path", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);
  }

  // Tests two sockets
  {
    std::string error("Conflicting options: provided socket differs from the socket in the URI.");
    execute({_mysqlsh, "--uri=root@/socket", "--socket=/some/path", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=root@/socket", "--socket", "/some/path", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=root@/socket", "-S/some/path", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=root@/socket", "-S", "/some/path", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);
  }

  // Tests URI + port
  {
    std::string error("Conflicting options: port can not be used if the URI contains a socket.");
    execute({_mysqlsh, "--uri=root@/socket", "--port=3310", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=root@/socket", "--port", "3310", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=root@/socket", "-P3310", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=root@/socket", "-P", "3310", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);
  }
};
}
