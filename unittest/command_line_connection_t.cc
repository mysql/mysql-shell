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

extern "C" const char *g_argv0;

namespace tests {

class Command_line_connection_test : public Command_line_test {
protected:
  int test_classic_connection(const std::vector<const char*>& additional_args) {
    std::string pwd_param = "--password=" + _pwd;
    std::vector<const char *> args = { _mysqlsh,
      "--classic",
      "--interactive=full",
      pwd_param.c_str(),
      "-e",
      "\\status",
    };

    for (auto arg: additional_args)
      args.emplace_back(arg);

    args.push_back(NULL);

    return execute(args);
  }

  void test_classic_connection_attempt(bool expected, const std::vector<const char*>& cmdline_args) {

    test_classic_connection(cmdline_args);

    if (expected)
      MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic Session to ");
    else
      MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("Creating a Classic Session to ");
  }

private:
  std::string pwd_param;
};

TEST_F(Command_line_connection_test, classic_no_socket_no_port) {

  int ret_val = test_classic_connection({ "-u", _user.c_str() });

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic Session to '" + _user + "@localhost'");

#ifdef _WIN32
  // On windows a tcp connection is expected
  // If the UT port is the default port, the connection will suceed
  if (_mysql_port == "3306") {
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Your MySQL connection id is");
	  MY_EXPECT_CMD_OUTPUT_CONTAINS("No default schema selected; type \\use <schema> to set one.");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("localhost via TCP/IP");
  }
  else {
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Can't connect to MySQL server on 'localhost'");
  }
#else
  if (ret_val) {
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Can't connect to local MySQL server through socket");
  } else {
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Localhost via UNIX socket");
  }
#endif
};

TEST_F(Command_line_connection_test, classic_port) {

  test_classic_connection({ "-u", _user.c_str(), "-P", _mysql_port.c_str()});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic Session to '" + _user + "@localhost:" + _mysql_port + "'");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Your MySQL connection id is");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("No default schema selected; type \\use <schema> to set one.");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("localhost via TCP/IP");
};

TEST_F(Command_line_connection_test, bug25268670) {
  execute({_mysqlsh, "-e", "shell.connect({user:'root',password:'',host:'localhost',invalid_option:'wahtever'})", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Shell.connect: Invalid values in connection data: invalid_option");
};
}
