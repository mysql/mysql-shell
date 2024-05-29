/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_REST_SIGNED_SIGNED_REST_SERVICE_H_
#define MYSQLSHDK_LIBS_REST_SIGNED_SIGNED_REST_SERVICE_H_

#include <ctime>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "mysqlshdk/libs/rest/headers.h"
#include "mysqlshdk/libs/rest/response.h"
#include "mysqlshdk/libs/rest/rest_service.h"
#include "mysqlshdk/libs/rest/signed/signed_request.h"
#include "mysqlshdk/libs/rest/signed/signed_rest_service_config.h"
#include "mysqlshdk/libs/rest/signed/signer.h"

namespace mysqlshdk {
namespace rest {

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

  bool should_sign_request(const Signed_request &request) const {
    return m_signer->should_sign_request(request);
  }

  Headers make_headers(const Signed_request &request, time_t now);

  bool valid_headers(const Signed_request &request, time_t now) const;

  void clear_cache(time_t now);

  void invalidate_cache();

  using Method_time = std::unordered_map<Type, time_t>;
  std::unordered_map<std::string, Method_time> m_signed_header_cache_time;

  using Header_method = std::unordered_map<Type, Headers>;
  std::unordered_map<std::string, Header_method> m_cached_header;

  time_t m_cache_cleared_at = 0;
  std::string m_endpoint;
  std::string m_label;
  std::unique_ptr<ISigner> m_signer;
  bool m_enable_signature_caching;
  std::unique_ptr<IRetry_strategy> m_retry_strategy;
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_SIGNED_SIGNED_REST_SERVICE_H_
