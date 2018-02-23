/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/db/ssl_options.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace db {
using mysqlshdk::utils::nullable_options::Set_mode;
using mysqlshdk::utils::nullable_options::Comparison_mode;

constexpr const char* Ssl_options::option_str_list[];

Ssl_options::Ssl_options(Comparison_mode mode)
    : Nullable_options(mode, "SSL Connection") {
  for (auto o : option_str_list)
    Nullable_options::set(o, nullptr, Set_mode::CREATE);
}

bool Ssl_options::has_data() const {
  for (auto o : option_str_list) {
    if (has_value(o))
      return true;
  }

  return false;
}

void Ssl_options::set_mode(Ssl_mode value) {
  std::string str_mode =
    MapSslModeNameToValue::get_value(static_cast<int>(value));

  Nullable_options::set(kSslMode, str_mode, Set_mode::UPDATE_NULL);
}

Ssl_mode Ssl_options::get_mode() const {
  int mode = MapSslModeNameToValue::get_value(get_value(kSslMode));

  return static_cast<Ssl_mode>(mode);
}

std::string Ssl_options::get_mode_name() const {
  return get_value(kSslMode);
}

const std::string& Ssl_options::_get(const std::string& id) const {
  return get_value(id);
}

void Ssl_options::set_ca(const std::string& value) {
  Nullable_options::set(kSslCa, value, Set_mode::UPDATE_NULL);
}

void Ssl_options::set_capath(const std::string& value) {
  Nullable_options::set(kSslCaPath, value, Set_mode::UPDATE_NULL);
}

void Ssl_options::set_crl(const std::string& value) {
  Nullable_options::set(kSslCrl, value, Set_mode::UPDATE_NULL);
}

void Ssl_options::set_crlpath(const std::string& value) {
  Nullable_options::set(kSslCrlPath, value, Set_mode::UPDATE_NULL);
}

void Ssl_options::set_cipher(const std::string& value) {
  Nullable_options::set(kSslCipher, value, Set_mode::UPDATE_NULL);
}

void Ssl_options::set_tls_version(const std::string& value) {
  Nullable_options::set(kSslTlsVersion, value, Set_mode::UPDATE_NULL);
}

void Ssl_options::set_cert(const std::string& value) {
  Nullable_options::set(kSslCert, value, Set_mode::UPDATE_NULL);
}

void Ssl_options::set_key(const std::string& value) {
  Nullable_options::set(kSslKey, value, Set_mode::UPDATE_NULL);
}

void Ssl_options::remove(const std::string& name) {
  if (has(name))
    throw std::invalid_argument("Unable to remove " + _ctx + " option");
  else
    throw_invalid_option(name);
}

void Ssl_options::set(const std::string& name, const std::string& value) {
  if (compare(name, kSslMode) == 0) {
    int tmp_mode = MapSslModeNameToValue::get_value(value);

    if (tmp_mode == 0) {
      throw std::invalid_argument(shcore::str_format(
          "Invalid value '%s' for '%s'. Allowed values: "
          "DISABLED, PREFERRED, REQUIRED, VERIFY_CA, VERIFY_IDENTITY.",
          value.c_str(), name.c_str()));
    }
  }

  Nullable_options::set(name, value, Set_mode::UPDATE_NULL);
}

void Ssl_options::validate() const {
  if (has_mode()) {
    auto mode = get_mode();

    // Temporary copy of the options for validation
    Ssl_options options = *this;
    options.clear_mode();

    if (mode == Ssl_mode::Disabled && options.has_data())
      throw std::invalid_argument(shcore::str_format(
          "SSL options are not allowed when %s is set to '%s'.", kSslMode,
          kSslModeDisabled));

    if (mode != Ssl_mode::VerifyCa && mode != Ssl_mode::VerifyIdentity &&
        (has_ca() || has_capath() || has_crl() || has_crlpath())) {
      throw std::invalid_argument(shcore::str_format(
          "Invalid %s, value should be either '%s' or '%s' when any of '%s', "
          "'%s', '%s' or '%s' are provided.",
          kSslMode, kSslModeVerifyCA, kSslModeVerifyIdentity, kSslCa,
          kSslCaPath, kSslCrl, kSslCrlPath));
    }
  }
}

}  // namespace db
}  // namespace mysqlshdk
