/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_REST_SIGNED_REST_SERVICE_H_
#define MYSQLSHDK_LIBS_REST_SIGNED_REST_SERVICE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "mysqlshdk/libs/rest/response.h"
#include "mysqlshdk/libs/rest/rest_service.h"

namespace mysqlshdk {
namespace rest {

class Signed_rest_service;

struct Signed_request : public rest::Request {
 public:
  explicit Signed_request(Masked_string path, rest::Headers headers = {})
      : Request(std::move(path), std::move(headers)) {}

  const Headers &headers() const override;

  const Headers &unsigned_headers() const { return m_headers; }

 private:
  friend class Signed_rest_service;

  Signed_rest_service *m_service = nullptr;

  rest::Headers m_signed_headers;
};

class Signer {
 public:
  Signer() = default;

  Signer(const Signer &) = default;
  Signer(Signer &&) = default;

  Signer &operator=(const Signer &) = default;
  Signer &operator=(Signer &&) = default;

  virtual ~Signer() = default;

  virtual bool should_sign_request(const Signed_request *request) const = 0;

  virtual Headers sign_request(const Signed_request *request,
                               time_t now) const = 0;

  virtual bool refresh_auth_data() = 0;
};

class Signed_rest_service_config {
 public:
  Signed_rest_service_config() = default;

  Signed_rest_service_config(const Signed_rest_service_config &) = default;
  Signed_rest_service_config(Signed_rest_service_config &&) = default;

  Signed_rest_service_config &operator=(const Signed_rest_service_config &) =
      default;
  Signed_rest_service_config &operator=(Signed_rest_service_config &&) =
      default;

  virtual ~Signed_rest_service_config() = default;

  virtual const std::string &service_endpoint() const = 0;

  virtual const std::string &service_label() const = 0;

  virtual std::unique_ptr<Signer> signer() const = 0;
};

class Signed_rest_service {
 public:
  Signed_rest_service() = delete;

  explicit Signed_rest_service(const Signed_rest_service_config &config);

  Signed_rest_service(const Signed_rest_service &) = delete;
  Signed_rest_service(Signed_rest_service &&) = default;

  Signed_rest_service &operator=(const Signed_rest_service &) = delete;
  Signed_rest_service &operator=(Signed_rest_service &&) = default;

  virtual ~Signed_rest_service() = default;

  Response::Status_code get(Signed_request *request,
                            Response *response = nullptr);

  Response::Status_code head(Signed_request *request,
                             Response *response = nullptr);

  Response ::Status_code post(Signed_request *request,
                              Response *response = nullptr);

  Response::Status_code put(Signed_request *request,
                            Response *response = nullptr);

  Response::Status_code delete_(Signed_request *request);

 private:
  friend struct Signed_request;

  Response::Status_code execute(Signed_request *request,
                                Response *response = nullptr);

  Headers make_headers(const Signed_request *request, time_t now);

  bool valid_headers(const Signed_request *request, time_t now) const;

  void clear_cache(time_t now);

  using Method_time = std::unordered_map<Type, time_t>;
  std::unordered_map<std::string, Method_time> m_signed_header_cache_time;

  using Header_method = std::unordered_map<Type, Headers>;
  std::unordered_map<std::string, Header_method> m_cached_header;

  time_t m_cache_cleared_at = 0;
  std::string m_endpoint;
  std::string m_label;
  std::unique_ptr<Signer> m_signer;
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_SIGNED_REST_SERVICE_H_
