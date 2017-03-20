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
#include <cstdlib>
#include <fstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <stdarg.h>
#include <string>

#include "gtest/gtest.h"
#include "mysqlsh/shell_cmdline_options.h"

namespace shcore {
#define IS_CONNECTION_DATA true
#define IS_NULLABLE true
#define AS__STRING(x) std::to_string(x)

std::string session_type_name(mysqlsh::SessionType type){
  std::string ret_val;

  switch(type){
    case mysqlsh::SessionType::Auto:
      ret_val = "Auto";
      break;
    case mysqlsh::SessionType::Classic:
      ret_val = "Classic";
      break;
    case mysqlsh::SessionType::Node:
      ret_val = "Node";
      break;
    case mysqlsh::SessionType::X:
      ret_val = "X";
      break;
  }

  return ret_val;
}

std::string shell_mode_name(IShell_core::Mode mode){
  std::string ret_val;

  switch(mode){
    case IShell_core::Mode::JScript:
      ret_val = "JavaScript";
      break;
    case IShell_core::Mode::Python:
      ret_val = "Python";
      break;
    case IShell_core::Mode::SQL:
      ret_val = "SQL";
      break;
    case IShell_core::Mode::None:
      ret_val = "None";
      break;
  }

  return ret_val;
}


class Shell_cmdline_options_t : public ::testing::Test {
public:
  Shell_cmdline_options_t() {}

  std::string get_string(mysqlsh::Shell_options* options, const std::string &option) {
    if (option == "host")
      return options->host;
    else if (option == "user")
      return options->user;
    else if (option == "password")
      return options->password ? std::string(options->password) : "";
    else if (option == "port")
      return AS__STRING(options->port);
    else if (option == "schema")
      return options->schema;
    else if (option == "sock")
      return options->sock;
    else if (option == "ssl-ca")
      return options->ssl_info.ca;
    else if (option == "ssl-cert")
      return options->ssl_info.cert;
    else if (option == "ssl-key")
      return options->ssl_info.key;
    else if (option == "ssl-capath")
      return options->ssl_info.capath;
    else if (option == "ssl-crl")
      return options->ssl_info.crl;
    else if (option == "ssl-crlpath")
      return options->ssl_info.crlpath;
    else if (option == "ssl-cipher")
      return options->ssl_info.ciphers;
    else if (option == "tls-version")
      return options->ssl_info.tls_version;
    else if (option == "ssl")
      return AS__STRING(options->ssl_info.skip? 0 : 1);
    else if (option == "uri")
      return options->uri;
    else if (option == "output_format")
      return options->output_format;
    else if (option == "session_type")
      return session_type_name(options->session_type);
    else if (option == "force")
      return AS__STRING(options->force);
    else if (option == "interactive")
      return AS__STRING(options->interactive);
    else if (option == "full_interactive")
      return AS__STRING(options->full_interactive);
    else if (option == "passwords_from_stdin")
      return AS__STRING(options->passwords_from_stdin);
    else if (option == "prompt_password")
      return AS__STRING(options->prompt_password);
    else if (option == "recreate_database")
      return AS__STRING(options->recreate_database);
    else if (option == "trace_protocol")
      return AS__STRING(options->trace_protocol);
    else if (option == "log_level")
      return AS__STRING(options->log_level);
    else if (option == "initial-mode")
      return shell_mode_name(options->initial_mode);
    else if (option == "session-type")
      return session_type_name(options->session_type);
    else if (option == "wizards")
      return AS__STRING(options->wizards);
    else if (option == "execute_statement")
      return options->execute_statement;


    return "";
  }

  void test_option_equal_value(const std::string &option, const std::string & value, bool connection_data, const std::string& target_option = "", const char *target_value = NULL) {
    // Redirect cerr.
    std::streambuf* backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests --option=value
    std::string arg;
    arg.append("--").append(option).append("=").append(value);
    SCOPED_TRACE("TESTING: " + arg);
    char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(arg.c_str()), NULL};
    Shell_command_line_options cmd_options(2, argv);
    mysqlsh::Shell_options options = cmd_options.get_options();
    EXPECT_EQ(0, options.exit_code);
    EXPECT_EQ(connection_data, options.has_connection_data());

    std::string tgt_val;
    if (target_value)
      tgt_val.assign(target_value);
    else
      tgt_val = value;

    std::string tgt_option = target_option.empty() ? option : target_option;
    EXPECT_STREQ(tgt_val.c_str(), get_string(&options, tgt_option).c_str());

    EXPECT_STREQ("", cerr.str().c_str());

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }

  void test_option_space_value(const std::string &option, const std::string & value, bool connection_data, const std::string &target_option = "", const char *target_value = NULL) {
    // Redirect cerr.
    std::streambuf* backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests --option [value]
    std::string arg;
    arg.append("--").append(option);
    SCOPED_TRACE("TESTING: " + arg + " " + value);
    char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(arg.c_str()), const_cast<char *>(value.c_str()), NULL};
    Shell_command_line_options cmd_options(3, argv);
    mysqlsh::Shell_options options = cmd_options.get_options();
    EXPECT_EQ(0, options.exit_code);
    EXPECT_EQ(connection_data, options.has_connection_data());

    std::string tgt_val;
    if (target_value)
      tgt_val.assign(target_value);
    else
      tgt_val = value;

    std::string tgt_option = target_option.empty() ? option : target_option;
    EXPECT_STREQ(tgt_val.c_str(), get_string(&options, tgt_option).c_str());

    EXPECT_STREQ("", cerr.str().c_str());

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }

  void test_short_option_value(const std::string &option, const std::string &soption, const std::string &value, bool connection_data, const std::string& target_option = "", const char *target_value = NULL) {
    // Redirect cerr.
    std::streambuf* backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests --ovalue
    std::string arg;
    arg.append("-").append(soption).append(value);
    SCOPED_TRACE("TESTING: " + arg);
    char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(arg.c_str()), NULL};
    Shell_command_line_options cmd_options(2, argv);
    mysqlsh::Shell_options options = cmd_options.get_options();
    EXPECT_EQ(0, options.exit_code);
    EXPECT_EQ(connection_data, options.has_connection_data());

    std::string tgt_val;
    if (target_value)
      tgt_val.assign(target_value);
    else
      tgt_val = value;

    std::string tgt_option = target_option.empty() ? option : target_option;
    EXPECT_STREQ(tgt_val.c_str(), get_string(&options, tgt_option).c_str());

    EXPECT_STREQ("", cerr.str().c_str());

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }

  void test_short_option_space_value(const std::string &option, const std::string& soption, const std::string &value, bool connection_data, const std::string& target_option = "", const char* target_value = NULL) {
    // Redirect cerr.
    std::streambuf* backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests --option [value]
    std::string arg;
    arg.append("-").append(soption);
    SCOPED_TRACE("TESTING: " + arg + " " + value);
    char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(arg.c_str()), const_cast<char *>(value.c_str()), NULL};
    Shell_command_line_options cmd_options(3, argv);
    mysqlsh::Shell_options options = cmd_options.get_options();
    EXPECT_EQ(0, options.exit_code);
    EXPECT_EQ(connection_data, options.has_connection_data());

    std::string tgt_val;
    if (target_value)
      tgt_val.assign(target_value);
    else
      tgt_val = value;

    std::string tgt_option = target_option.empty() ? option : target_option;
    EXPECT_STREQ(tgt_val.c_str(), get_string(&options, tgt_option).c_str());

    EXPECT_STREQ("", cerr.str().c_str());

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }

  void test_option_space_no_value(const std::string &option, bool valid, const std::string& defval, const std::string target_option = "", const char *target_value = NULL) {
    // Redirect cerr.
    std::streambuf* backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests --option [value]
    std::string arg;
    arg.append("--").append(option);
    SCOPED_TRACE("TESTING: " + arg);
    char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(arg.c_str()), NULL};
    Shell_command_line_options cmd_options(2, argv);
    mysqlsh::Shell_options options = cmd_options.get_options();

    if (valid) {
      EXPECT_EQ(0, options.exit_code);

      std::string tgt_val;
      if (target_value)
        tgt_val.assign(target_value);
      else
        tgt_val = defval;

      std::string tgt_option = target_option.empty() ? option : target_option;
      EXPECT_STREQ(tgt_val.c_str(), get_string(&options, tgt_option).c_str());

      EXPECT_STREQ("", cerr.str().c_str());
    } else {
      EXPECT_EQ(1, options.exit_code);
      std::string message = "ut: option ";
      message.append("--").append(option).append(" requires an argument\n");
      EXPECT_STREQ(message.c_str(), cerr.str().c_str());
    }

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }

  void test_option_equal_no_value(const std::string &option, bool valid) {
    // Redirect cerr.
    std::streambuf* backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests --option=
    std::string arg;
    arg.append("--").append(option).append("=");
    SCOPED_TRACE("TESTING: " + arg);
    char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(arg.c_str()), NULL};
    Shell_command_line_options options(2, argv);

    if (valid) {
      EXPECT_EQ(0, options.exit_code);
      EXPECT_STREQ("", cerr.str().c_str());
    } else {
      EXPECT_EQ(1, options.exit_code);
      std::string message = "ut: option ";
      message.append("--").append(option).append("= requires an argument\n");
      EXPECT_STREQ(message.c_str(), cerr.str().c_str());
    }

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }

  void test_option_with_value(const std::string &option, const std::string &soption, const std::string &value, const std::string &defval, bool is_connection_data, bool nullable, const std::string& target_option = "", const char *target_value = NULL) {
    // --option=<value>
    test_option_equal_value(option, value, is_connection_data, target_option, target_value);

    // --option value
    test_option_space_value(option, value, is_connection_data, target_option, target_value);

    // --option
    test_option_space_no_value(option, !defval.empty() || nullable, defval, target_option);

    if (!soption.empty()) {
      // -o<value>
      test_short_option_value(option, soption, value, is_connection_data, target_option, target_value);

      // -o <value>
      test_short_option_space_value(option, soption, value, is_connection_data, target_option, target_value);
    }

    // --option=
    test_option_equal_no_value(option, !defval.empty() || nullable);
  }

  void test_option_with_no_value(const std::string &option, const std::string &target_option, const std::string &target_value) {
    // Redirect cerr.
    std::streambuf* backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests --option or -o

    SCOPED_TRACE("TESTING: " + option);
    char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(option.c_str()), NULL};
    Shell_command_line_options cmd_options(2, argv);
    mysqlsh::Shell_options options = cmd_options.get_options();

    EXPECT_EQ(0, options.exit_code);
    EXPECT_STREQ(target_value.c_str(), get_string(&options, target_option).c_str());
    EXPECT_STREQ("", cerr.str().c_str());

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }

  void test_session_type_conflicts(const std::string& firstArg, const std::string& secondArg, const std::string& firstST, const std::string& secondST, int ret_code) {
    std::streambuf* backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    {
      SCOPED_TRACE("TESTING: " + firstArg + " " + secondArg);
      char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(firstArg.c_str()), const_cast<char *>(secondArg.c_str()), NULL};
      Shell_command_line_options options(3, argv);

      EXPECT_EQ(ret_code, options.exit_code);

      std::string error = options.exit_code ? "Session type already configured to " + firstST + ", unable to change to " + secondST + " with option " + secondArg + "\n" : "";
      EXPECT_STREQ(error.c_str(), cerr.str().c_str());
    }

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }
};

TEST(Shell_cmdline_options, default_values) {
  int argc = 0;
  char **argv = NULL;

  Shell_command_line_options cmd_options(argc, argv);
  mysqlsh::Shell_options options = cmd_options.get_options();

  EXPECT_TRUE(options.exit_code == 0);
  EXPECT_FALSE(options.force);
  EXPECT_FALSE(options.full_interactive);
  EXPECT_FALSE(options.has_connection_data());
  EXPECT_TRUE(options.host.empty());

#ifdef HAVE_V8
  EXPECT_EQ(options.initial_mode, IShell_core::Mode::JScript);
#else
#ifdef HAVE_PYTHON
  EXPECT_EQ(options.initial_mode, IShell_core::Mode::Python);
#else
  EXPECT_EQ(options.initial_mode, IShell_core::Mode::SQL);
#endif
#endif

  EXPECT_FALSE(options.interactive);
  EXPECT_EQ(options.log_level, ngcommon::Logger::LOG_INFO);
  EXPECT_TRUE(options.output_format.empty());
  EXPECT_EQ(NULL, options.password);
  EXPECT_FALSE(options.passwords_from_stdin);
  EXPECT_EQ(options.port, 0);
  EXPECT_FALSE(options.prompt_password);
  EXPECT_TRUE(options.protocol.empty());
  EXPECT_FALSE(options.recreate_database);
  EXPECT_TRUE(options.run_file.empty());
  EXPECT_TRUE(options.schema.empty());
  EXPECT_EQ(options.session_type, mysqlsh::SessionType::Auto);
  EXPECT_TRUE(options.sock.empty());
  EXPECT_EQ(options.ssl_info.skip, true);
  EXPECT_TRUE(options.ssl_info.ca.empty());
  EXPECT_TRUE(options.ssl_info.cert.empty());
  EXPECT_TRUE(options.ssl_info.key.empty());
  EXPECT_TRUE(options.ssl_info.capath.empty());
  EXPECT_TRUE(options.ssl_info.crl.empty());
  EXPECT_TRUE(options.ssl_info.crlpath.empty());
  EXPECT_TRUE(options.ssl_info.ciphers.empty());
  EXPECT_TRUE(options.ssl_info.tls_version.empty());
  EXPECT_FALSE(options.trace_protocol);
  EXPECT_TRUE(options.uri.empty());
  EXPECT_TRUE(options.user.empty());
  EXPECT_TRUE(options.execute_statement.empty());
  EXPECT_TRUE(options.wizards);
  EXPECT_TRUE(options.default_session_type);
}

TEST_F(Shell_cmdline_options_t, app) {
  test_option_with_value("host", "h", "localhost", "", IS_CONNECTION_DATA, !IS_NULLABLE);
  test_option_with_value("port", "P", "3306", "", IS_CONNECTION_DATA, !IS_NULLABLE);
  test_option_with_value("schema", "D", "sakila", "", IS_CONNECTION_DATA, !IS_NULLABLE);
  test_option_with_value("database", "", "sakila", "", IS_CONNECTION_DATA, !IS_NULLABLE, "schema");
  test_option_with_value("user", "u", "root", "", IS_CONNECTION_DATA, !IS_NULLABLE);
  test_option_with_value("dbuser", "u", "root", "", IS_CONNECTION_DATA, !IS_NULLABLE, "user");

  test_option_with_no_value("-p", "prompt_password", "1");

  test_option_equal_value("dbpassword", "mypwd", IS_CONNECTION_DATA, "password");
  test_option_space_value("dbpassword", "mypwd", IS_CONNECTION_DATA, "password", "");
  test_option_space_value("dbpassword", "mypwd", IS_CONNECTION_DATA, "prompt_password", "1");
  test_short_option_value("dbpassword", "p", "mypwd", IS_CONNECTION_DATA, "password");
  test_short_option_space_value("dbpassword", "p", "mypwd", IS_CONNECTION_DATA, "password", "");
  test_short_option_space_value("dbpassword", "p", "mypwd", IS_CONNECTION_DATA, "prompt_password", "1");
  test_option_equal_no_value("dbpassword", true);
  test_option_with_no_value("--dbpassword", "prompt_password", "1");

  test_option_with_value("ssl-ca", "", "some_value", "", IS_CONNECTION_DATA, false);
  test_option_with_value("ssl-cert", "", "some_value", "", IS_CONNECTION_DATA, !IS_NULLABLE);
  test_option_with_value("ssl-key", "", "some_value", "", IS_CONNECTION_DATA, !IS_NULLABLE);
  test_option_with_value("ssl", "", "1", "1", IS_CONNECTION_DATA, !IS_NULLABLE);
  test_option_with_value("ssl", "", "0", "1", !IS_CONNECTION_DATA, !IS_NULLABLE);
  test_option_with_value("ssl", "", "yes", "1", IS_CONNECTION_DATA, !IS_NULLABLE, "", "1");
  //test_option_with_value("ssl", "", "no", "1", !IS_CONNECTION_DATA, !IS_NULLABLE, "", "0");

  test_option_with_value("execute", "e", "show databases;", "", !IS_CONNECTION_DATA, !IS_NULLABLE, "execute_statement");

  test_option_with_no_value("--classic", "session-type", session_type_name(mysqlsh::SessionType::Classic));
  test_option_with_no_value("--node", "session-type", session_type_name(mysqlsh::SessionType::Node));

  test_option_with_no_value("--sql", "session-type", session_type_name(mysqlsh::SessionType::Auto));
  test_option_with_no_value("--sql", "initial-mode", shell_mode_name(IShell_core::Mode::SQL));

  test_option_with_no_value("--sqlc", "session-type", session_type_name(mysqlsh::SessionType::Classic));
  test_option_with_no_value("--sqlc", "initial-mode", shell_mode_name(IShell_core::Mode::SQL));

  test_option_with_no_value("--sqln", "session-type", session_type_name(mysqlsh::SessionType::Node));
  test_option_with_no_value("--sqln", "initial-mode", shell_mode_name(IShell_core::Mode::SQL));

  test_option_with_no_value("--javascript", "initial-mode", shell_mode_name(IShell_core::Mode::JScript));
  test_option_with_no_value("--js", "initial-mode", shell_mode_name(IShell_core::Mode::JScript));

  test_option_with_no_value("--python", "initial-mode", shell_mode_name(IShell_core::Mode::Python));
  test_option_with_no_value("--py", "initial-mode", shell_mode_name(IShell_core::Mode::Python));

  test_option_with_no_value("--recreate-schema", "recreate_database", "1");

  test_option_with_value("json", "", "pretty", "json", !IS_CONNECTION_DATA, IS_NULLABLE, "output_format", "json");
  test_option_with_value("json", "", "raw", "json", !IS_CONNECTION_DATA, IS_NULLABLE, "output_format", "json/raw");
  test_option_with_no_value("--json", "output_format", "json");
  test_option_with_no_value("--table", "output_format", "table");
  test_option_with_no_value("--vertical", "output_format", "vertical");
  test_option_with_no_value("-E", "output_format", "vertical");
  test_option_with_no_value("--trace-proto", "trace_protocol", "1");
  test_option_with_no_value("--force", "force", "1");
  test_option_with_no_value("--interactive", "interactive", "1");
  test_option_with_no_value("-i", "interactive", "1");
  test_option_with_no_value("--no-wizard", "wizards", "0");
  test_option_with_no_value("--nw", "wizards", "0");

  test_option_with_value("interactive", "", "full", "1", !IS_CONNECTION_DATA, IS_NULLABLE, "interactive", "1");
  //test_option_with_value("interactive", "", "full", "1", !IS_CONNECTION_DATA, IS_NULLABLE, "full_interactive", "1");

  test_option_with_no_value("--passwords-from-stdin", "passwords_from_stdin", "1");
}

TEST_F(Shell_cmdline_options_t, test_session_type_conflicts) {
  test_session_type_conflicts("--sqlc", "--sqlc", "Classic", "Classic", 0);
  test_session_type_conflicts("--sqlc", "--classic", "Classic", "Classic", 0);
  test_session_type_conflicts("--sqlc", "--node", "Classic", "Node", 1);
  test_session_type_conflicts("--sqlc", "--sqln", "Classic", "Node", 1);

  test_session_type_conflicts("--sqln", "--sqln", "Node", "Node", 0);
  test_session_type_conflicts("--sqln", "--node", "Node", "Node", 0);
  test_session_type_conflicts("--sqln", "--classic", "Node", "Classic", 1);
  test_session_type_conflicts("--sqln", "--sqlc", "Node", "Classic", 1);

  test_session_type_conflicts("--node", "--node", "Node", "Node", 0);
  test_session_type_conflicts("--node", "--sqln", "Node", "Node", 0);
  test_session_type_conflicts("--node", "--classic", "Node", "Classic", 1);
  test_session_type_conflicts("--node", "--sqlc", "Node", "Classic", 1);

  test_session_type_conflicts("--classic", "--classic", "Classic", "Classic", 0);
  test_session_type_conflicts("--classic", "--sqlc", "Classic", "Classic", 0);
  test_session_type_conflicts("--classic", "--node", "Classic", "Node", 1);
  test_session_type_conflicts("--classic", "--sqln", "Classic", "Node", 1);
}

TEST_F(Shell_cmdline_options_t, test_positional_argument) {
  // Redirect cerr.
  std::streambuf* backup = std::cerr.rdbuf();
  std::ostringstream cerr;
  std::cerr.rdbuf(cerr.rdbuf());
  std::string firstArg, secondArg;

  // Using a correct uri should yield no error
  SCOPED_TRACE("TESTING: uri as positional argument.");
  firstArg = "root@localhost:3301";

  char *argv[] {const_cast<char *>("ut"),
                const_cast<char *>(firstArg.c_str()), NULL};
  Shell_command_line_options cmd_options(2, argv);
  mysqlsh::Shell_options options = cmd_options.get_options();


  EXPECT_EQ(0, options.exit_code);
  EXPECT_STREQ(options.uri.c_str(), "root@localhost:3301");
  EXPECT_STREQ("", cerr.str().c_str());


  // Using a correct uri plus the --uri option
  SCOPED_TRACE("TESTING: uri as positional argument followed by --uri option");
  firstArg = "root@localhost:3301";
  secondArg = "--uri=user2:pass@localhost";

  char *argv2[] {const_cast<char *>("ut"),
                 const_cast<char *>(firstArg.c_str()),
                 const_cast<char *>(secondArg.c_str()), NULL};
  cmd_options = Shell_command_line_options(3, argv2);
  options = cmd_options.get_options();

  EXPECT_EQ(0, options.exit_code);
  EXPECT_STREQ(options.uri.c_str(), "user2:pass@localhost");
  EXPECT_STREQ("", cerr.str().c_str());

  SCOPED_TRACE("TESTING: --uri option followed by uri as positional argument");
  firstArg = "--uri=user2@localhost";
  secondArg = "root:pass@localhost:3301";

  char *argv3[] {const_cast<char *>("ut"),
                 const_cast<char *>(firstArg.c_str()),
                 const_cast<char *>(secondArg.c_str()), NULL};

  cmd_options = Shell_command_line_options(3, argv3);
  options = cmd_options.get_options();

  EXPECT_EQ(0, options.exit_code);
  EXPECT_STREQ(options.uri.c_str(), "root:pass@localhost:3301");
  EXPECT_STREQ("", cerr.str().c_str());

  //Using an invalid uri as positional argument
  SCOPED_TRACE("TESTING: invalid uri as positional argument");
  firstArg = "not:valid_uri";

  char *argv4[] {const_cast<char *>("ut"),
                 const_cast<char *>(firstArg.c_str()), NULL};

  cmd_options = Shell_command_line_options(2, argv4);
  options = cmd_options.get_options();
  EXPECT_EQ(1, options.exit_code);
  EXPECT_STREQ(options.uri.c_str(), "");
  EXPECT_STREQ("Invalid uri parameter.\n", cerr.str().c_str());
  // Restore old cerr.
  std::cerr.rdbuf(backup);
}

}
