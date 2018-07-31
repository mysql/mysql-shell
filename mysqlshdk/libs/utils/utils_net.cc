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
// for GetAdaptersAddresses()
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

#include "mysqlshdk/libs/utils/enumset.h"
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
 * @return AF_INET if address is an IPv4 address, AF_INET6 if it's IPv6,
 *         AF_UNSPEC in case of failure.
 */
int get_protocol_family(const std::string &address) {
  addrinfo hints;

  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_NUMERICHOST;

  addrinfo *info = nullptr;
  int result = getaddrinfo(address.c_str(), nullptr, &hints, &info);
  int family = AF_UNSPEC;

  if (result == 0) {
    family = info->ai_family;
    freeaddrinfo(info);
    info = nullptr;
  }

  return family;
}

void get_host_ipv4_addresses(const std::string &host,
                             std::vector<std::string> *out_addrs) {
  assert(out_addrs);

  if (AF_INET == get_protocol_family(host)) {
    // getaddrinfo() on macOS does not work correctly in case of IPv4 addresses
    // in octal format: it first assumes that address is in decimal format,
    // only if parsing fails, it tries other formats. This causes i.e.
    // address 010.010.010.010 to be resolved to 10.10.10.10 instead of 8.8.8.8.
    // This is a workaround for all the platforms.
    in_addr address;
    address.s_addr = inet_addr(host.c_str());
    out_addrs->emplace_back(inet_ntoa(address));
    return;
  }

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
  char hostname[NI_MAXHOST] = {'\0'};
  if (gethostname(hostname, sizeof(hostname)) < 0)
    throw net_error("Could not get local host address: " +
                    shcore::errno_to_string(errno));
  return hostname;
}

namespace detail {

enum class Address_type { LOOPBACK, LOCAL };

using Address_types =
    mysqlshdk::utils::Enum_set<Address_type, Address_type::LOCAL>;

#ifdef _WIN32

void get_this_host_addresses_impl(const Address_types &types,
                                  std::vector<std::string> *out_addrs) {
  ULONG size = 15 * 1024;  // 15KB is the recommended buffer size
  std::vector<char> buffer;
  DWORD ret = 0;
  IP_ADAPTER_ADDRESSES *addresses = nullptr;

  do {
    buffer.resize(size);
    addresses = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(&buffer[0]);

    ret = GetAdaptersAddresses(AF_UNSPEC,  // return both IPv4 and IPv6
                               GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                                   GAA_FLAG_SKIP_DNS_SERVER |
                                   GAA_FLAG_SKIP_FRIENDLY_NAME,
                               nullptr, addresses, &size);
  } while (ret == ERROR_BUFFER_OVERFLOW);

  if (NO_ERROR == ret) {
    for (auto iface = addresses; nullptr != iface; iface = iface->Next) {
      // Skip interfaces that are not UP
      if (IfOperStatusUp != iface->OperStatus) {
        continue;
      }

      const bool is_loopback = IF_TYPE_SOFTWARE_LOOPBACK == iface->IfType;

      if (!((types & Address_type::LOOPBACK) && is_loopback) &&
          !((types & Address_type::LOCAL) && !is_loopback)) {
        continue;
      }

      for (auto unicast = iface->FirstUnicastAddress; nullptr != unicast;
           unicast = unicast->Next) {
        DWORD length = 0;
        // first call to get length of a string
        WSAAddressToString(unicast->Address.lpSockaddr,
                           unicast->Address.iSockaddrLength, nullptr, nullptr,
                           &length);

        std::string address;
        // WSAAddressToString() will write address along with null terminator
        address.resize(length);
        // second call to perform conversion
        WSAAddressToString(unicast->Address.lpSockaddr,
                           unicast->Address.iSockaddrLength, nullptr,
                           &address[0], &length);
        // trim null terminator
        address.resize(length - 1);

        out_addrs->emplace_back(address);
      }
    }
  } else {
    throw std::runtime_error("Error on call to GetAdaptersAddresses(): " +
                             shcore::last_error_to_string(ret));
  }
}

#else  // ! _WIN32

void get_this_host_addresses_impl(const Address_types &types,
                                  std::vector<std::string> *out_addrs) {
  struct ifaddrs *ifa = nullptr;

  if (getifaddrs(&ifa) != 0) {
    throw net_error("Could not get local host address: " +
                    shcore::errno_to_string(errno));
  }

  std::unique_ptr<ifaddrs, void (*)(ifaddrs *)> deleter{ifa, freeifaddrs};

  for (auto ifap = ifa; ifap != nullptr; ifap = ifap->ifa_next) {
    // Skip interfaces that are not UP, and do not have configured addresses
    if ((ifap->ifa_addr == nullptr) || (!(ifap->ifa_flags & IFF_UP))) {
      continue;
    }

    const bool is_loopback = (ifap->ifa_flags & IFF_LOOPBACK) != 0;

    if (!((types & Address_type::LOOPBACK) && is_loopback) &&
        !((types & Address_type::LOCAL) && !is_loopback)) {
      continue;
    }

    // Only handle IPv4 and IPv6 addresses
    const auto family = ifap->ifa_addr->sa_family;
    if (family != AF_INET && family != AF_INET6) continue;

    char hostname[NI_MAXHOST] = {'\0'};
    if (getnameinfo(ifap->ifa_addr,
                    (family == AF_INET) ? sizeof(struct sockaddr_in)
                                        : sizeof(struct sockaddr_in6),
                    hostname, sizeof(hostname), nullptr, 0,
                    NI_NUMERICHOST) == 0) {
      out_addrs->push_back(hostname);
    }
  }
}

#endif  // ! _WIN32

void get_this_host_addresses(const Address_types &types,
                             std::vector<std::string> *out_addrs) {
  assert(out_addrs);

  get_this_host_addresses_impl(types, out_addrs);

  if ((types & Address_type::LOCAL) && out_addrs->empty()) {
    out_addrs->push_back(get_this_hostname());
  }
}

}  // namespace detail
}  // namespace

Net *Net::s_implementation = nullptr;

std::string Net::resolve_hostname_ipv4(const std::string &name) {
  return resolve_hostname_ipv4_all(name)[0];
}

std::vector<std::string> Net::resolve_hostname_ipv4_all(
    const std::string &name) {
  return get()->resolve_hostname_ipv4_all_impl(name);
}

bool Net::is_ipv4(const std::string &host) {
  return get_protocol_family(host) == AF_INET;
}

bool Net::is_ipv6(const std::string &host) {
  // Handling of the zone ID by getaddrinfo() varies on different platforms:
  // numeric values are always accepted, some of them require the zone ID to
  // match the name of one of the network interfaces, while others accept
  // any value. To accommodate for that, we strip the zone ID part from
  // the address as it's enough to check the remaining part to decide if the
  // whole address is IPv6.
  return get_protocol_family(host.substr(0, host.find('%'))) == AF_INET6;
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

std::vector<std::string> Net::get_local_addresses() {
  using namespace detail;
  std::vector<std::string> out_addrs;
  get_this_host_addresses(Address_types{Address_type::LOCAL}, &out_addrs);
  return out_addrs;
}

std::vector<std::string> Net::get_loopback_addresses() {
  using namespace detail;
  std::vector<std::string> out_addrs;
  get_this_host_addresses(Address_types{Address_type::LOOPBACK}, &out_addrs);
  return out_addrs;
}

Net *Net::get() {
  static Net instance;

  if (s_implementation != nullptr)
    return s_implementation;
  else
    return &instance;
}

void Net::set(Net *implementation) { s_implementation = implementation; }

std::vector<std::string> Net::resolve_hostname_ipv4_all_impl(
    const std::string &name) const {
  std::vector<std::string> addrs;
  get_host_ipv4_addresses(name, &addrs);
  return addrs;
}

bool Net::is_loopback_impl(const std::string &name) const {
  if (name == "localhost" || name == "::1" || name.compare(0, 4, "127.") == 0) {
    return true;
  }

  const auto loopback = get_loopback_addresses();

  if (loopback.end() != std::find(loopback.begin(), loopback.end(), name)) {
    return true;
  }

  try {
    for (const auto &ipv4 : resolve_hostname_ipv4_all(name)) {
      if (ipv4.compare(0, 4, "127.") == 0 ||
          loopback.end() != std::find(loopback.begin(), loopback.end(), ipv4)) {
        return true;
      }
    }
  } catch (const std::exception &e) {
    // Exception means that 'name' is an IPv6 address or a host which could not
    // be resolved. In case of former, it means that address was not found
    // in loopback addresses. In case of latter, we cannot obtain an address.
    // In both cases it's not a loopback address.
    log_info("%s", e.what());
  }

  return false;
}

bool Net::is_local_address_impl(const std::string &name) const {
  if (is_loopback(name)) return true;

  const auto local = get_local_addresses();

  if (local.end() != std::find(local.begin(), local.end(), name)) {
    return true;
  }

  try {
    for (const auto &ipv4 : resolve_hostname_ipv4_all(name)) {
      if (local.end() != std::find(local.begin(), local.end(), ipv4)) {
        return true;
      }
    }
  } catch (const std::exception &e) {
    // Exception means that 'name' is an IPv6 address or a host which could not
    // be resolved. In case of former, it means that address was not found
    // in local addresses. In case of latter, we cannot obtain an address.
    // In both cases it's not a local address.
    log_info("%s", e.what());
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
