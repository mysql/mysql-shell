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

#include "unittest/gtest_clean.h"

#include "mysqlshdk/libs/utils/utils_encoding.h"

namespace shcore {
namespace {

constexpr std::string_view k_test_string = R"(
Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor
incididunt ut labore et dolore magna aliqua.
)";

constexpr std::string_view k_test_string_base64 =
    R"(CkxvcmVtIGlwc3VtIGRvbG9yIHNpdCBhbWV0LCBjb25zZWN0ZXR1ciBhZGlwaXNjaW5nIGVsaXQsIHNlZCBkbyBlaXVzbW9kIHRlbXBvcgppbmNpZGlkdW50IHV0IGxhYm9yZSBldCBkb2xvcmUgbWFnbmEgYWxpcXVhLgo=)";

TEST(Utils_encoding, encode_base64) {
  std::string dst;

  EXPECT_TRUE(encode_base64(k_test_string, &dst));
  EXPECT_EQ(k_test_string_base64, dst);
}

TEST(Utils_encoding, decode_base64) {
  std::string dst;

  EXPECT_TRUE(decode_base64(k_test_string_base64, &dst));
  EXPECT_EQ(k_test_string, dst);
}

constexpr char k_test_data_array[] = {
    0x0B,
    0x5F,
    static_cast<char>(0xA1),
    0x69,
    static_cast<char>(0x8F),
    0x26,
    static_cast<char>(0xC9),
    static_cast<char>(0x8F),
    static_cast<char>(0xF0),
    0x32,
    static_cast<char>(0x94),
    0x58,
};
constexpr std::string_view k_test_data{k_test_data_array,
                                       std::size(k_test_data_array)};

constexpr std::string_view k_test_data_base64url = "C1-haY8myY_wMpRY";

TEST(Utils_encoding, encode_base64url) {
  std::string dst;

  EXPECT_TRUE(encode_base64url(k_test_data, &dst));
  EXPECT_EQ(k_test_data_base64url, dst);

  EXPECT_TRUE(
      encode_base64url(k_test_data.substr(0, k_test_data.length() - 1), &dst));
  EXPECT_EQ("C1-haY8myY_wMpQ", dst);

  EXPECT_TRUE(
      encode_base64url(k_test_data.substr(0, k_test_data.length() - 2), &dst));
  EXPECT_EQ("C1-haY8myY_wMg", dst);

  EXPECT_TRUE(
      encode_base64url(k_test_data.substr(0, k_test_data.length() - 3), &dst));
  EXPECT_EQ(k_test_data_base64url.substr(0, k_test_data_base64url.length() - 4),
            dst);
}

TEST(Utils_encoding, decode_base64url) {
  std::string dst;

  EXPECT_TRUE(decode_base64url(k_test_data_base64url, &dst));
  EXPECT_EQ(k_test_data, dst);

  EXPECT_TRUE(decode_base64url(
      k_test_data_base64url.substr(0, k_test_data_base64url.length() - 1),
      &dst));
  EXPECT_EQ(k_test_data.substr(0, k_test_data.length() - 1), dst);

  EXPECT_TRUE(decode_base64url(
      k_test_data_base64url.substr(0, k_test_data_base64url.length() - 2),
      &dst));
  EXPECT_EQ(k_test_data.substr(0, k_test_data.length() - 2), dst);

  EXPECT_THROW(decode_base64url(k_test_data_base64url.substr(
                                    0, k_test_data_base64url.length() - 3),
                                &dst),
               std::runtime_error);

  EXPECT_TRUE(decode_base64url(
      k_test_data_base64url.substr(0, k_test_data_base64url.length() - 4),
      &dst));
  EXPECT_EQ(k_test_data.substr(0, k_test_data.length() - 3), dst);
}

}  // namespace
}  // namespace shcore
