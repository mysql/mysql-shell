/*
 * Copyright (c) 2019, 2021, Oracle and/or its affiliates.
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

#include <curl/curl.h>
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
      return std::string{"Continue"};
    case Status_code::SWITCHING_PROTOCOLS:
      return std::string{"Switching Protocols"};
    case Status_code::PROCESSING:
      return std::string{"Processing"};
    case Status_code::EARLY_HINTS:
      return std::string{"Early Hints"};
    case Status_code::OK:
      return std::string{"OK"};
    case Status_code::CREATED:
      return std::string{"Created"};
    case Status_code::ACCEPTED:
      return std::string{"Accepted"};
    case Status_code::NON_AUTHORITATIVE_INFORMATION:
      return std::string{"Non-Authoritative Information"};
    case Status_code::NO_CONTENT:
      return std::string{"No Content"};
    case Status_code::RESET_CONTENT:
      return std::string{"Reset Content"};
    case Status_code::PARTIAL_CONTENT:
      return std::string{"Partial Content"};
    case Status_code::MULTI_STATUS:
      return std::string{"Multi-Status"};
    case Status_code::ALREADY_REPORTED:
      return std::string{"Already Reported"};
    case Status_code::IM_USED:
      return std::string{"IM Used"};
    case Status_code::MULTIPLE_CHOICES:
      return std::string{"Multiple Choices"};
    case Status_code::MOVED_PERMANENTLY:
      return std::string{"Moved Permanently"};
    case Status_code::FOUND:
      return std::string{"Found"};
    case Status_code::SEE_OTHER:
      return std::string{"See Other"};
    case Status_code::NOT_MODIFIED:
      return std::string{"Not Modified"};
    case Status_code::USE_PROXY:
      return std::string{"Use Proxy"};
    case Status_code::SWITCH_PROXY:
      return std::string{"Switch Proxy"};
    case Status_code::TEMPORARY_REDIRECT:
      return std::string{"Temporary Redirect"};
    case Status_code::PERMANENT_REDIRECT:
      return std::string{"Permanent Redirect"};
    case Status_code::BAD_REQUEST:
      return std::string{"Bad Request"};
    case Status_code::UNAUTHORIZED:
      return std::string{"Unauthorized"};
    case Status_code::PAYMENT_REQUIRED:
      return std::string{"Payment Required"};
    case Status_code::FORBIDDEN:
      return std::string{"Forbidden"};
    case Status_code::NOT_FOUND:
      return std::string{"Not Found"};
    case Status_code::METHOD_NOT_ALLOWED:
      return std::string{"Method Not Allowed"};
    case Status_code::NOT_ACCEPTABLE:
      return std::string{"Not Acceptable"};
    case Status_code::PROXY_AUTHENTICATION_REQUIRED:
      return std::string{"Proxy Authentication Required"};
    case Status_code::REQUEST_TIMEOUT:
      return std::string{"Request Timeout"};
    case Status_code::CONFLICT:
      return std::string{"Conflict"};
    case Status_code::GONE:
      return std::string{"Gone"};
    case Status_code::LENGTH_REQUIRED:
      return std::string{"Length Required"};
    case Status_code::PRECONDITION_FAILED:
      return std::string{"Precondition Failed"};
    case Status_code::PAYLOAD_TOO_LARGE:
      return std::string{"Request Entity Too Large"};
    case Status_code::URI_TOO_LONG:
      return std::string{"Request-URI Too Long"};
    case Status_code::UNSUPPORTED_MEDIA_TYPE:
      return std::string{"Unsupported Media Type"};
    case Status_code::RANGE_NOT_SATISFIABLE:
      return std::string{"Requested Range Not Satisfiable"};
    case Status_code::EXPECTATION_FAILED:
      return std::string{"Expectation Failed"};
    case Status_code::IM_A_TEAPOT:
      return std::string{"I'm a teapot"};
    case Status_code::MISDIRECTED_REQUEST:
      return std::string{"Misdirected Request"};
    case Status_code::UNPROCESSABLE_ENTITY:
      return std::string{"Unprocessable Entity"};
    case Status_code::LOCKED:
      return std::string{"Locked"};
    case Status_code::FAILED_DEPENDENCY:
      return std::string{"Failed Dependency"};
    case Status_code::TOO_EARLY:
      return std::string{"Too Early"};
    case Status_code::UPGRADE_REQUIRED:
      return std::string{"Upgrade Required"};
    case Status_code::PRECONDITION_REQUIRED:
      return std::string{"Precondition Required"};
    case Status_code::TOO_MANY_REQUESTS:
      return std::string{"Too Many Requests"};
    case Status_code::REQUEST_HEADER_FIELDS_TOO_LARGE:
      return std::string{"Request Header Fields Too Large"};
    case Status_code::UNAVAILABLE_FOR_LEGAL_REASONS:
      return std::string{"Unavailable For Legal Reasons"};
    case Status_code::INTERNAL_SERVER_ERROR:
      return std::string{"Internal Server Error"};
    case Status_code::NOT_IMPLEMENTED:
      return std::string{"Not Implemented"};
    case Status_code::BAD_GATEWAY:
      return std::string{"Bad Gateway"};
    case Status_code::SERVICE_UNAVAILABLE:
      return std::string{"Service Unavailable"};
    case Status_code::GATEWAY_TIMEOUT:
      return std::string{"Gateway Timeout"};
    case Status_code::HTTP_VERSION_NOT_SUPPORTED:
      return std::string{"HTTP Version Not Supported"};
    case Status_code::VARIANT_ALSO_NEGOTIATES:
      return std::string{"Variant Also Negotiates"};
    case Status_code::INSUFFICIENT_STORAGE:
      return std::string{"Insufficient Storage"};
    case Status_code::LOOP_DETECTED:
      return std::string{"Loop Detected"};
    case Status_code::NOT_EXTENDED:
      return std::string{"Not Extended"};
    case Status_code::NETWORK_AUTHENTICATION_REQUIRED:
      return std::string{"Network Authentication Required"};
  }
  return std::string{"Unknown HTTP status code"};
}

bool Response::is_json(const Headers &hdrs) {
  const auto content_type = hdrs.find(k_content_type);
  return content_type != hdrs.end() &&
         shcore::str_ibeginswith(content_type->second, k_application_json);
}

shcore::Value Response::json() const {
  if (is_json(headers) && body && body->size()) {
    return shcore::Value::parse(body->data(), body->size());
  }

  throw std::runtime_error("Response " + std::string{k_content_type} +
                           " is not a " + std::string{k_application_json});
}

std::optional<Response_error> Response::get_error() const {
  if (status < Response::Status_code::OK ||
      status >= Response::Status_code::MULTIPLE_CHOICES) {
    if (Response::is_json(headers) && body && body->size()) {
      try {
        auto json = shcore::Value::parse(body->data(), body->size()).as_map();
        if (json->get_type("message") == shcore::Value_type::String) {
          return Response_error(status, json->get_string("message"));
        }
      } catch (const shcore::Exception &error) {
        // This handles the case where the content/type indicates it's JSON but
        // parsing failed. The default error message is used on this case.
      }
    }

    return Response_error(status);
  }

  return {};
}

void Response::throw_if_error() const {
  if (const auto error = get_error()) {
    throw error.value();
  }
}

std::string Response_error::format() const {
  return shcore::str_format("%s (%d)", what(), static_cast<int>(m_code));
}

size_t Static_char_ref_buffer::append_data(const char *data, size_t data_size) {
  if (m_content_size + data_size <= m_buffer_size) {
    std::copy(data, data + data_size, m_buffer + m_content_size);

    m_content_size += data_size;

    return data_size;
  } else {
    return static_cast<size_t>(0);
  }
}

}  // namespace rest
}  // namespace mysqlshdk
