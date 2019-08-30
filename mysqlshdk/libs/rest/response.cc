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

#include "mysqlshdk/libs/rest/response.h"

#include <string>
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace rest {

namespace {
static constexpr auto k_content_type = "Content-Type";
static constexpr auto k_application_json = "application/json";
}  // namespace

std::string Response::status_code(Status_code c) {
  switch (c) {
    case Status_code::CONTINUE:
      return std::string{"100 Continue"};
    case Status_code::SWITCHING_PROTOCOLS:
      return std::string{"101 Switching Protocols"};
    case Status_code::OK:
      return std::string{"200 OK"};
    case Status_code::CREATED:
      return std::string{"201 Created"};
    case Status_code::ACCEPTED:
      return std::string{"202 Accepted"};
    case Status_code::NON_AUTHORITATIVE_INFORMATION:
      return std::string{"203 Non-Authoritative Information"};
    case Status_code::NO_CONTENT:
      return std::string{"204 No Content"};
    case Status_code::RESET_CONTENT:
      return std::string{"205 Reset Content"};
    case Status_code::PARTIAL_CONTENT:
      return std::string{"206 Partial Content"};
    case Status_code::MULTIPLE_CHOICES:
      return std::string{"300 Multiple Choices"};
    case Status_code::MOVED_PERMANENTLY:
      return std::string{"301 Moved Permanently"};
    case Status_code::FOUND:
      return std::string{"302 Found"};
    case Status_code::SEE_OTHER:
      return std::string{"303 See Other"};
    case Status_code::NOT_MODIFIED:
      return std::string{"304 Not Modified"};
    case Status_code::USE_PROXY:
      return std::string{"305 Use Proxy"};
    case Status_code::SWITCH_PROXY:
      return std::string{"306 Switch Proxy"};
    case Status_code::TEMPORARY_REDIRECT:
      return std::string{"307 Temporary Redirect"};
    case Status_code::BAD_REQUEST:
      return std::string{"400 Bad Request"};
    case Status_code::UNAUTHORIZED:
      return std::string{"401 Unauthorized"};
    case Status_code::PAYMENT_REQUIRED:
      return std::string{"402 Payment Required"};
    case Status_code::FORBIDDEN:
      return std::string{"403 Forbidden"};
    case Status_code::NOT_FOUND:
      return std::string{"404 Not Found"};
    case Status_code::METHOD_NOT_ALLOWED:
      return std::string{"405 Method Not Allowed"};
    case Status_code::NOT_ACCEPTABLE:
      return std::string{"406 Not Acceptable"};
    case Status_code::PROXY_AUTHENTICATION_REQUIRED:
      return std::string{"407 Proxy Authentication Required"};
    case Status_code::REQUEST_TIMEOUT:
      return std::string{"408 Request Timeout"};
    case Status_code::CONFLICT:
      return std::string{"409 Conflict"};
    case Status_code::GONE:
      return std::string{"410 Gone"};
    case Status_code::LENGTH_REQUIRED:
      return std::string{"411 Length Required"};
    case Status_code::PRECONDITION_FAILED:
      return std::string{"412 Precondition Failed"};
    case Status_code::PAYLOAD_TOO_LARGE:
      return std::string{"413 Request Entity Too Large"};
    case Status_code::URI_TOO_LONG:
      return std::string{"414 Request-URI Too Long"};
    case Status_code::UNSUPPORTED_MEDIA_TYPE:
      return std::string{"415 Unsupported Media Type"};
    case Status_code::RANGE_NOT_SATISFIABLE:
      return std::string{"416 Requested Range Not Satisfiable"};
    case Status_code::EXPECTATION_FAILED:
      return std::string{"417 Expectation Failed"};
    case Status_code::UPGRADE_REQUIRED:
      return std::string{"426 Upgrade Required"};
    case Status_code::INTERNAL_SERVER_ERROR:
      return std::string{"500 Internal Server Error"};
    case Status_code::NOT_IMPLEMENTED:
      return std::string{"501 Not Implemented"};
    case Status_code::BAD_GATEWAY:
      return std::string{"502 Bad Gateway"};
    case Status_code::SERVICE_UNAVAILABLE:
      return std::string{"503 Service Unavailable"};
    case Status_code::GATEWAY_TIMEOUT:
      return std::string{"504 Gateway Timeout"};
    case Status_code::HTTP_VERSION_NOT_SUPPORTED:
      return std::string{"505 HTTP Version Not Supported"};
  }
  return std::string{std::to_string(static_cast<int>(c)) +
                     " Unknown HTTP status code"};
}

shcore::Value Response::json() const {
  const auto content_type = headers.find(k_content_type);
  if (content_type != headers.end() &&
      shcore::str_ibeginswith(content_type->second, k_application_json) &&
      !body.get_string().empty()) {
    return shcore::Value::parse(body.get_string());
  }
  throw std::runtime_error("Response " + std::string{k_content_type} +
                           " is not a " + std::string{k_application_json});
}

}  // namespace rest
}  // namespace mysqlshdk
