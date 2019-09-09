/* Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.

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

#include <cstdio>

#include "unittest/gprod_clean.h"

#include "src/mysqlsh/cmdline_shell.h"
#include "unittest/test_utils.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"

extern mysqlshdk::utils::Version g_target_server_version;

namespace mysqlsh {
class Interactive_shell_test : public Shell_core_test_wrapper {
 protected:
#ifdef HAVE_V8
  const std::string to_scripting = "\\js";
#else
  const std::string to_scripting = "\\py";
#endif
 public:
  virtual void set_options() {
    _options->interactive = true;
    _options->wizards = true;
  }

  static void SetUpTestCase() {
    run_script_classic({"drop user if exists expired@localhost"});
  }
};

TEST_F(Interactive_shell_test, shell_get_session_BUG27809310) {
  EXPECT_NO_THROW(execute("shell.getSession()"));
}

TEST_F(Interactive_shell_test, shell_command_connect_node) {
  execute("\\connect --mx " + _uri);
  MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to '" +
                            _uri_nopasswd + "'");
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  MY_EXPECT_STDOUT_CONTAINS("(X protocol)");
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<Session:" + _uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("", output_handler.std_out.c_str());
  EXPECT_STREQ("", output_handler.std_err.c_str());

  execute("session.close()");

  execute("\\connect --mx " + _uri + "/mysql");
  MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to '" +
                            _uri_nopasswd + "/mysql'");
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  MY_EXPECT_STDOUT_CONTAINS("(X protocol)");
  MY_EXPECT_STDOUT_CONTAINS("Default schema `mysql` accessible through db.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<Session:" + _uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  MY_EXPECT_STDOUT_CONTAINS("<Schema:mysql>");
  output_handler.wipe_all();

  execute("session.close()");

  execute("\\connect --mx mysql://" + _mysql_uri);
  MY_EXPECT_STDERR_CONTAINS(
      "The given URI conflicts with the --mysqlx session type option.");
  output_handler.wipe_all();

  execute("\\connect --mx " + _mysql_uri);
  // wrong protocol can manifest as this error or a 2006 'gone away' error
  if (output_handler.std_err.find("MySQL Error 2006") == std::string::npos)
    MY_EXPECT_STDERR_CONTAINS(
        "Requested session assumes MySQL X Protocol but '" + _host + ":" +
        _mysql_port + "' seems to speak the classic MySQL protocol");
  output_handler.wipe_all();

  // Invalid user/password
  output_handler.passwords.push_back({"*", "whatever"});
  execute("\\connect --mx " + _uri_nopasswd);
  if (g_target_server_version >= mysqlshdk::utils::Version(8, 0, 12)) {
    MY_EXPECT_STDERR_CONTAINS("Access denied for user 'root'@'localhost'");
  } else {
    MY_EXPECT_STDERR_CONTAINS("MySQL Error 1045: Invalid user or password");
  }
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_connect_classic) {
  execute("\\connect --mc " + _mysql_uri);
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic session to '" +
                            _mysql_uri_nopasswd + "'");
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("", output_handler.std_out.c_str());
  EXPECT_STREQ("", output_handler.std_err.c_str());

  execute("session.close()");

  execute("\\connect --mc " + _mysql_uri + "/mysql");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic session to '" +
                            _mysql_uri_nopasswd + "/mysql'");
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
  output_handler.wipe_all();

  execute("db");

  output_handler.wipe_all();

  execute("session.close()");

  execute("\\connect --mc mysqlx://" + _uri);
  MY_EXPECT_STDERR_CONTAINS(
      "The given URI conflicts with the --mysql session type option.");
  output_handler.wipe_all();

  // FR_EXTRA_SUCCEED_7 : \connect --mc mysql://user@host:3306/db
  {
    execute("\\connect --mc mysql://" + _mysql_uri + "/mysql");
    MY_EXPECT_STDOUT_CONTAINS("Creating a Classic session to '" +
                              _mysql_uri_nopasswd + "/mysql'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // FR_EXTRA_SUCCEED_8 : \c --mc mysql://user@host:3306/db
  {
    execute("\\c --mc mysql://" + _mysql_uri + "/mysql");
    MY_EXPECT_STDOUT_CONTAINS("Creating a Classic session to '" +
                              _mysql_uri_nopasswd + "/mysql'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }
}

TEST_F(Interactive_shell_test, shell_command_connect_x) {
  // FR_EXTRA_SUCCEED_11 : \connect --mx mysqlx://user@host:33060/db
  {
    execute("\\connect --mx mysqlx://" + _uri + "/mysql");
    MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to '" +
                              _uri_nopasswd + "/mysql'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS("Default schema `mysql` accessible through db.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<Session:" + _uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // FR_EXTRA_SUCCEED_12 : \c --mx mysqlx://user@host:33060/db
  {
    execute("\\c --mx mysqlx://" + _uri + "/mysql");
    MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to '" +
                              _uri_nopasswd + "/mysql'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS("Default schema `mysql` accessible through db.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<Session:" + _uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }
}

TEST_F(Interactive_shell_test, shell_command_connect_auto) {
  // Session type determined from connection success
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
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
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
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
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
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
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
    MY_EXPECT_STDOUT_CONTAINS("<Session:" + _uri_nopasswd);
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
    MY_EXPECT_STDOUT_CONTAINS("<Session:" + _uri_nopasswd);
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
    MY_EXPECT_STDOUT_CONTAINS("<Session:" + _uri_nopasswd);
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
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
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
    MY_EXPECT_STDOUT_CONTAINS("<Session:" + _uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

#ifndef _WIN32
  {
    auto data = shcore::get_connection_options(_uri);
    std::string user = data.get_user();
    data.clear_host();
    data.clear_port();

    // Check for URI syntax with socket path (classic)
    if (!_mysql_socket.empty()) {
      data.clear_socket();
      data.set_socket(_mysql_socket);
      // \connect --mc user@/path%2Fto%2Fsocket
      {
        execute("\\connect --mc " +
                data.as_uri(mysqlshdk::db::uri::formats::full()));
        MY_EXPECT_STDOUT_CONTAINS("Creating a Classic session to ");
        MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
        MY_EXPECT_STDOUT_CONTAINS(
            "No default schema selected; type \\use <schema> to set one.");
        output_handler.wipe_all();

        execute("session");
        MY_EXPECT_STDOUT_CONTAINS(
            "<ClassicSession:" +
            data.as_uri(mysqlshdk::db::uri::formats::full_no_password()) + ">");
        output_handler.wipe_all();

        execute("session.close()");
        execute("session");
        MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:disconnected>");
        output_handler.wipe_all();
      }
      // \connect user@/path%2Fto%2Fsocket
      {
        execute("\\connect " +
                data.as_uri(mysqlshdk::db::uri::formats::full()));
        MY_EXPECT_STDOUT_CONTAINS("Creating a session to ");
        MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
        MY_EXPECT_STDOUT_CONTAINS(
            "No default schema selected; type \\use <schema> to set one.");
        output_handler.wipe_all();

        execute("session");
        MY_EXPECT_STDOUT_CONTAINS(
            "<ClassicSession:" +
            data.as_uri(mysqlshdk::db::uri::formats::full_no_password()) + ">");
        output_handler.wipe_all();

        execute("session.close()");
        execute("session");
        MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:disconnected>");
        output_handler.wipe_all();
      }
      // \connect user@/path%2Fto%2Fsocket
      // using classic socket as X protocol connection must fail
      {
        execute("\\connect --mx " +
                data.as_uri(mysqlshdk::db::uri::formats::full()));
        MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to ");
        // wrong protocol can manifest as this error or a 2006 'gone away' error
        if (output_handler.std_err.find("MySQL Error 2006") ==
            std::string::npos) {
          MY_EXPECT_STDERR_CONTAINS("MySQL Error 2027");
          MY_EXPECT_STDERR_CONTAINS(
              "Requested session assumes MySQL X Protocol but");
          MY_EXPECT_STDERR_CONTAINS(
              "seems to speak the classic MySQL protocol");
          output_handler.wipe_all();
        }
        execute("session");
        MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:disconnected>");
        output_handler.wipe_all();
      }
    }

    // Check for URI syntax with xsocket path (x protocol)
    if (!_socket.empty()) {
      data.clear_socket();
      data.set_socket(_socket);
      // \connect --mx user@/path%2Fto%2Fx_socket
      {
        execute("\\connect --mx " +
                data.as_uri(mysqlshdk::db::uri::formats::full()));
        MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to ");
        MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
        MY_EXPECT_STDOUT_CONTAINS(
            "No default schema selected; type \\use <schema> to set one.");
        output_handler.wipe_all();

        execute("session");
        MY_EXPECT_STDOUT_CONTAINS(
            "<Session:" +
            data.as_uri(mysqlshdk::db::uri::formats::full_no_password()) + ">");
        output_handler.wipe_all();

        execute("session.close()");
        execute("session");
        MY_EXPECT_STDOUT_CONTAINS("<Session:disconnected>");
        output_handler.wipe_all();
      }
      // \connect user@/path%2Fto%2Fx_socket
      {
        execute("\\connect " +
                data.as_uri(mysqlshdk::db::uri::formats::full()));
        MY_EXPECT_STDOUT_CONTAINS("Creating a session to ");
        MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
        MY_EXPECT_STDOUT_CONTAINS(
            "No default schema selected; type \\use <schema> to set one.");
        output_handler.wipe_all();

        execute("session");
        MY_EXPECT_STDOUT_CONTAINS(
            "<Session:" +
            data.as_uri(mysqlshdk::db::uri::formats::full_no_password()) + ">");
        output_handler.wipe_all();

        execute("session.close()");
        execute("session");
        MY_EXPECT_STDOUT_CONTAINS("<Session:disconnected>");
        output_handler.wipe_all();
      }
      // \connect --mc user@/path%2Fto%2Fx_socket
      // using x socket as classic connection must fail
      // Test omitted due to long time to discover wrong protocol
    }
  }
#endif  // !_WIN32
}

TEST_F(Interactive_shell_test, shell_function_connect_node) {
  execute("shell.connect('mysqlx://" + _uri + "');");
  MY_EXPECT_STDOUT_CONTAINS("Creating an X protocol session to '" +
                            _uri_nopasswd + "'");
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<Session:" + _uri_nopasswd);
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
  MY_EXPECT_STDOUT_CONTAINS("<Session:" + _uri_nopasswd);
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
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
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
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("", output_handler.std_out.c_str());
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
  execute("\\connect --mx --mc " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [--mx|--mysqlx|--mc|--mysql] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect --mysqlx --mc " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [--mx|--mysqlx|--mc|--mysql] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect --mx --mysql " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [--mx|--mysqlx|--mc|--mysql] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect --mysql --mysqlx " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [--mx|--mysqlx|--mc|--mysql] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect --mc --mx " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [--mx|--mysqlx|--mc|--mysql] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect --mc --mysqlx " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [--mx|--mysqlx|--mc|--mysql] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect -ma --mysqlx " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [--mx|--mysqlx|--mc|--mysql] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect --mx -ma " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [--mx|--mysqlx|--mc|--mysql] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect --mysql -ma " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [--mx|--mysqlx|--mc|--mysql] <URI>\n");
  output_handler.wipe_all();

  execute("\\connect -ma --mc " + _uri);
  MY_EXPECT_STDERR_CONTAINS("\\connect [--mx|--mysqlx|--mc|--mysql] <URI>\n");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_connect_deprecated_options) {
  // Deprecated is not the same as removed, so these are supposed to still
  // work in 8.0
  execute("\\connect -n " + _uri);
  MY_EXPECT_STDOUT_CONTAINS(
      "WARNING: The -n option is deprecated, please use --mysqlx or --mx "
      "instead.");
  MY_EXPECT_STDOUT_CONTAINS("Creating an X");
  output_handler.wipe_all();

  execute("\\connect -mx " + _uri);
  MY_EXPECT_STDOUT_CONTAINS(
      "WARNING: The -mx option is deprecated, please use --mysqlx or --mx "
      "instead.");
  MY_EXPECT_STDOUT_CONTAINS("Creating an X");
  output_handler.wipe_all();

  execute("\\connect -N " + _uri);
  MY_EXPECT_STDOUT_CONTAINS(
      "WARNING: The -N option is deprecated, please use --mysqlx or --mx "
      "instead.");
  MY_EXPECT_STDOUT_CONTAINS("Creating an X");
  output_handler.wipe_all();

  execute("\\connect -c " + _mysql_uri);
  MY_EXPECT_STDOUT_CONTAINS(
      "WARNING: The -c option is deprecated, please use --mysql or --mc "
      "instead.");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic");
  output_handler.wipe_all();

  execute("\\connect -mc " + _mysql_uri);
  MY_EXPECT_STDOUT_CONTAINS(
      "WARNING: The -mc option is deprecated, please use --mysql or --mc "
      "instead.");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic");
  output_handler.wipe_all();

  execute("\\connect -C " + _mysql_uri);
  MY_EXPECT_STDOUT_CONTAINS(
      "WARNING: The -C option is deprecated, please use --mysql or --mc "
      "instead.");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic");
  output_handler.wipe_all();

  execute("\\connect -ma " + _mysql_uri);
  MY_EXPECT_STDOUT_CONTAINS("WARNING: The -ma option is deprecated.");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_use) {
  execute("\\use mysql");
  MY_EXPECT_STDERR_CONTAINS("Not connected.");
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

  execute("\\connect --mx " + _uri);
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

  execute("\\connect --mc " + _mysql_uri);
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
  EXPECT_STREQ("", output_handler.std_out.c_str());
  output_handler.wipe_all();

  execute("session.close()");

  execute("\\sql");
  execute("\\connect --mc " + _mysql_uri);
  execute("use mysql; select 'sabracadabra';");
  MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`.");
  MY_EXPECT_STDOUT_CONTAINS("sabracadabra");
  EXPECT_TRUE(output_handler.std_err.empty());
  output_handler.wipe_all();

  execute("USE information_schema;");
  MY_EXPECT_STDOUT_CONTAINS("Default schema set to `information_schema`.");
  EXPECT_TRUE(output_handler.std_err.empty());
  output_handler.wipe_all();

  execute("Use mysql;");
  MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`.");
  EXPECT_TRUE(output_handler.std_err.empty());
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_sql_use) {
  _options->interactive = true;
  _options->db_name_cache = true;
  reset_shell();
  // check that SQL use command is overriden and will trigger
  // internal actions, like updating the completion cache and setting the db
  // variable
  execute("\\sql");
  output_handler.wipe_all();

  execute("use mysql");
  MY_EXPECT_STDERR_CONTAINS("Not connected.");
  output_handler.wipe_all();

  execute("use mysql;");
  MY_EXPECT_STDERR_CONTAINS("Not connected.");
  output_handler.wipe_all();

  execute("\\connect " + _uri);
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("use mysql");
  MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`");
  MY_EXPECT_STDOUT_CONTAINS(
      "Fetching table and column names from `mysql` for auto-completion...");
  output_handler.wipe_all();

  execute("use mysql;");
  MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`");
  MY_EXPECT_STDOUT_CONTAINS(
      "Fetching table and column names from `mysql` for auto-completion...");
  output_handler.wipe_all();

  execute(to_scripting);

  output_handler.wipe_all();
  execute("db");
  EXPECT_STREQ("<Schema:mysql>\n", output_handler.std_out.c_str());
  execute("\\sql");
  output_handler.wipe_all();

  execute("use unexisting");
  MY_EXPECT_STDERR_CONTAINS("Unknown database 'unexisting'");
  output_handler.wipe_all();

  execute("use unexisting;");
  MY_EXPECT_STDERR_CONTAINS("Unknown database 'unexisting'");
  output_handler.wipe_all();

  execute("\\connect --mx " + _uri);
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("use mysql");
  MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`");
  output_handler.wipe_all();

  execute("\\connect --mc " + _mysql_uri);
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("use mysql");
  MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`.");
  MY_EXPECT_STDOUT_CONTAINS(
      "Fetching table and column names from `mysql` for auto-completion...");
  output_handler.wipe_all();
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

TEST_F(Interactive_shell_test, shell_empty_source_command) {
  _interactive_shell->process_line("\\source");
  MY_EXPECT_STDERR_CONTAINS("Filename not specified");
  output_handler.wipe_all();
}

#ifdef HAVE_V8
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
#endif

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

#ifdef HAVE_V8
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
#endif

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

  execute(to_scripting);
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
  execute("ALTER USER expired@'%' IDENTIFIED BY 'updated';");
  MY_EXPECT_STDOUT_CONTAINS("Query OK, 0 rows affected");
  output_handler.wipe_all();

  execute("select host from mysql.user where user = 'expired';");
  MY_EXPECT_STDOUT_CONTAINS("1 row in set");
  output_handler.wipe_all();

  execute(to_scripting);
  execute("session.close()");
  execute("\\sql");

  execute("\\c " + _mysql_uri);
  execute("drop user if exists expired;");
  execute(to_scripting);
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

  execute(to_scripting);
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
  execute("ALTER USER expired@'%' IDENTIFIED BY 'updated';");
  MY_EXPECT_STDOUT_CONTAINS("Query OK, 0 rows affected");
  output_handler.wipe_all();

  execute("select host from mysql.user where user = 'expired';");
  MY_EXPECT_STDOUT_CONTAINS("1 row in set");
  output_handler.wipe_all();

  execute(to_scripting);
  execute("session.close()");
  execute("\\sql");

  execute("\\c " + _uri);
  execute("drop user if exists expired;");
  execute(to_scripting);
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
  MY_EXPECT_MULTILINE_OUTPUT(
      "test zerofill",
      multiline({
      "+---+-------+--------+----------+-------+-------+-------+----------------------+------------------------+",
      "| a | b     | c      | d        | e     | f     | ggggg | h                    | i                      |",
      "+---+-------+--------+----------+-------+-------+-------+----------------------+------------------------+",
      "| 1 | one   |    -42 | 00000042 |    42 | 00042 |   042 | 00000000000000000042 | 0000000000000000000042 |",
      "| 2 | two   | -12345 | 00012345 | 12345 | 12345 |   123 | 00000000000000012345 | 0000000000000000012345 |",
      "| 3 | three |      0 | 00000000 |    00 | 00000 |   000 | 00000000000000000000 | 0000000000000000000000 |",
      "| 4 | four  |   NULL |     NULL |  NULL |  NULL |  NULL |                 NULL |                   NULL |",  // NOLINT
      "+---+-------+--------+----------+-------+-------+-------+----------------------+------------------------+",  // NOLINT
      }), output_handler.std_out.c_str());
  // clang-format on
  execute(to_scripting);
  execute("shell.options['resultFormat']='vertical'");
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

  execute(to_scripting);
  execute("shell.options['resultFormat']='json'");
  execute("\\sql");
  wipe_all();

  execute("select * from itst.tbl;");
  MY_EXPECT_STDOUT_CONTAINS(
      "{\n"
      "    \"a\": 1,\n"
      "    \"b\": \"one\",\n"
      "    \"c\": -42,\n"
      "    \"d\": 42,\n"
      "    \"e\": 42,\n"
      "    \"f\": 42,\n"
      "    \"ggggg\": 42,\n"
      "    \"h\": 42,\n"
      "    \"i\": 42\n"
      "}\n"
      "{\n"
      "    \"a\": 2,\n"
      "    \"b\": \"two\",\n"
      "    \"c\": -12345,\n"
      "    \"d\": 12345,\n"
      "    \"e\": 12345,\n"
      "    \"f\": 12345,\n"
      "    \"ggggg\": 123,\n"
      "    \"h\": 12345,\n"
      "    \"i\": 12345\n"
      "}\n"
      "{\n"
      "    \"a\": 3,\n"
      "    \"b\": \"three\",\n"
      "    \"c\": 0,\n"
      "    \"d\": 0,\n"
      "    \"e\": 0,\n"
      "    \"f\": 0,\n"
      "    \"ggggg\": 0,\n"
      "    \"h\": 0,\n"
      "    \"i\": 0\n"
      "}\n"
      "{\n"
      "    \"a\": 4,\n"
      "    \"b\": \"four\",\n"
      "    \"c\": null,\n"
      "    \"d\": null,\n"
      "    \"e\": null,\n"
      "    \"f\": null,\n"
      "    \"ggggg\": null,\n"
      "    \"h\": null,\n"
      "    \"i\": null\n"
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
      "+---+-------+--------+----------+-------+-------+-------+----------------------+-------+\n");
  // clang-format on
  execute(to_scripting);
  execute("shell.options['resultFormat']='vertical'");
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

  execute(to_scripting);
  execute("shell.options['resultFormat']='json'");
  execute("\\sql");
  wipe_all();

  execute("select * from itst.tbl;");
  MY_EXPECT_STDOUT_CONTAINS(
      "{\n"
      "    \"a\": 1,\n"
      "    \"b\": \"one\",\n"
      "    \"c\": -42,\n"
      "    \"d\": 42,\n"
      "    \"e\": 42,\n"
      "    \"f\": 42,\n"
      "    \"ggggg\": 42,\n"
      "    \"h\": 42,\n"
      "    \"i\": 42\n"
      "}\n"
      "{\n"
      "    \"a\": 2,\n"
      "    \"b\": \"two\",\n"
      "    \"c\": -12345,\n"
      "    \"d\": 12345,\n"
      "    \"e\": 12345,\n"
      "    \"f\": 12345,\n"
      "    \"ggggg\": 123,\n"
      "    \"h\": 12345,\n"
      "    \"i\": 12345\n"
      "}\n"
      "{\n"
      "    \"a\": 3,\n"
      "    \"b\": \"three\",\n"
      "    \"c\": 0,\n"
      "    \"d\": 0,\n"
      "    \"e\": 0,\n"
      "    \"f\": 0,\n"
      "    \"ggggg\": 0,\n"
      "    \"h\": 0,\n"
      "    \"i\": 0\n"
      "}\n"
      "{\n"
      "    \"a\": 4,\n"
      "    \"b\": \"four\",\n"
      "    \"c\": null,\n"
      "    \"d\": null,\n"
      "    \"e\": null,\n"
      "    \"f\": null,\n"
      "    \"ggggg\": null,\n"
      "    \"h\": null,\n"
      "    \"i\": null\n"
      "}\n");

  execute("drop schema itst;");
}

TEST_F(Interactive_shell_test, ssl_status) {
  if (g_target_server_version == mysqlshdk::utils::Version(8, 0, 4)) {
    PENDING_BUG_TEST("Caching_sha2 with xproto and no ssl is broken");
  } else {
    wipe_all();
    execute("\\connect " + _uri + "?ssl-Mode=DISABLED");
    execute("\\status");
    MY_EXPECT_STDOUT_CONTAINS("Not in use.");
    EXPECT_EQ("", _interactive_shell->prompt_variables()->at("ssl"));
  }
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
  if (g_target_server_version == mysqlshdk::utils::Version(8, 0, 4)) {
    PENDING_BUG_TEST("Caching_sha2 with xproto and no ssl is broken");
    return;
  }

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
  EXPECT_EQ(1, sscanf(line.c_str(), "TCP port:                     %d", &dec));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Server characterset"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Schema characterset"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Client characterset"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Conn. characterset"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Uptime"));
  EXPECT_NE(std::string::npos, line.find("sec"));
}

TEST_F(Interactive_shell_test, status_classic) {
  execute("\\connect " + _mysql_uri + "?ssl-Mode=DISABLED&compression=1");
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
  EXPECT_EQ(1, sscanf(line.c_str(), "TCP port:                     %d", &dec));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Server characterset"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Schema characterset"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Client characterset"));

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  ASSERT_NE(line.find("Conn. characterset"), std::string::npos);

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  ASSERT_NE(line.find("Compression"), std::string::npos);
  ASSERT_NE(line.find("Enabled"), std::string::npos);

  ASSERT_TRUE(static_cast<bool>(std::getline(ss, line)));
  EXPECT_NE(std::string::npos, line.find("Uptime"));
  EXPECT_NE(std::string::npos, line.find("sec"));
}

TEST_F(Interactive_shell_test, reconnect_command) {
  execute("\\reconnect");
  MY_EXPECT_STDERR_CONTAINS("enable reconnection");
  wipe_all();

  execute("\\reconnect dummy_arg");
  MY_EXPECT_STDERR_CONTAINS("not accept");
  wipe_all();

  execute("\\connect " + _mysql_uri);
  ASSERT_TRUE(output_handler.std_err.empty());
  wipe_all();

  execute("\\reconnect");
  MY_EXPECT_STDOUT_CONTAINS("successfully reconnected");
  wipe_all();

  execute("\\sql");
  execute("kill CONNECTION_ID();");
  MY_EXPECT_STDERR_CONTAINS("interrupted");
  wipe_all();

  execute("\\reconnect");
  MY_EXPECT_STDOUT_CONTAINS("successfully reconnected");
  wipe_all();

  // connect to mysql schema
  execute("\\connect " + _mysql_uri + "/mysql");
  MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`");
  ASSERT_TRUE(output_handler.std_err.empty());
  wipe_all();

  execute("\\use information_schema");
  ASSERT_TRUE(output_handler.std_err.empty());
  wipe_all();

  // check current schema
  execute("\\sql select database();");
  MY_EXPECT_STDOUT_CONTAINS("information_schema");
  wipe_all();

  execute("\\reconnect");
  MY_EXPECT_STDOUT_CONTAINS("successfully reconnected");
  ASSERT_TRUE(output_handler.std_err.empty());
  wipe_all();

  // check if schema was retained
  execute("\\sql select database();");
  MY_EXPECT_STDOUT_CONTAINS("information_schema");
  wipe_all();

  // check reconnect when previous default schema was dropped.
  execute("\\sql");
  execute("CREATE DATABASE IF NOT EXISTS dropped_schema_reconnect_test;");
  execute("\\use dropped_schema_reconnect_test");
  execute("DROP DATABASE dropped_schema_reconnect_test;");
  ASSERT_TRUE(output_handler.std_err.empty());
  wipe_all();
  execute("\\reconnect");
  MY_EXPECT_STDOUT_CONTAINS(
      "WARNING: Unable to reconnect to default schema "
      "'dropped_schema_reconnect_test'");
  MY_EXPECT_STDOUT_CONTAINS("Ignoring schema, attempting to reconnect to");
  MY_EXPECT_STDOUT_CONTAINS("successfully reconnected");
  ASSERT_TRUE(output_handler.std_err.empty());
  wipe_all();
}

TEST_F(Interactive_shell_test, mod_shell_options) {
  execute("\\py");
  ASSERT_TRUE(_options->wizards);
  execute("shell.options[\"useWizards\"]");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_NE(std::string::npos, output_handler.std_out.find("true"));

  execute("shell.options[\"useWizards\"] = False");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_FALSE(_options->wizards);
  _options->wizards = true;
  wipe_all();

  execute("shell.options.set_persist(\"showWarnings\", False)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_FALSE(_options->show_warnings);
  wipe_all();

  execute("shell.options.unset(\"showWarnings\");");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_TRUE(_options->show_warnings);

  execute("shell.options.unset_persist(\"showWarnings\");");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_TRUE(_options->show_warnings);

#ifdef HAVE_V8
  execute("\\js");
  wipe_all();

  execute("shell.options");
  auto named_opts = _opts->get_named_options();
  for (const auto &name : named_opts) {
    MY_EXPECT_STDOUT_CONTAINS("\"" + name + "\":");
  }
  wipe_all();

  execute("shell.options.resultFormat = 'json'");
  execute("shell.options");
  for (const auto &name : named_opts) {
    MY_EXPECT_STDOUT_CONTAINS("\"" + name + "\":");
  }
  reset_options(0, nullptr, false);
  reset_shell();
  wipe_all();

  EXPECT_TRUE(_options->show_warnings);
  execute("shell.options.showWarnings=false");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_FALSE(_options->show_warnings);

  execute("shell.options.set(\"showWarnings\", true)");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_TRUE(_options->show_warnings);

  reset_options(0, nullptr, false);
  reset_shell();
  EXPECT_TRUE(_options->show_warnings);
  wipe_all();

  execute("shell.options.setPersist(\"showWarnings\", false)");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_FALSE(_options->show_warnings);

  reset_options(0, nullptr, false);
  reset_shell();
  EXPECT_FALSE(_options->show_warnings);
  wipe_all();

  execute("shell.options.unset(\"showWarnings\");");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_TRUE(_options->show_warnings);
  reset_options(0, nullptr, false);
  reset_shell();
  EXPECT_FALSE(_options->show_warnings);

  execute("shell.options.unsetPersist(\"showWarnings\");");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_TRUE(_options->show_warnings);
  reset_options(0, nullptr, false);
  reset_shell();
  EXPECT_TRUE(_options->show_warnings);

  wipe_all();
#endif
  reset_options();
  reset_shell();
}

TEST_F(Interactive_shell_test, option_command) {
  wipe_all();
  execute("\\option");
  MY_EXPECT_STDERR_CONTAINS("Allows working with the available shell options.");
  wipe_all();

  auto named_opts = _opts->get_named_options();
  execute("\\option --list");
  for (const auto &name : named_opts) {
    MY_EXPECT_STDOUT_CONTAINS(name);
  }
  MY_EXPECT_STDOUT_NOT_CONTAINS(
      to_string(shcore::opts::Source::Compiled_default));
  wipe_all();

  execute("\\option -l --show-origin");
  for (const auto &name : named_opts) {
    MY_EXPECT_STDOUT_CONTAINS(name);
  }
  MY_EXPECT_STDOUT_CONTAINS(to_string(shcore::opts::Source::Compiled_default));
  wipe_all();

  execute("\\option -h");
  for (const auto &name : named_opts) {
    MY_EXPECT_STDOUT_CONTAINS(name);
  }
  wipe_all();

  execute("\\option -h " SHCORE_USE_WIZARDS);
  MY_EXPECT_STDOUT_CONTAINS(SHCORE_USE_WIZARDS);
  MY_EXPECT_STDOUT_NOT_CONTAINS(SHCORE_HISTIGNORE);
  wipe_all();

  execute("\\option -h zzzzz");
  MY_EXPECT_STDERR_CONTAINS("No help found for filter: zzzzz");
  wipe_all();

  for (const auto &name : named_opts) {
    execute("\\option " + name);
    MY_EXPECT_STDOUT_CONTAINS(_opts->get_value_as_string(name));
    wipe_all();
  }

  execute("\\option zzzzz");
  MY_EXPECT_STDERR_CONTAINS("Unrecognized option: zzzzz");
  wipe_all();

  execute("\\option zzzzz=val");
  MY_EXPECT_STDERR_CONTAINS("Unrecognized option: zzzzz");
  wipe_all();

  execute("\\option zzzzz 1");
  MY_EXPECT_STDERR_CONTAINS("Unrecognized option: zzzzz");
  wipe_all();

  ASSERT_TRUE(_options->wizards);
  execute("\\option " SHCORE_USE_WIZARDS " false");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_FALSE(_options->wizards);
  wipe_all();

  execute("\\option -l --show-origin");
  MY_EXPECT_STDOUT_CONTAINS(to_string(shcore::opts::Source::User));
  wipe_all();

  execute("\\option --persist " SHCORE_USE_WIZARDS "=true");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_TRUE(_options->wizards);
  wipe_all();

  const char *args[] = {"ut", "--show-warnings=false"};
  reset_options(2, args, false);
  reset_shell();
  EXPECT_TRUE(_options->wizards);
  execute("\\option -l --show-origin");
  MY_EXPECT_STDOUT_CONTAINS(
      to_string(shcore::opts::Source::Configuration_file));
  MY_EXPECT_STDOUT_CONTAINS(to_string(shcore::opts::Source::Command_line));
  wipe_all();

  execute("\\option " SHCORE_USE_WIZARDS " =false");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_FALSE(_options->wizards);
  wipe_all();

  execute("\\option " SHCORE_USE_WIZARDS "= false");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_FALSE(_options->wizards);
  wipe_all();

  execute("\\option " SHCORE_USE_WIZARDS " dummy");
  EXPECT_FALSE(output_handler.std_err.empty());
  EXPECT_FALSE(_options->wizards);
  wipe_all();

  execute("\\option --unset --persist " SHCORE_USE_WIZARDS);
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_TRUE(_options->wizards);
  wipe_all();

  execute("\\option " SHCORE_USE_WIZARDS " = false");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_FALSE(_options->wizards);
  wipe_all();

  EXPECT_EQ("", _options->pager);
  execute("\\option " SHCORE_PAGER " = less -E");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_EQ("less -E", _options->pager);
  wipe_all();

  execute("\\option " SHCORE_PAGER " =more -E");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_EQ("more -E", _options->pager);
  wipe_all();

  execute("\\option " SHCORE_PAGER "= less -E");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_EQ("less -E", _options->pager);
  wipe_all();

  execute("\\option " SHCORE_PAGER "=more -E");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_EQ("more -E", _options->pager);
  wipe_all();

  execute("\\option " SHCORE_PAGER " less -E");
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_EQ("less -E", _options->pager);
  wipe_all();

  execute("\\option --unset " SHCORE_PAGER);
  EXPECT_TRUE(output_handler.std_err.empty());
  EXPECT_EQ("", _options->pager);
  wipe_all();

  reset_options(0, nullptr, false);
  reset_shell();
  EXPECT_TRUE(_options->wizards);
  execute("\\option -l --show-origin");
  MY_EXPECT_STDOUT_NOT_CONTAINS(
      to_string(shcore::opts::Source::Configuration_file));
  wipe_all();

  execute("\\option dummy_variable=5");
  EXPECT_FALSE(output_handler.std_err.empty());
  wipe_all();

  // value already unsaved
  execute("\\option -u " SHCORE_USE_WIZARDS);
  EXPECT_FALSE(output_handler.std_err.empty());
  wipe_all();
  reset_options();
  reset_shell();
}

TEST_F(Interactive_shell_test, bug_28240437) {
  static constexpr auto select_output = R"(+---+
| 1 |
+---+
| 1 |
+---+
1 row in set)";

  wipe_all();
  execute("\\sql");
  MY_EXPECT_STDOUT_CONTAINS("Switching to SQL mode... Commands end with ;");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();

  execute("select 1;");
  EXPECT_EQ("", output_handler.std_out);
  MY_EXPECT_STDERR_CONTAINS("Not connected.");
  wipe_all();

  execute("\\connect " + _mysql_uri);
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();

  execute("select 1;");
  MY_EXPECT_STDOUT_CONTAINS(select_output);
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();

  execute(to_scripting);
#ifdef HAVE_V8
  MY_EXPECT_STDOUT_CONTAINS("Switching to JavaScript mode...");
#else
  MY_EXPECT_STDOUT_CONTAINS("Switching to Python mode...");
#endif
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();

  execute("session.close();");
  EXPECT_EQ("", output_handler.std_out);
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();

  execute("\\sql");
  MY_EXPECT_STDOUT_CONTAINS("Switching to SQL mode... Commands end with ;");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();

  execute("select 1;");
  EXPECT_EQ("", output_handler.std_out);
  MY_EXPECT_STDERR_CONTAINS("Not connected.");
  wipe_all();
}

TEST_F(Interactive_shell_test, invalid_command) {
  wipe_all();
  execute("\\js");
  wipe_all();
  execute("\\invalid");
  MY_EXPECT_STDERR_CONTAINS("Unknown command: '\\invalid'");

  wipe_all();
  execute("\\py");
  wipe_all();
  execute("\\invalid");
  MY_EXPECT_STDERR_CONTAINS("Unknown command: '\\invalid'");

  wipe_all();
  execute("\\sql");
  wipe_all();
  execute("\\invalid");
  MY_EXPECT_STDERR_CONTAINS("Unknown command: '\\invalid'");

  wipe_all();
}

TEST_F(Interactive_shell_test, multi_line_command) {
#ifdef HAVE_V8
  wipe_all();
  execute("\\js");
  wipe_all();
  execute("\\");
  MY_EXPECT_STDERR_CONTAINS("SyntaxError: Invalid or unexpected token");
#endif

  wipe_all();
  execute("\\py");
  wipe_all();
  execute("\\");
  EXPECT_TRUE(output_handler.std_err.empty());
  execute("print(1 + 2)");
  execute("");
  MY_EXPECT_STDOUT_CONTAINS("3");

  wipe_all();
  execute("\\sql");
  wipe_all();
  execute("\\");
  EXPECT_TRUE(output_handler.std_err.empty());
  execute("select 1;");
  execute("");
  MY_EXPECT_STDERR_CONTAINS("ERROR: Not connected.");

  wipe_all();
}

TEST_F(Interactive_shell_test, pager_command) {
  // get the initial pager
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  const auto default_pager = shcore::str_strip(output_handler.std_out);
  wipe_all();

  // set pager to a command enclosed in quotes, they should be removed
  execute("\\pager \"more\"");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ("more", shcore::str_strip(output_handler.std_out));
  wipe_all();

  // reset pager to the initial value using command with no arguments
  execute("\\pager");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ(default_pager, shcore::str_strip(output_handler.std_out));
  wipe_all();

  // set pager to a command
  execute("\\pager more");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ("more", shcore::str_strip(output_handler.std_out));
  wipe_all();

  // reset pager to the initial value using command with an empty argument
  execute("\\pager \"\"");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ(default_pager, shcore::str_strip(output_handler.std_out));
  wipe_all();

  // set pager to a command with argument, both enclosed in quotes
  execute("\\pager \"more -10\"");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ("more -10", shcore::str_strip(output_handler.std_out));
  wipe_all();

  // set pager to a command with argument, the first one enclosed in quotes
  execute("\\pager \"more\" -10");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ("more -10", shcore::str_strip(output_handler.std_out));
  wipe_all();

  // set pager to a command with argument, the second one enclosed in quotes
  execute("\\pager more \"-10\"");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ("more -10", shcore::str_strip(output_handler.std_out));
  wipe_all();

  // set pager to a command with argument without using quotes
  execute("\\pager more -10");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ("more -10", shcore::str_strip(output_handler.std_out));
  wipe_all();

  // set pager to a command with argument which is enclosed in quotes
  execute("\\pager \"some_cmd print \\\"hello\\\"\"");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ("some_cmd print \"hello\"",
            shcore::str_strip(output_handler.std_out));
  wipe_all();

  reset_options();
}

TEST_F(Interactive_shell_test, pager_short_command) {
  // get the initial pager
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  const auto default_pager = shcore::str_strip(output_handler.std_out);
  wipe_all();

  // set pager to a command enclosed in quotes, they should be removed
  execute("\\P \"more\"");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ("more", shcore::str_strip(output_handler.std_out));
  wipe_all();

  // reset pager to the initial value using command with no arguments
  execute("\\P");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ(default_pager, shcore::str_strip(output_handler.std_out));
  wipe_all();

  // set pager to a command
  execute("\\P more");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ("more", shcore::str_strip(output_handler.std_out));
  wipe_all();

  // reset pager to the initial value using command with an empty argument
  execute("\\P \"\"");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ(default_pager, shcore::str_strip(output_handler.std_out));
  wipe_all();

  // set pager to a command with argument, both enclosed in quotes
  execute("\\P \"more -10\"");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ("more -10", shcore::str_strip(output_handler.std_out));
  wipe_all();

  // set pager to a command with argument, the first one enclosed in quotes
  execute("\\P \"more\" -10");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ("more -10", shcore::str_strip(output_handler.std_out));
  wipe_all();

  // set pager to a command with argument, the second one enclosed in quotes
  execute("\\P more \"-10\"");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ("more -10", shcore::str_strip(output_handler.std_out));
  wipe_all();

  // set pager to a command with argument without using quotes
  execute("\\pager more -10");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ("more -10", shcore::str_strip(output_handler.std_out));
  wipe_all();

  // set pager to a command with argument which is enclosed in quotes
  execute("\\P \"some_cmd print \\\"hello\\\"\"");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ("some_cmd print \"hello\"",
            shcore::str_strip(output_handler.std_out));
  wipe_all();

  reset_options();
}

TEST_F(Interactive_shell_test, nopager_command) {
  // set pager to sample command
  execute("\\pager more -10");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ("more -10", shcore::str_strip(output_handler.std_out));
  wipe_all();

  // reset pager to an empty string
  execute("\\nopager");
  EXPECT_EQ("", output_handler.std_err);
  wipe_all();
  execute("print(shell.options.pager)");
  EXPECT_EQ("", output_handler.std_err);
  EXPECT_EQ("", shcore::str_strip(output_handler.std_out));
  wipe_all();

  static constexpr auto expected_error = R"(NAME
      \nopager - Disables the current pager.

SYNTAX
      \nopager)";

  // nopager does not take any arguments, this should be an error
  execute("\\nopager more");
  EXPECT_EQ(expected_error, output_handler.std_err);
  EXPECT_EQ("", output_handler.std_out);
  wipe_all();

  reset_options();
}

TEST_F(Interactive_shell_test, inline_commands) {
  execute("\\sql");
  execute("\\connect " + _uri);
  wipe_all();

  // ensure behaviour of inline \commands match old cli
  execute(";;");
  EXPECT_EQ("", output_handler.std_out);
  EXPECT_EQ("ERROR: 1065: Query was empty\nERROR: 1065: Query was empty\n",
            output_handler.std_err);

  wipe_all();
  execute("select 1\\w");
  EXPECT_NE("",
            _interactive_shell->shell_context()->get_continued_input_context());
  execute("\\g");
  EXPECT_EQ("",
            _interactive_shell->shell_context()->get_continued_input_context());
  MY_EXPECT_STDOUT_CONTAINS(
      "Show warnings disabled.\n"
      "+---+\n"
      "| 1 |\n"
      "+---+\n"
      "| 1 |\n"
      "+---+\n");

  wipe_all();
  execute("select 1\\w\\G");
  EXPECT_EQ("",
            _interactive_shell->shell_context()->get_continued_input_context());

  MY_EXPECT_STDOUT_CONTAINS(
      "Show warnings disabled.\n"
      "*************************** 1. row ***************************\n"
      "1: 1\n"
      "1 row in set");

  wipe_all();
  execute("select 1\\W\\w\\W;");
  MY_EXPECT_STDOUT_CONTAINS(
      "Show warnings enabled.\n"
      "Show warnings disabled.\n"
      "Show warnings enabled.\n"
      "+---+\n"
      "| 1 |\n"
      "+---+\n"
      "| 1 |\n"
      "+---+\n"
      "1 row in set");
  EXPECT_EQ("",
            _interactive_shell->shell_context()->get_continued_input_context());

  wipe_all();
  execute("\\W");
  EXPECT_EQ("",
            _interactive_shell->shell_context()->get_continued_input_context());
  MY_EXPECT_STDOUT_CONTAINS("Show warnings enabled.");

  wipe_all();
  // \ at the beginning of stmt means a full line command
  execute("\\W\\w\\W");
  EXPECT_EQ("",
            _interactive_shell->shell_context()->get_continued_input_context());
  MY_EXPECT_STDERR_CONTAINS("Unknown command: '\\W\\w\\W'");

  // \ after stmt means single letter escape commands
  wipe_all();
  execute(";\\W\\w\\W");
  EXPECT_EQ("",
            _interactive_shell->shell_context()->get_continued_input_context());
  MY_EXPECT_STDOUT_CONTAINS(
      "Show warnings enabled.\n"
      "Show warnings disabled.\n"
      "Show warnings enabled.");

  wipe_all();
  execute("\\l");  // invalid cmd
  EXPECT_EQ("",
            _interactive_shell->shell_context()->get_continued_input_context());
  EXPECT_EQ("", output_handler.std_out);
  EXPECT_EQ("Unknown command: '\\l'", output_handler.std_err);

  // invalid delimiters
  wipe_all();
  execute("delimiter");
  EXPECT_EQ(
      "ERROR: DELIMITER must be followed by a 'delimiter' character or "
      "string\n",
      output_handler.std_out);

  wipe_all();
  execute("delimiter$");
  EXPECT_EQ(
      "ERROR: DELIMITER must be followed by a 'delimiter' character or "
      "string\n",
      output_handler.std_out);

  wipe_all();
  execute("delimiter \\x");
  EXPECT_EQ("ERROR: DELIMITER cannot contain a backslash character\n",
            output_handler.std_out);

  wipe_all();
  execute("select 1;");
  EXPECT_EQ("", output_handler.std_err);
}

// This tests is added for Bug #28894826
// EMPTY RESULTSET TABLES ARE PRINTED WHEN THERE ARE NO RESULTS TO DISPLAY
#ifdef HAVE_V8
TEST_F(Interactive_shell_test, not_empty_tables_js) {
  execute("\\connect " + _mysql_uri);
  wipe_all();
  execute("session.runSql('select 0 from dual where 0')");
  EXPECT_TRUE(
      shcore::str_beginswith(output_handler.std_out.c_str(), "Empty set"));
  execute("session.close()");
}

TEST_F(Interactive_shell_test, not_empty_collections_js) {
  execute("\\connect " + _uri);
  wipe_all();
  execute("var schema = session.createSchema('empty_collection')");
  execute("var collection = schema.createCollection('empty')");
  execute("collection.find()");
  EXPECT_TRUE(
      shcore::str_beginswith(output_handler.std_out.c_str(), "Empty set"));
  execute("session.dropSchema('empty_collection')");
  execute("session.close()");
}
#else
TEST_F(Interactive_shell_test, not_empty_tables_py) {
  execute("\\connect " + _mysql_uri);
  wipe_all();
  execute("session.run_sql('select 0 from dual where 0')");
  EXPECT_TRUE(
      shcore::str_beginswith(output_handler.std_out.c_str(), "Empty set"));
  execute("session.close()");
}

TEST_F(Interactive_shell_test, not_empty_collections_py) {
  execute("\\connect " + _uri);
  wipe_all();
  execute("schema = session.create_schema('empty_collection')");
  execute("collection = schema.create_collection('empty')");
  execute("collection.find()");
  EXPECT_TRUE(
      shcore::str_beginswith(output_handler.std_out.c_str(), "Empty set"));
  execute("session.drop_schema('empty_collection')");
  execute("session.close()");
}
#endif

TEST_F(Interactive_shell_test, compression) {
  ASSERT_FALSE(_options->default_compress);
  const auto check_compression = [this](const char *status) {
    execute("show session status like 'compression';");
    SCOPED_TRACE(output_handler.std_err.c_str());
    EXPECT_TRUE(output_handler.std_err.empty());
    MY_EXPECT_STDOUT_CONTAINS(status);
    execute("\\s");
    if (strcmp(status, "ON") == 0)
      MY_EXPECT_STDOUT_CONTAINS("Compression:                  Enabled");
    else if (output_handler.std_out.find("Compression:") != std::string::npos)
      MY_EXPECT_STDOUT_CONTAINS("Compression:                  Disabled");
    wipe_all();
  };

  execute("shell.connect(\"" + _mysql_uri + "?compression=true\");");
  execute("\\sql");
  check_compression("ON");

  execute("\\connect " + _mysql_uri + "?compression=true");
  check_compression("ON");

  execute("\\connect " + _mysql_uri);
  check_compression("OFF");

  execute("\\option defaultCompress=true");
  execute("\\connect " + _mysql_uri);
  check_compression("ON");
#ifdef HAVE_V8
  execute("\\js");
#else
  execute("\\py");
#endif
  execute("shell.connect(\"" + _mysql_uri + "\");");
  execute("\\sql");
  check_compression("ON");

  execute("\\option defaultCompress=false");
  execute("\\connect " + _mysql_uri);
  check_compression("OFF");

  execute("\\connect " + _uri + "?compression=true");
  MY_EXPECT_STDOUT_CONTAINS(
      "X Protocol: Compression is not supported and will be ignored.");
  check_compression("OFF");
  execute("\\js");
  wipe_all();
}

TEST_F(Interactive_shell_test, sql_command) {
#ifdef HAVE_V8
  execute("\\js");
#else
  execute("\\py");
#endif
  execute("\\sql show databases;");
  MY_EXPECT_STDERR_CONTAINS("ERROR: Not connected.");
  wipe_all();

  execute("\\connect " + _uri);

  execute("\\sql create database if not exists sqlcommandtest");
  MY_EXPECT_STDOUT_CONTAINS("Query OK, 1 row affected");
  wipe_all();

  execute(
      "\\sql create table if not exists `sqlcommandtest`.t1 (i integer, s "
      "varchar(20));");
  MY_EXPECT_STDOUT_CONTAINS("Query OK, 0 rows affected");
  wipe_all();

  execute("\\use sqlcommandtest");
  execute("\\sql insert into t1 values (0, \"foo\")");
  MY_EXPECT_STDOUT_CONTAINS("Query OK, 1 row affected");
  wipe_all();

  execute("\\sql select * from t1 where s=\"foo\";");
  MY_EXPECT_STDOUT_CONTAINS("1 row in set");
  wipe_all();

  execute("\\sql select * from t1 where s=\"foo\"\\G");
  MY_EXPECT_STDOUT_CONTAINS(
      "*************************** 1. row ***************************");
  wipe_all();

  execute("\\sql select d from t1;");
  MY_EXPECT_STDERR_CONTAINS("Unknown column 'd' in 'field list'");
  wipe_all();

  execute("    \\sql show tables");
  MY_EXPECT_STDOUT_CONTAINS("1 row in set");
  wipe_all();

  execute("\\sql show tables\\g");
  MY_EXPECT_STDOUT_CONTAINS("1 row in set");
  wipe_all();

  execute("\\sql drop database sqlcommandtest\\G");
  MY_EXPECT_STDOUT_CONTAINS("Query OK, 1 row affected");
  wipe_all();

  execute("\\sql show table");
  MY_EXPECT_STDERR_CONTAINS("You have an error in your SQL syntax");
  wipe_all();

  execute("\\sql show databases; show tables;");
  MY_EXPECT_STDERR_CONTAINS("You have an error in your SQL syntax");
  wipe_all();
}

TEST_F(Interactive_shell_test, ansi_quotes) {
  execute("\\sql");
  execute("\\connect " + _uri);
  execute("set @old_mode=@@sql_mode;");
  execute(R"(select "\"";select version();#";)");
  ASSERT_TRUE(output_handler.std_err.empty());
  wipe_all();

  for (const std::string s :
       {"set @@session.sql_mode='ANSI_QUOTES';",
        "set LOCAL sql_mode='ANSI_QUOTES';", "set sql_mode = 'ANSI_QUOTES';"}) {
    execute(s);
    EXPECT_TRUE(output_handler.std_err.empty());
    execute(R"(select "\"";select version();#";)");
    EXPECT_NE(std::string::npos,
              output_handler.std_err.find(
                  R"(Unknown column '\";select version();#')"));
    wipe_all();
    execute("set sql_mode=@old_mode;");
    execute(R"(select "\"";select version();#";)");
    EXPECT_TRUE(output_handler.std_err.empty());
  }
}

TEST_F(Interactive_shell_test, sql_source_cmd_after_delimeter) {
  std::string file_name = "f.sql";
  if (!std::ifstream(file_name).good()) {
    std::ofstream f(file_name);
    f << "select 2;\n";
    f.close();
  }

  execute("\\sql");
  execute("\\connect " + _uri);
  execute("select 1; \\source " + file_name);
  int i = 0;
  size_t pos = 0;
  while ((pos = output_handler.std_out.find("1 row in set", pos)) !=
         std::string::npos) {
    i++;
    pos++;
  }
  EXPECT_EQ(2, i);
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();

  shcore::delete_file(file_name);
}

TEST_F(Interactive_shell_test, tls_ciphersuites) {
  // Feature not yet supported on X protocol
  execute("shell.connect(\"" + _uri +
          "?tls-ciphersuites=[TLS_DHE_PSK_WITH_AES_128_GCM_SHA256,"
          "TLS_CHACHA20_POLY1305_SHA256]\");");
  MY_EXPECT_STDOUT_CONTAINS("tls-ciphersuites option is not yet supported");
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
  execute("shell.connect(\"" + _uri +
          "?tls-ciphersuites=TLS_DHE_PSK_WITH_AES_128_GCM_SHA256:"
          "TLS_CHACHA20_POLY1305_SHA256\");");
  MY_EXPECT_STDOUT_CONTAINS("tls-ciphersuites option is not yet supported");
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();

  // needed to force classic protocol to use TCP and SSL instead of unix sockets
  std::string classic_uri =
      shcore::str_replace(_mysql_uri, "localhost", "127.0.0.1");

  // We only check if there is no error since this feature requires TLSv1.3
  // which is not available on all the platforms e.g. Ubuntu 16.04
  execute("shell.connect(\"" + classic_uri +
          "?tls-ciphersuites=[TLS_DHE_PSK_WITH_AES_128_GCM_SHA256,"
          "TLS_CHACHA20_POLY1305_SHA256]\");");
  EXPECT_TRUE(output_handler.std_err.empty());
  MY_EXPECT_STDOUT_NOT_CONTAINS("tls-ciphersuites option is not yet supported");
  wipe_all();
  execute("shell.connect(\"" + classic_uri +
          "?tls-ciphersuites=TLS_DHE_PSK_WITH_AES_128_GCM_SHA256:"
          "TLS_CHACHA20_POLY1305_SHA256\");");
  EXPECT_TRUE(output_handler.std_err.empty());
  MY_EXPECT_STDOUT_NOT_CONTAINS("tls-ciphersuites option is not yet supported");
  wipe_all();
}

#ifdef HAVE_V8
TEST_F(Interactive_shell_test, js_interactive_multiline_comments) {
  execute("\\js");
  execute("print(1);/*");
  execute("print(2);");
  execute("print(3); */print(4)");
  EXPECT_EQ("14", output_handler.std_out);
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();

  execute("var f = function () { /*");
  execute("print(1);");
  execute("*/print(7)");
  execute("}");
  execute("");
  execute("f();");
  EXPECT_EQ("7", output_handler.std_out);
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();

  execute("print(1); /*/*");
  execute("*/print(2);");
  EXPECT_EQ("12", output_handler.std_out);
  EXPECT_TRUE(output_handler.std_err.empty());
  wipe_all();
}
#endif
}  // namespace mysqlsh
