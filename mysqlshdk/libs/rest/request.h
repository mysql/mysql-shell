/*
 * Copyright (c) 2021, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_REST_REQUEST_H_
#define MYSQLSHDK_LIBS_REST_REQUEST_H_

#include <map>
#include <optional>
#include <string>
#include <utility>

#include "mysqlshdk/include/scripting/types.h"

#include "mysqlshdk/libs/utils/masked_value.h"

#include "mysqlshdk/libs/rest/headers.h"

namespace mysqlshdk {
namespace rest {

class IRetry_strategy;

#ifdef _WIN32
#undef DELETE
#endif

enum class Type { GET, HEAD, POST, PUT, PATCH, DELETE };

std::string type_name(Type t);

using Query = std::map<std::string, std::optional<std::string>>;

/**
 * Creates a string which can be sent as a body of
 * application/x-www-form-urlencoded Content-Type.
 *
 * NOTE: keys are NOT encoded (in other words they are expected to be
 *       alphanumeric).
 *
 * @param query Key-value pairs to be encoded.
 * @param include_empty_values If set to true, empty values are written as
 *        `key=`, otherwise empty values are written as: `key`.
 *
 * @returns Encoded string.
 */
std::string www_form_urlencoded(const Query &query,
                                bool include_empty_values = false);

/**
 * A REST request.
 */
struct Request {
 public:
  /**
   * @param path Path to the request, it is going to be appended to the base
   *        URL.
   * @param headers Optional request-specific headers. If default headers were
   *        also specified, request-specific headers are going to be appended
   *        that set, overwriting any duplicated values.
   * @param query Optional query to be appended to the path.
   * @param include_empty_values If set to true, empty query values are written
   *        as `key=`, otherwise empty values are written as: `key`.
   */
  explicit Request(Masked_string path, Headers headers = {}, Query query = {},
                   bool include_empty_values = false);

  /**
   * Type of the request.
   */

  Type type;

  /**
   * Path to the request.
   */
  const Masked_string &path() const { return m_path; }

  /**
   * Path and query to the request
   */
  const Masked_string &full_path() const { return m_full_path; }

  /**
   * Optional body which is going to be sent along with the request.
   */
  const char *body = nullptr;

  /**
   * Size of the body.
   */
  size_t size = 0;

  /**
   * Headers sent along with the request.
   */
  virtual const Headers &headers() const { return m_headers; }

  /**
   * Query parameters for the request.
   */
  virtual const Query &query() const { return m_query; }

  /**
   * Retry strategy to use.
   */
  IRetry_strategy *retry_strategy = nullptr;

 protected:
  Masked_string m_path;

  Headers m_headers;

  Query m_query;

  Masked_string m_full_path;
};

struct Json_request final : public Request {
 public:
  /**
   * @param path Path to the request, it is going to be appended to the base
   *        URL.
   * @param data Optional body which is going to be sent along with the request.
   *        Content-Type is going to be set to 'application/json', unless it is
   *        explicitly set in headers.
   * @param headers Optional request-specific headers. If default headers were
   *        also specified, request-specific headers are going to be appended
   *        that set, overwriting any duplicated values.
   */
  explicit Json_request(Masked_string path, const shcore::Value &data = {},
                        Headers headers = {})
      : Request(std::move(path), std::move(headers)) {
    if (data.get_type() != shcore::Value_type::Undefined) {
      m_json = data.repr();
    }

    size = m_json.size();

    if (size) {
      body = m_json.c_str();
    }
  }

 private:
  std::string m_json;
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_REQUEST_H_
