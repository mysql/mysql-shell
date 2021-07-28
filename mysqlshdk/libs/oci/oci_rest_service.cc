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

#include "mysqlshdk/libs/oci/oci_rest_service.h"
#include "mysqlshdk/libs/oci/oci_setup.h"
#include "mysqlshdk/libs/rest/retry_strategy.h"
#include "mysqlshdk/libs/utils/ssl_keygen.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/shellcore/private_key_manager.h"

namespace mysqlshdk {
namespace oci {

std::string service_identifier(Oci_service service) {
  switch (service) {
    case Oci_service::IDENTITY:
      return "OCI-ID";
    case Oci_service::OBJECT_STORAGE:
      return "OCI-OS";
  }
  return "";
}

namespace {
std::string sign(EVP_PKEY *sigkey, const std::string &string_to_sign) {
// EVP_MD_CTX_create() and EVP_MD_CTX_destroy() were renamed to EVP_MD_CTX_new()
// and EVP_MD_CTX_free() in OpenSSL 1.1.
#if OPENSSL_VERSION_NUMBER >= 0x10100000L /* 1.1.x */
  std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_free)> mctx(
      EVP_MD_CTX_new(), ::EVP_MD_CTX_free);
#else
  std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_destroy)> mctx(
      EVP_MD_CTX_create(), ::EVP_MD_CTX_destroy);
#endif
  const EVP_MD *md = EVP_sha256();
  int r = EVP_DigestSignInit(mctx.get(), nullptr, md, nullptr, sigkey);
  if (r != 1) {
    throw std::runtime_error("Cannot setup signing context.");
  }

  const auto siglen = EVP_PKEY_size(sigkey);
  const auto md_value = std::make_unique<unsigned char[]>(siglen + 1);

  r = EVP_DigestSignUpdate(mctx.get(), string_to_sign.data(),
                           string_to_sign.size());
  if (r != 1) {
    throw std::runtime_error("Cannot hash data while signing request.");
  }

  size_t md_len = siglen;
  r = EVP_DigestSignFinal(mctx.get(), md_value.get(), &md_len);
  if (r != 1) {
    throw std::runtime_error("Cannot finalize signing data.");
  }

  std::string signature_b64;
  shcore::ssl::encode_base64(md_value.get(), md_len, &signature_b64);
  return signature_b64;
}

std::string encode_sha256(const char *data, size_t size) {
// EVP_MD_CTX_create() and EVP_MD_CTX_destroy() were renamed to EVP_MD_CTX_new()
// and EVP_MD_CTX_free() in OpenSSL 1.1.
#if OPENSSL_VERSION_NUMBER >= 0x10100000L /* 1.1.x */
  std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_free)> mctx(
      EVP_MD_CTX_new(), ::EVP_MD_CTX_free);
#else
  std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_destroy)> mctx(
      EVP_MD_CTX_create(), ::EVP_MD_CTX_destroy);
#endif
  const EVP_MD *md = EVP_sha256();
  int r = EVP_DigestInit_ex(mctx.get(), md, nullptr);
  if (r != 1) {
    throw std::runtime_error("SHA256: error initializing encoder.");
  }

  const auto md_value = std::make_unique<unsigned char[]>(EVP_MAX_MD_SIZE);

  r = EVP_DigestUpdate(mctx.get(), data, size);
  if (r != 1) {
    throw std::runtime_error("SHA256: error while encoding data.");
  }

  unsigned int md_len = EVP_MAX_MD_SIZE;
  r = EVP_DigestFinal_ex(mctx.get(), md_value.get(), &md_len);
  if (r != 1) {
    throw std::runtime_error("SHA256: error completing encode operation.");
  }

  std::string encoded;
  shcore::ssl::encode_base64(md_value.get(), md_len, &encoded);

  return encoded;
}

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

void log_failed_request(Type type, const std::string &path,
                        const Headers &headers,
                        const std::string &context = {}) {
  std::string full_context;

  if (!context.empty()) {
    full_context = " (" + context + ")";
  }

  log_warning("Request failed: %s %s%s",
              shcore::str_upper(type_name(type)).c_str(), path.c_str(),
              full_context.c_str());
  log_info("REQUEST HEADERS:\n%s", format_headers(headers).c_str());
}

void log_failed_response(const Headers &headers, Base_response_buffer *buffer) {
  log_info("RESPONSE HEADERS:\n%s", format_headers(headers).c_str());
  log_info("RESPONSE BODY:\n%s", buffer->size() ? buffer->data() : "<EMPTY>");
}

}  // namespace

Oci_rest_service::Oci_rest_service(Oci_service service,
                                   const Oci_options &options) {
  // NOTE: At this point we assume the options are valid, this is, the
  // configured profile exists on the indicated configuration file
  if (!options.get_par().is_null()) {
    m_region = *options.oci_region;
  } else {
    auto oci_profile = *options.config_profile;
    mysqlshdk::oci::Oci_setup oci_setup(*options.config_path);
    const auto config = oci_setup.get_cfg();

    auto key_file = *config.get(oci_profile, "key_file");
    m_tenancy_id = *config.get(oci_profile, "tenancy");
    auto user_id = *config.get(oci_profile, "user");
    auto fingerprint = *config.get(oci_profile, "fingerprint");
    m_region = *config.get(oci_profile, "region");

    auto passphrase = config.has_option(oci_profile, "pass_phrase")
                          ? *config.get(oci_profile, "pass_phrase")
                          : std::string{};

    auto k = shcore::Private_key_storage::get().contains(key_file);
    if (!k.second) {
      oci_setup.load_profile(oci_profile);
      // Check if load_profile opened successfully private key and put it into
      // private key storage.
      auto pkey = shcore::Private_key_storage::get().contains(key_file);
      if (!pkey.second) {
        throw std::runtime_error("Cannot load \"" + key_file +
                                 "\" private key associated with OCI "
                                 "configuration profile named \"" +
                                 oci_profile + "\".");
      } else {
        m_private_key = pkey.first->second;
      }
    } else {
      m_private_key = k.first->second;
    }

    shcore::clear_buffer(&passphrase);

    m_auth_keyId = m_tenancy_id + "/" + user_id + "/" + fingerprint;
  }

  m_service = service;
}

void Oci_rest_service::set_service(Oci_service service) {
  if (!m_rest || m_service != service) {
    switch (service) {
      case Oci_service::IDENTITY:
        m_host = "identity." + m_region + ".oraclecloud.com";
        break;
      case Oci_service::OBJECT_STORAGE:
        m_host = "objectstorage." + m_region + ".oraclecloud.com";
        break;
    }

    m_service = service;

    m_rest = std::make_unique<mysqlshdk::rest::Rest_service>(
        "https://" + m_host, true, service_identifier(service));
  }
}

Response::Status_code Oci_rest_service::get(const Masked_string &path,
                                            const Headers &headers,
                                            Base_response_buffer *buffer,
                                            Headers *response_headers,
                                            bool sign_request) {
  return execute(Type::GET, path, nullptr, 0L, headers, buffer,
                 response_headers, sign_request);
}

Response::Status_code Oci_rest_service::head(const Masked_string &path,
                                             const Headers &headers,
                                             Base_response_buffer *buffer,
                                             Headers *response_headers,
                                             bool sign_request) {
  return execute(Type::HEAD, path, nullptr, 0, headers, buffer,
                 response_headers, sign_request);
}

Response::Status_code Oci_rest_service::post(const Masked_string &path,
                                             const char *body, size_t size,
                                             const Headers &headers,
                                             Base_response_buffer *buffer,
                                             Headers *response_headers) {
  return execute(Type::POST, path, body, size, headers, buffer,
                 response_headers);
}

Response::Status_code Oci_rest_service::put(const Masked_string &path,
                                            const char *body, size_t size,
                                            const Headers &headers,
                                            Base_response_buffer *buffer,
                                            Headers *response_headers,
                                            bool sign_request) {
  return execute(Type::PUT, path, body, size, headers, buffer, response_headers,
                 sign_request);
}

Response::Status_code Oci_rest_service::delete_(const Masked_string &path,
                                                const char *body, size_t size,
                                                const Headers &headers) {
  return execute(Type::DELETE, path, body, size, headers);
}

Headers Oci_rest_service::make_header(Type method, const std::string &path,
                                      const char *body, size_t size,
                                      const Headers headers) {
  time_t now = time(nullptr);

  if (m_signed_header_cache_time.find(path) ==
      m_signed_header_cache_time.end()) {
    if (m_signed_header_cache_time[path].find(method) ==
        m_signed_header_cache_time[path].end()) {
      m_signed_header_cache_time[path][method] = 0;
    }
  }

  Headers all_headers;

  // Maximum Allowed Client Clock Skew from the server's clock for OCI
  // requests is 5 minutes. We can exploit that feature to cache auth header,
  // because it is expensive to calculate.
  //
  // PUT and POST requests are exceptions as the signature includes the body
  // sha256
  if (now - m_signed_header_cache_time[path][method] > 60 ||
      method == Type::POST) {
    const auto date = mysqlshdk::utils::fmttime(
        "%a, %d %b %Y %H:%M:%S GMT", mysqlshdk::utils::Time_type::GMT, &now);

    std::string string_to_sign{
        "(request-target): " + mysqlshdk::rest::type_name(method) + " " + path +
        "\nhost: " + m_host + "\nx-date: " + date};

    // Sets the content type to application/json if no other specified
    if (headers.find("content-type") != headers.end())
      all_headers["content-type"] = headers.at("content-type");
    else
      all_headers["content-type"] = "application/json";

    if (method == Type::POST) {
      all_headers["x-content-sha256"] = encode_sha256(size ? body : "", size);
      all_headers["content-length"] = std::to_string(size);
      string_to_sign.append(
          "\nx-content-sha256: " + all_headers["x-content-sha256"] +
          "\ncontent-length: " + all_headers["content-length"] +
          "\ncontent-type: " + all_headers["content-type"]);
    }

    const std::string signature_b64 = sign(m_private_key.get(), string_to_sign);
    std::string auth_header =
        "Signature version=\"1\",headers=\"(request-target) host x-date";

    if (method == Type::POST) {
      auth_header.append(" x-content-sha256 content-length content-type");
    }

    auth_header.append("\",keyId=\"" + m_auth_keyId +
                       "\",algorithm=\"rsa-sha256\",signature=\"" +
                       signature_b64 + "\"");

    m_signed_header_cache_time[path][method] = now;

    all_headers["authorization"] = auth_header;
    all_headers["x-date"] = date;

    m_cached_header[path][method] = all_headers;
  }

  auto final_headers = m_cached_header[path][method];

  // Adds any additional headers
  for (const auto &header : headers) {
    final_headers[header.first] = header.second;
  }

  return final_headers;
}

Response::Status_code Oci_rest_service::execute(
    Type type, const Masked_string &path, const char *body, size_t size,
    const Headers &request_headers, Base_response_buffer *buffer,
    Headers *response_headers, bool sign_request) {
  Headers rheaders;

  if (!m_rest) set_service(m_service);

  if (!response_headers) response_headers = &rheaders;

  auto retry_strategy = rest::default_retry_strategy();

  // Caller might not be interested on handling the response data directly, i.e.
  // if just expecting the call to either succeed or fail.
  // We need to ensure a response handler is in place in order to properly fail
  // on errors.
  mysqlshdk::rest::String_buffer response_data;
  Base_response_buffer *response_buffer_ptr = buffer;
  if (!response_buffer_ptr) response_buffer_ptr = &response_data;

  const auto &headers_to_send =
      sign_request ? make_header(type, path.real(), body, size, request_headers)
                   : request_headers;

  try {
    auto code =
        m_rest->execute(type, path, body, size, headers_to_send,
                        response_buffer_ptr, response_headers, &retry_strategy);

    mysqlshdk::rest::Response::check_and_throw(code, *response_headers,
                                               response_buffer_ptr->data(),
                                               response_buffer_ptr->size());

    return code;
  } catch (const rest::Connection_error &e) {
    // exception was thrown when connection was being established, there is no
    // response
    log_failed_request(type, path.masked(), headers_to_send,
                       format_exception(e));
    throw;
  } catch (const Response_error &e) {
    // exception was thrown after we got the response, log it as well
    log_failed_request(type, path.masked(), headers_to_send,
                       format_code(e.code()));
    log_failed_response(*response_headers, response_buffer_ptr);
    throw;
  } catch (const std::exception &e) {
    // we don't know when the exception was thrown, log response just in case
    log_failed_request(type, path.masked(), headers_to_send,
                       format_exception(e));
    log_failed_response(*response_headers, response_buffer_ptr);
    throw;
  }
}

}  // namespace oci
}  // namespace mysqlshdk
