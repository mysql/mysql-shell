/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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


#ifndef _MYSQLD_CONTEXT_SSL_H_
#define _MYSQLD_CONTEXT_SSL_H_

#include <boost/asio/buffer.hpp>
#include <queue>


#if defined(HAVE_YASSL)
using namespace yaSSL;
#endif // defined(HAVE_YASSL)

namespace mysqld
{
  void set_context(SSL_CTX* ssl_context, const bool is_client, const std::string &ssl_key,
                   const std::string &ssl_cert,    const std::string &ssl_ca,
                   const std::string &ssl_ca_path, const std::string &ssl_cipher,
                   const std::string &ssl_crl,     const std::string &ssl_crl_path,
                   const std::string &ssl_tls_version, int ssl_mode);

  enum class TlsVersion
  {
    None = 0,
    TLS_V10 = 0x1,
    TLS_V11 = 0x2,
    TLS_V12 = 0x4
  };

  int parse_tls_version(const std::string& tls_version);

  // The following from vio/viosslfactories.c

//#ifdef HAVE_YASSL
  static const char tls_ciphers_list[] = "DHE-RSA-AES256-SHA:DHE-RSA-AES128-SHA:"
    "AES128-RMD:DES-CBC3-RMD:DHE-RSA-AES256-RMD:"
    "DHE-RSA-AES128-RMD:DHE-RSA-DES-CBC3-RMD:"
    "AES256-SHA:RC4-SHA:RC4-MD5:DES-CBC3-SHA:"
    "DES-CBC-SHA:EDH-RSA-DES-CBC3-SHA:"
    "EDH-RSA-DES-CBC-SHA:AES128-SHA:AES256-RMD";
  static const char tls_cipher_blocked[] = "!aNULL:!eNULL:!EXPORT:!LOW:!MD5:!DES:!RC2:!RC4:!PSK:";
/*#else
  static const char tls_ciphers_list[] = "ECDHE-ECDSA-AES128-GCM-SHA256:"
    "ECDHE-ECDSA-AES256-GCM-SHA384:"
    "ECDHE-RSA-AES128-GCM-SHA256:"
    "ECDHE-RSA-AES256-GCM-SHA384:"
    "ECDHE-ECDSA-AES128-SHA256:"
    "ECDHE-RSA-AES128-SHA256:"
    "ECDHE-ECDSA-AES256-SHA384:"
    "ECDHE-RSA-AES256-SHA384:"
    "DHE-RSA-AES128-GCM-SHA256:"
    "DHE-DSS-AES128-GCM-SHA256:"
    "DHE-RSA-AES128-SHA256:"
    "DHE-DSS-AES128-SHA256:"
    "DHE-DSS-AES256-GCM-SHA384:"
    "DHE-RSA-AES256-SHA256:"
    "DHE-DSS-AES256-SHA256:"
    "ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:"
    "ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:"
    "DHE-DSS-AES128-SHA:DHE-RSA-AES128-SHA:"
    "TLS_DHE_DSS_WITH_AES_256_CBC_SHA:DHE-RSA-AES256-SHA:"
    "AES128-GCM-SHA256:DH-DSS-AES128-GCM-SHA256:"
    "ECDH-ECDSA-AES128-GCM-SHA256:AES256-GCM-SHA384:"
    "DH-DSS-AES256-GCM-SHA384:ECDH-ECDSA-AES256-GCM-SHA384:"
    "AES128-SHA256:DH-DSS-AES128-SHA256:ECDH-ECDSA-AES128-SHA256:AES256-SHA256:"
    "DH-DSS-AES256-SHA256:ECDH-ECDSA-AES256-SHA384:AES128-SHA:"
    "DH-DSS-AES128-SHA:ECDH-ECDSA-AES128-SHA:AES256-SHA:"
    "DH-DSS-AES256-SHA:ECDH-ECDSA-AES256-SHA:DHE-RSA-AES256-GCM-SHA384:"
    "DH-RSA-AES128-GCM-SHA256:ECDH-RSA-AES128-GCM-SHA256:DH-RSA-AES256-GCM-SHA384:"
    "ECDH-RSA-AES256-GCM-SHA384:DH-RSA-AES128-SHA256:"
    "ECDH-RSA-AES128-SHA256:DH-RSA-AES256-SHA256:"
    "ECDH-RSA-AES256-SHA384:ECDHE-RSA-AES128-SHA:"
    "ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA:"
    "ECDHE-ECDSA-AES256-SHA:DHE-DSS-AES128-SHA:DHE-RSA-AES128-SHA:"
    "TLS_DHE_DSS_WITH_AES_256_CBC_SHA:DHE-RSA-AES256-SHA:"
    "AES128-SHA:DH-DSS-AES128-SHA:ECDH-ECDSA-AES128-SHA:AES256-SHA:"
    "DH-DSS-AES256-SHA:ECDH-ECDSA-AES256-SHA:DH-RSA-AES128-SHA:"
    "ECDH-RSA-AES128-SHA:DH-RSA-AES256-SHA:ECDH-RSA-AES256-SHA:DES-CBC3-SHA";
  static const char tls_cipher_blocked[] = "!aNULL:!eNULL:!EXPORT:!LOW:!MD5:!DES:!RC2:!RC4:!PSK:"
    "!DHE-DSS-DES-CBC3-SHA:!DHE-RSA-DES-CBC3-SHA:"
    "!ECDH-RSA-DES-CBC3-SHA:!ECDH-ECDSA-DES-CBC3-SHA:"
    "!ECDHE-RSA-DES-CBC3-SHA:!ECDHE-ECDSA-DES-CBC3-SHA:";
#endif*/

  const int SSL_CIPHER_LIST_SIZE = 4096;
}  // namespace mysqld


#endif // _MYSQLD_CONTEXT_SSL_H_
