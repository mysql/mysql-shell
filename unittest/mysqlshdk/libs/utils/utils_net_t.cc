/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "unittest/gprod_clean.h"
#include "unittest/gtest_clean.h"

#include "mysqlshdk/libs/utils/utils_net.h"

#include <algorithm>
#include <iterator>

namespace mysqlshdk {
namespace utils {

TEST(utils_net, resolve_hostname_ipv4) {
  EXPECT_EQ("0.0.0.0", Net::resolve_hostname_ipv4("0.0.0.0"));
  EXPECT_EQ("0.0.0.0", Net::resolve_hostname_ipv4("000.000.000.000"));
  EXPECT_EQ("127.0.0.1", Net::resolve_hostname_ipv4("127.0.0.1"));
  EXPECT_EQ("127.0.0.1", Net::resolve_hostname_ipv4("127.000.000.001"));
  EXPECT_EQ("8.8.8.8", Net::resolve_hostname_ipv4("8.8.8.8"));
  EXPECT_EQ("8.8.8.8", Net::resolve_hostname_ipv4("010.010.010.010"));
  EXPECT_EQ("127.0.0.1", Net::resolve_hostname_ipv4("0x7F.0.0.1"));

  EXPECT_NO_THROW(Net::resolve_hostname_ipv4("localhost"));
  EXPECT_NO_THROW(Net::resolve_hostname_ipv4("google.pl"));

#ifdef WIN32
  // On Windows, using an empty string will resolve to any registered
  // address
  EXPECT_NO_THROW(Net::resolve_hostname_ipv4(""));
#else
  EXPECT_THROW(Net::resolve_hostname_ipv4(""), net_error);
#endif  // WIN32

  EXPECT_THROW(Net::resolve_hostname_ipv4("unknown_host"), net_error);
  EXPECT_THROW(Net::resolve_hostname_ipv4("127.0.0.1.."), net_error);
  EXPECT_THROW(Net::resolve_hostname_ipv4("127.0.0.256"), net_error);
}

TEST(utils_net, resolve_hostname_ipv4_all) {
  {
    try {
      std::vector<std::string> addrs =
          Net::resolve_hostname_ipv4_all("localhost");
      for (const auto &a : addrs) {
        std::cout << "localhost: " << a << "\n";
      }
    } catch (const std::runtime_error &err) {
      std::cout << "Unable to resolve hostname localhost as IPv4 address: "
                << err.what() << std::endl;
    }
  }

  {
    try {
      std::vector<std::string> addrs =
          Net::resolve_hostname_ipv4_all("oracle.com");
      for (const auto &a : addrs) {
        std::cout << "oracle.com: " << a << "\n";
      }
    } catch (const std::runtime_error &err) {
      std::cout << "Unable to resolve hostname oracle.com as IPv4 address: "
                << err.what() << std::endl;
    }
  }

  {
    try {
      std::vector<std::string> addrs =
          Net::resolve_hostname_ipv4_all(Net::get_hostname());
      for (const auto &a : addrs) {
        std::cout << Net::get_hostname() << ": " << a << "\n";
      }
    } catch (const std::runtime_error &err) {
      std::cout << "Unable to resolve hostname " << Net::get_hostname()
                << " as IPv4 address: " << err.what() << std::endl;
    }
  }
}

TEST(utils_net, resolve_hostname_ipv6_all) {
  {
    try {
      std::vector<std::string> addrs =
          Net::resolve_hostname_ipv6_all("localhost");

      for (const auto &a : addrs) {
        std::cout << "localhost: " << a << "\n";
      }
    } catch (const std::runtime_error &err) {
      std::cout << "Unable to resolve hostname localhost as IPv6 address: "
                << err.what() << std::endl;
    }
  }

  {
    try {
      std::vector<std::string> addrs =
          Net::resolve_hostname_ipv6_all("google.com");
      for (const auto &a : addrs) {
        std::cout << "google.com: " << a << "\n";
      }
    } catch (const std::runtime_error &err) {
      std::cout << "Unable to resolve hostname google.com as IPv6 address: "
                << err.what() << std::endl;
    }
  }

  {
    try {
      std::vector<std::string> addrs =
          Net::resolve_hostname_ipv6_all(Net::get_hostname());
      for (const auto &a : addrs) {
        std::cout << Net::get_hostname() << ": " << a << "\n";
      }
    } catch (const std::runtime_error &err) {
      std::cout << "Unable to resolve hostname " << Net::get_hostname()
                << " as IPv6 address: " << err.what() << std::endl;
    }
  }
}

TEST(utils_net, get_hostname_ips) {
  {
    try {
      std::vector<std::string> addrs = Net::get_hostname_ips("localhost");
      for (const auto &a : addrs) {
        std::cout << "localhost: " << a << "\n";
      }
    } catch (const std::runtime_error &err) {
      std::cout << "Unable to resolve hostname localhost: " << err.what()
                << std::endl;
    }
  }

  {
    try {
      std::vector<std::string> addrs = Net::get_hostname_ips("oracle.com");
      for (const auto &a : addrs) {
        std::cout << "google.com: " << a << "\n";
      }
    } catch (const std::runtime_error &err) {
      std::cout << "Unable to resolve hostname oracle.com: " << err.what()
                << std::endl;
    }
  }

  {
    try {
      std::vector<std::string> addrs =
          Net::get_hostname_ips(Net::get_hostname());
      for (const auto &a : addrs) {
        std::cout << Net::get_hostname() << ": " << a << "\n";
      }
    } catch (const std::runtime_error &err) {
      std::cout << "Unable to resolve hostname " << Net::get_hostname() << ": "
                << err.what() << std::endl;
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

// TODO(someone): For some reason this address is not resolved as IPv6 in
// Solaris, failed finding out the reason, will let it as a TODO for further
// investigation.
#ifndef __sun
  EXPECT_TRUE(Net::is_ipv6("::FFFF:129.144.52.38"));
#endif

  EXPECT_TRUE(Net::is_ipv6("2010:836B:4179::836B:4179"));
  // IPv6 scoped addressing zone identifiers
  EXPECT_TRUE(Net::is_ipv6("fe80::850a:5a7c:6ab7:aec4%1"));
  EXPECT_TRUE(Net::is_ipv6("fe80::850a:5a7c:6ab7:aec4%eth0"));
  EXPECT_TRUE(Net::is_ipv6("fe80::850a:5a7c:6ab7:aec4%enp0s3"));

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
  EXPECT_FALSE(Net::is_loopback("127.bing.com"));
}

TEST(utils_net, is_local_address) {
  std::cout << "Net::get_hostname() = " << Net::get_hostname() << std::endl;
  EXPECT_TRUE(Net::is_local_address(Net::get_hostname()));
  EXPECT_TRUE(Net::is_local_address("localhost"));
  EXPECT_TRUE(Net::is_local_address("127.0.0.1"));
  EXPECT_TRUE(
      Net::is_local_address(Net::resolve_hostname_ipv4(Net::get_hostname())));

  EXPECT_FALSE(Net::is_local_address("127.bing.com"));
  EXPECT_FALSE(Net::is_local_address("oracle.com"));
  EXPECT_FALSE(Net::is_local_address("bogus-host"));
  EXPECT_FALSE(Net::is_local_address("8.8.8.8"));
}

TEST(utils_net, is_externally_addressable) {
  EXPECT_FALSE(Net::is_externally_addressable("localhost"));
  EXPECT_FALSE(Net::is_externally_addressable("127.0.0.1"));
  EXPECT_FALSE(Net::is_externally_addressable("127.0.1.1"));
  EXPECT_FALSE(Net::is_externally_addressable("127.0.2.1"));
  EXPECT_FALSE(Net::is_externally_addressable("127.1.2.1"));

  const auto hostname = Net::get_hostname();
  const auto self_fqdn = Net::get_fqdn(hostname);
  const std::string domain{".oracle.com"};

  auto contains_domain = std::search(self_fqdn.begin(), self_fqdn.end(),
                                     domain.begin(), domain.end());
  if (contains_domain != self_fqdn.end()) {
    std::advance(contains_domain, domain.size());
    if (contains_domain == self_fqdn.end()) {
      EXPECT_TRUE(Net::is_externally_addressable(self_fqdn));
    }
  }
  EXPECT_TRUE(Net::is_externally_addressable("oracle.com"));
  EXPECT_TRUE(Net::is_externally_addressable("8.8.4.4"));
  EXPECT_TRUE(Net::is_externally_addressable("127.bing.com"));
}

TEST(utils_net, is_port_listening) {
  int port = std::atoi(getenv("MYSQL_PORT"));
  EXPECT_TRUE(Net::is_port_listening("localhost", port));
#ifndef _WIN32
  // it's not possible to check if 0.0.0.0:port is in use on Windows,
  // 0.0.0.0 address is not allowed
  EXPECT_TRUE(Net::is_port_listening("0.0.0.0", port));
#endif
  EXPECT_TRUE(Net::is_port_listening("127.0.0.1", port));

  EXPECT_FALSE(Net::is_port_listening("localhost", 1));

  EXPECT_THROW(Net::is_port_listening("someunexistingsite.com", 0),
               std::runtime_error);
}

TEST(utils_net, get_local_addresses) {
  // Not really a unit-test, just get whatever get_local_addresses() returns
  // and print out, so we can inspect visually...
  for (const auto &a : Net::get_local_addresses()) {
    std::cout << a << "\n";
  }
}

TEST(utils_net, get_loopback_addresses) {
  // Not really a unit-test, just get whatever get_loopback_addresses()
  // returns and print out, so we can inspect visually...
  for (const auto &a : Net::get_loopback_addresses()) {
    std::cout << a << "\n";
  }
}

TEST(utils_net, strip_cidr) {
  // Test invalid_argument
  {
    const std::string address = "192.168.1.1/a bad value 1";
    EXPECT_THROW(Net::strip_cidr(address), std::invalid_argument);
  }

  // Test missing cidr
  {
    const std::string address = "192.168.1.1/";
    EXPECT_THROW(Net::strip_cidr(address), std::invalid_argument);
  }

  // Address without CIDR value should return false
  {
    const std::string address = "192.168.1.1";
    auto ip = Net::strip_cidr(address);
    EXPECT_FALSE(!std::get<1>(ip).is_null());
  }

  // Address with CIDR value should return true and the values
  // should be set
  {
    const std::string address = "192.168.1.1/8";
    auto ip = Net::strip_cidr(address);
    EXPECT_TRUE(!std::get<1>(ip).is_null());
    EXPECT_EQ("192.168.1.1", std::get<0>(ip));
    EXPECT_EQ(8, *std::get<1>(ip));
  }
}

}  // namespace utils
}  // namespace mysqlshdk
