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

#include "mysqlshdk/libs/rest/rest_service.h"

#include <curl/curl.h>
#include <utility>
#include <vector>

#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/rest/retry_strategy.h"
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

        // if `begin` is std::string::npos, header has an empty value
        headers[it->substr(0, colon)] = std::string::npos != begin
                                            ? it->substr(begin, end - begin + 1)
                                            : "";
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

std::string format_headers(const Headers &headers) {
  std::string result;

  if (headers.empty()) {
    result = "<EMPTY>";
  } else {
    result = "  " + shcore::str_join(headers, "\n  ", [](const auto &header) {
               return "'" + header.first + "' : '" + header.second + "'";
             });
  }

  return result;
}

std::string format_code(Response::Status_code code) {
  return std::to_string(static_cast<int>(code)) + " - " +
         Response::status_code(code);
}

std::string format_exception(const std::exception &e) {
  return std::string{"Exception: "} + e.what();
}

void log_failed_request(const std::string &base_url, const Request &request,
                        const std::string &context = {}) {
  std::string full_context;

  if (!context.empty()) {
    full_context = " (" + context + ")";
  }

  log_warning("Request failed: %s %s %s%s", base_url.c_str(),
              shcore::str_upper(type_name(request.type)).c_str(),
              request.path().masked().c_str(), full_context.c_str());
  log_info("REQUEST HEADERS:\n%s", format_headers(request.headers()).c_str());
}

void log_failed_response(const Response &response) {
  log_info("RESPONSE HEADERS:\n%s", format_headers(response.headers).c_str());
  log_info("RESPONSE BODY:\n%s", response.body && response.body->size()
                                     ? response.body->data()
                                     : "<EMPTY>");
}

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
  Impl(const Masked_string &base_url, bool verify, const std::string &label)
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

  void log_request(const Request &request) {
    if (shcore::current_logger()->get_log_level() >=
        shcore::Logger::LOG_LEVEL::LOG_DEBUG) {
      log_debug("%s-%d: %s %s %s", m_id.c_str(), m_request_sequence,
                m_base_url.masked().c_str(),
                shcore::str_upper(type_name(request.type)).c_str(),
                request.path().masked().c_str());

      if (shcore::current_logger()->get_log_level() >=
          shcore::Logger::LOG_LEVEL::LOG_DEBUG2) {
        log_debug2("%s-%d: REQUEST HEADERS:\n  %s", m_id.c_str(),
                   m_request_sequence,
                   format_headers(request.headers()).c_str());
      }
    }
  }

  void log_response(int sequence, Response::Status_code code,
                    const std::string &headers) {
    if (shcore::current_logger()->get_log_level() >=
        shcore::Logger::LOG_LEVEL::LOG_DEBUG) {
      log_debug("%s-%d: %d-%s", m_id.c_str(), sequence, static_cast<int>(code),
                Response::status_code(code).c_str());

      if (!headers.empty()) {
        log_debug2("%s-%d: RESPONSE HEADERS:\n  %s", m_id.c_str(), sequence,
                   headers.c_str());
      }
    }
  }

  Response::Status_code execute(bool synch, Request *request,
                                Response *response = nullptr) {
    assert(request);

    m_request_sequence++;

    log_request(*request);

    set_url(request->path().real());
    // body needs to be set before the type, because it implicitly sets type
    // to POST
    set_body(request->body, request->size, synch);
    set_type(request->type);
    const auto headers_deleter =
        set_headers(request->headers(), request->size != 0);

    // set callbacks which will receive the response
    std::string header_data;
    curl_easy_setopt(m_handle.get(), CURLOPT_HEADERDATA, &header_data);
    curl_easy_setopt(m_handle.get(), CURLOPT_WRITEDATA,
                     response ? response->body : nullptr);

    // execute the request
    auto ret_val = curl_easy_perform(m_handle.get());
    if (ret_val != CURLE_OK) {
      log_error("%s-%d: %s (CURLcode = %i)", m_id.c_str(), m_request_sequence,
                m_error_buffer, ret_val);
      throw Connection_error{m_error_buffer};
    }

    const auto status = get_status_code();

    log_response(m_request_sequence, status, header_data);

    if (response) {
      response->status = status;
      response->headers = parse_headers(header_data);
    }

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

  std::string get_last_request_id() const {
    return m_id + "-" + std::to_string(m_request_sequence);
  }

  const Masked_string &base_url() const { return m_base_url; }

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
    curl_easy_setopt(m_handle.get(), CURLOPT_URL,
                     (m_base_url.real() + path).c_str());
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

  Masked_string m_base_url;

  Headers m_default_headers;

  std::string m_id;

  int m_request_sequence;

  long m_default_timeout;
};

Rest_service::Rest_service(const Masked_string &base_url, bool verify_ssl,
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

String_response Rest_service::get(Request *request) {
  request->type = Type::GET;
  request->body = nullptr;
  request->size = 0;
  return execute_internal(request);
}

String_response Rest_service::head(Request *request) {
  request->type = Type::HEAD;
  request->body = nullptr;
  request->size = 0;
  return execute_internal(request);
}

String_response Rest_service::post(Request *request) {
  request->type = Type::POST;
  return execute_internal(request);
}

String_response Rest_service::put(Request *request) {
  request->type = Type::PUT;
  return execute_internal(request);
}

String_response Rest_service::patch(Request *request) {
  request->type = Type::PATCH;
  return execute_internal(request);
}

String_response Rest_service::delete_(Request *request) {
  request->type = Type::DELETE;
  return execute_internal(request);
}

String_response Rest_service::execute_internal(Request *request) {
  String_response response;

  execute(request, &response);

  return response;
}

Response::Status_code Rest_service::execute(Request *request,
                                            Response *response) {
  const auto retry_strategy = request->retry_strategy;

  if (retry_strategy) {
    retry_strategy->init();
  }

  const auto retry = [&response, &retry_strategy, this](const char *msg) {
    if (response && response->body) response->body->clear();
    retry_strategy->wait_for_retry();
    // this log is to have visibility of the error
    log_info("RETRYING %s: %s", m_impl->get_last_request_id().c_str(), msg);
  };

  const auto &base_url = m_impl->base_url().masked();

  while (true) {
    try {
      const auto code = m_impl->execute(true, request, response);
      std::optional<Response_error> error;

      if (response) {
        error = response->get_error();
      }

      if (retry_strategy && retry_strategy->should_retry(
                                code, error ? error.value().what() : "")) {
        // A retriable error occurred
        retry(shcore::str_format("%d-%s", static_cast<int>(code),
                                 Response::status_code(code).c_str())
                  .c_str());
      } else {
        if (Response::is_error(code)) {
          // response was an error, log it as well
          log_failed_request(base_url, *request, format_code(code));
          if (response) log_failed_response(*response);
        }

        return code;
      }
    } catch (const rest::Connection_error &error) {
      // exception was thrown when connection was being established, there is
      // no response; this is not a recoverable error, we do not retry here
      log_failed_request(base_url, *request, format_exception(error));
      throw;
    } catch (const std::exception &error) {
      if (retry_strategy && retry_strategy->should_retry()) {
        // A unexpected error occurred but the retry strategy indicates we
        // should retry
        retry(error.what());
      } else {
        // we don't know when the exception was thrown, log response just in
        // case
        log_failed_request(base_url, *request, format_exception(error));
        if (response) log_failed_response(*response);
        throw;
      }
    }
  }
}

}  // namespace rest
}  // namespace mysqlshdk
