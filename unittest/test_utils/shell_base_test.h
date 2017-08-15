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

#define MY_EXPECT_OUTPUT_CONTAINS(e, o)                    \
  do {                                                     \
    check_string_expectation(e, o, true); \
    SCOPED_TRACE("");                                      \
  } while (0)

#define MY_EXPECT_OUTPUT_NOT_CONTAINS(e, o)                 \
  do {                                                      \
    check_string_expectation(e, o, false); \
    SCOPED_TRACE("");                                       \
  } while (0)

#define MY_EXPECT_MULTILINE_OUTPUT(c, e, o)                     \
  do {                                                          \
    SCOPED_TRACE("...in stdout check\n");                       \
    check_multiline_expect(c, "STDOUT", e, o); \
  } while (0)

#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "unittest/test_utils/shell_test_env.h"
#include "unittest/test_utils/server_mock.h"

namespace tests {

#define START_SERVER_MOCK(P, D) ASSERT_EQ("", start_server_mock(P, D))

class Shell_base_test : public Shell_test_env {
 public:
  Shell_base_test();

 protected:
  virtual void TearDown();

 public:
  void check_string_expectation(const std::string& expected_str,
                                const std::string& actual,
                                bool expected);
  bool multi_value_compare(const std::string& expected,
                           const std::string& actual);
  bool check_multiline_expect(const std::string& context,
                              const std::string& stream,
                              const std::string& expected,
                              const std::string& actual);
  std::string multiline(const std::vector<std::string> input);

  void create_file(const std::string& name, const std::string& content);
  std::string start_server_mock(
      int port, const std::vector<testing::Fake_result_data>& data);
  void stop_server_mock(int port);

 private:
  std::map<int, std::shared_ptr<tests::Server_mock> > _servers;
};
}

#endif  // UNITTEST_TEST_UTILS_SHELL_BASE_TEST_H_
