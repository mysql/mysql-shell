/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_DB_SSL_OPTIONS_H_
#define MYSQLSHDK_LIBS_DB_SSL_OPTIONS_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "mysqlshdk/include/mysqlshdk_export.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/nullable_options.h"

namespace mysqlshdk {
namespace db {
using mysqlshdk::utils::nullable_options::Comparison_mode;
using mysqlshdk::utils::nullable_options::Set_mode;

enum class SHCORE_PUBLIC Ssl_mode {
  Disabled = 1,
  Preferred = 2,
  Required = 3,
  VerifyCa = 4,
  VerifyIdentity = 5
};

struct SHCORE_PUBLIC Ssl_options : public mysqlshdk::utils::Nullable_options {
  explicit Ssl_options(
      Comparison_mode mode = Comparison_mode::CASE_INSENSITIVE);

  bool has_data() const;
  bool has_mode() const { return has_value(kSslMode); }
  bool has_ca() const { return has_value(kSslCa); }
  bool has_capath() const { return has_value(kSslCaPath); }
  bool has_cert() const { return has_value(kSslCert); }
  bool has_key() const { return has_value(kSslKey); }
  bool has_crl() const { return has_value(kSslCrl); }
  bool has_crlpath() const { return has_value(kSslCrlPath); }
  bool has_cipher() const { return has_value(kSslCipher); }
  bool has_tls_version() const { return has_value(kSslTlsVersion); }
  bool has_tls_ciphersuites() const { return has_value(kSslTlsCiphersuites); }

  Ssl_mode get_mode() const;
  std::string get_mode_name() const;
  const std::string &get_ca() const { return _get(kSslCa); }
  const std::string &get_capath() const { return _get(kSslCaPath); }
  const std::string &get_cert() const { return _get(kSslCert); }
  const std::string &get_key() const { return _get(kSslKey); }
  const std::string &get_crl() const { return _get(kSslCrl); }
  const std::string &get_crlpath() const { return _get(kSslCrlPath); }
  const std::string &get_cipher() const { return _get(kSslCipher); }
  const std::string &get_tls_version() const { return _get(kSslTlsVersion); }
  const std::string &get_tls_ciphersuites() const {
    return _get(kSslTlsCiphersuites);
  }

  void clear_mode() { clear_value(kSslMode); }
  void clear_ca() { clear_value(kSslCa); }
  void clear_capath() { clear_value(kSslCaPath); }
  void clear_cert() { clear_value(kSslCert); }
  void clear_key() { clear_value(kSslKey); }
  void clear_crl() { clear_value(kSslCrl); }
  void clear_crlpath() { clear_value(kSslCrlPath); }
  void clear_cipher() { clear_value(kSslCipher); }
  void clear_tls_version() { clear_value(kSslTlsVersion); }
  void clear_tls_ciphersuites() { clear_value(kSslTlsCiphersuites); }

  void set_mode(Ssl_mode value);
  void set_ca(const std::string &value);
  void set_capath(const std::string &value);
  void set_crl(const std::string &value);
  void set_crlpath(const std::string &value);
  void set_cipher(const std::string &value);
  void set_tls_version(const std::string &value);
  void set_tls_ciphersuites(const std::string &value);
  void set_cert(const std::string &value);
  void set_key(const std::string &value);

  void set(const std::string &name, const std::string &value);
  void remove(const std::string &name);
  void validate() const;

  static constexpr const char *const option_str_list[] = {
      kSslCa,     kSslCaPath,     kSslCert,
      kSslKey,    kSslCrl,        kSslCrlPath,
      kSslCipher, kSslTlsVersion, kSslTlsCiphersuites,
      kSslMode};

 private:
  const std::string &_get(const std::string &attribute) const;
};
}  // namespace db
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_DB_SSL_OPTIONS_H_
