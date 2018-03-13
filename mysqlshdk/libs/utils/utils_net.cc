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
// clang-format off
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Iphlpapi.h>
#include <windows.h>
// clang-format on
// for GetIpAddrTable()
#pragma comment(lib, "IPHLPAPI.lib")
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <errno.h>
#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"

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

void get_host_ip_addresses(const std::string &host,
                           std::vector<std::string> *out_addrs) {
  assert(out_addrs);

  auto prepare_net_error = [&host](const char *error) -> net_error {
    return net_error{"Could not resolve " + host + ": " + error + "."};
  };

  addrinfo hints;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;  // IPv4

  addrinfo *info = nullptr;
  int result = getaddrinfo(host.c_str(), nullptr, &hints, &info);

  if (result != 0) throw prepare_net_error(gai_strerror(result));

  if (info != nullptr) {
    std::unique_ptr<addrinfo, void (*)(addrinfo *)> deleter{info, freeaddrinfo};

    // return first IP address
    char ip[NI_MAXHOST];

    result = getnameinfo(info->ai_addr, info->ai_addrlen, ip, sizeof(ip),
                         nullptr, 0, NI_NUMERICHOST);

    if (result != 0) throw prepare_net_error(gai_strerror(result));

    out_addrs->push_back(ip);
  } else {
    throw prepare_net_error("Unable to resolve host");
  }
}

std::string get_this_hostname() {
  char hostname[1024] = {'\0'};
  if (gethostname(hostname, sizeof(hostname)) < 0)
    throw net_error("Could not get local host address: " +
                    shcore::errno_to_string(errno));
  return hostname;
}

#ifdef _WIN32
void get_this_host_addresses(bool prefer_name,
                             std::vector<std::string> *out_addrs) {
  assert(out_addrs);

  MIB_IPADDRTABLE *addr_table;
  DWORD size = 0;
  DWORD ret = 0;
  std::vector<char> buffer;

  buffer.resize(sizeof(MIB_IPADDRTABLE));
  addr_table = reinterpret_cast<MIB_IPADDRTABLE *>(&buffer[0]);
  // Note: unlike getifaddrs() this will include loopback interfaces
  if (GetIpAddrTable(addr_table, &size, 0) == ERROR_INSUFFICIENT_BUFFER) {
    buffer.resize(size);
    addr_table = reinterpret_cast<MIB_IPADDRTABLE *>(&buffer[0]);
  }

  if ((ret = GetIpAddrTable(addr_table, &size, 0)) != NO_ERROR) {
    LPVOID msgbuf;

    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL, ret, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)&msgbuf, 0, NULL)) {
      std::string tmp(static_cast<char *>(msgbuf));
      LocalFree(msgbuf);
      throw std::runtime_error(tmp);
    }
    throw std::runtime_error("Error on call to GetIpAddrTable()");
  }

  for (DWORD i = 0; i < addr_table->dwNumEntries; i++) {
    IN_ADDR addr;
    memset(&addr, 0, sizeof(addr));
    addr.S_un.S_addr = (u_long)addr_table->table[i].dwAddr;
    out_addrs->push_back(inet_ntoa(addr));
  }
  if (out_addrs->empty()) out_addrs->push_back(get_this_hostname());
}

#elif defined(__APPLE__) || defined(__linux__)

void get_this_host_addresses(bool prefer_name,
                             std::vector<std::string> *out_addrs) {
  assert(out_addrs);

  struct ifaddrs *ifa, *ifap;
  int ret = EAI_NONAME, family, addrlen;

  if (getifaddrs(&ifa) != 0)
    throw net_error("Could not get local host address: " +
                    shcore::errno_to_string(errno));

  for (ifap = ifa; ifap != NULL; ifap = ifap->ifa_next) {
    /* Skip interfaces that are not UP, do not have configured addresses, and
     * loopback interface */
    if ((ifap->ifa_addr == NULL) || (ifap->ifa_flags & IFF_LOOPBACK) ||
        (!(ifap->ifa_flags & IFF_UP)))
      continue;

    /* Only handle IPv4 and IPv6 addresses */
    family = ifap->ifa_addr->sa_family;
    if (family != AF_INET && family != AF_INET6) continue;

    addrlen = (family == AF_INET) ? sizeof(struct sockaddr_in)
                                  : sizeof(struct sockaddr_in6);

    /* Skip IPv6 link-local addresses */
    if (family == AF_INET6) {
      struct sockaddr_in6 *sin6;

      sin6 = (struct sockaddr_in6 *)ifap->ifa_addr;
      if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr) ||
          IN6_IS_ADDR_MC_LINKLOCAL(&sin6->sin6_addr))
        continue;
    }

    char hostname[1024] = {'\0'};
    ret = -1;
    if (prefer_name)
      ret = getnameinfo(ifap->ifa_addr, addrlen, hostname, sizeof(hostname),
                        NULL, 0, NI_NAMEREQD);
    if (ret != 0)
      ret = getnameinfo(ifap->ifa_addr, addrlen, hostname, sizeof(hostname),
                        NULL, 0, NI_NUMERICHOST);
    if (ret == 0) out_addrs->push_back(hostname);
  }
  if (out_addrs->empty()) {
    out_addrs->push_back(get_this_hostname());
  }
}

#else

void get_this_host_addresses(std::vector<std::string> *out_addrs,
                             bool /*prefer_name*/) {
  assert(out_addrs);
  out_addrs->push_back(get_this_hostname());
}
#endif

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

bool Net::is_loopback(const std::string &address) {
  return get()->is_loopback_impl(address);
}

bool Net::is_local_address(const std::string &address) {
  return get()->is_local_address_impl(address);
}

std::string Net::get_hostname() { return get()->get_hostname_impl(); }

void Net::get_local_addresses(std::vector<std::string> *out_addrs) {
  assert(out_addrs);
  get_this_host_addresses(false, out_addrs);
}

Net *Net::get() {
  static Net instance;

  if (s_implementation != nullptr)
    return s_implementation;
  else
    return &instance;
}

void Net::set(Net *implementation) { s_implementation = implementation; }

std::string Net::resolve_hostname_ipv4_impl(const std::string &name) const {
  auto prepare_net_error = [&name](const char *error) -> net_error {
    return net_error{"Could not resolve " + name + ": " + error + "."};
  };

  if (is_ipv4(name)) return name;

  addrinfo hints;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;  // IPv4

  addrinfo *info = nullptr;
  int result = getaddrinfo(name.c_str(), nullptr, &hints, &info);

  if (result != 0) throw prepare_net_error(gai_strerror(result));

  if (info != nullptr) {
    std::unique_ptr<addrinfo, void (*)(addrinfo *)> deleter{info, freeaddrinfo};

    // return first IP address
    char ip[NI_MAXHOST];

    result = getnameinfo(info->ai_addr, info->ai_addrlen, ip, sizeof(ip),
                         nullptr, 0, NI_NUMERICHOST);

    if (result != 0) throw prepare_net_error(gai_strerror(result));

    return ip;
  } else {
    throw prepare_net_error("Unable to resolve host");
  }
}

bool Net::is_loopback_impl(const std::string &name) const {
  if (name == "localhost" || name == "::1") return true;

  std::vector<std::string> addresses;
  try {
    get_host_ip_addresses(name, &addresses);
  } catch (std::exception &e) {
    log_info("%s", e.what());
    return false;
  }

  return (name.compare(0, 4, "127.") == 0);
}

bool Net::is_local_address_impl(const std::string &name) const {
  if (is_loopback_impl(name)) return true;

  std::vector<std::string> addresses;
  get_this_host_addresses(false, &addresses);

  if (std::find(addresses.begin(), addresses.end(), name) != addresses.end())
    return true;

  try {
    std::vector<std::string> name_addr;
    get_host_ip_addresses(name, &name_addr);

    for (const std::string &n : name_addr) {
      if (n == name ||
          std::find(addresses.begin(), addresses.end(), n) != addresses.end())
        return true;
    }
  } catch (std::exception &e) {
    log_info("%s", e.what());
  }

  return false;
}

std::string Net::get_hostname_impl() const { return get_this_hostname(); }

}  // namespace utils
}  // namespace mysqlshdk
