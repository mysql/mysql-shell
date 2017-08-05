/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mysqlshdk/libs/db/ssl_options.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace db {
using mysqlshdk::utils::nullable_options::Set_mode;
using mysqlshdk::utils::nullable_options::Comparison_mode;

const std::set<std::string> Ssl_options::option_str_list = {
    kSslCa,      kSslCaPath,  kSslCert,       kSslKey, kSslCrl,
    kSslCrlPath, kSslCiphers, kSslTlsVersion, kSslMode};

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

void Ssl_options::set_mode(int value) {
  std::string str_mode = MapSslModeNameToValue::get_value(value);

  if (str_mode.empty()) {
    std::string error = "Invalid value for '";
    error.append(kSslMode);
    error.append("'.");
    throw std::invalid_argument(error);
  }

  Nullable_options::set(kSslMode, str_mode, Set_mode::UPDATE_NULL);
}

int Ssl_options::get_mode() const {
  return MapSslModeNameToValue::get_value(get_value(kSslMode));
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

void Ssl_options::set_ciphers(const std::string& value) {
  Nullable_options::set(kSslCiphers, value, Set_mode::UPDATE_NULL);
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

}  // namespace db
}  // namespace mysqlshdk
