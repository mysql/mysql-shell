/* Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.

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

#ifndef UNITTEST_TEST_UTILS_SHELL_BASE_TEST_H_
#define UNITTEST_TEST_UTILS_SHELL_BASE_TEST_H_

#define MY_EXPECT_OUTPUT_CONTAINS(e, o)                       \
  do {                                                        \
    check_string_expectation(__FILE__, __LINE__, e, o, true); \
  } while (0)

#define MY_EXPECT_OUTPUT_NOT_CONTAINS(e, o)                    \
  do {                                                         \
    check_string_expectation(__FILE__, __LINE__, e, o, false); \
  } while (0)

#define MY_EXPECT_MULTILINE_OUTPUT(c, e, o)    \
  do {                                         \
    SCOPED_TRACE("...in stdout check\n");      \
    check_multiline_expect(c, "STDOUT", e, o); \
  } while (0)

#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "unittest/test_utils/shell_test_env.h"

namespace tests {

/**
 * \ingroup UTFramework
 * Base utilities class for the test framework.
 */
class Shell_base_test : public Shell_test_env {
 public:
  Shell_base_test();

 protected:
  virtual void TearDown();

 public:
  void check_string_expectation(const char* file, int line,
                                const std::string& expected_str,
                                const std::string& actual, bool expected);
  void check_string_list_expectation(
      const char* file, int line, const std::vector<std::string>& expected_strs,
      const std::string& actual, bool expected);
  bool multi_value_compare(const std::string& expected,
                           const std::string& actual);
  bool check_multiline_expect(const std::string& context,
                              const std::string& stream,
                              const std::string& expected,
                              const std::string& actual,
                              int srcline = 0, int valline = 0);
  std::string multiline(const std::vector<std::string> input);

  void create_file(const std::string& name, const std::string& content);
};
}  // namespace tests

#endif  // UNITTEST_TEST_UTILS_SHELL_BASE_TEST_H_
