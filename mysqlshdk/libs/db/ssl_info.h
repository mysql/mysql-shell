/*
* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __CORELIBS_UTILS_SSL_INFO_H__
#define __CORELIBS_UTILS_SSL_INFO_H__

#include "mysqlshdk_export.h"
#include "mysqlshdk/libs/utils/nullable.h"

#include <string>


namespace mysqlshdk {
namespace utils {

struct SHCORE_PUBLIC Ssl_info
{
  Ssl_info() {
    skip = true;
    mode = 0;
  }

  Ssl_info(const Ssl_info& s);
  bool skip;
  int mode;
  nullable<std::string> ca;
  nullable<std::string> capath;
  nullable<std::string> crl;
  nullable<std::string> crlpath;
  nullable<std::string> ciphers;
  nullable<std::string> tls_version;
  nullable<std::string> cert;
  nullable<std::string> key;

  bool has_data() const {
    return skip == false ||
      mode != 0 ||
      !ca.is_null() ||
      !capath.is_null() ||
      !crl.is_null() ||
      !crlpath.is_null() ||
      !ciphers.is_null() ||
      !tls_version.is_null() ||
      !cert.is_null() ||
      !key.is_null();
  }
};


enum class SHCORE_PUBLIC Ssl_mode {
  Disabled = 1,
  Preferred = 2,
  Required = 3,
  VerifyCa = 4,
  VerifyIdentity = 5
};
}
}
#endif /* __mysh__utils_connection__ */
