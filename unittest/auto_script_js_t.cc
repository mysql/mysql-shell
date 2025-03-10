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

#include "modules/adminapi/common/preconditions.h"
#include "mysqlshdk/libs/azure/signer.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/gtest_clean.h"
#include "unittest/mysqlshdk/libs/azure/test_utils.h"
#include "unittest/shell_script_tester.h"

extern "C" const char *g_test_home;
extern const char *g_mysqld_path_variables;

namespace tests {

class Auto_script_js : public Shell_js_script_tester,
                       public ::testing::WithParamInterface<std::string> {
 protected:
  bool _skip_set_defaults = false;

  // You can define per-test set-up and tear-down logic as usual.
  void SetUp() override {
    _skip_set_defaults = true;
    Shell_js_script_tester::SetUp();

    // Common setup script
    set_setup_script(shcore::path::join_path(g_test_home, "scripts", "setup_js",
                                             "setup.js"));
  }

  bool skip_set_defaults() {
    if (_skip_set_defaults) {
      _skip_set_defaults = false;
      return true;
    } else {
      return false;
    }
  }

  void set_defaults() override {
    if (skip_set_defaults()) {
      return;
    }

    Shell_js_script_tester::set_defaults();

    std::string user, host, password;
    auto connection_options = mysqlshdk::db::Connection_options(_uri);

    if (connection_options.has_user()) user = connection_options.get_user();

    if (connection_options.has_host()) host = connection_options.get_host();

    if (connection_options.has_password())
      password = connection_options.get_password();

    assert(!m_port.empty());
    assert(!m_mysql_port.empty());

    std::string code = "var hostname = '" + hostname() + "';";
    execute(code);
    code = "var real_hostname = '" + real_hostname() + "';";
    execute(code);

    if (real_host_is_loopback())
      code = "var real_host_is_loopback = true;";
    else
      code = "var real_host_is_loopback = false;";
    execute(code);

    code = "var hostname_ip = '" + hostname_ip() + "';";
    execute(code);
    code = "var __user = '" + user + "';";
    execute(code);
    code = "var __host = '" + host + "';";
    execute(code);
    code = "var __port = " + m_port + ";";
    execute(code);
    code = "var __mysql_port = " + m_mysql_port + ";";
    execute(code);

    code = shcore::str_format("var __secure_password = '%.*s';",
                              static_cast<int>(k_secure_password.length()),
                              k_secure_password.data());
    exec_and_out_equals(code);

    code = "var __uri = '" + user + "@" + host + ":" + m_port + "';";
    execute(code);
    code =
        "var __mysql_uri = '" + user + "@" + host + ":" + m_mysql_port + "';";
    execute(code);

    for (int i = 0; i < sandbox::k_num_ports; ++i) {
      code = shcore::str_format("var __mysql_sandbox_port%i = %i;", i + 1,
                                _mysql_sandbox_ports[i]);
      execute(code);
      // With 8.0.27, the GR Protocol Communication Stack became MySQL by
      // default and localAddress is set to the server port by default
      if (_target_server_version < mysqlshdk::utils::Version("8.0.27")) {
        code = shcore::str_format("var __mysql_sandbox_gr_port%i = %i;", i + 1,
                                  _mysql_sandbox_ports[i] * 10 + 1);
      } else {
        code = shcore::str_format("var __mysql_sandbox_gr_port%i = %i;", i + 1,
                                  _mysql_sandbox_ports[i]);
      }
      execute(code);
      code = shcore::str_format("var __mysql_sandbox_x_port%i = %i;", i + 1,
                                _mysql_sandbox_ports[i] * 10);
      execute(code);

      code = shcore::str_format(
          "var __sandbox_uri%i = 'mysql://root:root@localhost:%i';", i + 1,
          _mysql_sandbox_ports[i]);
      execute(code);

      code = shcore::str_format(
          "var __sandbox_uri_secure_password%i = "
          "'mysql://root:%.*s@localhost:%i';",
          i + 1, static_cast<int>(k_secure_password.length()),
          k_secure_password.data(), _mysql_sandbox_ports[i]);
      exec_and_out_equals(code);

      code = shcore::str_format("var __endpoint%i = '%s:%i';", i + 1,
                                hostname().c_str(), _mysql_sandbox_ports[i]);
      execute(code);

      code = shcore::str_format("var __sandbox%i = 'localhost:%i';", i + 1,
                                _mysql_sandbox_ports[i]);
      execute(code);

      code = shcore::str_format(
          "var __hostname_uri%i = 'mysql://root:root@%s:%i';", i + 1,
          hostname().c_str(), _mysql_sandbox_ports[i]);
      execute(code);
    }

    code = "var localhost = 'localhost'";
    execute(code);

    code = "var __uripwd = '" + user + ":" + password + "@" + host + ":" +
           m_port + "';";
    execute(code);
    code = "var __mysqluripwd = '" + user + ":" + password + "@" + host + ":" +
           m_mysql_port + "';";
    execute(code);

    // for MySQL 5.6 tests
    if (getenv("MYSQL56_URI")) {
      code = "var __mysql56_uri = '";
      code += getenv("MYSQL56_URI");
      code += "';";
      execute(code);
    } else {
      execute("var __mysql56_uri = null;");
    }

    code = "var __system_user = '" + shcore::get_system_user() + "';";
    execute(code);

    if (_replaying)
      code = "var __replaying = true;";
    else
      code = "var __replaying = false;";
    execute(code);
    if (_recording)
      code = "var __recording = true;";
    else
      code = "var __recording = false;";
    execute(code);

    code = "var __data_path = " +
           shcore::quote_string(shcore::path::join_path(g_test_home, "data"),
                                '\'') +
           ";";
    execute(code);

    code = "var __import_data_path = " +
           shcore::quote_string(
               shcore::path::join_path(g_test_home, "data", "import"), '\'') +
           ";";
    execute(code);

    code =
        "var __tmp_dir = " + shcore::quote_string(getenv("TMPDIR"), '\'') + ";";
    execute(code);

    code = "var __bin_dir = " +
           shcore::quote_string(shcore::get_binary_folder(), '\'') + ";";
    execute(code);

    code =
        "var __mysqlsh = " + shcore::quote_string(get_path_to_mysqlsh(), '\'') +
        ";";
    execute(code);

    code = "var __dba_data_path = " +
           shcore::quote_string(
               shcore::path::join_path(g_test_home, "data", "dba"), '\'') +
           ";";
    execute(code);

    // bundled plugins
    for (const auto plugin : {"gui", "mds", "mrs", "msm"}) {
      code = std::string("var __has_") + plugin +
             "_plugin = (function() {try {" + plugin +
             ";return true;} catch(e) {return false;}})();";
      execute(code);
    }
  }

  void run_and_check() {
    // Enable interactive/wizard mode if the test file ends with _interactive.js
    _options->wizards =
        strstr(GetParam().c_str(), "_interactive.") ? true : false;

    // if the test concerns the AdminAPI (without the "record" suffix) and the
    // current server version is not supported, we might as well skip it
    if (shcore::str_beginswith(GetParam(), "js_adminapi") &&
        !shcore::str_endswith(GetParam(), "_norecord")) {
      if (!Shell_test_env::check_min_version_skip_test()) return;
    }

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

    if (g_mysqld_path_variables && folder == "js_mixed_versions") {
      auto variables = shcore::str_split(g_mysqld_path_variables, ",");

      std::string secondary_var_name{"MYSQLD_SECONDARY_SERVER_A"};

      for (const auto &variable : variables) {
        auto serverParts = shcore::str_split(variable, ";");
        assert(serverParts.size() == 2);

        auto &serverVersion = serverParts[0];
        auto &serverPath = serverParts[1];

        {
          auto code =
              shcore::str_format("var %s = os.getenv('%s')", serverPath.c_str(),
                                 serverPath.c_str());
          execute(code);
        }

        if (secondary_var_name.back() <= 'Z') {
          auto code = shcore::str_format(
              "var %s = { path: os.getenv('%s'), version: \"%s\" }",
              secondary_var_name.c_str(), serverPath.c_str(),
              serverVersion.c_str());
          execute(code);

          secondary_var_name.back()++;
        }
      }
    }

    execute("const __script_file = '" + GetParam() + "'");

    set_config_folder("auto/" + folder);
    wipe_all();
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

  void set_options() override {
    _options->interactive =
        strstr(GetParam().c_str(), "_interactive.") ? true : false;
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
    if (skip_set_defaults()) {
      return;
    }

    Auto_script_js::set_defaults();

    std::string user = std::string(k_first_user);
    std::string password = std::string(k_first_password);
    std::string code = "var __cred = {x:{}, mysql:{}, second:{}, third:{}};";
    execute(code);
    code = "__cred.user = '" + user + "';";
    execute(code);
    code = "__cred.pwd = '" + password + "';";
    execute(code);
    code = "__cred.host = '" + m_host + "';";
    execute(code);
    code = "__cred.x.port = " + _port + ";";
    execute(code);
    code = "__cred.mysql.port = " + _mysql_port + ";";
    execute(code);
    code = "__cred.x.uri = '" + user + "@" + m_host + ":" + _port + "';";
    execute(code);
    code =
        "__cred.mysql.uri = '" + user + "@" + m_host + ":" + _mysql_port + "';";
    execute(code);
    code = "__cred.x.uri_pwd = '" + user + ":" + password + "@" + m_host + ":" +
           _port + "';";
    execute(code);
    code = "__cred.mysql.uri_pwd = '" + user + ":" + password + "@" + m_host +
           ":" + _mysql_port + "';";
    execute(code);
    code = "__cred.x.host_port = '" + m_host + ":" + _port + "';";
    execute(code);
    code = "__cred.mysql.host_port = '" + m_host + ":" + _mysql_port + "';";
    execute(code);

    user = std::string(k_second_user);
    password = std::string(k_second_password);

    code = "__cred.second.uri_pwd = '" + user + ":" + password + "@" + m_host +
           ":" + _port + "';";
    execute(code);
    code = "__cred.second.uri = '" + user + "@" + m_host + ":" + _port + "';";
    execute(code);
    code = "__cred.second.pwd = '" + password + "';";
    execute(code);

    user = std::string(k_third_user);
    password = std::string(k_third_password);

    code = "__cred.third.uri_pwd = '" + user + ":" + password + "@" + m_host +
           ":" + _port + "';";
    execute(code);
    code = "__cred.third.uri = '" + user + "@" + m_host + ":" + _port + "';";
    execute(code);
    code = "__cred.third.pwd = '" + password + "';";
    execute(code);
    wipe_all();
  }

 private:
  void prepare_session() {
    m_session = mysqlshdk::db::mysql::Session::create();
    m_session->connect(mysqlshdk::db::Connection_options(_mysql_uri));
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
    if (skip_set_defaults()) {
      return;
    }

    Auto_script_js::set_defaults();

    std::string code = "var __pager = {};";
    execute(code);
    code = "__pager.file = '" + s_output_file + "';";
    execute(code);
    code =
        "__pager.cmd = '" + s_pager_binary + " --file=" + s_output_file + "';";
    execute(code);
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

class Azure_tests : public Auto_script_js {
 public:
  void SetUp() override {
    Auto_script_js::SetUp();
    auto config = testing::get_config("devtesting");
    m_azure_configured = (config && config->valid());
    if (m_azure_configured) {
      m_endpoint = config->endpoint() + config->endpoint_path();
      m_using_emulator = m_endpoint.find("127.0.0.1") != std::string::npos;
      m_account = config->account_name();
      m_key = config->account_key();

      auto signer = config->signer();
      auto azure_signer =
          dynamic_cast<mysqlshdk::azure::Signer *>(signer.get());
      m_account_rwl_sas_token = azure_signer->create_account_sas_token();
      m_account_rl_sas_token =
          azure_signer->create_account_sas_token(mysqlshdk::azure::k_read_list);
    }
  }

  void TearDown() override {
    Auto_script_js::TearDown();
    if (m_azure_configured) {
      mysqlshdk::azure::Blob_container container(
          testing::get_config("devtesting"));

      testing::clean_container(container);
    }
  }

  static void SetUpTestCase() { testing::create_container("devtesting"); }

  static void TearDownTestCase() { testing::delete_container("devtesting"); }

  void set_defaults() override {
    if (skip_set_defaults()) {
      return;
    }

    Auto_script_js::set_defaults();

    execute("let __container_name = 'devtesting';");
    execute(shcore::str_format("let __azure_configured = %s;",
                               m_azure_configured ? "true" : "false"));
    execute(shcore::str_format("let __azure_emulator = %s;",
                               m_using_emulator ? "true" : "false"));
    execute(
        shcore::str_format("let __azure_endpoint = '%s';", m_endpoint.c_str()));
    execute(
        shcore::str_format("let __azure_account = '%s';", m_account.c_str()));
    execute(shcore::str_format("let __azure_key = '%s';", m_key.c_str()));
    execute(shcore::str_format("let __azure_account_rwl_sas_token = '%s';",
                               m_account_rwl_sas_token.c_str()));
    execute(shcore::str_format("let __azure_account_rl_sas_token = '%s';",
                               m_account_rl_sas_token.c_str()));

    // Updates the sys.path to enable loading modules at the js_azure/scripts
    // folder
    execute("var __paths = []");
    execute("for(p in sys.path) { __paths.push(sys.path[p]); }");
    execute(shcore::str_format(
        "__paths.push('%s');",
        shcore::path::join_path(g_test_home, "scripts", "auto", "js_azure",
                                "scripts")
            .c_str()));
    execute("sys.path = __paths");
  }

 private:
  bool m_azure_configured = false;
  std::string m_account;
  std::string m_key;
  std::string m_endpoint;
  std::string m_account_rwl_sas_token;
  std::string m_account_rl_sas_token;
  bool m_using_emulator;
};

TEST_P(Azure_tests, run_and_check) {
  SCOPED_TRACE("Azure_tests::run_and_check()");
  run_and_check();
}

INSTANTIATE_TEST_SUITE_P(Azure_scripted, Azure_tests,
                         testing::ValuesIn(find_js_tests("js_azure", ".js")),
                         fmt_param);

}  // namespace tests
