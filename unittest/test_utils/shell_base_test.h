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

#ifndef UNITTEST_TEST_UTILS_SHELL_BASE_TEST_H_
#define UNITTEST_TEST_UTILS_SHELL_BASE_TEST_H_

#define MY_EXPECT_OUTPUT_CONTAINS(e,o) Shell_base_test::check_string_expectation(e,o,true)
#define MY_EXPECT_OUTPUT_NOT_CONTAINS(e,o) Shell_base_test::check_string_expectation(e,o,false)

#include <string>
#include <map>
#include <memory>
#include <thread>
#include <gtest/gtest.h>
#include <vector>
#include "unittest/test_utils/server_mock.h"

extern "C" const char *g_argv0;

namespace tests {


class Shell_base_test : public ::testing::Test {
 protected:
  virtual void SetUp();
  virtual void TearDown();

  std::string _host;
  std::string _port;
  std::string _user;
  int _port_number;
  std::string _uri;
  std::string _uri_nopasswd;
  std::string _pwd;
  std::string _mysql_port;
  int _mysql_port_number;
  std::string _mysql_uri;
  std::string _mysql_uri_nopasswd;

 public:
  static void check_string_expectation(const std::string &expected_str,
                                       const std::string &actual,
                                       bool expected);
  static void create_file(const std::string& name, const std::string& content);
  void start_server_mock(int port,
                         const std::vector< testing::Fake_result_data >& data);
  void stop_server_mock(int port);

 private:
  std::map<int, std::shared_ptr<tests::Server_mock> > _servers;
};
}

#endif  // UNITTEST_TEST_UTILS_SHELL_BASE_TEST_H_