/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#include "unittest/gprod_clean.h"

#include "mysqlshdk/libs/utils/utils_ssl.h"

#include "unittest/gtest_clean.h"

namespace shcore {
namespace ssl {
namespace {

TEST(utils_ssl, md5) {
  const std::string data = "abcABC";
  std::vector<unsigned char> expected{
      0x0A, 0xCE, 0x32, 0x55, 0x45, 0x11, 0x9A, 0xC9,
      0x9F, 0x35, 0xA5, 0x8E, 0x04, 0xAC, 0x2D, 0xF1,
  };
  EXPECT_EQ(expected, restricted::md5(data));
}

TEST(utils_ssl, sha1) {
  const std::string data = "abcABC";
  std::vector<unsigned char> expected{
      0x46, 0x2C, 0x34, 0xC4, 0xD1, 0x76, 0xB5, 0x0B, 0xC3, 0x1C,
      0xD3, 0xA6, 0x27, 0xED, 0x44, 0x67, 0x13, 0xF9, 0xF5, 0x85,
  };
  EXPECT_EQ(expected, restricted::sha1(data));
}

TEST(utils_ssl, sha256) {
  const std::string data = "abcABC";
  std::vector<unsigned char> expected{
      0x5D, 0x59, 0xEC, 0x3D, 0x67, 0x72, 0x1A, 0xBC, 0xF3, 0xE0, 0x5C,
      0xD9, 0x85, 0x94, 0x35, 0xB3, 0x66, 0xA0, 0x8E, 0xAC, 0x6D, 0x7C,
      0xAA, 0x9D, 0xDE, 0xD3, 0x71, 0xD4, 0x90, 0xB5, 0xB4, 0xCF,
  };
  EXPECT_EQ(expected, sha256(data));
}

TEST(utils_ssl, hmac_sha256) {
  const std::string data = "abcABC";
  std::vector<unsigned char> key{
      0x01,
      0x02,
      0x03,
      0x04,
  };
  std::vector<unsigned char> expected{
      0x53, 0x64, 0xF6, 0x58, 0x14, 0x45, 0xD5, 0x69, 0x34, 0x22, 0x58,
      0x79, 0xA5, 0x00, 0x5F, 0xB2, 0x53, 0xEA, 0x5F, 0x4A, 0x5B, 0xBF,
      0x00, 0x42, 0x06, 0x37, 0xCD, 0xF6, 0x16, 0x06, 0xA0, 0xC0,
  };
  EXPECT_EQ(expected, hmac_sha256(key, data));
}

}  // namespace
}  // namespace ssl
}  // namespace shcore
