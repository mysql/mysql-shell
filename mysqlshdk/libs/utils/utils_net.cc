/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/utils/utils_net.h"
#ifdef WIN32
#include <windows.h>
#include <WinSock2.h>
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#endif

#include <cstring>

namespace mysqlshdk {
namespace utils {

std::string resolve_hostname_ipv4(const std::string &name) {
  struct hostent *hpaddr;
#ifndef WIN32
  struct in_addr addr;
#endif

#ifdef WIN32
  if (inet_addr(name.c_str()) != INADDR_NONE) {
#else
  if (inet_aton(name.c_str(), &addr) != 0) {
#endif
    return name;
  } else {
    hpaddr = gethostbyname(name.c_str());
    if (!hpaddr) {
      switch (h_errno) {
        case HOST_NOT_FOUND:
          throw net_error("Could not resolve " + name + ": host not found");
        case TRY_AGAIN:
          throw net_error("Could not resolve " + name + ": try again");
        case NO_RECOVERY:
          throw net_error("Could not resolve " + name + ": no recovery");
        case NO_DATA:
          throw net_error("Could not resolve " + name + ": no_data");
        default:
          throw net_error("Could not resolve " + name);
      }
    } else {
      struct in_addr in;
      memcpy(reinterpret_cast<char *>(&in.s_addr),
             reinterpret_cast<char *>(*hpaddr->h_addr_list), sizeof(in.s_addr));
      return inet_ntoa(in);
    }
  }
}

}  // namespace utils
}  // namespace mysqlshdk
