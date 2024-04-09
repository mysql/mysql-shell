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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_X509_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_X509_H_

#include <ctime>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/utils/utils_ssl.h"

struct x509_st;
using X509 = x509_st;

namespace shcore {
namespace ssl {

/**
 * Native representation of a X.509 certificate.
 */
using Native_x509 = std::shared_ptr<X509>;

/**
 * A X.509 attribute.
 */
struct X509_attribute {
  /**
   * Long name of the attribute.
   */
  std::string name;

  /**
   * Attribute's value.
   */
  std::string value;
};

/**
 * A X.509 certificate.
 */
class X509_certificate {
 public:
  X509_certificate(const X509_certificate &) = default;
  X509_certificate(X509_certificate &&) = default;

  X509_certificate &operator=(const X509_certificate &) = default;
  X509_certificate &operator=(X509_certificate &&) = default;

  ~X509_certificate() = default;

  /**
   * Loads a certificate from a string representation.
   *
   * @param contents A string representation of a X.509 certificate.
   */
  static X509_certificate from_string(std::string_view contents);

  /**
   * Native handle to the certificate.
   */
  [[nodiscard]] X509 *ptr() const noexcept { return m_cert.get(); }

  /**
   * Provides a string representation of this certificate.
   */
  std::string to_string() const;

  /**
   * Provides attributes associated with the subject line.
   */
  std::vector<X509_attribute> subject() const;

  /**
   * Valid not before.
   */
  std::time_t not_before() const;

  /**
   * Valid not after.
   */
  std::time_t not_after() const;

  /**
   * Provides fingerprint of this certificate.
   *
   * @param hash Hash to use
   */
  std::string fingerprint(Hash hash = Hash::RESTRICTED_SHA1) const;

 private:
  explicit X509_certificate(Native_x509 cert) : m_cert(std::move(cert)) {}

  Native_x509 m_cert;
};

}  // namespace ssl
}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_X509_H_
