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

#ifndef __mysh__utils_connection__
#define __mysh__utils_connection__

#include <string>
#include <set>

#include "shellcore/common.h"

#ifdef _WIN32
#include <string.h>
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

namespace shcore {

struct SHCORE_PUBLIC SslInfo
{
  SslInfo() {
    skip = true;
    mode = 0;
    ca = "";
    cert = "";
    key = "";
    capath = "";
    ciphers = "";
    crl = "";
    crlpath = "";
    tls_version = "";

  }
  SslInfo(const SslInfo& s);
  bool skip;
  int mode;
  std::string ca;
  std::string capath;
  std::string crl;
  std::string crlpath;
  std::string ciphers;
  std::string tls_version;
  std::string cert;
  std::string key;
  bool has_data() {
    return skip == false ||
      mode != 0 ||
      !ca.empty() ||
      !capath.empty() ||
      !crl.empty() ||
      !crlpath.empty() ||
      !ciphers.empty() ||
      !tls_version.empty() ||
      !cert.empty() ||
      !key.empty();
  }
};


enum class SslMode {
  Disabled = 1,
  Preferred = 2,
  Required = 3,
  VerifyCa = 4,
  VerifyIdentity = 5
};

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

  static int get_value(const std::string& value);
  static const std::string get_value(int value);

private:
  static std::string _empty;
};

// common keys for dictionary connection data
const std::string kHost = "host";
const std::string kPort = "port";
const std::string kSocket = "socket";
const std::string kScheme = "scheme";
const std::string kSchema = "schema";
const std::string kUser = "user";
const std::string kDbUser = "dbUser";
const std::string kPassword = "password";
const std::string kDbPassword = "dbPassword";
const std::string kSslCa = "sslCa";
const std::string kSslCaPath = "sslCaPath";
const std::string kSslCert = "sslCert";
const std::string kSslKey = "sslKey";
const std::string kSslCrl = "sslCrl";
const std::string kSslCrlPath = "sslCrlPath";
const std::string kSslCiphers = "sslCiphers";
const std::string kSslTlsVersion = "sslTlsVersion";
const std::string kSslMode = "sslMode";
const std::string kAuthMethod = "authMethod";

const std::set<std::string> connection_attributes = {kHost, kPort, kSocket,
  kScheme, kSchema, kUser, kDbUser, kPassword, kDbPassword, kSslCa, kSslCaPath,
  kSslCert, kSslKey, kSslCrl, kSslCrlPath, kSslCiphers, kSslTlsVersion,
  kSslMode, kAuthMethod };

};

#endif /* __mysh__utils_connection__ */

