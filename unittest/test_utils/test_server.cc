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

#include "unittest/test_utils/test_server.h"

#ifndef _WIN32
#include <unistd.h>
extern char **environ;
#endif

#include <iostream>

#include "mysqlshdk/libs/rest/rest_service.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_path.h"

extern "C" const char *g_test_home;

namespace tests {

namespace {

bool debug() {
  const static bool s_debug = ::getenv("TEST_DEBUG") != nullptr;
  return s_debug;
}

void debug_env_vars() {
  if (debug()) {
    std::cerr << "Environment variables:" << std::endl;

#ifdef _WIN32
    auto env = GetEnvironmentStringsA();
    const auto env_head = env;

    while (*env) {
      std::cerr << '\t' << env << std::endl;
      env += strlen(env) + 1;
    }

    if (env_head) {
      FreeEnvironmentStringsA(env_head);
    }
#else
    auto env = environ;

    while (*env) {
      std::cerr << '\t' << *env << std::endl;
      ++env;
    }
#endif
  }
}

bool find_free_port(int *port, bool use_env_var) {
  bool port_found = false;

  if (const auto test_port = ::getenv("MYSQLSH_TEST_HTTP_PORT");
      use_env_var && test_port) {
    *port = std::stod(test_port);
    port_found = true;
  } else {
    for (int i = 0; !port_found && i < 100; ++i) {
      if (!mysqlshdk::utils::Net::is_port_listening("127.0.0.1", *port + i)) {
        *port += i;
        port_found = true;
      }
    }
  }

  return port_found;
}

}  // namespace

Test_server::Test_server() { setup_env_vars(); }

bool Test_server::start_server(int start_port, bool use_env_var, bool https) {
  const auto tag = https ? "HTTPS" : "HTTP";
  m_port = start_port;

  if (!find_free_port(&m_port, use_env_var)) {
    std::cerr << "Could not find an available port for " << tag << " server"
              << std::endl;
    return false;
  }

  const auto shell_executable =
      shcore::path::join_path(shcore::get_binary_folder(), "mysqlsh");
  const auto script =
      shcore::path::join_path(g_test_home, "data", "rest", "test-server.py");
  const auto port_number = std::to_string(m_port);
  m_address =
      "http" + std::string{https ? "s" : ""} + "://127.0.0.1:" + port_number;

  bool server_ready = false;

  const std::vector<const char *> args = {
      shell_executable.c_str(),
      "--py",
      "--file",
      script.c_str(),
      port_number.c_str(),
      https ? nullptr : "no-https",
      nullptr,
  };

  if (debug()) {
    std::cerr << "executing: ";

    for (const auto arg : args) {
      if (arg) {
        std::cerr << arg << ' ';
      }
    }

    std::cerr << std::endl;
  }

  m_server = std::make_unique<shcore::Process_launcher>(&args[0]);
  m_server->enable_reader_thread();
#ifdef _WIN32
  m_server->set_create_process_group();
#endif  // _WIN32
  m_server->start();

  static constexpr uint32_t sleep_time = 100;   // 100ms
  static constexpr uint32_t wait_time = 10100;  // 10s
  uint32_t current_time = 0;
  mysqlshdk::rest::Rest_service rest{m_address, false};

  if (debug()) {
    std::cerr << "Trying to connect to: " << m_address << std::endl;
  }

  while (!server_ready) {
    shcore::sleep_ms(sleep_time);
    current_time += sleep_time;

    // Server not running or time exceeded
    if (m_server->check() || current_time > wait_time) break;

    try {
      auto request = mysqlshdk::rest::Request("/ping");
      const auto response = rest.head(&request);

      if (debug()) {
        std::cerr << tag << " server replied with: "
                  << mysqlshdk::rest::Response::status_code(response.status)
                  << ", body: " << response.buffer.raw()
                  << ", waiting time: " << current_time << "ms" << std::endl;
      }

      server_ready =
          response.status == mysqlshdk::rest::Response::Status_code::OK;
    } catch (const std::exception &e) {
      std::cerr << tag << " server not ready after " << current_time << "ms";

      if (debug()) {
        std::cerr << ": " << e.what() << std::endl;
        std::cerr << "Output so far:" << std::endl;
        std::cerr << m_server->read_all();
      }

      std::cerr << std::endl;
    }
  }

  if (server_ready) {
    std::cerr << tag << " server is running: " << address() << std::endl;
  } else {
    std::cerr << tag << " server at " << address() << " failed to start"
              << std::endl;
  }

  return server_ready;
}

void Test_server::stop_server() {
  if (debug()) {
    std::cerr << m_server->read_all() << std::endl;
  }

  m_server->kill();
  m_server.reset();
}

bool Test_server::is_alive() { return m_server && !m_server->check(); }

void Test_server::setup_env_vars() {
  debug_env_vars();

  m_cleanup = Cleanup::set_env_var("no_proxy", "*");

  debug_env_vars();
}

}  // namespace tests
