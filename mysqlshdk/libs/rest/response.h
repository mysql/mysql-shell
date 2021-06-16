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

#ifndef MYSQLSHDK_LIBS_REST_RESPONSE_H_
#define MYSQLSHDK_LIBS_REST_RESPONSE_H_

#include "mysqlshdk/include/scripting/types.h"

#include <algorithm>
#include <string>

#include "mysqlshdk/libs/rest/headers.h"

namespace mysqlshdk {
namespace rest {

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
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NON_AUTHORITATIVE_INFORMATION = 203,
    NO_CONTENT = 204,
    RESET_CONTENT = 205,
    PARTIAL_CONTENT = 206,
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    USE_PROXY = 305,
    SWITCH_PROXY = 306,
    TEMPORARY_REDIRECT = 307,
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
    UPGRADE_REQUIRED = 426,
    TOO_MANY_REQUESTS = 429,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505,
  };

  static void check_and_throw(Response::Status_code code,
                              const Headers &headers, const char *buffer,
                              size_t size);

  void check_and_throw();

  static std::string status_code(Status_code c);

  /**
   * Indicates if Content-Type of the response is set to 'application/json'.
   */
  static bool is_json(const Headers &hdrs);

  /**
   * Decode body as JSON object if Content-Type of the response is set to
   * 'application/json'.
   *
   * @return Decoded JSON object.
   */
  shcore::Value json() const;

  /**
   * HTTP status code of the response.
   */
  Status_code status;

  /**
   * Headers transmitted along with the response.
   */
  Headers headers;

  /**
   * Body of the response. Raw string.
   */
  std::string body;
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

  //! Retuns a reference to the data received
  virtual const char *data() const = 0;

  //! Clears the buffer..
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

 private:
  std::string m_buffer;
};

/**
 * Response buffer that uses external char* to write the data directly there.
 *
 * Instances of this class require both valid buffer and buffer size to be
 * provided. Unlimited buffer is not allowed (0).
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

/**
 * Response buffer that uses external string to hold the data.
 *
 * @param buffer pointer to a string to hold the data.
 */
class String_ref_buffer : public Base_response_buffer {
 public:
  explicit String_ref_buffer(std::string *buffer) : m_buffer(buffer) {}

  String_ref_buffer(const String_ref_buffer &other) = delete;
  String_ref_buffer(String_ref_buffer &&other) = delete;

  String_ref_buffer &operator=(const String_ref_buffer &other) = delete;
  String_ref_buffer &operator=(String_ref_buffer &&other) = delete;

  size_t append_data(const char *data, size_t data_size) override {
    m_buffer->append(data, data_size);
    return data_size;
  }

  const char *data() const override { return m_buffer->data(); }

  size_t size() const override { return m_buffer->size(); }

  void clear() override { m_buffer->clear(); }

 private:
  std::string *m_buffer;
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_RESPONSE_H_
