/*
 * Copyright (c) 2015, 2024, Oracle and/or its affiliates.
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
#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/shell_script_tester.h"

namespace shcore {
class Shell_js_mysql_tests : public Shell_js_script_tester {
 protected:
  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp() {
    Shell_js_script_tester::SetUp();

    std::string user, host, password;
    auto connection_options = shcore::get_connection_options(_uri);

    if (connection_options.has_user()) user = connection_options.get_user();

    if (connection_options.has_host()) host = connection_options.get_host();

    if (connection_options.has_password())
      password = connection_options.get_password();

    // Setups some variables on the JS context, these will be used on some test
    // cases
    if (_mysql_port.empty()) _mysql_port = "3306";

    std::string code = "var __user = '" + user + "';";
    exec_and_out_equals(code);
    code = "var __host = '" + host + "';";
    exec_and_out_equals(code);
    code = "var __port = " + _mysql_port + ";";
    exec_and_out_equals(code);
    code = "var __socket = '" + _mysql_socket + "';";
    exec_and_out_equals(code);
    code = "var __xsocket = '" + _socket + "';";
    exec_and_out_equals(code);
    code = "var __schema = 'mysql';";
    exec_and_out_equals(code);
    code = "var __uri = '" + user + "@" + host + ":" + _mysql_port + "';";
    exec_and_out_equals(code);
    code = "var __uripwd = '" + user + ":" + password + "@" + host + ":" +
           _mysql_port + "';";
    exec_and_out_equals(code);
    code =
        "var __displayuri = '" + user + "@" + host + ":" + _mysql_port + "';";
    exec_and_out_equals(code);
    code = "var __displayuridb = '" + user + "@" + host + ":" + _mysql_port +
           "/mysql';";
    exec_and_out_equals(code);
    code = "var __system_user = '" + get_system_user() + "';";
    exec_and_out_equals(code);

    // All of the test cases share the same config folder
    // and setup script
    set_config_folder("js_devapi");
    set_setup_script("setup.js");
  }
};

TEST_F(Shell_js_mysql_tests, mysql_module) {
  validate_interactive("mysql_module.js");
}

TEST_F(Shell_js_mysql_tests, mysql_session) {
  validate_interactive("mysql_session.js");
}

TEST_F(Shell_js_mysql_tests, mysql_resultset) {
  validate_interactive("mysql_resultset.js");
}

#if !(_WIN32 || __APPLE__)
// Windows and MacOS doesn't support auth_socket plugin.
// SO_PEERCRED socket option is required.
TEST_F(Shell_js_mysql_tests, mysql_auth_socket) {
  validate_interactive("mysql_auth_socket.js");
}
#endif
}  // namespace shcore
