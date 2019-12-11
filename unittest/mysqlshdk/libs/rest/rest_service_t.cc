/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <memory>
#include <string>
#include <vector>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_path.h"

#include "mysqlshdk/libs/rest/rest_service.h"

extern "C" const char *g_test_home;

namespace mysqlshdk {
namespace rest {
namespace test {

class Rest_service_test : public ::testing::Test {
 public:
  Rest_service_test() : m_service(s_test_server_address, false) {}

 protected:
  static void SetUpTestCase() {
    int port = 8080;
    bool port_found = false;

    if (getenv("MYSQLSH_TEST_HTTP_PORT")) {
      port = std::stod(getenv("MYSQLSH_TEST_HTTP_PORT"));
      port_found = true;
    } else {
      for (int i = 0; !port_found && i < 100; ++i) {
        if (!utils::Net::is_port_listening("127.0.0.1", port + i)) {
          port += i;
          port_found = true;
        }
      }
    }

    if (port_found) {
      const auto script = shcore::path::join_path(g_test_home, "data", "rest",
                                                  "test-server.py");
      const auto port_number = std::to_string(port);
      std::vector<const char *> args{"python", script.c_str(),
                                     port_number.c_str(), nullptr};

      s_test_server = std::make_unique<shcore::Process_launcher>(&args[0]);
      s_test_server->enable_reader_thread();
#ifdef _WIN32
      s_test_server->set_create_process_group();
#endif  // _WIN32
      s_test_server->start();

      s_test_server_address = "https://127.0.0.1:" + port_number;

      static constexpr uint32_t sleep_time = 100;   // 100ms
      static constexpr uint32_t wait_time = 10000;  // 10s
      uint32_t current_time = 0;
      bool server_ready = false;
      Rest_service rest{s_test_server_address, false};
      const auto debug = getenv("TEST_DEBUG") != nullptr;

      while (!server_ready && current_time < wait_time) {
        shcore::sleep_ms(sleep_time);
        current_time += sleep_time;

        if (s_test_server->check()) {
          // process is not running, exit immediately
          break;
        }

        try {
          const auto response = rest.head("/ping");

          if (debug) {
            std::cerr << "HTTPS server replied with: "
                      << Response::status_code(response.status)
                      << ", waiting time: " << current_time << "ms"
                      << std::endl;
          }

          server_ready = response.status == Response::Status_code::OK;
        } catch (const std::exception &e) {
          std::cerr << "HTTPS server not ready after " << current_time << "ms";

          if (debug) {
            std::cerr << ": " << e.what() << std::endl;
            std::cerr << "Output so far:" << std::endl;
            std::cerr << s_test_server->read_all();
          }

          std::cerr << std::endl;
        }
      }

      if (!server_ready) {
        // server is not running, find out what went wrong
        std::cerr << "HTTPS server failed to start:\n"
                  << s_test_server->read_all() << std::endl;
        // kill the process (if it's there), all test cases will fail with
        // appropriate message
        TearDownTestCase();
      } else {
        std::cerr << "HTTPS server is running: " << s_test_server_address
                  << std::endl;
      }
    } else {
      std::cerr << "Could not find an available port for HTTPS server"
                << std::endl;
    }
  }

  static void TearDownTestCase() {
    if (s_test_server) {
      if (getenv("TEST_DEBUG") != nullptr) {
        std::cerr << s_test_server->read_all() << std::endl;
      }

      s_test_server->kill();
      s_test_server.reset();
    }
  }

  static bool has_server() { return s_test_server && !s_test_server->check(); }

  static std::unique_ptr<shcore::Process_launcher> s_test_server;

  static std::string s_test_server_address;

  Rest_service m_service;
};

std::unique_ptr<shcore::Process_launcher> Rest_service_test::s_test_server;
std::string Rest_service_test::s_test_server_address;

#define FAIL_IF_NO_SERVER                                \
  if (!has_server()) {                                   \
    FAIL() << "The HTTPS test server is not available."; \
  }

TEST_F(Rest_service_test, cycle_methods) {
  FAIL_IF_NO_SERVER

  // execute all possible requests, make sure that transmitted data is updated

  auto response = m_service.get("/get", {{"one", "1"}});
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("GET", response.json().as_map()->get_string("method"));
  EXPECT_EQ(nullptr, response.json().as_map()->get_map("json"));
  EXPECT_EQ("1",
            response.json().as_map()->get_map("headers")->get_string("one"));

  response = m_service.head("/head", {{"two", "2"}});
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("", response.body.as_string());

  response = m_service.post("/post", shcore::Value::parse("{'id' : 10}"),
                            {{"three", "3"}});
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("POST", response.json().as_map()->get_string("method"));
  EXPECT_EQ(10, response.json().as_map()->get_map("json")->get_int("id"));
  EXPECT_EQ(shcore::Value_type::Undefined,
            response.json().as_map()->get_map("headers")->get_type("one"));
  EXPECT_EQ(shcore::Value_type::Undefined,
            response.json().as_map()->get_map("headers")->get_type("two"));
  EXPECT_EQ("3",
            response.json().as_map()->get_map("headers")->get_string("three"));

  response = m_service.put("/put", shcore::Value::parse("{'id' : 20}"),
                           {{"four", "4"}});
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("PUT", response.json().as_map()->get_string("method"));
  EXPECT_EQ(20, response.json().as_map()->get_map("json")->get_int("id"));
  EXPECT_EQ(shcore::Value_type::Undefined,
            response.json().as_map()->get_map("headers")->get_type("three"));
  EXPECT_EQ("4",
            response.json().as_map()->get_map("headers")->get_string("four"));

  response = m_service.patch("/patch", shcore::Value::parse("{'id' : 30}"),
                             {{"five", "5"}});
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("PATCH", response.json().as_map()->get_string("method"));
  EXPECT_EQ(30, response.json().as_map()->get_map("json")->get_int("id"));
  EXPECT_EQ(shcore::Value_type::Undefined,
            response.json().as_map()->get_map("headers")->get_type("four"));
  EXPECT_EQ("5",
            response.json().as_map()->get_map("headers")->get_string("five"));

  response = m_service.delete_("/delete", shcore::Value::parse("{'id' : 40}"),
                               {{"six", "6"}});
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("DELETE", response.json().as_map()->get_string("method"));
  EXPECT_EQ(40, response.json().as_map()->get_map("json")->get_int("id"));
  EXPECT_EQ(shcore::Value_type::Undefined,
            response.json().as_map()->get_map("headers")->get_type("five"));
  EXPECT_EQ("6",
            response.json().as_map()->get_map("headers")->get_string("six"));

  response = m_service.get("/get", {{"seven", "7"}});
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("GET", response.json().as_map()->get_string("method"));
  EXPECT_EQ(nullptr, response.json().as_map()->get_map("json"));
  EXPECT_EQ(shcore::Value_type::Undefined,
            response.json().as_map()->get_map("headers")->get_type("six"));
  EXPECT_EQ("7",
            response.json().as_map()->get_map("headers")->get_string("seven"));
}

TEST_F(Rest_service_test, cycle_async_methods) {
  FAIL_IF_NO_SERVER

  // execute all possible requests, make sure that transmitted data is updated

  auto response = m_service.async_get("/get", {{"one", "1"}}).get();
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("GET", response.json().as_map()->get_string("method"));
  EXPECT_EQ(nullptr, response.json().as_map()->get_map("json"));
  EXPECT_EQ("1",
            response.json().as_map()->get_map("headers")->get_string("one"));

  response = m_service.async_head("/head", {{"two", "2"}}).get();
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("", response.body.as_string());

  response = m_service
                 .async_post("/post", shcore::Value::parse("{'id' : 10}"),
                             {{"three", "3"}})
                 .get();
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("POST", response.json().as_map()->get_string("method"));
  EXPECT_EQ(10, response.json().as_map()->get_map("json")->get_int("id"));
  EXPECT_EQ(shcore::Value_type::Undefined,
            response.json().as_map()->get_map("headers")->get_type("one"));
  EXPECT_EQ(shcore::Value_type::Undefined,
            response.json().as_map()->get_map("headers")->get_type("two"));
  EXPECT_EQ("3",
            response.json().as_map()->get_map("headers")->get_string("three"));

  response = m_service
                 .async_put("/put", shcore::Value::parse("{'id' : 20}"),
                            {{"four", "4"}})
                 .get();
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("PUT", response.json().as_map()->get_string("method"));
  EXPECT_EQ(20, response.json().as_map()->get_map("json")->get_int("id"));
  EXPECT_EQ(shcore::Value_type::Undefined,
            response.json().as_map()->get_map("headers")->get_type("three"));
  EXPECT_EQ("4",
            response.json().as_map()->get_map("headers")->get_string("four"));

  response = m_service
                 .async_patch("/patch", shcore::Value::parse("{'id' : 30}"),
                              {{"five", "5"}})
                 .get();
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("PATCH", response.json().as_map()->get_string("method"));
  EXPECT_EQ(30, response.json().as_map()->get_map("json")->get_int("id"));
  EXPECT_EQ(shcore::Value_type::Undefined,
            response.json().as_map()->get_map("headers")->get_type("four"));
  EXPECT_EQ("5",
            response.json().as_map()->get_map("headers")->get_string("five"));

  response = m_service
                 .async_delete("/delete", shcore::Value::parse("{'id' : 40}"),
                               {{"six", "6"}})
                 .get();
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("DELETE", response.json().as_map()->get_string("method"));
  EXPECT_EQ(40, response.json().as_map()->get_map("json")->get_int("id"));
  EXPECT_EQ(shcore::Value_type::Undefined,
            response.json().as_map()->get_map("headers")->get_type("five"));
  EXPECT_EQ("6",
            response.json().as_map()->get_map("headers")->get_string("six"));

  response = m_service.async_get("/get", {{"seven", "7"}}).get();
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("GET", response.json().as_map()->get_string("method"));
  EXPECT_EQ(nullptr, response.json().as_map()->get_map("json"));
  EXPECT_EQ(shcore::Value_type::Undefined,
            response.json().as_map()->get_map("headers")->get_type("six"));
  EXPECT_EQ("7",
            response.json().as_map()->get_map("headers")->get_string("seven"));
}

TEST_F(Rest_service_test, redirect) {
  FAIL_IF_NO_SERVER

  // we're allowing for 20 redirections, 21 should result in error
  EXPECT_THROW(
      {
        try {
          m_service.get("/redirect/21");
        } catch (const Connection_error &ex) {
          EXPECT_THAT(ex.what(),
                      ::testing::HasSubstr("Maximum (20) redirects followed"));
          throw;
        }
      },
      Connection_error);

  // 20 redirections is OK
  EXPECT_EQ(Response::Status_code::OK, m_service.get("/redirect/20").status);
}

TEST_F(Rest_service_test, user_agent) {
  FAIL_IF_NO_SERVER

  const auto response = m_service.get("/get");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ(
      "mysqlsh/" MYSH_VERSION,
      response.json().as_map()->get_map("headers")->get_string("user-agent"));
}

TEST_F(Rest_service_test, basic_authentication) {
  FAIL_IF_NO_SERVER

  m_service.set(Basic_authentication{"first", "one"});
  auto response = m_service.get("/basic/first/one");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("OK", response.json().as_map()->get_string("authentication"));

  m_service.set(Basic_authentication{"second", "two"});
  response = m_service.head("/basic/second/two");
  EXPECT_EQ(Response::Status_code::OK, response.status);

  m_service.set(Basic_authentication{"third", "three"});
  response = m_service.post("/basic/third/three");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("OK", response.json().as_map()->get_string("authentication"));

  m_service.set(Basic_authentication{"fourth", "four"});
  response = m_service.put("/basic/fourth/four");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("OK", response.json().as_map()->get_string("authentication"));

  m_service.set(Basic_authentication{"fifth", "five"});
  response = m_service.patch("/basic/fifth/five");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("OK", response.json().as_map()->get_string("authentication"));

  m_service.set(Basic_authentication{"sixth", "six"});
  response = m_service.delete_("/basic/sixth/six");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("OK", response.json().as_map()->get_string("authentication"));

  // same login:pass, different method
  response = m_service.get("/basic/sixth/six");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("OK", response.json().as_map()->get_string("authentication"));
}

TEST_F(Rest_service_test, basic_authentication_failure) {
  FAIL_IF_NO_SERVER

  m_service.set(Basic_authentication{"first", "one"});

  auto response = m_service.get("/basic/first/two");
  EXPECT_EQ(Response::Status_code::UNAUTHORIZED, response.status);
  EXPECT_EQ("NO", response.json().as_map()->get_string("authentication"));

  response = m_service.get("/basic/second/one");
  EXPECT_EQ(Response::Status_code::UNAUTHORIZED, response.status);
  EXPECT_EQ("NO", response.json().as_map()->get_string("authentication"));

  response = m_service.get("/basic/second/two");
  EXPECT_EQ(Response::Status_code::UNAUTHORIZED, response.status);
  EXPECT_EQ("NO", response.json().as_map()->get_string("authentication"));
}

TEST_F(Rest_service_test, request_headers) {
  FAIL_IF_NO_SERVER

  // request without any extra headers
  auto response = m_service.get("/get");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ(shcore::Value_type::Undefined,
            response.json().as_map()->get_map("headers")->get_type("one"));

  // request with extra header
  response = m_service.get("/get", {{"one", "1"}});
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("1",
            response.json().as_map()->get_map("headers")->get_string("one"));

  // set the default headers, it should be used for all requests from now on
  m_service.set_default_headers({{"two", "2"}});

  response = m_service.get("/first");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("2",
            response.json().as_map()->get_map("headers")->get_string("two"));

  response = m_service.get("/second");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("2",
            response.json().as_map()->get_map("headers")->get_string("two"));

  // override the default header, new value should be used
  response = m_service.get("/third", {{"two", "3"}});
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("3",
            response.json().as_map()->get_map("headers")->get_string("two"));

  // this one should use the default header
  response = m_service.get("/fourth");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("2",
            response.json().as_map()->get_map("headers")->get_string("two"));
}

TEST_F(Rest_service_test, content_type) {
  FAIL_IF_NO_SERVER

  // POST with no data means that CURL will set Content-Type to its default
  auto response = m_service.post("/post");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ(
      "application/x-www-form-urlencoded",
      response.json().as_map()->get_map("headers")->get_string("content-type"));

  // POST with some data will set the Content-Type to application/json
  response = m_service.post("/post", shcore::Value::parse("{'id' : 30}"));
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ(
      "application/json",
      response.json().as_map()->get_map("headers")->get_string("content-type"));

  // if Content-Type header is specified, it's going to be used instead
  response = m_service.post("/post", shcore::Value::parse("{'id' : 30}"),
                            {{"Content-Type", "text/plain"}});
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ(
      "text/plain",
      response.json().as_map()->get_map("headers")->get_string("content-type"));
}

TEST_F(Rest_service_test, response_headers) {
  FAIL_IF_NO_SERVER

  // server should reply with some headers by default
  auto response = m_service.get("/get");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_FALSE(response.headers["Content-Length"].empty());

  // check if we get the headers we requested
  response = m_service.get("/headers?one=two");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("two", response.headers["one"]);

  response = m_service.get("/headers?one=two&three=four");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("two", response.headers["one"]);
  EXPECT_EQ("four", response.headers["three"]);
}

TEST_F(Rest_service_test, response_content_type) {
  FAIL_IF_NO_SERVER

  // server replies with application/json by default
  auto response = m_service.get("/get");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("application/json", response.headers["Content-Type"]);
  EXPECT_EQ(shcore::Value_type::Map, response.json().type);
  EXPECT_EQ(shcore::Value_type::String, response.body.type);

  // force server to return different Content-Type, body is going to be an
  // unparsed string
  response = m_service.get("/headers?Content-Type=text/plain");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("text/plain", response.headers["Content-Type"]);
  EXPECT_EQ(shcore::Value_type::String, response.body.type);

  response = m_service.get(
      "/headers?Content-Type=application/json%3B%20charset=UTF-8");
  EXPECT_EQ(Response::Status_code::OK, response.status);
  EXPECT_EQ("application/json; charset=UTF-8",
            response.headers["Content-Type"]);
  EXPECT_EQ(shcore::Value_type::Map, response.json().type);
  EXPECT_EQ(shcore::Value_type::String, response.body.type);
}

TEST_F(Rest_service_test, timeout) {
  FAIL_IF_NO_SERVER

  // by default, timeout is set to two seconds, perform request which takes 2.1s
  EXPECT_THROW(
      {
        try {
          m_service.get("/timeout/2.1");
        } catch (const Connection_error &ex) {
          EXPECT_THAT(ex.what(),
                      ::testing::HasSubstr("Operation timed out after "));
          throw;
        }
      },
      Connection_error);

  // increase the timeout
  m_service.set_timeout(3000);

  // request should be completed without any issues
  EXPECT_EQ(Response::Status_code::OK, m_service.get("/timeout/2.1").status);
}

}  // namespace test
}  // namespace rest
}  // namespace mysqlshdk
