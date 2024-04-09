/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/utils_x509.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace shcore {
namespace ssl {

namespace {

using BIO_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;

void throw_last_error(const std::string &context) {
  const auto rc = ERR_get_error();
  ERR_clear_error();

  std::string error_str = "Unexpected error";

  if (!context.empty()) error_str.append(1, ' ').append(context);

  error_str.append(": ").append(ERR_reason_error_string(rc));

  throw std::runtime_error(error_str);
}

std::string bio_mem_to_string(BIO *bio) {
  char *data = nullptr;
  size_t size = BIO_get_mem_data(bio, &data);

  return std::string{data, size};
}

std::time_t to_time_t(const ASN1_TIME *t) {
  std::unique_ptr<ASN1_TIME, decltype(&::ASN1_TIME_free)> epoch{
      ASN1_TIME_set(nullptr, 0), ::ASN1_TIME_free};

  if (!epoch) {
    throw_last_error("acquiring memory");
  }

  int days;
  int seconds;

  if (1 != ASN1_TIME_diff(&days, &seconds, epoch.get(), t)) {
    throw_last_error("computing time difference");
  }

  return static_cast<std::time_t>(60 * 60 * 24) * days + seconds;
}

}  // namespace

X509_certificate X509_certificate::from_string(std::string_view contents) {
  BIO_ptr bio{BIO_new_mem_buf(contents.data(), contents.length()), ::BIO_free};

  if (!bio) {
    throw_last_error("acquiring memory");
  }

  const auto cert_ptr = PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr);

  if (!cert_ptr) {
    throw_last_error("loading X.509 certificate");
  }

  return X509_certificate{{cert_ptr, ::X509_free}};
}

std::string X509_certificate::to_string() const {
  BIO_ptr bio{BIO_new(BIO_s_mem()), ::BIO_free};

  if (1 != PEM_write_bio_X509(bio.get(), ptr())) {
    throw_last_error("writing X.509 certificate");
  }

  return bio_mem_to_string(bio.get());
}

std::vector<X509_attribute> X509_certificate::subject() const {
#if OPENSSL_VERSION_NUMBER < 0x10100000L /* 1.1.x */
  static constexpr auto ASN1_STRING_get0_data = &::ASN1_STRING_data;
#endif

  const auto name = X509_get_subject_name(ptr());
  const auto count = X509_NAME_entry_count(name);
  std::vector<X509_attribute> result;
  result.resize(count);

  for (int i = 0; i < count; ++i) {
    const auto entry = X509_NAME_get_entry(name, i);
    const auto data = X509_NAME_ENTRY_get_data(entry);

    result[i].value =
        std::string{reinterpret_cast<const char *>(ASN1_STRING_get0_data(data)),
                    static_cast<std::size_t>(ASN1_STRING_length(data))};
    result[i].name = OBJ_nid2ln(OBJ_obj2nid(X509_NAME_ENTRY_get_object(entry)));
  }

  return result;
}

std::time_t X509_certificate::not_before() const {
#if OPENSSL_VERSION_NUMBER < 0x10100000L /* 1.1.x */
  static constexpr auto X509_get0_notBefore = [](const X509 *x) {
    return X509_get_notBefore(x);
  };
#endif

  return to_time_t(X509_get0_notBefore(ptr()));
}

std::time_t X509_certificate::not_after() const {
#if OPENSSL_VERSION_NUMBER < 0x10100000L /* 1.1.x */
  static constexpr auto X509_get0_notAfter = [](const X509 *x) {
    return X509_get_notAfter(x);
  };
#endif

  return to_time_t(X509_get0_notAfter(ptr()));
}

std::string X509_certificate::fingerprint(Hash hash) const {
  // NOTE: we're not using X509_digest() here, as it would prevent us from
  // using i.e. MD5 in FIPS mode
  std::string der;

  if (const auto size = i2d_X509(ptr(), nullptr); size < 0) {
    throw std::runtime_error("error converting certificate");
  } else {
    der.resize(size);
  }

  if (auto data = reinterpret_cast<unsigned char *>(der.data());
      i2d_X509(ptr(), &data) < 0) {
    throw std::runtime_error("error converting certificate");
  }

  auto fun = &sha256;

  switch (hash) {
    case Hash::RESTRICTED_MD5:
      fun = restricted::md5;
      break;

    case Hash::RESTRICTED_SHA1:
      fun = restricted::sha1;
      break;

    case Hash::SHA256:
      fun = sha256;
      break;
  }

  return to_fingerprint(fun(der));
}

}  // namespace ssl
}  // namespace shcore
