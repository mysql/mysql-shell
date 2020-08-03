/* Copyright (c) 2014, 2020, Oracle and/or its affiliates.

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
#include "modules/mod_shell_options.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "shellcore/base_session.h"
#include "shellcore/shell_core.h"
#include "shellcore/shell_resultset_dumper.h"
#include "shellcore/shell_sql.h"
#include "test_utils.h"
#include "utils/utils_file.h"

extern "C" const char *g_test_home;

namespace shcore {
namespace shell_core_tests {
class Shell_core_test : public Shell_core_test_wrapper {
 protected:
  std::string _file_name;
  int _ret_val;

#ifdef HAVE_V8
  const std::string to_scripting = "\\js";
#else
  const std::string to_scripting = "\\py";
#endif

  virtual void SetUp() {
    Shell_core_test_wrapper::SetUp();

    _interactive_shell->process_line("\\sql");
  }

  void connect() {
    _interactive_shell->process_line("\\connect --mc " + _mysql_uri);
    if (!output_handler.std_err.empty()) {
      std::cerr << "ERROR connecting to " << _mysql_uri << ":"
                << output_handler.std_err << "\n";
      std::cerr << "Test environment is probably wrong. Please check values of "
                   "MYSQL_URI, MYSQL_PORT, MYSQL_PWD environment variables.\n";
      FAIL();
    }
  }

  void process(const std::string &path) {
    wipe_all();

    _file_name = shcore::path::join_path(g_test_home, "data", path);

    std::ifstream stream(_file_name.c_str());
    if (stream.fail()) FAIL();

    _ret_val = _interactive_shell->process_stream(stream, _file_name, {});
    stream.close();
  }
};

TEST_F(Shell_core_test, test_process_stream) {
  connect();
  const char *err_table_57 = "Table 'unexisting.whatever' doesn't exist";
  const char *err_table_80 = "Unknown database 'unexisting'";

  // Successfully processed file
  _options->force = false;
  process("sql/sql_ok.sql");
  EXPECT_EQ(0, _ret_val);
  EXPECT_NE(-1, static_cast<int>(output_handler.std_out.find("first_result")));

  // Failed without the force option
  process("sql/sql_err.sql");
  EXPECT_EQ(1, _ret_val);
  EXPECT_NE(-1, static_cast<int>(output_handler.std_out.find("first_result")));
  EXPECT_TRUE(std::string::npos != output_handler.std_err.find(err_table_57) ||
              std::string::npos != output_handler.std_err.find(err_table_80));
  EXPECT_EQ(-1, static_cast<int>(output_handler.std_out.find("second_result")));

  // Failed without the force option
  _options->force = true;
  process("sql/sql_err.sql");
  EXPECT_EQ(0, _ret_val);  // if force is enabled, return code is 0 on error
  EXPECT_NE(-1, static_cast<int>(output_handler.std_out.find("first_result")));
  EXPECT_TRUE(std::string::npos != output_handler.std_err.find(err_table_57) ||
              std::string::npos != output_handler.std_err.find(err_table_80));
  EXPECT_NE(-1, static_cast<int>(output_handler.std_out.find("second_result")));

#ifdef HAVE_V8
  // JS tests: outputs are not validated since in batch mode there's no
  // autoprinting of resultsets
  //
  // Error is also directed to the std::cerr directly
  _interactive_shell->process_line("\\js");
  process("js/js_ok.js");
  EXPECT_EQ(0, _ret_val);

  process("js/js_err.js");
  // Error in 5.7 and 8.0 are different
  EXPECT_TRUE(std::string::npos != output_handler.std_err.find(err_table_57) ||
              std::string::npos != output_handler.std_err.find(err_table_80));
#endif

  // PY tests: outputs are not validated since in batch mode there's no
  // autoprinting of resultsets
  //
  // Error is also directed to the std::cerr directly
  _interactive_shell->process_line("\\py");
  process("py/py_ok.py");
  EXPECT_EQ(0, _ret_val);

  process("py/py_err.py");
  // Error in 5.7 and 8.0 are different
  EXPECT_TRUE(std::string::npos != output_handler.std_err.find(err_table_57) ||
              std::string::npos != output_handler.std_err.find(err_table_80));

  // Closes the connection
  _interactive_shell->process_line("session.close()");
}

#ifdef HAVE_V8
TEST_F(Shell_core_test, test_process_js_file_with_params) {
  // TODO(alfredo) this test and feature are broken.. it was
  // testing \source on non-interactive no, where the feature works
  // but in interactive it doesn't. \source must work the same way
  // in both modes (should be reimplemented)
  _options->interactive = false;
  reset_shell();

  _interactive_shell->process_line("\\js");

  std::string file_name =
      shcore::path::join_path(g_test_home, "data", "js", "script.js");

  _file_name = file_name + " one two";
  _interactive_shell->process_line("\\. " + _file_name);
  MY_EXPECT_STDOUT_CONTAINS("Processing file: " + file_name);
  MY_EXPECT_STDOUT_CONTAINS("The received arguments were: one and two");
  MY_EXPECT_STDERR_CONTAINS("");

  wipe_all();

  _file_name = "'" + file_name + "' one two";
  _interactive_shell->process_line("\\. " + _file_name);
  MY_EXPECT_STDOUT_CONTAINS("Processing file: " + file_name);
  MY_EXPECT_STDOUT_CONTAINS("The received arguments were: one and two");
  MY_EXPECT_STDERR_CONTAINS("");

  wipe_all();

  _file_name = "\"" + file_name + "\" one two";
  _interactive_shell->process_line("\\. " + _file_name);
  MY_EXPECT_STDOUT_CONTAINS("Processing file: " + file_name);
  MY_EXPECT_STDOUT_CONTAINS("The received arguments were: one and two");
  MY_EXPECT_STDERR_CONTAINS("");

  wipe_all();
}
#endif

TEST_F(Shell_core_test, test_process_py_file_with_params) {
  _options->interactive = false;
  reset_shell();

  _interactive_shell->process_line("\\py");

  std::string file_name =
      shcore::path::join_path(g_test_home, "data", "py", "script.py");

  _file_name = file_name + " one two";
  _interactive_shell->process_line("\\. " + _file_name);
  MY_EXPECT_STDOUT_CONTAINS("Processing file: " + file_name);
  MY_EXPECT_STDOUT_CONTAINS("The received arguments were: one and two");
  MY_EXPECT_STDERR_CONTAINS("");

  wipe_all();

  _file_name = "'" + file_name + "' one two";
  _interactive_shell->process_line("\\. " + _file_name);
  MY_EXPECT_STDOUT_CONTAINS("Processing file: " + file_name);
  MY_EXPECT_STDOUT_CONTAINS("The received arguments were: one and two");
  MY_EXPECT_STDERR_CONTAINS("");

  wipe_all();

  _file_name = "\"" + file_name + "\" one two";
  _interactive_shell->process_line("\\. " + _file_name);
  MY_EXPECT_STDOUT_CONTAINS("Processing file: " + file_name);
  MY_EXPECT_STDOUT_CONTAINS("The received arguments were: one and two");
  MY_EXPECT_STDERR_CONTAINS("");

  wipe_all();
}

TEST_F(Shell_core_test, python_dictionary_key_handling) {
  _options->interactive = true;
  reset_shell();

  _interactive_shell->process_line("\\py");

  // Tests a string key is correctly retrieved
  _interactive_shell->process_line("{'type':'sample'}");
  MY_EXPECT_STDOUT_CONTAINS("\"type\": \"sample\"");
  wipe_all();

  // Tests an object key retrieves it's string representation
  _interactive_shell->process_line("{type:'sample'}");
#define CLASS_KEYWORD "class"
  MY_EXPECT_STDOUT_CONTAINS("\"<" CLASS_KEYWORD " 'type'>\": \"sample\"");
#undef CLASS_KEYWORD
  wipe_all();
  wipe_all();
}

TEST_F(Shell_core_test, regression_prompt_on_override_session) {
  connect();

  EXPECT_EQ("mysql-sql []> ", _interactive_shell->prompt());
  _options->wizards = false;
  _interactive_shell->shell_context()->set_global(
      "session", Value(std::static_pointer_cast<Object_bridge>(
                     std::make_shared<mysqlsh::Options>(get_options()))));
  _options->wizards = true;
  EXPECT_EQ("mysql-sql []> ", _interactive_shell->prompt());

  // The session object has been overriden, even so we need to close th session
  _interactive_shell->shell_context()->get_dev_session()->close();
}

TEST_F(Shell_core_test, process_sql_no_delim_from_stream) {
  connect();

  std::stringstream stream("show databases");
  _ret_val = _interactive_shell->process_stream(stream, "STDIN", {});
  EXPECT_EQ(0, _ret_val);
  MY_EXPECT_STDOUT_CONTAINS("| Database");

  _interactive_shell->process_line("\\js");
  _interactive_shell->process_line("session.close();");
}

TEST_F(Shell_core_test, autocache_use_command_node) {
  execute("\\connect " + _uri);
  process("sql/simple_schema.sql");
  execute(to_scripting);
  wipe_all();
  execute("\\use simple_schema");
  execute("print(db.city)");
  MY_EXPECT_STDOUT_CONTAINS("<Table:city>");
  wipe_all();
  execute("print(db.city_list)");
  MY_EXPECT_STDOUT_CONTAINS("<Table:city_list>");
  wipe_all();
  execute("print(db.country_info)");
  MY_EXPECT_STDOUT_CONTAINS("<Collection:country_info>");
  wipe_all();
  execute("session.close();");
}

TEST_F(Shell_core_test, autocache_connect_command_node) {
  connect();
  process("sql/simple_schema.sql");
  execute(to_scripting);
  execute("session.close()");
  wipe_all();
  execute("\\connect " + _uri + "/simple_schema");
  execute("print(db.city)");
  MY_EXPECT_STDOUT_CONTAINS("<Table:city>");
  wipe_all();
  execute("print(db.city_list)");
  MY_EXPECT_STDOUT_CONTAINS("<Table:city_list>");
  wipe_all();
  execute("print(db.country_info)");
  MY_EXPECT_STDOUT_CONTAINS("<Collection:country_info>");
  wipe_all();
  execute("session.close();");
}

TEST_F(Shell_core_test, autocache_shell_connect_node) {
  connect();
  process("sql/simple_schema.sql");
  execute(to_scripting);
  execute("session.close()");
  wipe_all();
  execute("shell.connect('" + _uri + "/simple_schema')");
  execute("print(db.city)");
  MY_EXPECT_STDOUT_CONTAINS("<Table:city>");
  wipe_all();
  execute("print(db.city_list)");
  MY_EXPECT_STDOUT_CONTAINS("<Table:city_list>");
  wipe_all();
  execute("print(db.country_info)");
  MY_EXPECT_STDOUT_CONTAINS("<Collection:country_info>");
  wipe_all();
  execute("session.close();");
}
}  // namespace shell_core_tests
}  // namespace shcore
