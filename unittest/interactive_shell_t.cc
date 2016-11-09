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
#include "utils/utils_file.h"

namespace shcore {
namespace shell_core_tests {
class Interactive_shell_test : public Shell_core_test_wrapper {
public:
  virtual void set_options() {
    _options->interactive = true;
    _options->wizards = true;
  };
};

TEST_F(Interactive_shell_test, warning_insecure_password) {
  // Test secure call passing uri with no password (will be prompted)
  _options->uri = "root@localhost";
  reset_shell();
  output_handler.passwords.push_back("whatever");

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

TEST_F(Interactive_shell_test, shell_command_connect_node) {
  _interactive_shell->process_line("\\connect -n " + _uri);
  MY_EXPECT_STDOUT_CONTAINS("Creating a Node Session to '" + _uri_nopasswd + "'");
  MY_EXPECT_STDOUT_CONTAINS("Session successfully established. No default schema selected.");
  output_handler.wipe_all();

  _interactive_shell->process_line("session");
  MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  EXPECT_STREQ("<Undefined>\n", output_handler.std_out.c_str());

  _interactive_shell->process_line("session.close()");

  _interactive_shell->process_line("\\connect -n " + _uri + "/mysql");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Node Session to '" + _uri_nopasswd + "/mysql'");
  MY_EXPECT_STDOUT_CONTAINS("Session successfully established. Default schema `mysql` accessible through db.");
  output_handler.wipe_all();

  _interactive_shell->process_line("session");
  MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  MY_EXPECT_STDOUT_CONTAINS("<Schema:mysql>");
  output_handler.wipe_all();

  _interactive_shell->process_line("session.close()");

  _interactive_shell->process_line("\\connect -n mysql://" + _mysql_uri);
  MY_EXPECT_STDERR_CONTAINS("Invalid URI for Node session");
  output_handler.wipe_all();

  _interactive_shell->process_line("\\connect -n " + _mysql_uri);
  MY_EXPECT_STDERR_CONTAINS("Requested session assumes MySQL X Protocol but '" + _host + ":" + _mysql_port + "' seems to speak the classic MySQL protocol");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_connect_classic) {
  _interactive_shell->process_line("\\connect -c " + _mysql_uri);
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic Session to '" + _mysql_uri_nopasswd + "'");
  MY_EXPECT_STDOUT_CONTAINS("Session successfully established. No default schema selected.");
  output_handler.wipe_all();

  _interactive_shell->process_line("session");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  EXPECT_STREQ("<Undefined>\n", output_handler.std_out.c_str());

  _interactive_shell->process_line("session.close()");

  _interactive_shell->process_line("\\connect -c " + _mysql_uri + "/mysql");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic Session to '" + _mysql_uri_nopasswd + "/mysql'");
  MY_EXPECT_STDOUT_CONTAINS("Session successfully established. Default schema set to `mysql`.");
  output_handler.wipe_all();

  _interactive_shell->process_line("session");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSchema:mysql>");
  output_handler.wipe_all();

  _interactive_shell->process_line("session.close()");

  _interactive_shell->process_line("\\connect -c mysqlx://" + _uri);
  MY_EXPECT_STDERR_CONTAINS("Invalid URI for Classic session");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_connect_auto) {
  // Session type determined from connection success
  {
    _interactive_shell->process_line("\\connect " + _uri);
    MY_EXPECT_STDOUT_CONTAINS("Creating a Session to '" + _uri_nopasswd + "'");
    MY_EXPECT_STDOUT_CONTAINS("Node Session successfully established. No default schema selected.");
    output_handler.wipe_all();

    _interactive_shell->process_line("session");
    MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
    output_handler.wipe_all();

    _interactive_shell->process_line("session.close()");
  }

  // Session type determined from connection success
  {
    _interactive_shell->process_line("\\connect " + _mysql_uri);
    MY_EXPECT_STDOUT_CONTAINS("Creating a Session to '" + _mysql_uri_nopasswd + "'");
    MY_EXPECT_STDOUT_CONTAINS("Classic Session successfully established. No default schema selected.");
    output_handler.wipe_all();

    _interactive_shell->process_line("session");
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
    output_handler.wipe_all();

    _interactive_shell->process_line("session.close()");
  }

  // Session type determined by the URI scheme
  {
    _interactive_shell->process_line("\\connect mysql://" + _mysql_uri);
    MY_EXPECT_STDOUT_CONTAINS("Creating a Classic Session to '" + _mysql_uri_nopasswd + "'");
    MY_EXPECT_STDOUT_CONTAINS("Session successfully established. No default schema selected.");
    output_handler.wipe_all();

    _interactive_shell->process_line("session");
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
    output_handler.wipe_all();

    _interactive_shell->process_line("session.close()");
  }

  // Session type determined by the URI scheme
  {
    _interactive_shell->process_line("\\connect mysqlx://" + _uri);
    MY_EXPECT_STDOUT_CONTAINS("Creating a Node Session to '" + _uri_nopasswd + "'");
    MY_EXPECT_STDOUT_CONTAINS("Session successfully established. No default schema selected.");
    output_handler.wipe_all();

    _interactive_shell->process_line("session");
    MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
    output_handler.wipe_all();

    _interactive_shell->process_line("session.close()");
  }
}

TEST_F(Interactive_shell_test, shell_function_connect_node) {
  _interactive_shell->process_line("shell.connect('mysqlx://" + _uri+"');");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Node Session to '" + _uri_nopasswd + "'");
  MY_EXPECT_STDOUT_CONTAINS("Session successfully established. No default schema selected.");
  output_handler.wipe_all();

  _interactive_shell->process_line("session");
  MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  EXPECT_STREQ("<Undefined>\n", output_handler.std_out.c_str());

  _interactive_shell->process_line("session.close()");

  _interactive_shell->process_line("shell.connect('mysqlx://" + _uri + "/mysql');");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Node Session to '" + _uri_nopasswd + "/mysql'");
  MY_EXPECT_STDOUT_CONTAINS("Session successfully established. Default schema `mysql` accessible through db.");
  output_handler.wipe_all();

  _interactive_shell->process_line("session");
  MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  MY_EXPECT_STDOUT_CONTAINS("<Schema:mysql>");
  output_handler.wipe_all();

  _interactive_shell->process_line("session.close()");
}

TEST_F(Interactive_shell_test, shell_function_connect_classic) {
  _interactive_shell->process_line("shell.connect('mysql://" + _mysql_uri + "');");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic Session to '" + _mysql_uri_nopasswd + "'");
  MY_EXPECT_STDOUT_CONTAINS("Session successfully established. No default schema selected.");
  output_handler.wipe_all();

  _interactive_shell->process_line("session");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  EXPECT_STREQ("<Undefined>\n", output_handler.std_out.c_str());

  _interactive_shell->process_line("session.close()");

  _interactive_shell->process_line("shell.connect('mysql://" + _mysql_uri + "/mysql');");
  MY_EXPECT_STDOUT_CONTAINS("Creating a Classic Session to '" + _mysql_uri_nopasswd + "/mysql'");
  MY_EXPECT_STDOUT_CONTAINS("Session successfully established. Default schema set to `mysql`.");
  output_handler.wipe_all();

  _interactive_shell->process_line("session");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  MY_EXPECT_STDOUT_CONTAINS("<ClassicSchema:mysql>");
  output_handler.wipe_all();

  _interactive_shell->process_line("session.close()");
}

TEST_F(Interactive_shell_test, shell_function_connect_auto) {
  // Session type determined from connection success
  {
    _interactive_shell->process_line("shell.connect('" + _uri + "');");
    MY_EXPECT_STDOUT_CONTAINS("Creating a Session to '" + _uri_nopasswd + "'");
    MY_EXPECT_STDOUT_CONTAINS("Node Session successfully established. No default schema selected.");
    output_handler.wipe_all();

    _interactive_shell->process_line("session");
    MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
    output_handler.wipe_all();

    _interactive_shell->process_line("session.close()");
  }

  // Session type determined from connection success
  {
    _interactive_shell->process_line("shell.connect('" + _mysql_uri + "');");
    MY_EXPECT_STDOUT_CONTAINS("Creating a Session to '" + _mysql_uri_nopasswd + "'");
    MY_EXPECT_STDOUT_CONTAINS("Classic Session successfully established. No default schema selected.");
    output_handler.wipe_all();

    _interactive_shell->process_line("session");
    MY_EXPECT_STDOUT_CONTAINS("<ClassicSession:" + _mysql_uri_nopasswd);
    output_handler.wipe_all();

    _interactive_shell->process_line("session.close()");
  }
}

TEST_F(Interactive_shell_test, shell_command_use) {
  _interactive_shell->process_line("\\use mysql");
  MY_EXPECT_STDERR_CONTAINS("Not Connected.");
  output_handler.wipe_all();

  _interactive_shell->process_line("\\connect " + _uri);
  MY_EXPECT_STDOUT_CONTAINS("No default schema selected.");
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  EXPECT_STREQ("<Undefined>\n", output_handler.std_out.c_str());
  output_handler.wipe_all();

  _interactive_shell->process_line("\\use mysql");
  MY_EXPECT_STDOUT_CONTAINS("Schema `mysql` accessible through db.");
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  EXPECT_STREQ("<Schema:mysql>\n", output_handler.std_out.c_str());
  output_handler.wipe_all();

  _interactive_shell->process_line("\\use unexisting");
  MY_EXPECT_STDERR_CONTAINS("Unknown database 'unexisting'");
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  EXPECT_STREQ("<Schema:mysql>\n", output_handler.std_out.c_str());
  output_handler.wipe_all();

  _interactive_shell->process_line("session.close()");

  _interactive_shell->process_line("\\connect -n " + _uri);
  MY_EXPECT_STDOUT_CONTAINS("No default schema selected.");
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  EXPECT_STREQ("<Undefined>\n", output_handler.std_out.c_str());
  output_handler.wipe_all();

  _interactive_shell->process_line("\\use mysql");
  MY_EXPECT_STDOUT_CONTAINS("Schema `mysql` accessible through db.");
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  EXPECT_STREQ("<Schema:mysql>\n", output_handler.std_out.c_str());
  output_handler.wipe_all();

  _interactive_shell->process_line("session.close()");

  _interactive_shell->process_line("\\connect -c " + _mysql_uri);
  MY_EXPECT_STDOUT_CONTAINS("No default schema selected.");
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  EXPECT_STREQ("<Undefined>\n", output_handler.std_out.c_str());
  output_handler.wipe_all();

  _interactive_shell->process_line("\\use mysql");
  MY_EXPECT_STDOUT_CONTAINS("Schema set to `mysql`.");
  output_handler.wipe_all();

  _interactive_shell->process_line("db");
  EXPECT_STREQ("<ClassicSchema:mysql>\n", output_handler.std_out.c_str());
  output_handler.wipe_all();

  _interactive_shell->process_line("session.close()");
}

TEST_F(Interactive_shell_test, shell_command_warnings) {
  _options->interactive = true;
  reset_shell();
  _interactive_shell->process_line("\\warnings");
  MY_EXPECT_STDOUT_CONTAINS("Show warnings enabled.");
  output_handler.wipe_all();

  _interactive_shell->process_line("\\W");
  MY_EXPECT_STDOUT_CONTAINS("Show warnings enabled.");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_no_warnings) {
  _options->interactive = true;
  reset_shell();
  _interactive_shell->process_line("\\nowarnings");
  MY_EXPECT_STDOUT_CONTAINS("Show warnings disabled.");
  output_handler.wipe_all();

  _interactive_shell->process_line("\\w");
  MY_EXPECT_STDOUT_CONTAINS("Show warnings disabled.");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, shell_command_help) {
  // Cleanup for the test
  _interactive_shell->process_line("\\?");
  MY_EXPECT_STDOUT_CONTAINS("===== Global Commands =====");
  MY_EXPECT_STDOUT_CONTAINS("\\help       (\\?,\\h)    Print this help.");
  MY_EXPECT_STDOUT_CONTAINS("\\sql                   Switch to SQL processing mode.");
  MY_EXPECT_STDOUT_CONTAINS("\\js                    Switch to JavaScript processing mode.");
  MY_EXPECT_STDOUT_CONTAINS("\\py                    Switch to Python processing mode.");
  MY_EXPECT_STDOUT_CONTAINS("\\source     (\\.)       Execute a script file. Takes a file name as an argument.");
  MY_EXPECT_STDOUT_CONTAINS("\\                      Start multi-line input when in SQL mode.");
  MY_EXPECT_STDOUT_CONTAINS("\\quit       (\\q,\\exit) Quit MySQL Shell.");
  MY_EXPECT_STDOUT_CONTAINS("\\connect    (\\c)       Connect to a server.");
  MY_EXPECT_STDOUT_CONTAINS("\\warnings   (\\W)       Show warnings after every statement.");
  MY_EXPECT_STDOUT_CONTAINS("\\nowarnings (\\w)       Don't show warnings after every statement.");
  MY_EXPECT_STDOUT_CONTAINS("\\status     (\\s)       Print information about the current global connection.");
  MY_EXPECT_STDOUT_CONTAINS("\\use        (\\u)       Set the current schema for the global session.");
  MY_EXPECT_STDOUT_CONTAINS("For help on a specific command use the command as \\? <command>");

  _interactive_shell->process_line("\\help \\source");
  MY_EXPECT_STDOUT_CONTAINS("NOTE: Can execute files from the supported types: SQL, Javascript, or Python.");
  output_handler.wipe_all();

  _interactive_shell->process_line("\\help \\connect");
  MY_EXPECT_STDOUT_CONTAINS("If the session type is not specified, an Node session will be established.");
  output_handler.wipe_all();

  _interactive_shell->process_line("\\help \\use");
  MY_EXPECT_STDOUT_CONTAINS("The global db variable will be updated to hold the requested schema.");
  output_handler.wipe_all();
}

TEST_F(Interactive_shell_test, js_db_usage_with_no_wizards) {
  _options->wizards = false;
  reset_shell();

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

  _interactive_shell->process_line("session.close()");
}

TEST_F(Interactive_shell_test, js_session_usage_with_no_wizards) {
  _options->wizards = false;
  reset_shell();

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
  MY_EXPECT_STDOUT_CONTAINS("<NodeSession:" + _uri_nopasswd);
  output_handler.wipe_all();

  _interactive_shell->process_line("session.getUri()");
  MY_EXPECT_STDOUT_CONTAINS(_uri_nopasswd);
  output_handler.wipe_all();

  _interactive_shell->process_line("session.uri");
  MY_EXPECT_STDOUT_CONTAINS(_uri_nopasswd);
  output_handler.wipe_all();

  _interactive_shell->process_line("session.close()");
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

  _interactive_shell->process_line("mysqlx");
  MY_EXPECT_STDOUT_CONTAINS("<module '__mysqlx__' (built-in)>");
  output_handler.wipe_all();

  EXPECT_EQ("---> ", _interactive_shell->prompt());
  output_handler.wipe_all();

  _interactive_shell->process_line("the_variable");
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

  _interactive_shell->process_line("dir(mysqlx)");
  MY_EXPECT_STDOUT_CONTAINS("getNodeSession");
  output_handler.wipe_all();

  EXPECT_EQ("---> ", _interactive_shell->prompt());
  output_handler.wipe_all();

  _interactive_shell->process_line("the_variable");
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
}
}
