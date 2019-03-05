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

#include <stdarg.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "unittest/test_utils/mocks/gmock_clean.h"
#include "unittest/test_utils/shell_base_test.h"
#include "utils/utils_general.h"

using mysqlsh::Shell_options;

namespace shcore {
#define IS_CONNECTION_DATA true
#define IS_NULLABLE true
#define AS__STRING(x) std::to_string(x)

std::string session_type_name(mysqlsh::SessionType type) {
  std::string ret_val;

  switch (type) {
    case mysqlsh::SessionType::Auto:
      ret_val = "Auto";
      break;
    case mysqlsh::SessionType::Classic:
      ret_val = "Classic";
      break;
    case mysqlsh::SessionType::X:
      ret_val = "X protocol";
      break;
  }

  return ret_val;
}

std::string shell_mode_name(IShell_core::Mode mode) {
  std::string ret_val;

  switch (mode) {
    case IShell_core::Mode::JavaScript:
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

class Shell_cmdline_options : public tests::Shell_base_test {
 public:
  Shell_cmdline_options() {}

  std::string get_string(const Shell_options::Storage *options,
                         const std::string &option) {
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
      return *options->sock;
    else if (option == "ssl-ca")
      return options->ssl_options.get_ca();
    else if (option == "ssl-cert")
      return options->ssl_options.get_cert();
    else if (option == "ssl-key")
      return options->ssl_options.get_key();
    else if (option == "ssl-capath")
      return options->ssl_options.get_capath();
    else if (option == "ssl-crl")
      return options->ssl_options.get_crl();
    else if (option == "ssl-crlpath")
      return options->ssl_options.get_crlpath();
    else if (option == "ssl-cipher")
      return options->ssl_options.get_cipher();
    else if (option == "tls-version")
      return options->ssl_options.get_tls_version();
    else if (option == "uri")
      return options->uri;
    else if (option == "result_format")
      return options->result_format;
    else if (option == "wrap_json")
      return options->wrap_json;
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
    else if (option == "run_file")
      return options->run_file;
    else if (option == "connect-timeout")
      return options->m_connect_timeout;
    else if (option == "quiet-start")
      return AS__STRING(static_cast<int>(options->quiet_start));
    else if (option == "showColumnTypeInfo")
      return AS__STRING(options->show_column_type_info);
    else if (option == "compress")
      return AS__STRING(options->compress);

    return "";
  }

  void test_option_equal_value(const std::string &option,
                               const std::string &value,
                               bool connection_options,
                               const std::string &target_option = "",
                               const char *target_value = NULL) {
    // Redirect cerr.
    std::streambuf *backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests --option=value
    std::string arg;
    arg.append("--").append(option).append("=").append(value);
    SCOPED_TRACE("TESTING: " + arg);
    char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(arg.c_str()),
                    NULL};
    Shell_options cmd_options(2, argv);
    const Shell_options::Storage &options = cmd_options.get();
    EXPECT_EQ(0, options.exit_code);
    EXPECT_EQ(connection_options, options.has_connection_data());

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

  void test_option_space_value(const std::string &option,
                               const std::string &value,
                               bool connection_options,
                               const std::string &target_option = "",
                               const char *target_value = NULL) {
    // Redirect cerr.
    std::streambuf *backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests --option [value]
    std::string arg;
    arg.append("--").append(option);
    SCOPED_TRACE("TESTING: " + arg + " " + value);
    char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(arg.c_str()),
                    const_cast<char *>(value.c_str()), NULL};
    Shell_options cmd_options(3, argv);
    const Shell_options::Storage &options = cmd_options.get();
    EXPECT_EQ(0, options.exit_code);
    EXPECT_EQ(connection_options, options.has_connection_data());

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

  void test_short_option_value(const std::string &option,
                               const std::string &soption,
                               const std::string &value,
                               bool connection_options,
                               const std::string &target_option = "",
                               const char *target_value = NULL) {
    // Redirect cerr.
    std::streambuf *backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests --ovalue
    std::string arg;
    arg.append("-").append(soption).append(value);
    SCOPED_TRACE("TESTING: " + arg);
    char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(arg.c_str()),
                    NULL};
    Shell_options cmd_options(2, argv);
    const Shell_options::Storage &options = cmd_options.get();
    EXPECT_EQ(0, options.exit_code);
    EXPECT_EQ(connection_options, options.has_connection_data());

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

  void test_short_option_space_value(const std::string &option,
                                     const std::string &soption,
                                     const std::string &value,
                                     bool connection_options,
                                     const std::string &target_option = "",
                                     const char *target_value = NULL) {
    // Redirect cerr.
    std::streambuf *backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests --option [value]
    std::string arg;
    arg.append("-").append(soption);
    SCOPED_TRACE("TESTING: " + arg + " " + value);
    char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(arg.c_str()),
                    const_cast<char *>(value.c_str()), NULL};
    Shell_options cmd_options(3, argv);
    const Shell_options::Storage &options = cmd_options.get();
    EXPECT_EQ(0, options.exit_code);
    EXPECT_EQ(connection_options, options.has_connection_data());

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

  void test_option_space_no_value(const std::string &option, bool valid,
                                  const std::string &defval,
                                  const std::string target_option = "",
                                  const char *target_value = NULL) {
    // Redirect cerr.
    std::streambuf *backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests --option [value]
    std::string arg;
    arg.append("--").append(option);
    SCOPED_TRACE("TESTING: " + arg);
    char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(arg.c_str()),
                    NULL};
    Shell_options cmd_options(2, argv);
    const Shell_options::Storage &options = cmd_options.get();

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

  void test_option_space_empty_value(const std::string &option, bool nullable) {
    // Redirect cerr.
    std::streambuf *backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests --option ""
    std::string arg;
    arg.append("--").append(option);
    SCOPED_TRACE("TESTING: " + arg + " \"\"");
    char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(arg.c_str()),
                    const_cast<char *>(""), NULL};
    Shell_options cmd_options(3, argv);
    const Shell_options::Storage &options = cmd_options.get();

    if (nullable) {
      if (options.exit_code == 1) {
        const std::string message = "Value for --" + option;
        EXPECT_THAT(cerr.str(), ::testing::StartsWith(message));
      }
    } else {
      EXPECT_EQ(1, options.exit_code);
      std::string message = "ut: option ";
      message.append("--").append(option).append(" requires an argument\n");
      EXPECT_STREQ(message.c_str(), cerr.str().c_str());
    }

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }

  void test_short_option_no_value(const std::string &option,
                                  const std::string &soption, bool valid,
                                  const std::string &defval,
                                  const std::string target_option = "",
                                  const char *target_value = NULL) {
    // Redirect cerr.
    std::streambuf *backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests -o
    std::string arg;
    arg.append("-").append(soption);
    SCOPED_TRACE("TESTING: " + arg);
    char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(arg.c_str()),
                    NULL};
    Shell_options cmd_options(2, argv);
    const Shell_options::Storage &options = cmd_options.get();

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
      message.append("-").append(soption).append(" requires an argument\n");
      EXPECT_STREQ(message.c_str(), cerr.str().c_str());
    }

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }

  void test_short_option_space_empty_value(const std::string &option,
                                           const std::string &soption,
                                           bool valid,
                                           const std::string &defval,
                                           const std::string target_option = "",
                                           const char *target_value = NULL) {
    // Redirect cerr.
    std::streambuf *backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests -o ""
    std::string arg;
    arg.append("-").append(soption);
    SCOPED_TRACE("TESTING: " + arg + " \"\"");
    char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(arg.c_str()),
                    const_cast<char *>(""), NULL};
    Shell_options cmd_options(3, argv);
    const Shell_options::Storage &options = cmd_options.get();

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
      message.append("-").append(soption).append(" requires an argument\n");
      EXPECT_STREQ(message.c_str(), cerr.str().c_str());
    }

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }

  void test_option_equal_no_value(const std::string &option, bool valid) {
    // Redirect cerr.
    std::streambuf *backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests --option=
    std::string arg;
    arg.append("--").append(option).append("=");
    SCOPED_TRACE("TESTING: " + arg);
    char *argv[] = {const_cast<char *>("ut"), const_cast<char *>(arg.c_str()),
                    NULL};
    Shell_options options(2, argv);

    if (valid) {
      EXPECT_EQ(0, options.get().exit_code);
      EXPECT_STREQ("", cerr.str().c_str());
    } else {
      EXPECT_EQ(1, options.get().exit_code);
      std::string message = "ut: option ";
      message.append("--").append(option).append(" requires an argument\n");
      EXPECT_STREQ(message.c_str(), cerr.str().c_str());
    }

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }

  void test_deprecated_ssl(const std::string &scope, std::vector<char *> *args,
                           const std::string &warning, const std::string &error,
                           int expected_exit_code,
                           mysqlshdk::db::Ssl_mode mode) {
    // Redirect cerr.
    std::streambuf *cerr_backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Redirect cout.
    std::streambuf *cout_backup = std::cout.rdbuf();
    std::ostringstream cout;
    std::cout.rdbuf(cout.rdbuf());

    SCOPED_TRACE("TESTING: " + scope);
    Shell_options options(args->size() - 1, &(args->at(0)));

    EXPECT_EQ(expected_exit_code, options.get().exit_code);

    if (!expected_exit_code) {
      MY_EXPECT_OUTPUT_CONTAINS(warning.c_str(), cout.str());
      EXPECT_EQ(mode, options.get().ssl_options.get_mode());
    } else {
      EXPECT_STREQ(error.c_str(), cerr.str().c_str());
    }

    // Restore old cerr.
    std::cerr.rdbuf(cerr_backup);
    // Restore old cout.
    std::cout.rdbuf(cout_backup);
  }

  void test_option_with_value(const std::string &option,
                              const std::string &soption,
                              const std::string &value,
                              const std::string &defval,
                              bool is_connection_data, bool nullable,
                              const std::string &target_option = "",
                              const char *target_value = NULL) {
    // --option=<value>
    test_option_equal_value(option, value, is_connection_data, target_option,
                            target_value);

    // --option value
    test_option_space_value(option, value, is_connection_data, target_option,
                            target_value);

    // --option
    test_option_space_no_value(option, !defval.empty() || nullable, defval,
                               target_option);

    // --option ""
    test_option_space_empty_value(option, nullable);

    if (!soption.empty()) {
      // -o<value>
      test_short_option_value(option, soption, value, is_connection_data,
                              target_option, target_value);

      // -o <value>
      test_short_option_space_value(option, soption, value, is_connection_data,
                                    target_option, target_value);

      // -o
      test_short_option_no_value(option, soption, !defval.empty() || nullable,
                                 defval, target_option);

      // -o ""
      test_short_option_space_empty_value(
          option, soption, !defval.empty() || nullable, defval, target_option);
    }

    // --option=
    test_option_equal_no_value(option, !defval.empty() || nullable);
  }

  void test_option_with_no_value(const std::string &option,
                                 const std::string &target_option,
                                 const std::string &target_value) {
    // Redirect cerr.
    std::streambuf *backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    // Tests --option or -o

    SCOPED_TRACE("TESTING: " + option);
    char *argv[] = {const_cast<char *>("ut"),
                    const_cast<char *>(option.c_str()), NULL};
    Shell_options cmd_options(2, argv);
    const Shell_options::Storage &options = cmd_options.get();

    EXPECT_EQ(0, options.exit_code);
    EXPECT_STREQ(target_value.c_str(),
                 get_string(&options, target_option).c_str());
    EXPECT_STREQ("", cerr.str().c_str());

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }

  void test_session_type_conflicts(const std::string &firstArg,
                                   const std::string &secondArg,
                                   const std::string &firstST,
                                   const std::string &secondST, int ret_code) {
    std::streambuf *backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    {
      SCOPED_TRACE("TESTING: " + firstArg + " " + secondArg);
      char *argv[] = {const_cast<char *>("ut"),
                      const_cast<char *>(firstArg.c_str()),
                      const_cast<char *>(secondArg.c_str()), NULL};
      Shell_options options(3, argv);

      EXPECT_EQ(ret_code, options.get().exit_code);

      std::string error = "";
      if (options.get().exit_code) {
        if (firstST.empty())
          error =
              "Automatic protocol detection is enabled, unable to change to " +
              secondST + " with option " + secondArg + "\n";
        else if (secondST.empty())
          error = "Session type already configured to " + firstST +
                  ", automatic protocol detection (-ma) can't be enabled.\n";
        else
          error = "Session type already configured to " + firstST +
                  ", unable to"
                  " change to " +
                  secondST + " with option " + secondArg + "\n";
      }
      EXPECT_STREQ(error.c_str(), cerr.str().c_str());
    }

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }

  void test_conflicting_options(std::string context, size_t argc, char *argv[],
                                std::string error) {
    std::streambuf *backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    SCOPED_TRACE("TESTING: " + context);

    Shell_options options(argc, argv);

    EXPECT_EQ(1, options.get().exit_code);

    EXPECT_STREQ(error.c_str(), cerr.str().c_str());

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }
};

TEST_F(Shell_cmdline_options, default_values) {
  int argc = 0;
  char **argv = nullptr;

  Shell_options cmd_options(argc, argv);
  const Shell_options::Storage &options = cmd_options.get();

  EXPECT_EQ(0, options.exit_code);
  EXPECT_FALSE(options.force);
  EXPECT_FALSE(options.full_interactive);
  EXPECT_FALSE(options.has_connection_data());
  EXPECT_TRUE(options.host.empty());

  EXPECT_EQ(options.initial_mode, IShell_core::Mode::None);

  EXPECT_FALSE(options.interactive);
  EXPECT_EQ(options.log_level, ngcommon::Logger::LOG_INFO);
  EXPECT_EQ("table", options.result_format);
  EXPECT_EQ("off", options.wrap_json);
  EXPECT_EQ(nullptr, options.password);
  EXPECT_FALSE(options.passwords_from_stdin);
  EXPECT_EQ(options.port, 0);
  EXPECT_FALSE(options.prompt_password);
  EXPECT_TRUE(options.protocol.empty());
  EXPECT_FALSE(options.recreate_database);
  EXPECT_TRUE(options.run_file.empty());
  EXPECT_TRUE(options.schema.empty());
  EXPECT_EQ(options.session_type, mysqlsh::SessionType::Auto);
  EXPECT_TRUE(options.sock.is_null());
  EXPECT_TRUE(!options.ssl_options.has_ca());
  EXPECT_TRUE(!options.ssl_options.has_cert());
  EXPECT_TRUE(!options.ssl_options.has_key());
  EXPECT_TRUE(!options.ssl_options.has_capath());
  EXPECT_TRUE(!options.ssl_options.has_crl());
  EXPECT_TRUE(!options.ssl_options.has_crlpath());
  EXPECT_TRUE(!options.ssl_options.has_cipher());
  EXPECT_TRUE(!options.ssl_options.has_tls_version());
  EXPECT_FALSE(options.trace_protocol);
  EXPECT_TRUE(options.uri.empty());
  EXPECT_TRUE(options.user.empty());
  EXPECT_TRUE(options.execute_statement.empty());
  EXPECT_TRUE(options.wizards);
  EXPECT_TRUE(options.default_session_type);
  EXPECT_TRUE(options.m_connect_timeout.empty());
  EXPECT_EQ(Shell_options::Quiet_start::NOT_SET, options.quiet_start);
  EXPECT_FALSE(options.show_column_type_info);
  EXPECT_FALSE(options.compress);
  EXPECT_FALSE(options.default_compress);
}

TEST_F(Shell_cmdline_options, app) {
  test_option_with_value("host", "h", "localhost", "", IS_CONNECTION_DATA,
                         !IS_NULLABLE);
  test_option_with_value("port", "P", "3306", "", IS_CONNECTION_DATA,
                         !IS_NULLABLE);
  test_option_with_value("schema", "D", "sakila", "", IS_CONNECTION_DATA,
                         !IS_NULLABLE);
  test_option_with_value("database", "", "sakila", "", IS_CONNECTION_DATA,
                         !IS_NULLABLE, "schema");
  test_option_with_value("user", "u", "root", "", IS_CONNECTION_DATA,
                         !IS_NULLABLE);
  test_option_with_value("dbuser", "u", "root", "", IS_CONNECTION_DATA,
                         !IS_NULLABLE, "user");
#ifdef _WIN32
  test_option_with_value("socket", "S", "/some/socket/path", "",
                         IS_CONNECTION_DATA, !IS_NULLABLE, "sock");
#else
  test_option_with_value("socket", "S", "/some/socket/path", "",
                         IS_CONNECTION_DATA, IS_NULLABLE, "sock");
#endif
  test_option_with_value("connect-timeout", "", "1000", "", IS_CONNECTION_DATA,
                         !IS_NULLABLE);
  test_option_with_no_value("-C", "compress", "1");
  test_option_with_no_value("--compress", "compress", "1");
  test_option_with_no_value("-p", "prompt_password", "1");

  test_option_equal_value("dbpassword", "mypwd", IS_CONNECTION_DATA,
                          "password");
  test_option_space_value("dbpassword", "mypwd", IS_CONNECTION_DATA, "password",
                          "");
  test_option_space_value("dbpassword", "mypwd", IS_CONNECTION_DATA,
                          "prompt_password", "1");
  test_short_option_value("dbpassword", "p", "mypwd", IS_CONNECTION_DATA,
                          "password");
  test_short_option_space_value("dbpassword", "p", "mypwd", IS_CONNECTION_DATA,
                                "password", "");
  test_short_option_space_value("dbpassword", "p", "mypwd", IS_CONNECTION_DATA,
                                "prompt_password", "1");
  test_option_equal_no_value("dbpassword", true);
  test_option_with_no_value("--dbpassword", "prompt_password", "1");

  test_option_with_value("ssl-ca", "", "some_value", "", IS_CONNECTION_DATA,
                         false);
  test_option_with_value("ssl-cert", "", "some_value", "", IS_CONNECTION_DATA,
                         !IS_NULLABLE);
  test_option_with_value("ssl-key", "", "some_value", "", IS_CONNECTION_DATA,
                         !IS_NULLABLE);

  test_option_with_value("execute", "e", "show databases;", "",
                         !IS_CONNECTION_DATA, !IS_NULLABLE,
                         "execute_statement");

  test_option_with_no_value("--mysql", "session-type",
                            session_type_name(mysqlsh::SessionType::Classic));
  test_option_with_no_value("--mysqlx", "session-type",
                            session_type_name(mysqlsh::SessionType::X));

  test_option_with_no_value("--sql", "session-type",
                            session_type_name(mysqlsh::SessionType::Auto));
  test_option_with_no_value("--sql", "initial-mode",
                            shell_mode_name(IShell_core::Mode::SQL));

  test_option_with_no_value("--sqlc", "session-type",
                            session_type_name(mysqlsh::SessionType::Classic));
  test_option_with_no_value("--sqlc", "initial-mode",
                            shell_mode_name(IShell_core::Mode::SQL));

  test_option_with_no_value("--sqlx", "session-type",
                            session_type_name(mysqlsh::SessionType::X));
  test_option_with_no_value("--sqlx", "initial-mode",
                            shell_mode_name(IShell_core::Mode::SQL));

#ifdef HAVE_V8
  test_option_with_no_value("--javascript", "initial-mode",
                            shell_mode_name(IShell_core::Mode::JavaScript));
  test_option_with_no_value("--js", "initial-mode",
                            shell_mode_name(IShell_core::Mode::JavaScript));
#endif

  test_option_with_no_value("--python", "initial-mode",
                            shell_mode_name(IShell_core::Mode::Python));
  test_option_with_no_value("--py", "initial-mode",
                            shell_mode_name(IShell_core::Mode::Python));

  test_option_with_no_value("--recreate-schema", "recreate_database", "1");

  test_option_with_no_value("--table", "result_format", "table");
  test_option_with_no_value("--tabbed", "result_format", "tabbed");
  test_option_with_no_value("--vertical", "result_format", "vertical");
  test_option_with_no_value("-E", "result_format", "vertical");
  test_option_equal_value("result-format", "table", false, "result_format",
                          "table");
  test_option_equal_value("result-format", "tabbed", false, "result_format",
                          "tabbed");
  test_option_equal_value("result-format", "vertical", false, "result_format",
                          "vertical");
  test_option_equal_value("result-format", "json", false, "result_format",
                          "json");
  test_option_equal_value("result-format", "json/raw", false, "result_format",
                          "json/raw");

  test_option_with_no_value("--json", "wrap_json", "json");
  test_option_equal_value("json", "pretty", false, "wrap_json", "json");
  test_option_equal_value("json", "raw", false, "wrap_json", "json/raw");
  test_option_equal_value("json", "off", false, "wrap_json", "off");

  test_option_with_no_value("--trace-proto", "trace_protocol", "1");
  test_option_with_no_value("--force", "force", "1");
  test_option_with_no_value("--interactive", "interactive", "1");
  test_option_with_no_value("-i", "interactive", "1");
  test_option_with_no_value("--no-wizard", "wizards", "0");
  test_option_with_no_value("--nw", "wizards", "0");
  test_option_with_no_value("--quiet-start", "quiet-start", "1");
  test_option_with_value("quiet-start", "", "2", "1", !IS_CONNECTION_DATA,
                         IS_NULLABLE, "quiet-start", "2");
  test_option_with_no_value("--column-type-info", "showColumnTypeInfo", "1");
  test_option_with_value("interactive", "", "full", "1", !IS_CONNECTION_DATA,
                         IS_NULLABLE, "interactive", "1");
  // test_option_with_value("interactive", "", "full", "1", !IS_CONNECTION_DATA,
  // IS_NULLABLE, "full_interactive", "1");

  test_option_with_no_value("--passwords-from-stdin", "passwords_from_stdin",
                            "1");

  test_option_with_value("file", "f", "/some/file", "", !IS_CONNECTION_DATA,
                         !IS_NULLABLE, "run_file");
}

TEST_F(Shell_cmdline_options, test_session_type_conflicts) {
  test_session_type_conflicts("--sqlc", "--sqlc", "Classic", "Classic", 0);
  test_session_type_conflicts("--sqlc", "--mysql", "Classic", "Classic", 0);
  test_session_type_conflicts("--sqlc", "--mysqlx", "Classic", "X protocol", 1);
  test_session_type_conflicts("--sqlc", "--sqlx", "Classic", "X protocol", 1);

  test_session_type_conflicts("--sqlx", "--sqlx", "X protocol", "X protocol",
                              0);
  test_session_type_conflicts("--sqlx", "--mysqlx", "X protocol", "X protocol",
                              0);
  test_session_type_conflicts("--sqlx", "--mysql", "X protocol", "Classic", 1);
  test_session_type_conflicts("--sqlx", "--sqlc", "X protocol", "Classic", 1);

  test_session_type_conflicts("--mx", "--mysqlx", "X protocol", "X protocol",
                              0);
  test_session_type_conflicts("--mysqlx", "--sqlx", "X protocol", "X protocol",
                              0);
  test_session_type_conflicts("--mx", "--mysql", "X protocol", "Classic", 1);
  test_session_type_conflicts("--mysqlx", "--sqlc", "X protocol", "Classic", 1);
  test_session_type_conflicts("--mysqlx", "--mysql", "X protocol", "Classic",
                              1);

  test_session_type_conflicts("--mc", "--mysql", "Classic", "Classic", 0);
  test_session_type_conflicts("--mysql", "--sqlc", "Classic", "Classic", 0);
  test_session_type_conflicts("--mc", "--mysqlx", "Classic", "X protocol", 1);
  test_session_type_conflicts("--mysql", "--sqlx", "Classic", "X protocol", 1);
  test_session_type_conflicts("--mysql", "--mysqlx", "Classic", "X protocol",
                              1);

  test_session_type_conflicts("-ma", "--mysql", "", "Classic", 1);
  test_session_type_conflicts("--mysql", "-ma", "Classic", "", 1);
  test_session_type_conflicts("-ma", "--mysqlx", "", "X protocol", 1);
  test_session_type_conflicts("--mysqlx", "-ma", "X protocol", "", 1);
  test_session_type_conflicts("--mc", "-ma", "Classic", "", 1);
  test_session_type_conflicts("--mx", "-ma", "X protocol", "", 1);
}

TEST_F(Shell_cmdline_options, test_deprecated_arguments) {
  std::string firstArg, secondArg;

  {
    SCOPED_TRACE("TESTING: deprecated --node argument");
    firstArg = "root@localhost:3301";
    secondArg = "--node";

    char *argv[] = {const_cast<char *>("ut"),
                    const_cast<char *>(firstArg.c_str()),
                    const_cast<char *>(secondArg.c_str()), NULL};

    // Redirect cout.
    std::streambuf *cout_backup = std::cout.rdbuf();
    std::ostringstream cout;
    std::cout.rdbuf(cout.rdbuf());

    Shell_options cmd_options(3, argv);

    // Restore old cout.
    std::cout.rdbuf(cout_backup);

    EXPECT_EQ(0, cmd_options.get().exit_code);
    EXPECT_STREQ(cmd_options.get().uri.c_str(), "root@localhost:3301");
    MY_EXPECT_OUTPUT_CONTAINS(
        "The --node option has been deprecated, "
        "please use --mysqlx instead. (Option has been processed "
        "as --mysqlx).",
        cout.str());
  }

  {
    SCOPED_TRACE("TESTING: deprecated --classic argument");
    firstArg = "root@localhost:3301";
    secondArg = "--classic";

    char *argv[] = {const_cast<char *>("ut"),
                    const_cast<char *>(firstArg.c_str()),
                    const_cast<char *>(secondArg.c_str()), NULL};

    // Redirect cout.
    std::streambuf *cout_backup = std::cout.rdbuf();
    std::ostringstream cout;
    std::cout.rdbuf(cout.rdbuf());

    Shell_options cmd_options(3, argv);

    // Restore old cout.
    std::cout.rdbuf(cout_backup);

    EXPECT_EQ(0, cmd_options.get().exit_code);
    EXPECT_STREQ(cmd_options.get().uri.c_str(), "root@localhost:3301");
    MY_EXPECT_OUTPUT_CONTAINS(
        "The --classic option has been deprecated, "
        "please use --mysql instead. (Option has been processed as "
        "--mysql).",
        cout.str());
  }

  {
    SCOPED_TRACE("TESTING: deprecated --dbpassword argument");
    firstArg = "root@localhost:3301";
    secondArg = "--dbpassword=pass";

    char *argv[] = {const_cast<char *>("ut"),
                    const_cast<char *>(firstArg.c_str()),
                    const_cast<char *>(secondArg.c_str()), NULL};

    // Redirect cout.
    std::streambuf *cout_backup = std::cout.rdbuf();
    std::ostringstream cout;
    std::cout.rdbuf(cout.rdbuf());

    Shell_options cmd_options(3, argv);

    // Restore old cout.
    std::cout.rdbuf(cout_backup);

    EXPECT_EQ(0, cmd_options.get().exit_code);
    EXPECT_STREQ(cmd_options.get().uri.c_str(), "root@localhost:3301");
    MY_EXPECT_OUTPUT_CONTAINS(
        "The --dbpassword option has been deprecated, please use --password "
        "instead.",
        cout.str());
  }

  {
    SCOPED_TRACE("TESTING: deprecated --dbuser argument");
    firstArg = "root@localhost:3301";
    secondArg = "--dbuser=root";

    char *argv[] = {const_cast<char *>("ut"),
                    const_cast<char *>(firstArg.c_str()),
                    const_cast<char *>(secondArg.c_str()), NULL};

    // Redirect cout.
    std::streambuf *cout_backup = std::cout.rdbuf();
    std::ostringstream cout;
    std::cout.rdbuf(cout.rdbuf());

    Shell_options cmd_options(3, argv);

    // Restore old cout.
    std::cout.rdbuf(cout_backup);

    EXPECT_EQ(0, cmd_options.get().exit_code);
    EXPECT_STREQ(cmd_options.get().uri.c_str(), "root@localhost:3301");
    MY_EXPECT_OUTPUT_CONTAINS(
        "The --dbuser option has been deprecated, please use --user instead. "
        "(Option has been processed as --user=root).",
        cout.str());
  }
}

TEST_F(Shell_cmdline_options, test_positional_argument) {
  // Redirect cerr.
  std::streambuf *backup = std::cerr.rdbuf();
  std::ostringstream cerr;
  std::cerr.rdbuf(cerr.rdbuf());
  std::string firstArg, secondArg;

  // Using a correct uri should yield no error
  {
    SCOPED_TRACE("TESTING: uri as positional argument.");
    firstArg = "root@localhost:3301";

    char *argv[] = {const_cast<char *>("ut"),
                    const_cast<char *>(firstArg.c_str()), NULL};
    Shell_options cmd_options(2, argv);
    const Shell_options::Storage &options = cmd_options.get();

    EXPECT_EQ(0, options.exit_code);
    EXPECT_STREQ(options.uri.c_str(), "root@localhost:3301");
    EXPECT_STREQ("", cerr.str().c_str());
  }

  // Using a correct uri plus the --uri option
  {
    SCOPED_TRACE(
        "TESTING: uri as positional argument followed by --uri option");
    firstArg = "root@localhost:3301";
    secondArg = "--uri=user2:pass@localhost";

    char *argv2[] = {const_cast<char *>("ut"),
                     const_cast<char *>(firstArg.c_str()),
                     const_cast<char *>(secondArg.c_str()), NULL};
    Shell_options cmd_options(3, argv2);
    const Shell_options::Storage &options = cmd_options.get();

    EXPECT_EQ(0, options.exit_code);
    EXPECT_STREQ(options.uri.c_str(), "user2:pass@localhost");
    EXPECT_STREQ("", cerr.str().c_str());
  }

  {
    SCOPED_TRACE(
        "TESTING: --uri option followed by uri as positional argument");
    firstArg = "--uri=user2@localhost";
    secondArg = "root:pass@localhost:3301";

    char *argv3[] = {const_cast<char *>("ut"),
                     const_cast<char *>(firstArg.c_str()),
                     const_cast<char *>(secondArg.c_str()), NULL};

    Shell_options cmd_options(3, argv3);
    const Shell_options::Storage &options = cmd_options.get();

    EXPECT_EQ(0, options.exit_code);
    EXPECT_STREQ(options.uri.c_str(), "root:pass@localhost:3301");
    EXPECT_STREQ("", cerr.str().c_str());
  }

  // Using an invalid uri as positional argument
  {
    SCOPED_TRACE("TESTING: invalid uri as positional argument");
    firstArg = "not:valid_uri";

    char *argv4[] = {const_cast<char *>("ut"),
                     const_cast<char *>(firstArg.c_str()), NULL};

    Shell_options cmd_options(2, argv4);
    const Shell_options::Storage &options = cmd_options.get();
    EXPECT_EQ(1, options.exit_code);
    EXPECT_STREQ("Invalid URI: Illegal character [v] found at position 4\n",
                 cerr.str().c_str());
  }
  // Restore old cerr.
  std::cerr.rdbuf(backup);
}

TEST_F(Shell_cmdline_options, conflicts_session_type) {
  {
    auto error =
        "The given URI conflicts with the --mysql session type option.\n";

    char *argv0[] = {const_cast<char *>("ut"), const_cast<char *>("--mysql"),
                     const_cast<char *>("--uri=mysqlx://root@localhost"), NULL};

    test_conflicting_options("--mysql --uri", 3, argv0, error);
  }

  {
    auto error =
        "The given URI conflicts with the --sqlc session type option.\n";
    char *argv0[] = {const_cast<char *>("ut"), const_cast<char *>("--sqlc"),
                     const_cast<char *>("--uri=mysqlx://root@localhost"), NULL};

    test_conflicting_options("--mysql --uri", 3, argv0, error);
  }

  {
    auto error =
        "The given URI conflicts with the --mysqlx session type option.\n";

    char *argv1[] = {const_cast<char *>("ut"), const_cast<char *>("--mysqlx"),
                     const_cast<char *>("--uri=mysql://root@localhost"), NULL};

    test_conflicting_options("--mysql --uri", 3, argv1, error);
  }

  {
    auto error =
        "The given URI conflicts with the --sqlx session type option.\n";

    char *argv1[] = {const_cast<char *>("ut"), const_cast<char *>("--sqlx"),
                     const_cast<char *>("--uri=mysql://root@localhost"), NULL};

    test_conflicting_options("--mysql --uri", 3, argv1, error);
  }
}

TEST_F(Shell_cmdline_options, conflicts_user) {
  auto error =
      "Conflicting options: provided user name differs from the "
      "user in the URI.\n";

  char *argv0[] = {const_cast<char *>("ut"), const_cast<char *>("--user=guest"),
                   const_cast<char *>("--uri=mysqlx://root@localhost"), NULL};

  test_conflicting_options("--user --uri", 3, argv0, error);
}

TEST_F(Shell_cmdline_options, conflicts_password) {
  auto error =
      "Conflicting options: provided password differs from the "
      "password in the URI.\n";

  char pwd[] = {"--password=example"};
  char uri[] = {"--uri=mysqlx://root:password@localhost"};
  char *argv0[] = {const_cast<char *>("ut"), pwd, uri, NULL};

  test_conflicting_options("--password --uri", 3, argv0, error);
}

TEST_F(Shell_cmdline_options, conflicts_host) {
  auto error =
      "Conflicting options: provided host differs from the "
      "host in the URI.\n";

  char uri[] = "--uri=mysqlx://root:password@localhost";
  char *argv0[] = {const_cast<char *>("ut"),
                   const_cast<char *>("--host=127.0.0.1"), uri, NULL};

  test_conflicting_options("--host --uri", 3, argv0, error);
}

TEST_F(Shell_cmdline_options, conflicts_output) {
  auto error =
      "Conflicting options: resultFormat cannot be set to 'table' when --json "
      "option implying 'json' value is used.\n";

  char *argv0[] = {const_cast<char *>("ut"), const_cast<char *>("--json"),
                   const_cast<char *>("--table"), NULL};

  test_conflicting_options("--json --table", 3, argv0, error);

  char *argv1[] = {const_cast<char *>("ut"), const_cast<char *>("--table"),
                   const_cast<char *>("--json"), NULL};

  test_conflicting_options("--table --json", 3, argv1, error);

  char *argv2[] = {const_cast<char *>("ut"),
                   const_cast<char *>("--result-format=meh"), NULL};

  test_conflicting_options("--result-format=meh", 2, argv2,
                           "The acceptable values for the option "
                           "--result-format are: tabbed, table, vertical, json "
                           "or json/raw.\n");
}

#ifdef _WIN32
#define SOCKET_NAME "named pipe"
#else  // !_WIN32
#define SOCKET_NAME "socket"
#endif  // !_WIN32

TEST_F(Shell_cmdline_options, conflicts_host_socket) {
  auto error = "Conflicting options: " SOCKET_NAME
               " cannot be used if host is "
#ifdef _WIN32
               "neither '.' nor 'localhost'.\n";
#else   // !_WIN32
               "not 'localhost'.\n";
#endif  // !_WIN32

  char *argv0[] = {const_cast<char *>("ut"),
                   const_cast<char *>("--uri=root@127.0.0.1"),
                   const_cast<char *>("--socket=/some/socket/path"), NULL};
  test_conflicting_options("--uri --socket", 3, argv0, error);

  char *argv1[] = {const_cast<char *>("ut"),
                   const_cast<char *>("--host=127.0.0.1"),
                   const_cast<char *>("--socket=/some/socket/path"), NULL};
  test_conflicting_options("--host --socket", 3, argv1, error);

#ifndef _WIN32
  char *argv2[] = {const_cast<char *>("ut"),
                   const_cast<char *>("--host=127.0.0.1"),
                   const_cast<char *>("--socket"), NULL};
  test_conflicting_options("--host --socket", 3, argv2, error);
#endif
}

TEST_F(Shell_cmdline_options, conflicts_port) {
  auto error =
      "Conflicting options: provided port differs from the "
      "port in the URI.\n";

  char uri[] = {"--uri=mysqlx://root:password@localhost:3307"};
  char *argv0[] = {const_cast<char *>("ut"), const_cast<char *>("--port=3306"),
                   uri, NULL};
  test_conflicting_options("--port --uri", 3, argv0, error);
}

TEST_F(Shell_cmdline_options, conflicts_socket) {
  auto error = "Conflicting options: provided " SOCKET_NAME
               " differs from the " SOCKET_NAME " in the URI.\n";

  char *argv0[] = {const_cast<char *>("ut"),
                   const_cast<char *>("--socket=/path/to/socket"),
#ifdef _WIN32
                   const_cast<char *>("--uri=mysql://root@\\\\.\\named.pipe"),
#else   // !_WIN32
                   const_cast<char *>("--uri=mysqlx://root@/socket"),
#endif  // !_WIN32
                   NULL};

  test_conflicting_options("--socket --uri", 3, argv0, error);
}

TEST_F(Shell_cmdline_options, conflicting_port_and_socket) {
  std::string error0 = "Conflicting options: port and " SOCKET_NAME
                       " cannot be used together.\n";

  char *argv0[] = {const_cast<char *>("ut"), const_cast<char *>("--port=3307"),
                   const_cast<char *>("--socket=/some/weird/path"), NULL};

  test_conflicting_options("--port --socket", 3, argv0, error0);

#ifndef _WIN32
  char *argv0b[] = {const_cast<char *>("ut"), const_cast<char *>("--port=3307"),
                    const_cast<char *>("--socket"), NULL};

  test_conflicting_options("--port --socket", 3, argv0b, error0);
#endif

  auto error1 =
      "Conflicting options: port cannot be used if the URI "
      "contains a " SOCKET_NAME ".\n";

  char *argv1[] = {const_cast<char *>("ut"),
#ifdef _WIN32
                   const_cast<char *>("--uri=root@\\\\.\\named.pipe"),
#else   // !_WIN32
                   const_cast<char *>("--uri=root@/socket"),
#endif  // !_WIN32
                   const_cast<char *>("--port=3306"), NULL};

  test_conflicting_options("--uri --port", 3, argv1, error1);

  auto error2 = "Conflicting options: " SOCKET_NAME
                " cannot be used if the URI contains a port.\n";

  char *argv2[] = {const_cast<char *>("ut"),
                   const_cast<char *>("--uri=root@localhost:3306"),
                   const_cast<char *>("--socket=/some/socket/path"), NULL};

  test_conflicting_options("--uri --socket", 3, argv2, error2);
}

TEST_F(Shell_cmdline_options, test_uri_with_password) {
  // Redirect cerr.
  std::streambuf *backup = std::cerr.rdbuf();
  std::ostringstream cerr;
  std::cerr.rdbuf(cerr.rdbuf());
  std::string firstArg, secondArg;

  SCOPED_TRACE("TESTING: user with password in uri");
  firstArg = "--uri=root:pass@localhost:3301";

  char *argv[]{const_cast<char *>("ut"), const_cast<char *>(firstArg.c_str()),
               NULL};
  std::shared_ptr<Shell_options> options =
      std::make_shared<Shell_options>(2, argv);

  EXPECT_EQ(0, options->get().exit_code);
  EXPECT_STREQ(argv[1], "--uri=root:****@localhost:3301");
  EXPECT_STREQ("", cerr.str().c_str());

  SCOPED_TRACE("TESTING: single letter user without password in uri");
  firstArg = "--uri=r:@localhost:3301";

  char *argv1[]{const_cast<char *>("ut"), const_cast<char *>(firstArg.c_str()),
                NULL};
  options = std::make_shared<Shell_options>(2, argv1);

  EXPECT_EQ(0, options->get().exit_code);
  EXPECT_STREQ(argv1[1], "--uri=r:@localhost:3301");
  EXPECT_STREQ("", cerr.str().c_str());

  SCOPED_TRACE("TESTING: single letter user with password in uri");
  firstArg = "--uri=r:pass@localhost:3301";

  char *argv2[]{const_cast<char *>("ut"), const_cast<char *>(firstArg.c_str()),
                NULL};
  options = std::make_shared<Shell_options>(2, argv2);

  EXPECT_EQ(0, options->get().exit_code);
  EXPECT_STREQ(argv2[1], "--uri=r:****@localhost:3301");
  EXPECT_STREQ("", cerr.str().c_str());

  SCOPED_TRACE("TESTING: single letter user with single letter password");
  firstArg = "--uri=r:r@localhost:3301";

  char *argv3[]{const_cast<char *>("ut"), const_cast<char *>(firstArg.c_str()),
                NULL};
  options = std::make_shared<Shell_options>(2, argv3);

  EXPECT_EQ(0, options->get().exit_code);
  EXPECT_STREQ(argv3[1], "--uri=r:*@localhost:3301");
  EXPECT_STREQ("", cerr.str().c_str());

  SCOPED_TRACE("TESTING: user with password with special sign encoded in uri");
  firstArg = "--uri=root:123%21@localhost:3301";

  char *argv4[]{const_cast<char *>("ut"), const_cast<char *>(firstArg.c_str()),
                NULL};
  options = std::make_shared<Shell_options>(2, argv4);

  EXPECT_EQ(0, options->get().exit_code);
  EXPECT_STREQ(argv4[1], "--uri=root:******@localhost:3301");
  EXPECT_STREQ("", cerr.str().c_str());

  SCOPED_TRACE(
      "TESTING: anonymous uri with with password with special sign encoded");
  firstArg = "root:123%21@localhost:3301";

  char *argv5[]{const_cast<char *>("ut"), const_cast<char *>(firstArg.c_str()),
                NULL};
  options = std::make_shared<Shell_options>(2, argv5);

  EXPECT_EQ(0, options->get().exit_code);
  EXPECT_STREQ(argv5[1], "root:******@localhost:3301");
  EXPECT_STREQ("", cerr.str().c_str());

  // Restore old cerr.
  std::cerr.rdbuf(backup);
}

TEST_F(Shell_cmdline_options, test_deprecated_ssl) {
  std::string warning =
      "The --ssl option has been deprecated, please use --ssl-mode instead. ";
  {
    std::vector<char *> options = {const_cast<char *>("ut"),
                                   const_cast<char *>("--ssl"),
                                   const_cast<char *>("something"), NULL};

    std::string error =
        "--ssl-mode must be any of [DISABLED, PREFERRED, REQUIRED, VERIFY_CA, "
        "VERIFY_IDENTITY]\n";

    test_deprecated_ssl("--ssl=something", &options, "", error, 1,
                        mysqlshdk::db::Ssl_mode::Preferred);
    // This last param is
    // ignored on this case
  }
  {
    std::vector<char *> options = {const_cast<char *>("ut"),
                                   const_cast<char *>("--ssl"), NULL};
    std::string mywarning(warning);
    mywarning.append("(Option has been processed as --ssl-mode=REQUIRED).");
    test_deprecated_ssl("--ssl", &options, mywarning, "", 0,
                        mysqlshdk::db::Ssl_mode::Required);
  }
  {
    std::vector<char *> options = {const_cast<char *>("ut"),
                                   const_cast<char *>("--ssl=1"), NULL};
    std::string mywarning(warning);
    mywarning.append("(Option has been processed as --ssl-mode=REQUIRED).");
    test_deprecated_ssl("--ssl=1", &options, mywarning, "", 0,
                        mysqlshdk::db::Ssl_mode::Required);
  }
  {
    std::vector<char *> options = {const_cast<char *>("ut"),
                                   const_cast<char *>("--ssl=yes"), NULL};
    std::string mywarning(warning);
    mywarning.append("(Option has been processed as --ssl-mode=REQUIRED).");
    test_deprecated_ssl("--ssl=yes", &options, mywarning, "", 0,
                        mysqlshdk::db::Ssl_mode::Required);
  }

  {
    std::vector<char *> options = {const_cast<char *>("ut"),
                                   const_cast<char *>("--ssl=0"), NULL};
    std::string mywarning(warning);
    mywarning.append("(Option has been processed as --ssl-mode=DISABLED).");
    test_deprecated_ssl("--ssl=0", &options, mywarning, "", 0,
                        mysqlshdk::db::Ssl_mode::Disabled);
  }
  {
    std::vector<char *> options = {const_cast<char *>("ut"),
                                   const_cast<char *>("--ssl=no"), NULL};
    std::string mywarning(warning);
    mywarning.append("(Option has been processed as --ssl-mode=DISABLED).");
    test_deprecated_ssl("--ssl=no", &options, mywarning, "", 0,
                        mysqlshdk::db::Ssl_mode::Disabled);
  }
}

TEST_F(Shell_cmdline_options, dict_access_for_named_options) {
  Shell_options options(0, nullptr);

  for (const std::string &optname : options.get_named_options()) {
    EXPECT_NO_THROW(options.get(optname));
  }
}

TEST_F(Shell_cmdline_options, invalid_connect_timeout) {
  auto invalid_values = {"-1", "10.0", "whatever"};

  for (auto value : invalid_values) {
    std::string option("--connect-timeout=");
    option.append(value);
    char *argv[]{const_cast<char *>("ut"), const_cast<char *>(option.c_str()),
                 NULL};

    std::string error("Invalid value '");
    error.append(value);
    error.append(
        "' for 'connect-timeout'. The connection timeout value must "
        "be a positive integer (including 0).\n");

    std::streambuf *backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    Shell_options opts(3, argv);

    EXPECT_EQ(1, opts.get().exit_code);

    EXPECT_STREQ(error.c_str(), cerr.str().c_str());

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }
}

#ifdef WITH_OCI
TEST_F(Shell_cmdline_options, conflicts_oci) {
  auto error =
      "Conflicting options: --oci can not be used unless initial mode is "
      "Python.\n";
  std::vector<std::string> options = {"--js", "--javascript", "--sql", "--sqlc",
                                      "--sqlc"};

  for (const auto &option : options) {
    char *argv0[] = {const_cast<char *>("ut"),
                     const_cast<char *>(option.c_str()),
                     const_cast<char *>("--oci"), NULL};

    test_conflicting_options(option + " --oci", 3, argv0, error);
  }
}
#endif

}  // namespace shcore
