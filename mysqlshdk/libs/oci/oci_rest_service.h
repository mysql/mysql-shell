/*
 * Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_OCI_OCI_REST_SERVICE_H_
#define MYSQLSHDK_LIBS_OCI_OCI_REST_SERVICE_H_

#include <openssl/pem.h>
#include <string>

#include "mysqlshdk/libs/oci/oci_options.h"
#include "mysqlshdk/libs/rest/response.h"
#include "mysqlshdk/libs/rest/rest_service.h"

namespace mysqlshdk {
namespace oci {

using mysqlshdk::oci::Oci_options;
using mysqlshdk::rest::Base_response_buffer;
using mysqlshdk::rest::Headers;
using mysqlshdk::rest::Response;
using mysqlshdk::rest::Response_error;
using mysqlshdk::rest::Type;

enum class Oci_service { IDENTITY, OBJECT_STORAGE };

std::string service_identifier(Oci_service service);

class Oci_rest_service;

struct Oci_request : public rest::Request {
 public:
  explicit Oci_request(Masked_string path, rest::Headers headers = {},
                       bool sign = true, bool is_par = false)
      : Request(std::move(path), std::move(headers)),
        sign_request(sign),
        is_par_request(is_par) {}

  bool sign_request = true;

  bool is_par_request = false;

  const Headers &headers() const override;

 private:
  friend class Oci_rest_service;

  Oci_rest_service *m_service = nullptr;

  rest::Headers m_signed_headers;
};

class Oci_rest_service {
 public:
  Oci_rest_service() = default;
  Oci_rest_service &operator=(Oci_rest_service &&other) = delete;
  Oci_rest_service(Oci_service service, const Oci_options &options);

  Response::Status_code get(Oci_request *request, Response *response = nullptr);

  Response::Status_code head(Oci_request *request,
                             Response *response = nullptr);

  Response ::Status_code post(Oci_request *request,
                              Response *response = nullptr);

  Response::Status_code put(Oci_request *request, Response *response = nullptr);

  Response::Status_code delete_(Oci_request *request);

  Response::Status_code execute(Oci_request *request,
                                Response *response = nullptr);

  // TODO(rennox): These configuration properties/functions exists here because
  // the configuration was loaded on the constructor of the REST service,
  // however, it cuold be i.e. passed down through all the chain call when a
  // Bucket/Directory/Object is created and reside somewhere else.
  const std::string &get_region() const { return m_region; }
  const std::string &get_tenancy_id() const { return m_tenancy_id; }

  /**
   * This function allows updating the m_host to point to the right service to
   * be used
   */
  void set_service(Oci_service service);

  const std::string &end_point() const { return m_end_point; }

 private:
  friend struct Oci_request;

  Headers make_headers(Oci_request *request, time_t now);

  bool valid_headers(Oci_request *request, time_t now) const;

  void clear_cache(time_t now);

  Oci_service m_service;
  std::string m_region;
  std::string m_tenancy_id;
  std::string m_host;
  std::string m_auth_keyId;
  std::shared_ptr<EVP_PKEY> m_private_key;

  using Method_time = std::unordered_map<Type, time_t>;
  std::unordered_map<std::string, Method_time> m_signed_header_cache_time;
  using Header_method = std::unordered_map<Type, Headers>;
  std::unordered_map<std::string, Header_method> m_cached_header;
  time_t m_cache_cleared_at = 0;

  std::string m_end_point;
  std::string m_service_label;
};
}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_OCI_REST_SERVICE_H_
