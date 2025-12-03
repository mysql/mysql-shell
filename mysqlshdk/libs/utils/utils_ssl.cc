/*
 * Copyright (c) 2019, 2025, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/utils_ssl.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/ssl.h>

#include <cassert>
#include <memory>
#include <mutex>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {
namespace ssl {
namespace {

struct SSL_init {
  std::once_flag flag;

  SSL_init() {
    std::call_once(flag, []() { SSL_library_init(); });
  }
} s_ssl_init;

namespace md {

template <Hash h>
constexpr inline const char *name() {
  if constexpr (Hash::SHA256 == h) {
    return "SHA256";
  } else if constexpr (Hash::RESTRICTED_SHA1 == h) {
    return "SHA1";
  } else if constexpr (Hash::RESTRICTED_MD5 == h) {
    return "MD5";
  }
}

template <Hash h>
constexpr inline auto ptr() {
  if constexpr (Hash::SHA256 == h) {
    return ::EVP_sha256();
  } else if constexpr (Hash::RESTRICTED_SHA1 == h) {
    return ::EVP_sha1();
  } else if constexpr (Hash::RESTRICTED_MD5 == h) {
    return ::EVP_md5();
  }
}

}  // namespace md

namespace fips {

template <Hash>
struct Config {};

template <>
struct Config<Hash::SHA256> {
  static constexpr bool disabled = false;
};

template <>
struct Config<Hash::RESTRICTED_SHA1> {
  static constexpr bool disabled = true;
};

template <>
struct Config<Hash::RESTRICTED_MD5> {
  static constexpr bool disabled = true;
};

namespace safe {

template <Hash h>
inline std::vector<unsigned char> hash(std::string_view data) {
  std::vector<unsigned char> md;
  md.resize(EVP_MAX_MD_SIZE);

  unsigned int size;

  if (1 != EVP_Digest(data.data(), data.size(), md.data(), &size, md::ptr<h>(),
                      nullptr)) {
    throw std::runtime_error(
        shcore::str_format("%s: error computing digest", md::name<h>()));
  }

  md.resize(size);

  return md;
}

template <Hash h>
inline std::vector<unsigned char> hmac(const std::vector<unsigned char> &key,
                                       std::string_view data) {
  std::vector<unsigned char> md;
  md.resize(EVP_MAX_MD_SIZE);

  unsigned int size;

  if (!HMAC(md::ptr<h>(), key.data(), key.size(),
            reinterpret_cast<const unsigned char *>(data.data()), data.size(),
            md.data(), &size)) {
    throw std::runtime_error(
        shcore::str_format("%s: error computing HMAC", md::name<h>()));
  }

  md.resize(size);

  return md;
}

}  // namespace safe

namespace unsafe {

template <Hash h>
inline std::vector<unsigned char> hash(std::string_view data) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L /* 1.1.x */
  static constexpr auto ctx_new = &::EVP_MD_CTX_create;
  static constexpr auto ctx_free = &::EVP_MD_CTX_destroy;
#else
  static constexpr auto ctx_new = &::EVP_MD_CTX_new;
  static constexpr auto ctx_free = &::EVP_MD_CTX_free;
#endif

  std::unique_ptr<EVP_MD_CTX, decltype(ctx_free)> ctx{ctx_new(), ctx_free};

  // allow hash in FIPS mode
#if OPENSSL_VERSION_NUMBER >= 0x30000000L /* 3.0.x */
  std::unique_ptr<EVP_MD, decltype(&::EVP_MD_free)> type_ptr{
      EVP_MD_fetch(nullptr, md::name<h>(), "fips=no"), ::EVP_MD_free};
  const auto type = type_ptr.get();
#else  // < 3.0.x
  const auto type = md::ptr<h>();
#ifdef EVP_MD_CTX_FLAG_NON_FIPS_ALLOW
  EVP_MD_CTX_set_flags(ctx.get(), EVP_MD_CTX_FLAG_NON_FIPS_ALLOW);
#endif  // EVP_MD_CTX_FLAG_NON_FIPS_ALLOW
#endif  // < 3.0.x

  if (1 != EVP_DigestInit_ex(ctx.get(), type, nullptr)) {
    throw std::runtime_error(
        shcore::str_format("%s: error initializing digest", md::name<h>()));
  }

  if (1 != EVP_DigestUpdate(ctx.get(), data.data(), data.length())) {
    throw std::runtime_error(
        shcore::str_format("%s: error updating digest", md::name<h>()));
  }

  std::vector<unsigned char> md;
  md.resize(EVP_MAX_MD_SIZE);

  unsigned int size;

  if (1 != EVP_DigestFinal_ex(ctx.get(), md.data(), &size)) {
    throw std::runtime_error(shcore::str_format(
        "%s: error completing digest operation", md::name<h>()));
  }

  md.resize(size);

  return md;
}

}  // namespace unsafe
}  // namespace fips

template <Hash h>
inline std::vector<unsigned char> hash(std::string_view data) {
  if constexpr (fips::Config<h>::disabled) {
    return fips::unsafe::hash<h>(data);
  } else {
    return fips::safe::hash<h>(data);
  }
}

template <Hash h>
inline std::vector<unsigned char> hmac(const std::vector<unsigned char> &key,
                                       std::string_view data) {
  static_assert(!fips::Config<h>::disabled,
                "This hash is not allowed in FIPS mode");

  return fips::safe::hmac<h>(key, data);
}

}  // namespace

std::vector<unsigned char> sha256(std::string_view data) {
  return hash<Hash::SHA256>(data);
}

namespace restricted {

std::vector<unsigned char> sha1(std::string_view data) {
  return hash<Hash::RESTRICTED_SHA1>(data);
}

std::vector<unsigned char> md5(std::string_view data) {
  return hash<Hash::RESTRICTED_MD5>(data);
}

}  // namespace restricted

std::vector<unsigned char> hmac_sha256(const std::vector<unsigned char> &key,
                                       std::string_view data) {
  return hmac<Hash::SHA256>(key, data);
}

std::string to_fingerprint(const std::vector<unsigned char> &hash) {
  if (hash.empty()) {
    return {};
  }

  std::string encoded;
  encoded.resize(3 * hash.size());

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
    *ptr++ = ':';
  }

  // remove last ':'
  encoded.pop_back();

  return encoded;
}

std::vector<unsigned long> openssl_error_stack() {
  std::vector<unsigned long> stack;

  const char *file;
  const char *data;
  int line;
  int flags;
  char error_string[256];

  while (const auto error_code =
             ERR_get_error_line_data(&file, &line, &data, &flags)) {
    ERR_error_string_n(error_code, error_string, sizeof(error_string));

    log_debug2("%s %s:%s:%d:%s", stack.empty() ? "OpenSSL" : "       ",
               error_string, file, line, (flags & ERR_TXT_STRING) ? data : "");

    stack.emplace_back(error_code);
  }

  if (stack.empty()) {
    // no errors
    stack.emplace_back(0);
  }

  return stack;
}

unsigned long openssl_error() { return openssl_error_stack().front(); }

}  // namespace ssl
}  // namespace shcore
