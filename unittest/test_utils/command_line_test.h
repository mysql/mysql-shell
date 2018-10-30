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

#ifndef UNITTEST_TEST_UTILS_COMMAND_LINE_TEST_H_
#define UNITTEST_TEST_UTILS_COMMAND_LINE_TEST_H_

#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "mysqlshdk/libs/utils/process_launcher.h"
#include "unittest/test_utils/shell_base_test.h"

#define MY_EXPECT_CMD_OUTPUT_CONTAINS(e)                            \
  do {                                                              \
    SCOPED_TRACE("...in stdout check\n");                           \
    check_string_expectation(__FILE__, __LINE__, e, _output, true); \
  } while (0)

#define MY_EXPECT_CMD_OUTPUT_CONTAINS_ONE_OF(e)                          \
  do {                                                                   \
    SCOPED_TRACE("...in stdout check\n");                                \
    check_string_list_expectation(__FILE__, __LINE__, e, _output, true); \
  } while (0)

#define MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS(e)                         \
  do {                                                               \
    SCOPED_TRACE("...in stdout check\n");                            \
    check_string_expectation(__FILE__, __LINE__, e, _output, false); \
  } while (0)

namespace tests {

/**
 * \ingroup UTFramework
 * Base class for tests that call the mysqlsh binary.
 */
class Command_line_test : public Shell_base_test {
 public:
  virtual void SetUp();

  bool grep_stdout(const std::string &s);

  void wipe_out() { _output.clear(); }

  void leave_carriage_returns() { _strip_carriage_returns = false; }

 protected:
  std::string _mysqlsh_path;
  const char *_mysqlsh;
  shcore::Process_launcher *_process = nullptr;
  std::mutex _process_mutex;

  bool _strip_carriage_returns = true;
  std::string _output;
  std::mutex _output_mutex;
  int execute(const std::vector<const char *> &args,
              const char *password = NULL, const char *input_file = nullptr,
              const std::vector<std::string> &env = {});

  void send_ctrlc();
};

}  // namespace tests
#endif  // UNITTEST_TEST_UTILS_COMMAND_LINE_TEST_H_
