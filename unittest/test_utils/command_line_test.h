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

#ifndef UNITTEST_TEST_UTILS_COMMAND_LINE_TEST_H_
#define UNITTEST_TEST_UTILS_COMMAND_LINE_TEST_H_

#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "mysqlshdk/libs/utils/process_launcher.h"
#include "unittest/test_utils/shell_base_test.h"

#define MY_EXPECT_CMD_OUTPUT_CONTAINS(e)                         \
  do {                                                           \
    SCOPED_TRACE("...in stdout check\n");                        \
    Shell_base_test::check_string_expectation(e, _output, true); \
  } while (0)

#define MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS(e)                      \
  do {                                                            \
    SCOPED_TRACE("...in stdout check\n");                         \
    Shell_base_test::check_string_expectation(e, _output, false); \
  } while (0)

namespace tests {

class Command_line_test : public Shell_base_test {
 public:
  virtual void SetUp();

  bool grep_stdout(const std::string &s);

 protected:
  std::string _mysqlsh_path;
  const char *_mysqlsh;
  shcore::Process_launcher *_process = nullptr;
  std::mutex _process_mutex;

  std::string get_path_to_mysqlsh();
  std::string _output;
  std::mutex _output_mutex;
  int execute(const std::vector<const char *> &args);

  void send_ctrlc();
  std::string _new_line_char;
};

}  // namespace tests
#endif  // UNITTEST_TEST_UTILS_COMMAND_LINE_TEST_H_
