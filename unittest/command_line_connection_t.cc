/* Copyright (c) 2017, 2025, Oracle and/or its affiliates.

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

#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/test_utils/command_line_test.h"
#include "utils/utils_string.h"

extern mysqlshdk::utils::Version g_target_server_version;

namespace tests {

class Command_line_connection_test : public Command_line_test {
 protected:
  int execute_in_session(const std::string &uri,
                         const std::string &session_type,
                         const std::string &command = "",
                         const std::string &mode = "--sql") {
    std::string used_uri = uri.empty() ? _mysql_uri : uri;

    std::string pwd_param = "--password=" + _pwd;
    std::vector<const char *> args = {
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

  bool is_ssl_enabled() {
    // Default ssl-mode as required must work regardless if the server has or
    // not SSL enabled (i.e. commercial vs gpl)
    std::string query{"show variables like 'have_ssl'"};
    if (g_target_server_version >= mysqlshdk::utils::Version(8, 0, 21)) {
      query =
          "SELECT value FROM performance_schema.tls_channel_status WHERE "
          "channel='mysql_main' AND property = 'Enabled'";
    }

    // Default ssl-mode as required must work regardless if the server has or
    // not SSL enabled (i.e. commercial vs gpl)
    execute_in_session(_mysql_uri, "--mysql", query);

    return shcore::str_upper(_output).find("YES") != std::string::npos;
  }

  int test_classic_connection(const std::vector<const char *> &additional_args,
                              const char *password = NULL) {
    std::string pwd_param = "--password=" + _pwd;
    std::vector<const char *> args = {
        _mysqlsh,          "--mc", "--interactive=full",
        pwd_param.c_str(), "-e",   "\\status",
    };

    for (auto arg : additional_args) args.emplace_back(arg);

    args.push_back(NULL);

    return execute(args, password);
  }

  void test_classic_connection_attempt(
      bool expected, const std::vector<const char *> &cmdline_args) {
    test_classic_connection(cmdline_args);

    if (expected)
      MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic session to ");
    else
      MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("Creating a Classic session to ");
  }
};

TEST_F(Command_line_connection_test, classic_no_socket_no_port) {
  int ret_val = test_classic_connection({"-u", _user.c_str()});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic session to '" + _user +
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
    // Can't expect this case to NOT succeed, since there could be
    // an unrelated server running on 3306
    // MY_EXPECT_CMD_OUTPUT_CONTAINS(
    //    "Can't connect to MySQL server on 'localhost'");
  }
#else
  // test_main sets the variable that overrides the default socket
  // for classic protocol connections: MYSQL_UNIX_PORT so this connection
  // is expected to suceed
  EXPECT_EQ(0, ret_val);
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Localhost via UNIX socket");
#endif
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Classic 10");
}

TEST_F(Command_line_connection_test, classic_port) {
  test_classic_connection({"-u", _user.c_str(), "-P", _mysql_port.c_str()});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic session to '" + _user +
                                "@localhost:" + _mysql_port + "'");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Your MySQL connection id is");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("localhost via TCP/IP");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Classic 10");
}

TEST_F(Command_line_connection_test, bug25268670) {
  execute(
      {_mysqlsh, "--js", "-e",
       "shell.connect({'user':'root','password':'','host':'localhost','invalid_"
       "option':'wahtever'})",
       NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "Argument #1: Invalid values in connection options: "
      "invalid_option");
}

TEST_F(Command_line_connection_test, bug28899522) {
  static constexpr auto expected =
      "Argument #1: Host value cannot be an empty string.";

  execute({_mysqlsh, "--js", "-e", "shell.connect({'user':'root','host':''})",
           nullptr});

  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected);

  wipe_out();

  execute({_mysqlsh, "--js", "-e",
           "shell.connect({'user':'root','host':'','password':''})", nullptr});

  MY_EXPECT_CMD_OUTPUT_CONTAINS(expected);
}

// This example tests shows a case where the password will be prompted by the
// Shell. The password is provided appart as a second parameter after the
// list argument
TEST_F(Command_line_connection_test, prompt_sample) {
  execute({_mysqlsh, "--uri", _mysql_uri_nopasswd.c_str(), "--interactive=full",
           "--passwords-from-stdin", "-e", "session", NULL},
          _pwd.c_str());

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Please provide the password for '" +
                                _mysql_uri_nopasswd + "':");
}

TEST_F(Command_line_connection_test, no_pwd_save_from_stdin) {
  // if passwords are entered via stdin, then don't prompt if they should be
  // saved (the prompt would be unexpected and block the shell)
  // Bug #34501568	mysqlsh --passwords-from-stdin not working

  execute_in_session(
      _mysql_uri_nopasswd, "--mysql",
      "create user if not exists pwduser@'%' identified by 'verysecret'");

  wipe_out();

  shcore::unsetenv("MYSQLSH_CREDENTIAL_STORE_HELPER");
  shcore::unsetenv("MYSQLSH_CREDENTIAL_STORE_SAVE_PASSWORDS");

  std::string uri = "pwduser@localhost:" + _mysql_port;
  execute({_mysqlsh, uri.c_str(), "--passwords-from-stdin", "--sql", "-e",
           "select upper('hello')", NULL},
          "verysecret");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("HELLO");

  shcore::setenv("MYSQLSH_CREDENTIAL_STORE_HELPER", "<disabled>");

  execute_in_session(_mysql_uri_nopasswd, "--mysql", "drop user pwduser@'%'");
}

TEST_F(Command_line_connection_test, session_cmdline_options) {
  std::string port = "--port=" + _port;
  std::string mysql_port = "--port=" + _mysql_port;
  std::string uri_db = _uri + "/mysql";
  std::string mysql_uri_db = _mysql_uri + "/mysql";
  std::string uri_scheme_db = "mysql://" + _uri + "/mysql";
  std::string mysql_uri_xscheme_db = "mysqlx://" + _mysql_uri + "/mysql";
  std::string uri_xscheme_db = "mysqlx://" + _uri + "/mysql";
  std::string mysql_uri_scheme_db = "mysql://" + _mysql_uri + "/mysql";

  // FR2_5 : mysqlsh -u <user> -p --port=<mysql_port>
  execute(
      {_mysqlsh, "-u", _user.c_str(), "-p", mysql_port.c_str(),
       "--interactive=full", "--passwords-from-stdin", "-e", "\\status", NULL},
      _pwd.c_str());

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a session to '");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Protocol version:             Classic 10");

  // FR2_7 : mysqlsh -u <user> -p --port=<mysqlx_port>
  execute(
      {_mysqlsh, "-u", _user.c_str(), "-p", port.c_str(), "--interactive=full",
       "--passwords-from-stdin", "-e", "\\status", NULL},
      _pwd.c_str());

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a session to '");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Protocol version:             X protocol");

  // FR2_10 : mysqlsh --uri user@host:33060/db --mysqlx
  execute({_mysqlsh, "--uri", uri_db.c_str(), "--mysqlx", "--interactive=full",
           "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session to '");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Protocol version:             X protocol");

  // FR_EXTRA_11 : mysqlsh --uri mysqlx://user@host:3306/db --mysqlx
  execute({_mysqlsh, "--uri", mysql_uri_xscheme_db.c_str(), "--mysqlx",
           "--interactive=full", "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session to ");
  MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF(
      "MySQL server has gone away",
      "Requested session assumes MySQL X Protocol but '" + _host + ":" +
          _mysql_port + "' seems to speak the classic MySQL protocol");

  // FR_EXTRA_12 : mysqlsh --uri mysqlx://user@host:3306/db --mx
  execute({_mysqlsh, "--uri", mysql_uri_xscheme_db.c_str(), "--mx",
           "--interactive=full", "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session to ");
  MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF(
      "MySQL server has gone away",
      "Requested session assumes MySQL X Protocol but '" + _host + ":" +
          _mysql_port + "' seems to speak the classic MySQL protocol");

  // FR_EXTRA_16 : mysqlsh --uri mysqlx://user@host:33060/db --mx
  execute({_mysqlsh, "--uri", uri_xscheme_db.c_str(), "--mx",
           "--interactive=full", "-e", "\\status", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session to ");

  // FR_EXTRA_19 : mysqlsh --uri user@host:3306/db --mysqlx
  execute({_mysqlsh, "--uri", mysql_uri_db.c_str(), "--mysqlx",
           "--interactive=full", "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF(
      "MySQL server has gone away",
      "Requested session assumes MySQL X Protocol but '" + _host + ":" +
          _mysql_port + "' seems to speak the classic MySQL protocol");

  // FR_EXTRA_20 : mysqlsh --uri user@host:3306/db --mx
  execute({_mysqlsh, "--uri", mysql_uri_db.c_str(), "--mx",
           "--interactive=full", "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF(
      "MySQL server has gone away",
      "Requested session assumes MySQL X Protocol but '" + _host + ":" +
          _mysql_port + "' seems to speak the classic MySQL protocol");

  // FR_EXTRA_SUCCEED_1 : mysqlsh --uri mysql://user@host:3306/db
  execute({_mysqlsh, "--uri", mysql_uri_scheme_db.c_str(), "--interactive=full",
           "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic session to ");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Protocol version:             Classic 10");

  // FR_EXTRA_SUCCEED_2 : mysqlsh --uri mysql://user@host:3306/db --mc
  execute({_mysqlsh, mysql_uri_db.c_str(), "--mc", "--interactive=full", "-e",
           "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic session to ");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Protocol version:             Classic 10");

  // FR_EXTRA_SUCCEED_3 : mysqlsh --uri mysqlx://user@host:33060/db
  execute({_mysqlsh, uri_xscheme_db.c_str(), "--interactive=full", "-e",
           "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session to ");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Protocol version:             X protocol");

  // FR_EXTRA_SUCCEED_4 : mysqlsh --uri mysqlx://user@host:33060/db --mx
  execute({_mysqlsh, uri_xscheme_db.c_str(), "--mx", "--interactive=full", "-e",
           "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session to ");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Protocol version:             X protocol");
}

TEST_F(Command_line_connection_test, uri_ssl_mode_classic) {
  if (is_ssl_enabled()) {
    _output.clear();

    // Having SSL enabled sets secure_transport_required=ON
    bool require_secure_transport = false;
    execute_in_session(_mysql_uri, "--mysql",
                       "show variables like 'require_secure_transport';");
    if (_output.find("ON") != std::string::npos)
      require_secure_transport = true;

    _output.clear();

    if (!require_secure_transport) {
      execute_in_session(_mysql_uri, "--mysql",
                         "set global require_secure_transport=ON;");
      _output.clear();
    }

    // Tests the ssl-mode=DISABLED to make sure it is not
    // ignored when coming in a URI
    std::string ssl_uri = _mysql_uri + "?ssl-mode=DISABLED";

    execute_in_session(ssl_uri, "--mysql");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic session to");
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "MySQL Error 3159 (HY000): Connections using "
        "insecure transport are prohibited while "
        "--require_secure_transport=ON.");
    _output.clear();

    if (!require_secure_transport)
      execute_in_session(_mysql_uri, "--mysql",
                         "set global require_secure_transport=OFF;");
  } else {
    // Having SSL disabled test the ssl-mode=REQUIRED to make sure
    // it is not ignored when coming in a URI

    std::string ssl_uri = _mysql_uri + "?ssl-mode=REQUIRED";

    execute_in_session(ssl_uri, "--mysql");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic session to");
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "MySQL Error 2026: SSL connection error: SSL is required "
        "but the server doesn't support it");
    _output.clear();
  }
}

TEST_F(Command_line_connection_test, uri_ssl_mode_node) {
  if (is_ssl_enabled()) {
    _output.clear();
    // Having SSL enabled sets secure_transport_required=ON
    bool require_secure_transport = false;
    execute_in_session(_mysql_uri, "--mysql",
                       "show variables like 'require_secure_transport';");
    if (_output.find("ON") != std::string::npos)
      require_secure_transport = true;

    _output.clear();

    if (!require_secure_transport) {
      execute_in_session(_mysql_uri, "--mysql",
                         "set global require_secure_transport=ON;");
      _output.clear();
    }

    // Tests the ssl-mode=DISABLED to make sure it is not
    // ignored when coming in a URI
    std::string ssl_uri = _uri + "?ssl-mode=DISABLED";

    execute_in_session(ssl_uri, "--mysqlx");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session to");
    if (g_target_server_version >= mysqlshdk::utils::Version(8, 0, 4)) {
      MY_EXPECT_CMD_OUTPUT_CONTAINS(
          "Connections using insecure transport are "
          "prohibited while --require_secure_transport=ON");
    } else {
      MY_EXPECT_CMD_OUTPUT_CONTAINS(
          "MySQL Error 1045: Secure transport required. To log in you must use "
          "TCP+SSL or UNIX socket connection.");
    }
    _output.clear();

    if (!require_secure_transport)
      execute_in_session(_mysql_uri, "--mysql",
                         "set global require_secure_transport=OFF;");
  } else {
    // Having SSL disabled test the ssl-mode=REQUIRED to make sure
    // it is not ignored when coming in a URI

    std::string ssl_uri = _uri + "?ssl-mode=REQUIRED";

    execute_in_session(ssl_uri, "--mysqlx");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session to");
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "MySQL Error 5001: Capability prepare failed for 'tls'");
    _output.clear();
  }
}

TEST_F(Command_line_connection_test, basic_ssl_check_x) {
  const char *ssl_check =
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
  const char *ssl_check =
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

TEST_F(Command_line_connection_test, expired_account) {
  _output.clear();
  execute({_mysqlsh, _mysql_uri.c_str(), "--sql", "-e",
           "drop user if exists expired@localhost; "
           "create user expired@localhost password expire;",
           nullptr},
          nullptr, nullptr, {"MYSQLSH_TERM_COLOR_MODE=nocolor"});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "WARNING: Using a password on the command line interface can be "
      "insecure.");

  std::string uri;

  uri = "expired:@" + shcore::str_partition(_mysql_uri, "@").second;
  _output.clear();
  execute({_mysqlsh, uri.c_str(), "--interactive=full", "--js", "-e",
           "print('DONE')", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("DONE");
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("ERROR");
}

TEST_F(Command_line_connection_test, invalid_options_WL10912) {
  const std::string prefix =
      "Argument #1: Invalid values in connection options: ";
  {
    execute({_mysqlsh, "--js", "-e",
             "shell.connect({'user':'root','password':'','host':'localhost','"
             "sslMode':"
             "'whatever'})",
             NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(prefix + "sslMode");
  }  // namespace tests

  {
    execute({_mysqlsh, "--js", "-e",
             "shell.connect({'user':'root','password':'','host':'localhost','"
             "sslCa':'"
             "whatever'})",
             NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(prefix + "sslCa");
  }

  {
    execute({_mysqlsh, "--js", "-e",
             "shell.connect({'user':'root','password':'','host':'localhost','"
             "sslCaPath':'"
             "whatever'})",
             NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(prefix + "sslCaPath");
  }

  {
    execute({_mysqlsh, "--js", "-e",
             "shell.connect({'user':'root','password':'','host':'localhost','"
             "sslCrl':'"
             "whatever'})",
             NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(prefix + "sslCrl");
  }

  {
    execute({_mysqlsh, "--js", "-e",
             "shell.connect({'user':'root','password':'','host':'localhost','"
             "sslCrlPath':"
             "'whatever'})",
             NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(prefix + "sslCrlPath");
  }

  {
    execute({_mysqlsh, "--js", "-e",
             "shell.connect({'user':'root','password':'','host':'localhost','"
             "sslCert':'"
             "whatever'})",
             NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(prefix + "sslCert");
  }

  {
    execute({_mysqlsh, "--js", "-e",
             "shell.connect({'user':'root','password':'','host':'localhost','"
             "sslKey':'"
             "whatever'})",
             NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(prefix + "sslKey");
  }

  {
    execute({_mysqlsh, "--js", "-e",
             "shell.connect({'user':'root','password':'','host':'localhost','"
             "sslCipher':"
             "'whatever'})",
             NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(prefix + "sslCipher");
  }

  {
    execute({_mysqlsh, "--js", "-e",
             "shell.connect({'user':'root','password':'','host':'localhost',"
             "'sslTlsVersion':'whatever'})",
             NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(prefix + "sslTlsVersion");
  }
}

#ifndef _WIN32
TEST_F(Command_line_connection_test, socket_connection) {
  {
    std::string cmd = "shell.connect(\"" + _user + ":" + _pwd + "@(" +
                      _mysql_socket + ")\");shell.status()";
    execute({_mysqlsh, "--js", "-e", cmd.c_str(), nullptr});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Connection:                   Localhost via UNIX socket");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Unix socket:");
  }

  {
    std::string cmd = "shell.connect(\"" + _user + ":" + _pwd + "@(" + _socket +
                      ")\");shell.status()";
    execute({_mysqlsh, "--js", "-e", cmd.c_str(), nullptr});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Connection:                   Localhost via UNIX socket");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Unix socket:");
  }
}

TEST_F(Command_line_connection_test, socket_connection_with_default_path) {
  if (g_test_recording_mode != mysqlshdk::db::replay::Mode::Direct) {
    SKIP_TEST("Skipping tests for default socket paths");
  }

  // This test connects to the default socket path, which may not be available
  // at certain test runtime environments
  std::string cmd = "shell.status()";
  std::string pwd = "--password=" + _pwd;
  {
    execute({_mysqlsh, "-u", _user.c_str(), pwd.c_str(), "--socket", "--js",
             "-e", cmd.c_str(), nullptr});

    if (_output.find("Can't connect to local MySQL server through socket") ==
        std::string::npos) {
      MY_EXPECT_CMD_OUTPUT_CONTAINS(
          "Connection:                   Localhost via UNIX socket");
      MY_EXPECT_CMD_OUTPUT_CONTAINS("Unix socket:");
    }
  }
  {
    execute({_mysqlsh, "-u", _user.c_str(), pwd.c_str(), "-S", "--mc", "--js",
             "-e", cmd.c_str(), nullptr});

    if (_output.find("Can't connect to local MySQL server through socket") ==
        std::string::npos) {
      MY_EXPECT_CMD_OUTPUT_CONTAINS(
          "Connection:                   Localhost via UNIX socket");
      MY_EXPECT_CMD_OUTPUT_CONTAINS("Unix socket:");
    }
  }
}
#endif  // !_WIN32

TEST_F(Command_line_connection_test, compression) {
  // X protocol
  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 19)) {
    std::string cmd_x =
        "select case when variable_value > 0 then 'COMPRESSION_ON' else 'OFF' "
        "end from performance_schema.session_status where variable_name = "
        "'Mysqlx_bytes_sent_compressed_payload';";
    execute({_mysqlsh, _uri.c_str(), "--compress", "--sql", "-e", cmd_x.c_str(),
             nullptr});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("COMPRESSION_ON");

    execute({_mysqlsh, _uri.c_str(), "--compression-algorithms=lz4", "--sql",
             "-e", cmd_x.c_str(), nullptr});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("COMPRESSION_ON");
  }
  wipe_out();

  // classic protocol
  std::string cmd = "show session status like 'compression%';";
  std::string com_uri = _mysql_uri + "?compression=true";
  execute({_mysqlsh, _mysql_uri.c_str(), "--compress", "--sql", "-e",
           cmd.c_str(), nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Compression\tON");

  execute({_mysqlsh, com_uri.c_str(), "--sql", "-e", cmd.c_str(), nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Compression\tON");
  wipe_out();

  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 18)) {
    execute({_mysqlsh, _mysql_uri.c_str(), "--compression-algorithms=zstd",
             "--compression-level=12", "--sql", "-e", cmd.c_str(), nullptr});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Compression\tON");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Compression_algorithm\tzstd");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Compression_level\t12");
    wipe_out();

    execute({_mysqlsh, _mysql_uri.c_str(), "--zstd-compression-level=12",
             "--sql", "-e", cmd.c_str(), nullptr});
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Compression\tON");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Compression_algorithm\tzstd");
    MY_EXPECT_CMD_OUTPUT_CONTAINS("Compression_level\t12");
    wipe_out();
  }
}

TEST_F(Command_line_connection_test, auth_method) {
  const auto protocol_mismatch_error =
      _target_server_version >= mysqlshdk::utils::Version("8.0.0")
          ? "MySQL Error 2007 (HY000): Protocol mismatch; server version = 11, "
            "client version = 10"
          : "MySQL Error 2013 (HY000): Lost connection to MySQL server at "
            "'waiting for initial communication packet', system error: ";

  // auto: X port - invalid authentication method
  execute({_mysqlsh, _uri.c_str(), "--interactive=full",
           "--auth-method=invalid", "-e", "\\status", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a session to");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "Failed to set the authentication method: Invalid value for option");

  // auto: X port - valid authentication method
  execute({_mysqlsh, _uri.c_str(), "--interactive=full", "--auth-method=PLAIN",
           "-e", "\\status", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a session to");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Protocol version:             X protocol");

  // auto: X port - valid authentication method - lowercase
  execute({_mysqlsh, _uri.c_str(), "--interactive=full", "--auth-method=plain",
           "-e", "\\status", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a session to");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Protocol version:             X protocol");

  // auto: classic port - invalid authentication method ignored in favor of the
  // authentication method sent by the server
  execute({_mysqlsh, _mysql_uri.c_str(), "--interactive=full",
           "--auth-method=invalid", "-e", "\\status", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a session to");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Protocol version:             Classic 10");

  // auto: classic port - invalid (but valid X protocol) authentication method
  // ignored in favor of the authentication plugin name sent by the server
  execute({_mysqlsh, _mysql_uri.c_str(), "--interactive=full",
           "--auth-method=PLAIN", "-e", "\\status", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a session to");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Protocol version:             Classic 10");

  // auto: classic port - valid authentication method
  execute({_mysqlsh, _mysql_uri.c_str(), "--interactive=full",
           "--auth-method=mysql_native_password", "-e", "\\status", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a session to");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Protocol version:             Classic 10");

  // mysqlx: X port - invalid authentication method
  execute({_mysqlsh, _uri.c_str(), "--interactive=full",
           "--auth-method=invalid", "--mysqlx", "-e", "\\status", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "MySQL Error 2505: Failed to set the authentication method: Invalid "
      "value for option");

  // mysqlx: X port - valid authentication method
  execute({_mysqlsh, _uri.c_str(), "--interactive=full", "--auth-method=PLAIN",
           "--mysqlx", "-e", "\\status", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Protocol version:             X protocol");

  // mysqlx: X port - valid authentication method - lowercase
  execute({_mysqlsh, _uri.c_str(), "--interactive=full", "--auth-method=plain",
           "--mysqlx", "-e", "\\status", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Protocol version:             X protocol");

  // mysqlx: classic port - invalid authentication method
  execute({_mysqlsh, _mysql_uri.c_str(), "--interactive=full",
           "--auth-method=invalid", "--mysqlx", "-e", "\\status", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "MySQL Error 2505: Failed to set the authentication method: Invalid "
      "value for option");

  // mysqlx: classic port - valid authentication method
  execute({_mysqlsh, _mysql_uri.c_str(), "--interactive=full",
           "--auth-method=PLAIN", "--mysqlx", "-e", "\\status", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "MySQL Error 2027: Requested session assumes MySQL X Protocol but");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "seems to speak the classic MySQL protocol (Unexpected response received "
      "from server, msg-id:");

  // mysql: classic port - invalid authentication method is ignored in favor of
  // the authentication plugin sent by the server
  execute({_mysqlsh, _mysql_uri.c_str(), "--interactive=full",
           "--auth-method=invalid", "--mysql", "-e", "\\status", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic session");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Protocol version:             Classic 10");

  // mysql: classic port - valid authentication method
  execute({_mysqlsh, _mysql_uri.c_str(), "--interactive=full",
           "--auth-method=mysql_native_password", "--mysql", "-e", "\\status",
           NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic session");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Protocol version:             Classic 10");

  // mysql: X port - invalid authentication method
  execute({_mysqlsh, _uri.c_str(), "--interactive=full",
           "--auth-method=invalid", "--mysql", "-e", "\\status", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic session");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(protocol_mismatch_error);
}

TEST_F(Command_line_connection_test, invalid_port) {
  execute({_mysqlsh, _uri.c_str(), "--port=invalidPort1", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Invalid value for --port: invalidPort1");

  execute({_mysqlsh, _uri.c_str(), "--port", "invalidPort2", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Invalid value for --port: invalidPort2");

  execute({_mysqlsh, _uri.c_str(), "-PinvalidPort3", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Invalid value for -P: invalidPort3");
}

TEST_F(Command_line_connection_test, kerberos_auth_mode) {
  execute({_mysqlsh, _uri.c_str(),
           "--plugin-authentication-kerberos-client-mode=InvalidMode", "--port",
           "invalidPort", NULL});
#ifdef _WIN32
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "Invalid value: InvalidMode. Allowed values: SSPI, GSSAPI.");

  execute({_mysqlsh, _uri.c_str(),
           "--plugin-authentication-kerberos-client-mode=GsSApi", "--port",
           "invalidPort", NULL});

  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("GsSApi");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Invalid value for --port: invalidPort");

#else
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "unknown option --plugin-authentication-kerberos-client-mod");
#endif
}

TEST_F(Command_line_connection_test, local_infile) {
  // X protocol is not supported
  execute({_mysqlsh, _uri.c_str(), "--local-infile", "--interactive=full",
           nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a session to '" + _uri_nopasswd +
                                "?local-infile=true'");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "X Protocol: LOAD DATA LOCAL INFILE is not supported.");

  // classic protocol is supported
  execute({_mysqlsh, _mysql_uri.c_str(), "--local-infile", "--interactive=full",
           "-e", "\\status", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a session to '" +
                                _mysql_uri_nopasswd + "?local-infile=true'");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Connection Id:");

  // last option wins, --local-infile is last
  execute({_mysqlsh, (_mysql_uri + "?local-infile=0").c_str(), "--local-infile",
           "--interactive=full", "-e", "\\status", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a session to '" +
                                _mysql_uri_nopasswd + "?local-infile=true'");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Connection Id:");

  // last option wins, URI is last
  execute({_mysqlsh, "--local-infile", (_mysql_uri + "?local-infile=0").c_str(),
           "--interactive=full", "-e", "\\status", nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a session to '" +
                                _mysql_uri_nopasswd + "?local-infile=0'");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Connection Id:");
}

}  // namespace tests
