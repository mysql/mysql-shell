/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#ifndef UNITTEST_TEST_UTILS_TEST_SERVER_H_
#define UNITTEST_TEST_UTILS_TEST_SERVER_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/utils/process_launcher.h"

#include "unittest/test_utils/cleanup.h"

namespace tests {

class Test_server {
 public:
  Test_server() = default;

  Test_server(const Test_server &) = delete;
  Test_server(Test_server &&) = default;

  Test_server &operator=(const Test_server &) = delete;
  Test_server &operator=(Test_server &&) = default;

  ~Test_server() = default;

  bool start_server(int start_port, bool use_env_var = true, bool https = true);

  void stop_server();

  const std::string &address() const { return m_address; }

  int port() const { return m_port; }

  bool is_alive();

 private:
  void setup_env_vars();

  void clear_env_vars();

  std::unique_ptr<shcore::Process_launcher> m_server;
  std::string m_address;
  int m_port;
  Cleanup m_env_vars;
};

}  // namespace tests

#endif  // UNITTEST_TEST_UTILS_TEST_SERVER_H_
