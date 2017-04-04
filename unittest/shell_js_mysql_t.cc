/*
* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "shell_script_tester.h"
#include <boost/lexical_cast.hpp>
#include "utils/utils_general.h"
#include "mysqlshdk/libs/db/ssl_info.h"

namespace shcore {
class Shell_js_mysql_tests : public Shell_js_script_tester {
protected:
  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp() {
    Shell_js_script_tester::SetUp();

    int port = 3306, pwd_found;
    std::string protocol, user, password, host, sock, schema;
    mysqlshdk::utils::Ssl_info ssl_info;;
    shcore::parse_mysql_connstring(_uri, protocol, user, password, host, port, sock, schema, pwd_found, ssl_info);

    // Setups some variables on the JS context, these will be used on some test cases
    if (_mysql_port.empty())
      _mysql_port = "3306";

    std::string code = "var __user = '" + user + "';";
    exec_and_out_equals(code);
    code = "var __pwd = '" + password + "';";
    exec_and_out_equals(code);
    code = "var __host = '" + host + "';";
    exec_and_out_equals(code);
    code = "var __port = " + _mysql_port + ";";
    exec_and_out_equals(code);
    code = "var __schema = 'mysql';";
    exec_and_out_equals(code);
    code = "var __uri = '" + user + "@" + host + ":" + _mysql_port + "';";
    exec_and_out_equals(code);
    code = "var __uripwd = '" + user + ":" + password + "@" + host + ":" + _mysql_port + "';";
    exec_and_out_equals(code);
    code = "var __displayuri = '" + user + "@" + host + ":" + _mysql_port + "';";
    exec_and_out_equals(code);
    code = "var __displayuridb = '" + user + "@" + host + ":" + _mysql_port + "/mysql';";
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

TEST_F(Shell_js_mysql_tests, mysql_schema) {
  validate_interactive("mysql_schema.js");
}

TEST_F(Shell_js_mysql_tests, mysql_table) {
  validate_interactive("mysql_table.js");
}

TEST_F(Shell_js_mysql_tests, mysql_view) {
  validate_interactive("mysql_view.js");
}

TEST_F(Shell_js_mysql_tests, mysql_resultset) {
  validate_interactive("mysql_resultset.js");
}
}
