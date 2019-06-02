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

#include "modules/util/import_table/file_backends/oci_object_storage.h"

#include <openssl/err.h>
#include <openssl/pem.h>
#include <algorithm>
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/rest/rest_service.h"
#include "mysqlshdk/libs/utils/ssl_keygen.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/shellcore/private_key_manager.h"

#ifdef WITH_OCI
#include "modules/util/oci_setup.h"
#else
#include "mysqlshdk/libs/config/config_file.h"
#endif

using Rest_service = mysqlshdk::rest::Rest_service;
using Headers = mysqlshdk::rest::Headers;
using Response = mysqlshdk::rest::Response;
using BIO_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;

namespace mysqlsh {
namespace import_table {

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
  std::unique_ptr<unsigned char[]> md_value{new unsigned char[siglen + 1]};

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
}  // namespace

void Oci_object_storage::parse_uri(const std::string &uri) {
  const auto parts = shcore::str_split(uri, "/");
  if (parts.size() != 6 || !shcore::str_beginswith(uri, "oci+os://")) {
    throw std::runtime_error(
        "Invalid URI. Use oci+os://namespace/region/bucket/object pattern.");
  }
  m_uri.region = parts[2];
  m_uri.tenancy = parts[3];
  m_uri.bucket = parts[4];
  m_uri.object = parts[5];

  m_uri.hostname = "objectstorage." + m_uri.region + ".oraclecloud.com";
  m_uri.path =
      "/n/" + m_uri.tenancy + "/b/" + m_uri.bucket + "/o/" + m_uri.object;
}

Oci_object_storage::Oci_object_storage(const std::string &uri)
    : m_oci_uri(uri) {
  parse_uri(uri);
  m_rest = shcore::make_unique<Rest_service>("https://" + m_uri.hostname, true);
  m_rest->set_timeout(30000);  // todo(kg): default 2s was not enough, 30s is
                               // ok? maybe we could make it configurable

  const auto oci_config_path =
      mysqlsh::current_shell_options()->get().oci_config_file;
  const auto oci_profile = mysqlsh::current_shell_options()->get().oci_profile;

#ifdef WITH_OCI
  oci::Oci_setup oci_setup(oci_config_path);
  bool has_profile = oci_setup.has_profile(oci_profile);
  const auto config = oci_setup.get_cfg();
#else
  mysqlshdk::config::Config_file config(mysqlshdk::config::Case::SENSITIVE,
                                        mysqlshdk::config::Escape::NO);
  config.read(oci_config_path);
  bool has_profile = config.has_group(oci_profile);
#endif
  if (!has_profile) {
    throw std::runtime_error("Profile named \"" + oci_profile +
                             "\" does not exists in \"" + oci_config_path +
                             "\" OCI configuration file.");
  }
  m_key_file = *config.get(oci_profile, "key_file");
  m_tenancy_id = *config.get(oci_profile, "tenancy");
  m_user_id = *config.get(oci_profile, "user");
  m_fingerprint = *config.get(oci_profile, "fingerprint");

  std::string passphrase = config.has_option(oci_profile, "pass_phrase")
                               ? *config.get(oci_profile, "pass_phrase")
                               : std::string{};

  auto k = shcore::Private_key_storage::get().contains(m_key_file);
  if (!k.second) {
#ifdef WITH_OCI
    oci_setup.load_profile(oci_profile);
    // Check if load_profile opened successfully private key and put it into
    // private key storage.
    auto pkey = shcore::Private_key_storage::get().contains(m_key_file);
    if (!pkey.second) {
      throw std::runtime_error(
          "Cannot load \"" + m_key_file +
          "\" private key associated with OCI configuration profile named \"" +
          oci_profile + "\".");
    } else {
      m_private_key = pkey.first->second;
    }
#else
    m_private_key =
        shcore::Private_key_storage::get().from_file(m_key_file, passphrase);
#endif
  } else {
    m_private_key = k.first->second;
  }

  shcore::clear_buffer(&passphrase);

  m_auth_keyId = m_tenancy_id + "/" + m_user_id + "/" + m_fingerprint;

  auto extract_from_ok_response = [&](mysqlshdk::rest::Response *response) {
    m_file_size = std::stoul(response->headers["content-length"]);
    m_open_status_code = response->status;
    if (response->headers["Accept-Ranges"] != "bytes") {
      throw std::runtime_error(
          "Target server does not support partial requests.");
    }
  };
  // Wrongly signed headers disallow access to public buckets, so firstly we
  // check if bucket is public by making an anonymous request (I didn't found
  // OCI SDK API call that returns bucket type).
  is_public_bucket = [&]() -> bool {
    // Anonymous access to private buckets returns 404 BucketNotFound, while bad
    // signed headers to public buckets returns 401 NotAuthorized
    auto anonymous = m_rest->head(m_uri.path);
    if (anonymous.status == Response::Status_code::OK) {
      extract_from_ok_response(&anonymous);
      return true;
    }
    // bucket either do not exists or is private
    return false;
  }();

  if (!is_public_bucket) {
    Headers h = make_signed_header(false);
    auto response = m_rest->head(m_uri.path, h);
    if (response.status == Response::Status_code::OK) {
      extract_from_ok_response(&response);
    } else {
      Headers get_headers = make_signed_header();
      auto get_response = m_rest->get(m_uri.path, get_headers);
      const auto &json = get_response.json().as_map();
      const int http_status_code = static_cast<int>(get_response.status);
      const std::string error_msg = std::to_string(http_status_code) + " " +
                                    json->get_string("code") + ": " +
                                    json->get_string("message");
      throw std::runtime_error(error_msg);
    }
  }
}

Oci_object_storage::~Oci_object_storage() { shcore::clear_buffer(&m_key_file); }

void Oci_object_storage::open() { m_offset = 0; }

bool Oci_object_storage::is_open() {
  return m_open_status_code == Response::Status_code::OK;
}

void Oci_object_storage::close() {}

size_t Oci_object_storage::file_size() { return m_file_size; }

std::string Oci_object_storage::file_name() { return m_oci_uri; }

off64_t Oci_object_storage::seek(off64_t offset) {
  const off64_t fsize = file_size();
  m_offset = std::min(offset, fsize);
  return m_offset;
}

ssize_t Oci_object_storage::read(void *buffer, size_t length) {
  if (!(length > 0)) return 0;
  const off64_t fsize = file_size();
  if (m_offset >= fsize) return 0;

  const size_t first = m_offset;
  const size_t last_unbounded = m_offset + length - 1;
  // http range request is both sides inclusive
  const size_t last = std::min(file_size() - 1, last_unbounded);
  const std::string range =
      "bytes=" + std::to_string(first) + "-" + std::to_string(last);

  Headers h = make_header();
  h["range"] = range;

  auto response = m_rest->get(m_uri.path, h);

  if (Response::Status_code::PARTIAL_CONTENT == response.status) {
    const auto &content = response.body.get_string();
    std::copy(content.data(), content.data() + content.size(),
              reinterpret_cast<uint8_t *>(buffer));
    m_offset += content.size();
    return content.size();
  } else if (Response::Status_code::OK == response.status) {
    throw std::runtime_error("Range requests are not supported.");
  } else if (Response::Status_code::RANGE_NOT_SATISFIABLE == response.status) {
    throw std::runtime_error("Range request " + std::to_string(first) + "-" +
                             std::to_string(last) + " is out of bounds.");
  }
  return 0;
}

mysqlshdk::rest::Headers Oci_object_storage::make_signed_header(
    bool is_get_request) {
  time_t now = time(nullptr);
  const auto idx = is_get_request ? 1 : 0;

  // Maximum Allowed Client Clock Skew from the server's clock for OCI requests
  // is 5 minutes. We can exploit that feature to cache auth header, because it
  // is expensive to calculate.
  if (now - m_signed_header_cache_time[idx] > 60) {
    const auto date = mysqlshdk::utils::fmttime(
        "%a, %d %b %Y %H:%M:%S GMT", mysqlshdk::utils::Time_type::GMT, &now);
    const std::string string_to_sign{
        "x-date: " + date +
        "\n(request-target): " + (is_get_request ? "get " : "head ") +
        m_uri.path + "\nhost: " + m_uri.hostname};
    const std::string signature_b64 = sign(m_private_key.get(), string_to_sign);
    const std::string auth_header =
        "Signature version=\"1\",headers=\"x-date (request-target) "
        "host\",keyId=\"" +
        m_auth_keyId + "\",algorithm=\"rsa-sha256\",signature=\"" +
        signature_b64 + "\"";
    m_signed_header_cache_time[idx] = now;
    m_cached_header[idx] =
        Headers{{"authorization", auth_header}, {"x-date", date}};
  }

  return m_cached_header[idx];
}

mysqlshdk::rest::Headers Oci_object_storage::make_header(bool is_get_request) {
  if (is_public_bucket) {
    return {};
  }
  return make_signed_header(is_get_request);
}

}  // namespace import_table
}  // namespace mysqlsh
