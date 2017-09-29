/* Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.

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

#include <gtest/gtest_prod.h>
#include <cstdio>
#include "src/mysqlsh/cmdline_shell.h"
#include "unittest/test_utils.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"

namespace mysqlsh {
class Interactive_shell_test : public Shell_core_test_wrapper {
 public:
  virtual void set_options() {
    _options->interactive = true;
    _options->wizards = true;
  }

  static void SetUpTestCase() {
    run_script_classic({"drop user if exists expired@localhost"});
  }
};

TEST_F(Interactive_shell_test, shell_command_connect_node) {
  execute("\\connect -mx " + _uri);
  MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to '" +
                            _uri_nopasswd + "'");
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  MY_EXPECT_STDOUT_CONTAINS("(X protocol)");
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<Session:mysqlx://" + _uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("", output_handler.std_out.c_str());
  EXPECT_STREQ("", output_handler.std_err.c_str());

  execute("session.close()");

  execute("\\connect -mx " + _uri + "/mysql");
  MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to '" +
                            _uri_nopasswd + "/mysql'");
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  MY_EXPECT_STDOUT_CONTAINS("(X protocol)");
  MY_EXPECT_STDOUT_CONTAINS("Default schema `mysql` accessible through db.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<Session:mysqlx://" + _uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  MY_EXPECT_STDOUT_CONTAINS("<Schema:mysql>");
  output_handler.wipe_all();

  execute("session.close()");

  execute("\\connect -mx mysql://" + _mysql_uri);
  MY_EXPECT_STDERR_CONTAINS(
      "The given URI conflicts with the --mysqlx session type option.");
  output_handler.wipe_all();

  execute("\\connect -mx " + _mysql_uri);
  // wrong protocol can manifest as this error or a 2006 'gone away' error
  if (output_handler.std_err.find("MySQL Error 2006") == std::string::npos)
    MY_EXPECT_STDERR_CONTAINS(
        "Requested session assumes MySQL X Protocol but '" + _host + ":" +
        _mysql_port + "' seems to speak the classic MySQL protocol");
  output_handler.wipe_all();

  // Invalid user/password
  output_handler.passwords.push_back("whatever");
  execute("\\connect -mx " + _uri_nopasswd);
  MY_EXPECT_STDERR_CONTAINS("MySQL Error 1045: Invalid user or password");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_connect_classic) {
  execute("\\connect -mc " + _mysql_uri);
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic session to '" +
                            _mysql_uri_nopasswd + "'");
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:mysql://" + _mysql_uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("", output_handler.std_out.c_str());
  EXPECT_STREQ("", output_handler.std_err.c_str());

  execute("session.close()");

  execute("\\connect -mc " + _mysql_uri + "/mysql");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic session to '" +
                            _mysql_uri_nopasswd + "/mysql'");
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:mysql://" + _mysql_uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSchema:mysql>");
  output_handler.wipe_all();

  execute("session.close()");

  execute("\\connect -mc mysqlx://" + _uri);
  MY_EXPECT_STDERR_CONTAINS(
      "The given URI conflicts with the --mysql session type option.");
  output_handler.wipe_all();

  // FR_EXTRA_SUCCEED_7 : \connect -mc mysql://user@host:3306/db
  {
    execute("\\connect -mc mysql://" + _mysql_uri + "/mysql");
    MY_EXPECT_STDOUT_CONTAINS("Creating a Classic session to '" +
                              _mysql_uri_nopasswd + "/mysql'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:mysql://" + _mysql_uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // FR_EXTRA_SUCCEED_8 : \c -mc mysql://user@host:3306/db
  {
    execute("\\c -mc mysql://" + _mysql_uri + "/mysql");
    MY_EXPECT_STDOUT_CONTAINS("Creating a Classic session to '" +
                              _mysql_uri_nopasswd + "/mysql'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:mysql://" + _mysql_uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }
}

TEST_F(Interactive_shell_test, shell_command_connect_x) {
  // FR_EXTRA_SUCCEED_11 : \connect -mx mysqlx://user@host:33060/db
  {
    execute("\\connect -mx mysqlx://" + _uri + "/mysql");
    MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to '" +
                              _uri_nopasswd + "/mysql'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS("Default schema `mysql` accessible through db.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<Session:mysqlx://" + _uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // FR_EXTRA_SUCCEED_12 : \c -mx mysqlx://user@host:33060/db
  {
    execute("\\c -mx mysqlx://" + _uri + "/mysql");
    MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to '" +
                              _uri_nopasswd + "/mysql'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS("Default schema `mysql` accessible through db.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<Session:mysqlx://" + _uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }
}

TEST_F(Interactive_shell_test, shell_command_connect_auto) {
  // Session type determined from connection success
  {
    execute("\\connect " + _uri);
    MY_EXPECT_STDOUT_CONTAINS("Creating a session to '" + _uri_nopasswd + "'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS(
        "No default schema selected; type \\use <schema> to set one.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<Session:" + _uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // Session type determined from connection success
  {
    execute("\\connect " + _mysql_uri);
    MY_EXPECT_STDOUT_CONTAINS("Creating a session to '" + _mysql_uri_nopasswd +
                              "'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS(
        "No default schema selected; type \\use <schema> to set one.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // Session type determined by the URI scheme
  {
    execute("\\connect mysql://" + _mysql_uri);
    MY_EXPECT_STDOUT_CONTAINS("Creating a Classic session to '" +
                              _mysql_uri_nopasswd + "'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS(
        "No default schema selected; type \\use <schema> to set one.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:mysql://" + _mysql_uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // FR_EXTRA_SUCCEED_5 : \connect -ma mysql://user@host:3306/db
  {
    execute("\\connect -ma mysql://" + _mysql_uri + "/mysql");
    MY_EXPECT_STDOUT_CONTAINS("Creating a Classic session to '" +
                              _mysql_uri_nopasswd + "/mysql'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:mysql://" + _mysql_uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // FR_EXTRA_SUCCEED_6 : "\c -ma mysql://user@host:3306/db
  {
    execute("\\c -ma mysql://" + _mysql_uri + "/mysql");
    MY_EXPECT_STDOUT_CONTAINS("Creating a Classic session to '" +
                              _mysql_uri_nopasswd + "/mysql'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:mysql://" + _mysql_uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // FR_EXTRA_SUCCEED_9 : \connect -ma mysqlx://user@host:33060/db
  {
    execute("\\connect -ma mysqlx://" + _uri + "/mysql");
    MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to '" +
                              _uri_nopasswd + "/mysql'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS("Default schema `mysql` accessible through db.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<Session:mysqlx://" + _uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // FR_EXTRA_SUCCEED_10 : \c -ma mysqlx://user@host:33060/db
  {
    execute("\\c -ma mysqlx://" + _uri + "/mysql");
    MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to '" +
                              _uri_nopasswd + "/mysql'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS("Default schema `mysql` accessible through db.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<Session:mysqlx://" + _uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // Session type determined by the URI scheme
  {
    execute("\\connect mysqlx://" + _uri);
    MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to '" +
                              _uri_nopasswd + "'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS("(X protocol)");
    MY_EXPECT_STDOUT_CONTAINS(
        "No default schema selected; type \\use <schema> to set one.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<Session:mysqlx://" + _uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // Using -ma for X protocol session(type determined from connection success)
  {
    execute("\\connect -ma " + _uri);
    MY_EXPECT_STDOUT_CONTAINS("Creating a session to '" + _uri_nopasswd + "'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS("(X protocol)");
    MY_EXPECT_STDOUT_CONTAINS(
        "No default schema selected; type \\use <schema> to set one.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<Session:" + _uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // Using -ma for Classic session(type determined from connection success)
  {
    execute("\\connect -ma " + _mysql_uri);
    MY_EXPECT_STDOUT_CONTAINS("Creating a session to '" + _mysql_uri_nopasswd +
                              "'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS(
        "No default schema selected; type \\use <schema> to set one.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // Using -ma for session type determined by the URI scheme(Classic session)
  {
    execute("\\connect -ma mysql://" + _mysql_uri);
    MY_EXPECT_STDOUT_CONTAINS("Creating a Classic session to '" +
                              _mysql_uri_nopasswd + "'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS(
        "No default schema selected; type \\use <schema> to set one.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:mysql://" + _mysql_uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // Using -ma for session type determined by the URI scheme(X protocol session)
  {
    execute("\\connect -ma mysqlx://" + _uri);
    MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to '" +
                              _uri_nopasswd + "'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS("(X protocol)");
    MY_EXPECT_STDOUT_CONTAINS(
        "No default schema selected; type \\use <schema> to set one.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<Session:mysqlx://" + _uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }
}

TEST_F(Interactive_shell_test, shell_function_connect_node) {
  execute("shell.connect('mysqlx://" + _uri + "');");
  MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to '" +
                            _uri_nopasswd + "'");
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<Session:mysqlx://" + _uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("", output_handler.std_out.c_str());

  execute("session.close()");

  execute("shell.connect('mysqlx://" + _uri + "/mysql');");
  MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to '" +
                            _uri_nopasswd + "/mysql'");
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  MY_EXPECT_STDOUT_CONTAINS("(X protocol)");
  MY_EXPECT_STDOUT_CONTAINS("Default schema `mysql` accessible through db.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<Session:mysqlx://" + _uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  MY_EXPECT_STDOUT_CONTAINS("<Schema:mysql>");
  output_handler.wipe_all();

  execute("session.close()");
}

TEST_F(Interactive_shell_test, shell_function_connect_classic) {
  execute("shell.connect('mysql://" + _mysql_uri + "');");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic session to '" +
                            _mysql_uri_nopasswd + "'");
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:mysql://" + _mysql_uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("", output_handler.std_out.c_str());

  execute("session.close()");

  execute("shell.connect('mysql://" + _mysql_uri + "/mysql');");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic session to '" +
                            _mysql_uri_nopasswd + "/mysql'");
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:mysql://" + _mysql_uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSchema:mysql>");
  output_handler.wipe_all();

  execute("session.close()");
}

TEST_F(Interactive_shell_test, shell_function_connect_auto) {
  // Session type determined from connection success
  {
    execute("shell.connect('" + _uri + "');");
    MY_EXPECT_STDOUT_CONTAINS("Creating a session to '" + _uri_nopasswd + "'");
    MY_EXPECT_STDOUT_CONTAINS(
        "No default schema selected; type \\use <schema> to set one.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<Session:" + _uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // Session type determined from connection success
  {
    execute("shell.connect('" + _mysql_uri + "');");
    MY_EXPECT_STDOUT_CONTAINS("Creating a session to '" + _mysql_uri_nopasswd +
                              "'");
    MY_EXPECT_STDOUT_CONTAINS(
        "No default schema selected; type \\use <schema> to set one.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }
}

TEST_F(Interactive_shell_test, shell_command_connect_conflicts) {
  execute("\\connect -mx -mc " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [-mx|--mysqlx|-mc|--mysql|-ma] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect --mysqlx -mc " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [-mx|--mysqlx|-mc|--mysql|-ma] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect -mx --mysql " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [-mx|--mysqlx|-mc|--mysql|-ma] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect --mysql --mysqlx " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [-mx|--mysqlx|-mc|--mysql|-ma] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect -mc -mx " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [-mx|--mysqlx|-mc|--mysql|-ma] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect -mc --mysqlx " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [-mx|--mysqlx|-mc|--mysql|-ma] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect -ma --mysqlx " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [-mx|--mysqlx|-mc|--mysql|-ma] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect -mx -ma " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [-mx|--mysqlx|-mc|--mysql|-ma] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect --mysql -ma " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [-mx|--mysqlx|-mc|--mysql|-ma] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect -ma -mc " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [-mx|--mysqlx|-mc|--mysql|-ma] <URI>\n");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_connect_deprecated_options) {
  // Deprecated is not the same as removed, so these are supposed to still
  // work in 8.0
  execute("\\connect -n " + _uri);
  MY_EXPECT_STDERR_CONTAINS(
      "The -n option is deprecated, please use --mysqlx or -mx instead\n");
  MY_EXPECT_STDOUT_CONTAINS("Creating an X");
  output_handler.wipe_all();

  execute("\\connect -N " + _uri);
  MY_EXPECT_STDERR_CONTAINS(
      "The -n option is deprecated, please use --mysqlx or -mx instead\n");
  MY_EXPECT_STDOUT_CONTAINS("Creating an X");
  output_handler.wipe_all();

  execute("\\connect -c " + _mysql_uri);
  MY_EXPECT_STDERR_CONTAINS(
      "The -c option is deprecated, please use --mysql or -mc instead\n");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic");
  output_handler.wipe_all();

  execute("\\connect -C " + _mysql_uri);
  MY_EXPECT_STDERR_CONTAINS(
      "The -c option is deprecated, please use --mysql or -mc instead\n");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_use) {
  execute("\\use mysql");
  MY_EXPECT_STDERR_CONTAINS("Not Connected.");
  output_handler.wipe_all();

  execute("\\connect " + _uri);
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("", output_handler.std_out.c_str());
  output_handler.wipe_all();

  execute("\\use mysql");
  MY_EXPECT_STDOUT_CONTAINS("Default schema `mysql` accessible through db.");
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("<Schema:mysql>\n", output_handler.std_out.c_str());
  output_handler.wipe_all();

  execute("\\use unexisting");
  MY_EXPECT_STDERR_CONTAINS("Unknown database 'unexisting'");
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("<Schema:mysql>\n", output_handler.std_out.c_str());
  output_handler.wipe_all();

  execute("session.close()");

  execute("\\connect -mx " + _uri);
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("", output_handler.std_out.c_str());
  output_handler.wipe_all();

  execute("\\use mysql");
  MY_EXPECT_STDOUT_CONTAINS("Default schema `mysql` accessible through db.");
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("<Schema:mysql>\n", output_handler.std_out.c_str());
  output_handler.wipe_all();

  execute("session.close()");

  execute("\\connect -mc " + _mysql_uri);
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("", output_handler.std_out.c_str());
  output_handler.wipe_all();

  execute("\\use mysql");
  MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`.");
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("<ClassicSchema:mysql>\n", output_handler.std_out.c_str());
  output_handler.wipe_all();

  execute("session.close()");
}

TEST_F(Interactive_shell_test, shell_command_warnings) {
  _options->interactive = true;
  reset_shell();
  execute("\\warnings");
  MY_EXPECT_STDOUT_CONTAINS("Show warnings enabled.");
  output_handler.wipe_all();

  execute("\\W");
  MY_EXPECT_STDOUT_CONTAINS("Show warnings enabled.");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_no_warnings) {
  _options->interactive = true;
  reset_shell();
  execute("\\nowarnings");
  MY_EXPECT_STDOUT_CONTAINS("Show warnings disabled.");
  output_handler.wipe_all();

  execute("\\w");
  MY_EXPECT_STDOUT_CONTAINS("Show warnings disabled.");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_help_js) {
  // Cleanup for the test
  execute("\\?");
  MY_EXPECT_STDOUT_CONTAINS("===== Global Commands =====");
  MY_EXPECT_STDOUT_CONTAINS("\\help       (\\?,\\h)    Print this help.");
  MY_EXPECT_STDOUT_CONTAINS(
      "\\sql                   Switch to SQL processing mode.");
  MY_EXPECT_STDOUT_CONTAINS(
      "\\js                    Switch to JavaScript processing mode.");
  MY_EXPECT_STDOUT_CONTAINS(
      "\\py                    Switch to Python processing mode.");
  MY_EXPECT_STDOUT_CONTAINS(
      "\\source     (\\.)       Execute a script file. Takes a file name as an "
      "argument.");
  MY_EXPECT_STDOUT_CONTAINS(
      "\\                      Start multi-line input when in SQL mode.");
  MY_EXPECT_STDOUT_CONTAINS("\\quit       (\\q,\\exit) Quit MySQL Shell.");
  MY_EXPECT_STDOUT_CONTAINS("\\connect    (\\c)       Connect to a server.");
  MY_EXPECT_STDOUT_CONTAINS(
      "\\warnings   (\\W)       Show warnings after every statement.");
  MY_EXPECT_STDOUT_CONTAINS(
      "\\nowarnings (\\w)       Don't show warnings after every statement.");
  MY_EXPECT_STDOUT_CONTAINS(
      "\\status     (\\s)       Print information about the current global "
      "connection.");
  MY_EXPECT_STDOUT_CONTAINS(
      "\\use        (\\u)       Set the current schema for the active "
      "session.");
  MY_EXPECT_STDOUT_CONTAINS(
      "For help on a specific command use the command as \\? <command>");

  execute("\\help \\source");
  MY_EXPECT_STDOUT_CONTAINS(
      "NOTE: Can execute files from the supported types: SQL, Javascript, or "
      "Python.");
  output_handler.wipe_all();

  execute("\\help \\connect");
  MY_EXPECT_STDOUT_CONTAINS(
      "If TYPE is omitted, -ma is assumed by default, unless the protocol is "
      "given in the URI.");
  output_handler.wipe_all();

  execute("\\h \\connect");
  MY_EXPECT_STDOUT_CONTAINS("Connect to a server.");
  MY_EXPECT_STDOUT_CONTAINS("NAME: \\connect or \\c");
  MY_EXPECT_STDOUT_CONTAINS("SYNTAX:");
  MY_EXPECT_STDOUT_CONTAINS("   \\connect [<TYPE>] <URI>");
  MY_EXPECT_STDOUT_CONTAINS("WHERE:");
  MY_EXPECT_STDOUT_CONTAINS(
      "   TYPE is an optional parameter to specify the session type. Accepts "
      "the following values:");
  MY_EXPECT_STDOUT_CONTAINS(
      "        -mc, --mysql: open a classic MySQL protocol session (default "
      "port 3306)");
  MY_EXPECT_STDOUT_CONTAINS(
      "        -mx, --mysqlx: open an X protocol session (default port 33060)");
  MY_EXPECT_STDOUT_CONTAINS(
      "        -ma: attempt automatic detection of the protocol type");
  MY_EXPECT_STDOUT_CONTAINS(
      "        If TYPE is omitted, -ma is assumed by default, unless the "
      "protocol is given in the URI.");
  MY_EXPECT_STDOUT_CONTAINS(
      "   URI format is: [user[:password]@]hostname[:port]");
  MY_EXPECT_STDOUT_CONTAINS("EXAMPLE:");
  MY_EXPECT_STDOUT_CONTAINS("   \\connect -mx root@localhost");
  output_handler.wipe_all();

  execute("\\help \\use");
  MY_EXPECT_STDOUT_CONTAINS(
      "The global \'db\' variable will be updated to hold the requested "
      "schema.");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_help_global_objects_js) {
  // Cleanup for the test
  execute("\\js");
  execute("\\connect -mx " + _uri + "/mysql");
  execute("\\?");
  MY_EXPECT_STDOUT_CONTAINS("===== Global Objects =====");
  MY_EXPECT_STDOUT_CONTAINS(
      "db         Used to work with database schema objects.");
  MY_EXPECT_STDOUT_CONTAINS(
      "dba        Enables you to administer InnoDB clusters using the "
      "AdminAPI.");
  MY_EXPECT_STDOUT_CONTAINS(
      "mysql      Used to work with classic MySQL sessions using SQL.");
  MY_EXPECT_STDOUT_CONTAINS(
      "mysqlx     Used to work with X Protocol sessions using the MySQL X "
      "DevAPI.");
  MY_EXPECT_STDOUT_CONTAINS(
      "session    Represents the currently open MySQL session.");
  MY_EXPECT_STDOUT_CONTAINS(
      "shell      Gives access to general purpose functions and properties.");
  MY_EXPECT_STDOUT_CONTAINS(
      "sys        Gives access to system specific parameters.");
  execute("session.close()");

  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_help_global_objects_py) {
  // Cleanup for the test
  execute("\\py");
  execute("\\connect -mx " + _uri + "/mysql");
  execute("\\?");
  MY_EXPECT_STDOUT_CONTAINS("===== Global Objects =====");
  MY_EXPECT_STDOUT_CONTAINS(
      "db         Used to work with database schema objects.");
  MY_EXPECT_STDOUT_CONTAINS(
      "dba        Enables you to administer InnoDB clusters using the "
      "AdminAPI.");
  MY_EXPECT_STDOUT_CONTAINS(
      "mysql      Used to work with classic MySQL sessions using SQL.");
  MY_EXPECT_STDOUT_CONTAINS(
      "mysqlx     Used to work with X Protocol sessions using the MySQL X "
      "DevAPI.");
  MY_EXPECT_STDOUT_CONTAINS(
      "session    Represents the currently open MySQL session.");
  MY_EXPECT_STDOUT_CONTAINS(
      "shell      Gives access to general purpose functions and properties.");
  MY_EXPECT_STDOUT_NOT_CONTAINS(
      "sys        Gives access to system specific parameters.");
  execute("session.close()");

  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_help_global_objects_sql) {
  // Cleanup for the test
  execute("\\sql");
  execute("\\connect -mx " + _uri + "/mysql");
  execute("\\?");
  MY_EXPECT_STDOUT_NOT_CONTAINS("===== Global Objects =====");

  // We have to change to a scripting mode to close the session
  execute("\\js");
  execute("session.close()");

  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_empty_source_command) {
  _interactive_shell->process_line("\\source");
  MY_EXPECT_STDERR_CONTAINS("Filename not specified");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_source_invalid_path_js) {
  _interactive_shell->process_line("\\js");

  // directory
  std::string tmpdir = getenv("TMPDIR") ? getenv("TMPDIR") : ".";
  _interactive_shell->process_line("\\source " + tmpdir);
  MY_EXPECT_STDERR_CONTAINS("Failed to open file: '" + tmpdir +
                            "' is a directory");

  output_handler.wipe_all();

  std::string filename = "empty_file_" + random_string(10);

  // no such file
  _interactive_shell->process_line("\\source " + filename);
  MY_EXPECT_STDERR_CONTAINS("Failed to open file '" + filename +
                            "', error: No such file or directory");

  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_source_invalid_path_py) {
  _interactive_shell->process_line("\\py");

  // directory
  std::string tmpdir = getenv("TMPDIR") ? getenv("TMPDIR") : ".";
  _interactive_shell->process_line("\\source " + tmpdir);
  MY_EXPECT_STDERR_CONTAINS("Failed to open file: '" + tmpdir +
                            "' is a directory");

  output_handler.wipe_all();

  std::string filename = "empty_file_" + random_string(10);

  // no such file
  _interactive_shell->process_line("\\source " + filename);
  MY_EXPECT_STDERR_CONTAINS("Failed to open file '" + filename +
                            "', error: No such file or directory");

  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, python_startup_scripts) {
  std::string user_path = shcore::get_user_config_path();
  user_path += "mysqlshrc.py";

  // User Config path is executed last
  std::string user_backup;
  bool user_existed = shcore::load_text_file(user_path, user_backup);

  std::ofstream out;
  out.open(user_path, std::ios_base::trunc);
  if (!out.fail()) {
    out << "the_variable = 'Local Value'" << std::endl;
    out.close();
  }

  std::string bin_path = shcore::get_binary_folder();
  bin_path += "/mysqlshrc.py";

  // Binary Config path is executed first
  std::string bin_backup;
  bool bin_existed = shcore::load_text_file(bin_path, user_backup);

  out.open(bin_path, std::ios_base::app);
  if (!out.fail()) {
    out << "from mysqlsh import mysqlx" << std::endl;
    out << "the_variable = 'Global Value'" << std::endl;
    out.close();
  }

  _options->full_interactive = true;
  _options->initial_mode = shcore::IShell_core::Mode::Python;
  reset_shell();
  output_handler.wipe_all();

  execute("mysqlx");
  MY_EXPECT_STDOUT_CONTAINS("<module '__mysqlx__' (built-in)>");
  output_handler.wipe_all();

  execute("the_variable");
  MY_EXPECT_STDOUT_CONTAINS("Local Value");
  output_handler.wipe_all();

  std::remove(user_path.c_str());
  std::remove(bin_path.c_str());

  // If there startup files on the paths used on this test, they are restored
  if (user_existed) {
    out.open(user_path, std::ios_base::trunc);
    if (!out.fail()) {
      out.write(user_backup.c_str(), user_backup.size());
      out.close();
    }
  }

  if (bin_existed) {
    out.open(bin_path, std::ios_base::trunc);
    if (!out.fail()) {
      out.write(bin_backup.c_str(), bin_backup.size());
      out.close();
    }
  }
}

TEST_F(Interactive_shell_test, js_startup_scripts) {
  std::string user_path = shcore::get_user_config_path();
  user_path += "mysqlshrc.js";

  // User Config path is executed last
  std::string user_backup;
  bool user_existed = shcore::load_text_file(user_path, user_backup);

  std::ofstream out;
  out.open(user_path, std::ios_base::trunc);
  if (!out.fail()) {
    out << "var the_variable = 'Local Value';" << std::endl;
    out.close();
  }

  std::string bin_path = shcore::get_binary_folder();
  bin_path += "/mysqlshrc.js";

  // Binary Config path is executed first
  std::string bin_backup;
  bool bin_existed = shcore::load_text_file(bin_path, user_backup);

  out.open(bin_path, std::ios_base::app);
  if (!out.fail()) {
    out << "var mysqlx = require('mysqlx');" << std::endl;
    out << "var the_variable = 'Global Value';" << std::endl;
    out.close();
  }

  _options->full_interactive = true;
  reset_shell();
  output_handler.wipe_all();

  execute("dir(mysqlx)");
  MY_EXPECT_STDOUT_CONTAINS("getSession");
  output_handler.wipe_all();

  execute("the_variable");
  MY_EXPECT_STDOUT_CONTAINS("Local Value");
  output_handler.wipe_all();

  std::remove(user_path.c_str());
  std::remove(bin_path.c_str());

  // If there startup files on the paths used on this test, they are restored
  if (user_existed) {
    out.open(user_path, std::ios_base::trunc);
    if (!out.fail()) {
      out.write(user_backup.c_str(), user_backup.size());
      out.close();
    }
  }

  if (bin_existed) {
    out.open(bin_path, std::ios_base::trunc);
    if (!out.fail()) {
      out.write(bin_backup.c_str(), bin_backup.size());
      out.close();
    }
  }
}

TEST_F(Interactive_shell_test, expired_account_support_classic) {
  // Test secure call passing uri with no password (will be prompted)
  _options->uri = _mysql_uri;
  _options->initial_mode = shcore::IShell_core::Mode::SQL;
  reset_shell();

  // Setup an expired account for the test
  _interactive_shell->connect(_options->connection_options());
  output_handler.wipe_all();
  execute("drop user if exists expired;");
  output_handler.wipe_all();
  execute("create user expired@'%' identified by 'sample';");
  MY_EXPECT_STDOUT_CONTAINS("Query OK, 0 rows affected");
  output_handler.wipe_all();
  execute("grant all on *.* to expired@'%';");
  MY_EXPECT_STDOUT_CONTAINS("Query OK, 0 rows affected");
  output_handler.wipe_all();
  execute("alter user expired@'%' password expire;");
  MY_EXPECT_STDOUT_CONTAINS("Query OK, 0 rows affected");
  output_handler.wipe_all();

  execute("\\js");
  execute("session.close()");
  execute("\\sql");

  // Connects with the expired account
  execute("\\c mysql://expired:sample@localhost:" + _mysql_port);
  MY_EXPECT_STDOUT_CONTAINS(
      "Creating a Classic session to 'expired@localhost:" + _mysql_port + "'");
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  // Tests unable to execute any statement with an expired account
  execute("select host from mysql.user where user = 'expired';");
  MY_EXPECT_STDERR_CONTAINS(
      "ERROR: 1820 (HY000): You must reset your password using ALTER USER "
      "statement before executing this statement.");
  output_handler.wipe_all();

  // Tests allow reseting the password on an expired account
  execute("set password = password('updated');");
  MY_EXPECT_STDOUT_CONTAINS("Query OK, 0 rows affected");
  output_handler.wipe_all();

  execute("select host from mysql.user where user = 'expired';");
  MY_EXPECT_STDOUT_CONTAINS("1 row in set");
  output_handler.wipe_all();

  execute("\\js");
  execute("session.close()");
  execute("\\sql");

  execute("\\c " + _mysql_uri);
  execute("drop user if exists expired;");
  execute("\\js ");
  execute("session.close()");
  MY_EXPECT_STDOUT_CONTAINS("");
}

TEST_F(Interactive_shell_test, expired_account_support_node) {
  // Test secure call passing uri with no password (will be prompted)
  _options->uri = _uri;
  _options->initial_mode = shcore::IShell_core::Mode::SQL;
  reset_shell();

  // Setup an expired account for the test
  _interactive_shell->connect(_options->connection_options());
  output_handler.wipe_all();
  execute("drop user if exists expired;");
  output_handler.wipe_all();
  execute("create user expired@'%' identified by 'sample';");
  MY_EXPECT_STDOUT_CONTAINS("Query OK, 0 rows affected");
  output_handler.wipe_all();
  execute("grant all on *.* to expired@'%';");
  MY_EXPECT_STDOUT_CONTAINS("Query OK, 0 rows affected");
  output_handler.wipe_all();
  execute("alter user expired@'%' password expire;");
  MY_EXPECT_STDOUT_CONTAINS("Query OK, 0 rows affected");
  output_handler.wipe_all();

  execute("\\js");
  execute("session.close()");
  execute("\\sql");

  // Connects with the expired account
  execute("\\c mysqlx://expired:sample@localhost:" + _port);
  MY_EXPECT_STDOUT_CONTAINS(
      "Creating an X protocol session to 'expired@localhost:" + _port + "'");
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  // Tests unable to execute any statement with an expired account
  execute("select host from mysql.user where user = 'expired';");
  MY_EXPECT_STDERR_CONTAINS(
      "ERROR: 1820: You must reset your password using ALTER USER statement "
      "before executing this statement.");
  output_handler.wipe_all();

  // Tests allow reseting the password on an expired account
  execute("set password = password('updated');");
  MY_EXPECT_STDOUT_CONTAINS("Query OK, 0 rows affected");
  output_handler.wipe_all();

  execute("select host from mysql.user where user = 'expired';");
  MY_EXPECT_STDOUT_CONTAINS("1 row in set");
  output_handler.wipe_all();

  execute("\\js");
  execute("session.close()");
  execute("\\sql");

  execute("\\c " + _uri);
  execute("drop user if exists expired;");
  execute("\\js ");
  execute("session.close()");
  MY_EXPECT_STDOUT_CONTAINS("");
}

TEST_F(Interactive_shell_test, classic_sql_result) {
  execute("\\connect " + _mysql_uri);
  execute("\\sql");
  execute("drop schema if exists itst;");
  execute("create schema itst;");
  // Regression test for
  // Bug#26406214 WRONG VALUE FOR SMALLINT ZEROFILL COLUMNS HOLDING NULL VALUE
  execute(
      "create table itst.tbl (a int, b varchar(30), c int(10), "
      "      d int(8) unsigned zerofill, e int(2) zerofill, "
      "      f smallint unsigned zerofill, ggggg tinyint unsigned zerofill, "
      "      h bigint unsigned zerofill, i double unsigned zerofill);");
  execute(
      "insert into itst.tbl values (1, 'one', -42, 42, 42, 42, 42, 42, 42), "
      "         (2, 'two', -12345, 12345, 12345, 12345, 123, 12345, 12345),"
      "         (3, 'three', 0, 0, 0, 0, 0, 0, 0),"
      "         (4, 'four', null, null, null, null, null, null, null);");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("select 1, 'two', 3.3, null;");
  MY_EXPECT_STDOUT_CONTAINS(
      "+---+-----+-----+------+\n"
      "| 1 | two | 3.3 | NULL |\n"
      "+---+-----+-----+------+\n"
      "| 1 | two | 3.3 | NULL |\n"
      "+---+-----+-----+------+\n"
      "1 row in set (");

  wipe_all();
  // test zerofill
  execute("select * from itst.tbl;");
  // clang-format off
  MY_EXPECT_STDOUT_CONTAINS(
      "+---+-------+--------+----------+-------+-------+-------+----------------------+------------------------+\n"
      "| a | b     | c      | d        | e     | f     | ggggg | h                    | i                      |\n"
      "+---+-------+--------+----------+-------+-------+-------+----------------------+------------------------+\n"
      "| 1 | one   |    -42 | 00000042 |    42 | 00042 |   042 | 00000000000000000042 | 0000000000000000000042 |\n"
      "| 2 | two   | -12345 | 00012345 | 12345 | 12345 |   123 | 00000000000000012345 | 0000000000000000012345 |\n"
      "| 3 | three |      0 | 00000000 |    00 | 00000 |   000 | 00000000000000000000 | 0000000000000000000000 |\n"
      "| 4 | four  |   NULL |     NULL |  NULL |  NULL |  NULL |                 NULL |                   NULL |\n"
      "+---+-------+--------+----------+-------+-------+-------+----------------------+------------------------+\n"
      "4 rows in set (");
  // clang-format on
  execute("\\js");
  execute("shell.options['outputFormat']='vertical'");
  execute("\\sql");
  wipe_all();

  execute("select * from itst.tbl;");
  MY_EXPECT_STDOUT_CONTAINS(
      "*************************** 1. row ***************************\n"
      "    a: 1\n"
      "    b: one\n"
      "    c: -42\n"
      "    d: 00000042\n"
      "    e: 42\n"
      "    f: 00042\n"
      "ggggg: 042\n"
      "    h: 00000000000000000042\n"
      "    i: 0000000000000000000042\n"
      "*************************** 2. row ***************************\n"
      "    a: 2\n"
      "    b: two\n"
      "    c: -12345\n"
      "    d: 00012345\n"
      "    e: 12345\n"
      "    f: 12345\n"
      "ggggg: 123\n"
      "    h: 00000000000000012345\n"
      "    i: 0000000000000000012345\n"
      "*************************** 3. row ***************************\n"
      "    a: 3\n"
      "    b: three\n"
      "    c: 0\n"
      "    d: 00000000\n"
      "    e: 00\n"
      "    f: 00000\n"
      "ggggg: 000\n"
      "    h: 00000000000000000000\n"
      "    i: 0000000000000000000000\n"
      "*************************** 4. row ***************************\n"
      "    a: 4\n"
      "    b: four\n"
      "    c: NULL\n"
      "    d: NULL\n"
      "    e: NULL\n"
      "    f: NULL\n"
      "ggggg: NULL\n"
      "    h: NULL\n"
      "    i: NULL\n"
      "4 rows in set");

  execute("\\js");
  execute("shell.options['outputFormat']='json'");
  execute("\\sql");
  wipe_all();

  execute("select * from itst.tbl;");
  MY_EXPECT_STDOUT_CONTAINS(
      "    \"info\": \"\",\n"
      "    \"rows\": [\n"
      "        {\n"
      "            \"a\": 1,\n"
      "            \"b\": \"one\",\n"
      "            \"c\": -42,\n"
      "            \"d\": 42,\n"
      "            \"e\": 42,\n"
      "            \"f\": 42,\n"
      "            \"ggggg\": 42,\n"
      "            \"h\": 42,\n"
      "            \"i\": 42.0\n"
      "        },\n"
      "        {\n"
      "            \"a\": 2,\n"
      "            \"b\": \"two\",\n"
      "            \"c\": -12345,\n"
      "            \"d\": 12345,\n"
      "            \"e\": 12345,\n"
      "            \"f\": 12345,\n"
      "            \"ggggg\": 123,\n"
      "            \"h\": 12345,\n"
      "            \"i\": 12345.0\n"
      "        },\n"
      "        {\n"
      "            \"a\": 3,\n"
      "            \"b\": \"three\",\n"
      "            \"c\": 0,\n"
      "            \"d\": 0,\n"
      "            \"e\": 0,\n"
      "            \"f\": 0,\n"
      "            \"ggggg\": 0,\n"
      "            \"h\": 0,\n"
      "            \"i\": 0.0\n"
      "        },\n"
      "        {\n"
      "            \"a\": 4,\n"
      "            \"b\": \"four\",\n"
      "            \"c\": null,\n"
      "            \"d\": null,\n"
      "            \"e\": null,\n"
      "            \"f\": null,\n"
      "            \"ggggg\": null,\n"
      "            \"h\": null,\n"
      "            \"i\": null\n"
      "        }\n"
      "    ],\n"
      "    \"warningCount\": 0,\n"
      "    \"warnings\": [],\n"
      "    \"hasData\": true,\n"
      "    \"affectedRowCount\": 0,\n"
      "    \"autoIncrementValue\": 0\n"
      "}\n");

  execute("drop schema itst;");
}

TEST_F(Interactive_shell_test, x_sql_result) {
  execute("\\connect " + _uri);
  execute("\\sql");
  execute("drop schema if exists itst;");
  execute("create schema itst;");
  execute(
      "create table itst.tbl (a int, b varchar(30), c int(10), "
      "    d int(8) unsigned zerofill, e int(2) zerofill, "
      "    f smallint unsigned zerofill, ggggg tinyint unsigned zerofill, "
      "    h bigint unsigned zerofill, i double unsigned zerofill);");
  execute(
      "insert into itst.tbl values (1, 'one', -42, 42, 42, 42, 42, 42, 42), "
      "       (2, 'two', -12345, 12345, 12345, 12345, 123, 12345, 12345),"
      "       (3, 'three', 0, 0, 0, 0, 0, 0, 0),"
      "       (4, 'four', null, null, null, null, null, null, null);");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("select 1, 'two', 3.3, null;");
  MY_EXPECT_STDOUT_CONTAINS(
      "+---+-----+-----+------+\n"
      "| 1 | two | 3.3 | NULL |\n"
      "+---+-----+-----+------+\n"
      "| 1 | two | 3.3 | NULL |\n"
      "+---+-----+-----+------+\n"
      "1 row in set (");

  wipe_all();
  // test zerofill
  execute("select * from itst.tbl;");
  // NOTE: X protocol does not support zerofill for double atm
  // clang-format off
  MY_EXPECT_STDOUT_CONTAINS(
      "+---+-------+--------+----------+-------+-------+-------+----------------------+-------+\n"
      "| a | b     | c      | d        | e     | f     | ggggg | h                    | i     |\n"
      "+---+-------+--------+----------+-------+-------+-------+----------------------+-------+\n"
      "| 1 | one   |    -42 | 00000042 |    42 | 00042 |   042 | 00000000000000000042 |    42 |\n"
      "| 2 | two   | -12345 | 00012345 | 12345 | 12345 |   123 | 00000000000000012345 | 12345 |\n"
      "| 3 | three |      0 | 00000000 |    00 | 00000 |   000 | 00000000000000000000 |     0 |\n"
      "| 4 | four  |   NULL |     NULL |  NULL |  NULL |  NULL |                 NULL |  NULL |\n"
      "+---+-------+--------+----------+-------+-------+-------+----------------------+-------+\n"
      "4 rows in set (");
  // clang-format on
  execute("\\js");
  execute("shell.options['outputFormat']='vertical'");
  execute("\\sql");
  wipe_all();

  execute("select * from itst.tbl;");
  MY_EXPECT_STDOUT_CONTAINS(
      "*************************** 1. row ***************************\n"
      "    a: 1\n"
      "    b: one\n"
      "    c: -42\n"
      "    d: 00000042\n"
      "    e: 42\n"
      "    f: 00042\n"
      "ggggg: 042\n"
      "    h: 00000000000000000042\n"
      "    i: 42\n"
      "*************************** 2. row ***************************\n"
      "    a: 2\n"
      "    b: two\n"
      "    c: -12345\n"
      "    d: 00012345\n"
      "    e: 12345\n"
      "    f: 12345\n"
      "ggggg: 123\n"
      "    h: 00000000000000012345\n"
      "    i: 12345\n"
      "*************************** 3. row ***************************\n"
      "    a: 3\n"
      "    b: three\n"
      "    c: 0\n"
      "    d: 00000000\n"
      "    e: 00\n"
      "    f: 00000\n"
      "ggggg: 000\n"
      "    h: 00000000000000000000\n"
      "    i: 0\n"
      "*************************** 4. row ***************************\n"
      "    a: 4\n"
      "    b: four\n"
      "    c: NULL\n"
      "    d: NULL\n"
      "    e: NULL\n"
      "    f: NULL\n"
      "ggggg: NULL\n"
      "    h: NULL\n"
      "    i: NULL\n"
      "4 rows in set");

  execute("\\js");
  execute("shell.options['outputFormat']='json'");
  execute("\\sql");
  wipe_all();

  execute("select * from itst.tbl;");
  MY_EXPECT_STDOUT_CONTAINS(
      "    \"warningCount\": 0,\n"
      "    \"warnings\": [],\n"
      "    \"rows\": [\n"
      "        {\n"
      "            \"a\": 1,\n"
      "            \"b\": \"one\",\n"
      "            \"c\": -42,\n"
      "            \"d\": 42,\n"
      "            \"e\": 42,\n"
      "            \"f\": 42,\n"
      "            \"ggggg\": 42,\n"
      "            \"h\": 42,\n"
      "            \"i\": 42.0\n"
      "        },\n"
      "        {\n"
      "            \"a\": 2,\n"
      "            \"b\": \"two\",\n"
      "            \"c\": -12345,\n"
      "            \"d\": 12345,\n"
      "            \"e\": 12345,\n"
      "            \"f\": 12345,\n"
      "            \"ggggg\": 123,\n"
      "            \"h\": 12345,\n"
      "            \"i\": 12345.0\n"
      "        },\n"
      "        {\n"
      "            \"a\": 3,\n"
      "            \"b\": \"three\",\n"
      "            \"c\": 0,\n"
      "            \"d\": 0,\n"
      "            \"e\": 0,\n"
      "            \"f\": 0,\n"
      "            \"ggggg\": 0,\n"
      "            \"h\": 0,\n"
      "            \"i\": 0.0\n"
      "        },\n"
      "        {\n"
      "            \"a\": 4,\n"
      "            \"b\": \"four\",\n"
      "            \"c\": null,\n"
      "            \"d\": null,\n"
      "            \"e\": null,\n"
      "            \"f\": null,\n"
      "            \"ggggg\": null,\n"
      "            \"h\": null,\n"
      "            \"i\": null\n"
      "        }\n"
      "    ],\n"
      "    \"hasData\": true,\n"
      "    \"affectedRowCount\": 0,\n");

  execute("drop schema itst;");
}

TEST_F(Interactive_shell_test, BUG25974014) {
  // We are going to test if shell attempts to reconnect after session is
  // disconnected
  _options->uri = _uri + "/mysql";
  _options->interactive = true;
  _options->initial_mode = shcore::IShell_core::Mode::SQL;
  reset_shell();
  _interactive_shell->connect(_options->connection_options());
  output_handler.wipe_all();
  std::stringstream cmd;
  cmd << get_path_to_mysqlsh() << " " << _mysql_uri << " --sql -e \"kill "
      << _interactive_shell->shell_context()
             ->get_dev_session()
             ->get_connection_id()
      << "\"";
  EXPECT_EQ(system(cmd.str().c_str()), 0);
  output_handler.wipe_all();

  // After the kill first command receives interrupted error
  execute("\\use mysql");
  MY_EXPECT_STDERR_CONTAINS("interrupted");
  EXPECT_EQ("", output_handler.std_out);
  output_handler.wipe_all();

  // On second execution shell should attempt to reconnect
  execute("\\use mysql");
  MY_EXPECT_STDOUT_CONTAINS("Attempting to reconnect");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, ssl_status) {
  wipe_all();
  execute("\\connect " + _uri + "?ssl-Mode=DISABLED");
  execute("\\status");
  MY_EXPECT_STDOUT_CONTAINS("Not in use.");
  EXPECT_EQ("", _interactive_shell->prompt_variables()->at("ssl"));

  wipe_all();
  execute("\\connect " + _uri + "?ssl-Mode=REQUIRED");
  execute("\\status");
  MY_EXPECT_STDOUT_CONTAINS("Cipher in use: ");
  MY_EXPECT_STDOUT_CONTAINS("TLSv");
  EXPECT_EQ("SSL", _interactive_shell->prompt_variables()->at("ssl"));

  wipe_all();
  execute("\\connect " + _mysql_uri + "?ssl-Mode=DISABLED");
  execute("\\status");
  MY_EXPECT_STDOUT_CONTAINS("Not in use.");
  EXPECT_EQ("", _interactive_shell->prompt_variables()->at("ssl"));

  wipe_all();
  execute("\\connect " + _mysql_uri + "?ssl-Mode=REQUIRED");
  execute("\\status");
  MY_EXPECT_STDOUT_CONTAINS("Cipher in use: ");
  MY_EXPECT_STDOUT_CONTAINS("TLSv");
  EXPECT_EQ("SSL", _interactive_shell->prompt_variables()->at("ssl"));
}

TEST_F(Interactive_shell_test, status_x) {
  execute("\\connect " + _uri + "?ssl-Mode=DISABLED");
  wipe_all();
  execute("\\status");

  char c = 0;
  int dec = 0;
  std::stringstream ss(output_handler.std_out);
  std::string line;
  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("X"));

  // ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  // EXPECT_NE(std::string::npos, line.find("Server type"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_EQ(1, sscanf(line.c_str(), "Connection Id:                %d", &dec));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Default schema"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Current schema"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Current user"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("SSL"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_EQ(1, sscanf(line.c_str(), "Using delimiter:              %c", &c));
  EXPECT_EQ(c, ';');

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Server version"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_EQ(line, "Protocol version:             X protocol");

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_EQ(3, sscanf(line.c_str(), "Client library:               %d.%d.%d",
                      &dec, &dec, &dec));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Connection"));
  EXPECT_NE(std::string::npos, line.find("via TCP"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Server characterset"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Schema characterset"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Client characterset"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Conn. characterset"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_EQ(1, sscanf(line.c_str(), "TCP port:                     %d", &dec));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Uptime"));
  EXPECT_NE(std::string::npos, line.find("sec"));
}

TEST_F(Interactive_shell_test, status_classic) {
  execute("\\connect " + _mysql_uri + "?ssl-Mode=DISABLED");
  wipe_all();
  execute("\\status");

  char c = 0;
  int dec = 0;
  std::stringstream ss(output_handler.std_out);
  std::string line;
  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Classic"));

  // ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  // EXPECT_NE(std::string::npos, line.find("Server type"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_EQ(1, sscanf(line.c_str(), "Connection Id:                %d", &dec));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Current schema"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Current user"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("SSL"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_EQ(1, sscanf(line.c_str(), "Using delimiter:              %c", &c));
  EXPECT_EQ(c, ';');

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Server version"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_EQ(1, sscanf(line.c_str(), "Protocol version:             classic %d",
                      &dec));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_EQ(3, sscanf(line.c_str(), "Client library:               %d.%d.%d",
                      &dec, &dec, &dec));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Connection"));
  EXPECT_NE(std::string::npos, line.find("via TCP"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Server characterset"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Schema characterset"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Client characterset"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  ASSERT_NE(line.find("Conn. characterset"), std::string::npos);

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_EQ(1, sscanf(line.c_str(), "TCP port:                     %d", &dec));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Uptime"));
  EXPECT_NE(std::string::npos, line.find("sec"));
}

}  // namespace mysqlsh
