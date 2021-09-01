/*
 * Copyright (c) 2019, 2021 Oracle and/or its affiliates. All rights reserved.
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

#include <iterator>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

#include "mysqlshdk/libs/rest/rest_service.h"
#include "mysqlshdk/libs/rest/retry_strategy.h"
#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "unittest/test_utils/shell_test_env.h"

extern "C" const char *g_test_home;

namespace mysqlshdk {
namespace rest {
namespace test {

class Test_server {
 public:
  bool start_server(int start_port, bool use_env_var) {
    m_port = start_port;

    if (!find_free_port(&m_port, use_env_var)) {
      std::cerr << "Could not find an available port for HTTPS server"
                << std::endl;
      return false;
    }

    const auto script =
        shcore::path::join_path(g_test_home, "data", "rest", "test-server.py");
    const auto port_number = std::to_string(m_port);
    m_address = "https://127.0.0.1:" + port_number;

    const auto debug = getenv("TEST_DEBUG") != nullptr;
    bool server_ready = false;

    for (const auto python_cmd : {"python", "python3"}) {
      if (server_ready) break;

      if (debug) {
        std::cerr << "executing: " << python_cmd << " " << script << " "
                  << port_number << std::endl;
      }

      std::vector<const char *> args = {python_cmd, script.c_str(),
                                        port_number.c_str(), nullptr};

      m_server = std::make_unique<shcore::Process_launcher>(&args[0]);
      m_server->enable_reader_thread();
#ifdef _WIN32
      m_server->set_create_process_group();
#endif  // _WIN32
      m_server->start();

      static constexpr uint32_t sleep_time = 100;   // 100ms
      static constexpr uint32_t wait_time = 10100;  // 10s
      uint32_t current_time = 0;
      Rest_service rest{m_address, false};

      if (debug) {
        std::cerr << "Trying to connect to: " << m_address << std::endl;
      }

      while (!server_ready) {
        shcore::sleep_ms(sleep_time);
        current_time += sleep_time;

        // Server not running or time exceeded
        if (m_server->check() || current_time > wait_time) break;

        try {
          auto request = Request("/ping");
          const auto response = rest.head(&request);

          if (debug) {
            std::cerr << "HTTPS server replied with: "
                      << Response::status_code(response.status)
                      << ", body: " << response.buffer.raw()
                      << ", waiting time: " << current_time << "ms"
                      << std::endl;
          }

          server_ready = response.status == Response::Status_code::OK;
        } catch (const std::exception &e) {
          std::cerr << "HTTPS server not ready after " << current_time << "ms";

          if (debug) {
            std::cerr << ": " << e.what() << std::endl;
            std::cerr << "Output so far:" << std::endl;
            std::cerr << m_server->read_all();
          }

          std::cerr << std::endl;
        }
      }
    }

    return server_ready;
  }

  void stop_server() {
    if (getenv("TEST_DEBUG") != nullptr) {
      std::cerr << m_server->read_all() << std::endl;
    }

    m_server->kill();
    m_server.reset();
  }

  const std::string &get_address() const { return m_address; }
  int get_port() const { return m_port; }

  bool is_alive() { return m_server && !m_server->check(); }

 private:
  std::unique_ptr<shcore::Process_launcher> m_server;
  std::string m_address;
  int m_port;

  bool find_free_port(int *port, bool use_env_var) {
    bool port_found = false;

    if (use_env_var && getenv("MYSQLSH_TEST_HTTP_PORT")) {
      *port = std::stod(getenv("MYSQLSH_TEST_HTTP_PORT"));
      port_found = true;
    } else {
      for (int i = 0; !port_found && i < 100; ++i) {
        if (!utils::Net::is_port_listening("127.0.0.1", *port + i)) {
          *port += i;
          port_found = true;
        }
      }
    }
    return port_found;
  }
};

class Rest_service_test : public ::testing::Test {
 public:
  Rest_service_test() : m_service(s_test_server->get_address(), false) {}

 protected:
  static void SetUpTestCase() {
    setup_env_vars();

    s_test_server = std::make_unique<Test_server>();

    if (s_test_server->start_server(8080, true)) {
      std::cerr << "HTTPS server is running: " << s_test_server->get_address()
                << std::endl;
    } else {
      // server is not running, find out what went wrong
      std::cerr << "HTTPS server failed to start:\n" << std::endl;
      // kill the process (if it's there), all test cases will fail with
      // appropriate message
      TearDownTestCase();
    }
  }

  void SetUp() override {
    if (!s_test_server->is_alive()) {
      s_test_server->start_server(s_test_server->get_port(), true);
    }
  }

  static void TearDownTestCase() {
    if (s_test_server && s_test_server->is_alive()) {
      s_test_server->stop_server();
    }

    restore_env_vars();
  }

  static void setup_env_vars() {
    const auto no_proxy = ::getenv(s_no_proxy);

    if (no_proxy) {
      s_no_proxy_env = no_proxy;
    }

    std::unordered_set<std::string> unique_hosts;

    {
      auto hosts = shcore::str_split(s_no_proxy_env, ",", -1, true);
      std::move(hosts.begin(), hosts.end(),
                std::inserter(unique_hosts, unique_hosts.begin()));
    }

    unique_hosts.emplace("localhost");
    unique_hosts.emplace("127.0.0.1");
    unique_hosts.emplace("::1");

    shcore::setenv(s_no_proxy, shcore::str_join(unique_hosts, ","));
  }

  static void restore_env_vars() {
    if (s_no_proxy_env.empty()) {
      shcore::unsetenv(s_no_proxy);
    } else {
      shcore::setenv(s_no_proxy, s_no_proxy_env);
    }
  }

  static std::unique_ptr<Test_server> s_test_server;

  static constexpr auto s_no_proxy = "no_proxy";

  static std::string s_no_proxy_env;

  // static std::string s_test_server_address;

  Rest_service m_service;
};  // namespace test

std::unique_ptr<Test_server> Rest_service_test::s_test_server;
std::string Rest_service_test::s_no_proxy_env;
// std::string Rest_service_test::s_test_server_address;

#define FAIL_IF_NO_SERVER                                \
  if (!s_test_server->is_alive()) {                      \
    FAIL() << "The HTTPS test server is not available."; \
  }

TEST_F(Rest_service_test, cycle_methods) {
  FAIL_IF_NO_SERVER

  // execute all possible requests, make sure that transmitted data is updated

  {
    auto request = Request("/get", {{"one", "1"}});
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("GET", response.json().as_map()->get_string("method"));
    EXPECT_EQ(nullptr, response.json().as_map()->get_map("json"));
    EXPECT_EQ("1",
              response.json().as_map()->get_map("headers")->get_string("one"));
  }

  {
    auto request = Request("/head", {{"two", "2"}});
    auto response = m_service.head(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("", response.buffer.raw());
  }

  {
    auto request = Json_request("/post", shcore::Value::parse("{'id' : 10}"),
                                {{"three", "3"}});
    auto response = m_service.post(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("POST", response.json().as_map()->get_string("method"));
    EXPECT_EQ(10, response.json().as_map()->get_map("json")->get_int("id"));
    EXPECT_EQ(shcore::Value_type::Undefined,
              response.json().as_map()->get_map("headers")->get_type("one"));
    EXPECT_EQ(shcore::Value_type::Undefined,
              response.json().as_map()->get_map("headers")->get_type("two"));
    EXPECT_EQ(
        "3", response.json().as_map()->get_map("headers")->get_string("three"));
  }

  {
    auto request = Json_request("/put", shcore::Value::parse("{'id' : 20}"),
                                {{"four", "4"}});
    auto response = m_service.put(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("PUT", response.json().as_map()->get_string("method"));
    EXPECT_EQ(20, response.json().as_map()->get_map("json")->get_int("id"));
    EXPECT_EQ(shcore::Value_type::Undefined,
              response.json().as_map()->get_map("headers")->get_type("three"));
    EXPECT_EQ("4",
              response.json().as_map()->get_map("headers")->get_string("four"));
  }

  {
    auto request = Json_request("/patch", shcore::Value::parse("{'id' : 30}"),
                                {{"five", "5"}});
    auto response = m_service.patch(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("PATCH", response.json().as_map()->get_string("method"));
    EXPECT_EQ(30, response.json().as_map()->get_map("json")->get_int("id"));
    EXPECT_EQ(shcore::Value_type::Undefined,
              response.json().as_map()->get_map("headers")->get_type("four"));
    EXPECT_EQ("5",
              response.json().as_map()->get_map("headers")->get_string("five"));
  }

  {
    auto request = Json_request("/delete", shcore::Value::parse("{'id' : 40}"),
                                {{"six", "6"}});
    auto response = m_service.delete_(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("DELETE", response.json().as_map()->get_string("method"));
    EXPECT_EQ(40, response.json().as_map()->get_map("json")->get_int("id"));
    EXPECT_EQ(shcore::Value_type::Undefined,
              response.json().as_map()->get_map("headers")->get_type("five"));
    EXPECT_EQ("6",
              response.json().as_map()->get_map("headers")->get_string("six"));
  }

  {
    auto request = Request("/get", {{"seven", "7"}});
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("GET", response.json().as_map()->get_string("method"));
    EXPECT_EQ(nullptr, response.json().as_map()->get_map("json"));
    EXPECT_EQ(shcore::Value_type::Undefined,
              response.json().as_map()->get_map("headers")->get_type("six"));
    EXPECT_EQ(
        "7", response.json().as_map()->get_map("headers")->get_string("seven"));
  }
}

TEST_F(Rest_service_test, redirect) {
  FAIL_IF_NO_SERVER

  // we're allowing for 20 redirections, 21 should result in error
  EXPECT_THROW(
      {
        try {
          auto request = Request("/redirect/21");
          m_service.get(&request);
        } catch (const Connection_error &ex) {
          EXPECT_THAT(ex.what(),
                      ::testing::HasSubstr("Maximum (20) redirects followed"));
          throw;
        }
      },
      Connection_error);

  // 20 redirections is OK
  auto request = Request("/redirect/20");
  EXPECT_EQ(Response::Status_code::OK, m_service.get(&request).status);
}

TEST_F(Rest_service_test, user_agent) {
  FAIL_IF_NO_SERVER

  auto request = Request("/get");
  const auto response = m_service.get(&request);
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ(
      "mysqlsh/" MYSH_VERSION,
      response.json().as_map()->get_map("headers")->get_string("user-agent"));
}

TEST_F(Rest_service_test, basic_authentication) {
  FAIL_IF_NO_SERVER

  {
    m_service.set(Basic_authentication{"first", "one"});
    auto request = Request("/basic/first/one");
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("OK", response.json().as_map()->get_string("authentication"));
  }

  {
    m_service.set(Basic_authentication{"second", "two"});
    auto request = Request("/basic/second/two");
    auto response = m_service.head(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
  }

  {
    m_service.set(Basic_authentication{"third", "three"});
    auto request = Request("/basic/third/three");
    auto response = m_service.post(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("OK", response.json().as_map()->get_string("authentication"));
  }

  {
    m_service.set(Basic_authentication{"fourth", "four"});
    auto request = Request("/basic/fourth/four");
    auto response = m_service.put(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("OK", response.json().as_map()->get_string("authentication"));
  }

  {
    m_service.set(Basic_authentication{"fifth", "five"});
    auto request = Request("/basic/fifth/five");
    auto response = m_service.patch(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("OK", response.json().as_map()->get_string("authentication"));
  }

  {
    m_service.set(Basic_authentication{"sixth", "six"});
    auto request = Request("/basic/sixth/six");
    auto response = m_service.delete_(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("OK", response.json().as_map()->get_string("authentication"));

    // same login:pass, different method
    response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("OK", response.json().as_map()->get_string("authentication"));
  }
}

TEST_F(Rest_service_test, basic_authentication_failure) {
  FAIL_IF_NO_SERVER

  m_service.set(Basic_authentication{"first", "one"});

  {
    auto request = Request("/basic/first/two");
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::UNAUTHORIZED, response.status);
    EXPECT_EQ("NO", response.json().as_map()->get_string("authentication"));
  }

  {
    auto request = Request("/basic/second/one");
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::UNAUTHORIZED, response.status);
    EXPECT_EQ("NO", response.json().as_map()->get_string("authentication"));
  }

  {
    auto request = Request("/basic/second/two");
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::UNAUTHORIZED, response.status);
    EXPECT_EQ("NO", response.json().as_map()->get_string("authentication"));
  }
}

TEST_F(Rest_service_test, request_headers) {
  FAIL_IF_NO_SERVER

  {
    // request without any extra headers
    auto request = Request("/get");
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ(shcore::Value_type::Undefined,
              response.json().as_map()->get_map("headers")->get_type("one"));
  }

  {
    // request with extra header
    auto request = Request("/get", {{"one", "1"}});
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("1",
              response.json().as_map()->get_map("headers")->get_string("one"));
  }

  {
    // set the default headers, it should be used for all requests from now on
    m_service.set_default_headers({{"two", "2"}});

    auto request = Request("/first");
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("2",
              response.json().as_map()->get_map("headers")->get_string("two"));
  }

  {
    auto request = Request("/second");
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("2",
              response.json().as_map()->get_map("headers")->get_string("two"));
  }

  {
    // override the default header, new value should be used
    auto request = Request("/third", {{"two", "3"}});
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("3",
              response.json().as_map()->get_map("headers")->get_string("two"));
  }

  {
    // this one should use the default header
    auto request = Request("/fourth");
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("2",
              response.json().as_map()->get_map("headers")->get_string("two"));
  }
}

TEST_F(Rest_service_test, content_type) {
  FAIL_IF_NO_SERVER

  {
    // POST with no data means that CURL will set Content-Type to its default
    auto request = Json_request("/post", shcore::Value());
    auto response = m_service.post(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("application/x-www-form-urlencoded",
              response.json().as_map()->get_map("headers")->get_string(
                  "content-type"));
  }

  {
    // POST with some data will set the Content-Type to application/json
    auto request = Json_request("/post", shcore::Value::parse("{'id' : 30}"));
    auto response = m_service.post(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("application/json",
              response.json().as_map()->get_map("headers")->get_string(
                  "content-type"));
  }

  {
    // if Content-Type header is specified, it's going to be used instead
    auto request = Json_request("/post", shcore::Value::parse("{'id' : 30}"),
                                {{"Content-Type", "text/plain"}});
    auto response = m_service.post(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("text/plain",
              response.json().as_map()->get_map("headers")->get_string(
                  "content-type"));
  }
}

TEST_F(Rest_service_test, response_headers) {
  FAIL_IF_NO_SERVER

  {
    // server should reply with some headers by default
    auto request = Request("/get");
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_FALSE(response.headers["Content-Length"].empty());
  }

  {
    // check if we get the headers we requested
    auto request = Request("/headers?one=two");
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("two", response.headers["one"]);
  }

  {
    auto request = Request("/headers?one=two&three=four");
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("two", response.headers["one"]);
    EXPECT_EQ("four", response.headers["three"]);
  }

  {
    // BUG#31979374 it should be possible to fetch a header with an empty value
    auto request = Request("/headers?one=");
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    ASSERT_TRUE(response.headers.find("one") != response.headers.end());
    EXPECT_EQ("", response.headers["one"]);
  }
}

TEST_F(Rest_service_test, response_content_type) {
  FAIL_IF_NO_SERVER

  {
    // server replies with application/json by default
    auto request = Request("/get");
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("application/json", response.headers["Content-Type"]);
    EXPECT_EQ(shcore::Value_type::Map, response.json().type);
  }

  {
    // force server to return different Content-Type, body is going to be an
    // unparsed string
    auto request = Request("/headers?Content-Type=text/plain");
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("text/plain", response.headers["Content-Type"]);
  }

  {
    auto request =
        Request("/headers?Content-Type=application/json%3B%20charset=UTF-8");
    auto response = m_service.get(&request);
    EXPECT_EQ(Response::Status_code::OK, response.status);
    EXPECT_EQ("application/json; charset=UTF-8",
              response.headers["Content-Type"]);
    EXPECT_EQ(shcore::Value_type::Map, response.json().type);
  }
}

TEST_F(Rest_service_test, timeout) {
  FAIL_IF_NO_SERVER

  // reduce the timeout
  m_service.set_timeout(1999, 1, 2);

  auto request = Request("/timeout/2.1");

  EXPECT_THROW_LIKE(m_service.head(&request), Connection_error,
                    "Operation timed out after ");

  EXPECT_THROW_LIKE(m_service.delete_(&request), Connection_error,
                    "Operation timed out after ");

  // Tests for low transfer rate timeout in EL6 fail because the functionality
  // seems to be not working correctly in the system curl. This means we are
  // running without timeouts there which is acceptable.
  // We are disabling such tests when using CURL >= 7.19.7
  mysqlshdk::utils::Version curl_version(CURL_VERSION);
  mysqlshdk::utils::Version no_transfer_rate_timeout_curl_version("7.19.7");

  if (curl_version > no_transfer_rate_timeout_curl_version) {
    EXPECT_THROW_LIKE(
        m_service.get(&request), Connection_error,
        "Operation too slow. Less than 1 bytes/sec transferred the "
        "last 2 seconds");

    EXPECT_THROW_LIKE(
        m_service.put(&request), Connection_error,
        "Operation too slow. Less than 1 bytes/sec transferred the "
        "last 2 seconds");

    EXPECT_THROW_LIKE(
        m_service.post(&request), Connection_error,
        "Operation too slow. Less than 1 bytes/sec transferred the "
        "last 2 seconds");
  }

  // increase the timeout
  m_service.set_timeout(3000, 0, 0);

  // request should be completed without any issues
  EXPECT_EQ(Response::Status_code::OK, m_service.get(&request).status);

  EXPECT_EQ(Response::Status_code::OK, m_service.head(&request).status);
}

TEST_F(Rest_service_test, retry_strategy_generic_errors) {
  FAIL_IF_NO_SERVER

  // Once the rest service is initialized, we close the server
  Rest_service local_service(s_test_server->get_address(), false);

  s_test_server->stop_server();

  auto request = Request("/get");
  request.type = Type::GET;

  // With no retries a generic error would throw an exception
  EXPECT_THROW_MSG_CONTAINS(local_service.execute(&request), Connection_error,
                            "Connection refused|couldn't connect to host");

  // One second retry strategy
  Retry_strategy retry_strategy(1);
  request.retry_strategy = &retry_strategy;

  // Some stop criteria must be defined otherwise we have infinite retries
  EXPECT_THROW_MSG(
      local_service.execute(&request), std::logic_error,
      "A stop criteria must be defined to avoid infinite retries.");

  retry_strategy.set_max_attempts(2);

  // Even with retry logic the same error is generated at the end
  EXPECT_THROW_MSG_CONTAINS(local_service.execute(&request), Connection_error,
                            "Connection refused|couldn't connect to host");
  EXPECT_EQ(2, retry_strategy.get_retry_count());
}

TEST_F(Rest_service_test, retry_strategy_server_errors) {
  FAIL_IF_NO_SERVER

  auto request_500 = Request("/server_error/500");
  request_500.type = Type::GET;

  // No retry test
  auto code = m_service.execute(&request_500);
  EXPECT_EQ(Response::Status_code::INTERNAL_SERVER_ERROR, code);

  // One second retry strategy
  Retry_strategy retry_strategy(1);
  retry_strategy.set_max_attempts(2);

  request_500.retry_strategy = &retry_strategy;

  // Server error codes are not configured to be retriable, so no retry is done
  code = m_service.execute(&request_500);
  EXPECT_EQ(code, Response::Status_code::INTERNAL_SERVER_ERROR);
  EXPECT_EQ(0, retry_strategy.get_retry_count());

  // Configure specific error to be retriable
  retry_strategy.add_retriable_status(
      Response::Status_code::INTERNAL_SERVER_ERROR);
  code = m_service.execute(&request_500);
  EXPECT_EQ(code, Response::Status_code::INTERNAL_SERVER_ERROR);
  EXPECT_EQ(2, retry_strategy.get_retry_count());

  auto request_503 = Request("/server_error/503");
  request_503.type = Type::GET;
  request_503.retry_strategy = &retry_strategy;

  // Other server error would not be retriable
  code = m_service.execute(&request_503);
  EXPECT_EQ(code, Response::Status_code::SERVICE_UNAVAILABLE);
  EXPECT_EQ(0, retry_strategy.get_retry_count());

  // add handling of a status code with specific error message
  retry_strategy.add_retriable_status(
      Response::Status_code::INTERNAL_SERVER_ERROR, "Unexpected-error");

  // response is not used (so there is no place to store body), but this error
  // is retriable regardless of error message, because it is also registered
  // using just the status code
  code = m_service.execute(&request_500);
  EXPECT_EQ(code, Response::Status_code::INTERNAL_SERVER_ERROR);
  EXPECT_EQ(2, retry_strategy.get_retry_count());

  {
    // response is used, this error is retriable regardless of error message,
    // even if it does not match
    auto request = Request("/server_error/500/Some-error");
    request.type = Type::GET;
    request.retry_strategy = &retry_strategy;
    String_response response;

    code = m_service.execute(&request, &response);
    EXPECT_EQ(code, Response::Status_code::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(2, retry_strategy.get_retry_count());
  }

  {
    // response is used, error message matches
    auto request = Request("/server_error/500/Unexpected-error");
    request.type = Type::GET;
    request.retry_strategy = &retry_strategy;
    String_response response;

    code = m_service.execute(&request, &response);
    EXPECT_EQ(code, Response::Status_code::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(2, retry_strategy.get_retry_count());
  }

  // add handling of a status code with specific another error message
  retry_strategy.add_retriable_status(Response::Status_code::GATEWAY_TIMEOUT,
                                      "Serious-error");

  {
    // response is not used, not possible to get the error message -> no retires
    auto request = Request("/server_error/504");
    request.type = Type::GET;
    request.retry_strategy = &retry_strategy;

    code = m_service.execute(&request);
    EXPECT_EQ(code, Response::Status_code::GATEWAY_TIMEOUT);
    EXPECT_EQ(0, retry_strategy.get_retry_count());
  }

  {
    // response is used, error message does not match -> no retires
    auto request = Request("/server_error/504/Something-went-wrong");
    request.type = Type::GET;
    request.retry_strategy = &retry_strategy;
    String_response response;

    code = m_service.execute(&request, &response);
    EXPECT_EQ(code, Response::Status_code::GATEWAY_TIMEOUT);
    EXPECT_EQ(0, retry_strategy.get_retry_count());
  }

  {
    // response is used, error message matches -> retires
    auto request = Request("/server_error/504/Serious-error");
    request.type = Type::GET;
    request.retry_strategy = &retry_strategy;
    String_response response;

    code = m_service.execute(&request, &response);
    EXPECT_EQ(code, Response::Status_code::GATEWAY_TIMEOUT);
    EXPECT_EQ(2, retry_strategy.get_retry_count());
  }

  // All the server error are now retriable
  retry_strategy.set_retry_on_server_errors(true);
  code = m_service.execute(&request_503);
  EXPECT_EQ(code, Response::Status_code::SERVICE_UNAVAILABLE);
  EXPECT_EQ(2, retry_strategy.get_retry_count());
}

TEST_F(Rest_service_test, retry_strategy_max_ellapsed_time) {
  FAIL_IF_NO_SERVER

  // One second retry strategy
  Retry_strategy retry_strategy(1);
  retry_strategy.set_max_ellapsed_time(5);

  // All the server error are now retriable
  retry_strategy.set_retry_on_server_errors(true);

  auto request = Request("/server_error/500");
  request.type = Type::GET;
  request.retry_strategy = &retry_strategy;

  auto code = m_service.execute(&request);
  EXPECT_EQ(code, Response::Status_code::INTERNAL_SERVER_ERROR);

  {
    auto ellapsed_time = static_cast<unsigned long long int>(
        retry_strategy.get_ellapsed_time().count());
    auto next_sleep_time = static_cast<unsigned long long int>(
        retry_strategy.get_next_sleep_time().count());
    auto max_ellapsed_time = static_cast<unsigned long long int>(
        retry_strategy.get_max_ellapsed_time().count());
    SCOPED_TRACE(shcore::str_format("Ellapsed Time: %llu", ellapsed_time));
    SCOPED_TRACE(shcore::str_format("Next Sleep Time: %llu", next_sleep_time));
    SCOPED_TRACE(
        shcore::str_format("Max Ellapsed Time: %llu", max_ellapsed_time));

    EXPECT_TRUE(retry_strategy.get_ellapsed_time() +
                    retry_strategy.get_next_sleep_time() >=
                retry_strategy.get_max_ellapsed_time());
  }

  // Considering each attempt consumes at least 1 (sleep time) the 5th
  // attempt would not take place
  EXPECT_EQ(4, retry_strategy.get_retry_count());
}

TEST_F(Rest_service_test, retry_strategy_throttling) {
  FAIL_IF_NO_SERVER

  // 1 second as base wait time
  // 2 as exponential grow factor
  // 4 seconds as max wait time between calls
  Exponential_backoff_retry retry_strategy(1, 2, 4);

  // Time Table
  // MET: Max exponential time topped with the maximum allowed between retries
  // GW: Guaranteed Wait
  // GA: Guaranteed Accumulated
  // JW: Wait caused by Jitter
  // MAA: Max time in attempt
  // MTA: Max Total Accumulated (GA + Max JW)
  // -------------------------------------------|
  // Attempt | MET  | GW | GA | JW  | MAA | MTA |
  // --------|------|----|----|-----|-----|-----|
  // 1       | 2/2  | 1  | 1  | 0-1 | 2   | 2   |
  // 2       | 4/4  | 2  | 3  | 0-2 | 4   | 6   |
  // 3       | 8/4  | 2  | 5  | 0-2 | 4   | 10  |<-- Worse Case Scenario in 12s
  // 4       | 16/4 | 2  | 7  | 0-2 | 4   | 14  |    Picked max jitter/attempt
  // 5       | 32/4 | 2  | 9  | 0-2 | 4   | 18  |
  // 6       | 64/4 | 2  | 11 | 0-2 | 4   | 20  |<-- Best Case Scenario in 12s
  // 6       | 64/4 | 2  | 13 | 0-2 | 4   | 24  |    Picked 0 jitter/attemp
  // --------------------------------------------

  retry_strategy.set_equal_jitter_for_throttling(true);
  retry_strategy.set_max_ellapsed_time(12);

  auto request = Request("/server_error/429");
  request.type = Type::GET;
  request.retry_strategy = &retry_strategy;

  auto code = m_service.execute(&request);

  EXPECT_EQ(code, Response::Status_code::TOO_MANY_REQUESTS);

  // Worse case scenario determines minimum attempts in 12 secs
  EXPECT_TRUE(retry_strategy.get_retry_count() >= 3);

  // Best case scenario determines maximum attempts in 12 secs
  EXPECT_TRUE(retry_strategy.get_retry_count() <= 6);
}

}  // namespace test
}  // namespace rest
}  // namespace mysqlshdk
