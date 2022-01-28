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

#include "mysqlshdk/libs/rest/signed_rest_service.h"

#include <string>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/rest/retry_strategy.h"

namespace mysqlshdk {
namespace rest {

namespace {

constexpr time_t k_header_cache_ttl = 60;

mysqlshdk::rest::Rest_service *get_rest_service(const std::string &uri,
                                                const std::string &label) {
  static thread_local std::unordered_map<
      std::string, std::unique_ptr<mysqlshdk::rest::Rest_service>>
      services;

  auto &service = services[uri];

  if (!service) {
    // copy the value, Rest_service may outlive the original caller
    auto real = uri;
    Masked_string copy = {std::move(real)};
    service =
        std::make_unique<mysqlshdk::rest::Rest_service>(copy, true, label);
  }

  return service.get();
}

}  // namespace

const Headers &Signed_request::headers() const {
  if (m_service && m_service->m_signer->should_sign_request(this)) {
    auto self = const_cast<Signed_request *>(this);
    const auto now = time(nullptr);

    if (!m_signed_headers.empty() && !m_service->valid_headers(this, now)) {
      self->m_signed_headers.clear();
    }

    if (m_signed_headers.empty()) {
      self->m_signed_headers = m_service->make_headers(this, now);
    }

    return m_signed_headers;
  } else {
    return m_headers;
  }
}

Signed_rest_service::Signed_rest_service(
    const Signed_rest_service_config &config)
    : m_endpoint{config.service_endpoint()},
      m_label{config.service_label()},
      m_signer{config.signer()} {}

Response::Status_code Signed_rest_service::get(Signed_request *request,
                                               Response *response) {
  request->type = Type::GET;
  request->body = nullptr;
  request->size = 0;
  return execute(request, response);
}

Response::Status_code Signed_rest_service::head(Signed_request *request,
                                                Response *response) {
  request->type = Type::HEAD;
  request->body = nullptr;
  request->size = 0;
  return execute(request, response);
}

Response::Status_code Signed_rest_service::post(Signed_request *request,
                                                Response *response) {
  request->type = Type::POST;
  return execute(request, response);
}

Response::Status_code Signed_rest_service::put(Signed_request *request,
                                               Response *response) {
  request->type = Type::PUT;
  return execute(request, response);
}

Response::Status_code Signed_rest_service::delete_(Signed_request *request) {
  request->type = Type::DELETE;
  return execute(request);
}

void Signed_rest_service::clear_cache(time_t now) {
  if (0 == m_cache_cleared_at) {
    m_cache_cleared_at = now;
  }

  if (now - m_cache_cleared_at > k_header_cache_ttl) {
    std::vector<std::pair<std::reference_wrapper<const std::string>, Type>>
        entries;

    for (const auto &path : m_signed_header_cache_time) {
      for (const auto &method : path.second) {
        if (now - method.second > k_header_cache_ttl) {
          entries.emplace_back(path.first, method.first);
        }
      }
    }

    log_debug("Removing %zu entries from header cache", entries.size());

    for (const auto &entry : entries) {
      auto &cached_path = m_signed_header_cache_time[entry.first];

      m_cached_header[entry.first].erase(entry.second);
      cached_path.erase(entry.second);

      if (cached_path.empty()) {
        m_cached_header.erase(entry.first);
        m_signed_header_cache_time.erase(entry.first);
      }
    }

    m_cache_cleared_at = now;
  }
}

bool Signed_rest_service::valid_headers(const Signed_request *request,
                                        time_t now) const {
  const auto &path = request->path().real();
  const auto method = request->type;

  // we assume that headers were cached, and that body did not change (no need
  // to rehash the body)
  return now - m_signed_header_cache_time.at(path).at(method) <=
         k_header_cache_ttl;
}

Headers Signed_rest_service::make_headers(const Signed_request *request,
                                          time_t now) {
  clear_cache(now);

  const auto &path = request->path().real();
  const auto method = request->type;
  auto &cached_time = m_signed_header_cache_time[path][method];

  // Maximum Allowed Client Clock Skew from the server's clock for OCI requests
  // is 5 minutes, 15 minutes in case of AWS. We can exploit that feature to
  // cache auth header, because it is expensive to calculate.
  //
  // Any requests with body are exceptions as the signature may include the body
  // sha256
  if (now - cached_time > k_header_cache_ttl || request->size) {
    m_cached_header[path][method] = m_signer->sign_request(request, now);
    cached_time = now;
  }

  auto final_headers = m_cached_header[path][method];

  // Adds any additional headers
  for (const auto &header : request->m_headers) {
    final_headers[header.first] = header.second;
  }

  return final_headers;
}

Response::Status_code Signed_rest_service::execute(Signed_request *request,
                                                   Response *response) {
  const auto rest = get_rest_service(m_endpoint, m_label);

  // Caller might not be interested on handling the response data directly, i.e.
  // if just expecting the call to either succeed or fail.
  // We need to ensure a response handler is in place in order to properly fail
  // on errors.
  rest::String_response string_response;
  if (!response) response = &string_response;
  if (!response->body) response->body = &string_response.buffer;

  std::unique_ptr<rest::Retry_strategy> retry_strategy;

  if (!request->retry_strategy) {
    retry_strategy = rest::default_retry_strategy();
    request->retry_strategy = retry_strategy.get();
  }

  request->m_service = this;

  rest::Response::Status_code code;
  bool retry = false;
  int retries = 0;
  constexpr int k_authorization_retry_limit = 2;

  do {
    code = rest->execute(request, response);
    retry = Response::Status_code::UNAUTHORIZED == code &&
            ++retries <= k_authorization_retry_limit &&
            m_signer->refresh_auth_data();

    if (retry) {
      // we've refreshed the authorization data, cache is no longer valid
      m_signed_header_cache_time.clear();
      m_cached_header.clear();
    }
  } while (retry);

  response->throw_if_error();

  return code;
}

}  // namespace rest
}  // namespace mysqlshdk
