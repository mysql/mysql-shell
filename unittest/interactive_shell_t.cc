/* Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.

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

#include "test_utils.h"

namespace shcore {
  namespace shell_core_tests {
    class Interactive_shell_test : public Shell_core_test_wrapper
    {
    public:
      virtual void set_options()
      {
        _options->interactive = true;
        _options->wizards = false;
      };
    };

    TEST_F(Interactive_shell_test, warning_insecure_password)
    {
      // Test secure call passing uri with no password (will be prompted)
      _options->uri = "root@localhost";
      reset_shell();
      output_handler.ret_pwd = "whatever";

      _interactive_shell->connect(true);
      MY_EXPECT_STDOUT_NOT_CONTAINS("mysqlx: [Warning] Using a password on the command line interface can be insecure.");
      output_handler.wipe_all();

      // Test non secure call passing uri and password with cmd line params
      _options->password = "whatever";
      reset_shell();

      _interactive_shell->connect(true);
      MY_EXPECT_STDOUT_CONTAINS("mysqlx: [Warning] Using a password on the command line interface can be insecure.");
      output_handler.wipe_all();

      // Test secure call passing uri with empty password
      reset_options();
      _options->uri = "root:@localhost";
      reset_shell();

      _interactive_shell->connect(true);
      MY_EXPECT_STDOUT_NOT_CONTAINS("mysqlx: [Warning] Using a password on the command line interface can be insecure.");
      output_handler.wipe_all();

      // Test non secure call passing uri with password
      _options->uri = "root:whatever@localhost";
      reset_shell();

      _interactive_shell->connect(true);
      MY_EXPECT_STDOUT_CONTAINS("mysqlx: [Warning] Using a password on the command line interface can be insecure.");
      output_handler.wipe_all();
    }

    TEST_F(Interactive_shell_test, shell_command_connect)
    {
      _interactive_shell->process_line("\\connect " + _uri);
      MY_EXPECT_STDOUT_CONTAINS("No default schema selected.");
      output_handler.wipe_all();

      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<XSession:" + _uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("db");
      EXPECT_STREQ("\n", output_handler.std_out.c_str());

      _interactive_shell->process_line("\\connect " + _uri + "/mysql");
      MY_EXPECT_STDOUT_CONTAINS("Default schema `mysql` accessible through db.");
      output_handler.wipe_all();

      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<XSession:" + _uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("db");
      MY_EXPECT_STDOUT_CONTAINS("<Schema:mysql>");
      output_handler.wipe_all();
    }

    TEST_F(Interactive_shell_test, shell_command_connect_node)
    {
      _interactive_shell->process_line("\\connect -n " + _uri);
      MY_EXPECT_STDOUT_CONTAINS("No default schema selected.");
      output_handler.wipe_all();

      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("db");
      EXPECT_STREQ("\n", output_handler.std_out.c_str());

      _interactive_shell->process_line("\\connect -n " + _uri + "/mysql");
      MY_EXPECT_STDOUT_CONTAINS("Default schema `mysql` accessible through db.");
      output_handler.wipe_all();

      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("db");
      MY_EXPECT_STDOUT_CONTAINS("<Schema:mysql>");
      output_handler.wipe_all();
    }

    TEST_F(Interactive_shell_test, shell_command_connect_classic)
    {
      _interactive_shell->process_line("\\connect -c " + _mysql_uri);
      MY_EXPECT_STDOUT_CONTAINS("No default schema selected.");
      output_handler.wipe_all();

      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("db");
      EXPECT_STREQ("\n", output_handler.std_out.c_str());

      _interactive_shell->process_line("\\connect -c " + _mysql_uri + "/mysql");
      MY_EXPECT_STDOUT_CONTAINS("Default schema `mysql` accessible through db.");
      output_handler.wipe_all();

      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("db");
      MY_EXPECT_STDOUT_CONTAINS("<ClassicSchema:mysql>");
      output_handler.wipe_all();
    }

    TEST_F(Interactive_shell_test, shell_command_use)
    {
      _interactive_shell->process_line("\\use mysql");
      MY_EXPECT_STDERR_CONTAINS("Not Connected.");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\connect " + _uri);
      MY_EXPECT_STDOUT_CONTAINS("No default schema selected.");
      output_handler.wipe_all();

      _interactive_shell->process_line("db");
      EXPECT_STREQ("\n", output_handler.std_out.c_str());
      output_handler.wipe_all();

      _interactive_shell->process_line("\\use mysql");
      MY_EXPECT_STDOUT_CONTAINS("Schema `mysql` accessible through db.");
      output_handler.wipe_all();

      _interactive_shell->process_line("db");
      EXPECT_STREQ("<Schema:mysql>\n", output_handler.std_out.c_str());
      output_handler.wipe_all();

      _interactive_shell->process_line("\\use unexisting");
      MY_EXPECT_STDERR_CONTAINS("RuntimeError: Unknown database 'unexisting'");
      output_handler.wipe_all();

      _interactive_shell->process_line("db");
      EXPECT_STREQ("<Schema:mysql>\n", output_handler.std_out.c_str());
      output_handler.wipe_all();

      _interactive_shell->process_line("\\connect -n " + _uri);
      MY_EXPECT_STDOUT_CONTAINS("No default schema selected.");
      output_handler.wipe_all();

      _interactive_shell->process_line("db");
      EXPECT_STREQ("\n", output_handler.std_out.c_str());
      output_handler.wipe_all();

      _interactive_shell->process_line("\\use mysql");
      MY_EXPECT_STDOUT_CONTAINS("Schema `mysql` accessible through db.");
      output_handler.wipe_all();

      _interactive_shell->process_line("db");
      EXPECT_STREQ("<Schema:mysql>\n", output_handler.std_out.c_str());
      output_handler.wipe_all();

      _interactive_shell->process_line("\\connect -c " + _mysql_uri);
      MY_EXPECT_STDOUT_CONTAINS("No default schema selected.");
      output_handler.wipe_all();

      _interactive_shell->process_line("db");
      EXPECT_STREQ("\n", output_handler.std_out.c_str());
      output_handler.wipe_all();

      _interactive_shell->process_line("\\use mysql");
      MY_EXPECT_STDOUT_CONTAINS("Schema `mysql` accessible through db.");
      output_handler.wipe_all();

      _interactive_shell->process_line("db");
      EXPECT_STREQ("<ClassicSchema:mysql>\n", output_handler.std_out.c_str());
      output_handler.wipe_all();
    }

    TEST_F(Interactive_shell_test, shell_command_warnings)
    {
      _options->interactive = true;
      reset_shell();
      _interactive_shell->process_line("\\warnings");
      MY_EXPECT_STDOUT_CONTAINS("Show warnings enabled.");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\W");
      MY_EXPECT_STDOUT_CONTAINS("Show warnings enabled.");
      output_handler.wipe_all();
    }

    TEST_F(Interactive_shell_test, shell_command_no_warnings)
    {
      _options->interactive = true;
      reset_shell();
      _interactive_shell->process_line("\\nowarnings");
      MY_EXPECT_STDOUT_CONTAINS("Show warnings disabled.");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\w");
      MY_EXPECT_STDOUT_CONTAINS("Show warnings disabled.");
      output_handler.wipe_all();
    }

    TEST_F(Interactive_shell_test, shell_command_store_connection)
    {
      // Cleanup for the test
      _interactive_shell->process_line("\\rmconn test_01");
      _interactive_shell->process_line("\\rmconn test_02");
      output_handler.wipe_all();

      // Command errors
      _interactive_shell->process_line("\\addc 1");
      MY_EXPECT_STDERR_CONTAINS("The session configuration name '1' is not a valid identifier");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\addc test_example");
      MY_EXPECT_STDERR_CONTAINS("Unable to save session information, no active session available");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\addc");
      MY_EXPECT_STDERR_CONTAINS("\\addconn [-f] <session_cfg_name> [<uri>]");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\addc -f");
      MY_EXPECT_STDERR_CONTAINS("\\addconn [-f] <session_cfg_name> [<uri>]");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\addc wrong params root@localhost");
      MY_EXPECT_STDERR_CONTAINS("\\addconn [-f] <session_cfg_name> [<uri>]");
      output_handler.wipe_all();

      // Passing URI
      _interactive_shell->process_line("\\addc test_01 sample:pwd@sometarget:45/schema");
      MY_EXPECT_STDOUT_CONTAINS("Successfully stored sample@sometarget:45/schema as test_01.");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\addc test_01 sample2:pwd@sometarget:46");
      MY_EXPECT_STDERR_CONTAINS("ShellRegistry.add: The name 'test_01' already exists");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\addc -f test_01 sample2:pwd@sometarget:46");
      MY_EXPECT_STDOUT_CONTAINS("Successfully stored sample2@sometarget:46 as test_01.");
      output_handler.wipe_all();

      // Working with the current session
      _interactive_shell->process_line("\\connect " + _uri);
      _interactive_shell->process_line("\\addc test_02");
      MY_EXPECT_STDOUT_CONTAINS("Successfully stored " + _uri_nopasswd + ":33060 as test_02.");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\addc test_02");
      MY_EXPECT_STDERR_CONTAINS("ShellRegistry.add: The name 'test_02' already exists");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\addc -f test_02");
      MY_EXPECT_STDOUT_CONTAINS("Successfully stored " + _uri_nopasswd + ":33060 as test_02.");
      output_handler.wipe_all();
    }

    TEST_F(Interactive_shell_test, shell_command_update_connection)
    {
      // Cleanup for the test
      _interactive_shell->process_line("\\rmconn test_01");
      output_handler.wipe_all();

      // Command errors
      _interactive_shell->process_line("\\chconn");
      MY_EXPECT_STDERR_CONTAINS("\\chconn <session_cfg_name> <URI>");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\chconn test_01");
      MY_EXPECT_STDERR_CONTAINS("\\chconn <session_cfg_name> <URI>");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\chconn test_01 sample@whatever");
      MY_EXPECT_STDERR_CONTAINS("ShellRegistry.update: The name 'test_01' does not exist");
      output_handler.wipe_all();

      // Passing URI
      _interactive_shell->process_line("\\addc test_01 sample:pwd@sometarget:45/schema");
      output_handler.wipe_all();
      _interactive_shell->process_line("\\chconn test_01 sample2:pwd@sometarget:46");
      MY_EXPECT_STDOUT_CONTAINS("Successfully updated test_01 to sample2@sometarget:46.");
      output_handler.wipe_all();
    }

    TEST_F(Interactive_shell_test, shell_command_delete_connection)
    {
      // Cleanup for the test
      _interactive_shell->process_line("\\rmconn test_01");
      _interactive_shell->process_line("\\rmconn test_02");
      _interactive_shell->process_line("\\addconn test_01 sample@host:port");
      output_handler.wipe_all();

      // Command errors
      _interactive_shell->process_line("\\rmconn");
      MY_EXPECT_STDERR_CONTAINS("\\rmconn <session_cfg_name>");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\rmconn test_02");
      MY_EXPECT_STDERR_CONTAINS("The name 'test_02' does not exist");
      output_handler.wipe_all();

      // Passing URI
      _interactive_shell->process_line("\\rmconn test_01");
      MY_EXPECT_STDOUT_CONTAINS("Successfully deleted session configuration named test_01.");
      output_handler.wipe_all();
    }

    TEST_F(Interactive_shell_test, shell_command_list_connections)
    {
      // Cleanup for the test
      _interactive_shell->process_line("\\rmconn test_01");
      _interactive_shell->process_line("\\rmconn test_02");
      _interactive_shell->process_line("\\addconn test_01 sample:pwd@host:4520");
      output_handler.wipe_all();

      // Command errors
      _interactive_shell->process_line("\\lsc test_01");
      MY_EXPECT_STDERR_CONTAINS("\\lsconn");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\lsconn");
      MY_EXPECT_STDOUT_CONTAINS("test_01 : sample@host:4520");
      output_handler.wipe_all();
    }

    TEST_F(Interactive_shell_test, shell_command_help)
    {
      // Cleanup for the test
      _interactive_shell->process_line("\\?");
      MY_EXPECT_STDOUT_CONTAINS("===== Global Commands =====");
      MY_EXPECT_STDOUT_CONTAINS("\\help       (\\?,\\h)    Print this help.");
      MY_EXPECT_STDOUT_CONTAINS("\\sql                   Sets shell on SQL processing mode.");
      MY_EXPECT_STDOUT_CONTAINS("\\js                    Sets shell on JavaScript processing mode.");
      MY_EXPECT_STDOUT_CONTAINS("\\py                    Sets shell on Python processing mode.");
      MY_EXPECT_STDOUT_CONTAINS("\\source     (\\.)       Execute a script file. Takes a file name as an argument.");
      MY_EXPECT_STDOUT_CONTAINS("\\                      Start multiline input when in SQL mode.");
      MY_EXPECT_STDOUT_CONTAINS("\\quit       (\\q,\\exit) Quit mysh.");
      MY_EXPECT_STDOUT_CONTAINS("\\connect    (\\c)       Connect to server using an application mode session.");
      MY_EXPECT_STDOUT_CONTAINS("\\warnings   (\\W)       Show warnings after every statement.");
      MY_EXPECT_STDOUT_CONTAINS("\\nowarnings (\\w)       Don't show warnings after every statement.");
      MY_EXPECT_STDOUT_CONTAINS("\\status     (\\s)       Prints information about the current global connection.");
      MY_EXPECT_STDOUT_CONTAINS("\\use        (\\u)       Sets the current schema on the global session.");
      MY_EXPECT_STDOUT_CONTAINS("\\addconn    (\\addc)    Inserts/updates new/existing session configuration.");
      MY_EXPECT_STDOUT_CONTAINS("\\rmconn                Removes a stored session configuration.");
      MY_EXPECT_STDOUT_CONTAINS("\\lsconn     (\\lsc)     List the stored session configurations.");
      MY_EXPECT_STDOUT_CONTAINS("\\chconn                Updates a stored session configuration.");
      MY_EXPECT_STDOUT_CONTAINS("For help on a specific command use the command as \\? <command>");

      _interactive_shell->process_line("\\help \\source");
      MY_EXPECT_STDOUT_CONTAINS("NOTE: Can execute files from the supported types: SQL, Javascript, or Python.");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\help \\connect");
      MY_EXPECT_STDOUT_CONTAINS("If the session type is not specified, an X session will be established.");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\help \\use");
      MY_EXPECT_STDOUT_CONTAINS("The global db variable will be updated to hold the requested schema.");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\help \\addconn");
      MY_EXPECT_STDOUT_CONTAINS("SESSION_CONFIG_NAME is the name to be assigned to the session configuration.");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\help \\rmconn");
      MY_EXPECT_STDOUT_CONTAINS("SESSION_CONFIG_NAME is the name of session configuration to be deleted.");
      output_handler.wipe_all();

      _interactive_shell->process_line("\\help \\chconn");
      MY_EXPECT_STDOUT_CONTAINS("SESSION_CONFIG_NAME is the name of the session configuration to be updated.");
      output_handler.wipe_all();
    }

    TEST_F(Interactive_shell_test, js_db_usage_with_no_wizards)
    {
      _interactive_shell->process_line("db");
      MY_EXPECT_STDERR_CONTAINS("ReferenceError: db is not defined");
      output_handler.wipe_all();

      _interactive_shell->process_line("db.getName()");
      MY_EXPECT_STDERR_CONTAINS("ReferenceError: db is not defined");
      output_handler.wipe_all();

      _interactive_shell->process_line("db.name");
      MY_EXPECT_STDERR_CONTAINS("ReferenceError: db is not defined");
      output_handler.wipe_all();

      _options->uri = _uri + "/mysql";
      _options->interactive = true;
      reset_shell();
      _interactive_shell->connect(true);
      output_handler.wipe_all();

      _interactive_shell->process_line("db");
      MY_EXPECT_STDOUT_CONTAINS("<Schema:mysql>");
      output_handler.wipe_all();

      _interactive_shell->process_line("db.getName()");
      MY_EXPECT_STDOUT_CONTAINS("mysql");
      output_handler.wipe_all();

      _interactive_shell->process_line("db.name");
      MY_EXPECT_STDOUT_CONTAINS("mysql");
      output_handler.wipe_all();
    }

    TEST_F(Interactive_shell_test, js_session_usage_with_no_wizards)
    {
      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_NOT_CONTAINS("ReferenceError: session is not defined");
      output_handler.wipe_all();

      _interactive_shell->process_line("session.getUri()");
      MY_EXPECT_STDOUT_NOT_CONTAINS("ReferenceError: session is not defined");
      output_handler.wipe_all();

      _interactive_shell->process_line("session.uri");
      MY_EXPECT_STDOUT_NOT_CONTAINS("ReferenceError: session is not defined");
      output_handler.wipe_all();

      _options->uri = _uri + "/mysql";
      _options->interactive = true;
      reset_shell();
      _interactive_shell->connect(true);
      output_handler.wipe_all();

      _interactive_shell->process_line("session");
      MY_EXPECT_STDOUT_CONTAINS("<XSession:" + _uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("session.getUri()");
      MY_EXPECT_STDOUT_CONTAINS(_uri_nopasswd);
      output_handler.wipe_all();

      _interactive_shell->process_line("session.uri");
      MY_EXPECT_STDOUT_CONTAINS(_uri_nopasswd);
      output_handler.wipe_all();
    }
  }
}