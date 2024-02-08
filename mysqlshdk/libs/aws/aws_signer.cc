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

#include "mysqlshdk/libs/aws/aws_signer.h"

#include <cassert>
#include <memory>
#include <utility>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/ssl_keygen.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace aws {

namespace {

const std::string k_empty_payload_hash =
    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

const std::string k_authorization_header = "Authorization";
const std::string k_host_header = "Host";
const std::string k_date_header = "x-amz-date";
const std::string k_hash_header = "x-amz-content-sha256";
const std::string k_token_header = "x-amz-security-token";

std::string hex(const std::vector<unsigned char> &hash) {
  std::string encoded;
  encoded.resize(2 * hash.size());

  auto ptr = encoded.data();

  static constexpr auto lut =
      "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20212223"
      "2425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f4041424344454647"
      "48494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b"
      "6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f"
      "909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3"
      "b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7"
      "d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafb"
      "fcfdfeff";

  for (const auto &byte : hash) {
    auto c = lut + (byte << 1);
    *ptr++ = *c++;
    *ptr++ = *c;
  }

  return encoded;
}

std::string hex_sha256(const char *data, size_t size) {
  return hex(shcore::ssl::sha256(data, size));
}

}  // namespace

Aws_signer::Aws_signer(const S3_bucket_config &config)
    : m_host(config.host()),
      m_region(config.region()),
      m_credentials_provider(config.credentials_provider()) {
  update_credentials();
}

bool Aws_signer::should_sign_request(const rest::Signed_request *) const {
  return !m_credentials->anonymous_access();
}

rest::Headers Aws_signer::sign_request(const rest::Signed_request *request,
                                       time_t now) const {
  using mysqlshdk::utils::fmttime;
  using mysqlshdk::utils::Time_type;
  // YYYYMMDDTHHMMSSZ
  const auto date = fmttime("%Y%m%dT%H%M%SZ", Time_type::GMT, &now);
  const auto short_date = date.substr(0, 8);

  // choose which headers to sign
  rest::Headers result;

  if (m_sign_all_headers) {
    result = request->unsigned_headers();
  } else {
    // only Host, Content-MD5, Content-Type, and headers beginning with
    // x-amz-* are included (Host is added below)
    for (const auto &header : request->unsigned_headers()) {
      if (shcore::str_caseeq(header.first, "Content-MD5") ||
          shcore::str_caseeq(header.first, "Content-Type") ||
          shcore::str_ibeginswith(header.first, "x-amz-")) {
        result.emplace(header);
      }
    }
  }

  // hash of the payload - Hex(SHA256Hash(<payload>)
  const auto &payload_hash = request->size
                                 ? hex_sha256(request->body, request->size)
                                 : k_empty_payload_hash;

  // add required headers
  result[k_host_header] = m_host;
  result[k_date_header] = date;
  result[k_hash_header] = payload_hash;

  if (const auto &token = m_credentials->session_token(); !token.empty()) {
    result[k_token_header] = token;
  }

  // CanonicalRequest = <HTTPMethod>\n
  //                    <CanonicalURI>\n
  //                    <CanonicalQueryString>\n
  //                    <CanonicalHeaders>\n
  //                    <SignedHeaders>\n
  //                    <HashedPayload>
  std::string canonical_request;
  canonical_request.reserve(512);

  // HTTPMethod
  canonical_request += shcore::str_upper(rest::type_name(request->type));
  canonical_request += '\n';

  // CanonicalURI & CanonicalQueryString
  const auto &path = request->full_path().real();
  const auto query_pos = path.find('?');

  assert('/' == path.front());

  if (std::string::npos == query_pos) {
    // no query string, empty CanonicalQueryString
    canonical_request += path;
    canonical_request += '\n';
    canonical_request += '\n';
  } else {
    // CanonicalURI - URI-encoded version of the absolute path component
    canonical_request += path.substr(0, query_pos);
    canonical_request += '\n';

    const auto query = path.substr(query_pos + 1);

    // CanonicalQueryString - URI-encoded query string parameters, sorted
    // alphabetically by key name
    canonical_request += query;

    if (std::string::npos == query.find('=')) {
      canonical_request += '=';
    }

    canonical_request += '\n';
  }

  std::string signed_headers;
  signed_headers.reserve(256);

  // CanonicalHeaders - list of request headers with their values, sorted
  // alphabetically by lower-case header name:
  //   Lowercase(<HeaderNameN>):Trim(<ValueN>)\n
  for (const auto &header : result) {
    const auto lower = shcore::str_lower(header.first);

    canonical_request += lower;
    canonical_request += ':';
    canonical_request += shcore::str_strip(header.second);
    canonical_request += '\n';

    signed_headers += lower;
    signed_headers += ';';
  }

  // remove last semicolon
  signed_headers.pop_back();
  // need to add a newline which separates CanonicalHeaders and SignedHeaders
  canonical_request += '\n';

  // SignedHeaders - alphabetically sorted, semicolon-separated list of
  // lower-case request header names
  canonical_request += signed_headers;
  canonical_request += '\n';

  // HashedPayload - Hex(SHA256Hash(<payload>)
  canonical_request += payload_hash;

  // Scope = <YYYYMMDD>/<region>/<service>/aws4_request
  std::string scope;
  scope.reserve(50);
  scope += short_date;
  scope += '/';
  scope += m_region;
  scope += '/';
  scope += m_service;
  scope += "/aws4_request";

  // StringToSign = AWS4-HMAC-SHA256\n
  //                <timeStampISO8601Format>\n
  //                <Scope>\n
  //                Hex(SHA256Hash(<CanonicalRequest>))
  std::string string_to_sign;
  string_to_sign.reserve(150);
  string_to_sign += "AWS4-HMAC-SHA256\n";
  string_to_sign += date;
  string_to_sign += '\n';
  string_to_sign += scope;
  string_to_sign += '\n';
  string_to_sign +=
      hex_sha256(canonical_request.c_str(), canonical_request.length());

  // calculate signature
  // DateKey = HMAC-SHA256("AWS4"+"<SecretAccessKey>", "<YYYYMMDD>")
  const auto date_key =
      shcore::ssl::hmac_sha256(m_secret_access_key, short_date);
  // DateRegionKey = HMAC-SHA256(<DateKey>, "<aws-region>")
  const auto date_region_key = shcore::ssl::hmac_sha256(date_key, m_region);
  // DateRegionServiceKey = HMAC-SHA256(<DateRegionKey>, "<aws-service>")
  const auto date_region_service_key =
      shcore::ssl::hmac_sha256(date_region_key, m_service);
  // SigningKey = HMAC-SHA256(<DateRegionServiceKey>, "aws4_request")
  const auto signing_key =
      shcore::ssl::hmac_sha256(date_region_service_key, "aws4_request");
  // signature = Hex(HMAC-SHA256(SigningKey, StringToSign))
  const auto signature =
      hex(shcore::ssl::hmac_sha256(signing_key, string_to_sign));

  // add Authorization header:
  // AWS4-HMAC-SHA256 Credential=<access key ID>/<Scope>,
  //   SignedHeaders=<SignedHeaders>, Signature=<signature>
  std::string authorization;
  authorization.reserve(512);
  authorization += "AWS4-HMAC-SHA256 Credential=";
  authorization += m_credentials->access_key_id();
  authorization += '/';
  authorization += scope;
  authorization += ", SignedHeaders=";
  authorization += signed_headers;
  authorization += ", Signature=";
  authorization += signature;

  result[k_authorization_header] = std::move(authorization);

  if (request->size) {
    result["Content-Length"] = std::to_string(request->size);
  }

  return result;
}

bool Aws_signer::refresh_auth_data() { return update_credentials(); }

bool Aws_signer::auth_data_expired(time_t now) const {
  return m_credentials->expired(now);
}

bool Aws_signer::is_authorization_error(const rest::Signed_request &request,
                                        const rest::Response &response) const {
  if (rest::Response::Status_code::BAD_REQUEST == response.status) {
    if (rest::Type::HEAD == request.type) {
      // if this was a HEAD request, then we won't get the body and the error
      // code, retry just in case, it's a lightweight request
      return true;
    }

    if (const auto error = response.get_error(); error.has_value()) {
      if ("ExpiredToken" == error->code() ||
          "TokenRefreshRequired" == error->code()) {
        // ExpiredToken: The provided token has expired.
        // TokenRefreshRequired: The provided token must be refreshed.
        return true;
      }
    }
  }

  return Signer::is_authorization_error(request, response);
}

bool Aws_signer::update_credentials() {
  if (auto credentials = m_credentials_provider->credentials()) {
    if (!m_credentials || *credentials != *m_credentials) {
      if (m_credentials) {
        log_info("The AWS credentials have been refreshed.");
      }

      set_credentials(std::move(credentials));
      return true;
    }
  }

  return false;
}

void Aws_signer::set_credentials(std::shared_ptr<Aws_credentials> credentials) {
  m_credentials = std::move(credentials);

  static constexpr auto prefix_size = 4;
  const auto &key = m_credentials->secret_access_key();

  m_secret_access_key.resize(prefix_size + key.size());

  memcpy(m_secret_access_key.data(), "AWS4", prefix_size);
  memcpy(m_secret_access_key.data() + prefix_size, key.c_str(), key.size());
}

}  // namespace aws
}  // namespace mysqlshdk
