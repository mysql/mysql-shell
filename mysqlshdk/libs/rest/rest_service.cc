/*
 * Copyright (c) 2019, 2020 Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/rest/rest_service.h"

#include <curl/curl.h>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#ifdef _WIN32
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <wincrypt.h>
#endif  // _WIN32

namespace mysqlshdk {
namespace rest {

namespace {

static constexpr auto k_content_type = "Content-Type";
static constexpr auto k_application_json = "application/json";

std::string get_user_agent() { return "mysqlsh/" MYSH_VERSION; }

size_t request_callback(char *, size_t, size_t, void *) {
  // some older versions of CURL may call this callback when performing
  // POST-like request with Content-Length set to 0
  return 0;
}

size_t response_data_callback(char *ptr, size_t size, size_t nmemb,
                              void *userdata) {
  auto buffer = static_cast<Base_response_buffer *>(userdata);
  const auto bytes = size * nmemb;

  // If a handler for the response was set, the data is passed there
  if (buffer) buffer->append_data(ptr, bytes);

  return bytes;
}

size_t response_header_callback(char *ptr, size_t size, size_t nmemb,
                                void *userdata) {
  auto data = static_cast<std::string *>(userdata);
  const auto bytes = size * nmemb;

  data->append(ptr, bytes);

  return bytes;
}

Headers parse_headers(const std::string &s) {
  Headers headers;

  if (!s.empty()) {
    const auto lines = shcore::str_split(s, "\r\n", -1, true);

    // first line is the status line, can be skipped
    // header-field   = field-name ":" OWS field-value OWS
    // OWS            = *( SP / HTAB )
    for (auto it = std::next(lines.begin()); it != lines.end(); ++it) {
      const auto colon = it->find(':');

      if (std::string::npos != colon) {
        const auto begin = it->find_first_not_of(" \t", colon + 1);
        const auto end = it->find_last_not_of(" \t");

        headers[it->substr(0, colon)] = it->substr(begin, end - begin + 1);
      }
    }
  }

  return headers;
}

#ifdef _WIN32

static CURLcode sslctx_function(CURL *curl, void *sslctx, void *parm) {
  const auto store = CertOpenSystemStore(NULL, "ROOT");

  if (store) {
    PCCERT_CONTEXT context = nullptr;
    X509_STORE *cts = SSL_CTX_get_cert_store((SSL_CTX *)sslctx);

    while (context = CertEnumCertificatesInStore(store, context)) {
      // temporary variable is mandatory
      const unsigned char *encoded_cert = context->pbCertEncoded;
      const auto x509 =
          d2i_X509(nullptr, &encoded_cert, context->cbCertEncoded);

      if (x509) {
        X509_STORE_add_cert(cts, x509);
        X509_free(x509);
      }
    }

    CertCloseStore(store, 0);
  }

  return CURLE_OK;
}

#endif  // _WIN32

}  // namespace

std::string type_name(Type method) {
  switch (method) {
    case Type::DELETE:
      return "delete";
    case Type::GET:
      return "get";
    case Type::HEAD:
      return "head";
    case Type::PATCH:
      return "patch";
    case Type::POST:
      return "post";
    case Type::PUT:
      return "put";
  }
  throw std::logic_error("Unknown method received");
}

class Rest_service::Impl {
 public:
  /**
   * Type of the HTTP request.
   */
  Impl(const std::string &base_url, bool verify, const std::string &label)
      : m_handle(curl_easy_init(), &curl_easy_cleanup),
        m_base_url{base_url},
        m_request_sequence(0) {
    // Disable signal handlers used by libcurl, we're potentially going to use
    // timeouts and background threads.
    curl_easy_setopt(m_handle.get(), CURLOPT_NOSIGNAL, 1L);
    // we're going to automatically follow redirections
    curl_easy_setopt(m_handle.get(), CURLOPT_FOLLOWLOCATION, 1L);
    // most modern browsers allow for more or less 20 redirections
    curl_easy_setopt(m_handle.get(), CURLOPT_MAXREDIRS, 20L);
    // introduce ourselves to the server
    curl_easy_setopt(m_handle.get(), CURLOPT_USERAGENT,
                     get_user_agent().c_str());

#if LIBCURL_VERSION_NUM >= 0x071900
    // CURLOPT_TCP_KEEPALIVE was added in libcurl 7.25.0
    curl_easy_setopt(m_handle.get(), CURLOPT_TCP_KEEPALIVE, 1L);
#endif

    // error buffer, once set, must be available until curl_easy_cleanup() is
    // called
    curl_easy_setopt(m_handle.get(), CURLOPT_ERRORBUFFER, m_error_buffer);

    verify_ssl(verify);

    // Default timeout for HEAD/DELETE: 30000 milliseconds
    // The rest of the operations will timeout if transfer rate is lower than
    // one byte in 5 minutes (300 seconds)
    set_timeout(30000, 1, 300);

    // set the callbacks
    curl_easy_setopt(m_handle.get(), CURLOPT_READDATA, nullptr);
    curl_easy_setopt(m_handle.get(), CURLOPT_READFUNCTION, request_callback);
    curl_easy_setopt(m_handle.get(), CURLOPT_WRITEFUNCTION,
                     response_data_callback);
    curl_easy_setopt(m_handle.get(), CURLOPT_HEADERFUNCTION,
                     response_header_callback);

#ifdef _WIN32
    curl_easy_setopt(m_handle.get(), CURLOPT_CAINFO, nullptr);
    curl_easy_setopt(m_handle.get(), CURLOPT_CAPATH, nullptr);
    curl_easy_setopt(m_handle.get(), CURLOPT_SSL_CTX_FUNCTION,
                     *sslctx_function);
#endif  // _WIN32

    // Initializes the ID for this Rest Service Instance
    m_id = "REST-";
    if (!label.empty()) m_id += label + "-";

    m_id += shcore::get_random_string(
        5, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890");
  }

  ~Impl() = default;

  void log_request(Type type, const std::string &path, const Headers &headers) {
    log_debug("%s-%d: %s %s", m_id.c_str(), m_request_sequence,
              shcore::str_upper(type_name(type)).c_str(), path.c_str());

    if (!headers.empty() && shcore::Logger::singleton()->get_log_level() >=
                                shcore::Logger::LOG_LEVEL::LOG_DEBUG2) {
      std::vector<std::string> header_data;
      for (const auto &header : headers) {
        header_data.push_back("'" + header.first + "' : '" + header.second +
                              "'");
      }
      log_debug2("%s-%d: REQUEST HEADERS:\n  %s", m_id.c_str(),
                 m_request_sequence,
                 shcore::str_join(header_data, "\n  ").c_str());
    }
  }

  void log_response(int sequence, Response::Status_code code,
                    const std::string &headers) {
    log_debug("%s-%d: %d-%s", m_id.c_str(), sequence, static_cast<int>(code),
              Response::status_code(code).c_str());

    if (!headers.empty()) {
      log_debug2("%s-%d: RESPONSE HEADERS:\n  %s", m_id.c_str(), sequence,
                 headers.c_str());
    }
  }

  Response execute(Type type, const std::string &path,
                   const shcore::Value &body, const Headers &headers,
                   bool synch = true) {
    m_request_sequence++;

    log_request(type, path, headers);

    set_url(path);
    // body needs to be set before the type, because it implicitly sets type to
    // POST
    // NOTE: This variable is required here so in synch requests the buffer is
    // valid through the entire request
    auto body_str = body.repr();
    if (body.type == shcore::Value_type::Undefined) body_str.clear();

    set_body(body_str.data(), body_str.length(), synch);

    set_type(type);
    const auto headers_deleter =
        set_headers(headers, body.type != shcore::Value_type::Undefined);

    // set callbacks which will receive the response
    Response response;

    std::string response_headers;

    curl_easy_setopt(m_handle.get(), CURLOPT_HEADERDATA, &response_headers);
    String_ref_buffer buffer(&response.body);
    curl_easy_setopt(m_handle.get(), CURLOPT_WRITEDATA, &buffer);

    // execute the request
    if (curl_easy_perform(m_handle.get()) != CURLE_OK) {
      log_error("%s-%d %s", m_id.c_str(), m_request_sequence, m_error_buffer);
      throw Connection_error{m_error_buffer};
    }

    response.status = get_status_code();
    response.headers = parse_headers(response_headers);

    log_response(m_request_sequence, response.status, response_headers);

    return response;
  }

  Response::Status_code execute(Type type, const std::string &path,
                                const char *body, size_t size,
                                const Headers &request_headers,
                                Base_response_buffer *buffer,
                                Headers *response_headers) {
    m_request_sequence++;

    log_request(type, path, request_headers);

    set_url(path);
    // body needs to be set before the type, because it implicitly sets type
    // to POST
    set_body(body, size, true);
    set_type(type);
    const auto headers_deleter = set_headers(request_headers, true);

    // set callbacks which will receive the response
    std::string header_data;
    curl_easy_setopt(m_handle.get(), CURLOPT_HEADERDATA, &header_data);
    curl_easy_setopt(m_handle.get(), CURLOPT_WRITEDATA, buffer);

    // execute the request
    auto ret_val = curl_easy_perform(m_handle.get());
    if (ret_val != CURLE_OK) {
      log_error("%s-%d: %s", m_id.c_str(), m_request_sequence, m_error_buffer);
      throw Connection_error{m_error_buffer};
    }

    if (response_headers) *response_headers = parse_headers(header_data);

    auto status = get_status_code();

    log_response(m_request_sequence, status, header_data);

    return status;
  }

  void set_body(const char *body, size_t size, bool synch) {
    curl_easy_setopt(m_handle.get(), CURLOPT_POSTFIELDSIZE, size);
    if (synch) {
      curl_easy_setopt(m_handle.get(), CURLOPT_COPYPOSTFIELDS, NULL);
      curl_easy_setopt(m_handle.get(), CURLOPT_POSTFIELDS, body);
    } else {
      curl_easy_setopt(m_handle.get(), CURLOPT_COPYPOSTFIELDS, body);
    }
  }

  std::future<Response> execute_async(Type type, const std::string &path,
                                      const shcore::Value &body,
                                      const Headers &headers) {
    return std::async(std::launch::async, [this, type, path, body, headers]() {
      return execute(type, path, body, headers, false);
    });
  }

  void set(const Basic_authentication &basic) {
    curl_easy_setopt(m_handle.get(), CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(m_handle.get(), CURLOPT_USERNAME,
                     basic.username().c_str());
    curl_easy_setopt(m_handle.get(), CURLOPT_PASSWORD,
                     basic.password().c_str());
  }

  void set_default_headers(const Headers &headers) {
    m_default_headers = headers;
  }

  void set_timeout(long timeout, long low_speed_limit, long low_speed_time) {
    m_default_timeout = timeout;
    curl_easy_setopt(m_handle.get(), CURLOPT_LOW_SPEED_LIMIT,
                     static_cast<long>(low_speed_limit));
    curl_easy_setopt(m_handle.get(), CURLOPT_LOW_SPEED_TIME,
                     static_cast<long>(low_speed_time));
  }

 private:
  void verify_ssl(bool verify) {
    curl_easy_setopt(m_handle.get(), CURLOPT_SSL_VERIFYHOST, verify ? 2L : 0L);
    curl_easy_setopt(m_handle.get(), CURLOPT_SSL_VERIFYPEER, verify ? 1L : 0L);
  }

  void set_type(Type type) {
    // custom request overwrites any other option, make sure it's set to
    // default
    curl_easy_setopt(m_handle.get(), CURLOPT_CUSTOMREQUEST, nullptr);
    curl_easy_setopt(m_handle.get(), CURLOPT_TIMEOUT_MS, 0);

    switch (type) {
      case Type::GET:
        curl_easy_setopt(m_handle.get(), CURLOPT_HTTPGET, 1L);
        break;

      case Type::HEAD:
        curl_easy_setopt(m_handle.get(), CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(m_handle.get(), CURLOPT_NOBODY, 1L);
        curl_easy_setopt(m_handle.get(), CURLOPT_TIMEOUT_MS, m_default_timeout);
        break;

      case Type::POST:
        curl_easy_setopt(m_handle.get(), CURLOPT_NOBODY, 0L);
        curl_easy_setopt(m_handle.get(), CURLOPT_POST, 1L);
        break;

      case Type::PUT:
        curl_easy_setopt(m_handle.get(), CURLOPT_NOBODY, 0L);
        // We could use CURLOPT_UPLOAD here, but that would mean we have to
        // provide request data using CURLOPT_READDATA. Using custom request
        // allows to always use CURLOPT_COPYPOSTFIELDS.
        curl_easy_setopt(m_handle.get(), CURLOPT_CUSTOMREQUEST, "PUT");
        break;

      case Type::PATCH:
        curl_easy_setopt(m_handle.get(), CURLOPT_NOBODY, 0L);
        curl_easy_setopt(m_handle.get(), CURLOPT_CUSTOMREQUEST, "PATCH");
        break;

      case Type::DELETE:
        curl_easy_setopt(m_handle.get(), CURLOPT_NOBODY, 0L);
        curl_easy_setopt(m_handle.get(), CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(m_handle.get(), CURLOPT_TIMEOUT_MS, m_default_timeout);
        break;
    }
  }

  void set_url(const std::string &path) {
    curl_easy_setopt(m_handle.get(), CURLOPT_URL, (m_base_url + path).c_str());
  }

  std::unique_ptr<curl_slist, void (*)(curl_slist *)> set_headers(
      const Headers &headers, bool has_body) {
    // create the headers list
    curl_slist *header_list = nullptr;

    {
      // headers which are going to be sent with the request
      Headers merged_headers;

      // if body is set assume 'application/json' content type
      if (has_body) {
        merged_headers.emplace(k_content_type, k_application_json);
      }

      // add default headers
      for (const auto &h : m_default_headers) {
        merged_headers[h.first] = h.second;
      }

      // add request-specific headers
      for (const auto &h : headers) {
        merged_headers[h.first] = h.second;
      }

      // initialize CURL list
      for (const auto &h : merged_headers) {
        // empty headers are delimited with semicolons
        header_list = curl_slist_append(
            header_list,
            (h.first + (h.second.empty() ? ";" : ": " + h.second)).c_str());
      }
    }

    // set the headers
    curl_easy_setopt(m_handle.get(), CURLOPT_HTTPHEADER, header_list);
    // automatically delete the headers when leaving the scope
    return std::unique_ptr<curl_slist, void (*)(curl_slist *)>{
        header_list, &curl_slist_free_all};
  }

  Response::Status_code get_status_code() const {
    long response_code = 0;
    curl_easy_getinfo(m_handle.get(), CURLINFO_RESPONSE_CODE, &response_code);
    return static_cast<Response::Status_code>(response_code);
  }

  std::unique_ptr<CURL, void (*)(CURL *)> m_handle;

  char m_error_buffer[CURL_ERROR_SIZE];

  std::string m_base_url;

  Headers m_default_headers;

  std::string m_id;

  int m_request_sequence;

  long m_default_timeout;
};

Rest_service::Rest_service(const std::string &base_url, bool verify_ssl,
                           const std::string &service_label)
    : m_impl(std::make_unique<Impl>(base_url, verify_ssl, service_label)) {}

Rest_service::Rest_service(Rest_service &&) = default;

Rest_service &Rest_service::operator=(Rest_service &&) = default;

Rest_service::~Rest_service() = default;

Rest_service &Rest_service::set(const Basic_authentication &basic) {
  m_impl->set(basic);
  return *this;
}

Rest_service &Rest_service::set_default_headers(const Headers &headers) {
  m_impl->set_default_headers(headers);
  return *this;
}

Rest_service &Rest_service::set_timeout(long timeout, long low_speed_limit,
                                        long low_speed_time) {
  m_impl->set_timeout(timeout, low_speed_limit, low_speed_time);
  return *this;
}

Response Rest_service::get(const std::string &path, const Headers &headers) {
  return m_impl->execute(Type::GET, path, {}, headers);
}

Response Rest_service::head(const std::string &path, const Headers &headers) {
  return m_impl->execute(Type::HEAD, path, {}, headers);
}

Response Rest_service::post(const std::string &path, const shcore::Value &body,
                            const Headers &headers) {
  return m_impl->execute(Type::POST, path, body, headers);
}

Response Rest_service::put(const std::string &path, const shcore::Value &body,
                           const Headers &headers) {
  return m_impl->execute(Type::PUT, path, body, headers);
}

Response Rest_service::patch(const std::string &path, const shcore::Value &body,
                             const Headers &headers) {
  return m_impl->execute(Type::PATCH, path, body, headers);
}

Response Rest_service::delete_(const std::string &path,
                               const shcore::Value &body,
                               const Headers &headers) {
  return m_impl->execute(Type::DELETE, path, body, headers);
}

Response::Status_code Rest_service::execute(Type type, const std::string &path,
                                            const char *body, size_t size,
                                            const Headers &request_headers,
                                            Base_response_buffer *buffer,
                                            Headers *response_headers,
                                            Retry_strategy *retry_strategy) {
  if (retry_strategy) retry_strategy->init();

  while (true) {
    try {
      auto code = m_impl->execute(type, path, body, size, request_headers,
                                  buffer, response_headers);

      if (!retry_strategy || !retry_strategy->should_retry(code)) {
        return code;
      } else {
        retry_strategy->wait_for_retry();
      }
    } catch (...) {
      if (retry_strategy && retry_strategy->should_retry())
        retry_strategy->wait_for_retry();
      else
        throw;
    }
  }
}

std::future<Response> Rest_service::async_get(const std::string &path,
                                              const Headers &headers) {
  return m_impl->execute_async(Type::GET, path, {}, headers);
}

std::future<Response> Rest_service::async_head(const std::string &path,
                                               const Headers &headers) {
  return m_impl->execute_async(Type::HEAD, path, {}, headers);
}

std::future<Response> Rest_service::async_post(const std::string &path,
                                               const shcore::Value &body,
                                               const Headers &headers) {
  return m_impl->execute_async(Type::POST, path, body, headers);
}

std::future<Response> Rest_service::async_put(const std::string &path,
                                              const shcore::Value &body,
                                              const Headers &headers) {
  return m_impl->execute_async(Type::PUT, path, body, headers);
}

std::future<Response> Rest_service::Rest_service::async_patch(
    const std::string &path, const shcore::Value &body,
    const Headers &headers) {
  return m_impl->execute_async(Type::PATCH, path, body, headers);
}

std::future<Response> Rest_service::async_delete(const std::string &path,
                                                 const shcore::Value &body,
                                                 const Headers &headers) {
  return m_impl->execute_async(Type::DELETE, path, body, headers);
}

}  // namespace rest
}  // namespace mysqlshdk
