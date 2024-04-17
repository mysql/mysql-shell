/*
 * Copyright (c) 2019, 2024 Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_REST_REST_SERVICE_H_
#define MYSQLSHDK_LIBS_REST_REST_SERVICE_H_

#include <future>
#include <memory>
#include <string>

#include "mysqlshdk/libs/utils/masked_value.h"

#include "mysqlshdk/libs/rest/authentication.h"
#include "mysqlshdk/libs/rest/error.h"
#include "mysqlshdk/libs/rest/headers.h"
#include "mysqlshdk/libs/rest/request.h"
#include "mysqlshdk/libs/rest/response.h"

namespace mysqlshdk {
namespace rest {

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
  explicit Rest_service(const Masked_string &base_url, bool verify_ssl = true,
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
   * Sets the maximum time in seconds the execution of a request can take.
   * By default, timeout is set to 30 seconds. Use 0 for an infinite timeout.
   *
   * @param timeout Maximum allowed time in seconds for the execution of
   * HEAD/DELETE requests.
   * @param low_speed_limit Lowest transfer rate in bytes/second.
   * @param low_speed_time Number of seconds to meter the low_speed_limit before
   * timing out.
   *
   * @returns Reference to self.
   *
   * A timeout will occurs in two situations:
   * - A HEAD or DELETE request took more than timeout seconds to complete.
   * - The transfer/rate has been lower than low_speed_limit for more than
   *   low_speed_time.
   */
  Rest_service &set_timeout(long timeout, long low_speed_limit,
                            long low_speed_time);

  /**
   * Sets the maximum time in seconds the connection phase to the server can
   * take. By default, timeout is set to 300 seconds. Use 0 for the default
   * timeout.
   *
   * @param timeout Maximum allowed time in seconds for the connection phase
   *
   * @returns Reference to self.
   */
  Rest_service &set_connect_timeout(long timeout);

  /**
   * Executes a GET request, blocks until response is available.
   *
   * @param request Request to be sent.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems.
   */
  String_response get(Request *request);

  /**
   * Executes a HEAD request, blocks until response is available.
   *
   * @param request Request to be sent.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems.
   */
  String_response head(Request *request);

  /**
   * Executes a POST request, blocks until response is available.
   *
   * @param request Request to be sent.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems.
   */
  String_response post(Request *request);

  /**
   * Executes a PUT request, blocks until response is available.
   *
   * @param request Request to be sent.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems.
   */
  String_response put(Request *request);

  /**
   * Executes a PATCH request, blocks until response is available.
   *
   * @param request Request to be sent.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems.
   */
  String_response patch(Request *request);

  /**
   * Executes a DELETE request, blocks until response is available.
   *
   * @param request Request to be sent.
   *
   * @returns Received response.
   *
   * @throws Connection_error In case of any connection-related problems.
   */
  String_response delete_(Request *request);

  /**
   * Executes a request, blocks until response is available.
   *
   * @param request Request to be sent.
   * @param response Response received.
   *
   * @returns The code of the request response.
   *
   * @throw Connection_error In case of any connection-related problems.
   */
  Response::Status_code execute(Request *request, Response *response = nullptr);

 private:
  String_response execute_internal(Request *request);

  class Impl;
  std::unique_ptr<Impl> m_impl;
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_REST_SERVICE_H_
