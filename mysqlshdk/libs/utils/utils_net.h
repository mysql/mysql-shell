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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_NET_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_NET_H_

#include <stdexcept>
#include <string>

namespace mysqlshdk {
namespace utils {

class net_error : public std::runtime_error {
 public:
  explicit net_error(const std::string &what) : std::runtime_error(what) {
  }
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

  /**
   * Implementation of resolve_hostname_ipv4() method.
   */
  virtual std::string resolve_hostname_ipv4_impl(const std::string &name) const;

 private:
  static Net *s_implementation;
};

}  // namespace utils
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_NET_H_
