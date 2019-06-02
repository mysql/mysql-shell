/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_DB_UTILS_CONNECTION_H_
#define MYSQLSHDK_LIBS_DB_UTILS_CONNECTION_H_

#include <set>
#include <string>
#include <vector>

#include "mysqlshdk/include/mysqlshdk_export.h"

#ifdef _WIN32
#include <string.h>
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

namespace mysqlshdk {
namespace db {

/**
 * Bidirectional map of modes values:
 * "DISABLED" <-> 1
 * "PREFERRED" <-> 2
 * "REQUIRED" <-> 3
 * "VERIFY_CA" <-> 4
 * "VERIFY_IDENTITY" <-> 5
 */
class SHCORE_PUBLIC MapSslModeNameToValue {
 private:
  MapSslModeNameToValue() {}

 public:
  static int get_value(const std::string &value);
  static const std::string &get_value(int value);
};

// common keys for dictionary connection data
constexpr const char kHost[] = "host";
constexpr const char kPort[] = "port";
constexpr const char kSocket[] = "socket";
constexpr const char kScheme[] = "scheme";
constexpr const char kSchema[] = "schema";
constexpr const char kUser[] = "user";
constexpr const char kDbUser[] = "dbUser";
constexpr const char kPassword[] = "password";
constexpr const char kDbPassword[] = "dbPassword";
constexpr const char kSslCa[] = "ssl-ca";
constexpr const char kSslCaPath[] = "ssl-capath";
constexpr const char kSslCert[] = "ssl-cert";
constexpr const char kSslKey[] = "ssl-key";
constexpr const char kSslCrl[] = "ssl-crl";
constexpr const char kSslCrlPath[] = "ssl-crlpath";
constexpr const char kSslCipher[] = "ssl-cipher";
constexpr const char kSslTlsVersion[] = "tls-version";
constexpr const char kSslTlsVersions[] = "tls-versions";
constexpr const char kSslTlsCiphersuites[] = "tls-ciphersuites";
constexpr const char kSslMode[] = "ssl-mode";
constexpr const char kAuthMethod[] = "auth-method";
constexpr const char kGetServerPublicKey[] = "get-server-public-key";
constexpr const char kServerPublicKeyPath[] = "server-public-key-path";
constexpr const char kConnectTimeout[] = "connect-timeout";
constexpr const char kNetReadTimeout[] = "net-read-timeout";
constexpr const char kCompression[] = "compression";
constexpr const char kLocalInfile[] = "local-infile";
constexpr const char kNetBufferLength[] = "net-buffer-length";
constexpr const char kConnectionAttributes[] = "connection-attributes";

constexpr const char kSslModeDisabled[] = "disabled";
constexpr const char kSslModePreferred[] = "preferred";
constexpr const char kSslModeRequired[] = "required";
constexpr const char kSslModeVerifyCA[] = "verify_ca";
constexpr const char kSslModeVerifyIdentity[] = "verify_identity";

const std::set<std::string> connection_attributes = {kHost,
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
                                                     kCompression,
                                                     kConnectionAttributes};

const std::set<std::string> uri_connection_attributes = {kSslCa,
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
                                                         kCompression,
                                                         kConnectionAttributes};

const std::set<std::string> uri_extra_options = {
    kAuthMethod,  kGetServerPublicKey, kServerPublicKeyPath, kConnectTimeout,
    kCompression, kLocalInfile,        kNetBufferLength};

const std::vector<std::string> ssl_modes = {"",
                                            kSslModeDisabled,
                                            kSslModePreferred,
                                            kSslModeRequired,
                                            kSslModeVerifyCA,
                                            kSslModeVerifyIdentity};

}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_UTILS_CONNECTION_H_
