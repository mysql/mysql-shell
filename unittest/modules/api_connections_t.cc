/* Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.

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

#include <string>
#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/shell_script_tester.h"
#include "unittest/test_utils/shell_test_wrapper.h"
#include "utils/utils_string.h"

namespace tests {

class Api_connections : public Shell_js_script_tester {
 public:
  virtual void SetUp() {
    if (_sb_port == 0) FAIL() << "No MySQL Server available for test.";

    enable_debug();
    Shell_js_script_tester::SetUp();

    set_config_folder("js_devapi");
    set_setup_script("setup.js");
    execute_setup();

    _my_port = std::to_string(_sb_port);
    _my_x_port = get_sandbox_x_port();

    _my_cnf_path = testutil->get_sandbox_conf_path(_sb_port);
    _my_ca_file = testutil->get_sandbox_path(_sb_port, "ca.pem");
    mysqlshdk::db::uri::Uri_encoder encoder;
    _my_ca_file_uri = encoder.encode_value(_my_ca_file);

    execute("var __my_port = " + _my_port + ";");
    execute("var __my_x_port = " + _my_x_port + ";");
    execute("var __my_ca_file = testutil.getSandboxPath(__my_port, 'ca.pem');");
    execute("var __my_ca_file_uri = '" + _my_ca_file_uri + "';");
    execute("var __sandbox_dir = testutil.getSandboxPath();");
    execute("var __my_user = 'root';");

    auto tls1_2 = mysqlshdk::utils::Version("1.2");
    if (_highest_tls_version > tls1_2) {
      execute("var __default_cipher = 'TLS_AES_256_GCM_SHA384';");
    } else if (_highest_tls_version == tls1_2) {
      execute("var __default_cipher = 'DHE-RSA-AES128-GCM-SHA256';");
    } else {
      execute("var __default_cipher = 'DHE-RSA-AES256-SHA';");
    }
  }

  static void SetUpTestCase() {
    Shell_test_wrapper shell_env(true);

    std::vector<int> ports{shell_env.sb_port1(), shell_env.sb_port2(),
                           shell_env.sb_port3()};

    for (auto port : ports) {
      try {
        shell_env.utils()->deploy_sandbox(port, "root");
        _sb_port = port;
        break;
      } catch (const std::runtime_error &err) {
        std::cerr << err.what();
        shell_env.utils()->destroy_sandbox(port, true);
      }
    }
  }

  static void TearDownTestCase() {
    Shell_test_wrapper shell_env(true);
    if (_sb_port != 0) shell_env.utils()->destroy_sandbox(_sb_port);
  }

  static std::string get_scripting_path(const std::string &path) {
    std::string ret_val = path;
#ifdef _WIN32
    ret_val = shcore::str_replace(ret_val, "\\", "/");
#endif
    return ret_val;
  }

  void disable_ssl_on_instance(int port, const std::string &unsecure_user) {
    auto session = mysqlshdk::db::mysql::Session::create();

    auto connection_options = shcore::get_connection_options(
        "root:root@localhost:" + std::to_string(port), false);
    session->connect(connection_options);

    session->query("create user " + unsecure_user +
                   "@'%' identified with "
                   "mysql_native_password by 'root'");
    session->close();

    testutil->stop_sandbox(port, "root");
    testutil->change_sandbox_conf(port, "ssl", "0", "mysqld");
    testutil->change_sandbox_conf(port, "default_authentication_plugin",
                                  "mysql_native_password", "mysqld");
    testutil->start_sandbox(port);
  }

  std::string get_sandbox_x_port() { return std::to_string(_sb_port * 10); }

 protected:
  std::string _my_port;
  std::string _my_x_port;
  std::string _my_ca_file;
  std::string _my_ca_file_uri;
  std::string _my_sandbox_path;
  std::string _my_cnf_path;
  static int _sb_port;
};

int Api_connections::_sb_port = 0;

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
  disable_ssl_on_instance(_sb_port, "unsecure");

  execute("var __my_user = 'unsecure';");

  shcore::sleep_ms(5000);

  validate_chunks("api_connections.js", "api_connections_ssl_off.val");
}

}  // namespace tests
