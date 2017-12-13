/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_stacktrace.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "unittest/gtest_clean.h"
#include "unittest/shell_script_tester.h"

extern "C" const char *g_test_home;

namespace tests {

static int find_column_in_select_stmt(const std::string &sql,
  const std::string &column) {
  std::string s = shcore::str_lower(sql);
  // sanity checks for things we don't support
  assert(s.find(" from ") == std::string::npos);
  assert(s.find(" where ") == std::string::npos);
  assert(s.find("select ") == 0);
  assert(column.find(" ") == std::string::npos);
  // other not supported things...: ``, column names with special chars,
  // aliases, stuff in comments etc
  s = s.substr(strlen("select "));

  // trim out stuff inside parenthesis which can confuse the , splitter and
  // are not supported anyway, like:
  // select (select a, b from something), c
  // select concat(a, b), c
  std::string::size_type p = s.find("(");
  while (p != std::string::npos) {
    std::string::size_type pp = s.find(")", p);
    if (pp != std::string::npos) {
      s = s.substr(0, p + 1) + s.substr(pp);
    } else {
      break;
    }
    p = s.find("(", pp);
  }

  std::vector<std::string> columns(shcore::str_split(s, ","));
  // the last column name can contain other stuff
  columns[columns.size() - 1] =
      shcore::str_split(shcore::str_strip(columns.back()), " \t\n\r").front();

  int i = 0;
  for (const auto &c : columns) {
    if (shcore::str_strip(c) == column)
      return i;
    ++i;
  }
  return -1;
}


class Auto_script_js : public Shell_js_script_tester,
                           public ::testing::WithParamInterface<std::string> {
 protected:
  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp() {
    // Force reset_shell() to happen when reset_shell() is called explicitly
    // in each test case
    _delay_reset_shell = true;
    Shell_js_script_tester::SetUp();

    // Common setup script
    set_setup_script(shcore::path::join_path(g_test_home, "scripts", "setup_js", "setup.js"));
  }

  void reset_replayable_shell(const char *sub_test_name) {
    setup_recorder(sub_test_name);  // must be called before set_defaults()
    reset_shell();
    execute_setup();

#ifdef _WIN32
    mysqlshdk::db::replay::set_replay_query_hook([](const std::string& sql) {
      return shcore::str_replace(sql, ".dll", ".so");
    });
#endif

    // Intercept queries and hack their results so that we can have
    // recorded local sessions that match the actual local environment
    mysqlshdk::db::replay::set_replay_row_hook(
        [this](const mysqlshdk::db::Connection_options& target,
               const std::string& sql,
               std::unique_ptr<mysqlshdk::db::IRow> source)
            -> std::unique_ptr<mysqlshdk::db::IRow> {
          int datadir_column = -1;

          if (sql.find("@@datadir") != std::string::npos &&
              shcore::str_ibeginswith(sql, "select ")) {
            // find the index for @@datadir in the query
            datadir_column = find_column_in_select_stmt(sql, "@@datadir");
            assert(datadir_column >= 0);
          } else {
            assert(sql.find("@@datadir") == std::string::npos);
          }

          // replace sandbox @@datadir from results with actual datadir
          if (datadir_column >= 0) {
            std::string prefix = shcore::path::dirname(
                shcore::path::dirname(source->get_string(datadir_column)));
            std::string suffix =
                source->get_string(datadir_column).substr(prefix.length() + 1);
            std::string datadir = shcore::path::join_path(_sandbox_dir, suffix);
#ifdef _WIN32
            datadir = shcore::str_replace(datadir, "/", "\\");
#endif
            return std::unique_ptr<mysqlshdk::db::IRow>{new Override_row_string(
                std::move(source), datadir_column, datadir)};
          }
          return source;
        });
  }

  virtual void set_defaults() {
    Shell_js_script_tester::set_defaults();

    std::string user, host, password;
    auto connection_options = shcore::get_connection_options(_uri);

    if (getenv("TEST_DEBUG"))
      output_handler.set_log_level(ngcommon::Logger::LOG_DEBUG);

    if (connection_options.has_user())
      user = connection_options.get_user();

    if (connection_options.has_host())
      host = connection_options.get_host();

    if (connection_options.has_password())
      password = connection_options.get_password();

    if (_port.empty())
      _port = "33060";

    if (_port.empty()) {
      _port = "33060";
    }
    if (_mysql_port.empty()) {
      _mysql_port = "3306";
    }

    std::string code = "var hostname = '" + _hostname + "';";
    exec_and_out_equals(code);
    code = "var hostname_ip = '" + _hostname_ip + "';";
    exec_and_out_equals(code);
    code = "var __user = '" + user + "';";
    exec_and_out_equals(code);
    code = "var __pwd = '" + password + "';";
    exec_and_out_equals(code);
    code = "var __host = '" + host + "';";
    exec_and_out_equals(code);
    code = "var __port = " + _port + ";";
    exec_and_out_equals(code);
    code = "var __schema = 'mysql';";
    exec_and_out_equals(code);
    code = "var __uri = '" + user + "@" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "var __mysql_uri = '" + user + "@" + host + ":" + _mysql_port + "';";
    exec_and_out_equals(code);
    code = "var __xhost_port = '" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "var __host_port = '" + host + ":" + _mysql_port + "';";
    exec_and_out_equals(code);
    code = "var __mysql_port = " + _mysql_port + ";";
    exec_and_out_equals(code);
    code = "var __mysql_sandbox_port1 = " + _mysql_sandbox_port1 + ";";
    exec_and_out_equals(code);
    code = "var __mysql_sandbox_port2 = " + _mysql_sandbox_port2 + ";";
    exec_and_out_equals(code);
    code = "var __mysql_sandbox_port3 = " + _mysql_sandbox_port3 + ";";
    exec_and_out_equals(code);
    code = "var __sandbox_uri1 = 'mysql://root:root@localhost:" +
           _mysql_sandbox_port1 + "';";
    exec_and_out_equals(code);
    code = "var __sandbox_uri2 = 'mysql://root:root@localhost:" +
           _mysql_sandbox_port2 + "';";
    exec_and_out_equals(code);
    code = "var __sandbox_uri3 = 'mysql://root:root@localhost:" +
           _mysql_sandbox_port3 + "';";
    exec_and_out_equals(code);

    code = "var localhost = 'localhost'";
    exec_and_out_equals(code);

    code = "var __uripwd = '" + user + ":" + password + "@" + host + ":" +
           _port + "';";
    exec_and_out_equals(code);
    code = "var __mysqluripwd = '" + user + ":" + password + "@" + host + ":" +
           _mysql_port + "';";
    exec_and_out_equals(code);
  }
};

TEST_P(Auto_script_js, run_and_check) {
  // Enable interactive/wizard mode if the test file ends with _interactive.js
  _options->wizards =
      strstr(GetParam().c_str(), "_interactive.") ? true : false;

  std::string folder;
  std::string name;
  std::tie(folder, name) = shcore::str_partition(GetParam(), "/");

  // Does not enable recording engine for the devapi tests
  // Recording for CRUD is not available
  if (folder != "js_devapi")
    reset_replayable_shell(name.c_str());

  fprintf(stdout, "Test script: %s\n", GetParam().c_str());
  exec_and_out_equals("const __script_file = '" + GetParam() + "'");

  set_config_folder("auto/" + folder);
  validate_interactive(name);

  // ensure nothing left in expected prompts
  EXPECT_EQ(0, output_handler.prompts.size());
  EXPECT_EQ(0, output_handler.passwords.size());
}

std::vector<std::string> find_js_tests(const std::string& subdir) {
  std::string path = shcore::path::join_path(
      g_test_home, "scripts", "auto", subdir, "scripts");
  if (!shcore::is_folder(path))
    return {};
  auto tests = shcore::listdir(path);
  std::sort(tests.begin(), tests.end());

  for (auto &s : tests)
    s = subdir+"/"+s;
  return tests;
}

#if 0
// Once we upgrade to gtest 1.8 this should be passed to INSTANTIATE_TEST_CASE_P
// so we can get filenames in the test name instead of /0
std::string fmt_param(testing::TestParamInfo<std::string> info) {
  return info.GetParam();
}
#endif

// General test cases
INSTANTIATE_TEST_CASE_P(Admin_api_scripted, Auto_script_js,
                        testing::ValuesIn(find_js_tests("js_adminapi")));

INSTANTIATE_TEST_CASE_P(Shell_scripted, Auto_script_js,
                        testing::ValuesIn(find_js_tests("js_shell")));

INSTANTIATE_TEST_CASE_P(Dev_api_scripted, Auto_script_js,
                        testing::ValuesIn(find_js_tests("js_devapi")));

}  // namespace tests
