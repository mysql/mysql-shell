/*
 * Copyright (c) 2014, 2023, Oracle and/or its affiliates.
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

#include "mysqlshdk/include/mysqlshdk_export.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#ifdef _WIN32
#include <string.h>
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
  MapSslModeNameToValue() = default;

 public:
  static int get_value(std::string value);
  static const std::string &get_value(int value);
};

// common keys for dictionary connection data
inline constexpr const char kHost[] = "host";
inline constexpr const char kPort[] = "port";
inline constexpr const char kSocket[] = "socket";
inline constexpr const char kPipe[] = "pipe";
inline constexpr const char kScheme[] = "scheme";
// schema in mysql uri is actually a path in generic URI
// so this two will point to the exact same place then
inline constexpr const char kPath[] = "schema";
inline constexpr const char kSchema[] = "schema";
inline constexpr const char kUser[] = "user";
inline constexpr const char kDbUser[] = "dbUser";
inline constexpr const char kPassword[] = "password";
inline constexpr const char kDbPassword[] = "dbPassword";
inline constexpr const char kSshRemoteHost[] = "ssh-remote-host";
inline constexpr const char kSshPassword[] = "ssh-password";
inline constexpr const char kSshIdentityFile[] = "ssh-identity-file";
inline constexpr const char kSshIdentityFilePassword[] =
    "ssh-identity-file-password";
inline constexpr const char kSshConfigFile[] = "ssh-config-file";
inline constexpr const char kSslCa[] = "ssl-ca";
inline constexpr const char kSslCaPath[] = "ssl-capath";
inline constexpr const char kSslCert[] = "ssl-cert";
inline constexpr const char kSslKey[] = "ssl-key";
inline constexpr const char kSslCrl[] = "ssl-crl";
inline constexpr const char kSslCrlPath[] = "ssl-crlpath";
inline constexpr const char kSslCipher[] = "ssl-cipher";
inline constexpr const char kSslTlsVersion[] = "tls-version";
inline constexpr const char kSslTlsVersions[] = "tls-versions";
inline constexpr const char kSslTlsCiphersuites[] = "tls-ciphersuites";
inline constexpr const char kSslMode[] = "ssl-mode";
inline constexpr const char kAuthMethod[] = "auth-method";
inline constexpr const char kGetServerPublicKey[] = "get-server-public-key";
inline constexpr const char kServerPublicKeyPath[] = "server-public-key-path";
inline constexpr const char kConnectTimeout[] = "connect-timeout";
inline constexpr const char kNetReadTimeout[] = "net-read-timeout";
inline constexpr const char kNetWriteTimeout[] = "net-write-timeout";
inline constexpr const char kCompression[] = "compression";
inline constexpr const char kCompressionAlgorithms[] = "compression-algorithms";
inline constexpr const char kCompressionLevel[] = "compression-level";
inline constexpr const char kLocalInfile[] = "local-infile";
inline constexpr const char kNetBufferLength[] = "net-buffer-length";
inline constexpr const char kMaxAllowedPacket[] = "max-allowed-packet";
inline constexpr const char kMysqlPluginDir[] = "mysql-plugin-dir";
inline constexpr const char kWebauthnClientPreservePrivacy[] =
    "plugin-authentication-webauthn-client-preserve-privacy";
inline constexpr const char kConnectionAttributes[] = "connection-attributes";
inline constexpr const char kUri[] = "uri";
inline constexpr const char kSsh[] = "ssh";
inline constexpr const char kKerberosClientAuthMode[] =
    "plugin-authentication-kerberos-client-mode";
inline constexpr const char kOciConfigFile[] = "oci-config-file";
inline constexpr const char kOciAuthenticationClientConfigProfile[] =
    "authentication-oci-client-config-profile";

inline constexpr const char kSslModeDisabled[] = "disabled";
inline constexpr const char kSslModePreferred[] = "preferred";
inline constexpr const char kSslModeRequired[] = "required";
inline constexpr const char kSslModeVerifyCA[] = "verify_ca";
inline constexpr const char kSslModeVerifyIdentity[] = "verify_identity";

inline constexpr const char kCompressionRequired[] = "REQUIRED";
inline constexpr const char kCompressionPreferred[] = "PREFERRED";
inline constexpr const char kCompressionDisabled[] = "DISABLED";

inline constexpr char kAuthMethodClearPassword[] = "mysql_clear_password";
inline constexpr char kAuthMethodLdapSasl[] = "authentication_ldap_sasl_client";
inline constexpr char kAuthMethodKerberos[] = "authentication_kerberos_client";
inline constexpr char kAuthMethodOci[] = "authentication_oci_client";
inline constexpr char kKerberosAuthModeSSPI[] = "SSPI";
inline constexpr char kKerberosAuthModeGSSAPI[] = "GSSAPI";

inline const std::set<std::string> ssh_uri_connection_attributes = {
    kSsh, kSshConfigFile, kSshIdentityFile, kSshIdentityFilePassword,
    kSshPassword};

std::set<std::string> connection_attributes();

inline const std::set<std::string> uri_connection_attributes = {
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

}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_UTILS_CONNECTION_H_
