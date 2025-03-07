/*
 * Copyright (c) 2017, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/azure/signer.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/gtest_clean.h"
#include "unittest/mysqlshdk/libs/azure/test_utils.h"
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

  bool skip_set_defaults() {
    if (_skip_set_defaults) {
      _skip_set_defaults = false;
      return true;
    } else {
      return false;
    }
  }

 protected:
  void set_defaults() override {
    if (skip_set_defaults()) {
      return;
    }

    Shell_py_script_tester::set_defaults();

    std::string user, host, password;
    auto connection_options = mysqlshdk::db::Connection_options(_uri);

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

    code = shcore::str_format("__secure_password = '%.*s';",
                              static_cast<int>(k_secure_password.length()),
                              k_secure_password.data());
    exec_and_out_equals(code);

    for (int i = 0; i < sandbox::k_num_ports; ++i) {
      code = shcore::str_format("__mysql_sandbox_port%i = %i;", i + 1,
                                _mysql_sandbox_ports[i]);
      exec_and_out_equals(code);

      // With 8.0.27, the GR Protocol Communication Stack became MySQL by
      // default and localAddress is set to the server port by default
      if (_target_server_version < mysqlshdk::utils::Version("8.0.27")) {
        code = shcore::str_format("__mysql_sandbox_gr_port%i = %i;", i + 1,
                                  _mysql_sandbox_ports[i] * 10 + 1);
      } else {
        code = shcore::str_format("__mysql_sandbox_gr_port%i = %i;", i + 1,
                                  _mysql_sandbox_ports[i]);
      }
      exec_and_out_equals(code);

      code = shcore::str_format(
          "__sandbox_uri%i = 'mysql://root:root@localhost:%i';", i + 1,
          _mysql_sandbox_ports[i]);
      exec_and_out_equals(code);

      code = shcore::str_format(
          "__sandbox_uri_secure_password%i = 'mysql://root:%.*s@localhost:%i';",
          i + 1, static_cast<int>(k_secure_password.length()),
          k_secure_password.data(), _mysql_sandbox_ports[i]);
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

    // bundled plugins
    for (const auto plugin : {"gui", "mds", "mrs", "msm"}) {
      code = std::string("__has_") + plugin + "_plugin = '" + plugin +
             "' in globals()";
      execute(code);
    }
  }

  void run_and_check() {
    // Enable interactive/wizard mode if the test file ends with _interactive.py
    _options->wizards =
        strstr(GetParam().c_str(), "_interactive.") ? true : false;

    // if the test concerns the AdminAPI (without the "record" suffix) and the
    // current server version is not supported, we might as well skip it
    if (shcore::str_beginswith(GetParam(), "py_adminapi") &&
        !shcore::str_endswith(GetParam(), "_norecord")) {
      if (!Shell_test_env::check_min_version_skip_test()) return;
    }

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

    set_config_folder(shcore::path::join_path("auto", folder));

    // To allow handling python modules on the path of the scripts
    execute("import sys");
    std::string code = "sys.path.append('" + _scripts_home + "')";

#ifdef WIN32
    code = shcore::str_replace(code, "\\", "\\\\");
#endif
    execute(code);

    validate_interactive(name);
  }
};

TEST_P(Auto_script_py, run_and_check) {
  SCOPED_TRACE("Auto_script_py::run_and_check()");
  run_and_check();
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

  if (!getenv("MYSQLSH_S3_BUCKET_NAME") && subdir == "py_aws") {
    std::cout << "Skipping AWS Tests" << std::endl;
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

INSTANTIATE_TEST_SUITE_P(Aws_scripted, Auto_script_py,
                         testing::ValuesIn(find_py_tests("py_aws", ".py")),
                         fmt_param);

INSTANTIATE_TEST_SUITE_P(Dev_api_scripted, Auto_script_py,
                         testing::ValuesIn(find_py_tests("py_devapi", ".py")),
                         fmt_param);

INSTANTIATE_TEST_SUITE_P(Mixed_versions, Auto_script_py,
                         testing::ValuesIn(find_py_tests("py_mixed_versions",
                                                         ".py")),
                         fmt_param);

class Azure_tests_py : public Auto_script_py {
 public:
  void SetUp() override {
    Auto_script_py::SetUp();

    const auto config = testing::get_config(s_container_name);
    m_azure_configured = (config && config->valid());

    if (m_azure_configured) {
      m_endpoint = config->endpoint() + config->endpoint_path();
      m_using_emulator = m_endpoint.find("127.0.0.1") != std::string::npos;
      m_account = config->account_name();
      m_key = config->account_key();

      const auto signer = config->signer();
      const auto azure_signer =
          dynamic_cast<mysqlshdk::azure::Signer *>(signer.get());

      m_account_rwl_sas_token = azure_signer->create_account_sas_token();
      m_account_rl_sas_token =
          azure_signer->create_account_sas_token(mysqlshdk::azure::k_read_list);
    }
  }

  void TearDown() override {
    Auto_script_py::TearDown();

    if (m_azure_configured) {
      mysqlshdk::azure::Blob_container container(
          testing::get_config(s_container_name));

      testing::clean_container(container);
    }
  }

  static void SetUpTestCase() { testing::create_container(s_container_name); }

  static void TearDownTestCase() {
    testing::delete_container(s_container_name);
  }

  void set_defaults() override {
    if (skip_set_defaults()) {
      return;
    }

    Auto_script_py::set_defaults();

    execute(shcore::str_format("__container_name = '%s'", s_container_name));
    execute(shcore::str_format("__azure_configured = %s",
                               m_azure_configured ? "True" : "False"));
    execute(shcore::str_format("__azure_emulator = %s",
                               m_using_emulator ? "True" : "False"));
    execute(shcore::str_format("__azure_endpoint = '%s'", m_endpoint.c_str()));
    execute(shcore::str_format("__azure_account = '%s'", m_account.c_str()));
    execute(shcore::str_format("__azure_key = '%s'", m_key.c_str()));
    execute(shcore::str_format("__azure_account_rwl_sas_token = '%s'",
                               m_account_rwl_sas_token.c_str()));
    execute(shcore::str_format("__azure_account_rl_sas_token = '%s'",
                               m_account_rl_sas_token.c_str()));
  }

 private:
  static constexpr const char *s_container_name = "devtestingpy";
  bool m_azure_configured = false;
  std::string m_account;
  std::string m_key;
  std::string m_endpoint;
  std::string m_account_rwl_sas_token;
  std::string m_account_rl_sas_token;
  bool m_using_emulator;
};

TEST_P(Azure_tests_py, run_and_check) {
  SCOPED_TRACE("Azure_tests_py::run_and_check()");
  run_and_check();
}

INSTANTIATE_TEST_SUITE_P(Azure_scripted, Azure_tests_py,
                         testing::ValuesIn(find_py_tests("py_azure", ".py")),
                         fmt_param);

}  // namespace tests
