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

#ifndef UNITTEST_TEST_UTILS_SHELL_TEST_ENV_H_
#define UNITTEST_TEST_UTILS_SHELL_TEST_ENV_H_

// Environment variables only

#include <vector>
#include <string>
#include <map>
#include "unittest/gtest_clean.h"

extern "C" const char* g_argv0;


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

class Shell_test_env : public ::testing::Test {
 public:
  Shell_test_env();

  virtual void SetUpOnce() {}

  std::string setup_recorder();

 protected:
  std::string _host;
  std::string _port;
  std::string _user;
  int _port_number;
  std::string _hostname;
  std::string _hostname_ip;
  std::string _uri;
  std::string _uri_nopasswd;
  std::string _pwd;
  std::string _mysql_port;
  int _mysql_port_number;
  std::string _mysql_uri;
  std::string _mysql_uri_nopasswd;
  std::string _socket;  //< env:MYSQLX_SOCKET
  std::string _mysql_socket;  //< env:MYSQL_SOCKET

  bool _recording_enabled = false;

  std::map<std::string, std::string> _output_tokens;
  std::string resolve_string(const std::string& source);

  void SetUp() override;
  void TearDown() override;
  static void SetUpTestCase();

 public:
  static std::string get_path_to_mysqlsh();

  std::string _mysql_sandbox_port1;
  std::string _mysql_sandbox_port2;
  std::string _mysql_sandbox_port3;

  int _mysql_sandbox_nport1;
  int _mysql_sandbox_nport2;
  int _mysql_sandbox_nport3;

  // Paths to the 3 commonly used sandboxes configuration files
  std::string _sandbox_cnf_1;
  std::string _sandbox_cnf_2;
  std::string _sandbox_cnf_3;

  std::string _sandbox_cnf_1_bkp;
  std::string _sandbox_cnf_2_bkp;
  std::string _sandbox_cnf_3_bkp;

  static std::string _path_splitter;

  std::string _sandbox_dir;
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
