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

#include "mysqlshdk/libs/aws/aws_signer.h"

#include <openssl/hmac.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <utility>

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

#if OPENSSL_VERSION_NUMBER < 0x10100000L /* 1.1.x */

HMAC_CTX *HMAC_CTX_new() {
  HMAC_CTX *ctx = new HMAC_CTX();
  HMAC_CTX_init(ctx);
  return ctx;
}

void HMAC_CTX_free(HMAC_CTX *ctx) {
  HMAC_CTX_cleanup(ctx);
  delete ctx;
}

#endif  // OPENSSL_VERSION_NUMBER < 0x10100000L

std::vector<unsigned char> hmac_sha256(const std::vector<unsigned char> &key,
                                       const std::string &data) {
  std::unique_ptr<HMAC_CTX, decltype(&HMAC_CTX_free)> ctx(HMAC_CTX_new(),
                                                          HMAC_CTX_free);
  const auto md = EVP_sha256();

  if (!HMAC_Init_ex(ctx.get(), key.data(), key.size(), md, nullptr)) {
    throw std::runtime_error("Cannot initialize HMAC context.");
  }

  if (!HMAC_Update(ctx.get(),
                   reinterpret_cast<const unsigned char *>(data.data()),
                   data.size())) {
    throw std::runtime_error("Cannot update HMAC signature.");
  }

  std::vector<unsigned char> hash;
  unsigned int hash_len = EVP_MAX_MD_SIZE;
  hash.resize(hash_len);

  if (!HMAC_Final(ctx.get(), hash.data(), &hash_len)) {
    throw std::runtime_error("Cannot finalize HMAC signature.");
  }

  hash.resize(hash_len);

  return hash;
}

}  // namespace

Aws_signer::Aws_signer(const S3_bucket_config &config)
    : m_host(config.m_host),
      m_access_key_id(config.m_access_key_id),
      m_session_token(config.m_session_token),
      m_region(config.m_region) {
  if (config.m_secret_access_key) {
    set_secret_access_key(*config.m_secret_access_key);
  }
}

bool Aws_signer::should_sign_request(const rest::Signed_request *) const {
  return m_access_key_id && !m_secret_access_key.empty();
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

  if (m_session_token) {
    result[k_token_header] = *m_session_token;
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
  const auto &path = request->path().real();
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
  const auto date_key = hmac_sha256(m_secret_access_key, short_date);
  // DateRegionKey = HMAC-SHA256(<DateKey>, "<aws-region>")
  const auto date_region_key = hmac_sha256(date_key, m_region);
  // DateRegionServiceKey = HMAC-SHA256(<DateRegionKey>, "<aws-service>")
  const auto date_region_service_key = hmac_sha256(date_region_key, m_service);
  // SigningKey = HMAC-SHA256(<DateRegionServiceKey>, "aws4_request")
  const auto signing_key = hmac_sha256(date_region_service_key, "aws4_request");
  // signature = Hex(HMAC-SHA256(SigningKey, StringToSign))
  const auto signature = hex(hmac_sha256(signing_key, string_to_sign));

  // add Authorization header:
  // AWS4-HMAC-SHA256 Credential=<access key ID>/<Scope>,
  //   SignedHeaders=<SignedHeaders>, Signature=<signature>
  std::string authorization;
  authorization.reserve(512);
  authorization += "AWS4-HMAC-SHA256 Credential=";
  authorization += *m_access_key_id;
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

void Aws_signer::set_secret_access_key(const std::string &key) {
  static constexpr auto prefix_size = 4;

  m_secret_access_key.resize(prefix_size + key.size());

  memcpy(m_secret_access_key.data(), "AWS4", prefix_size);
  memcpy(m_secret_access_key.data() + prefix_size, key.c_str(), key.size());
}

}  // namespace aws
}  // namespace mysqlshdk
