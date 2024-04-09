/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <string_view>

#include "mysqlshdk/libs/utils/utils_x509.h"

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

namespace shcore {
namespace ssl {
namespace {

constexpr std::string_view k_x509_cert = R"(-----BEGIN CERTIFICATE-----
MIIGITCCBAmgAwIBAgIUJJ2WSa2GCL8NlNcxpL1qPq2lCpcwDQYJKoZIhvcNAQEL
BQAwgZ8xCzAJBgNVBAYTAlVTMRIwEAYDVQQIDAlTdGF0ZU5hbWUxFTATBgNVBAcM
DERlZmF1bHQgQ2l0eTEcMBoGA1UECgwTRGVmYXVsdCBDb21wYW55IEx0ZDEQMA4G
A1UECwwHU2VjdGlvbjETMBEGA1UEAwwKQ29tbW9uTmFtZTEgMB4GCSqGSIb3DQEJ
ARYRZW1haWxAYWRkcmVzcy5jb20wHhcNMjQwNDIyMTY0MjI1WhcNMjUwNDIyMTY0
MjI1WjCBnzELMAkGA1UEBhMCVVMxEjAQBgNVBAgMCVN0YXRlTmFtZTEVMBMGA1UE
BwwMRGVmYXVsdCBDaXR5MRwwGgYDVQQKDBNEZWZhdWx0IENvbXBhbnkgTHRkMRAw
DgYDVQQLDAdTZWN0aW9uMRMwEQYDVQQDDApDb21tb25OYW1lMSAwHgYJKoZIhvcN
AQkBFhFlbWFpbEBhZGRyZXNzLmNvbTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCC
AgoCggIBALakffOjGQHPRfJ8w8em+gJgIoFgcfRRkIgOG2S8sgdNdZ0dhrzsH6Oi
n/wbv8ADZd3paY257AFLDqanNxfKyppd1C8PWONCh86NDO43KXtnDeYKJUej8AN3
PBHSy9LaxiNGLqqK6kjYe1mZWA/dLNRh0yVDdW5T7mI1dbqb6UT6aCnqFXCf0a59
cTEIHVGM1qLievNzuBPpyF13bGNjKDBaLeNclNZT8RS42Hl2Wg2cSiFrGsn/CsCv
dZvO1b9zkBp+l0Jbh+ySu2JmfiQF4S1VnQ4H5GBelj8AEGVKhkWOwV5febwCHqIv
LNmMNHrJMVeuu6MoFb3DBq68NqcdTLOUgZqdF+NVFiuPJLgLauQZcFWJFlkzFCmV
C3bjIeBPNapVsPFRCCqPV+rS4skg4xlrDx4av50U6qsPZ2UCaI75TSzT+vM6gOO3
HK/5jT5s9+LEZ+Faq0yFhlxM5if2Y9dq7ryhSzPt/tafDCQWIg1lHGGOMFp7dU11
kHF5hyCYEKUUxN9M9ViFSEgcyoSJ3iY4nL4dN1uquvRYJFuMl2Yx5y9L7WY3uMuS
aXRyQJ8IdUs7ti+0O5u8ALBBgAODlIJLmWGKyqsFwnaZ3kteD8UvjD16lxbZLToW
81mEZjmeXwztXnsZZoNr0ak29DnQCU8gl7swGpeV15yMhespJKpVAgMBAAGjUzBR
MB0GA1UdDgQWBBSPyeMzIbs9CWcXQr0ShaQmVb7TAjAfBgNVHSMEGDAWgBSPyeMz
Ibs9CWcXQr0ShaQmVb7TAjAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUA
A4ICAQAspgtbLLjGnSG6lQMRsSgcasEJQZwfB6fa0acJvURjd7C6Uthd9Kerknyh
W0HHHfwZ8r1+sWE0L2cxCkVwquT3VE1SauvdRgxKDYUwxIcUnOVy96CokF8WXABs
MyaXOO+FoQ55BFX4009+GtjVKNwRPXSOXYl9s6VBqIKdlv2pkOIUn2lqrzQ7EZ1d
pvsG2ud851ir3ISjrj82pXfAW9tGRkNXngr2oD1TNktS/g/Nz/RjvuAH9sIX4URQ
94qX+J1zu5Yzx2RMEOwTAHbJmXmdot7WgR1j4hewkXbB3J1XHQ2VvHyDWPO9EYPK
nRAxgAZab4HpZw8lZlRyU0M11qj4izVLqpG44ee2YHVqLK14O0cZgUhxQVfspvw2
0Yy1BEMZdAJkqomeG6bJDdRPnMWpK67RTLuRQYom4WdvhWmmFmNcYHrxBNju4GRu
rbTxuhec+Tt1MbZksQFDSuanIob1gziKIDShcbcpGDbQWYEKldp6CgEDiqRld4ta
W7/DuyXdZy1Peo9JZSJBFWZx7H+KZP5HjNO5szf+6+A/xKGoSW/Z/r5MmrJZ4BKb
SOkeqoPbZQrLwYiBLS7fMRJiqrJ39q59S/2iDwoMg3+GQgHauf157+VLBoNgp0se
XDYG4zcGKC22ZslFYgqAe4z7NGBtNOMrmzYM/SvVDIlPqpHltw==
-----END CERTIFICATE-----
)";

TEST(Utils_x509_test, from_string) {
  const auto cert = X509_certificate::from_string(k_x509_cert);
  const auto subject = cert.subject();

  ASSERT_EQ(7, subject.size());

  EXPECT_EQ("countryName", subject[0].name);
  EXPECT_EQ("US", subject[0].value);

  EXPECT_EQ("stateOrProvinceName", subject[1].name);
  EXPECT_EQ("StateName", subject[1].value);

  EXPECT_EQ("localityName", subject[2].name);
  EXPECT_EQ("Default City", subject[2].value);

  EXPECT_EQ("organizationName", subject[3].name);
  EXPECT_EQ("Default Company Ltd", subject[3].value);

  EXPECT_EQ("organizationalUnitName", subject[4].name);
  EXPECT_EQ("Section", subject[4].value);

  EXPECT_EQ("commonName", subject[5].name);
  EXPECT_EQ("CommonName", subject[5].value);

  EXPECT_EQ("emailAddress", subject[6].name);
  EXPECT_EQ("email@address.com", subject[6].value);

  EXPECT_EQ(1713804145, cert.not_before());
  EXPECT_EQ(1745340145, cert.not_after());

  EXPECT_EQ("e0:2f:43:a1:7a:f7:06:dc:0a:c3:20:78:d9:07:08:3a",
            cert.fingerprint(Hash::RESTRICTED_MD5));
  EXPECT_EQ("7f:7a:ab:e2:3f:e8:c8:be:95:9f:ef:dc:10:3b:4a:c9:1b:8e:4c:c7",
            cert.fingerprint(Hash::RESTRICTED_SHA1));
  EXPECT_EQ(
      "d9:d8:33:52:21:c1:90:a6:3e:ee:99:85:81:ee:7b:4a:77:63:be:0a:3d:fd:79:6d:"
      "b7:d6:f8:bb:54:f9:da:36",
      cert.fingerprint(Hash::SHA256));

  EXPECT_EQ(k_x509_cert, cert.to_string());
}

}  // namespace
}  // namespace ssl
}  // namespace shcore
