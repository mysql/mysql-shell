/* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

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

#include <string>
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/shell_script_tester.h"
#include "unittest/test_utils/shell_test_wrapper.h"
#include "utils/utils_string.h"
#include "mysqlshdk/libs/db/uri_encoder.h"

extern "C" const char *g_argv0;

namespace tests {

class Api_connections : public Shell_js_script_tester {
 public:
  virtual void SetUp() {
    Shell_js_script_tester::SetUp();

    set_config_folder("js_devapi");
    set_setup_script("setup.js");
    execute_setup();

    _my_port = get_sandbox_classic_port();
    _my_x_port = get_sandbox_x_port();

    _my_cnf_path = get_cnf_path();
    _my_sandbox_path = get_sandbox_path();

    std::string sandbox_path = get_sandbox_path();
    auto path_components =
      shcore::split_string(sandbox_path, _path_splitter);

    path_components.push_back(_my_port);
    path_components.push_back("sandboxdata");
    path_components.push_back("ca.pem");

    _my_ca_file = shcore::str_join(path_components, _path_splitter);
    mysqlshdk::db::uri::Uri_encoder encoder;
    _my_ca_file_uri = encoder.encode_value(_my_ca_file);

    execute("var __my_port = " + _my_port + ";");
    execute("var __my_x_port = " + _my_x_port + ";");
    execute("var __my_ca_file = '" + get_scripting_path(_my_ca_file) + "';");
    execute("var __my_ca_file_uri = '" + _my_ca_file_uri + "';");
    execute("var __sandbox_dir = '" + get_scripting_path(sandbox_path) + "';");
  }

  static void SetUpTestCase() {
    std::string sandbox_path = get_sandbox_path();

    // Deploys the sandbox instance to be used on this tests
    Shell_test_wrapper shell;
    shell.execute("dba.deploySandboxInstance(" + get_sandbox_classic_port() +
                  ", "
                  "{password: 'root', "
                  "sandboxDir: '" + get_scripting_path(sandbox_path) + "', "
                  "ignoreSslError: false});");

    ASSERT_STREQ("", shell.get_output_handler().std_err.c_str());
  }

  static void TearDownTestCase() {
    Shell_test_wrapper shell;
    std::string sandbox_path = get_sandbox_path();

    shell.execute("dba.stopSandboxInstance(" + get_sandbox_classic_port() +
                  ", "
                  "{sandboxDir: '" +
                  get_scripting_path(sandbox_path) +
                  "', "
                  "password: 'root'});");

    shell.execute("dba.killSandboxInstance(" + get_sandbox_classic_port() +
                  ", "
                  "{sandboxDir: '" +
                  get_scripting_path(sandbox_path) +
                  "'});");

    shell.execute("dba.deleteSandboxInstance(" + get_sandbox_classic_port() +
                  ", "
                  "{sandboxDir: '" +
                  get_scripting_path(sandbox_path) + "'});");

    // After sandboxes are down, they take a while until ports are released
    shcore::sleep_ms(5000);
  }

  static std::string get_scripting_path(const std::string& path) {
    std::string ret_val = path;
#ifdef WIN32
    auto tokens = shcore::split_string(path, "\\");
    ret_val = shcore::str_join(tokens, "\\\\");
#endif

    return ret_val;
  }

    static std::string get_sandbox_path() {
    std::string ret_val;

    const char *tmpdir = getenv("TMPDIR");
    if (tmpdir) {
      ret_val.assign(tmpdir);
    } else {
      // If not specified, the tests will create the sandboxes on the
      // binary folder
      ret_val = shcore::get_binary_folder();
    }

    return ret_val;
  }

  static int get_sandbox_port_number() {
    std::string ret_val;

    int offset = 0;
    const char *base_port = nullptr;
    base_port = getenv("MYSQL_SANDBOX_PORT1");
    if (!base_port) {
      base_port = getenv("MYSQL_PORT");
      offset = 10;
    }

    return atoi(base_port) + offset;
  }

  static std::string get_sandbox_classic_port() {
    return std::to_string(get_sandbox_port_number());
  }

  static std::string get_sandbox_x_port() {
    return std::to_string(get_sandbox_port_number() * 10);
  }

  static std::string get_cnf_path() {
    std::string ret_val;

    auto path_components =
        shcore::split_string(get_sandbox_path(), _path_splitter);
    if (path_components.back().empty())
      path_components.pop_back();
    path_components.push_back(get_sandbox_classic_port());

    std::vector<std::string> cnf_path_components(path_components);
    cnf_path_components.push_back("my.cnf");
    ret_val = shcore::str_join(cnf_path_components, _path_splitter);

    return ret_val;
  }


 protected:
  std::string _my_port;
  std::string _my_x_port;
  std::string _my_ca_file;
  std::string _my_ca_file_uri;
  std::string _my_sandbox_path;
  std::string _my_cnf_path;
};

TEST_F(Api_connections, ssl_enabled_require_secure_transport_off) {
  validate_chunks("api_connections.js",
                  "api_connections_ssl_on_secure_transport_off.val");
}

TEST_F(Api_connections, ssl_enabled_require_secure_transport_on) {
  // Turns the require_secure_transport variable ON
  execute("var uri = 'root:root@localhost:" + _my_port + "';");
  execute("var testSession = mysql.getClassicSession(uri);");
  execute("testSession.runSql('set global require_secure_transport=ON;');");

  validate_chunks("api_connections.js",
                  "api_connections_ssl_on_secure_transport_on.val");

  // Turns the require_secure_transport variable OFF
  execute("testSession.runSql('set global require_secure_transport=OFF;');");
  execute("testSession.close();");
}

TEST_F(Api_connections, ssl_disabled) {
  execute("localhost='localhost';");
  execute("dba.stopSandboxInstance(" + _my_port +
          ", "
          "{sandboxDir: '" +
          get_scripting_path(_my_sandbox_path) +
          "',"
          "password: 'root'});");

  add_to_cfg_file(_my_cnf_path, "ssl=OFF");

  execute("try_restart_sandbox(" + _my_port + ");");

  validate_chunks("api_connections.js",
                  "api_connections_ssl_off.val");
}

}  // namespace tests
