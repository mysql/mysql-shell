/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_NET_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_NET_H_

#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "mysqlshdk/libs/utils/nullable.h"

namespace mysqlshdk {
namespace utils {

class net_error : public std::runtime_error {
 public:
  explicit net_error(const std::string &what) : std::runtime_error(what) {}
};

/**
 * Various network-related utilities.
 *
 * This class allows to provide a custom implementation for network-related
 * operations, enabling i.e. mocking the behaviour for unit tests.
 */
class Net {
 public:
  virtual ~Net() = default;

  /**
   * Resolves the given hostname to an IPv4 address.
   *
   * @param name The hostname to be resolved.
   *
   * @return The resolved IPv4 address.
   * @throws net_error if address cannot be resolved.
   */
  static std::string resolve_hostname_ipv4(const std::string &name);

  /**
   * Resolves the given hostname to an IPv6 address.
   *
   * @param name The hostname to be resolved.
   *
   * @return The resolved IPv6 address.
   * @throws net_error if address cannot be resolved.
   */
  static std::string resolve_hostname_ipv6(const std::string &name);

  /**
   * Checks if the given host is an IPv4 literal address.
   *
   * @param host The string to be checked.
   *
   * @return True, if the host was specified using an IPv4 address.
   */
  static bool is_ipv4(const std::string &host);

  /**
   * Checks if the given host is an IPv6 literal address.
   *
   * @param host The string to be checked.
   *
   * @return True, if the host was specified using an IPv6 address.
   */
  static bool is_ipv6(const std::string &host);

  /**
   * Checks whether the given address is a loopback
   * (localhost, 127.*, ::1)
   *
   * @param  address IP or hostname to check
   * @return         true if the address is a loopback
   */
  static bool is_loopback(const std::string &address);

  /**
   * Checks whether the given address belongs to a network interface of this
   * host.
   *
   * @param  address IP or hostname to check
   * @return         true if the address is local.
   */
  static bool is_local_address(const std::string &address);

  /**
   * Returns the name of this host.
   * @return hostname
   */
  static std::string get_hostname();

  /**
   * Returns fully qualified domain name if provided address is a hostname or IP
   * address if provided address is a IP.
   *
   * @param address Hostname or IP address in text form.
   * @return Fully qualified domain name, canonical name or IP address.
   */
  static std::string get_fqdn(const std::string &address) noexcept;

  /**
   * Check if provided address is externally addressable.
   *
   * @param address Hostname or IP address in text form.
   * @return true if address can be reached from outside.
   * @return false if address belong to loopback interface.
   */
  static bool is_externally_addressable(const std::string &address);

  /**
   * Checks whether a TCP port is busy
   * @param address to check
   * @param port the port number to check
   * @returns true if the port is busy
   */
  static bool is_port_listening(const std::string &address, int port);

  static std::vector<std::string> resolve_hostname_ipv4_all(
      const std::string &name);

  static std::vector<std::string> resolve_hostname_ipv6_all(
      const std::string &name);

  /**
   * Gets a list of all IPs (IPv4 and IPv6) that the hostname resolves to.
   *
   * @param name The hostname to be resolved.
   *
   * @return A list with the IPv4 and IPv6 addresses the hostname resolves to.
   * @throws net_error if address cannot be resolved.
   */
  static std::vector<std::string> get_hostname_ips(const std::string &name);

  /**
   * Strips the CIDR value from the given address and converts it to integer
   *
   * @param address The address to be stripped.
   *
   * @return (IP, CIDR) tuple.
   *
   * @throws invalid_argument if the address cannot be parsed
   * @throws out_of_range if the converted integer value of cidr is out of range
   */
  static std::tuple<std::string, mysqlshdk::utils::nullable<int>> strip_cidr(
      const std::string &address);

 protected:
  /**
   * Provides the singleton instance of this class.
   *
   * @return currently used implementation.
   */
  static Net *get();

  /**
   * Overrides default implementation with a custom behaviour.
   *
   * @param implementation An implementation to use, nullptr restores default
   *                       behaviour.
   */
  static void set(Net *implementation);

  virtual std::vector<std::string> resolve_hostname_ipv4_all_impl(
      const std::string &name) const;

  virtual std::vector<std::string> resolve_hostname_ipv6_all_impl(
      const std::string &name) const;

  virtual std::vector<std::string> resolve_hostname_ipv_any_all_impl(
      const std::string &name) const;

  virtual bool is_loopback_impl(const std::string &address) const;

  virtual bool is_local_address_impl(const std::string &address) const;

  virtual std::string get_hostname_impl() const;

  virtual std::string get_fqdn_impl(const std::string &address) const noexcept;

  virtual bool is_port_listening_impl(const std::string &address,
                                      int port) const;

 private:
  static Net *s_implementation;

  static std::vector<std::string> get_local_addresses();

  static std::vector<std::string> get_loopback_addresses();

#ifdef FRIEND_TEST
  FRIEND_TEST(utils_net, get_local_addresses);
  FRIEND_TEST(utils_net, get_loopback_addresses);
#endif
};

/** Splits an address in format <host>:<port>.
 *
 * Supports hostname:port, ipv4:port and [ipv6]:port
 *
 * port value range is checked, but IP or hostname syntax is only roughly
 * checked (as in, only checks for valid characters.)
 *
 * IPv6 addresses are removed from the []
 */
std::pair<std::string, uint16_t> split_host_and_port(const std::string &s);

/** Joins a host and port into a host:port string, enclosing IPv6 addresses
 * within [] if needed.
 */
std::string make_host_and_port(const std::string &host, uint16_t port);

/**
 * Converts the given value from host byte order to network byte order.
 */
uint64_t host_to_network(uint64_t v);

/**
 * Converts the given value from network byte order to host byte order.
 */
uint64_t network_to_host(uint64_t v);

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_NET_H_
