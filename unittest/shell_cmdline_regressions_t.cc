/* Copyright (c) 2017, 2023, Oracle and/or its affiliates.

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
  }

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
        "ERROR: 1619 (HY000) at line 1: Built-in plugins "
        "cannot be deleted");
  }

  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 4)) {
    execute({_mysqlsh, uri.c_str(), "--sqlc", "-e",
             "uninstall plugin mysqlx_cache_cleaner;", NULL});
    if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 5)) {
      MY_EXPECT_CMD_OUTPUT_CONTAINS(
          "ERROR: 1619 (HY000) at line 1: Built-in plugins "
          "cannot be deleted");
    }
  }

  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 5)) {
    execute(
        {_mysqlsh, uri.c_str(), "--mysql", "--dba", "enableXProtocol", NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "enableXProtocol: Installing plugin "
        "mysqlx...");
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "enableXProtocol: successfully installed the X protocol plugin!");
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
      "enableXProtocol: The X Protocol plugin is already enabled and listening "
      "for connections on port " +
      _port);
}
#endif

TEST_F(Command_line_test, bug24905066) {
  // Tests URI formatting using classic protocol
  {
#ifdef _WIN32
    execute({_mysqlsh, "--mysql", "-i", "--uri",
             "root:@\\\\.\\(d:\\path\\to\\whatever\\socket.sock)", NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Creating a Classic session to "
        "'root@\\\\.\\d%3A%5Cpath%5Cto%5Cwhatever%5Csocket.sock'");
#else   // !_WIN32
    execute({_mysqlsh, "--mysql", "-i", "--uri",
             "root:@(/path/to/whatever/socket.sock)", NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Creating a Classic session to "
        "'root@/path%2Fto%2Fwhatever%2Fsocket.sock'");
#endif  // !_WIN32
  }

#ifndef _WIN32
  // Tests URI formatting using X protocol
  {
    execute({_mysqlsh, "--mysqlx", "-i", "--uri",
             "root:@(/path/to/whatever/socket.sock)", "-e", "1", NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Creating an X protocol session to "
        "'root@/path%2Fto%2Fwhatever%2Fsocket.sock'");
  }
#endif  // !_WIN32

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
                "echo kill CONNECTION_ID(); use mysql; | %s --uri=%s --sql "
                "--interactive 2> nul",
#else
                "echo \"kill CONNECTION_ID(); use mysql;\" | %s --uri=%s "
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
                "echo kill CONNECTION_ID(); use mysql; | %s --uri=%s --sql "
                "--interactive 2> nul",
#else
                "echo \"kill CONNECTION_ID(); use mysql;\" | %s --uri=%s "
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
           "kill CONNECTION_ID(); use mysql;", NULL});

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
      "ERROR: 1064 at line 1: You have an error in your SQL syntax; check the "
      "manual "
      "that corresponds to your MySQL server version for the right syntax to "
      "use near '/*' at line 1");
}

#ifdef HAVE_V8
TEST_F(Command_line_test, bug28814112_js) {
  // SEG-FAULT WHEN CALLING SHELL.SETCURRENTSCHEMA() WITHOUT AN ACTIVE SHELL
  // SESSION
  int rc = execute({_mysqlsh, "-e", "shell.setCurrentSchema('mysql')", NULL});
  EXPECT_EQ(1, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "Shell.setCurrentSchema: An open session is required to perform this "
      "operation. "
      "(RuntimeError)");
}
#else
TEST_F(Command_line_test, bug28814112_py) {
  // SEG-FAULT WHEN CALLING SHELL.SETCURRENTSCHEMA() WITHOUT AN ACTIVE SHELL
  // SESSION
  int rc = execute({_mysqlsh, "-e", "shell.set_current_schema('mysql')", NULL});
  EXPECT_EQ(1, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "Shell.set_current_schema: An open session is required to perform this "
      "operation.");
}
#endif

TEST_F(Command_line_test, batch_ansi_quotes) {
  // Check if processor is available
  ASSERT_NE(system(NULL), 0);
  char cmd[MAX_PATH];
  std::snprintf(
      cmd, MAX_PATH,
#ifdef _WIN32
      R"(echo set sql_mode = 'ANSI_QUOTES'; select "\"";select version();#";)"
#else
      R"(echo "set sql_mode = 'ANSI_QUOTES'; select \"\\\"\";select version();#\"")"
#endif
      " | %s --uri=%s --sql 2>&1",
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
  EXPECT_NE(std::string::npos,
            out.find(R"(Unknown column '\";select version();#')"));
}

TEST_F(Command_line_test, user_before_uri) {
  // Bug#35020175 -u/--user And --password Options Are Ignored When Specified
  // Before Uri
  mysqlshdk::db::Connection_options options;
  options.set_host(_host);
  options.set_port(std::stoi(_mysql_port));
  options.set_user(_user);
  options.set_password(_pwd);

  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(options);
  session->execute("drop user if exists testuser1@'%'");
  session->execute("drop user if exists testuser2@'%'");
  session->execute("create user testuser1@'%' identified by 'pass1'");
  session->execute("create user testuser2@'%' identified by 'pass2'");

  // -uuser mysql://localhost should become user@localhost
  // mysql://localhost -uuser should become user@localhost
  // -uuser mysql://localhost -utestuser2 should become testuser2@localhost
  // -uuser mysql://testuser2@localhost should become testuser2@localhost
  // mysql://user@localhost -utestuser2 should become testuser2@localhost

  {
    std::string uri = shcore::str_format("mysql://testuser1:pass1@localhost:%s",
                                         _mysql_port.c_str());
    execute({_mysqlsh, uri.c_str(), "--sql", "-e",
             "select concat('test1=', current_user())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("test1=testuser1@%");
  }
  {
    std::string uri = shcore::str_format("mysql://testuser1@localhost:%s",
                                         _mysql_port.c_str());
    execute({_mysqlsh, uri.c_str(), "-ppass1", "--sql", "-e",
             "select concat('test2=', current_user())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("test2=testuser1@%");
  }
  {
    std::string uri =
        shcore::str_format("mysql://localhost:%s", _mysql_port.c_str());
    execute({_mysqlsh, "-utestuser1", "-ppass1", uri.c_str(), "--sql", "-e",
             "select concat('test3=', current_user())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("test3=testuser1@%");
  }
  {
    std::string uri =
        shcore::str_format("mysql://localhost:%s", _mysql_port.c_str());
    execute({_mysqlsh, "-utestuser1", uri.c_str(), "-ppass1", "--sql", "-e",
             "select concat('test4=', current_user())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("test4=testuser1@%");
  }
  {
    std::string uri =
        shcore::str_format("mysql://localhost:%s", _mysql_port.c_str());
    execute({_mysqlsh, uri.c_str(), "-ppass1", "-utestuser1", "--sql", "-e",
             "select concat('test5=', current_user())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("test5=testuser1@%");
  }
  {
    std::string uri =
        shcore::str_format("mysql://localhost:%s", _mysql_port.c_str());
    execute({_mysqlsh, "-ppass1", uri.c_str(), "-utestuser1", "--sql", "-e",
             "select concat('test6=', current_user())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("test6=testuser1@%");
  }
  {
    std::string uri =
        shcore::str_format("mysql://localhost:%s", _mysql_port.c_str());
    execute({_mysqlsh, "-utestuser1", "-ppass1", uri.c_str(), "-utestuser2",
             "-ppass2", "--sql", "-e",
             "select concat('test7=', current_user())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("test7=testuser2@%");
  }
  {
    std::string uri = shcore::str_format("mysql://testuser2:pass2@localhost:%s",
                                         _mysql_port.c_str());
    execute({_mysqlsh, "-utestuser1", "-ppass1", uri.c_str(), "--sql", "-e",
             "select concat('test8=', current_user())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("test8=testuser2@%");
  }
  {
    std::string uri = shcore::str_format("mysql://testuser1@localhost:%s",
                                         _mysql_port.c_str());
    execute({_mysqlsh, uri.c_str(), "-utestuser2", "-ppass2", "--sql", "-e",
             "select concat('test9=', current_user())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("test9=testuser2@%");
  }

  session->execute("drop user testuser1@'%'");
  session->execute("drop user testuser2@'%'");
}

TEST_F(Command_line_test, socket_and_port) {
  // Bug#35023480	shell cannot connect if both port and socket file/named
  // path specified

  mysqlshdk::db::Connection_options options;
  options.set_host(_host);
  options.set_port(std::stoi(_mysql_port));
  options.set_user(_user);
  options.set_password(_pwd);

  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(options);
  session->execute("drop user if exists testuser1@'%'");
  session->execute("create user testuser1@'%' identified by 'pass1'");

  std::string port_ = "-P" + _mysql_port;
  const char *port = port_.c_str();

  std::string socket_ = "-S" + _mysql_socket;
  const char *socket = socket_.c_str();

  const char *user = "-utestuser1";
  const char *pwd = "-ppass1";

#ifdef _WIN32
  // just a plain successful connect to make caching_sha2_password work

  {
    execute({_mysqlsh, user, pwd, "-h.", socket, "--js", "-e",
             "print(shell.status())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF(
        "MySQL Error 2061 (HY000): Authentication plugin "
        "'caching_sha2_password' reported error: Authentication requires "
        "secure connection.",
        "Named pipe:");
  }
#endif

#ifdef _WIN32
#define CONNECT_TO_DOT_HOST_MESSAGE \
  "MySQL Error 2017 (HY000): Can't open named pipe to host: ."
#else
#define CONNECT_TO_DOT_HOST_MESSAGE \
  "MySQL Error 2005: No such host is known '.'"
#endif
  {
    execute({_mysqlsh, user, pwd, "-h.", port, "--js", "-e",
             "print(shell.status())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(CONNECT_TO_DOT_HOST_MESSAGE);
  }
  {
    execute({_mysqlsh, user, pwd, socket, "-h.", "--js", "-e",
             "print(shell.status())", NULL});
#ifdef _WIN32
    MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF(
        "MySQL Error 2061 (HY000): Authentication plugin "
        "'caching_sha2_password' reported error: Authentication requires "
        "secure connection.",
        "Named pipe:");
#else
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "MySQL Error 2005: No such host is known '.'");
#endif
  }
  {
    execute({_mysqlsh, user, pwd, port, "-h.", "--js", "-e",
             "print(shell.status())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(CONNECT_TO_DOT_HOST_MESSAGE);
  }

  {
    execute({_mysqlsh, user, pwd, socket, port, "--js", "-e",
             "print(shell.status())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("localhost via TCP/IP");
  }
  {
    execute({_mysqlsh, user, pwd, port, socket, "--js", "-e",
             "print(shell.status())", NULL});
#ifdef _WIN32
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Named pipe:");
#else
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Localhost via UNIX socket");
#endif
  }
  {
    execute({_mysqlsh, user, pwd, "-h.", socket, port, "--js", "-e",
             "print(shell.status())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(CONNECT_TO_DOT_HOST_MESSAGE);
  }
  {
    execute({_mysqlsh, user, pwd, socket, "-h.", port, "--js", "-e",
             "print(shell.status())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(CONNECT_TO_DOT_HOST_MESSAGE);
  }
  {
    execute({_mysqlsh, user, pwd, socket, port, "-h.", "--js", "-e",
             "print(shell.status())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(CONNECT_TO_DOT_HOST_MESSAGE);
  }
#ifdef _WIN32
  // -h. is not supported in linux, just treat it as undefined behaviour
  {
    execute({_mysqlsh, user, pwd, "-h.", port, socket, "--js", "-e",
             "print(shell.status())", NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS("Named pipe: ");
  }
  {
    execute({_mysqlsh, user, pwd, port, "-h.", socket, "--js", "-e",
             "print(shell.status())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Named pipe: ");
  }
#endif
#undef CONNECT_TO_DOT_HOST_MESSAGE
#ifdef _WIN32
#define CONNECT_TO_DOT_HOST_MESSAGE "Named pipe: "
#else
#define CONNECT_TO_DOT_HOST_MESSAGE \
  "MySQL Error 2005: No such host is known '.'"
#endif
  {
    execute({_mysqlsh, user, pwd, port, socket, "-h.", "--js", "-e",
             "print(shell.status())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(CONNECT_TO_DOT_HOST_MESSAGE);
  }

  {
    execute({_mysqlsh, user, pwd, "-hlocalhost", socket, port, "--js", "-e",
             "print(shell.status())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("localhost via TCP/IP");
  }
  {
    execute({_mysqlsh, user, pwd, socket, "-hlocalhost", port, "--js", "-e",
             "print(shell.status())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("localhost via TCP/IP");
  }
  {
    execute({_mysqlsh, user, pwd, socket, port, "-hlocalhost", "--js", "-e",
             "print(shell.status())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("localhost via TCP/IP");
  }
#ifdef _WIN32
#define CONNECT_TO_SOCKET_MESSAGE "Named pipe: "
#else
#define CONNECT_TO_SOCKET_MESSAGE "Localhost via UNIX socket"
#endif
  {
    execute({_mysqlsh, user, pwd, "-hlocalhost", port, socket, "--js", "-e",
             "print(shell.status())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(CONNECT_TO_SOCKET_MESSAGE);
  }
  {
    execute({_mysqlsh, user, pwd, port, "-hlocalhost", socket, "--js", "-e",
             "print(shell.status())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(CONNECT_TO_SOCKET_MESSAGE);
  }
#ifndef _WIN32
  {
    execute({_mysqlsh, user, pwd, port, socket, "-hlocalhost", "--js", "-e",
             "print(shell.status())", NULL});
    MY_EXPECT_CMD_OUTPUT_CONTAINS(CONNECT_TO_SOCKET_MESSAGE);
  }
#endif

  session->execute("drop user testuser1@'%'");
}

TEST_F(Command_line_test, empty_socket) {
  // mysqlsh -uroot -S   was producing:
  // ERROR: Failed to retrieve the password: Invalid URL: Invalid address
  {
    execute({_mysqlsh, "-uroot", "-S", "--nw", "--js", "-e",
             "print(shell.status())", NULL});
    MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("ERROR");
  }
}

TEST_F(Command_line_test, default_is_xproto_via_tcp) {
  mysqlshdk::db::Connection_options options;
  options.set_host(_host);
  options.set_port(std::stoi(_mysql_port));
  options.set_user(_user);
  options.set_password(_pwd);

  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(options);
  session->execute("drop user if exists testuser1@'%'");
  session->execute("create user testuser1@'%' identified by 'pass1'");

  // connection to localhost with all defaults is xprotocol through TCP/IP
  {
    execute({_mysqlsh, "-utestuser1", "-ppass1", "--js", "-e",
             "print(shell.status())", NULL},
            nullptr, nullptr,
            {"MYSQL_UNIX_PORT=", "MYSQLX_UNIX_PORT=", "MYSQLX_SOCKET="});
    // either succeeds connecting to xport or fails because server is in a
    // different port
    MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF("localhost via TCP/IP",
                                         "Can't connect to MySQL server on");
    MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF("X protocol",
                                         "Can't connect to MySQL server on");
  }
  {
    execute({_mysqlsh, "-utestuser1", "-hlocalhost", "-ppass1", "--js", "-e",
             "print(shell.status())", NULL},
            nullptr, nullptr,
            {"MYSQL_UNIX_PORT=", "MYSQLX_UNIX_PORT=", "MYSQLX_SOCKET="});
    MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF("localhost via TCP/IP",
                                         "Can't connect to MySQL server on");
    MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF("X protocol",
                                         "Can't connect to MySQL server on");
  }

#ifndef _WIN32
  // -S forces socket connections - test should either succeed and connect via
  // socket or fail because of socket not found
  {
    execute({_mysqlsh, "-utestuser1", "-ppass1", "-S", "--js", "-e",
             "print(shell.status())", NULL},
            nullptr, nullptr,
            {"MYSQL_UNIX_PORT=", "MYSQLX_UNIX_PORT=", "MYSQLX_SOCKET="});
    MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF(
        "UNIX socket", "Can't connect to local MySQL server through socket");
    MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF(
        "X protocol", "Can't connect to local MySQL server through socket");
  }
  {
    execute({_mysqlsh, "-utestuser1", "-hlocalhost", "-ppass1", "-S", "--js",
             "-e", "print(shell.status())", NULL},
            nullptr, nullptr, {"MYSQL_UNIX_PORT=", "MYSQLX_SOCKET="});
    MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF(
        "UNIX socket", "Can't connect to local MySQL server through socket");
    MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF(
        "X protocol", "Can't connect to local MySQL server through socket");
  }
#endif
  session->execute("drop user if exists testuser1@'%'");
}

}  // namespace tests
