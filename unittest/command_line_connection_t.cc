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

  int test_classic_connection(const std::vector<const char *> &additional_args,
                              const char *password = NULL) {
    std::string pwd_param = "--password=" + _pwd;
    std::vector<const char *> args = {
        _mysqlsh,          "-mc", "--interactive=full",
        pwd_param.c_str(), "-e",  "\\status",
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

 private:
  std::string pwd_param;
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
}

TEST_F(Command_line_connection_test, classic_port) {
  test_classic_connection({"-u", _user.c_str(), "-P", _mysql_port.c_str()});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic session to '" + _user +
                                "@localhost:" + _mysql_port + "'");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Your MySQL connection id is");
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("localhost via TCP/IP");
}

TEST_F(Command_line_connection_test, bug25268670) {
  execute({_mysqlsh, "-e",
           "shell.connect({user:'root',password:'',host:'localhost',invalid_"
           "option:'wahtever'})",
           NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "Shell.connect: Invalid values in connection options: invalid_option");
}

// This example tests shows a case where the password will be prompted by the
// Shell. The password is provided appart as a second parameter after the
// list argument
TEST_F(Command_line_connection_test, prompt_sample) {
  execute({_mysqlsh, "--uri", _mysql_uri_nopasswd.c_str(), "--interactive=full",
           "--passwords-from-stdin", "-e", "session", NULL},
          _pwd.c_str());

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Enter password:");
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

  // FR2_5 : mysqlsh -u <user> -p --port=<mysql_port> -ma
  execute(
      {_mysqlsh, "-u", _user.c_str(), "-p", mysql_port.c_str(), "-ma",
       "--interactive=full", "--passwords-from-stdin", "-e", "\\status", NULL},
      _pwd.c_str());

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a session to '");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Session type:                 Classic");

  // FR2_7 : mysqlsh -u <user> -p --port=<mysqlx_port> -ma
  execute(
      {_mysqlsh, "-u", _user.c_str(), "-p", port.c_str(), "-ma",
       "--interactive=full", "--passwords-from-stdin", "-e", "\\status", NULL},
      _pwd.c_str());

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a session to '");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Session type:                 X");

  // FR2_10 : mysqlsh --uri user@host:33060/db --mysqlx
  execute({_mysqlsh, "--uri", uri_db.c_str(), "--mysqlx", "--interactive=full",
           "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session to '");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Session type:                 X");

  // FR_EXTRA_3 : mysqlsh --uri mysql://user@host:33060/db --mysqlx
  execute({_mysqlsh, "--uri", uri_scheme_db.c_str(), "--mysqlx",
           "--interactive=full", "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "The given URI conflicts with the --mysqlx session type option.");

  // FR_EXTRA_4 : mysqlsh --uri mysql://user@host:33060/db -mx
  execute({_mysqlsh, "--uri", uri_scheme_db.c_str(), "-mx",
           "--interactive=full", "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "The given URI conflicts with the -mx session type option.");

  // FR_EXTRA_5 : mysqlsh --uri mysql://user@host:3306/db --mysqlx
  execute({_mysqlsh, "--uri", uri_scheme_db.c_str(), "--mysqlx",
           "--interactive=full", "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "The given URI conflicts with the --mysqlx session type option.");

  // FR_EXTRA_6 : mysqlsh --uri mysql://user@host:3306/db -mx
  execute({_mysqlsh, "--uri", uri_scheme_db.c_str(), "-mx",
           "--interactive=full", "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "The given URI conflicts with the -mx session type option.");

  // FR_EXTRA_11 : mysqlsh --uri mysqlx://user@host:3306/db --mysqlx
  execute({_mysqlsh, "--uri", mysql_uri_xscheme_db.c_str(), "--mysqlx",
           "--interactive=full", "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session to ");
  MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF(std::vector<std::string>(
      {"MySQL server has gone away",
       "Requested session assumes MySQL X Protocol but '" + _host + ":" +
           _mysql_port + "' seems to speak the classic MySQL protocol"}));

  // FR_EXTRA_12 : mysqlsh --uri mysqlx://user@host:3306/db -mx
  execute({_mysqlsh, "--uri", mysql_uri_xscheme_db.c_str(), "-mx",
           "--interactive=full", "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session to ");
  MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF(std::vector<std::string>(
      {"MySQL server has gone away",
       "Requested session assumes MySQL X Protocol but '" + _host + ":" +
           _mysql_port + "' seems to speak the classic MySQL protocol"}));

  // FR_EXTRA_15 : mysqlsh --uri mysqlx://user@host:33060/db --mysqlx -ma
  execute({_mysqlsh, "--uri", uri_xscheme_db.c_str(), "--mysqlx", "-ma",
           "--interactive=full", "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "Session type already configured to X protocol, automatic protocol "
      "detection (-ma) can't be enabled.");

  // FR_EXTRA_16 : mysqlsh --uri mysqlx://user@host:33060/db -mx -ma
  execute({_mysqlsh, "--uri", uri_xscheme_db.c_str(), "-mx", "-ma",
           "--interactive=full", "-e", "\\status", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "Session type already configured to X protocol, automatic protocol "
      "detection (-ma) can't be enabled.");

  // FR_EXTRA_19 : mysqlsh --uri user@host:3306/db --mysqlx
  execute({_mysqlsh, "--uri", mysql_uri_db.c_str(), "--mysqlx",
           "--interactive=full", "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF(std::vector<std::string>(
      {"MySQL server has gone away",
       "Requested session assumes MySQL X Protocol but '" + _host + ":" +
           _mysql_port + "' seems to speak the classic MySQL protocol"}));

  // FR_EXTRA_20 : mysqlsh --uri user@host:3306/db -mx
  execute({_mysqlsh, "--uri", mysql_uri_db.c_str(), "-mx", "--interactive=full",
           "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF(std::vector<std::string>(
      {"MySQL server has gone away",
       "Requested session assumes MySQL X Protocol but '" + _host + ":" +
           _mysql_port + "' seems to speak the classic MySQL protocol"}));

  // FR_EXTRA_SUCCEED_1 : mysqlsh --uri mysql://user@host:3306/db -ma
  execute({_mysqlsh, "--uri", mysql_uri_scheme_db.c_str(), "-ma",
           "--interactive=full", "-e", "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic session to ");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Session type:                 Classic");

  // FR_EXTRA_SUCCEED_2 : mysqlsh --uri mysql://user@host:3306/db -mc
  execute({_mysqlsh, mysql_uri_db.c_str(), "-mc", "--interactive=full", "-e",
           "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating a Classic session to ");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Session type:                 Classic");

  // FR_EXTRA_SUCCEED_3 : mysqlsh --uri mysqlx://user@host:33060/db -ma
  execute({_mysqlsh, uri_xscheme_db.c_str(), "-ma", "--interactive=full", "-e",
           "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session to ");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Session type:                 X");

  // FR_EXTRA_SUCCEED_4 : mysqlsh --uri mysqlx://user@host:33060/db -mx
  execute({_mysqlsh, uri_xscheme_db.c_str(), "-mx", "--interactive=full", "-e",
           "\\status", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("Creating an X protocol session to ");
  MY_EXPECT_CMD_OUTPUT_CONTAINS("Session type:                 X");
}

TEST_F(Command_line_connection_test, uri_ssl_mode_classic) {
  bool have_ssl = false;

  // Default ssl-mode as required must work regardless if the server has or not
  // SSL enabled (i.e. commercial vs gpl)
  execute_in_session(_mysql_uri, "--mysql", "show variables like 'have_ssl';");
  if (_output.find("YES") != std::string::npos) have_ssl = true;

  _output.clear();

  if (have_ssl) {
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
        "MySQL Error 2026 (HY000): SSL connection error: SSL is required "
        "but the server doesn't support it");
    _output.clear();
  }
}

TEST_F(Command_line_connection_test, uri_ssl_mode_node) {
  bool have_ssl = false;

  // Default ssl-mode as required must work regardless if the server has or not
  // SSL enabled (i.e. commercial vs gpl)
  execute_in_session(_mysql_uri, "--mysql", "show variables like 'have_ssl';");
  if (_output.find("YES") != std::string::npos) have_ssl = true;

  _output.clear();

  if (have_ssl) {
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
           nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "mysqlsh: [Warning] Using a password on the command line interface can "
      "be insecure.");

  std::string uri;

  uri = "expired:@" + shcore::str_partition(_mysql_uri, "@").second;
  _output.clear();
  execute({_mysqlsh, uri.c_str(), "--interactive=full", "-e", "print('DONE')",
           nullptr});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("DONE");
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("ERROR");
}

TEST_F(Command_line_connection_test, invalid_options_WL10912) {
  {
    execute({_mysqlsh, "-e",
             "shell.connect({user:'root',password:'',host:'localhost',sslMode:"
             "'whatever'})",
             NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Shell.connect: Invalid values in connection options: sslMode");
  }  // namespace tests

  {
    execute({_mysqlsh, "-e",
             "shell.connect({user:'root',password:'',host:'localhost',sslCa:'"
             "whatever'})",
             NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Shell.connect: Invalid values in connection options: sslCa");
  }

  {
    execute(
        {_mysqlsh, "-e",
         "shell.connect({user:'root',password:'',host:'localhost',sslCaPath:'"
         "whatever'})",
         NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Shell.connect: Invalid values in connection options: sslCaPath");
  }

  {
    execute({_mysqlsh, "-e",
             "shell.connect({user:'root',password:'',host:'localhost',sslCrl:'"
             "whatever'})",
             NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Shell.connect: Invalid values in connection options: sslCrl");
  }

  {
    execute(
        {_mysqlsh, "-e",
         "shell.connect({user:'root',password:'',host:'localhost',sslCrlPath:"
         "'whatever'})",
         NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Shell.connect: Invalid values in connection options: sslCrlPath");
  }

  {
    execute({_mysqlsh, "-e",
             "shell.connect({user:'root',password:'',host:'localhost',sslCert:'"
             "whatever'})",
             NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Shell.connect: Invalid values in connection options: sslCert");
  }

  {
    execute({_mysqlsh, "-e",
             "shell.connect({user:'root',password:'',host:'localhost',sslKey:'"
             "whatever'})",
             NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Shell.connect: Invalid values in connection options: sslKey");
  }

  {
    execute(
        {_mysqlsh, "-e",
         "shell.connect({user:'root',password:'',host:'localhost',sslCipher:"
         "'whatever'})",
         NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Shell.connect: Invalid values in connection options: sslCipher");
  }

  {
    execute({_mysqlsh, "-e",
             "shell.connect({user:'root',password:'',host:'localhost',"
             "sslTlsVersion:'whatever'})",
             NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Shell.connect: Invalid values in connection options: sslTlsVersion");
  }
}

}  // namespace tests
