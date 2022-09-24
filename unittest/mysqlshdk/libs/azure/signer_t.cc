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

#include "mysqlshdk/libs/azure/signer.h"
#include "mysqlshdk/libs/utils/utils_file.h"

#include <set>
#include <string>

#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/test_utils/mod_testutils.h"

#include "unittest/mysqlshdk/libs/azure/azure_tests.h"

namespace mysqlshdk {
namespace azure {

class Azure_signer_test : public testing::Test {
 public:
  static void SetUpTestCase() {
    shcore::create_file(
        k_config_file,
        "[storage]\nconnection_string="
        "DefaultEndpointsProtocol=http;AccountName=devstoreaccount1;AccountKey="
        "Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/"
        "K1SZFPTOtr/KBHBeksoGMGw==;BlobEndpoint=http://127.0.0.1:10000/"
        "devstoreaccount1;",
        false);
  }

  static void TearDownTestCase() { shcore::delete_file(k_config_file); }

  void SetUp() override {
    const auto options = shcore::make_dict(
        Blob_storage_options::container_name_option(), "testcontainer",
        Blob_storage_options::storage_account_option(), "devstoreaccount1",
        Blob_storage_options::config_file_option(), k_config_file);

    Blob_storage_options::options().unpack(options, &m_options);
    m_config = std::make_shared<Blob_storage_config>(m_options, false);
    m_signer = m_config->signer();
    m_azure_signer = dynamic_cast<mysqlshdk::azure::Signer *>(m_signer.get());
  }

 protected:
  Blob_storage_options m_options;
  std::shared_ptr<Blob_storage_config> m_config;
  std::unique_ptr<rest::Signer> m_signer;
  mysqlshdk::azure::Signer *m_azure_signer;

  // Friday, 24 May 2013 00:00:00
  static constexpr time_t k_now = 1369353600;
  static constexpr char k_authorization[] = "authorization";
  static constexpr char k_x_ms_date[] = "x-ms-date";
  static constexpr char k_x_ms_version[] = "x-ms-version";
  static constexpr char k_x_ms_date_value[] = "Fri, 24 May 2013 00:00:00 GMT";
  static constexpr char k_x_ms_version_value[] = "2021-08-06";
  static constexpr char k_config_file[] = "azure_signer.cfg";

  std::string missing_header(const std::string &name) {
    return shcore::str_format("Missing '%s header.", name.c_str());
  }

  std::string mismatched_header(const std::string &name,
                                const std::string &expected,
                                const std::string &actual) {
    return shcore::str_format(
        "Mismatched '%s header.\n-- Expecting: '%s'\n-- Actual: '%s'",
        name.c_str(), expected.c_str(), actual.c_str());
  }

  void test_sign_request(const std::string &context,
                         const rest::Signed_request *request,
                         const std::string &signature,
                         const std::string &expected_signature_prefix,
                         const std::string &canonical_resource,
                         const std::string &canonical_headers =
                             "x-ms-date:Fri, 24 May 2013 00:00:00 "
                             "GMT\nx-ms-version:2021-08-06\n") {
    // Verifies the signature headers
    SCOPED_TRACE(context.c_str());
    auto rheaders = m_azure_signer->get_required_headers(request, k_now);
    EXPECT_EQ(canonical_headers,
              m_azure_signer->get_canonical_headers(rheaders));

    // Verifies the signature result
    EXPECT_EQ(canonical_resource,
              m_azure_signer->get_canonical_resource(request));

    auto string_to_sign = m_azure_signer->get_string_to_sign(request, rheaders);
    auto actual_signature_prefix = string_to_sign.substr(
        0, string_to_sign.size() -
               (canonical_headers.size() + canonical_resource.size()));
    EXPECT_EQ(expected_signature_prefix, actual_signature_prefix);

    const auto signer_headers = m_azure_signer->sign_request(request, k_now);

    ASSERT_NE(signer_headers.end(), signer_headers.find(k_authorization));
    EXPECT_EQ(signature, signer_headers.at(k_authorization));

    ASSERT_NE(signer_headers.end(), signer_headers.find(k_x_ms_date));
    EXPECT_EQ(k_x_ms_date_value, signer_headers.at(k_x_ms_date));

    ASSERT_NE(signer_headers.end(), signer_headers.find(k_x_ms_version));
    EXPECT_EQ(k_x_ms_version_value, signer_headers.at(k_x_ms_version));

    for (const auto &header : request->headers()) {
      ASSERT_NE(signer_headers.end(), signer_headers.find(header.first));
      EXPECT_EQ(header.second, signer_headers.at(header.first));
    }
  }
};

TEST_F(Azure_signer_test, azure_requests) {
  Blob_container container(m_config);

  auto request = container.list_objects_request("", 0, true, {}, "");
  request.type = mysqlshdk::rest::Type::GET;
  test_sign_request(
      "LIST OBJECTS", &request,
      "SharedKey "
      "devstoreaccount1:EAGKNX/hpcvYJj5Zyoe8zNu/DtS7ChpE4STrNGkaDXA=",
      "GET\n\n\n\n\n\n\n\n\n\n\n\n",
      "/devstoreaccount1/devstoreaccount1/"
      "testcontainer\ncomp:list\nrestype:container");

  request = container.head_object_request("sample.txt");
  request.type = mysqlshdk::rest::Type::HEAD;
  test_sign_request(
      "HEAD OBJECT", &request,
      "SharedKey "
      "devstoreaccount1:Hdob74EiTY2Kkw19XLF2RTX8EnFpxgeG2xB3vwrKeyU=",
      "HEAD\n\n\n\n\n\n\n\n\n\n\n\n",
      "/devstoreaccount1/devstoreaccount1/testcontainer/sample.txt");

  std::string data = "sample.txt";
  request = container.put_object_request(
      "sample.txt", {{"x-ms-blob-type", "BlockBlob"},
                     {"content-type", "application/octet-stream"}});
  request.type = mysqlshdk::rest::Type::PUT;
  request.body = data.data();
  request.size = data.size();
  test_sign_request(
      "PUT OBJECT", &request,
      "SharedKey "
      "devstoreaccount1:g4WMGJTsSBzqltLGJTX7/iVF5YL0bqHhbmjDUkLFZy8=",
      "PUT\n\n\n10\n\napplication/octet-stream\n\n\n\n\n\n\n",
      "/devstoreaccount1/devstoreaccount1/testcontainer/sample.txt",
      "x-ms-blob-type:BlockBlob\nx-ms-date:Fri, 24 May 2013 00:00:00 "
      "GMT\nx-ms-version:2021-08-06\n");

  request = container.delete_object_request("sample.txt");
  request.type = mysqlshdk::rest::Type::DELETE;
  test_sign_request(
      "DELETE OBJECT", &request,
      "SharedKey "
      "devstoreaccount1:mT3CMI0wxLRWM6w/gXp85tzJKqjkQfsof045ht2OkMs=",
      "DELETE\n\n\n\n\napplication/x-www-form-urlencoded\n\n\n\n\n\n\n",
      "/devstoreaccount1/devstoreaccount1/testcontainer/sample.txt");

  request = container.get_object_request("sample.txt", {});
  request.type = mysqlshdk::rest::Type::GET;
  test_sign_request(
      "GET OBJECT", &request,
      "SharedKey "
      "devstoreaccount1:kZ3c3DgrUmJHuRDBnAJJPNvXjKR7Tqfkxk286v3E2D8=",
      "GET\n\n\n\n\n\n\n\n\n\n\n\n",
      "/devstoreaccount1/devstoreaccount1/testcontainer/sample.txt");

  request = container.get_object_request("sample.txt", {{"range", "bytes=5-"}});
  request.type = mysqlshdk::rest::Type::GET;
  test_sign_request(
      "GET OBJECT FROM BYTE", &request,
      "SharedKey "
      "devstoreaccount1:6QOWvxuB39OLiIZ9KqsXSns8Rx7T+KSXmA94JjbmnL4=",
      "GET\n\n\n\n\n\n\n\n\n\n\nbytes=5-\n",
      "/devstoreaccount1/devstoreaccount1/testcontainer/sample.txt");

  request =
      container.get_object_request("sample.txt", {{"range", "bytes=5-10"}});
  request.type = mysqlshdk::rest::Type::GET;
  test_sign_request(
      "GET OBJECT RANGE", &request,
      "SharedKey "
      "devstoreaccount1:S3rtRFTog4bFkOzIz5lN2q9ErnDCOCX8VphDdTVXekE=",
      "GET\n\n\n\n\n\n\n\n\n\n\nbytes=5-10\n",
      "/devstoreaccount1/devstoreaccount1/testcontainer/sample.txt");

  request = container.list_multipart_uploads_request(0);
  request.type = mysqlshdk::rest::Type::GET;
  test_sign_request(
      "LIST MULTIPART UPLOADS", &request,
      "SharedKey "
      "devstoreaccount1:nKXEe6KsgaLAwUBrWi01Xa17WFQAaLHFUCJLxGRivgs=",
      "GET\n\n\n\n\n\n\n\n\n\n\n\n",
      "/devstoreaccount1/devstoreaccount1/"
      "testcontainer\ncomp:list\ninclude:metadata,uncommittedblobs\nrestype:"
      "container");

  data = "-";
  request = container.create_multipart_upload_request("sample.txt", &data);
  request.type = mysqlshdk::rest::Type::PUT;
  test_sign_request(
      "CREATE MULTIPART UPLOAD", &request,
      "SharedKey "
      "devstoreaccount1:fNOEmX/yjNv36GVD23riZDiWFLYBAs+tSH6GJ3irj4Y=",
      "PUT\n\n\n1\n\napplication/octet-stream\n\n\n\n\n\n\n",
      "/devstoreaccount1/devstoreaccount1/testcontainer/"
      "sample.txt\nblockid:MDAwMDA=\ncomp:block");

  mysqlshdk::storage::backend::object_storage::Multipart_object object;
  object.name = "sample.txt";
  data = "sample.txt";
  request = container.upload_part_request(object, 1, data.size());
  request.body = data.data();
  request.size = data.size();
  request.type = mysqlshdk::rest::Type::PUT;
  test_sign_request(
      "UPLOAD PART", &request,
      "SharedKey "
      "devstoreaccount1:yOkmxDsgVv9JrjcFXDVB5ixPla63dWyhIMd55QTi1+0=",
      "PUT\n\n\n10\n\napplication/octet-stream\n\n\n\n\n\n\n",
      "/devstoreaccount1/devstoreaccount1/testcontainer/"
      "sample.txt\nblockid:MDAwMDE=\ncomp:block");

  request = container.list_multipart_uploaded_parts_request(object, 0);
  request.type = mysqlshdk::rest::Type::GET;
  test_sign_request(
      "LIST MULTIPART UPLOADED PARTS", &request,
      "SharedKey "
      "devstoreaccount1:I75Y7dw6PJ4M0i9AI53WSgFlC3eScA1QWP2WksxeYvE=",
      "GET\n\n\n\n\n\n\n\n\n\n\n\n",
      "/devstoreaccount1/devstoreaccount1/testcontainer/"
      "sample.txt\nblocklisttype:uncommitted\ncomp:blocklist");

  request = container.commit_multipart_upload_request(object, {}, &data);
  request.body = data.c_str();
  request.size = data.size();
  request.type = mysqlshdk::rest::Type::PUT;
  test_sign_request(
      "COMMIT MULTIPART UPLOAD", &request,
      "SharedKey "
      "devstoreaccount1:cPvaqQpFSdLcQ3sfZ3qKL198n6tanURxvOLtx/UZoL8=",
      "PUT\n\n\n50\n\ntext/plain\n\n\n\n\n\n\n",
      "/devstoreaccount1/devstoreaccount1/testcontainer/"
      "sample.txt\ncomp:blocklist");
}

}  // namespace azure
}  // namespace mysqlshdk
