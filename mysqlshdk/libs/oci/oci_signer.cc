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

#include "mysqlshdk/libs/oci/oci_signer.h"

#include <string_view>
#include <utility>

#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_encoding.h"
#include "mysqlshdk/libs/utils/utils_private_key.h"
#include "mysqlshdk/libs/utils/utils_ssl.h"

namespace mysqlshdk {
namespace oci {

namespace {

const std::string k_empty_payload_base64 =
    "47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=";

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
  shcore::encode_base64(md_value.get(), md_len, &signature_b64);
  return signature_b64;
}

std::string encode_sha256(std::string_view data) {
  const auto hash = shcore::ssl::sha256(data);
  std::string encoded;
  shcore::encode_base64(hash.data(), hash.size(), &encoded);

  return encoded;
}

}  // namespace

Oci_signer::Oci_signer(const Oci_signer_config &config)
    : Signer(config), m_host(config.host()) {}

rest::Headers Oci_signer::sign_request(const rest::Signed_request &request,
                                       time_t now) const {
  rest::Headers all_headers;
  const auto &path = request.full_path().real();
  const auto method = request.type;
  const auto &headers = request.unsigned_headers();

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

  if (method == rest::Type::POST) {
    all_headers["x-content-sha256"] =
        request.size ? encode_sha256({request.body, request.size})
                     : k_empty_payload_base64;
    all_headers["content-length"] = std::to_string(request.size);
    string_to_sign.append(
        "\nx-content-sha256: " + all_headers["x-content-sha256"] +
        "\ncontent-length: " + all_headers["content-length"] +
        "\ncontent-type: " + all_headers["content-type"]);
  }

  const std::string signature_b64 =
      sign(m_credentials->private_key().ptr(), string_to_sign);
  std::string auth_header =
      "Signature version=\"1\",headers=\"(request-target) host x-date";

  if (method == rest::Type::POST) {
    auth_header.append(" x-content-sha256 content-length content-type");
  }

  auth_header.append("\",keyId=\"" + m_credentials->auth_key_id() +
                     "\",algorithm=\"rsa-sha256\",signature=\"" +
                     signature_b64 + "\"");

  all_headers["authorization"] = std::move(auth_header);
  all_headers["x-date"] = std::move(date);

  return all_headers;
}

}  // namespace oci
}  // namespace mysqlshdk
