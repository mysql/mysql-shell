/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include <Ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include <cstring>
#include <memory>

namespace mysqlshdk {
namespace utils {

namespace {

/**
 * Provides the protocol family for the given literal address.
 *
 * @param address The address to be checked.
 *
 * @return AF_INET if address is an IPv4 address, AF_INET6 if it's IPv6, 0 in
 *         case of failure.
 */
int get_protocol_family(const std::string &address) {
  addrinfo hints;

  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_NUMERICHOST;

  addrinfo *info = nullptr;
  int result = getaddrinfo(address.c_str(), nullptr, &hints, &info);
  int family = 0;

  if (result == 0) {
    family = info->ai_family;
    freeaddrinfo(info);
    info = nullptr;
  }

  return family;
}

}  // namespace

Net *Net::s_implementation = nullptr;

std::string Net::resolve_hostname_ipv4(const std::string &name) {
  return get()->resolve_hostname_ipv4_impl(name);
}

bool Net::is_ipv4(const std::string &host) {
  return get_protocol_family(host) == AF_INET;
}

bool Net::is_ipv6(const std::string &host) {
  return get_protocol_family(host) == AF_INET6;
}

Net *Net::get() {
  static Net instance;

  if (s_implementation != nullptr)
    return s_implementation;
  else
    return &instance;
}

void Net::set(Net *implementation) {
  s_implementation = implementation;
}

std::string Net::resolve_hostname_ipv4_impl(const std::string &name) const {
  auto prepare_net_error = [&name](const char *error) -> net_error {
    return net_error{"Could not resolve " + name + ": " + error + "."};
  };

  if (is_ipv4(name))
    return name;

  addrinfo hints;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;  // IPv4

  addrinfo *info = nullptr;
  int result = getaddrinfo(name.c_str(), nullptr, &hints, &info);

  if (result != 0)
    throw prepare_net_error(gai_strerror(result));

  if (info != nullptr) {
    std::unique_ptr<addrinfo, void (*)(addrinfo *)> deleter{info, freeaddrinfo};

    // return first IP address
    char ip[NI_MAXHOST];

    result = getnameinfo(info->ai_addr, info->ai_addrlen, ip, sizeof(ip),
                          nullptr, 0, NI_NUMERICHOST);

    if (result != 0)
      throw prepare_net_error(gai_strerror(result));

    return ip;
  } else {
    throw prepare_net_error("Unable to resolve host");
  }
}

bool is_loopback_address(const std::string &name) {
  struct hostent *he;
  // if the host is not local, we try to resolve it and see if it points to
  // a loopback
  he = gethostbyname(name.c_str());
  if (he) {
    for (struct in_addr **h = (struct in_addr **)he->h_addr_list; *h; ++h) {
      const char *addr = inet_ntoa(**h);
      if (strncmp(addr, "127.", 4) == 0) {
        return true;
      }
    }
  }
  return false;
}

}  // namespace utils
}  // namespace mysqlshdk
