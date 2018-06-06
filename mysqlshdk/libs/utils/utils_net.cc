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
#include <bitset>
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

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
    for (; info != nullptr; info = info->ai_next) {
      // return first IP address
      char ip[NI_MAXHOST];

      result = getnameinfo(info->ai_addr, info->ai_addrlen, ip, sizeof(ip),
                           nullptr, 0, NI_NUMERICHOST);

      if (result != 0) throw prepare_net_error(gai_strerror(result));

      if (std::find(out_addrs->begin(), out_addrs->end(), ip) ==
          out_addrs->end())
        out_addrs->push_back(ip);
    }
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
    throw std::runtime_error("Error on call to GetIpAddrTable(): " +
                             shcore::last_error_to_string(ret));
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

std::vector<std::string> Net::resolve_hostname_ipv4_all(
    const std::string &name) {
  return get()->resolve_hostname_ipv4_all_impl(name);
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

bool Net::is_port_listening(const std::string &address, int port) {
  return get()->is_port_listening_impl(address, port);
}

bool Net::strip_cidr(std::string *address, int *cidr) {
  // Strip the CIDR value, if specified
  size_t pos = address->find("/");
  if (pos != std::string::npos) {
    std::string cidr_str = address->substr(pos + 1);

    try {
      *cidr = std::stoi(cidr_str);
    } catch (const std::invalid_argument &e) {
      throw std::invalid_argument("Invalid subnet CIDR value: '" + cidr_str +
                                  "': " + e.what());
    } catch (const std::out_of_range &e) {
      throw std::out_of_range("Converted CIDR value: '" + cidr_str +
                              "' out of range: " + e.what());
    }

    address->erase(pos);

    return true;
  }
  return false;
}

std::string Net::cidr_to_netmask(const std::string &address) {
  std::string ret = address;
  int cidr = 0;

  // Validate if the address value is not empty
  if (shcore::str_strip(address).empty())
    throw std::runtime_error(
        "Invalid value for address: string value cannot be empty.");

  // Strip the CIDR value, if specified
  if (strip_cidr(&ret, &cidr)) {
    if ((cidr < 1) || (cidr > 32))
      throw std::runtime_error(
          "Could not translate address: subnet value in CIDR notation is not "
          "valid.");
  }

  // Convert the CIDR value to netmask notation
  if (cidr != 0) {
    std::bitset<32> bitset;

    // Set the cidr bits in the bit_array
    for (int i = 31; i > (31 - cidr); i--) bitset.set(i);

    std::string full_bitset = bitset.to_string();

    // Split the full bit_array in the 4 blocks of 8 bits
    std::string bitset1_str = full_bitset.substr(0, 8);
    std::string bitset2_str = full_bitset.substr(8, 8);
    std::string bitset3_str = full_bitset.substr(16, 8);
    std::string bitset4_str = full_bitset.substr(24, 8);

    // Convert the bit arrays to decimal representation
    std::bitset<8> bitset1(bitset1_str);
    std::bitset<8> bitset2(bitset2_str);
    std::bitset<8> bitset3(bitset3_str);
    std::bitset<8> bitset4(bitset4_str);

    // Construct the final translated address
    bitset1_str = std::to_string(bitset1.to_ulong());
    bitset2_str = std::to_string(bitset2.to_ulong());
    bitset3_str = std::to_string(bitset3.to_ulong());
    bitset4_str = std::to_string(bitset4.to_ulong());

    ret += "/" + bitset1_str + "." + bitset2_str + "." + bitset3_str + "." +
           bitset4_str;
  }

  return ret;
}

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

std::vector<std::string> Net::resolve_hostname_ipv4_all_impl(
    const std::string &name) const {
  std::vector<std::string> addrs;
  get_host_ip_addresses(name, &addrs);
  return addrs;
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

  // todo(kg): loopback interface isn't limited to 127.0.0.0/8. We should list
  // IPs assigned to lo interface, iterate over them, and check if resolved name
  // belong to lo interface.
  for (const auto &a : addresses) {
    if (a.compare(0, 4, "127.") == 0) {
      return true;
    }
  }

  return (name.compare(0, 4, "127.") == 0);
}

bool Net::is_local_address_impl(const std::string &name) const {
  if (is_loopback_impl(name)) return true;

  std::vector<std::string> addresses;
  get_this_host_addresses(false, &addresses);

  if (std::find(addresses.begin(), addresses.end(), name) != addresses.end())
    return true;

  if (!is_ipv4(name) && !is_ipv6(name)) {
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
  }

  return false;
}

std::string Net::get_hostname_impl() const { return get_this_hostname(); }

#ifndef _WIN32
void closesocket(int sock) { ::close(sock); }
#endif

bool Net::is_port_listening_impl(const std::string &address, int port) const {
  addrinfo hints;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  addrinfo *info = nullptr;
  int result =
      getaddrinfo(address.c_str(), std::to_string(port).c_str(), &hints, &info);

  if (result != 0) throw net_error(gai_strerror(result));

  if (info != nullptr) {
    std::unique_ptr<addrinfo, void (*)(addrinfo *)> deleter{info, freeaddrinfo};

    auto sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (sock < 0) {
      throw std::runtime_error("Could not create socket: " +
                               shcore::errno_to_string(errno));
    }

    if (connect(sock, info->ai_addr, info->ai_addrlen) == 0) {
      closesocket(sock);
      return true;
    }
#ifdef _WIN32
    const int err = WSAGetLastError();
    const int connection_refused = WSAECONNREFUSED;
#else
    const int err = errno;
    const int connection_refused = ECONNREFUSED;
#endif
    closesocket(sock);

    if (err == connection_refused) {
      return false;
    }
#ifdef _WIN32
    throw net_error(shcore::last_error_to_string(err));
#else
    throw net_error(shcore::errno_to_string(err));
#endif
  }
  throw std::runtime_error("Could not resolve address");
}
}  // namespace utils
}  // namespace mysqlshdk
