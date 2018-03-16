/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <gtest/gtest_prod.h>
#include "mysqlshdk/libs/utils/utils_net.h"
#include "unittest/gtest_clean.h"

namespace mysqlshdk {
namespace utils {

TEST(utils_net, resolve_hostname_ipv4) {
  EXPECT_EQ("0.0.0.0", Net::resolve_hostname_ipv4("0.0.0.0"));
  EXPECT_EQ("000.000.000.000", Net::resolve_hostname_ipv4("000.000.000.000"));
  EXPECT_EQ("127.0.0.1", Net::resolve_hostname_ipv4("127.0.0.1"));
  EXPECT_EQ("127.000.000.001", Net::resolve_hostname_ipv4("127.000.000.001"));
  EXPECT_EQ("8.8.8.8", Net::resolve_hostname_ipv4("8.8.8.8"));
  EXPECT_EQ("010.010.010.010", Net::resolve_hostname_ipv4("010.010.010.010"));
  EXPECT_EQ("0x7F.0.0.1", Net::resolve_hostname_ipv4("0x7F.0.0.1"));

  EXPECT_NO_THROW(Net::resolve_hostname_ipv4("localhost"));
  EXPECT_NO_THROW(Net::resolve_hostname_ipv4("google.pl"));

#ifdef WIN32
  // On Windows, using an empty string will resolve to any registered
  // address
  EXPECT_NO_THROW(Net::resolve_hostname_ipv4(""), net_error);
#else
  EXPECT_THROW(Net::resolve_hostname_ipv4(""), net_error);
#endif  // WIN32

  EXPECT_THROW(Net::resolve_hostname_ipv4("unknown_host"), net_error);
  EXPECT_THROW(Net::resolve_hostname_ipv4("127.0.0.1.."), net_error);
  EXPECT_THROW(Net::resolve_hostname_ipv4("127.0.0.256"), net_error);
}

TEST(utils_net, resolve_hostname_ipv4_all) {
  {
    std::vector<std::string> addrs =
        Net::resolve_hostname_ipv4_all("localhost");
    for (const auto &a : addrs) {
      std::cout << "localhost: " << a << "\n";
    }
  }
  {
    std::vector<std::string> addrs =
        Net::resolve_hostname_ipv4_all("oracle.com");
    for (const auto &a : addrs) {
      std::cout << "oracle.com: " << a << "\n";
    }
  }
  {
    std::vector<std::string> addrs =
        Net::resolve_hostname_ipv4_all(Net::get_hostname());
    for (const auto &a : addrs) {
      std::cout << Net::get_hostname() << ": " << a << "\n";
    }
  }
}

TEST(utils_net, is_ipv4) {
  EXPECT_TRUE(Net::is_ipv4("0.0.0.0"));
  EXPECT_TRUE(Net::is_ipv4("127.0.0.1"));
  EXPECT_TRUE(Net::is_ipv4("8.8.8.8"));
  EXPECT_TRUE(Net::is_ipv4("255.255.255.255"));
  EXPECT_TRUE(Net::is_ipv4("000.000.000.000"));
  EXPECT_TRUE(Net::is_ipv4("0x7F.0.0.1"));

  EXPECT_FALSE(Net::is_ipv4(""));
  EXPECT_FALSE(Net::is_ipv4("localhost"));
  EXPECT_FALSE(Net::is_ipv4("google.pl"));
  EXPECT_FALSE(Net::is_ipv4("unknown_host"));
  EXPECT_FALSE(Net::is_ipv4("::8.8.8.8"));
  EXPECT_FALSE(Net::is_ipv4("255.255.255.256"));
  EXPECT_FALSE(Net::is_ipv4("2010:836B:4179::836B:4179"));
  EXPECT_FALSE(Net::is_ipv4("FEDC:BA98:7654:3210:FEDC:BA98:7654:3210"));
}

TEST(utils_net, is_ipv6) {
  EXPECT_TRUE(Net::is_ipv6("FEDC:BA98:7654:3210:FEDC:BA98:7654:3210"));
  EXPECT_TRUE(Net::is_ipv6("1080:0:0:0:8:800:200C:4171"));
  EXPECT_TRUE(Net::is_ipv6("3ffe:2a00:100:7031::1"));
  EXPECT_TRUE(Net::is_ipv6("1080::8:800:200C:417A"));
  EXPECT_TRUE(Net::is_ipv6("::192.9.5.5"));
  EXPECT_TRUE(Net::is_ipv6("::FFFF:129.144.52.38"));
  EXPECT_TRUE(Net::is_ipv6("2010:836B:4179::836B:4179"));
  // IPv6 scoped addressing zone identifiers
  EXPECT_TRUE(Net::is_ipv6("fe80::850a:5a7c:6ab7:aec4%1"));

  EXPECT_FALSE(Net::is_ipv6(""));
  EXPECT_FALSE(Net::is_ipv6("localhost"));
  EXPECT_FALSE(Net::is_ipv6("google.pl"));
  EXPECT_FALSE(Net::is_ipv6("unknown_host"));
  EXPECT_FALSE(Net::is_ipv6("127.0.0.1"));
  EXPECT_FALSE(Net::is_ipv6("FEDC:BA98:7654:3210:FEDC:BA98:7654:3210:"));
  EXPECT_FALSE(Net::is_ipv6("FEDC:BA98:7654:3210:GEDC:BA98:7654:3210"));
}

TEST(utils_net, is_loopback) {
  EXPECT_TRUE(Net::is_loopback("127.0.1.1"));
  EXPECT_TRUE(Net::is_loopback("127.0.0.1"));
  EXPECT_TRUE(Net::is_loopback("::1"));
  EXPECT_TRUE(Net::is_loopback("localhost"));

  EXPECT_FALSE(Net::is_loopback("192.168.1.1"));
}

TEST(utils_net, is_local_address) {
  std::cout << "Net::get_hostname() = " << Net::get_hostname() << std::endl;
  EXPECT_TRUE(Net::is_local_address(Net::get_hostname()));
  EXPECT_TRUE(Net::is_local_address("localhost"));
  EXPECT_TRUE(Net::is_local_address("127.0.0.1"));
  EXPECT_TRUE(
      Net::is_local_address(Net::resolve_hostname_ipv4(Net::get_hostname())));

  EXPECT_FALSE(Net::is_local_address("oracle.com"));
  EXPECT_FALSE(Net::is_local_address("bogus-host"));
}

TEST(utils_net, get_local_addresses) {
  // Not really a unit-test, just get whatever get_local_addresses() returns
  // and print out, so we can inspect visually...
  std::vector<std::string> addrs;
  Net::get_local_addresses(&addrs);
  for (const auto &a : addrs) {
    std::cout << a << "\n";
  }
}

}  // namespace utils
}  // namespace mysqlshdk
