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

extern "C" const char* g_argv0;

namespace tests {

class Command_line_connection_test : public Command_line_test {
 protected:
  int execute_in_session(const std::string& uri,
                         const std::string& session_type,
                         const std::string& command = "",
                         const std::string& mode = "--sql") {
    std::string used_uri = uri.empty() ? _mysql_uri : uri;

    std::string pwd_param = "--password=" + _pwd;
    std::vector<const char*> args = {
        _mysqlsh,
        session_type.c_str(),
        mode.c_str(),
        used_uri.c_str(),
        "--interactive=full",
        pwd_param.c_str(),
        "-e",
        command.empty() ? "\\status" : command.c_str(),
        NULL};

    return execute(args);
  }

  int test_classic_connection(const std::vector<const char*>& additional_args) {
    std::string pwd_param = "--password=" + _pwd;
    std::vector<const char*> args = {
        _mysqlsh,          "--classic", "--interactive=full",
        pwd_param.c_str(), "-e",        "\\status",
    };

    for (auto arg : additional_args)
      args.emplace_back(arg);

    args.push_back(NULL);

    return execute(args);
  }

  void test_classic_connection_attempt(
      bool expected, const std::vector<const char*>& cmdline_args) {
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
  int ret_val = test_classic_connection({"-u", _user.c_str()});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic Session to '" + _user +
                                "@localhost'");

#ifdef _WIN32
  // On windows a tcp connection is expected
  // If the UT port is the default port, the connection will suceed
  if (_mysql_port == "3306") {
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Your MySQL connection id is");
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "No default schema selected; type \\use <schema> to set one.");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("localhost via TCP/IP");
  } else {
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Can't connect to MySQL server on 'localhost'");
  }
#else
  if (ret_val) {
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Can't connect to local MySQL server through socket");
  } else {
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Localhost via UNIX socket");
  }
#endif
};

TEST_F(Command_line_connection_test, classic_port) {
  test_classic_connection({"-u", _user.c_str(), "-P", _mysql_port.c_str()});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic Session to '" + _user +
                                "@localhost:" + _mysql_port + "'");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Your MySQL connection id is");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("localhost via TCP/IP");
};

TEST_F(Command_line_connection_test, bug25268670) {
  execute({_mysqlsh, "-e",
           "shell.connect({user:'root',password:'',host:'localhost',invalid_"
           "option:'wahtever'})",
           NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "Shell.connect: Invalid values in connection options: invalid_option");
};

TEST_F(Command_line_connection_test, uri_ssl_mode_classic) {
  bool have_ssl = false;

  // Default sslMode as required must work regardless if the server has or not
  // SSL enabled (i.e. commercial vs gpl)
  execute_in_session(_mysql_uri, "--classic",
                     "show variables like 'have_ssl';");
  if (_output.find("YES") != std::string::npos)
    have_ssl = true;

  _output.clear();

  if (have_ssl) {
    // Having SSL enabled sets secure_transport_required=ON
    bool require_secure_transport = false;
    execute_in_session(_mysql_uri, "--classic",
                       "show variables like 'require_secure_transport';");
    if (_output.find("ON") != std::string::npos)
      require_secure_transport = true;

    _output.clear();

    if (!require_secure_transport) {
      execute_in_session(_mysql_uri, "--classic",
                         "set global require_secure_transport=ON;");
      _output.clear();
    }

    // Tests the sslMode=DISABLED to make sure it is not
    // ignored when coming in a URI
    std::string ssl_uri = _mysql_uri + "?sslMode=DISABLED";

    execute_in_session(ssl_uri, "--classic");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic Session to");
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "ERROR: 3159 (HY000): Connections using "
        "insecure transport are prohibited while "
        "--require_secure_transport=ON.");
    _output.clear();

    if (!require_secure_transport)
      execute_in_session(_mysql_uri, "--classic",
                         "set global require_secure_transport=OFF;");
  } else {
    // Having SSL disabled test the sslMode=REQUIRED to make sure
    // it is not ignored when coming in a URI

    std::string ssl_uri = _mysql_uri + "?sslMode=REQUIRED";

    execute_in_session(ssl_uri, "--classic");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic Session to");
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "ERROR: 2026 (HY000): SSL connection error: SSL is required "
        "but the server doesn't support it");
    _output.clear();
  }
}

TEST_F(Command_line_connection_test, uri_ssl_mode_node) {
  bool have_ssl = false;

  // Default sslMode as required must work regardless if the server has or not
  // SSL enabled (i.e. commercial vs gpl)
  execute_in_session(_mysql_uri, "--classic",
                     "show variables like 'have_ssl';");
  if (_output.find("YES") != std::string::npos)
    have_ssl = true;

  _output.clear();

  if (have_ssl) {
    // Having SSL enabled sets secure_transport_required=ON
    bool require_secure_transport = false;
    execute_in_session(_mysql_uri, "--classic",
                       "show variables like 'require_secure_transport';");
    if (_output.find("ON") != std::string::npos)
      require_secure_transport = true;

    _output.clear();

    if (!require_secure_transport) {
      execute_in_session(_mysql_uri, "--classic",
                         "set global require_secure_transport=ON;");
      _output.clear();
    }

    // Tests the sslMode=DISABLED to make sure it is not
    // ignored when coming in a URI
    std::string ssl_uri = _uri + "?sslMode=DISABLED";

    execute_in_session(ssl_uri, "--node");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Node Session to");
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "ERROR: 1045: Secure transport required. To log in you must use "
        "TCP+SSL or UNIX socket connection.");
    _output.clear();

    if (!require_secure_transport)
      execute_in_session(_mysql_uri, "--classic",
                         "set global require_secure_transport=OFF;");
  } else {
    // Having SSL disabled test the sslMode=REQUIRED to make sure
    // it is not ignored when coming in a URI

    std::string ssl_uri = _uri + "?sslMode=REQUIRED";

    execute_in_session(ssl_uri, "--node");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Node Session to");
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "ERROR: 5001: Capability prepare failed for 'tls'");
    _output.clear();
  }
}

TEST_F(Command_line_connection_test, basic_ssl_check_x) {
  const char* ssl_check =
      "SELECT IF(VARIABLE_VALUE='', 'SSL_OFF', 'SSL_ON')"
      " FROM performance_schema.session_status"
      " WHERE variable_name='mysqlx_ssl_cipher';";
  int rc;
  // default
  _output.clear();
  rc = execute({_mysqlsh, _uri.c_str(), "--sql", "-e", ssl_check, nullptr});
  EXPECT_EQ(0, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS("SSL_ON");

  // required
  _output.clear();
  rc = execute({_mysqlsh, _uri.c_str(), "--sql", "--ssl-mode=REQUIRED", "-e",
                ssl_check, nullptr});
  EXPECT_EQ(0, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS("SSL_ON");

  // disabled
  _output.clear();
  rc = execute({_mysqlsh, _uri.c_str(), "--sql", "--ssl-mode=DISABLED", "-e",
                ssl_check, nullptr});
  EXPECT_EQ(0, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS("SSL_OFF");

  // preferred
  _output.clear();
  rc = execute({_mysqlsh, _uri.c_str(), "--sql", "--ssl-mode=PREFERRED", "-e",
                ssl_check, nullptr});
  EXPECT_EQ(0, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS("SSL_ON");
}

TEST_F(Command_line_connection_test, basic_ssl_check_classic) {
  const char* ssl_check =
      "SELECT IF(VARIABLE_VALUE='', 'SSL_OFF', 'SSL_ON')"
      " FROM performance_schema.session_status"
      " WHERE variable_name='ssl_cipher';";
  int rc;
  // default
  _output.clear();
  rc = execute(
      {_mysqlsh, _mysql_uri.c_str(), "--sql", "-e", ssl_check, nullptr});
  EXPECT_EQ(0, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS("SSL_ON");

  // required
  _output.clear();
  rc = execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--ssl-mode=REQUIRED",
                "-e", ssl_check, nullptr});
  EXPECT_EQ(0, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS("SSL_ON");

  // disabled
  _output.clear();
  rc = execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--ssl-mode=DISABLED",
                "-e", ssl_check, nullptr});
  EXPECT_EQ(0, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS("SSL_OFF");

  // preferred
  _output.clear();
  rc = execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "--ssl-mode=PREFERRED",
                "-e", ssl_check, nullptr});
  EXPECT_EQ(0, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS("SSL_ON");
}

}  // namespace tests
