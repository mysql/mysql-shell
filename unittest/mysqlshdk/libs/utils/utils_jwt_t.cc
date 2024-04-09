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

#include "mysqlshdk/libs/utils/utils_jwt.h"

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

namespace shcore {
namespace {

TEST(utils_jwt, from_string) {
  const auto jwt = Jwt::from_string(
      "eyJ0eXAiOiJKV1QiLA0KICJhbGciOiJIUzI1NiJ9."
      "eyJpc3MiOiJqb2UiLA0KICJleHAiOjEzMDA4MTkzODAsDQogImh0dHA6Ly9leGFtcGxlLmNv"
      "bS9pc19yb290Ijp0cnVlfQ.dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk");

  EXPECT_EQ("JWT", jwt.header().get_string("typ"));
  EXPECT_EQ("HS256", jwt.header().get_string("alg"));

  EXPECT_EQ("joe", jwt.payload().get_string("iss"));
  EXPECT_EQ(1300819380, jwt.payload().get_uint("exp"));
  EXPECT_EQ(1300819380, jwt.payload().get_int("exp"));

  // missing key
  EXPECT_THROW(jwt.payload().get_string("iat"), std::runtime_error);
  EXPECT_THROW(jwt.payload().get_uint("iat"), std::runtime_error);
  EXPECT_THROW(jwt.payload().get_int("iat"), std::runtime_error);

  // wrong type
  EXPECT_THROW(jwt.payload().get_string("exp"), std::runtime_error);
  EXPECT_THROW(jwt.payload().get_uint("iss"), std::runtime_error);
  EXPECT_THROW(jwt.payload().get_int("iss"), std::runtime_error);
}

}  // namespace
}  // namespace shcore
