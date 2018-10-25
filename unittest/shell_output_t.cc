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

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "gtest_clean.h"
#include "scripting/common.h"
#include "scripting/lang_base.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"

#include "modules/devapi/base_resultset.h"
#include "mysqlshdk/include/shellcore/base_shell.h"
#include "shellcore/base_session.h"
#include "shellcore/shell_core.h"
#include "shellcore/shell_resultset_dumper.h"
#include "shellcore/shell_sql.h"
#include "test_utils.h"
#include "utils/utils_file.h"

namespace shcore {
namespace Shell_output_tests {
class Shell_output_test : public Shell_core_test_wrapper {
 protected:
  std::string _file_name;
  int _ret_val;

  virtual void SetUp() {
    Shell_core_test_wrapper::SetUp();

    _interactive_shell->process_line("\\sql");
    _interactive_shell->process_line("\\connect --mc " + _mysql_uri);
    if (!output_handler.std_err.empty()) {
      std::cerr << "ERROR connecting to " << _mysql_uri << ":"
                << output_handler.std_err << "\n";
      std::cerr << "Test environment is probably wrong. Please check values of "
                   "MYSQL_URI, MYSQL_PORT, MYSQL_PWD environment variables.\n";
      FAIL();
    }
    wipe_all();
  }
  virtual void TearDown() {
    _interactive_shell->process_line("\\js");
    _interactive_shell->process_line("session.close();");
  }
};

TEST_F(Shell_output_test, table_output) {
  std::stringstream stream("select 11 as a, 22 as b;");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});
  EXPECT_EQ(0, _ret_val);

  std::string expected_output =
      R"(+----+----+
| a  | b  |
+----+----+
| 11 | 22 |
+----+----+)";
  MY_EXPECT_STDOUT_CONTAINS(expected_output);
}

TEST_F(Shell_output_test, vertical_output) {
  std::stringstream stream("select 11 as a\\G");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});
  EXPECT_EQ(0, _ret_val);

  std::string expected_output =
      R"(*************************** 1. row ***************************
a: 11)";
  MY_EXPECT_STDOUT_CONTAINS(expected_output);
}

TEST_F(Shell_output_test, mixed_output) {
  std::stringstream stream("select 11 as a\\G select 12 as b");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});
  EXPECT_EQ(0, _ret_val);

  std::string expected_output =
      R"(*************************** 1. row ***************************
a: 11)";
  MY_EXPECT_STDOUT_CONTAINS(expected_output);

  expected_output =
      R"(+----+
| b  |
+----+
| 12 |
+----+)";
  MY_EXPECT_STDOUT_CONTAINS(expected_output);
}

TEST_F(Shell_output_test, vertical_output_column_align) {
  std::stringstream stream("select 11 as a, 12 as second, 1234 as third\\G");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});
  EXPECT_EQ(0, _ret_val);

  std::string expected_output =
      R"(*************************** 1. row ***************************
     a: 11
second: 12
 third: 1234)";
  MY_EXPECT_STDOUT_CONTAINS(expected_output);
}

TEST_F(Shell_output_test, vertical_output_result_with_newline) {
  std::stringstream stream("select \"te\nst\" as a\\G");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});
  EXPECT_EQ(0, _ret_val);

  std::string expected_output =
      R"(*************************** 1. row ***************************
a: te
st)";
  MY_EXPECT_STDOUT_CONTAINS(expected_output);
}

TEST_F(Shell_output_test, output_format_option) {
  _options->result_format = "vertical";

  std::stringstream stream("select 11 as a;");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});

  std::string expected_output =
      R"(*************************** 1. row ***************************
a: 11)";
  MY_EXPECT_STDOUT_CONTAINS(expected_output);

  wipe_all();
  _options->result_format = "table";
  stream.clear();
  stream.str("select 12 as a;");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});

  expected_output =
      R"(+----+
| a  |
+----+
| 12 |
+----+)";
  MY_EXPECT_STDOUT_CONTAINS(expected_output);

  wipe_all();
  _options->result_format = "table";
  stream.clear();
  stream.str("select 13 as a\\G");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});

  expected_output =
      R"(*************************** 1. row ***************************
a: 13)";
  MY_EXPECT_STDOUT_CONTAINS(expected_output);
}

}  // namespace Shell_output_tests
}  // namespace shcore
