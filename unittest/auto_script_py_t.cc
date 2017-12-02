/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "unittest/gtest_clean.h"
#include "unittest/shell_script_tester.h"

extern "C" const char *g_test_home;

namespace tests {

class Auto_script_py : public Shell_py_script_tester,
                           public ::testing::WithParamInterface<std::string> {
 protected:
  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp() {
    // Force reset_shell() to happen when reset_shell() is called explicitly
    // in each test case
    _delay_reset_shell = true;
    Shell_py_script_tester::SetUp();

    // Common setup script
    set_setup_script(shcore::path::join_path(g_test_home, "scripts", "setup_py", "setup.py"));
  }

  void reset_replayable_shell(const char *sub_test_name) {
    setup_recorder(sub_test_name);  // must be called before set_defaults()
    reset_shell();
    enable_testutil();
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
          if (sql == "SELECT @@datadir") {
            datadir_column = 0;
          } else if (sql == "select @@port, @@datadir;") {
            datadir_column = 1;
          }

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

          if (sql.find("@@datadir") != std::string::npos)
            throw std::logic_error(
                "query contains datadir but is not intercepted");
          return source;
        });
  }

  virtual void set_defaults() {
    Shell_py_script_tester::set_defaults();

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

    std::string mysql_uri = "mysql://";

    if (_port.empty())
      _port = "33060";

    if (_port.empty()) {
      _port = "33060";
    }
    if (_mysql_port.empty()) {
      _mysql_port = "3306";
    }

    std::string code = "hostname = '" + _hostname + "';";
    exec_and_out_equals(code);
    code = "hostname_ip = '" + _hostname_ip + "';";
    exec_and_out_equals(code);
    code = "__user = '" + user + "';";
    exec_and_out_equals(code);
    code = "__pwd = '" + password + "';";
    exec_and_out_equals(code);
    code = "__host = '" + host + "';";
    exec_and_out_equals(code);
    code = "__port = " + _port + ";";
    exec_and_out_equals(code);
    code = "__schema = 'mysql';";
    exec_and_out_equals(code);
    code = "__uri = '" + user + "@" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "__xhost_port = '" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "__host_port = '" + host + ":" + _mysql_port + "';";
    exec_and_out_equals(code);
    code = "__mysql_port = " + _mysql_port + ";";
    exec_and_out_equals(code);
    code = "__mysql_sandbox_port1 = " + _mysql_sandbox_port1 + ";";
    exec_and_out_equals(code);
    code = "__mysql_sandbox_port2 = " + _mysql_sandbox_port2 + ";";
    exec_and_out_equals(code);
    code = "__mysql_sandbox_port3 = " + _mysql_sandbox_port3 + ";";
    exec_and_out_equals(code);
    code = "__sandbox_uri1 = 'mysql://root:root@localhost:" +
           _mysql_sandbox_port1 + "';";
    exec_and_out_equals(code);
    code = "__sandbox_uri2 = 'mysql://root:root@localhost:" +
           _mysql_sandbox_port2 + "';";
    exec_and_out_equals(code);
    code = "__sandbox_uri3 = 'mysql://root:root@localhost:" +
           _mysql_sandbox_port3 + "';";
    exec_and_out_equals(code);

    code = "localhost = 'localhost'";
    exec_and_out_equals(code);

    code = "__uripwd = '" + user + ":" + password + "@" + host + ":" +
           _port + "';";
    exec_and_out_equals(code);
    code = "__mysqluripwd = '" + user + ":" + password + "@" + host + ":" +
           _mysql_port + "';";
    exec_and_out_equals(code);
  }
};

TEST_P(Auto_script_py, run_and_check) {
  // Enable interactive/wizard mode if the test file ends with _interactive.py
  _options->wizards =
      strstr(GetParam().c_str(), "_interactive.") ? true : false;

  std::string folder;
  std::string name;
  std::tie(folder, name) = shcore::str_partition(GetParam(), "/");

  // Does not enable recording engine for the devapi tests
  // Recording for CRUD is not available
  if (folder != "py_devapi")
    reset_replayable_shell(name.c_str());

  fprintf(stdout, "Test script: %s\n", GetParam().c_str());
  exec_and_out_equals("__script_file = '" + GetParam() + "'");

  set_config_folder("auto/" + folder);
  validate_interactive(name);
}

std::vector<std::string> find_py_tests(const std::string& subdir) {
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

// General test cases
INSTANTIATE_TEST_CASE_P(Admin_api_scripted, Auto_script_py,
                        testing::ValuesIn(find_py_tests("py_adminapi")));

INSTANTIATE_TEST_CASE_P(Shell_scripted, Auto_script_py,
                        testing::ValuesIn(find_py_tests("py_shell")));

INSTANTIATE_TEST_CASE_P(Dev_api_scripted, Auto_script_py,
                        testing::ValuesIn(find_py_tests("py_devapi")));

}  // namespace tests
