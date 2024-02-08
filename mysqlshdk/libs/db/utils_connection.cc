/*
 * Copyright (c) 2014, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/db/utils_connection.h"

#include <algorithm>
#include <array>
#include <iterator>

namespace mysqlshdk {
namespace db {

namespace {
const std::set<std::string> db_connection_attributes = {
    kUri,
    kHost,
    kPort,
    kSocket,
    kScheme,
    kSchema,
    kUser,
    kDbUser,
    kPassword,
    kDbPassword,
    kSslCa,
    kSslCaPath,
    kSslCert,
    kSslKey,
    kSslCrl,
    kSslCrlPath,
    kSslCipher,
    kSslTlsVersion,
    kSslTlsCiphersuites,
    kSslMode,
    kAuthMethod,
    kGetServerPublicKey,
    kServerPublicKeyPath,
    kConnectTimeout,
    kNetReadTimeout,
    kNetWriteTimeout,
    kCompression,
    kCompressionAlgorithms,
    kCompressionLevel,
    kConnectionAttributes,
#ifdef _WIN32
    kKerberosClientAuthMode,
#endif
    kOciConfigFile,
    kOciAuthenticationClientConfigProfile,
};

const std::array<std::string, 6> ssl_modes = {"",
                                              kSslModeDisabled,
                                              kSslModePreferred,
                                              kSslModeRequired,
                                              kSslModeVerifyCA,
                                              kSslModeVerifyIdentity};
}  // namespace

int MapSslModeNameToValue::get_value(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), ::tolower);

  auto index = std::find(ssl_modes.begin(), ssl_modes.end(), value);
  return index != ssl_modes.end() ? index - ssl_modes.begin() : 0;
}

const std::string &MapSslModeNameToValue::get_value(int value) {
  auto index = (value >= 1 && value <= (static_cast<int>(ssl_modes.size()) - 1))
                   ? value
                   : 0;

  return ssl_modes[index];
}

std::set<std::string> connection_attributes() {
  std::set<std::string> merged_attributes;
  std::set_union(db_connection_attributes.begin(),
                 db_connection_attributes.end(),
                 ssh_uri_connection_attributes.begin(),
                 ssh_uri_connection_attributes.end(),
                 std::inserter(merged_attributes, merged_attributes.begin()));
  return merged_attributes;
}

}  // namespace db
}  // namespace mysqlshdk
