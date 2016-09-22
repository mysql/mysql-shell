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
namespace shell_core_tests {
class Shell_core_test : public Shell_core_test_wrapper {
protected:
  std::string _file_name;
  int _ret_val;

  virtual void SetUp() {
    Shell_core_test_wrapper::SetUp();

    _interactive_shell->process_line("\\sql");
  }

  void connect() {
    _interactive_shell->process_line("\\connect -c " + _mysql_uri);
  }

  void process(const std::string& path) {
    wipe_all();

    _file_name = MYSQLX_SOURCE_HOME;
    _file_name += "/unittest/data/" + path;

    std::ifstream stream(_file_name.c_str());
    if (stream.fail())
      FAIL();

    _ret_val = _interactive_shell->process_stream(stream, _file_name);

    stream.close();
  }
};

TEST_F(Shell_core_test, test_process_stream) {
  connect();

  // Successfully processed file
  (*Shell_core_options::get())[SHCORE_BATCH_CONTINUE_ON_ERROR] = Value::False();
  process("sql/sql_ok.sql");
  EXPECT_EQ(0, _ret_val);
  EXPECT_NE(-1, static_cast<int>(output_handler.std_out.find("first_result")));

  // Failed without the force option
  process("sql/sql_err.sql");
  EXPECT_EQ(1, _ret_val);
  EXPECT_NE(-1, static_cast<int>(output_handler.std_out.find("first_result")));
  EXPECT_NE(-1, static_cast<int>(output_handler.std_err.find("Table 'unexisting.whatever' doesn't exist")));
  EXPECT_EQ(-1, static_cast<int>(output_handler.std_out.find("second_result")));

  // Failed without the force option
  (*Shell_core_options::get())[SHCORE_BATCH_CONTINUE_ON_ERROR] = Value::True();
  process("sql/sql_err.sql");
  EXPECT_EQ(1, _ret_val);
  EXPECT_NE(-1, static_cast<int>(output_handler.std_out.find("first_result")));
  EXPECT_NE(-1, static_cast<int>(output_handler.std_err.find("Table 'unexisting.whatever' doesn't exist")));
  EXPECT_NE(-1, static_cast<int>(output_handler.std_out.find("second_result")));

  // JS tests: outputs are not validated since in batch mode there's no autoprinting of resultsets
  // Error is also directed to the std::cerr directly
  _interactive_shell->process_line("\\js");
  process("js/js_ok.js");
  EXPECT_EQ(0, _ret_val);

  process("js/js_err.js");
  EXPECT_NE(-1, static_cast<int>(output_handler.std_err.find("Table 'unexisting.whatever' doesn't exist")));

  // Closes the connection
  _interactive_shell->process_line("session.close()");
}

TEST_F(Shell_core_test, regression_prompt_on_override_session) {
  connect();

  EXPECT_EQ("mysql-sql> ", _interactive_shell->prompt());
  (*Shell_core_options::get())[SHCORE_USE_WIZARDS] = shcore::Value::False();
  _interactive_shell->shell_context()->set_global("session", Value(std::static_pointer_cast<Object_bridge>(Shell_core_options::get_instance())));
  (*Shell_core_options::get())[SHCORE_USE_WIZARDS] = shcore::Value::True();
  EXPECT_EQ("mysql-sql> ", _interactive_shell->prompt());
}

TEST_F(Shell_core_test, process_sql_no_delim_from_stream) {
  connect();

  std::stringstream stream("show databases");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN");
  EXPECT_EQ(0, _ret_val);
  MY_EXPECT_STDOUT_CONTAINS("| Database");
}
}
}
