/*
 * Copyright (c) 2017, 2021, Oracle and/or its affiliates.
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
#include "mysqlshdk/libs/utils/utils_stacktrace.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/gtest_clean.h"
#include "unittest/shell_script_tester.h"

extern "C" const char *g_test_home;
extern const char *g_mysqld_path_variables;

namespace tests {

class Auto_script_js : public Shell_js_script_tester,
                       public ::testing::WithParamInterface<std::string> {
 protected:
  bool _skip_set_defaults = false;

  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp() {
    _skip_set_defaults = true;
    Shell_js_script_tester::SetUp();

    // Common setup script
    set_setup_script(shcore::path::join_path(g_test_home, "scripts", "setup_js",
                                             "setup.js"));
  }

  virtual void set_defaults() {
    if (_skip_set_defaults) {
      _skip_set_defaults = false;
      return;
    }

    Shell_js_script_tester::set_defaults();

    std::string user, host, password;
    auto connection_options = shcore::get_connection_options(_uri);

    if (connection_options.has_user()) user = connection_options.get_user();

    if (connection_options.has_host()) host = connection_options.get_host();

    if (connection_options.has_password())
      password = connection_options.get_password();

    assert(!m_port.empty());
    assert(!m_mysql_port.empty());

    std::string code = "var hostname = '" + hostname() + "';";
    exec_and_out_equals(code);
    code = "var real_hostname = '" + real_hostname() + "';";
    exec_and_out_equals(code);

    if (real_host_is_loopback())
      code = "var real_host_is_loopback = true;";
    else
      code = "var real_host_is_loopback = false;";
    exec_and_out_equals(code);

    code = "var hostname_ip = '" + hostname_ip() + "';";
    exec_and_out_equals(code);
    code = "var __user = '" + user + "';";
    exec_and_out_equals(code);
    code = "var __host = '" + host + "';";
    exec_and_out_equals(code);
    code = "var __port = " + m_port + ";";
    exec_and_out_equals(code);
    code = "var __mysql_port = " + m_mysql_port + ";";
    exec_and_out_equals(code);
    code = "var __uri = '" + user + "@" + host + ":" + m_port + "';";
    exec_and_out_equals(code);
    code =
        "var __mysql_uri = '" + user + "@" + host + ":" + m_mysql_port + "';";
    exec_and_out_equals(code);
    for (int i = 0; i < sandbox::k_num_ports; ++i) {
      code = shcore::str_format("var __mysql_sandbox_port%i = %i;", i + 1,
                                _mysql_sandbox_ports[i]);
      exec_and_out_equals(code);
      code = shcore::str_format("var __mysql_sandbox_gr_port%i = %i;", i + 1,
                                _mysql_sandbox_ports[i] * 10 + 1);
      exec_and_out_equals(code);
      code = shcore::str_format("var __mysql_sandbox_x_port%i = %i;", i + 1,
                                _mysql_sandbox_ports[i] * 10);
      exec_and_out_equals(code);
      code = shcore::str_format(
          "var __sandbox_uri%i = 'mysql://root:root@localhost:%i';", i + 1,
          _mysql_sandbox_ports[i]);
      exec_and_out_equals(code);
      code = shcore::str_format("var __endpoint%i = '%s:%i';", i + 1,
                                hostname().c_str(), _mysql_sandbox_ports[i]);
      exec_and_out_equals(code);
      code = shcore::str_format("var __sandbox%i = 'localhost:%i';", i + 1,
                                _mysql_sandbox_ports[i]);
      exec_and_out_equals(code);
      code = shcore::str_format(
          "var __hostname_uri%i = 'mysql://root:root@%s:%i';", i + 1,
          hostname().c_str(), _mysql_sandbox_ports[i]);
      exec_and_out_equals(code);
    }

    code = "var localhost = 'localhost'";
    exec_and_out_equals(code);

    code = "var __uripwd = '" + user + ":" + password + "@" + host + ":" +
           m_port + "';";
    exec_and_out_equals(code);
    code = "var __mysqluripwd = '" + user + ":" + password + "@" + host + ":" +
           m_mysql_port + "';";
    exec_and_out_equals(code);

    // for MySQL 5.6 tests
    if (getenv("MYSQL56_URI")) {
      code = "var __mysql56_uri = '";
      code += getenv("MYSQL56_URI");
      code += "';";
      exec_and_out_equals(code);
    } else {
      exec_and_out_equals("var __mysql56_uri = null;");
    }

    code = "var __system_user = '" + shcore::get_system_user() + "';";
    exec_and_out_equals(code);

    if (_replaying)
      code = "var __replaying = true;";
    else
      code = "var __replaying = false;";
    exec_and_out_equals(code);
    if (_recording)
      code = "var __recording = true;";
    else
      code = "var __recording = false;";
    exec_and_out_equals(code);

    code = "var __data_path = " +
           shcore::quote_string(shcore::path::join_path(g_test_home, "data"),
                                '\'') +
           ";";
    exec_and_out_equals(code);

    code = "var __import_data_path = " +
           shcore::quote_string(
               shcore::path::join_path(g_test_home, "data", "import"), '\'') +
           ";";
    exec_and_out_equals(code);

    code =
        "var __tmp_dir = " + shcore::quote_string(getenv("TMPDIR"), '\'') + ";";
    exec_and_out_equals(code);

    code = "var __bin_dir = " +
           shcore::quote_string(shcore::get_binary_folder(), '\'') + ";";
    exec_and_out_equals(code);

    code =
        "var __mysqlsh = " + shcore::quote_string(get_path_to_mysqlsh(), '\'') +
        ";";
    exec_and_out_equals(code);

    code = "var __dba_data_path = " +
           shcore::quote_string(
               shcore::path::join_path(g_test_home, "data", "dba"), '\'') +
           ";";
    exec_and_out_equals(code);
  }

  void run_and_check() {
    // Enable interactive/wizard mode if the test file ends with _interactive.js
    _options->wizards =
        strstr(GetParam().c_str(), "_interactive.") ? true : false;

    SCOPED_TRACE(GetParam());

    std::string folder;
    std::string name;
    std::tie(folder, name) = shcore::str_partition(GetParam(), "/");

    // Does not enable recording engine for the devapi tests
    // Recording for CRUD is not available
    if (folder != "js_devapi" &&
        GetParam().find("_norecord") == std::string::npos) {
      reset_replayable_shell(name.c_str());
    } else {
      set_defaults();
      execute_setup();
    }

    // todo(kg): _norecord files haven't defined functions from script.js, e.g.
    // `EXPECT_NE`. reset_replayable_shell call reset_shell therefore we lost
    // those previously defined and called in constructor functions.
    // I run setup.js script here once again, but this should be done somewhere
    // else, but I don't know where.
    set_setup_script(shcore::path::join_path(g_test_home, "scripts", "setup_js",
                                             "setup.js"));

    if (g_mysqld_path_variables && folder == "js_mixed_versions") {
      auto variables = shcore::str_split(g_mysqld_path_variables, ",");
      for (const auto &variable : variables) {
        std::string code = shcore::str_format(
            "var %s = os.getenv('%s')", variable.c_str(), variable.c_str());
        exec_and_out_equals(code);
      }
    }

    const std::vector<std::string> argv;
    _interactive_shell->process_file(_setup_script, argv);

    exec_and_out_equals("const __script_file = '" + GetParam() + "'");

    set_config_folder("auto/" + folder);
    validate_interactive(name);

    // ensure nothing left in expected prompts
    EXPECT_EQ(0, output_handler.prompts.size());
    EXPECT_EQ(0, output_handler.passwords.size());
  }
};

TEST_P(Auto_script_js, run_and_check) {
  SCOPED_TRACE("Auto_script_js::run_and_check()");
  run_and_check();
}

std::vector<std::string> find_js_tests(const std::string &subdir,
                                       const std::string &ext) {
  std::string path = shcore::path::join_path(g_test_home, "scripts", "auto",
                                             subdir, "scripts");
  if (!shcore::is_folder(path)) return {};
  auto tests = shcore::listdir(path);
  std::sort(tests.begin(), tests.end());

  std::vector<std::string> filtered;
  for (const auto &s : tests) {
    // We let files starting with underscore as modules
    if (shcore::str_endswith(s, ext) && s[0] != '_')
      filtered.push_back(subdir + "/" + s);
  }
  return filtered;
}

// General test cases
INSTANTIATE_TEST_SUITE_P(Admin_api, Auto_script_js,
                         testing::ValuesIn(find_js_tests("js_adminapi", ".js")),
                         fmt_param);

INSTANTIATE_TEST_SUITE_P(Admin_api_async, Auto_script_js,
                         testing::ValuesIn(find_js_tests("js_adminapi_async",
                                                         ".js")),
                         fmt_param);

INSTANTIATE_TEST_SUITE_P(
    Admin_api_clusterset, Auto_script_js,
    testing::ValuesIn(find_js_tests("js_adminapi_clusterset", ".js")),
    fmt_param);

INSTANTIATE_TEST_SUITE_P(Shell_scripted, Auto_script_js,
                         testing::ValuesIn(find_js_tests("js_shell", ".js")),
                         fmt_param);

INSTANTIATE_TEST_SUITE_P(Dev_api_scripted, Auto_script_js,
                         testing::ValuesIn(find_js_tests("js_devapi", ".js")),
                         fmt_param);

INSTANTIATE_TEST_SUITE_P(Mixed_versions, Auto_script_js,
                         testing::ValuesIn(find_js_tests("js_mixed_versions",
                                                         ".js")),
                         fmt_param);

namespace {

constexpr auto k_first_user = "cluster_admin";
constexpr auto k_first_password = "P4ssW0rd";

constexpr auto k_second_user = "default_user";
constexpr auto k_second_password = "secret";

constexpr auto k_third_user = "reporter";
constexpr auto k_third_password = "retroper";

}  // namespace

class Credential_store_test : public Auto_script_js {
 protected:
  void SetUp() override {
    prepare_session();
    Auto_script_js::SetUp();

    execute("shell.options.unset(\"credentialStore.helper\");");
    execute("shell.options.unset(\"credentialStore.savePasswords\");");
    execute("shell.options.unset(\"credentialStore.excludeFilters\");");
  }

  void TearDown() override {
    execute(
        "(function(){var a = shell.listCredentialHelpers();"
        "for (var i = 0; i < a.length; ++i) {"
        "shell.options[\"credentialStore.helper\"] = a[i];"
        "shell.deleteAllCredentials();}})();");
    execute("shell.options[\"credentialStore.helper\"] = \"<disabled>\";");

    Auto_script_js::TearDown();
    terminate_session();
  }

  void set_defaults() override {
    Auto_script_js::set_defaults();

    std::string user = std::string(k_first_user);
    std::string password = std::string(k_first_password);
    std::string code = "var __cred = {x:{}, mysql:{}, second:{}, third:{}};";
    exec_and_out_equals(code);
    code = "__cred.user = '" + user + "';";
    exec_and_out_equals(code);
    code = "__cred.pwd = '" + password + "';";
    exec_and_out_equals(code);
    code = "__cred.host = '" + m_host + "';";
    exec_and_out_equals(code);
    code = "__cred.x.port = " + _port + ";";
    exec_and_out_equals(code);
    code = "__cred.mysql.port = " + _mysql_port + ";";
    exec_and_out_equals(code);
    code = "__cred.x.uri = '" + user + "@" + m_host + ":" + _port + "';";
    exec_and_out_equals(code);
    code =
        "__cred.mysql.uri = '" + user + "@" + m_host + ":" + _mysql_port + "';";
    exec_and_out_equals(code);
    code = "__cred.x.uri_pwd = '" + user + ":" + password + "@" + m_host + ":" +
           _port + "';";
    exec_and_out_equals(code);
    code = "__cred.mysql.uri_pwd = '" + user + ":" + password + "@" + m_host +
           ":" + _mysql_port + "';";
    exec_and_out_equals(code);
    code = "__cred.x.host_port = '" + m_host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "__cred.mysql.host_port = '" + m_host + ":" + _mysql_port + "';";
    exec_and_out_equals(code);

    user = std::string(k_second_user);
    password = std::string(k_second_password);

    code = "__cred.second.uri_pwd = '" + user + ":" + password + "@" + m_host +
           ":" + _port + "';";
    exec_and_out_equals(code);
    code = "__cred.second.uri = '" + user + "@" + m_host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "__cred.second.pwd = '" + password + "';";
    exec_and_out_equals(code);

    user = std::string(k_third_user);
    password = std::string(k_third_password);

    code = "__cred.third.uri_pwd = '" + user + ":" + password + "@" + m_host +
           ":" + _port + "';";
    exec_and_out_equals(code);
    code = "__cred.third.uri = '" + user + "@" + m_host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "__cred.third.pwd = '" + password + "';";
    exec_and_out_equals(code);
  }

 private:
  void prepare_session() {
    m_session = mysqlshdk::db::mysql::Session::create();
    m_session->connect(shcore::get_connection_options(_mysql_uri));
    const auto &connection_options = m_session->get_connection_options();
    if (connection_options.has_host()) m_host = connection_options.get_host();
    m_session->executef("CREATE USER ?@? IDENTIFIED BY ?", k_first_user, m_host,
                        k_first_password);
    m_session->executef("CREATE USER ?@? IDENTIFIED BY ?", k_first_user, "%",
                        k_first_password);
    m_session->executef("CREATE USER ?@? IDENTIFIED BY ?", k_second_user,
                        m_host, k_second_password);
    m_session->executef("CREATE USER ?@? IDENTIFIED BY ?", k_third_user, m_host,
                        k_third_password);
  }

  void terminate_session() {
    m_session->executef("DROP USER ?@?", k_first_user, m_host);
    m_session->executef("DROP USER ?@?", k_first_user, "%");
    m_session->executef("DROP USER ?@?", k_second_user, m_host);
    m_session->executef("DROP USER ?@?", k_third_user, m_host);
    m_session->close();
  }

  std::shared_ptr<mysqlshdk::db::ISession> m_session;
  std::string m_host;
};

TEST_P(Credential_store_test, run_and_check) {
  SCOPED_TRACE("Credential_store_test::run_and_check()");
  run_and_check();
}

INSTANTIATE_TEST_SUITE_P(Credential_store_scripted, Credential_store_test,
                         testing::ValuesIn(find_js_tests("js_credential",
                                                         ".js")),
                         fmt_param);

class Pager_test : public Auto_script_js {
 public:
  static void SetUpTestCase() {
    static const auto tmp_dir = getenv("TMPDIR");
    static constexpr auto pager_file = "pager.txt";
    s_output_file = shcore::str_replace(
        shcore::path::join_path(tmp_dir, pager_file), "\\", "\\\\");

    static const auto binary_folder = shcore::get_binary_folder();
    static constexpr auto sample_pager = "sample_pager";
    s_pager_binary = shcore::str_replace(
        shcore::path::join_path(binary_folder, sample_pager), "\\", "\\\\");
  }

 protected:
  void TearDown() override {
    Auto_script_js::TearDown();

    shcore::delete_file(s_output_file);
  }

  void set_defaults() override {
    Auto_script_js::set_defaults();

    std::string code = "var __pager = {};";
    exec_and_out_equals(code);
    code = "__pager.file = '" + s_output_file + "';";
    exec_and_out_equals(code);
    code =
        "__pager.cmd = '" + s_pager_binary + " --file=" + s_output_file + "';";
    exec_and_out_equals(code);
  }

 private:
  static std::string s_output_file;
  static std::string s_pager_binary;
};

std::string Pager_test::s_output_file;
std::string Pager_test::s_pager_binary;

TEST_P(Pager_test, run_and_check) {
  SCOPED_TRACE("Pager_test::run_and_check()");
  run_and_check();
}

INSTANTIATE_TEST_SUITE_P(Pager_scripted, Pager_test,
                         testing::ValuesIn(find_js_tests("js_pager", ".js")),
                         fmt_param);

}  // namespace tests
