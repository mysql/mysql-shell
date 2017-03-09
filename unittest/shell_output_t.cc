/* Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.

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

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <boost/pointer_cast.hpp>

#include "gtest/gtest.h"
#include "shellcore/types.h"
#include "shellcore/lang_base.h"
#include "shellcore/types_cpp.h"
#include "shellcore/common.h"

#include "shellcore/shell_core.h"
#include "shellcore/shell_sql.h"
#include "../modules/base_session.h"
#include "../modules/base_resultset.h"
#include "shell/shell_resultset_dumper.h"
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
    _interactive_shell->process_line("\\connect -c " + _mysql_uri);
    if (!output_handler.std_err.empty()) {
      std::cerr << "ERROR connecting to "<<_mysql_uri<<":"<<output_handler.std_err<<"\n";
      std::cerr << "Test environment is probably wrong. Please check values of MYSQL_URI, MYSQL_PORT, MYSQL_PWD environment variables.\n";
      FAIL();
    }
    wipe_all();
  }
  virtual void TearDown() {
    _interactive_shell->process_line("\\js");
    _interactive_shell->process_line("session.close();");
  }

  Value::Map_type_ref options = Shell_core_options::get();
};

TEST_F(Shell_output_test, table_output) {
  std::stringstream stream("select 11 as a;");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});
  EXPECT_EQ(0, _ret_val);

  MY_EXPECT_STDOUT_CONTAINS(
R"(+----+
| a  |
+----+
| 11 |
+----+)");
}

TEST_F(Shell_output_test, vertical_output) {
  std::stringstream stream("select 11 as a\\G");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});
  EXPECT_EQ(0, _ret_val);

  MY_EXPECT_STDOUT_CONTAINS(
R"(*************************** 1. row ***************************
a: 11)");
}

TEST_F(Shell_output_test, mixed_output) {
  std::stringstream stream("select 11 as a\\G select 12 as b");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});
  EXPECT_EQ(0, _ret_val);

  MY_EXPECT_STDOUT_CONTAINS(
R"(*************************** 1. row ***************************
a: 11)");

  MY_EXPECT_STDOUT_CONTAINS(
R"(+----+
| b  |
+----+
| 12 |
+----+)");
}

TEST_F(Shell_output_test, vertical_output_column_align) {
  std::stringstream stream("select 11 as a, 12 as second, 1234 as third\\G");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});
  EXPECT_EQ(0, _ret_val);

  MY_EXPECT_STDOUT_CONTAINS(
R"(*************************** 1. row ***************************
     a: 11
second: 12
 third: 1234)");
}

TEST_F(Shell_output_test, vertical_output_result_with_newline) {
  std::stringstream stream("select \"te\nst\" as a\\G");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});
  EXPECT_EQ(0, _ret_val);

  MY_EXPECT_STDOUT_CONTAINS(
R"(*************************** 1. row ***************************
a: te
st)");
}

TEST_F(Shell_output_test, output_format_option) {
  (*options)[SHCORE_OUTPUT_FORMAT] = Value("vertical");

  std::stringstream stream("select 11 as a;");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});

  MY_EXPECT_STDOUT_CONTAINS(
R"(*************************** 1. row ***************************
a: 11)");

  wipe_all();
  (*options)[SHCORE_OUTPUT_FORMAT] = Value("table");
  stream.clear();
  stream.str("select 12 as a;");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});

  MY_EXPECT_STDOUT_CONTAINS(
R"(+----+
| a  |
+----+
| 12 |
+----+)");

  wipe_all();
  (*options)[SHCORE_OUTPUT_FORMAT] = Value("table");
  stream.clear();
  stream.str("select 13 as a\\G");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});

  MY_EXPECT_STDOUT_CONTAINS(
R"(*************************** 1. row ***************************
a: 13)");
}

} //namespace Shell_output_tests
} //namespace shcore
