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
#include "mysh_config.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "unittest/test_utils/command_line_test.h"
#include "utils/utils_file.h"
#include "utils/utils_path.h"

#ifndef MAX_PATH
const int MAX_PATH = 4096;
#endif

extern mysqlshdk::utils::Version g_target_server_version;

namespace tests {

TEST_F(Command_line_test, bug24912358) {
  // Tests with X Protocol Session
  {
    std::string uri = "--uri=" + _uri;
    execute({_mysqlsh, uri.c_str(), "--sql", "-e", "select -127 << 1.1", NULL});
    MY_EXPECT_MULTILINE_OUTPUT(
        "select -127 << 1.1",
        multiline({"-127 << 1.1", "18446744073709551362"}), _output);
  }  // namespace tests

  {
    std::string uri = "--uri=" + _uri;
    execute(
        {_mysqlsh, uri.c_str(), "--sql", "-e", "select -127 << -1.1", NULL});
    MY_EXPECT_MULTILINE_OUTPUT("select -127 << 1.1",
                               multiline({"-127 << -1.1", "0"}), _output);
  }

  // Tests with Classic Session
  {
    std::string uri = "--uri=" + _mysql_uri;
    execute({_mysqlsh, uri.c_str(), "--sql", "-e", "select -127 << 1.1", NULL});
    MY_EXPECT_MULTILINE_OUTPUT(
        "select -127 << 1.1",
        multiline({"-127 << 1.1", "18446744073709551362"}), _output);
  }

  {
    std::string uri = "--uri=" + _mysql_uri;
    execute(
        {_mysqlsh, uri.c_str(), "--sql", "-e", "select -127 << -1.1", NULL});
    MY_EXPECT_MULTILINE_OUTPUT("select -127 << 1.1",
                               multiline({"-127 << -1.1", "0"}), _output);
  }
}

#ifdef HAVE_V8
TEST_F(Command_line_test, bug23508428) {
  // Test if the xplugin is installed using enableXProtocol in the --dba option
  // In 8.0.4, the mysqlx_cache_cleaner is also supposed to be installed
  // In 8.0.11+, both plugins are built-in, cannot be uninstalled
  std::string uri = "--uri=" + _mysql_uri;

  execute({_mysqlsh, uri.c_str(), "--sqlc", "-e", "uninstall plugin mysqlx;",
           NULL});
  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 5)) {
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "ERROR: 1619 (HY000): Built-in plugins "
        "cannot be deleted");
  }

  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 4)) {
    execute({_mysqlsh, uri.c_str(), "--sqlc", "-e",
             "uninstall plugin mysqlx_cache_cleaner;", NULL});
    if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 5)) {
      MY_EXPECT_CMD_OUTPUT_CONTAINS(
          "ERROR: 1619 (HY000): Built-in plugins "
          "cannot be deleted");
    }
  }

  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 5)) {
    execute(
        {_mysqlsh, uri.c_str(), "--mysql", "--dba", "enableXProtocol", NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "enableXProtocol: Installing plugin "
        "mysqlx...");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("enableXProtocol: done");
  }

  execute({_mysqlsh, uri.c_str(), "--interactive=full", "-e",
           "session.runSql('SELECT COUNT(*) FROM information_schema.plugins "
           "WHERE PLUGIN_NAME in (\"mysqlx\", \"mysqlx_cache_cleaner\")')."
           "fetchOne()",
           NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("[");
  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 4)) {
    MY_EXPECT_CMD_OUTPUT_CONTAINS("    2");
  } else {
    MY_EXPECT_CMD_OUTPUT_CONTAINS("    1");
  }
  MY_EXPECT_CMD_OUTPUT_CONTAINS("]");

  execute({_mysqlsh, uri.c_str(), "--mysql", "--dba", "enableXProtocol", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "enableXProtocol: X Protocol plugin is already enabled and listening for "
      "connections on port " +
      _port);
}
#endif

TEST_F(Command_line_test, bug24905066) {
  // Tests URI formatting using classic protocol
  {
    execute({_mysqlsh, "--mysql", "-i", "--uri",
             "root:@(/path/to/whatever/socket.sock)", NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Creating a Classic session to "
        "'root@/path%2Fto%2Fwhatever%2Fsocket.sock'");
  }

  // Tests URI formatting using X protocol
  {
    execute({_mysqlsh, "--mysqlx", "-i", "--uri",
             "root:@(/path/to/whatever/socket.sock)", "-e", "1", NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Creating an X protocol session to "
        "'root@/path%2Fto%2Fwhatever%2Fsocket.sock'");
  }

  // Tests the connection fails if invalid schema is provided on classic session
  {
    std::string uri = _mysql_uri + "/some_unexisting_schema";

    execute({_mysqlsh, "--mysql", "-i", "--uri", uri.c_str(), NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "MySQL Error 1049 (42000): Unknown database "
        "'some_unexisting_schema'");
  }

  // Tests the connection fails if invalid schema is provided on x session
  {
    std::string uri = _uri + "/some_unexisting_schema";

    execute(
        {_mysqlsh, "--mysqlx", "-i", "--uri", uri.c_str(), "-e", "1", NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS("Unknown database 'some_unexisting_schema'");
  }
}

TEST_F(Command_line_test, bug24967838) {
  // Check if processor is available
  ASSERT_NE(system(NULL), 0);

  // Tests that if error happens when executing script, return code should be
  // different then 0
  {
    char cmd[MAX_PATH];
    std::snprintf(cmd, MAX_PATH,
#ifdef _WIN32
                  "echo no error | %s --uri=%s --sql --database=mysql "
                  "2> nul",
#else
                  "echo \"no error\" | %s --uri=%s --sql --database=mysql "
                  "2> /dev/null",
#endif
                  _mysqlsh, _uri.c_str());

    EXPECT_NE(system(cmd), 0);
  }

  // Test that 0 (130 on Windows) is returned of successful run of the command
  {
    char cmd[MAX_PATH];
    std::snprintf(cmd, MAX_PATH,
#ifdef _WIN32
                  "echo DROP TABLE IF EXISTS test; | %s --uri=%s "
#else
                  "echo \"DROP TABLE IF EXISTS test;\" | %s --uri=%s "
#endif
                  "--sql --database=mysql",
                  _mysqlsh, _uri.c_str());

    EXPECT_EQ(system(cmd), 0);
  }
}

TEST_F(Command_line_test, Bug25974014) {
  // Check if processor is available
  ASSERT_NE(system(NULL), 0);

  char buf[MAX_PATH];
  char cmd[MAX_PATH];
  std::string out;
  FILE *fp;

  // X session
  std::snprintf(cmd, MAX_PATH,
#ifdef _WIN32
                "echo kill CONNECTION_ID(); \\use mysql; | %s --uri=%s --sql "
                "--interactive 2> nul",
#else
                "echo \"kill CONNECTION_ID(); \\use mysql;\" | %s --uri=%s "
                "--sql --interactive 2> /dev/null",
#endif
                _mysqlsh, _uri.c_str());

#ifdef _WIN32
  fp = _popen(cmd, "r");
#else
  fp = popen(cmd, "r");
#endif
  ASSERT_NE(nullptr, fp);
  while (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
    out.append(buf);
  }
  EXPECT_NE(std::string::npos, out.find("Attempting to reconnect"));

  // Classic session
  out.clear();
  std::snprintf(cmd, MAX_PATH,
#ifdef _WIN32
                "echo kill CONNECTION_ID(); \\use mysql; | %s --uri=%s --sql "
                "--interactive 2> nul",
#else
                "echo \"kill CONNECTION_ID(); \\use mysql;\" | %s --uri=%s "
                "--sql --interactive 2> /dev/null",
#endif
                _mysqlsh, _mysql_uri.c_str());

#ifdef _WIN32
  fp = _popen(cmd, "r");
#else
  fp = popen(cmd, "r");
#endif
  ASSERT_NE(nullptr, fp);
  while (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
    out.append(buf);
  }
  EXPECT_NE(std::string::npos, out.find("Attempting to reconnect"));
}

TEST_F(Command_line_test, Bug25105307) {
  execute({_mysqlsh, _uri.c_str(), "--sql", "-e",
           "kill CONNECTION_ID(); \\use mysql;", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("interrupted");
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("Attempting to reconnect");
}

TEST_F(Command_line_test, retain_schema_after_reconnect) {
  // Check if processor is available
  ASSERT_NE(system(NULL), 0);

  char cmd[MAX_PATH];
  std::snprintf(
      cmd, MAX_PATH,
#ifdef _WIN32
      "echo kill CONNECTION_ID(); show tables; | %s --uri=%s/mysql --sql "
      "--interactive 2> nul",
#else
      "echo \"use mysql;\nkill CONNECTION_ID(); show tables;\" | %s --uri=%s "
      "--sql --interactive 2> /dev/null",
#endif
      _mysqlsh, _uri.c_str());

#ifdef _WIN32
  FILE *fp = _popen(cmd, "r");
#else
  FILE *fp = popen(cmd, "r");
#endif
  ASSERT_NE(nullptr, fp);
  char buf[MAX_PATH];
  /* Read the output a line at a time - output it. */
  std::string out;
  while (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
    out.append(buf);
  }
  EXPECT_NE(std::string::npos, out.find("Attempting to reconnect"));
  EXPECT_NE(std::string::npos, out.find("/mysql'.."));
}

TEST_F(Command_line_test, duplicate_not_connected_error) {
  // executing multiple statements while not connected should print
  // Not Connected just once, not once for each statement
  execute({_mysqlsh, "--sql", "-e", "select 1; select 2; select 3;", NULL});

  EXPECT_EQ("ERROR: Not connected.\n", _output);
}

TEST_F(Command_line_test, bug25653170) {
  // Executing scripts with incomplete SQL silently fails
  int rc =
      execute({_mysqlsh, _uri.c_str(), "--sql", "-e", "select 1 /*", NULL});
  EXPECT_EQ(1, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "ERROR: 1064: You have an error in your SQL syntax; check the manual "
      "that corresponds to your MySQL server version for the right syntax to "
      "use near '/*' at line 1");
}

// The following test is temporarily disabled in Windows.
// There's a bug in the shell which is not recognizing (.) as 'localhost'
// Resulting in the following failure in Windows:
// Conflicting options: socket can not be used if host is not 'localhost'.
#ifndef _WIN32
TEST_F(Command_line_test, bug26970629) {
  std::string variable;
  std::string host;
  std::string pwd = "--password=";

  if (!_pwd.empty()) pwd += _pwd;
#ifdef _WIN32
  variable = "named_pipe";
  host = "--host=.";
#else
  variable = "socket";
  host = "--host=localhost";
#endif
  auto session = mysqlshdk::db::mysql::Session::create();
  mysqlshdk::db::Connection_options options;
  options.set_host(_host);
  options.set_port(_mysql_port_number);
  options.set_user(_user);
  options.set_password(_pwd);

  session->connect(options);

  auto result = session->query("show variables like '" + variable + "'");
  auto row = result->fetch_one();
  std::string socket_path = row->get_as_string(1);
  if (!shcore::file_exists(socket_path)) {
    result = session->query("show variables like 'datadir'");
    row = result->fetch_one();
    socket_path = shcore::path::normalize(
        shcore::path::join_path(row->get_as_string(1), socket_path));
  }

  session->close();
  socket_path = "--socket=" + socket_path;

  if (socket_path.empty()) {
    SCOPED_TRACE("Socket/Pipe Connections are Disabled, they must be enabled.");
    FAIL();
  } else {
    std::string usr = "--user=" + _user;

#ifdef HAVE_V8
    execute({_mysqlsh, "--js", usr.c_str(), pwd.c_str(), host.c_str(),
             socket_path.c_str(), "-e", "dba.createCluster('sample')", NULL});
#else
    execute({_mysqlsh, "--py", usr.c_str(), pwd.c_str(), host.c_str(),
             socket_path.c_str(), "-e", "dba.create_cluster('sample')", NULL});
#endif
    SCOPED_TRACE(socket_path.c_str());
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "a MySQL session through TCP/IP is required to perform this operation");
  }
}
#endif
}  // namespace tests
