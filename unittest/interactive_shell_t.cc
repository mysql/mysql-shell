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

#include <cstdio>
#include "test_utils.h"
#include "utils/utils_file.h"

namespace shcore {
namespace shell_core_tests {
class Interactive_shell_test : public Shell_core_test_wrapper {
 public:
  virtual void set_options() {
    _options->interactive = true;
    _options->wizards = true;
  }
};

TEST_F(Interactive_shell_test, warning_insecure_password) {
  // Test secure call passing uri with no password (will be prompted)
  _options->uri = "root@localhost";
  reset_shell();
  output_handler.passwords.push_back("whatever");

  _interactive_shell->connect(true);
  MY_EXPECT_STDOUT_NOT_CONTAINS(
      "mysqlx: [Warning] Using a password on the command line interface can be "
      "insecure.");
  output_handler.wipe_all();

  // Test non secure call passing uri and password with cmd line params
  _options->password = "whatever";
  reset_shell();

  _interactive_shell->connect(true);
  MY_EXPECT_STDOUT_CONTAINS(
      "mysqlx: [Warning] Using a password on the command line interface can be "
      "insecure.");
  output_handler.wipe_all();

  // Test secure call passing uri with empty password
  reset_options();
  _options->uri = "root:@localhost";
  reset_shell();

  _interactive_shell->connect(true);
  MY_EXPECT_STDOUT_NOT_CONTAINS(
      "mysqlx: [Warning] Using a password on the command line interface can be "
      "insecure.");
  output_handler.wipe_all();

  // Test non secure call passing uri with password
  _options->uri = "root:whatever@localhost";
  reset_shell();

  _interactive_shell->connect(true);
  MY_EXPECT_STDOUT_CONTAINS(
      "mysqlx: [Warning] Using a password on the command line interface can be "
      "insecure.");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_connect_node) {
  execute("\\connect -n " + _uri);
  MY_EXPECT_STDOUT_CONTAINS("Creating a Node Session to '" + _uri_nopasswd +
                            "'");
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  MY_EXPECT_STDOUT_CONTAINS("(X protocol)");
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("<Undefined>\n", output_handler.std_out.c_str());

  execute("session.close()");

  execute("\\connect -n " + _uri + "/mysql");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Node Session to '" + _uri_nopasswd +
                            "/mysql'");
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  MY_EXPECT_STDOUT_CONTAINS("(X protocol)");
  MY_EXPECT_STDOUT_CONTAINS("Default schema `mysql` accessible through db.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  MY_EXPECT_STDOUT_CONTAINS("<Schema:mysql>");
  output_handler.wipe_all();

  execute("session.close()");

  execute("\\connect -n mysql://" + _mysql_uri);
  MY_EXPECT_STDERR_CONTAINS("Invalid URI for Node session");
  output_handler.wipe_all();

  execute("\\connect -n " + _mysql_uri);
  MY_EXPECT_STDERR_CONTAINS("Requested session assumes MySQL X Protocol but '" +
                            _host + ":" + _mysql_port +
                            "' seems to speak the classic MySQL protocol");
  output_handler.wipe_all();

  // Invalid user/password
  output_handler.passwords.push_back("whatever");
  execute("\\connect -n " + _uri_nopasswd);
  MY_EXPECT_STDERR_CONTAINS("ERROR: 1045: Invalid user or password");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_connect_classic) {
  execute("\\connect -c " + _mysql_uri);
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic Session to '" +
                            _mysql_uri_nopasswd + "'");
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("<Undefined>\n", output_handler.std_out.c_str());

  execute("session.close()");

  execute("\\connect -c " + _mysql_uri + "/mysql");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic Session to '" +
                            _mysql_uri_nopasswd + "/mysql'");
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSchema:mysql>");
  output_handler.wipe_all();

  execute("session.close()");

  execute("\\connect -c mysqlx://" + _uri);
  MY_EXPECT_STDERR_CONTAINS("Invalid URI for Classic session");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_connect_auto) {
  // Session type determined from connection success
  {
    execute("\\connect " + _uri);
    MY_EXPECT_STDOUT_CONTAINS("Creating a Session to '" + _uri_nopasswd + "'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS(
        "No default schema selected; type \\use <schema> to set one.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // Session type determined from connection success
  {
    execute("\\connect " + _mysql_uri);
    MY_EXPECT_STDOUT_CONTAINS("Creating a Session to '" + _mysql_uri_nopasswd +
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
    MY_EXPECT_STDOUT_CONTAINS("Creating a Classic Session to '" +
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

  // Session type determined by the URI scheme
  {
    execute("\\connect mysqlx://" + _uri);
    MY_EXPECT_STDOUT_CONTAINS("Creating a Node Session to '" + _uri_nopasswd +
                              "'");
    MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
    MY_EXPECT_STDOUT_CONTAINS("(X protocol)");
    MY_EXPECT_STDOUT_CONTAINS(
        "No default schema selected; type \\use <schema> to set one.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }
}

TEST_F(Interactive_shell_test, shell_function_connect_node) {
  execute("shell.connect('mysqlx://" + _uri + "');");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Node Session to '" + _uri_nopasswd +
                            "'");
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("<Undefined>\n", output_handler.std_out.c_str());

  execute("session.close()");

  execute("shell.connect('mysqlx://" + _uri + "/mysql');");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Node Session to '" + _uri_nopasswd +
                            "/mysql'");
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  MY_EXPECT_STDOUT_CONTAINS("(X protocol)");
  MY_EXPECT_STDOUT_CONTAINS("Default schema `mysql` accessible through db.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  MY_EXPECT_STDOUT_CONTAINS("<Schema:mysql>");
  output_handler.wipe_all();

  execute("session.close()");
}

TEST_F(Interactive_shell_test, shell_function_connect_classic) {
  execute("shell.connect('mysql://" + _mysql_uri + "');");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic Session to '" +
                            _mysql_uri_nopasswd + "'");
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("<Undefined>\n", output_handler.std_out.c_str());

  execute("session.close()");

  execute("shell.connect('mysql://" + _mysql_uri + "/mysql');");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic Session to '" +
                            _mysql_uri_nopasswd + "/mysql'");
  MY_EXPECT_STDOUT_CONTAINS("Your MySQL connection id is ");
  MY_EXPECT_STDOUT_CONTAINS("Default schema set to `mysql`.");
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
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
    MY_EXPECT_STDOUT_CONTAINS("Creating a Session to '" + _uri_nopasswd + "'");
    MY_EXPECT_STDOUT_CONTAINS(
        "No default schema selected; type \\use <schema> to set one.");
    output_handler.wipe_all();

    execute("session");
    MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
    output_handler.wipe_all();

    execute("session.close()");
  }

  // Session type determined from connection success
  {
    execute("shell.connect('" + _mysql_uri + "');");
    MY_EXPECT_STDOUT_CONTAINS("Creating a Session to '" + _mysql_uri_nopasswd +
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

TEST_F(Interactive_shell_test, shell_command_use) {
  execute("\\use mysql");
  MY_EXPECT_STDERR_CONTAINS("Not Connected.");
  output_handler.wipe_all();

  execute("\\connect " + _uri);
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("<Undefined>\n", output_handler.std_out.c_str());
  output_handler.wipe_all();

  execute("\\use mysql");
  MY_EXPECT_STDOUT_CONTAINS("Schema `mysql` accessible through db.");
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

  execute("\\connect -n " + _uri);
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("<Undefined>\n", output_handler.std_out.c_str());
  output_handler.wipe_all();

  execute("\\use mysql");
  MY_EXPECT_STDOUT_CONTAINS("Schema `mysql` accessible through db.");
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("<Schema:mysql>\n", output_handler.std_out.c_str());
  output_handler.wipe_all();

  execute("session.close()");

  execute("\\connect -c " + _mysql_uri);
  MY_EXPECT_STDOUT_CONTAINS(
      "No default schema selected; type \\use <schema> to set one.");
  output_handler.wipe_all();

  execute("db");
  EXPECT_STREQ("<Undefined>\n", output_handler.std_out.c_str());
  output_handler.wipe_all();

  execute("\\use mysql");
  MY_EXPECT_STDOUT_CONTAINS("Schema set to `mysql`.");
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
      "If the session type is not specified, an Node session will be "
      "established.");
  output_handler.wipe_all();

  execute("\\help \\use");
  MY_EXPECT_STDOUT_CONTAINS(
      "NOTE: This command works with the active session.\n");
  MY_EXPECT_STDOUT_CONTAINS(
      "The global db variable will be updated to hold the requested schema.");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_help_global_objects_js) {
  // Cleanup for the test
  execute("\\js");
  execute("\\connect -n " + _uri + "/mysql");
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
  execute("\\connect -n " + _uri + "/mysql");
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
  execute("\\connect -n " + _uri + "/mysql");
  execute("\\?");
  MY_EXPECT_STDOUT_CONTAINS("===== Global Objects =====");
  MY_EXPECT_STDOUT_NOT_CONTAINS(
      "db         Used to work with database schema objects.");
  MY_EXPECT_STDOUT_NOT_CONTAINS(
      "dba        Allows performing DBA operations using the MySQL X "
      "AdminAPI.");
  MY_EXPECT_STDOUT_NOT_CONTAINS(
      "mysql      Used to work with classic MySQL sessions using SQL.");
  MY_EXPECT_STDOUT_NOT_CONTAINS(
      "mysqlx     Used to work with X Protocol sessions using the MySQL X "
      "DevAPI.");
  MY_EXPECT_STDOUT_CONTAINS(
      "session    Represents the currently open MySQL session.");
  MY_EXPECT_STDOUT_NOT_CONTAINS(
      "shell      Gives access to general purpose functions and properties.");
  MY_EXPECT_STDOUT_NOT_CONTAINS(
      "sys        Gives access to system specific parameters.");

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

TEST_F(Interactive_shell_test, js_db_usage_with_no_wizards) {
  _options->wizards = false;
  reset_shell();

  execute("db");
  MY_EXPECT_STDERR_CONTAINS("ReferenceError: db is not defined");
  output_handler.wipe_all();

  execute("db.getName()");
  MY_EXPECT_STDERR_CONTAINS("ReferenceError: db is not defined");
  output_handler.wipe_all();

  execute("db.name");
  MY_EXPECT_STDERR_CONTAINS("ReferenceError: db is not defined");
  output_handler.wipe_all();

  _options->uri = _uri + "/mysql";
  _options->interactive = true;
  reset_shell();
  _interactive_shell->connect(true);
  output_handler.wipe_all();

  execute("db");
  MY_EXPECT_STDOUT_CONTAINS("<Schema:mysql>");
  output_handler.wipe_all();

  execute("db.getName()");
  MY_EXPECT_STDOUT_CONTAINS("mysql");
  output_handler.wipe_all();

  execute("db.name");
  MY_EXPECT_STDOUT_CONTAINS("mysql");
  output_handler.wipe_all();

  execute("session.close()");
}

TEST_F(Interactive_shell_test, js_session_usage_with_no_wizards) {
  _options->wizards = false;
  reset_shell();

  execute("session");
  MY_EXPECT_STDOUT_NOT_CONTAINS("ReferenceError: session is not defined");
  output_handler.wipe_all();

  execute("session.getUri()");
  MY_EXPECT_STDOUT_NOT_CONTAINS("ReferenceError: session is not defined");
  output_handler.wipe_all();

  execute("session.uri");
  MY_EXPECT_STDOUT_NOT_CONTAINS("ReferenceError: session is not defined");
  output_handler.wipe_all();

  _options->uri = _uri + "/mysql";
  _options->interactive = true;
  reset_shell();
  _interactive_shell->connect(true);
  output_handler.wipe_all();

  execute("session");
  MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
  output_handler.wipe_all();

  execute("session.getUri()");
  MY_EXPECT_STDOUT_CONTAINS(_uri_nopasswd);
  output_handler.wipe_all();

  execute("session.uri");
  MY_EXPECT_STDOUT_CONTAINS(_uri_nopasswd);
  output_handler.wipe_all();

  execute("session.close()");
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
    out << "def my_prompt():" << std::endl;
    out << "   return '---> '" << std::endl << std::endl;
    out << "shell.custom_prompt = my_prompt" << std::endl;
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

  EXPECT_EQ("---> ", _interactive_shell->prompt());
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
    out << "shell.customPrompt = function() {" << std::endl;
    out << "   return '---> ';" << std::endl;
    out << "}" << std::endl << std::endl;
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
  MY_EXPECT_STDOUT_CONTAINS("getNodeSession");
  output_handler.wipe_all();

  EXPECT_EQ("---> ", _interactive_shell->prompt());
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
  _interactive_shell->connect(true);
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
      "Creating a Classic Session to 'expired@localhost:" + _mysql_port + "'");
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
  _interactive_shell->connect(true);
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
      "Creating a Node Session to 'expired@localhost:" + _port + "'");
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
  execute("drop schema itst;");
  execute("create schema itst;");
  execute(
      "create table itst.tbl (a int, b varchar(30), c int(10), "
      "              d int(8) unsigned zerofill, e int(2) zerofill);");
  execute(
      "insert into itst.tbl values (1, 'one', -42, 42, 42), "
      "             (2, 'two', -12345, 12345, 12345),"
      "             (3, 'three', 0, 0, 0);");
  wipe_all();
  execute("select 1, 'two', 3.3, null;");
  MY_EXPECT_STDOUT_CONTAINS(
      "+---+-----+-----+------+\n"
      "| 1 | two | 3.3 | NULL |\n"
      "+---+-----+-----+------+\n"
      "| 1 | two | 3.3 | null |\n"
      "+---+-----+-----+------+\n"
      "1 row in set (");

  wipe_all();
  // test zerofill
  execute("select * from itst.tbl;");
  MY_EXPECT_STDOUT_CONTAINS(
      "+---+-------+--------+----------+-------+\n"
      "| a | b     | c      | d        | e     |\n"
      "+---+-------+--------+----------+-------+\n"
      "| 1 | one   |    -42 | 00000042 |    42 |\n"
      "| 2 | two   | -12345 | 00012345 | 12345 |\n"
      "| 3 | three |      0 | 00000000 |    00 |\n"
      "+---+-------+--------+----------+-------+\n"
      "3 rows in set (");

  execute("\\js");
  execute("shell.options['outputFormat']='vertical'");
  execute("\\sql");
  wipe_all();

  execute("select * from itst.tbl;");
  MY_EXPECT_STDOUT_CONTAINS(
      "*************************** 1. row ***************************\n"
      "a: 1\n"
      "b: one\n"
      "c: -42\n"
      "d: 42\n"
      "e: 42\n"
      "*************************** 2. row ***************************\n"
      "a: 2\n"
      "b: two\n"
      "c: -12345\n"
      "d: 12345\n"
      "e: 12345\n"
      "*************************** 3. row ***************************\n"
      "a: 3\n"
      "b: three\n"
      "c: 0\n"
      "d: 0\n"
      "e: 0\n"
      "3 rows in set (");

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
      "            \"e\": 42\n"
      "        },\n"
      "        {\n"
      "            \"a\": 2,\n"
      "            \"b\": \"two\",\n"
      "            \"c\": -12345,\n"
      "            \"d\": 12345,\n"
      "            \"e\": 12345\n"
      "        },\n"
      "        {\n"
      "            \"a\": 3,\n"
      "            \"b\": \"three\",\n"
      "            \"c\": 0,\n"
      "            \"d\": 0,\n"
      "            \"e\": 0\n"
      "        }\n"
      "    ],\n"
      "    \"warningCount\": 0,\n"
      "    \"warnings\": [],\n"
      "    \"hasData\": true,\n"
      "    \"affectedRowCount\": 0,\n");

  execute("drop schema itst;");
}

TEST_F(Interactive_shell_test, x_sql_result) {
  execute("\\connect " + _uri);
  execute("\\sql");
  execute("drop schema itst;");
  execute("create schema itst;");
  execute(
      "create table itst.tbl (a int, b varchar(30), c int(10), "
      "              d int(8) unsigned zerofill, e int(2) zerofill);");
  execute(
      "insert into itst.tbl values (1, 'one', -42, 42, 42), "
      "             (2, 'two', -12345, 12345, 12345),"
      "             (3, 'three', 0, 0, 0);");
  wipe_all();
  execute("select 1, 'two', 3.3, null;");
  MY_EXPECT_STDOUT_CONTAINS(
      "+---+-----+-----+------+\n"
      "| 1 | two | 3.3 | NULL |\n"
      "+---+-----+-----+------+\n"
      "| 1 | two | 3.3 | null |\n"
      "+---+-----+-----+------+\n"
      "1 row in set (");

  wipe_all();
  // test zerofill
  execute("select * from itst.tbl;");
  MY_EXPECT_STDOUT_CONTAINS(
      "+---+-------+--------+----------+-------+\n"
      "| a | b     | c      | d        | e     |\n"
      "+---+-------+--------+----------+-------+\n"
      "| 1 | one   |    -42 | 00000042 |    42 |\n"
      "| 2 | two   | -12345 | 00012345 | 12345 |\n"
      "| 3 | three |      0 | 00000000 |    00 |\n"
      "+---+-------+--------+----------+-------+\n"
      "3 rows in set (");

  execute("\\js");
  execute("shell.options['outputFormat']='vertical'");
  execute("\\sql");
  wipe_all();

  execute("select * from itst.tbl;");
  MY_EXPECT_STDOUT_CONTAINS(
      "*************************** 1. row ***************************\n"
      "a: 1\n"
      "b: one\n"
      "c: -42\n"
      "d: 42\n"
      "e: 42\n"
      "*************************** 2. row ***************************\n"
      "a: 2\n"
      "b: two\n"
      "c: -12345\n"
      "d: 12345\n"
      "e: 12345\n"
      "*************************** 3. row ***************************\n"
      "a: 3\n"
      "b: three\n"
      "c: 0\n"
      "d: 0\n"
      "e: 0\n"
      "3 rows in set (");

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
      "            \"e\": 42\n"
      "        },\n"
      "        {\n"
      "            \"a\": 2,\n"
      "            \"b\": \"two\",\n"
      "            \"c\": -12345,\n"
      "            \"d\": 12345,\n"
      "            \"e\": 12345\n"
      "        },\n"
      "        {\n"
      "            \"a\": 3,\n"
      "            \"b\": \"three\",\n"
      "            \"c\": 0,\n"
      "            \"d\": 0,\n"
      "            \"e\": 0\n"
      "        }\n"
      "    ],\n"
      "    \"hasData\": true,\n"
      "    \"affectedRowCount\": 0,\n");

  execute("drop schema itst;");
}

}  // namespace shell_core_tests
}  // namespace shcore
