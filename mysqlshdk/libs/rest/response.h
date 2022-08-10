/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_REST_RESPONSE_H_
#define MYSQLSHDK_LIBS_REST_RESPONSE_H_

#include "mysqlshdk/include/scripting/types.h"

#include <algorithm>
#include <optional>
#include <string>

#include "mysqlshdk/libs/rest/headers.h"

namespace mysqlshdk {
namespace rest {

class Response_error;
class Base_response_buffer;

/**
 * A REST response.
 */
struct Response {
 public:
  /**
   * HTTP status code, as specified in RFC7231.
   */
  enum class Status_code {
    CONTINUE = 100,
    SWITCHING_PROTOCOLS = 101,
    PROCESSING = 102,
    EARLY_HINTS = 103,
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NON_AUTHORITATIVE_INFORMATION = 203,
    NO_CONTENT = 204,
    RESET_CONTENT = 205,
    PARTIAL_CONTENT = 206,
    MULTI_STATUS = 207,
    ALREADY_REPORTED = 208,
    IM_USED = 226,
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    USE_PROXY = 305,
    SWITCH_PROXY = 306,
    TEMPORARY_REDIRECT = 307,
    PERMANENT_REDIRECT = 308,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    PAYMENT_REQUIRED = 402,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    NOT_ACCEPTABLE = 406,
    PROXY_AUTHENTICATION_REQUIRED = 407,
    REQUEST_TIMEOUT = 408,
    CONFLICT = 409,
    GONE = 410,
    LENGTH_REQUIRED = 411,
    PRECONDITION_FAILED = 412,
    PAYLOAD_TOO_LARGE = 413,
    URI_TOO_LONG = 414,
    UNSUPPORTED_MEDIA_TYPE = 415,
    RANGE_NOT_SATISFIABLE = 416,
    EXPECTATION_FAILED = 417,
    IM_A_TEAPOT = 418,
    MISDIRECTED_REQUEST = 421,
    UNPROCESSABLE_ENTITY = 422,
    LOCKED = 423,
    FAILED_DEPENDENCY = 424,
    TOO_EARLY = 425,
    UPGRADE_REQUIRED = 426,
    PRECONDITION_REQUIRED = 428,
    TOO_MANY_REQUESTS = 429,
    REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    UNAVAILABLE_FOR_LEGAL_REASONS = 451,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505,
    VARIANT_ALSO_NEGOTIATES = 506,
    INSUFFICIENT_STORAGE = 507,
    LOOP_DETECTED = 508,
    NOT_EXTENDED = 510,
    NETWORK_AUTHENTICATION_REQUIRED = 511,
  };

  virtual ~Response() = default;

  /**
   * Converts the status code to a string.
   */
  static std::string status_code(Status_code c);

  /**
   * Indicates if Content-Type of the response is set to 'application/json'.
   */
  static bool is_json(const Headers &hdrs);

  /**
   * Indicates if Content-Type of the response is set to 'application/xml'.
   */
  static bool is_xml(const Headers &hdrs);

  /**
   * Checks if the status code is an error.
   */
  static bool is_error(Status_code c);

  /**
   * Decode body as JSON object if Content-Type of the response is set to
   * 'application/json'.
   *
   * @return Decoded JSON object.
   *
   * @throws runtime_error If Content-Type is not 'application/json' or there's
   *         no body.
   */
  shcore::Value json() const;

  /**
   * If response is an error, returns the corresponding Response_error object.
   *
   * @returns Response_error if request failed with an error
   */
  std::optional<Response_error> get_error() const;

  /**
   * Throws an error returned by get_error(), if there's one.
   */
  void throw_if_error() const;

  /**
   * Reads the "Content-Length" header.
   *
   * @throws std::runtime_error If there's no "Content-Length" header.
   * @throws std::out_of_range If size is out of std::size_t range.
   */
  std::size_t content_length() const;

  /**
   * HTTP status code of the response.
   */
  Status_code status;

  /**
   * Headers transmitted along with the response.
   */
  Headers headers;

  /**
   * Body of the response.
   */
  Base_response_buffer *body = nullptr;
};

class Response_error : public std::runtime_error {
 public:
  explicit Response_error(Response::Status_code code,
                          const char *what = nullptr)
      : std::runtime_error(what == nullptr ? Response::status_code(code)
                                           : what),
        m_code(code) {}

  Response_error(Response::Status_code code, const std::string &description)
      : std::runtime_error(description.empty() ? Response::status_code(code)
                                               : description),
        m_code(code) {}

  virtual ~Response_error() {}

  Response::Status_code code() const noexcept { return m_code; }

  virtual std::string format() const;

 protected:
  Response::Status_code m_code;
};

/**
 * Interface to handle data received on a Response.
 */
class Base_response_buffer {
 public:
  //!  Appends the received data into the buffer
  virtual size_t append_data(const char *data, size_t data_size) = 0;

  //!  Returns the amount of data received so far
  virtual size_t size() const = 0;

  //! Returns a reference to the data received
  virtual const char *data() const = 0;

  //! Clears the buffer.
  virtual void clear() = 0;
};

/**
 * Response buffer that uses internal string to hold the data.
 *
 * The internal string will grow as needed to hold the received data.
 *
 * If buffer_length is used on the constructor, the internal string capacity
 * will increaste to hold such length.
 */
class String_buffer : public Base_response_buffer {
 public:
  explicit String_buffer(size_t buffer_length = 0) {
    if (buffer_length) m_buffer.reserve(buffer_length);
  };

  String_buffer(const String_buffer &other) = default;
  String_buffer(String_buffer &&other) = default;

  String_buffer &operator=(const String_buffer &other) = default;
  String_buffer &operator=(String_buffer &&other) = default;

  size_t append_data(const char *data, size_t data_size) override {
    m_buffer.append(data, data_size);
    return data_size;
  }

  const char *data() const override { return m_buffer.data(); }

  size_t size() const override { return m_buffer.size(); }

  void clear() override { m_buffer.clear(); }

  const std::string &raw() const { return m_buffer; }

 private:
  std::string m_buffer;
};

/**
 * Response buffer that uses external char* to write the data directly there.
 *
 * Instances of this class require both valid buffer and buffer size to be
 * provided. Unlimited buffer (0) is not allowed.
 */
class Static_char_ref_buffer : public Base_response_buffer {
 public:
  Static_char_ref_buffer(char *buffer_ptr, size_t bsize)
      : m_buffer(buffer_ptr), m_buffer_size(bsize), m_content_size(0) {
    // Both parameters should be valid
    assert(m_buffer);
    assert(m_buffer_size);
  }

  Static_char_ref_buffer(const Static_char_ref_buffer &other) = delete;
  Static_char_ref_buffer(Static_char_ref_buffer &&other) = delete;

  Static_char_ref_buffer &operator=(const Static_char_ref_buffer &other) =
      delete;
  Static_char_ref_buffer &operator=(Static_char_ref_buffer &&other) = delete;

  size_t append_data(const char *data, size_t data_size) override;

  const char *data() const override { return m_buffer; }

  size_t size() const override { return m_content_size; }

  void clear() override { m_content_size = 0; }

 private:
  char *m_buffer;
  size_t m_buffer_size;
  size_t m_content_size;
};

struct String_response : public Response {
 public:
  String_response() noexcept { body = &buffer; }

  String_response(const String_response &other) : String_response() {
    *this = other;
  }

  String_response(String_response &&other) noexcept : String_response() {
    *this = std::move(other);
  }

  String_response &operator=(const String_response &other) {
    buffer = other.buffer;
    return *this;
  }

  String_response &operator=(String_response &&other) noexcept {
    std::swap(buffer, other.buffer);
    return *this;
  }

  String_buffer buffer;
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_RESPONSE_H_
