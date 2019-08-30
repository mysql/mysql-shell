/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

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

size_t response_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
  const auto data = static_cast<std::string *>(userdata);
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

}  // namespace

class Rest_service::Impl {
 public:
  /**
   * Type of the HTTP request.
   */
  enum class Type { GET, HEAD, POST, PUT, PATCH, DELETE };

  Impl(const std::string &base_url, bool verify)
      : m_handle(curl_easy_init(), &curl_easy_cleanup), m_base_url{base_url} {
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
    // set timeout to 2 seconds
    set_timeout(2000);

    // set the callbacks
    curl_easy_setopt(m_handle.get(), CURLOPT_READDATA, nullptr);
    curl_easy_setopt(m_handle.get(), CURLOPT_READFUNCTION, request_callback);
    curl_easy_setopt(m_handle.get(), CURLOPT_WRITEFUNCTION, response_callback);
  }

  ~Impl() = default;

  Response execute(Type type, const std::string &path,
                   const shcore::Value &body, const Headers &headers) {
    set_url(path);
    // body needs to be set before the type, because it implicitly sets type to
    // POST
    set_body(body);
    set_type(type);
    const auto headers_deleter =
        set_headers(headers, body.type != shcore::Value_type::Undefined);

    // set callbacks which will receive the response
    std::string response_headers;
    std::string response_body;

    curl_easy_setopt(m_handle.get(), CURLOPT_HEADERDATA, &response_headers);
    curl_easy_setopt(m_handle.get(), CURLOPT_WRITEDATA, &response_body);

    // execute the request
    if (curl_easy_perform(m_handle.get()) != CURLE_OK) {
      throw Connection_error{m_error_buffer};
    }

    return get_raw_response(response_headers, std::move(response_body));
  }

  std::future<Response> execute_async(Type type, const std::string &path,
                                      const shcore::Value &body,
                                      const Headers &headers) {
    return std::async(std::launch::async, [this, type, path, body, headers]() {
      return execute(type, path, body, headers);
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

  void set_timeout(uint32_t timeout) {
    curl_easy_setopt(m_handle.get(), CURLOPT_TIMEOUT_MS,
                     static_cast<long>(timeout));
  }

 private:
  void verify_ssl(bool verify) {
    curl_easy_setopt(m_handle.get(), CURLOPT_SSL_VERIFYHOST, verify ? 2L : 0L);
    curl_easy_setopt(m_handle.get(), CURLOPT_SSL_VERIFYPEER, verify ? 1L : 0L);
  }

  void set_type(Type type) {
    // custom request overwrites any other option, make sure it's set to default
    curl_easy_setopt(m_handle.get(), CURLOPT_CUSTOMREQUEST, nullptr);

    switch (type) {
      case Type::GET:
        curl_easy_setopt(m_handle.get(), CURLOPT_HTTPGET, 1L);
        break;

      case Type::HEAD:
        curl_easy_setopt(m_handle.get(), CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(m_handle.get(), CURLOPT_NOBODY, 1L);
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
        break;
    }
  }

  void set_url(const std::string &path) {
    curl_easy_setopt(m_handle.get(), CURLOPT_URL, (m_base_url + path).c_str());
  }

  void set_body(const shcore::Value &body) {
    if (body.type != shcore::Value_type::Undefined) {
      // set the body
      const auto request_body = body.repr();
      curl_easy_setopt(m_handle.get(), CURLOPT_POSTFIELDSIZE,
                       request_body.length());
      curl_easy_setopt(m_handle.get(), CURLOPT_COPYPOSTFIELDS,
                       request_body.c_str());
    } else {
      // no body, remove post data (if present)
      curl_easy_setopt(m_handle.get(), CURLOPT_POSTFIELDSIZE, -1L);
      curl_easy_setopt(m_handle.get(), CURLOPT_COPYPOSTFIELDS, nullptr);
    }
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

  Response get_raw_response(const std::string &headers, std::string &&body) {
    // fill in the response object
    Response response;

    {
      long response_code = 0;
      curl_easy_getinfo(m_handle.get(), CURLINFO_RESPONSE_CODE, &response_code);
      response.status = static_cast<Response::Status_code>(response_code);
    }

    response.headers = parse_headers(headers);
    response.body = shcore::Value(std::move(body));

    return response;
  }

  std::unique_ptr<CURL, void (*)(CURL *)> m_handle;

  char m_error_buffer[CURL_ERROR_SIZE];

  std::string m_base_url;

  Headers m_default_headers;
};

Rest_service::Rest_service(const std::string &base_url, bool verify_ssl)
    : m_impl(shcore::make_unique<Impl>(base_url, verify_ssl)) {}

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

Rest_service &Rest_service::set_timeout(uint32_t timeout) {
  m_impl->set_timeout(timeout);
  return *this;
}

Response Rest_service::get(const std::string &path, const Headers &headers) {
  return m_impl->execute(Impl::Type::GET, path, {}, headers);
}

Response Rest_service::head(const std::string &path, const Headers &headers) {
  return m_impl->execute(Impl::Type::HEAD, path, {}, headers);
}

Response Rest_service::post(const std::string &path, const shcore::Value &body,
                            const Headers &headers) {
  return m_impl->execute(Impl::Type::POST, path, body, headers);
}

Response Rest_service::put(const std::string &path, const shcore::Value &body,
                           const Headers &headers) {
  return m_impl->execute(Impl::Type::PUT, path, body, headers);
}

Response Rest_service::patch(const std::string &path, const shcore::Value &body,
                             const Headers &headers) {
  return m_impl->execute(Impl::Type::PATCH, path, body, headers);
}

Response Rest_service::delete_(const std::string &path,
                               const shcore::Value &body,
                               const Headers &headers) {
  return m_impl->execute(Impl::Type::DELETE, path, body, headers);
}

std::future<Response> Rest_service::async_get(const std::string &path,
                                              const Headers &headers) {
  return m_impl->execute_async(Impl::Type::GET, path, {}, headers);
}

std::future<Response> Rest_service::async_head(const std::string &path,
                                               const Headers &headers) {
  return m_impl->execute_async(Impl::Type::HEAD, path, {}, headers);
}

std::future<Response> Rest_service::async_post(const std::string &path,
                                               const shcore::Value &body,
                                               const Headers &headers) {
  return m_impl->execute_async(Impl::Type::POST, path, body, headers);
}

std::future<Response> Rest_service::async_put(const std::string &path,
                                              const shcore::Value &body,
                                              const Headers &headers) {
  return m_impl->execute_async(Impl::Type::PUT, path, body, headers);
}

std::future<Response> Rest_service::Rest_service::async_patch(
    const std::string &path, const shcore::Value &body,
    const Headers &headers) {
  return m_impl->execute_async(Impl::Type::PATCH, path, body, headers);
}

std::future<Response> Rest_service::async_delete(const std::string &path,
                                                 const shcore::Value &body,
                                                 const Headers &headers) {
  return m_impl->execute_async(Impl::Type::DELETE, path, body, headers);
}

}  // namespace rest
}  // namespace mysqlshdk
