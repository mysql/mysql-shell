/* Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.

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

#ifndef UNITTEST_TEST_UTILS_SHELL_TEST_ENV_H_
#define UNITTEST_TEST_UTILS_SHELL_TEST_ENV_H_

// Environment variables only

#include <vector>
#include <string>
#include <utility>
#include <map>
#include <memory>
#include "unittest/gtest_clean.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/utils/version.h"

#define ASSERT_THROW_LIKE(expr, exc, msg)                              \
  try {                                                                \
    expr;                                                              \
    FAIL() << "Expected exception of type " #exc << " but got none\n"; \
  } catch (exc & e) {                                                  \
    if (std::string(e.what()).find(msg) == std::string::npos) {        \
      FAIL() << "Expected exception with message: " << msg             \
             << "\nbut got: " << typeid(e).name() << e.what() << "\n"; \
    }                                                                  \
  }

#define EXPECT_THROW_LIKE(expr, exc, msg)                                     \
  try {                                                                       \
    expr;                                                                     \
    ADD_FAILURE() << "Expected exception of type " #exc << " but got none\n"; \
  } catch (exc & e) {                                                         \
    if (std::string(e.what()).find(msg) == std::string::npos) {               \
      ADD_FAILURE() << "Expected exception with message: " << msg             \
                    << "\nbut got: " << e.what() << "\n";                     \
    }                                                                         \
  } catch (std::exception & e) {                                              \
    ADD_FAILURE() << "Expected exception of type " #exc << " but got "        \
                  << typeid(e).name() << ": " << e.what() << "\n";            \
  } catch (...) {                                                             \
    ADD_FAILURE() << "Expected exception of type " #exc                       \
                  << " but got another\n";                                    \
  }


namespace tests {
/**
 * \ingroup UTFramework
 * \todo This class must be documented.
 */
class Override_row_string : public mysqlshdk::db::replay::Row_hook {
 public:
  Override_row_string(std::unique_ptr<mysqlshdk::db::IRow> source,
                      uint32_t column, const std::string& value)
    : mysqlshdk::db::replay::Row_hook(std::move(source)),
    _column(column), _value(value) {
  }

  std::string get_string(uint32_t index) const override {
    if (index == _column)
      return _value;
    return Row_hook::get_string(index);
  }

  uint32_t _column;
  std::string _value;
};

/**
 * \ingroup UTFramework
 * Defines a base environment for unit tests.
 *
 * The attribues of this class are configured based on the environment where
 * the tests are executed, the values are available through all the test suite.
 */
class Shell_test_env : public ::testing::Test {
 public:
  Shell_test_env();

  virtual void SetUpOnce() {}

  static void SetUpTestCase();

  std::string setup_recorder(const char *sub_test_name = nullptr);

 protected:
  std::string _host;  //!< The host name configured in MYSQL_URI
  std::string _port;  //!< The port for X protocol sessions, env:MYSQLX_PORT
  std::string _user;  //!< The user configured in env:MYSQL_URI
  int _port_number;  //!< The port for X protocol sessions, env:MYSQLX_PORT
  std::string _hostname;  //!< TBD
  std::string _hostname_ip;  //!< TBD
  std::string _uri;  //!< A full URI for X protocol sessions
  std::string _uri_nopasswd;  //!< A password-less URI for X protocol sessions
  std::string _pwd;  //!< The password as configured in env:MYSQL_PWD
  std::string _mysql_port;  //!< The port for MySQL protocol sessions, env:MYSQL_PORT
  int _mysql_port_number;  //!< The port for MySQL protocol sessions, env:MYSQL_PORT
  std::string _mysql_uri;  //!< A full URI for MySQL protocol sessions
  std::string _mysql_uri_nopasswd;  //!< A password-less URI for MySQL protocol sessions
  std::string _socket;  //!< env:MYSQLX_SOCKET
  std::string _mysql_socket;  //!< env:MYSQL_SOCKET
  mysqlshdk::utils::Version _target_server_version;  //!< The version of the used MySQL Server
  mysqlshdk::utils::Version _highest_tls_version;  //!< The highest TLS version supported by MySQL Server
  std::string _test_context;  //!< Context for script validation engine

  bool _recording_enabled = false;

  std::map<std::string, std::string> _output_tokens; //!< Tokens for string resolution
  std::string resolve_string(const std::string& source);

  void SetUp() override;
  void TearDown() override;

 public:
  static std::string get_path_to_mysqlsh();

  std::string _mysql_sandbox_port1;  //!< Port of the first sandbox
  std::string _mysql_sandbox_port2;  //!< Port of the second sandbox
  std::string _mysql_sandbox_port3;  //!< Port of the third sandbox

  int _mysql_sandbox_nport1;  //!< Port of the first sandbox
  int _mysql_sandbox_nport2;  //!< Port of the second sandbox
  int _mysql_sandbox_nport3;  //!< Port of the third sandbox

  // Paths to the 3 commonly used sandboxes configuration files
  std::string _sandbox_cnf_1;  //!< Path to the first sandbox cfg file
  std::string _sandbox_cnf_2;  //!< Path to the second sandbox cfg file
  std::string _sandbox_cnf_3;  //!< Path to the third sandbox cfg file

  std::string _sandbox_cnf_1_bkp;  //!< Path to the first sandbox cfg file backup
  std::string _sandbox_cnf_2_bkp;  //!< Path to the second sandbox cfg file backup
  std::string _sandbox_cnf_3_bkp;  //!< Path to the third sandbox cfg file backup

  static std::string _path_splitter;  //!< OS path separator

  std::string _sandbox_dir;  //!< Path to the configured sandbox directory
};
}  // namespace tests

// proto = [a]uto, [c]lassic, [x]
std::string shell_test_server_uri(int proto = 'a');

std::string random_string(std::string::size_type length);

void run_script_classic(const std::vector<std::string> &script);

// Execute the given script file from the URI. File is assumed to be in
// unittest/data/sql/
void run_test_data_sql_file(const std::string &uri,
                            const std::string &filename);

#endif  // UNITTEST_TEST_UTILS_SHELL_TEST_ENV_H_
