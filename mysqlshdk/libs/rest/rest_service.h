/*
 * Copyright (c) 2019, 2021 Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_REST_REST_SERVICE_H_
#define MYSQLSHDK_LIBS_REST_REST_SERVICE_H_

#include <future>
#include <memory>
#include <string>

#include "mysqlshdk/include/scripting/types.h"

#include "mysqlshdk/libs/utils/masked_value.h"

#include "mysqlshdk/libs/rest/authentication.h"
#include "mysqlshdk/libs/rest/error.h"
#include "mysqlshdk/libs/rest/headers.h"
#include "mysqlshdk/libs/rest/response.h"

namespace mysqlshdk {
namespace rest {

enum class Type { GET, HEAD, POST, PUT, PATCH, DELETE };

std::string type_name(Type t);

class Retry_strategy;
/**
 * A REST service. By default, requests will follow redirections and
 * keep the connections alive.
 *
 * This is a move-only type.
 */
class Rest_service {
 public:
  /**
   * Constructs an object which is going to handle requests to the REST service
   * located at the specified base URL.
   *
   * @param base_url Base URL of the REST service.
   * @param verify_ssl If false, disables the SSL verification of target host.
   *        Validity of SSL certificates is not going to be checked.
   * @param service_label Label to be used when logging requests for this
   *        service.
   */
  explicit Rest_service(const std::string &base_url, bool verify_ssl = true,
                        const std::string &service_label = "");

  Rest_service(const Rest_service &) = delete;
  Rest_service(Rest_service &&);

  Rest_service &operator=(const Rest_service &) = delete;
  Rest_service &operator=(Rest_service &&);

  ~Rest_service();

  /**
   * Sets the Basic authentication to be used when executing the requests.
   *
   * @param basic Authentication details.
   *
   * @returns Reference to self.
   */
  Rest_service &set(const Basic_authentication &basic);

  /**
   * Sets the HTTP headers to be used when executing each request.
   *
   * @param headers HTTP headers.
   *
   * @returns Reference to self.
   */
  Rest_service &set_default_headers(const Headers &headers);

  /**
   * Sets the maximum time in milliseconds the execution of a request can take.
   * By default, timeout is set to two seconds. Use 0 for an infinite timeout.
   *
   * @param timeout Maximum allowed time for the execution of HEAD/DELETE
   * requests.
   * @param low_speed_limit Lowest transfer rate in bytes/second.
   * @param low_speed_time Number of seconds to meter the low_speed_limit before
   * timing out.
   *
   * @returns Reference to self.
   *
   * A timeout will occurs in two situations:
   * - A HEAD or DELETE request took more than timeout milliseconds to complete.
   * - The transfer/rate has been lower than low_speed_limit for more than
   *   low_speed_time.
   */
  Rest_service &set_timeout(long timeout, long low_speed_limit,
                            long low_speed_time);

  /**
   * Executes a GET request, blocks until response is available.
   *
   * @param path Path to the request, it is going to be appended to the base
   *        URL.
   * @param headers Optional request-specific headers. If default headers were
   *        also specified, request-specific headers are going to be appended
   *        that set, overwriting any duplicated values.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems.
   */
  Response get(const Masked_string &path, const Headers &headers = {},
               Retry_strategy *retry_strategy = nullptr);

  /**
   * Executes a HEAD request, blocks until response is available.
   *
   * @param path Path to the request, it is going to be appended to the base
   *        URL.
   * @param headers Optional request-specific headers. If default headers were
   *        also specified, request-specific headers are going to be appended
   *        that set, overwriting any duplicated values.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems.
   */
  Response head(const Masked_string &path, const Headers &headers = {},
                Retry_strategy *retry_strategy = nullptr);

  /**
   * Executes a POST request, blocks until response is available.
   *
   * @param path Path to the request, it is going to be appended to the base
   *        URL.
   * @param body Optional body which is going to be sent along with the request.
   *        Content-Type is going to be set to 'application/json', unless it is
   *        explicitly set in headers.
   * @param headers Optional request-specific headers. If default headers were
   *        also specified, request-specific headers are going to be appended
   *        that set, overwriting any duplicated values.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems.
   */
  Response post(const Masked_string &path, const shcore::Value &body = {},
                const Headers &headers = {},
                Retry_strategy *retry_strategy = nullptr);

  /**
   * Executes a PUT request, blocks until response is available.
   *
   * @param path Path to the request, it is going to be appended to the base
   *        URL.
   * @param body Optional body which is going to be sent along with the
   * request. Content-Type is going to be set to 'application/json', unless it
   * is explicitly set in headers.
   * @param headers Optional request-specific headers. If default headers were
   *        also specified, request-specific headers are going to be appended
   *        that set, overwriting any duplicated values.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems.
   */
  Response put(const Masked_string &path, const shcore::Value &body = {},
               const Headers &headers = {},
               Retry_strategy *retry_strategy = nullptr);

  /**
   * Executes a PATCH request, blocks until response is available.
   *
   * @param path Path to the request, it is going to be appended to the base
   *        URL.
   * @param body Optional body which is going to be sent along with the
   * request. Content-Type is going to be set to 'application/json', unless it
   * is explicitly set in headers.
   * @param headers Optional request-specific headers. If default headers were
   *        also specified, request-specific headers are going to be appended
   *        that set, overwriting any duplicated values.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems.
   */
  Response patch(const Masked_string &path, const shcore::Value &body = {},
                 const Headers &headers = {},
                 Retry_strategy *retry_strategy = nullptr);

  /**
   * Executes a DELETE request, blocks until response is available.
   *
   * @param path Path to the request, it is going to be appended to the base
   *        URL.
   * @param body Optional body which is going to be sent along with the
   * request. Content-Type is going to be set to 'application/json', unless it
   * is explicitly set in headers.
   * @param headers Optional request-specific headers. If default headers were
   *        also specified, request-specific headers are going to be appended
   *        that set, overwriting any duplicated values.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems.
   */
  Response delete_(const Masked_string &path, const shcore::Value &body = {},
                   const Headers &headers = {},
                   Retry_strategy *retry_strategy = nullptr);

  /**
   * Executes a request, blocks until response is available.
   *
   * @param type Method to be used on the request execution.
   * @param path Path to the request, it is going to be appended to the base
   *        URL.
   * @param body Optional body which is going to be sent along with the
   * request. Content-Type is going to be set to 'application/json', unless it
   * is explicitly set in headers.
   * @param size The length in bytes of the body to be sent.
   * @param headers Optional request-specific headers. If default headers were
   *        also specified, request-specific headers are going to be appended
   *        that set, overwriting any duplicated values.
   * @param response_data pointer to a string buffer where the content of the
   * response body will be written.
   * @param response_headers pointer to a Headers struct where the response
   * headers will be placed.
   *
   * @returns The code of the request response.
   *
   * @throw Connection_error In case of any connection-related problems.
   */
  Response::Status_code execute(Type type, const Masked_string &path,
                                const char *body = nullptr, size_t size = 0,
                                const Headers &request_headers = {},
                                Base_response_buffer *buffer = nullptr,
                                Headers *response_headers = nullptr,
                                Retry_strategy *retry_strategy = nullptr);

  /**
   * Asynchronously executes a GET request. This object must not be modified
   * nor any other request can be executed again until response is received.
   *
   * @param path Path to the request, it is going to be appended to the base
   *        URL.
   * @param headers Optional request-specific headers. If default headers were
   *        also specified, request-specific headers are going to be appended
   *        that set, overwriting any duplicated values.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems. This
   *         method does not throw on its own, exception could be thrown from
   *         future object.
   */
  std::future<Response> async_get(const Masked_string &path,
                                  const Headers &headers = {});

  /**
   * Asynchronously executes a HEAD request. This object must not be modified
   * nor any other request can be executed again until response is received.
   *
   * @param path Path to the request, it is going to be appended to the base
   *        URL.
   * @param headers Optional request-specific headers. If default headers were
   *        also specified, request-specific headers are going to be appended
   *        that set, overwriting any duplicated values.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems. This
   *         method does not throw on its own, exception could be thrown from
   *         future object.
   */
  std::future<Response> async_head(const Masked_string &path,
                                   const Headers &headers = {});

  /**
   * Asynchronously executes a POST request. This object must not be modified
   * nor any other request can be executed again until response is received.
   *
   * @param path Path to the request, it is going to be appended to the base
   *        URL.
   * @param body Optional body which is going to be sent along with the
   * request. Content-Type is going to be set to 'application/json', unless it
   * is explicitly set in headers.
   * @param headers Optional request-specific headers. If default headers were
   *        also specified, request-specific headers are going to be appended
   *        that set, overwriting any duplicated values.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems. This
   *         method does not throw on its own, exception could be thrown from
   *         future object.
   */
  std::future<Response> async_post(const Masked_string &path,
                                   const shcore::Value &body = {},
                                   const Headers &headers = {});

  /**
   * Asynchronously executes a PUT request. This object must not be modified
   * nor any other request can be executed again until response is received.
   *
   * @param path Path to the request, it is going to be appended to the base
   *        URL.
   * @param body Optional body which is going to be sent along with the
   * request. Content-Type is going to be set to 'application/json', unless it
   * is explicitly set in headers.
   * @param headers Optional request-specific headers. If default headers were
   *        also specified, request-specific headers are going to be appended
   *        that set, overwriting any duplicated values.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems. This
   *         method does not throw on its own, exception could be thrown from
   *         future object.
   */
  std::future<Response> async_put(const Masked_string &path,
                                  const shcore::Value &body = {},
                                  const Headers &headers = {});

  /**
   * Asynchronously executes a PATCH request. This object must not be modified
   * nor any other request can be executed again until response is received.
   *
   * @param path Path to the request, it is going to be appended to the base
   *        URL.
   * @param body Optional body which is going to be sent along with the
   * request. Content-Type is going to be set to 'application/json', unless it
   * is explicitly set in headers.
   * @param headers Optional request-specific headers. If default headers were
   *        also specified, request-specific headers are going to be appended
   *        that set, overwriting any duplicated values.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems. This
   *         method does not throw on its own, exception could be thrown from
   *         future object.
   */
  std::future<Response> async_patch(const Masked_string &path,
                                    const shcore::Value &body = {},
                                    const Headers &headers = {});

  /**
   * Asynchronously executes a DELETE request. This object must not be
   * modified nor any other request can be executed again until response is
   * received.
   *
   * @param path Path to the request, it is going to be appended to the base
   *        URL.
   * @param body Optional body which is going to be sent along with the
   * request. Content-Type is going to be set to 'application/json', unless it
   * is explicitly set in headers.
   * @param headers Optional request-specific headers. If default headers were
   *        also specified, request-specific headers are going to be appended
   *        that set, overwriting any duplicated values.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems. This
   *         method does not throw on its own, exception could be thrown from
   *         future object.
   */
  std::future<Response> async_delete(const Masked_string &path,
                                     const shcore::Value &body = {},
                                     const Headers &headers = {});

 private:
  Response execute_internal(Type type, const Masked_string &path,
                            const shcore::Value &body = shcore::Value(),
                            const Headers &request_headers = {},
                            Retry_strategy *retry_strategy = nullptr);

  class Impl;
  std::unique_ptr<Impl> m_impl;
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_REST_SERVICE_H_
