/*
 * Copyright (c) 2014, 2023, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdarg.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/test_utils/mocks/gmock_clean.h"
#include "unittest/test_utils/shell_base_test.h"

using mysqlsh::Shell_options;

namespace shcore {
#define IS_CONNECTION_DATA true
#define IS_NULLABLE true
#define AS__STRING(x) std::to_string(x)

std::string session_type_name(mysqlsh::SessionType type) {
  std::string ret_val;

  switch (type) {
    case mysqlsh::SessionType::Auto:
      ret_val = "auto";
      break;
    case mysqlsh::SessionType::Classic:
      ret_val = "mysql";
      break;
    case mysqlsh::SessionType::X:
      ret_val = "mysqlx";
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
  Shell_cmdline_options() {
    mycnf_path = getenv("TMPDIR");
    mycnf_path += "/testmy.cnf";

    defaults_file = "--defaults-file=" + mycnf_path;
  }

  std::string mycnf_path;
  std::string defaults_file;

  void make_mycnf(const std::string &text) { create_file(mycnf_path, text); }

  std::string get_string(const Shell_options::Storage *options,
                         const std::string &option) {
    if (option == "host")
      return options->connection_options().get_host();
    else if (option == "user")
      return options->connection_options().get_user();
    else if (option == "password" || option == "password1")
      return options->connection_options().get_mfa_passwords()[0].has_value()
                 ? std::string(
                       *options->connection_options().get_mfa_passwords()[0])
                 : "";
    else if (option == "password2")
      return options->connection_options().get_mfa_passwords()[1].has_value()
                 ? std::string(
                       *options->connection_options().get_mfa_passwords()[1])
                 : "";
    else if (option == "password3")
      return options->connection_options().get_mfa_passwords()[2].has_value()
                 ? std::string(
                       *options->connection_options().get_mfa_passwords()[2])
                 : "";
    else if (option == "port")
      return AS__STRING(options->connection_options().get_port());
    else if (option == "schema")
      return options->connection_options().get_schema();
    else if (option == "sock")
      return options->connection_options().has_socket()
                 ? options->connection_options().get_socket()
                 : "";
    else if (option == "ssl-ca")
      return options->connection_options().get_ssl_options().get_ca();
    else if (option == "ssl-cert")
      return options->connection_options().get_ssl_options().get_cert();
    else if (option == "ssl-key")
      return options->connection_options().get_ssl_options().get_key();
    else if (option == "ssl-capath")
      return options->connection_options().get_ssl_options().get_capath();
    else if (option == "ssl-crl")
      return options->connection_options().get_ssl_options().get_crl();
    else if (option == "ssl-crlpath")
      return options->connection_options().get_ssl_options().get_crlpath();
    else if (option == "ssl-cipher")
      return options->connection_options().get_ssl_options().get_cipher();
    else if (option == "tls-version")
      return options->connection_options().get_ssl_options().get_tls_version();
    else if (option == "tls-ciphersuites")
      return options->connection_options()
          .get_ssl_options()
          .get_tls_ciphersuites();
    else if (option == "uri")
      return options->connection_options().as_uri();
    else if (option == "result_format")
      return options->result_format;
    else if (option == "wrap_json")
      return options->wrap_json;
    else if (option == "session_type" || option == "session-type")
      return options->connection_options().has_scheme()
                 ? options->connection_options().get_scheme()
                 : "auto";
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
    else if (option == "wizards")
      return AS__STRING(options->wizards);
    else if (option == "execute_statement")
      return options->execute_statement;
    else if (option == "run_file")
      return options->run_file;
    else if (option == "connect-timeout")
      return std::to_string(
          options->connection_options().get_connect_timeout());
    else if (option == "quiet-start")
      return AS__STRING(static_cast<int>(options->quiet_start));
    else if (option == "showColumnTypeInfo")
      return AS__STRING(options->show_column_type_info);
    else if (option == "compress")
      return options->connection_options().get_compression();
    else if (option == "mysqlPluginDir")
      return options->mysql_plugin_dir;
    else if (option == "log-sql")
      return options->log_sql;
#ifdef _WIN32
    else if (option == "plugin-authentication-kerberos-client-mode")
      return options->connection_options().get_kerberos_auth_mode();
#endif

    throw std::logic_error(option);
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

  void test_option_equal_invalid_value(const std::string &option,
                                       const std::string &value,
                                       const std::string &error) {
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
    EXPECT_EQ(1, options.exit_code);

    EXPECT_STREQ(error.c_str(), cerr.str().c_str());

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  }

  void test_option_equal_invalid_values(const std::string &option,
                                        const std::vector<std::string> &values,
                                        const std::string &error) {
    for (const auto &v : values) {
      test_option_equal_invalid_value(option, v, error);
    }
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
      EXPECT_EQ(
          mode,
          options.get().connection_options().get_ssl_options().get_mode());
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

  void test_option_with_value_list(const std::string &option,
                                   const std::string &soption,
                                   const std::vector<std::string> &values,
                                   const std::string &defval,
                                   bool is_connection_data, bool nullable,
                                   const std::string &target_option = "",
                                   const char *target_value = NULL) {
    for (const auto &v : values) {
      test_option_with_value(option, soption, v, defval, is_connection_data,
                             nullable, target_option, target_value);
    }
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
                                   const std::string &secondArg, int ret_code) {
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
        std::string first_opt;
        std::string second_opt;
        if (firstArg[0] == '-') {
          first_opt = "option " + firstArg;
        } else {
          if (shcore::str_beginswith(firstArg, "mysql:"))
            first_opt = "URI scheme mysql://";
          else
            first_opt = "URI scheme mysqlx://";
        }
        if (secondArg[0] == '-') {
          second_opt = "Option " + secondArg;
        } else {
          if (shcore::str_beginswith(secondArg, "mysql:"))
            second_opt = "URI scheme mysql://";
          else
            second_opt = "URI scheme mysqlx://";
        }

        error = second_opt + " cannot be combined with " + first_opt + "\n";
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

  void test_overriding_options(std::string context, size_t argc, char *argv[],
                               const std::string &option,
                               const std::string &value) {
    SCOPED_TRACE("TESTING: " + context);

    Shell_options options(argc, argv);

    EXPECT_EQ(0, options.get().exit_code);

    EXPECT_EQ(value, get_string(&options.get(), option));
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

  EXPECT_EQ(options.initial_mode, IShell_core::Mode::None);

  EXPECT_FALSE(options.interactive);
  EXPECT_EQ(options.log_level, shcore::Logger::LOG_INFO);
  EXPECT_EQ("table", options.result_format);
  EXPECT_EQ("off", options.wrap_json);
  EXPECT_FALSE(options.connection_options().get_mfa_passwords()[0].has_value());
  EXPECT_FALSE(options.connection_options().get_mfa_passwords()[1].has_value());
  EXPECT_FALSE(options.connection_options().get_mfa_passwords()[2].has_value());
  EXPECT_FALSE(options.passwords_from_stdin);
  EXPECT_FALSE(options.prompt_password);
  EXPECT_TRUE(options.protocol.empty());
  EXPECT_FALSE(options.recreate_database);
  EXPECT_TRUE(options.run_file.empty());
  EXPECT_TRUE(!options.connection_options().has_schema());
  EXPECT_TRUE(!options.connection_options().has_scheme());
  EXPECT_TRUE(!options.connection_options().has_socket());
  EXPECT_TRUE(!options.connection_options().get_ssl_options().has_ca());
  EXPECT_TRUE(!options.connection_options().get_ssl_options().has_cert());
  EXPECT_TRUE(!options.connection_options().get_ssl_options().has_key());
  EXPECT_TRUE(!options.connection_options().get_ssl_options().has_capath());
  EXPECT_TRUE(!options.connection_options().get_ssl_options().has_crl());
  EXPECT_TRUE(!options.connection_options().get_ssl_options().has_crlpath());
  EXPECT_TRUE(!options.connection_options().get_ssl_options().has_cipher());
  EXPECT_TRUE(
      !options.connection_options().get_ssl_options().has_tls_version());
  EXPECT_FALSE(
      options.connection_options().get_ssl_options().has_tls_ciphersuites());
  EXPECT_FALSE(options.trace_protocol);
  EXPECT_TRUE(options.execute_statement.empty());
  EXPECT_TRUE(options.wizards);
  EXPECT_FALSE(
      options.connection_options().has(mysqlshdk::db::kConnectTimeout));
  EXPECT_EQ(Shell_options::Quiet_start::NOT_SET, options.quiet_start);
  EXPECT_FALSE(options.show_column_type_info);
  EXPECT_TRUE(!options.connection_options().has_compression());
  EXPECT_FALSE(options.default_compress);
  EXPECT_TRUE(!options.connection_options().has_compression_algorithms());
  EXPECT_TRUE(!options.connection_options().has_compression_level());
  EXPECT_EQ("error", options.log_sql);
  EXPECT_EQ("*SELECT*:SHOW*", options.log_sql_ignore);
  EXPECT_EQ("*IDENTIFIED*:*PASSWORD*", options.log_sql_ignore_unsafe);

#ifdef _WIN32
  EXPECT_FALSE(options.connection_options().has_kerberos_auth_mode());
#endif
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
  test_option_with_value("connect-timeout", "", "1000", "", !IS_CONNECTION_DATA,
                         !IS_NULLABLE);
  test_option_with_no_value("-C", "compress", "REQUIRED");
  test_option_with_no_value("--compress", "compress", "REQUIRED");
  test_option_with_no_value("-p", "prompt_password", "1");
  test_option_with_no_value("--password", "prompt_password", "1");
  test_option_with_no_value("--password1", "prompt_password", "1");

  test_option_equal_value("dbpassword", "mypwd", IS_CONNECTION_DATA,
                          "password");
  test_option_equal_value("password1", "mypwd", IS_CONNECTION_DATA, "password");
  test_option_equal_value("password2", "mypwd", IS_CONNECTION_DATA);
  test_option_equal_value("password3", "mypwd", IS_CONNECTION_DATA);

  test_option_space_value("dbpassword", "mypwd", IS_CONNECTION_DATA, "password",
                          "");
  test_option_space_value("dbpassword", "mypwd", IS_CONNECTION_DATA,
                          "prompt_password", "1");
  test_option_space_value("password1", "mypwd", IS_CONNECTION_DATA,
                          "prompt_password", "1");
  test_option_space_value("password2", "mypwd", IS_CONNECTION_DATA);
  test_option_space_value("password3", "mypwd", IS_CONNECTION_DATA);
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
  test_option_with_value("tls-ciphersuites", "", "some_value", "",
                         IS_CONNECTION_DATA, !IS_NULLABLE);

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

  test_option_with_value("mysql-plugin-dir", "", "/some/path", "",
                         !IS_CONNECTION_DATA, IS_NULLABLE, "mysqlPluginDir");

  test_option_with_value_list(
      "log-sql", "", {"off", "error", "on", "all", "ALL", "unfiltered"}, "",
      false, false);

  test_option_equal_invalid_values("log-sql", {"filtered"},
                                   "--log-sql: The log level value must be any "
                                   "of {off, error, on, all, unfiltered}.\n");

#ifdef _WIN32
  test_option_with_value_list("plugin-authentication-kerberos-client-mode", "",
                              {"GSSAPI", "SSPI"}, "", false, false);

  test_option_equal_invalid_value(
      "plugin-authentication-kerberos-client-mode", "GSSPI",
      "Invalid value: GSSPI. Allowed values: SSPI, GSSAPI.\n");
#endif
}

TEST_F(Shell_cmdline_options, test_session_type_conflicts) {
  test_session_type_conflicts("--sqlc", "--sqlc", 0);
  test_session_type_conflicts("--sqlc", "--mysql", 0);
  test_session_type_conflicts("--sqlc", "--mysqlx", 1);
  test_session_type_conflicts("--sqlc", "--sqlx", 1);

  test_session_type_conflicts("--sqlx", "--sqlx", 0);
  test_session_type_conflicts("--sqlx", "--mysqlx", 0);
  test_session_type_conflicts("--sqlx", "--mysql", 1);
  test_session_type_conflicts("--sqlx", "--sqlc", 1);

  test_session_type_conflicts("--mx", "--mysqlx", 0);
  test_session_type_conflicts("--mysqlx", "--sqlx", 0);
  test_session_type_conflicts("--mx", "--mysql", 0);
  test_session_type_conflicts("--mysqlx", "--sqlc", 1);
  test_session_type_conflicts("--mysqlx", "--mysql", 0);

  test_session_type_conflicts("--mc", "--mysql", 0);
  test_session_type_conflicts("--mysql", "--sqlc", 0);
  test_session_type_conflicts("--mc", "--mysqlx", 0);
  test_session_type_conflicts("--mysql", "--sqlx", 1);
  test_session_type_conflicts("--mysql", "--mysqlx", 0);

  test_session_type_conflicts("-ma", "--mysql", 0);
  test_session_type_conflicts("--mysql", "-ma", 0);
  test_session_type_conflicts("-ma", "--mysqlx", 0);
  test_session_type_conflicts("--mysqlx", "-ma", 0);
  test_session_type_conflicts("--mc", "-ma", 0);
  test_session_type_conflicts("--mx", "-ma", 0);

  test_session_type_conflicts("mysql://root@localhost", "--sqlc", 0);
  test_session_type_conflicts("mysql://root@localhost", "--mysql", 0);
  test_session_type_conflicts("mysql://root@localhost", "-ma", 0);

  test_session_type_conflicts("mysqlx://root@localhost", "--sqlx", 0);
  test_session_type_conflicts("mysqlx://root@localhost", "--mysqlx", 0);
  test_session_type_conflicts("mysqlx://root@localhost", "-ma", 0);

  test_session_type_conflicts("mysql://root@localhost", "--sqlx", 1);
  test_session_type_conflicts("mysqlx://root@localhost", "--sqlc", 1);

  test_session_type_conflicts("mysql://root@localhost", "--mysqlx", 0);
  test_session_type_conflicts("mysqlx://root@localhost", "--mysql", 0);
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
    EXPECT_STREQ(cmd_options.get().connection_options().as_uri().c_str(),
                 "mysqlx://root@localhost:3301");
    MY_EXPECT_OUTPUT_CONTAINS(
        "The --node option was deprecated, "
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
    EXPECT_STREQ(cmd_options.get().connection_options().as_uri().c_str(),
                 "mysql://root@localhost:3301");
    MY_EXPECT_OUTPUT_CONTAINS(
        "The --classic option was deprecated, "
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
    EXPECT_STREQ(cmd_options.get().connection_options().as_uri().c_str(),
                 "root@localhost:3301");
    MY_EXPECT_OUTPUT_CONTAINS(
        "The --dbpassword option was deprecated, please use --password "
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
    EXPECT_STREQ(cmd_options.get().connection_options().as_uri().c_str(),
                 "root@localhost:3301");
    MY_EXPECT_OUTPUT_CONTAINS(
        "The --dbuser option was deprecated, please use --user instead.",
        cout.str());
  }
  {
    SCOPED_TRACE("TESTING: deprecated --recreate-schema argument");
    firstArg = "root@localhost:3301/some-schema";
    secondArg = "--recreate-schema";

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
    EXPECT_STREQ(cmd_options.get().connection_options().as_uri().c_str(),
                 "root@localhost:3301/some-schema");
    MY_EXPECT_OUTPUT_CONTAINS("The --recreate-schema option was deprecated.",
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
    EXPECT_STREQ(options.connection_options().as_uri().c_str(),
                 "root@localhost:3301");
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
    EXPECT_STREQ(options.connection_options()
                     .as_uri(mysqlshdk::db::uri::formats::full())
                     .c_str(),
                 "user2:pass@localhost");
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
    EXPECT_STREQ(options.connection_options()
                     .as_uri(mysqlshdk::db::uri::formats::full())
                     .c_str(),
                 "root:pass@localhost:3301");
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

TEST_F(Shell_cmdline_options, override_session_type) {
  // first option then uri
  {
    char uri[] = "--uri=mysqlx://root@localhost";
    char *argv0[] = {const_cast<char *>("ut"), const_cast<char *>("--mysql"),
                     uri, NULL};

    test_overriding_options("--mysql --uri", 3, argv0, "session-type",
                            "mysqlx");
  }
  return;
  {
    char uri[] = "--uri=mysqlx://root@localhost";
    char *argv0[] = {const_cast<char *>("ut"), const_cast<char *>("--sqlc"),
                     uri, NULL};

    test_conflicting_options(
        "--mysql --uri", 3, argv0,
        "URI scheme mysqlx:// cannot be combined with option --sqlc\n");
  }
  return;
  {
    char uri[] = "--uri=mysql://root@localhost";
    char *argv1[] = {const_cast<char *>("ut"), const_cast<char *>("--mysql"),
                     uri, NULL};

    test_overriding_options("--mysql --uri", 3, argv1, "session-type", "mysql");
  }

  {
    char uri[] = "--uri=mysql://root@localhost";
    char *argv1[] = {const_cast<char *>("ut"), const_cast<char *>("--sqlx"),
                     uri, NULL};

    test_conflicting_options(
        "--mysql --uri", 3, argv1,
        "URI scheme mysql:// cannot be combined with option --sqlx\n");
  }

  // first uri then option
  {
    char uri[] = "--uri=mysqlx://root@localhost";
    char *argv0[] = {const_cast<char *>("ut"), uri,
                     const_cast<char *>("--mysql"), NULL};

    test_overriding_options("--mysql --uri", 3, argv0, "session-type", "mysql");
  }

  {
    char uri[] = "--uri=mysqlx://root@localhost";
    char *argv0[] = {const_cast<char *>("ut"), uri,
                     const_cast<char *>("--sqlc"), NULL};

    test_conflicting_options(
        "--mysql --uri", 3, argv0,
        "Option --sqlc cannot be combined with URI scheme mysqlx://\n");
  }

  {
    char uri[] = "--uri=mysql://root@localhost";
    char *argv1[] = {const_cast<char *>("ut"), uri,
                     const_cast<char *>("--mysqlx"), NULL};

    test_overriding_options("--mysql --uri", 3, argv1, "session-type",
                            "mysqlx");
  }

  {
    char uri[] = "--uri=mysql://root@localhost";
    char *argv1[] = {const_cast<char *>("ut"), uri,
                     const_cast<char *>("--sqlx"), NULL};

    test_conflicting_options(
        "--mysql --uri", 3, argv1,
        "Option --sqlx cannot be combined with URI scheme mysql://\n");
  }
}

TEST_F(Shell_cmdline_options, override_user) {
  char uri[] = "--uri=mysqlx://root@localhost";
  char *argv0[] = {const_cast<char *>("ut"), const_cast<char *>("--user=guest"),
                   uri, NULL};

  test_overriding_options("--user --uri", 3, argv0, "user", "root");

  char *argv1[] = {const_cast<char *>("ut"), uri,
                   const_cast<char *>("--user=guest"), NULL};
  test_overriding_options("--user --uri", 3, argv1, "user", "guest");
}

TEST_F(Shell_cmdline_options, override_password) {
  {
    char pwd[] = {"--password=example"};
    char uri[] = {"--uri=mysqlx://root:password@localhost"};
    char *argv0[] = {const_cast<char *>("ut"), pwd, uri, NULL};

    test_overriding_options("--password --uri", 3, argv0, "password",
                            "password");
  }
  {
    char pwd[] = {"--password=example"};
    char uri[] = {"--uri=mysqlx://root:password@localhost"};
    char *argv1[] = {const_cast<char *>("ut"), uri, pwd, NULL};

    test_overriding_options("--password --uri", 3, argv1, "password",
                            "example");
  }
}

TEST_F(Shell_cmdline_options, override_host) {
  char uri[] = "--uri=mysqlx://root:password@localhost";
  char *argv0[] = {const_cast<char *>("ut"),
                   const_cast<char *>("--host=127.0.0.1"), uri, NULL};

  test_overriding_options("--host --uri", 3, argv0, "host", "localhost");

  char *argv1[] = {const_cast<char *>("ut"), uri,
                   const_cast<char *>("--host=127.0.0.1"), NULL};

  test_overriding_options("--host --uri", 3, argv1, "host", "127.0.0.1");
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
                           "--result-format are: table, tabbed, vertical, "
                           "json, ndjson, json/raw, json/array, json/pretty\n");
}

TEST_F(Shell_cmdline_options, override_port) {
  char uri[] = {"--uri=mysqlx://root:password@localhost:3307"};
  char *argv0[] = {const_cast<char *>("ut"), const_cast<char *>("--port=3306"),
                   uri, NULL};
  test_overriding_options("--port --uri", 3, argv0, "port", "3307");
}

TEST_F(Shell_cmdline_options, override_socket) {
#ifdef _WIN32
  char socket[] = "named.pipe";
  char uri[] = "--uri=mysql://root@\\\\.\\named.pipe";
#else   // !_WIN32
  char socket[] = "/socket";
  char uri[] = "--uri=mysqlx://root@/socket";
#endif  // !_WIN32
  char *argv0[] = {const_cast<char *>("ut"),
                   const_cast<char *>("--socket=/path/to/socket"), uri, NULL};

  test_overriding_options("--socket --uri", 3, argv0, "sock", socket);
}

TEST_F(Shell_cmdline_options, override_port_and_socket) {
  // port overrides socket and vice-versa

  char *argv0[] = {const_cast<char *>("ut"), const_cast<char *>("--port=3307"),
                   const_cast<char *>("--socket=/some/weird/path"), NULL};

  test_overriding_options("--port --socket", 3, argv0, "sock",
                          "/some/weird/path");

#ifndef _WIN32
  char *argv0b[] = {const_cast<char *>("ut"), const_cast<char *>("--port=3307"),
                    const_cast<char *>("--socket"), NULL};

  test_overriding_options("--port --socket", 3, argv0b, "sock", "");
#endif

#ifdef _WIN32
  char uri1[] = "--uri=root@\\\\.\\named.pipe";
#else   // !_WIN32
  char uri1[] = "--uri=root@/socket";
#endif  // !_WIN32
  char *argv1[] = {const_cast<char *>("ut"), uri1,
                   const_cast<char *>("--port=3306"), NULL};

  test_overriding_options("--uri --port", 3, argv1, "port", "3306");

  char uri2[] = "--uri=root@localhost:3306";
  char *argv2[] = {const_cast<char *>("ut"), uri2,
                   const_cast<char *>("--socket=/some/socket/path"), NULL};

  test_overriding_options("--uri --socket", 3, argv2, "sock",
                          "/some/socket/path");
}

TEST_F(Shell_cmdline_options, conflicts_compression) {
  char uri[] = {"--uri=mysqlx://root:password@localhost"};
  char *argv0[] = {const_cast<char *>("ut"), const_cast<char *>("--compress"),
                   const_cast<char *>("--compression-algorithms=uncompressed"),
                   uri, NULL};
  test_conflicting_options(
      "compressoon conflicts", 3, argv0,
      "Conflicting connection options: compression=REQUIRED, "
      "compression-algorithms=uncompressed.\n");
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
  EXPECT_STREQ(argv[1], "--uri=root@localhost:3301");
  EXPECT_STREQ("", cerr.str().c_str());

  SCOPED_TRACE("TESTING: single letter user without password in uri");
  firstArg = "--uri=r:@localhost:3301";

  char *argv1[]{const_cast<char *>("ut"), const_cast<char *>(firstArg.c_str()),
                NULL};
  options = std::make_shared<Shell_options>(2, argv1);

  EXPECT_EQ(0, options->get().exit_code);
  EXPECT_STREQ(argv1[1], "--uri=r@localhost:3301");
  EXPECT_STREQ("", cerr.str().c_str());

  SCOPED_TRACE("TESTING: single letter user with password in uri");
  firstArg = "--uri=r:pass@localhost:3301";

  char *argv2[]{const_cast<char *>("ut"), const_cast<char *>(firstArg.c_str()),
                NULL};
  options = std::make_shared<Shell_options>(2, argv2);

  EXPECT_EQ(0, options->get().exit_code);
  EXPECT_STREQ(argv2[1], "--uri=r@localhost:3301");
  EXPECT_STREQ("", cerr.str().c_str());

  SCOPED_TRACE("TESTING: single letter user with single letter password");
  firstArg = "--uri=r:r@localhost:3301";

  char *argv3[]{const_cast<char *>("ut"), const_cast<char *>(firstArg.c_str()),
                NULL};
  options = std::make_shared<Shell_options>(2, argv3);

  EXPECT_EQ(0, options->get().exit_code);
  EXPECT_STREQ(argv3[1], "--uri=r@localhost:3301");
  EXPECT_STREQ("", cerr.str().c_str());

  SCOPED_TRACE("TESTING: user with password with special sign encoded in uri");
  firstArg = "--uri=root:123%21@localhost:3301";

  char *argv4[]{const_cast<char *>("ut"), const_cast<char *>(firstArg.c_str()),
                NULL};
  options = std::make_shared<Shell_options>(2, argv4);

  EXPECT_EQ(0, options->get().exit_code);
  EXPECT_STREQ(argv4[1], "--uri=root@localhost:3301");
  EXPECT_STREQ("", cerr.str().c_str());

  SCOPED_TRACE(
      "TESTING: anonymous uri with with password with special sign encoded");
  firstArg = "root:123%21@localhost:3301";

  char *argv5[]{const_cast<char *>("ut"), const_cast<char *>(firstArg.c_str()),
                NULL};
  options = std::make_shared<Shell_options>(2, argv5);

  EXPECT_EQ(0, options->get().exit_code);
  EXPECT_STREQ(argv5[1], "root@localhost:3301");
  EXPECT_STREQ("", cerr.str().c_str());

  // Restore old cerr.
  std::cerr.rdbuf(backup);
}

TEST_F(Shell_cmdline_options, test_deprecated_ssl) {
  std::string warning =
      "The --ssl option was deprecated, please use --ssl-mode instead. ";
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
        "' for 'connect-timeout'. The timeout value must "
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

TEST_F(Shell_cmdline_options, conflicts_execute_and_file) {
  static constexpr auto error =
      "Conflicting options: --execute and --file cannot be used at the same "
      "time.\n";

  {
    char *argv[] = {
        const_cast<char *>("ut"),           const_cast<char *>("-e"),
        const_cast<char *>("SELECT 1"),     const_cast<char *>("-f"),
        const_cast<char *>("select_2.sql"), nullptr};

    test_conflicting_options("-e -f", 5, argv, error);
  }

  {
    char *argv[] = {
        const_cast<char *>("ut"),           const_cast<char *>("--execute"),
        const_cast<char *>("SELECT 1"),     const_cast<char *>("-f"),
        const_cast<char *>("select_2.sql"), nullptr};

    test_conflicting_options("--execute -f", 5, argv, error);
  }

  {
    char *argv[] = {
        const_cast<char *>("ut"),           const_cast<char *>("-e"),
        const_cast<char *>("SELECT 1"),     const_cast<char *>("--file"),
        const_cast<char *>("select_2.sql"), nullptr};

    test_conflicting_options("-e --file", 5, argv, error);
  }

  {
    char *argv[] = {
        const_cast<char *>("ut"),           const_cast<char *>("--execute"),
        const_cast<char *>("SELECT 1"),     const_cast<char *>("--file"),
        const_cast<char *>("select_2.sql"), nullptr};

    test_conflicting_options("--execute --file", 5, argv, error);
  }
}

TEST_F(Shell_cmdline_options, test_file_and_execute) {
  const auto test_options = [](const std::string &context, size_t argc,
                               char *argv[]) {
    std::streambuf *backup = std::cerr.rdbuf();
    std::ostringstream cerr;
    std::cerr.rdbuf(cerr.rdbuf());

    SCOPED_TRACE("TESTING: " + context);

    Shell_options options(argc, argv);

    EXPECT_EQ(0, options.get().exit_code);

    EXPECT_STREQ("", cerr.str().c_str());

    // Restore old cerr.
    std::cerr.rdbuf(backup);
  };

  // if --file is before --execute then there's no conflict between these two,
  // as all command line options after --file are used as options of the
  // processed file

  {
    char *argv[] = {
        const_cast<char *>("ut"),           const_cast<char *>("-f"),
        const_cast<char *>("select_2.sql"), const_cast<char *>("-e"),
        const_cast<char *>("SELECT 1"),     nullptr};

    test_options("-f -e", 5, argv);
  }

  {
    char *argv[] = {
        const_cast<char *>("ut"),           const_cast<char *>("-f"),
        const_cast<char *>("select_2.sql"), const_cast<char *>("--execute"),
        const_cast<char *>("SELECT 1"),     nullptr};

    test_options("-f --execute", 5, argv);
  }

  {
    char *argv[] = {
        const_cast<char *>("ut"),           const_cast<char *>("--file"),
        const_cast<char *>("select_2.sql"), const_cast<char *>("-e"),
        const_cast<char *>("SELECT 1"),     nullptr};

    test_options("--file -e", 5, argv);
  }

  {
    char *argv[] = {
        const_cast<char *>("ut"),           const_cast<char *>("--file"),
        const_cast<char *>("select_2.sql"), const_cast<char *>("--execute"),
        const_cast<char *>("SELECT 1"),     nullptr};

    test_options("--file --execute", 5, argv);
  }
}

TEST_F(Shell_cmdline_options, mfa_tests) {
  {
    // Password in --password1 differs from password in URI
    char uri[] = {"--uri=mysqlx://root:password@localhost"};
    char wrong_pwd1[] = {"--password1=example"};
    char *argv0[] = {const_cast<char *>("ut"), uri, wrong_pwd1, NULL};

    test_overriding_options("Different passwords", 3, argv0, "password",
                            "example");
  }
  {
    // Password 1 and X Protocol is OK
    char uri1[] = "mysqlx://root@localhost";
    char pwd1[] = {"--password1=example"};
    char *argv0[] = {const_cast<char *>("ut"), uri1, pwd1, NULL};
    Shell_options so(3, argv0);
    EXPECT_EQ(0, so.get().exit_code);
    EXPECT_NO_THROW(so.get().connection_options());
  }
  {
    char *passwords[] = {const_cast<char *>("--password2=whatever"),
                         const_cast<char *>("--password3=whatever")};
    char *x_proto_args[] = {const_cast<char *>("--mysqlx"),
                            const_cast<char *>("--mx"),
                            const_cast<char *>("--sqlx")};

    // MFA and X Protocol are not OK
    for (size_t index = 0; index < 2; index++) {
      for (size_t pindex = 0; pindex < 2; pindex++) {
        char uri0[] = "root@localhost";
        char *argv0[] = {const_cast<char *>("ut"), uri0, passwords[pindex],
                         x_proto_args[index], NULL};

        test_conflicting_options("Different passwords 2", 4, argv0,
                                 "Multi-factor authentication is only "
                                 "compatible with MySQL protocol\n");
      }
    }
  }
}

TEST_F(Shell_cmdline_options, mycnf) {
  make_mycnf(R"*([mysqlsh]
user=rooot
host=localhost
mysqlx
)*");

  {
    char *argv0[] = {const_cast<char *>("ut"), &defaults_file[0], NULL};
    Shell_options so(2, argv0, "",
                     mysqlsh::Shell_options::Option_flags_set(
                         mysqlsh::Shell_options::Option_flags::READ_MYCNF));
    EXPECT_EQ(0, so.get().exit_code);
    EXPECT_NO_THROW(so.get().connection_options());

    EXPECT_EQ("rooot", so.get().connection_options().get_user());
    EXPECT_EQ("mysqlx", so.get().connection_options().get_scheme());
  }

  make_mycnf(R"*([client]
user=rooot
host=localhost
mysqlx
)*");

  {
    char *argv0[] = {const_cast<char *>("ut"), &defaults_file[0], NULL};
    Shell_options so(2, argv0, "",
                     mysqlsh::Shell_options::Option_flags_set(
                         mysqlsh::Shell_options::Option_flags::READ_MYCNF));
    EXPECT_EQ(0, so.get().exit_code);
    EXPECT_NO_THROW(so.get().connection_options());

    EXPECT_EQ("rooot", so.get().connection_options().get_user());
    EXPECT_EQ("mysqlx", so.get().connection_options().get_scheme());
  }
}

TEST_F(Shell_cmdline_options, mycnf_default_session_type) {
  // check that we'll default to mysql:// if there's any connection option
  // in my.cnf
  make_mycnf(R"*([mysqlsh]
user=rooot
host=localhost
)*");
  {
    char *argv0[] = {const_cast<char *>("ut"), &defaults_file[0],
                     const_cast<char *>("--mysqlx"), NULL};
    Shell_options so(3, argv0, "",
                     mysqlsh::Shell_options::Option_flags_set(
                         mysqlsh::Shell_options::Option_flags::READ_MYCNF));
    EXPECT_EQ(0, so.get().exit_code);
    EXPECT_NO_THROW(so.get().connection_options());

    EXPECT_EQ("rooot", so.get().connection_options().get_user());
    EXPECT_EQ("mysqlx", so.get().connection_options().get_scheme());
  }

  {
    char *argv0[] = {const_cast<char *>("ut"), &defaults_file[0],
                     const_cast<char *>("--sqlx"), NULL};
    Shell_options so(3, argv0, "",
                     mysqlsh::Shell_options::Option_flags_set(
                         mysqlsh::Shell_options::Option_flags::READ_MYCNF));
    EXPECT_EQ(0, so.get().exit_code);
    EXPECT_NO_THROW(so.get().connection_options());

    EXPECT_EQ("rooot", so.get().connection_options().get_user());
    EXPECT_EQ("mysqlx", so.get().connection_options().get_scheme());
  }
}

TEST_F(Shell_cmdline_options, mycnf_option_override) {
  // override socket with port
  make_mycnf(R"*([mysqlsh]
user=rooot
socket=/sock/et
)*");
  {
    char *argv0[] = {const_cast<char *>("ut"), &defaults_file[0], NULL};
    Shell_options so(2, argv0, "",
                     mysqlsh::Shell_options::Option_flags_set(
                         mysqlsh::Shell_options::Option_flags::READ_MYCNF));
    EXPECT_EQ(0, so.get().exit_code);
    EXPECT_NO_THROW(so.get().connection_options());

    EXPECT_FALSE(so.get().connection_options().has_port());
    EXPECT_EQ("/sock/et", so.get().connection_options().get_socket());
  }

  {
    char *argv0[] = {const_cast<char *>("ut"), &defaults_file[0],
                     const_cast<char *>("--port=3232"), NULL};
    Shell_options so(3, argv0, "",
                     mysqlsh::Shell_options::Option_flags_set(
                         mysqlsh::Shell_options::Option_flags::READ_MYCNF));
    EXPECT_EQ(0, so.get().exit_code);
    EXPECT_NO_THROW(so.get().connection_options());

    EXPECT_EQ(3232, so.get().connection_options().get_port());
    EXPECT_FALSE(so.get().connection_options().has_socket());
  }

  {
    char uri[] = "root@localhost:3333";
    char *argv0[] = {const_cast<char *>("ut"), &defaults_file[0], uri, NULL};
    Shell_options so(3, argv0, "",
                     mysqlsh::Shell_options::Option_flags_set(
                         mysqlsh::Shell_options::Option_flags::READ_MYCNF));
    EXPECT_EQ(0, so.get().exit_code);
    EXPECT_NO_THROW(so.get().connection_options());

    EXPECT_EQ(3333, so.get().connection_options().get_port());
    EXPECT_FALSE(so.get().connection_options().has_socket());
  }

  {
    char uri[] = "root@localhost";
    char *argv0[] = {const_cast<char *>("ut"), &defaults_file[0], uri, NULL};
    Shell_options so(3, argv0, "",
                     mysqlsh::Shell_options::Option_flags_set(
                         mysqlsh::Shell_options::Option_flags::READ_MYCNF));
    EXPECT_EQ(0, so.get().exit_code);
    EXPECT_NO_THROW(so.get().connection_options());

    EXPECT_FALSE(so.get().connection_options().has_port());
    EXPECT_FALSE(so.get().connection_options().has_socket());
  }

  // override port with socket
  make_mycnf(R"*([mysqlsh]
user=rooot
host=localhost
port=3232
)*");
  {
    char *argv0[] = {const_cast<char *>("ut"), &defaults_file[0], NULL};
    Shell_options so(2, argv0, "",
                     mysqlsh::Shell_options::Option_flags_set(
                         mysqlsh::Shell_options::Option_flags::READ_MYCNF));
    EXPECT_EQ(0, so.get().exit_code);
    EXPECT_NO_THROW(so.get().connection_options());

    EXPECT_FALSE(so.get().connection_options().has_socket());
    EXPECT_EQ(3232, so.get().connection_options().get_port());
  }

  {
    char *argv0[] = {const_cast<char *>("ut"), &defaults_file[0],
                     const_cast<char *>("--socket=/tmp/sock"), NULL};
    Shell_options so(3, argv0, "",
                     mysqlsh::Shell_options::Option_flags_set(
                         mysqlsh::Shell_options::Option_flags::READ_MYCNF));
    EXPECT_EQ(0, so.get().exit_code);
    EXPECT_NO_THROW(so.get().connection_options());

    EXPECT_FALSE(so.get().connection_options().has_port());
    EXPECT_EQ("/tmp/sock", so.get().connection_options().get_socket());
  }

  {
    char uri[] = "root@/tmp%2Fsock";
    char *argv0[] = {const_cast<char *>("ut"), &defaults_file[0], uri, NULL};
    Shell_options so(3, argv0, "",
                     mysqlsh::Shell_options::Option_flags_set(
                         mysqlsh::Shell_options::Option_flags::READ_MYCNF));
    EXPECT_EQ(0, so.get().exit_code);
    EXPECT_NO_THROW(so.get().connection_options());

    EXPECT_FALSE(so.get().connection_options().has_port());
    EXPECT_EQ("/tmp/sock", so.get().connection_options().get_socket());
  }
}

}  // namespace shcore
