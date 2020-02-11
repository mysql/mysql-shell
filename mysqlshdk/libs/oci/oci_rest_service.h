/*
 * Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_OCI_REST_SERVICE_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_OCI_REST_SERVICE_H_

#include <openssl/pem.h>
#include <string>

#include "mysqlshdk/libs/oci/oci_options.h"
#include "mysqlshdk/libs/rest/response.h"
#include "mysqlshdk/libs/rest/rest_service.h"

namespace mysqlshdk {
namespace oci {

using mysqlshdk::rest::Response;
using mysqlshdk::rest::Response_error;

class Oci_error : public Response_error {
 public:
  Oci_error(Response::Status_code code, const char *what = nullptr)
      : Response_error(code, what) {}

  Oci_error(Response::Status_code code, const std::string &description)
      : Response_error(code, description) {}

  Oci_error(const Response &response)
      : Response_error(response.status,
                       response.json().as_map()->get_string("message")) {}

  virtual ~Oci_error() {}

  std::string format() const override {
    return "OCI Error " + std::to_string(static_cast<int>(m_code)) + ": " +
           what();
  }
};

enum class Oci_service { IDENTITY, OBJECT_STORAGE };

class Oci_rest_service {
 public:
  Oci_rest_service() = default;
  Oci_rest_service &operator=(Oci_rest_service &&other) = delete;
  Oci_rest_service(Oci_service service, const Oci_options &options);

  mysqlshdk::rest::Response get(const std::string &path,
                                const mysqlshdk::rest::Headers &headers = {});

  mysqlshdk::rest::Response head(const std::string &path,
                                 const mysqlshdk::rest::Headers &headers = {});

  mysqlshdk::rest::Response post(const std::string &path, const char *body,
                                 size_t size,
                                 const mysqlshdk::rest::Headers &headers = {});

  mysqlshdk::rest::Response put(const std::string &path, const char *body,
                                size_t size,
                                const mysqlshdk::rest::Headers &headers = {});

  mysqlshdk::rest::Response delete_(
      const std::string &path, const char *body, size_t size,
      const mysqlshdk::rest::Headers &headers = {});

  // TODO(rennox): These configuration properties/functions exists here because
  // the configuration was loaded on the constructor of the RESR service,
  // however, it cuold be i.e. passed down through all the chain call when a
  // Bucket/Directory/Object is created and reside somewhere else.
  std::string get_region() { return m_region; }
  std::string get_tenancy_id() { return m_tenancy_id; }
  std::string get_uri() { return "oci+os://" + m_region; }

  /**
   * This function allows upadting the m_host to point to the right service to
   * be used
   */
  void set_service(Oci_service service);

 private:
  Oci_service m_service;
  std::string m_region;
  std::string m_tenancy_id;
  std::string m_host;
  std::string m_auth_keyId;
  std::shared_ptr<EVP_PKEY> m_private_key;
  std::unique_ptr<mysqlshdk::rest::Rest_service> m_rest;

  using Time_method = std::map<std::string, time_t>;
  std::map<std::string, Time_method> m_signed_header_cache_time;
  using Header_method = std::map<std::string, mysqlshdk::rest::Headers>;
  std::map<std::string, Header_method> m_cached_header;

  mysqlshdk::rest::Headers make_header(const std::string &path,
                                       const std::string &method,
                                       const char *body, size_t size,
                                       const mysqlshdk::rest::Headers headers);
};
}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_OCI_REST_SERVICE_H_
