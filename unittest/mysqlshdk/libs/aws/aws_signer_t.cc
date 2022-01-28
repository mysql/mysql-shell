/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/aws/aws_signer.h"

#include "unittest/gtest_clean.h"

#include <set>
#include <string>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace aws {

class Aws_signer_test : public testing::Test {
 protected:
  static Aws_signer create_signer() {
    Aws_signer signer;

    signer.m_host = k_host;
    signer.m_access_key_id = k_access_key_id;
    signer.set_secret_access_key("wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY");
    signer.m_region = k_region;
    signer.m_sign_all_headers = true;

    return signer;
  }

  static void test_sign_request(
      const rest::Signed_request *request, const std::string &signature,
      const std::string &sha256 =
          "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855") {
    const auto headers = create_signer().sign_request(request, k_now);

    ASSERT_NE(headers.end(), headers.find("Host"));
    EXPECT_EQ(k_host, headers.at("Host"));

    ASSERT_NE(headers.end(), headers.find("x-amz-content-sha256"));
    EXPECT_EQ(sha256, headers.at("x-amz-content-sha256"));

    ASSERT_NE(headers.end(), headers.find("x-amz-date"));
    EXPECT_EQ("20130524T000000Z", headers.at("x-amz-date"));

    ASSERT_NE(headers.end(), headers.find("Authorization"));

    std::set<std::string> signed_headers = {
        "host",
        "x-amz-content-sha256",
        "x-amz-date",
    };

    for (const auto &header : request->headers()) {
      signed_headers.emplace(shcore::str_lower(header.first));
    }

    std::string authorization = "AWS4-HMAC-SHA256 Credential=";
    authorization += k_access_key_id;
    authorization += '/';
    authorization += k_now_date;
    authorization += '/';
    authorization += k_region;
    authorization += "/s3/aws4_request, SignedHeaders=";
    authorization += shcore::str_join(signed_headers, ";");
    authorization += ", Signature=";
    authorization += signature;

    EXPECT_EQ(authorization, headers.at("Authorization"));
  }

 private:
  // Friday, 24 May 2013 00:00:00
  static constexpr time_t k_now = 1369353600;

  static constexpr auto k_now_date = "20130524";

  static constexpr auto k_host = "examplebucket.s3.amazonaws.com";

  static constexpr auto k_access_key_id = "AKIAIOSFODNN7EXAMPLE";

  static constexpr auto k_region = "us-east-1";
};

TEST_F(Aws_signer_test, get_object) {
  rest::Signed_request request{"/test.txt", {{"Range", "bytes=0-9"}}};
  request.type = rest::Type::GET;

  test_sign_request(
      &request,
      "f0e8bdb87c964420e857bd35b5d6ed310bd44f0170aba48dd91039c6036bdb41");
}

TEST_F(Aws_signer_test, put_object) {
  rest::Signed_request request{"/test%24file.text",
                               {{"Date", "Fri, 24 May 2013 00:00:00 GMT"},
                                {"x-amz-storage-class", "REDUCED_REDUNDANCY"}}};
  request.type = rest::Type::PUT;

  const std::string data = "Welcome to Amazon S3.";
  request.body = data.c_str();
  request.size = data.length();

  test_sign_request(
      &request,
      "98ad721746da40c64f1a55b78f14c238d841ea1380cd77a1b5971af0ece108bd",
      "44ce7dd67c959e0d3524ffac1771dfbba87d2b6b4b4e99e42034a8b803f8b072");
}

TEST_F(Aws_signer_test, get_bucket_lifecycle) {
  rest::Signed_request request{"/?lifecycle"};
  request.type = rest::Type::GET;

  test_sign_request(
      &request,
      "fea454ca298b7da1c68078a5d1bdbfbbe0d65c699e0f91ac7a200a0136783543");
}

TEST_F(Aws_signer_test, list_objects) {
  rest::Signed_request request{"/?max-keys=2&prefix=J"};
  request.type = rest::Type::GET;

  test_sign_request(
      &request,
      "34b48302e7b5fa45bde8084f4b7868a86f0a534bc59db6670ed5711ef69dc6f7");
}

}  // namespace aws
}  // namespace mysqlshdk
