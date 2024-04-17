/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

#include <iterator>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils/mocks/gmock_clean.h"
#include "unittest/test_utils/shell_test_env.h"
#include "unittest/test_utils/test_server.h"

#include "mysqlshdk/libs/rest/rest_service.h"
#include "mysqlshdk/libs/rest/retry_strategy.h"
#include "mysqlshdk/libs/utils/profiling.h"

namespace mysqlshdk {
namespace rest {
namespace test {

class Rest_service_test : public ::testing::Test {
 public:
  Rest_service_test() : m_service(s_test_server->address(), false) {}

 protected:
  static void SetUpTestCase() {
    s_test_server = std::make_unique<tests::Test_server>();

    if (!s_test_server->start_server(8080)) {
      // kill the process (if it's there), all test cases will fail with
      // appropriate message
      TearDownTestCase();
    }
  }

  void SetUp() override {
    if (!s_test_server->is_alive()) {
      s_test_server->start_server(s_test_server->port());
    }
  }

  static void TearDownTestCase() {
    if (s_test_server && s_test_server->is_alive()) {
      s_test_server->stop_server();
    }
  }

  static std::unique_ptr<tests::Test_server> s_test_server;

  Rest_service m_service;
};

std::unique_ptr<tests::Test_server> Rest_service_test::s_test_server;

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
    EXPECT_EQ(shcore::Value_type::Map, response.json().get_type());
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
    EXPECT_EQ(shcore::Value_type::Map, response.json().get_type());
  }
}

TEST_F(Rest_service_test, timeout) {
  FAIL_IF_NO_SERVER

  // reduce the timeout
  m_service.set_timeout(1, 1, 1);

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
        "last 1 seconds");

    EXPECT_THROW_LIKE(
        m_service.put(&request), Connection_error,
        "Operation too slow. Less than 1 bytes/sec transferred the "
        "last 1 seconds");

    EXPECT_THROW_LIKE(
        m_service.post(&request), Connection_error,
        "Operation too slow. Less than 1 bytes/sec transferred the "
        "last 1 seconds");
  }

  // increase the timeout
  m_service.set_timeout(3, 0, 0);

  // request should be completed without any issues
  EXPECT_EQ(Response::Status_code::OK, m_service.get(&request).status);

  EXPECT_EQ(Response::Status_code::OK, m_service.head(&request).status);
}

TEST_F(Rest_service_test, retry_strategy_server_errors) {
  FAIL_IF_NO_SERVER

  mysqlshdk::utils::Duration d;

  {
    // no retry strategy
    auto request = Request("/server_error/500/Unexpected-error");
    request.type = Type::GET;

    d.start();
    const auto code = m_service.execute(&request);
    d.finish();

    EXPECT_EQ(Response::Status_code::INTERNAL_SERVER_ERROR, code);
    EXPECT_LT(d.seconds_elapsed(), 1.0);
  }

  // One second retry strategy
  const auto retry_strategy =
      Retry_strategy_builder{1}.set_max_attempts(2).build();

  // add handling of a status code with specific error message
  retry_strategy->retry_on(Response::Status_code::INTERNAL_SERVER_ERROR,
                           "Unexpected-error");

  {
    // response is not used, not possible to get the error message -> no retries
    auto request = Request("/server_error/500/Unexpected-error");
    request.type = Type::GET;
    request.retry_strategy = retry_strategy.get();

    d.start();
    const auto code = m_service.execute(&request);
    d.finish();

    EXPECT_EQ(Response::Status_code::INTERNAL_SERVER_ERROR, code);
    EXPECT_LT(d.seconds_elapsed(), 1.0);
  }

  {
    // response is used, error message does not match -> no retries
    auto request =
        Request("/server_error/500/Something-went-wrong/Important-Code");
    request.type = Type::GET;
    request.retry_strategy = retry_strategy.get();

    String_response response;

    d.start();
    const auto code = m_service.execute(&request, &response);
    d.finish();

    EXPECT_EQ(Response::Status_code::INTERNAL_SERVER_ERROR, code);
    EXPECT_LT(d.seconds_elapsed(), 1.0);

    const auto error = response.get_error();
    ASSERT_TRUE(error.has_value());
    EXPECT_STREQ("Something-went-wrong", error->what());
    EXPECT_EQ("Important-Code", error->code());
  }

  {
    // response is used, error code does not match -> no retries
    auto request = Request("/server_error/502/Unexpected-error");
    request.type = Type::GET;
    request.retry_strategy = retry_strategy.get();

    String_response response;

    d.start();
    const auto code = m_service.execute(&request, &response);
    d.finish();

    EXPECT_EQ(Response::Status_code::BAD_GATEWAY, code);
    EXPECT_LT(d.seconds_elapsed(), 1.0);

    const auto error = response.get_error();
    ASSERT_TRUE(error.has_value());
    EXPECT_STREQ("Unexpected-error", error->what());
  }

  {
    // response is used, error message matches -> retries
    auto request = Request("/server_error/500/Unexpected-error/Important-Code");
    request.type = Type::GET;
    request.retry_strategy = retry_strategy.get();

    String_response response;

    d.start();
    const auto code = m_service.execute(&request, &response);
    d.finish();

    EXPECT_EQ(Response::Status_code::INTERNAL_SERVER_ERROR, code);
    EXPECT_GE(d.seconds_elapsed(), 2.0);  // two retries, one second each

    const auto error = response.get_error();
    ASSERT_TRUE(error.has_value());
    EXPECT_STREQ("Unexpected-error", error->what());
    EXPECT_EQ("Important-Code", error->code());
  }

  // All the server error are now retriable
  retry_strategy->retry_on_server_errors();

  {
    // error is not registered explicitly, but still retried
    auto request = Request("/server_error/503");
    request.type = Type::GET;
    request.retry_strategy = retry_strategy.get();

    d.start();
    const auto code = m_service.execute(&request);
    d.finish();

    EXPECT_EQ(Response::Status_code::SERVICE_UNAVAILABLE, code);
    EXPECT_GE(d.seconds_elapsed(), 2.0);  // two retries, one second each
  }
}

TEST_F(Rest_service_test, bug34765385) {
  // BUG#34765385: retry in case of CURL-related errors
  FAIL_IF_NO_SERVER

  // retry twice, waiting one second each time
  const auto retry_strategy =
      Retry_strategy_builder{1}.set_max_attempts(2).build();
  // CURLE_PARTIAL_FILE
  retry_strategy->retry_on(Error_code::PARTIAL_FILE);
  // if curl is using OpenSSL3, CURLE_RECV_ERROR error is reported instead
  retry_strategy->retry_on(Error_code::RECV_ERROR);

  auto request = Request("/partial_file/1000");
  request.type = Type::GET;
  request.retry_strategy = retry_strategy.get();

  mysqlshdk::utils::Duration d;
  d.start();
  // this throws, but first retries two times
  EXPECT_THROW(m_service.execute(&request), Connection_error);
  d.finish();

  EXPECT_GE(d.seconds_elapsed(), 2.0);  // two retries, one second each
}

}  // namespace test
}  // namespace rest
}  // namespace mysqlshdk
