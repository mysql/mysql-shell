/* Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2.0,
 as published by the Free Software Foundation.

 This program is also distributed with certain software (including
 but not limited to OpenSSL) that is licensed under separate terms, as
 designated in a particular file or component or in included license
 documentation.  The authors of MySQL hereby grant you an additional
 permission to link the program and your derivative works with the
 separately licensed software that they have included with MySQL.
 This program is distributed in the hope that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 the GNU General Public License, version 2.0, for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include <string>
#include "unittest/test_utils/command_line_test.h"

namespace tests {

#ifdef _WIN32
#define URI "root@\\\\.\\named.pipe"
#define SOCKET_NAME "named pipe"
#else  // !_WIN32
#define URI "root@/socket"
#define SOCKET_NAME "socket"
#endif  // !_WIN32

TEST_F(Command_line_test, bug24911173) {
  // Test all the --socket(-S)/--port(-P) possibilities
  {
    std::string error("Conflicting options: port and " SOCKET_NAME
                      " cannot be used together.");
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
    std::string error("Conflicting options: " SOCKET_NAME
                      " cannot be used if the URI contains a port.");
    execute(
        {_mysqlsh, "--uri=root@localhost:3306", "--socket=/some/path", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=root@localhost:3306", "--socket", "/some/path",
             NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=root@localhost:3306", "-S/some/path", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=root@localhost:3306", "-S", "/some/path", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);
  }

  // Tests two sockets
  {
    std::string error("Conflicting options: provided " SOCKET_NAME
                      " differs from the " SOCKET_NAME " in the URI.");
    execute({_mysqlsh, "--uri=" URI, "--socket=/some/path", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=" URI, "--socket", "/some/path", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=" URI, "-S/some/path", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=" URI, "-S", "/some/path", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);
  }

  // Tests URI + port
  {
    std::string error(
        "Conflicting options: port cannot be used if the URI contains "
        "a " SOCKET_NAME ".");
    execute({_mysqlsh, "--uri=" URI, "--port=3310", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=" URI, "--port", "3310", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=" URI, "-P3310", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);

    execute({_mysqlsh, "--uri=" URI, "-P", "3310", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(error);
  }
}

}  // namespace tests
