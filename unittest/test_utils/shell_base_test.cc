/* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.

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

#include <fstream>
#include "unittest/test_utils/shell_base_test.h"
#include "shellcore/types.h"
#include "utils/utils_general.h"
#include "utils/utils_json.h"

namespace tests {

void Shell_base_test::SetUp() {
  const char *uri = getenv("MYSQL_URI");
  if (uri) {
    // Creates connection data and recreates URI, this will fix URI if no
    // password is defined So the UT don't prompt for password ever
    shcore::Value::Map_type_ref data = shcore::get_connection_data(uri);

    _host = data->get_string("host");
    _user = data->get_string("dbUser");

    const char *pwd = getenv("MYSQL_PWD");
    if (pwd) {
      _pwd.assign(pwd);
      (*data)["dbPassword"] = shcore::Value(_pwd);
    }

    _uri = shcore::build_connection_string(data, true);
    _mysql_uri = _uri;

    const char *xport = getenv("MYSQLX_PORT");
    if (xport) {
      _port_number = atoi(xport);
      _port.assign(xport);
      (*data)["port"] = shcore::Value(_pwd);
      _uri += ":" + _port;
    }
    _uri_nopasswd = shcore::strip_password(_uri);

    const char *port = getenv("MYSQL_PORT");
    if (port) {
      _mysql_port_number = atoi(port);
      _mysql_port.assign(port);
      _mysql_uri += ":" + _mysql_port;
    }

    _mysql_uri_nopasswd = shcore::strip_password(_mysql_uri);
  }
}

void Shell_base_test::TearDown() {
  _servers.clear();
}

void Shell_base_test::create_file(const std::string& name,
                                  const std::string& content) {
  std::ofstream file(name, std::ofstream::out | std::ofstream::trunc);

  if (file.is_open()) {
    file << content;
    file.close();
  } else {
    SCOPED_TRACE("Error Creating File: " + name);
    ADD_FAILURE();
  }
}

void Shell_base_test::check_string_expectation(const std::string& expected_str,
                                               const std::string &actual,
                                               bool expected) {
  bool found = actual.find(expected_str) != std::string::npos;

  if (found != expected) {
    std::string error = expected ? "Missing" : "Unexpected";
    error += " Output: " + expected_str;
    SCOPED_TRACE("Actual: " + actual);
    SCOPED_TRACE(error);
    ADD_FAILURE();
  }
}

void Shell_base_test::start_server_mock
  (int port, const std::vector< Fake_result_data >& data) {
  assert(_servers.find(port) == _servers.end());
  _servers[port] = std::shared_ptr<Server_mock>(new Server_mock());
  _servers[port]->start(port, data);
}

void Shell_base_test::stop_server_mock(int port) {
  _servers[port]->stop();
  _servers.erase(port);
}

}  // namespace tests
