/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
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
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/gtest_clean.h"
#include "unittest/shell_script_tester.h"

#include <stdlib.h>

extern "C" const char *g_test_home;
extern const char *g_mysqld_path_variables;

namespace tests {

class Auto_script_py : public Shell_py_script_tester,
                       public ::testing::WithParamInterface<std::string> {
 protected:
  bool _skip_set_defaults = false;

  // You can define per-test set-up and tear-down logic as usual.
  void SetUp() override {
    _skip_set_defaults = true;
    Shell_py_script_tester::SetUp();

    // Common setup script
    set_setup_script(shcore::path::join_path(g_test_home, "scripts", "setup_py",
                                             "setup.py"));
  }

 protected:
  void set_defaults() override {
    if (_skip_set_defaults) {
      _skip_set_defaults = false;
      return;
    }
    Shell_py_script_tester::set_defaults();

    std::string user, host, password;
    auto connection_options = shcore::get_connection_options(_uri);

    if (connection_options.has_user()) user = connection_options.get_user();

    if (connection_options.has_host()) host = connection_options.get_host();

    if (connection_options.has_password())
      password = connection_options.get_password();

    assert(!m_port.empty());
    assert(!m_mysql_port.empty());

    std::string code = "hostname = '" + hostname() + "';";
    exec_and_out_equals(code);

    code = "real_hostname = '" + real_hostname() + "';";
    exec_and_out_equals(code);

    if (real_host_is_loopback())
      code = "real_host_is_loopback = True;";
    else
      code = "real_host_is_loopback = False;";
    exec_and_out_equals(code);

    code = "hostname_ip = '" + hostname_ip() + "';";
    exec_and_out_equals(code);
    code = "__user = '" + user + "';";
    exec_and_out_equals(code);
    code = "__host = '" + host + "';";
    exec_and_out_equals(code);
    code = "__port = " + m_port + ";";
    exec_and_out_equals(code);
    code = "__schema = 'mysql';";
    exec_and_out_equals(code);
    code = "__uri = '" + user + "@" + host + ":" + m_port + "';";
    exec_and_out_equals(code);
    code = "__mysql_uri = '" + user + "@" + host + ":" + m_mysql_port + "';";
    exec_and_out_equals(code);
    code = "__xhost_port = '" + host + ":" + m_port + "';";
    exec_and_out_equals(code);
    code = "__host_port = '" + host + ":" + m_mysql_port + "';";
    exec_and_out_equals(code);
    code = "__mysql_port = " + m_mysql_port + ";";
    exec_and_out_equals(code);
    for (int i = 0; i < sandbox::k_num_ports; ++i) {
      code = shcore::str_format("__mysql_sandbox_port%i = %i;", i + 1,
                                _mysql_sandbox_ports[i]);
      exec_and_out_equals(code);

      code = shcore::str_format("__mysql_sandbox_gr_port%i = %i;", i + 1,
                                _mysql_sandbox_ports[i] * 10 + 1);
      exec_and_out_equals(code);

      code = shcore::str_format(
          "__sandbox_uri%i = 'mysql://root:root@localhost:%i';", i + 1,
          _mysql_sandbox_ports[i]);
      exec_and_out_equals(code);
    }

    code = "localhost = 'localhost'";
    exec_and_out_equals(code);

    code = "__uripwd = '" + user + ":" + password + "@" + host + ":" + m_port +
           "';";
    exec_and_out_equals(code);
    code = "__mysqluripwd = '" + user + ":" + password + "@" + host + ":" +
           m_mysql_port + "';";
    exec_and_out_equals(code);

    code = "__system_user = '" + shcore::get_system_user() + "';";
    exec_and_out_equals(code);

    if (_replaying)
      code = "__replaying = True;";
    else
      code = "__replaying = False;";
    exec_and_out_equals(code);
    if (_recording)
      code = "__recording = True;";
    else
      code = "__recording = False;";
    exec_and_out_equals(code);

    code = "__home = '" + shcore::path::home() + "'";
#ifdef WIN32
    code = shcore::str_replace(code, "\\", "\\\\");
#endif
    exec_and_out_equals(code);

    code = "__data_path = " +
           shcore::quote_string(shcore::path::join_path(g_test_home, "data"),
                                '\'');
    exec_and_out_equals(code);

    code = "__import_data_path = " +
           shcore::quote_string(
               shcore::path::join_path(g_test_home, "data", "import"), '\'');
    exec_and_out_equals(code);

    code = "__tmp_dir = " + shcore::quote_string(getenv("TMPDIR"), '\'');
    exec_and_out_equals(code);

    code = "__bin_dir = " +
           shcore::quote_string(shcore::get_binary_folder(), '\'');
    exec_and_out_equals(code);

    code = "__mysqlsh = " + shcore::quote_string(get_path_to_mysqlsh(), '\'');
    exec_and_out_equals(code);
  }
};

TEST_P(Auto_script_py, run_and_check) {
  // Enable interactive/wizard mode if the test file ends with _interactive.py
  _options->wizards =
      strstr(GetParam().c_str(), "_interactive.") ? true : false;

  SCOPED_TRACE(GetParam());

  std::string folder;
  std::string name;
  std::tie(folder, name) = shcore::str_partition(GetParam(), "/");

  // Does not enable recording engine for the devapi tests
  // Recording for CRUD is not available
  if (folder != "py_devapi" &&
      GetParam().find("_norecord") == std::string::npos) {
    reset_replayable_shell(name.c_str());
  } else {
    set_defaults();
    execute_setup();
  }

  if (folder == "py_shell")
    output_handler.set_answers_to_stdout(true);
  else
    output_handler.set_answers_to_stdout(false);

  if (g_mysqld_path_variables) {
    auto variables = shcore::str_split(g_mysqld_path_variables, ",");

    std::string secondary_var_name{"MYSQLD_SECONDARY_SERVER_A"};

    exec_and_out_equals("import os");
    for (const auto &variable : variables) {
      auto serverParts = shcore::str_split(variable, ";");
      assert(serverParts.size() == 2);

      auto &serverVersion = serverParts[0];
      auto &serverPath = serverParts[1];

      {
        std::string code = shcore::str_format(
            "%s = os.getenv('%s')", serverPath.c_str(), serverPath.c_str());
        exec_and_out_equals(code);
      }

      if (secondary_var_name.back() <= 'Z') {
        auto code = shcore::str_format(
            "%s = { \"path\": os.getenv('%s'), \"version\": \"%s\" }",
            secondary_var_name.c_str(), serverPath.c_str(),
            serverVersion.c_str());
        execute(code);

        secondary_var_name.back()++;
      }
    }
  }

  fprintf(stdout, "Test script: %s\n", GetParam().c_str());
  exec_and_out_equals("__script_file = '" + GetParam() + "'");

  set_config_folder("auto/" + folder);

  // To allow handling python modules on the path of the scripts
  execute("import sys");
  execute("sys.path.append('" + _scripts_home + "')");

  validate_interactive(name);
}

std::vector<std::string> find_py_tests(const std::string &subdir,
                                       const std::string &ext) {
  std::string path = shcore::path::join_path(g_test_home, "scripts", "auto",
                                             subdir, "scripts");
  if (!shcore::is_folder(path)) return {};

  std::cout << subdir << std::endl;
  if (!getenv("OCI_CONFIG_HOME") && subdir == "py_oci") {
    std::cout << "Skipping OCI Tests" << std::endl;
    return {};
  }

  auto tests = shcore::listdir(path);
  std::sort(tests.begin(), tests.end());

  std::vector<std::string> filtered;

  for (const auto &s : tests) {
    // We let files starting with underscore as modules
    if (shcore::str_endswith(s, ext) && s[0] != '_')
      filtered.emplace_back(subdir + "/" + s);
  }

  return filtered;
}

// General test cases
INSTANTIATE_TEST_SUITE_P(Admin_api, Auto_script_py,
                         testing::ValuesIn(find_py_tests("py_adminapi", ".py")),
                         fmt_param);

INSTANTIATE_TEST_SUITE_P(Admin_api_async, Auto_script_py,
                         testing::ValuesIn(find_py_tests("py_adminapi_async",
                                                         ".py")),
                         fmt_param);

INSTANTIATE_TEST_SUITE_P(
    Admin_api_clusterset, Auto_script_py,
    testing::ValuesIn(find_py_tests("py_adminapi_clusterset", ".py")),
    fmt_param);

INSTANTIATE_TEST_SUITE_P(Shell_scripted, Auto_script_py,
                         testing::ValuesIn(find_py_tests("py_shell", ".py")),
                         fmt_param);

INSTANTIATE_TEST_SUITE_P(Oci_scripted, Auto_script_py,
                         testing::ValuesIn(find_py_tests("py_oci", ".py")),
                         fmt_param);

INSTANTIATE_TEST_SUITE_P(Dev_api_scripted, Auto_script_py,
                         testing::ValuesIn(find_py_tests("py_devapi", ".py")),
                         fmt_param);

INSTANTIATE_TEST_SUITE_P(Mixed_versions, Auto_script_py,
                         testing::ValuesIn(find_py_tests("py_mixed_versions",
                                                         ".py")),
                         fmt_param);
}  // namespace tests
