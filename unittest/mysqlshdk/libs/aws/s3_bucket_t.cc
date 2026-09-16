/*
 * Copyright (c) 2022, 2026, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/aws/s3_bucket.h"

#include <tinyxml2.h>

#include <chrono>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

#include "mysqlshdk/libs/rest/rest_utils.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_ssl.h"
#include "mysqlshdk/libs/utils/utils_time.h"

#include "unittest/mysqlshdk/libs/aws/aws_tests.h"
#include "unittest/test_utils/cleanup.h"

namespace mysqlshdk {
namespace aws {

using rest::Response_error;

namespace {

constexpr auto k_sha256_abc = "ungWv48Bz+pBQUDeXa4iI7ADYaOWF3qctBD/YfIAFa0=";
constexpr auto k_sha256_empty = "47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=";

}  // namespace

class Aws_s3_bucket_checksum_test : public testing::Test {
 protected:
  void SetUp() override {
    for (const auto name :
         {"AWS_ACCESS_KEY_ID", "AWS_SECRET_ACCESS_KEY", "AWS_SESSION_TOKEN"}) {
      m_cleanup += tests::Cleanup::unset_env_var(name);
    }

    const auto credentials = shcore::get_absolute_path("checksum-credentials");
    const auto config = shcore::get_absolute_path("checksum-config");
    m_cleanup += tests::Cleanup::write_file(
        credentials,
        "[checksum-test]\naws_access_key_id=test-key\n"
        "aws_secret_access_key=test-secret\n");
    m_cleanup += tests::Cleanup::write_file(config, "");

    const auto options = shcore::make_dict(
        S3_bucket_options::bucket_name_option(), "checksum-test",
        S3_bucket_options::credentials_file_option(), credentials,
        S3_bucket_options::config_file_option(), config,
        S3_bucket_options::profile_option(), "checksum-test",
        S3_bucket_options::region_option(), "us-east-1");
    S3_bucket_options parsed_options;
    S3_bucket_options::options().unpack(options, &parsed_options);
    m_bucket = std::make_unique<S3_bucket>(
        std::make_shared<S3_bucket_config>(parsed_options));
  }

  rest::Signed_request put_request() {
    return m_bucket->put_object_request(
        "object", {{"Content-Type", "application/octet-stream"}});
  }

  rest::Signed_request create_request(std::string *body) {
    return m_bucket->create_multipart_upload_request("object", body);
  }

  rest::Signed_request part_request() {
    return m_bucket->upload_part_request({"object", "upload-id"}, 1, 3);
  }

  rest::Signed_request complete_request(
      const std::vector<Multipart_object_part> &parts, std::string *body) {
    return m_bucket->commit_multipart_upload_request({"object", "upload-id"},
                                                     parts, body);
  }

  std::vector<Multipart_object_part> parse_parts(const std::string &xml) {
    rest::String_buffer buffer;
    buffer.append_data(xml.data(), xml.size());
    return m_bucket->parse_list_multipart_uploaded_parts(buffer);
  }

  Multipart_object_part::Metadata parse_part(const rest::Response &response) {
    return m_bucket->parse_multipart_upload(response);
  }

  Multipart_object_part::Metadata parse_copied_part(const std::string &xml) {
    rest::String_buffer buffer;
    buffer.append_data(xml.data(), xml.size());
    return m_bucket->parse_multipart_upload_copy(buffer);
  }

 private:
  tests::Cleanup m_cleanup;
  std::unique_ptr<S3_bucket> m_bucket;
};

TEST_F(Aws_s3_bucket_checksum_test, selects_checksum_operations) {
  const auto put = put_request();
  ASSERT_NE(put.unsigned_headers().end(),
            put.unsigned_headers().find("x-amz-sdk-checksum-algorithm"));
  EXPECT_EQ("SHA256",
            put.unsigned_headers().at("x-amz-sdk-checksum-algorithm"));
  EXPECT_EQ("application/octet-stream",
            put.unsigned_headers().at("Content-Type"));

  const auto part = part_request();
  ASSERT_NE(part.unsigned_headers().end(),
            part.unsigned_headers().find("x-amz-sdk-checksum-algorithm"));
  EXPECT_EQ("SHA256",
            part.unsigned_headers().at("x-amz-sdk-checksum-algorithm"));

  std::string body;
  const auto create = create_request(&body);
  ASSERT_NE(create.unsigned_headers().end(),
            create.unsigned_headers().find("x-amz-checksum-algorithm"));
  EXPECT_EQ("SHA256", create.unsigned_headers().at("x-amz-checksum-algorithm"));
  EXPECT_EQ(create.unsigned_headers().end(),
            create.unsigned_headers().find("x-amz-sdk-checksum-algorithm"));
  EXPECT_TRUE(body.empty());
}

TEST_F(Aws_s3_bucket_checksum_test, checksums_survive_listing_and_completion) {
  const auto parts = parse_parts(
      "<ListPartsResult>"
      "<Part><PartNumber>1</PartNumber><ETag>&quot;first-etag&quot;</ETag>"
      "<Size>3</Size><ChecksumSHA256>" +
      std::string{k_sha256_abc} +
      "</ChecksumSHA256></Part>"
      "<Part><PartNumber>2</PartNumber><ETag>&quot;second-etag&quot;</ETag>"
      "<Size>0</Size><ChecksumSHA256>" +
      k_sha256_empty + "</ChecksumSHA256></Part></ListPartsResult>");
  ASSERT_EQ(2, parts.size());
  EXPECT_EQ(3, parts[0].size);
  EXPECT_EQ(0, parts[1].size);
  EXPECT_EQ(k_sha256_abc, parts[0].metadata.checksum);
  EXPECT_EQ(k_sha256_empty, parts[1].metadata.checksum);

  std::string body;
  const auto complete = complete_request(parts, &body);
  EXPECT_EQ(complete.unsigned_headers().end(),
            complete.unsigned_headers().find("x-amz-sdk-checksum-algorithm"));
  EXPECT_EQ(complete.unsigned_headers().end(),
            complete.unsigned_headers().find("x-amz-checksum-sha256"));

  tinyxml2::XMLDocument xml;
  ASSERT_EQ(tinyxml2::XML_SUCCESS, xml.Parse(body.c_str()));
  const auto root = xml.FirstChildElement("CompleteMultipartUpload");
  ASSERT_NE(nullptr, root);
  auto part = root->FirstChildElement("Part");

  for (std::size_t index = 0; index < parts.size(); ++index) {
    ASSERT_NE(nullptr, part);
    const auto value = part->FirstChildElement("ChecksumSHA256");
    ASSERT_NE(nullptr, value);
    EXPECT_STREQ(parts[index].metadata.checksum.c_str(), value->GetText());
    ASSERT_NE(nullptr, part->FirstChildElement("ETag"));
    EXPECT_STREQ(parts[index].metadata.etag.c_str(),
                 part->FirstChildElement("ETag")->GetText());
    ASSERT_NE(nullptr, part->FirstChildElement("PartNumber"));
    EXPECT_EQ(index + 1, part->FirstChildElement("PartNumber")->IntText());
    part = part->NextSiblingElement("Part");
  }

  EXPECT_EQ(nullptr, part);
}

TEST_F(Aws_s3_bucket_checksum_test, parses_part_checksum) {
  rest::Response response;
  response.headers = {{"ETag", "\"server-etag\""},
                      {"x-amz-checksum-sha256", k_sha256_abc}};
  const auto metadata = parse_part(response);
  EXPECT_EQ("\"server-etag\"", metadata.etag);
  EXPECT_EQ(k_sha256_abc, metadata.checksum);
}

TEST_F(Aws_s3_bucket_checksum_test, rejects_missing_uploaded_part_checksum) {
  rest::Response response;
  response.headers = {{"ETag", "\"server-etag\""}};
  EXPECT_THROW_LIKE(parse_part(response), shcore::Exception,
                    "Missing SHA256 checksum in UploadPart response");

  response.headers["x-amz-checksum-sha256"] = "";
  EXPECT_THROW_LIKE(parse_part(response), shcore::Exception,
                    "Missing SHA256 checksum in UploadPart response");
}

TEST_F(Aws_s3_bucket_checksum_test, rejects_missing_listed_part_checksum) {
  for (const auto checksum :
       {"", "<ChecksumSHA256/>", "<ChecksumSHA256></ChecksumSHA256>"}) {
    SCOPED_TRACE(checksum);
    // The invalid part follows a valid one to check every returned part.
    const auto xml =
        "<ListPartsResult><Part><PartNumber>1</PartNumber>"
        "<ETag>first-etag</ETag><Size>3</Size><ChecksumSHA256>" +
        std::string{k_sha256_abc} +
        "</ChecksumSHA256></Part><Part><PartNumber>2</PartNumber>"
        "<ETag>second-etag</ETag><Size>3</Size>" +
        checksum + "</Part></ListPartsResult>";
    const auto expected =
        std::string{checksum}.empty()
            ? "'ListPartsResult'.'Part' does not have a 'ChecksumSHA256' "
              "element"
            : "'ListPartsResult'.'Part'.'ChecksumSHA256' element is empty";
    EXPECT_THROW_LIKE(parse_parts(xml), shcore::Exception, expected);
  }
}

TEST_F(Aws_s3_bucket_checksum_test, parses_copied_part_checksum) {
  const auto metadata = parse_copied_part(
      "<CopyPartResult><ETag>&quot;copy-etag&quot;</ETag><ChecksumSHA256>" +
      std::string{k_sha256_abc} + "</ChecksumSHA256></CopyPartResult>");
  EXPECT_EQ("\"copy-etag\"", metadata.etag);
  EXPECT_EQ(k_sha256_abc, metadata.checksum);
}

TEST_F(Aws_s3_bucket_checksum_test, rejects_missing_copied_part_checksum) {
  for (const auto checksum :
       {"", "<ChecksumSHA256/>", "<ChecksumSHA256></ChecksumSHA256>"}) {
    SCOPED_TRACE(checksum);
    const auto expected =
        std::string{checksum}.empty()
            ? "'CopyPartResult' does not have a 'ChecksumSHA256' element"
            : "'CopyPartResult'.'ChecksumSHA256' element is empty";
    EXPECT_THROW_LIKE(
        parse_copied_part("<CopyPartResult><ETag>copy-etag</ETag>" +
                          std::string{checksum} + "</CopyPartResult>"),
        shcore::Exception, expected);
  }
}

class Bucket_create_delete : public Aws_s3_tests {
 protected:
  // do not create&delete the bucket, test is going to do this
  void SetUp() override {
    Test::SetUp();
    setup_test();
  }

  void TearDown() override { Test::TearDown(); }
};

TEST_P(Bucket_create_delete, test) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  if (!endpoint_override().empty()) {
    SKIP_TEST("Skipping create bucket tests, endpoint override is set.");
  }

  S3_bucket bucket{get_config()};
  const auto exists = bucket.exists();

  if (exists) {
    clean_bucket(bucket);
    bucket.delete_();
  }

  EXPECT_FALSE(bucket.exists());
  EXPECT_NO_THROW(bucket.create());
  EXPECT_TRUE(bucket.exists());
  EXPECT_NO_THROW(bucket.delete_());
  EXPECT_FALSE(bucket.exists());

  if (exists) {
    bucket.create();
  }
}

INSTANTIATE_TEST_SUITE_P(Aws_s3, Bucket_create_delete, suffixes(),
                         format_suffix);

class Bucket_test : public Aws_s3_tests {
 protected:
  rest::Headers checksum_head(S3_bucket &bucket, const std::string &name) {
    auto request = bucket.create_signed_request(
        bucket.object_path(name), {{"x-amz-checksum-mode", "ENABLED"}}, {});
    rest::Response response;
    bucket.ensure_connection()->head(&request, &response);
    return response.headers;
  }

  void expect_bad_checksum(S3_bucket &bucket, const std::string &name,
                           rest::Query query = {}) {
    auto request = bucket.create_signed_request(
        bucket.object_path(name),
        {{"x-amz-sdk-checksum-algorithm", "SHA256"},
         {"x-amz-checksum-sha256", k_sha256_empty}},
        std::move(query));
    request.body = "abc";
    request.size = 3;

    try {
      bucket.ensure_connection()->put(&request);
      FAIL() << "S3 accepted an incorrect SHA256 checksum";
    } catch (const Response_error &error) {
      EXPECT_EQ(rest::Response::Status_code::BAD_REQUEST, error.status_code());
      EXPECT_EQ("BadDigest", error.code());
    }
  }
};

TEST_P(Bucket_test, checksum_put_and_empty_object) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  S3_bucket bucket(get_config());
  bucket.put_object("checksum-abc", "abc", 3);
  EXPECT_EQ(k_sha256_abc,
            checksum_head(bucket, "checksum-abc").at("x-amz-checksum-sha256"));

  bucket.put_object("checksum-empty", nullptr, 0);
  EXPECT_EQ(
      k_sha256_empty,
      checksum_head(bucket, "checksum-empty").at("x-amz-checksum-sha256"));
  rest::String_buffer buffer;
  EXPECT_EQ(0, bucket.get_object("checksum-empty", &buffer));
  EXPECT_EQ(0, buffer.size());
}

TEST_P(Bucket_test, checksum_multipart) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  S3_bucket bucket(get_config());
  const std::string first_data(k_min_part_size, 'a');
  const std::string second_data(1024, 'b');
  // Fixed vectors calculated independently from the original part bytes.
  constexpr auto first_checksum =
      "oplo+tLngqqfIECjXwWtuX7Yl56x9XLIyOp4Y34nXzw=";
  constexpr auto second_checksum =
      "DGbyxFQF3ldRiSCadoOZvK+IzMUQAkB+OVwBNqrShE0=";
  constexpr auto composite_checksum =
      "NLHrJlXEZEDn0NlgZhn8GLRMTzURmg2zW2TYiRdd7GU=-2";
  const auto upload = bucket.create_multipart_upload("checksum-multipart");
  const auto first =
      bucket.upload_part(upload, 1, first_data.data(), first_data.size());
  const auto second =
      bucket.upload_part(upload, 2, second_data.data(), second_data.size());
  EXPECT_EQ(first_checksum, first.metadata.checksum);
  EXPECT_EQ(second_checksum, second.metadata.checksum);

  // Recover both the upload descriptor and its parts as a resumed upload does.
  const auto uploads = bucket.list_multipart_uploads();
  ASSERT_EQ(1, uploads.size());
  EXPECT_EQ(upload.name, uploads[0].name);
  EXPECT_EQ(upload.upload_id, uploads[0].upload_id);
  const auto parts = bucket.list_multipart_uploaded_parts(uploads[0]);
  ASSERT_EQ(2, parts.size());
  EXPECT_EQ(1, parts[0].part_num);
  EXPECT_EQ(2, parts[1].part_num);
  EXPECT_EQ(first.metadata.etag, parts[0].metadata.etag);
  EXPECT_EQ(second.metadata.etag, parts[1].metadata.etag);
  EXPECT_EQ(first_data.size(), parts[0].size);
  EXPECT_EQ(second_data.size(), parts[1].size);
  EXPECT_EQ(first_checksum, parts[0].metadata.checksum);
  EXPECT_EQ(second_checksum, parts[1].metadata.checksum);
  bucket.commit_multipart_upload(uploads[0], parts);

  const auto data = first_data + second_data;
  bucket.copy_object_multipart(upload.name, "checksum-copy", data.size(),
                               k_min_part_size);

  for (const auto &name : {upload.name, std::string{"checksum-copy"}}) {
    SCOPED_TRACE(name);
    EXPECT_EQ(composite_checksum,
              checksum_head(bucket, name).at("x-amz-checksum-sha256"));
    rest::String_buffer buffer;
    EXPECT_EQ(data.size(), bucket.get_object(name, &buffer));
    EXPECT_EQ(data, std::string(buffer.data(), buffer.size()));
  }
}

TEST_P(Bucket_test, checksum_rejects_mismatch) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  S3_bucket bucket(get_config());
  expect_bad_checksum(bucket, "checksum-bad-put");

  const auto upload = bucket.create_multipart_upload("checksum-bad-part");
  expect_bad_checksum(bucket, upload.name,
                      {{"partNumber", "1"}, {"uploadId", upload.upload_id}});
  bucket.abort_multipart_upload(upload);
}

// MYSQLSH_S3_OBJECT_LOCK_BUCKET_NAME must name a bucket with GOVERNANCE default
// retention. In addition to the usual upload permissions, these tests need
// GetBucketObjectLockConfiguration, GetObjectRetention, ListBucketVersions,
// DeleteObjectVersion and BypassGovernanceRetention. The bucket is provisioned
// separately; all cleanup is limited to each test's unique object prefix.
class Aws_s3_object_lock_test : public Aws_tests {
 protected:
  void setup_test() override {
    Aws_tests::setup_test("MYSQLSH_S3_OBJECT_LOCK_BUCKET_NAME");
  }

  void SetUp() override {
    Aws_tests::SetUp();

    if (should_skip()) return;

    m_bucket = std::make_unique<S3_bucket>(get_config());
    auto request = m_bucket->create_bucket_request({{"object-lock", {}}});
    rest::String_response response;
    m_bucket->ensure_connection()->get(&request, &response);
    const auto xml = rest::parse_xml(response.buffer);
    const auto root = child(xml.get(), "ObjectLockConfiguration");
    ASSERT_EQ("Enabled", text(root, "ObjectLockEnabled"));
    const auto retention = child(child(root, "Rule"), "DefaultRetention");
    ASSERT_EQ("GOVERNANCE", text(retention, "Mode"));
    const auto days = retention->FirstChildElement("Days");
    const auto years = retention->FirstChildElement("Years");
    ASSERT_TRUE((days && days->IntText() > 0) ||
                (years && years->IntText() > 0));

    m_prefix = "object-lock-" +
               shcore::get_random_string(20, "0123456789abcdef") + "/";
  }

  void TearDown() override {
    if (m_prefix.empty()) return;

    try {
      for (const auto &upload : m_bucket->list_multipart_uploads()) {
        if (shcore::str_beginswith(upload.name, m_prefix)) {
          EXPECT_NO_THROW(m_bucket->abort_multipart_upload(upload))
              << upload.name;
        }
      }
    } catch (const std::exception &e) {
      ADD_FAILURE() << "Failed to clean up multipart uploads: " << e.what();
    }

    try {
      for (const auto &[name, version_id] : list_versions()) {
        EXPECT_NO_THROW(delete_version(name, version_id)) << name;
      }

      EXPECT_TRUE(list_versions().empty());
    } catch (const std::exception &e) {
      ADD_FAILURE() << "Failed to clean up Object Lock versions: " << e.what();
    }
  }

  std::string object_name(const std::string &name) const {
    return m_prefix + name;
  }

  void expect_retained_object(const std::string &name, const std::string &data,
                              const std::string &checksum) {
    SCOPED_TRACE(name);
    auto request = m_bucket->create_object_request(
        name, {{"x-amz-checksum-mode", "ENABLED"}}, {});
    rest::Response response;
    m_bucket->ensure_connection()->head(&request, &response);
    const auto &headers = response.headers;

    ASSERT_NE(headers.end(), headers.find("x-amz-version-id"));
    EXPECT_FALSE(headers.at("x-amz-version-id").empty());
    EXPECT_NE("null", headers.at("x-amz-version-id"));
    ASSERT_NE(headers.end(), headers.find("x-amz-object-lock-mode"));
    EXPECT_EQ("GOVERNANCE", headers.at("x-amz-object-lock-mode"));
    ASSERT_NE(headers.end(),
              headers.find("x-amz-object-lock-retain-until-date"));
    EXPECT_GT(shcore::rfc3339_to_time_point(
                  headers.at("x-amz-object-lock-retain-until-date")),
              std::chrono::system_clock::now());
    ASSERT_NE(headers.end(), headers.find("x-amz-checksum-sha256"));
    EXPECT_EQ(checksum, headers.at("x-amz-checksum-sha256"));

    rest::String_buffer buffer;
    EXPECT_EQ(data.size(), m_bucket->get_object(name, &buffer));
    EXPECT_EQ(data, std::string(buffer.data(), buffer.size()));
  }

  void upload_without_checksum(const std::string &name) {
    // Use a signed request without the checksum selection made by PutObject.
    auto request = m_bucket->create_object_request(name);
    request.body = "abc";
    request.size = 3;
    m_bucket->ensure_connection()->put(&request);
  }

  std::unique_ptr<S3_bucket> m_bucket;

 private:
  static const tinyxml2::XMLElement *child(const tinyxml2::XMLNode *parent,
                                           const char *name) {
    const auto element = parent->FirstChildElement(name);

    if (!element) {
      throw std::runtime_error("Missing XML element: " + std::string{name});
    }

    return element;
  }

  static std::string text(const tinyxml2::XMLNode *parent, const char *name) {
    const auto value = child(parent, name)->GetText();

    if (!value || !*value) {
      throw std::runtime_error("Empty XML element: " + std::string{name});
    }

    return value;
  }

  std::vector<std::pair<std::string, std::string>> list_versions() {
    std::vector<std::pair<std::string, std::string>> versions;
    rest::Query query = {{"versions", {}}, {"prefix", m_prefix}};

    while (true) {
      auto request = m_bucket->create_bucket_request(query);
      rest::String_response response;
      m_bucket->ensure_connection()->get(&request, &response);
      const auto xml = rest::parse_xml(response.buffer);
      const auto root = child(xml.get(), "ListVersionsResult");

      for (const auto type : {"Version", "DeleteMarker"}) {
        for (auto version = root->FirstChildElement(type); version;
             version = version->NextSiblingElement(type)) {
          const auto name = text(version, "Key");

          if (!shcore::str_beginswith(name, m_prefix)) {
            throw std::runtime_error("Object version outside the test prefix");
          }

          versions.emplace_back(name, text(version, "VersionId"));
        }
      }

      if (text(root, "IsTruncated") != "true") break;

      query["key-marker"] = text(root, "NextKeyMarker");
      query["version-id-marker"] = text(root, "NextVersionIdMarker");
    }

    return versions;
  }

  void delete_version(const std::string &name, const std::string &version_id) {
    auto request = m_bucket->create_object_request(
        name, {{"x-amz-bypass-governance-retention", "true"}},
        {{"versionId", version_id}});
    m_bucket->ensure_connection()->delete_(&request);
  }

  std::string m_prefix;
};

TEST_F(Aws_s3_object_lock_test, put_and_empty_objects) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  const auto name = object_name("abc");
  m_bucket->put_object(name, "abc", 3);
  expect_retained_object(name, "abc", k_sha256_abc);

  const auto empty_name = object_name("empty");
  m_bucket->put_object(empty_name, nullptr, 0);
  expect_retained_object(empty_name, "", k_sha256_empty);
}

TEST_F(Aws_s3_object_lock_test, multipart_upload_and_copy) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  constexpr std::size_t part_size = 5 * 1024 * 1024;
  const std::string first_data(part_size, 'a');
  const std::string second_data(1024, 'b');
  const auto upload =
      m_bucket->create_multipart_upload(object_name("multipart"));
  m_bucket->upload_part(upload, 1, first_data.data(), first_data.size());
  m_bucket->upload_part(upload, 2, second_data.data(), second_data.size());
  // Complete using the part checksums recovered from S3, as a resumed upload
  // does.
  const auto parts = m_bucket->list_multipart_uploaded_parts(upload);
  ASSERT_EQ(2, parts.size());
  m_bucket->commit_multipart_upload(upload, parts);

  constexpr auto composite_checksum =
      "NLHrJlXEZEDn0NlgZhn8GLRMTzURmg2zW2TYiRdd7GU=-2";
  const auto data = first_data + second_data;
  expect_retained_object(upload.name, data, composite_checksum);

  const auto copy_name = object_name("copy");
  m_bucket->copy_object_multipart(upload.name, copy_name, data.size(),
                                  part_size);
  expect_retained_object(copy_name, data, composite_checksum);
}

TEST_F(Aws_s3_object_lock_test, rejects_missing_checksum) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  try {
    upload_without_checksum(object_name("missing-checksum"));
    FAIL() << "S3 accepted an Object Lock upload without a checksum";
  } catch (const Response_error &error) {
    EXPECT_EQ(rest::Response::Status_code::BAD_REQUEST, error.status_code());
    EXPECT_EQ("InvalidRequest", error.code());
  }
}

TEST_P(Bucket_test, put_and_delete_object) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  S3_bucket bucket(get_config());

  auto objects = bucket.list_objects();
  EXPECT_TRUE(objects.empty());

  // add an object
  bucket.put_object("żółw.txt", "Zażółć gęślą jaźń", 14);

  objects = bucket.list_objects("", 0, true, Object_details::NAME);
  EXPECT_EQ(1, objects.size());
  EXPECT_STREQ("żółw.txt", objects[0].name.c_str());

  // delete an object
  bucket.delete_object("żółw.txt");

  objects = bucket.list_objects();
  EXPECT_TRUE(objects.empty());
}

TEST_P(Bucket_test, list_objects) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  S3_bucket bucket(get_config());
  std::unordered_set<std::string> prefixes;

  create_objects(bucket);

  // DEFAULT LIST: AWS does not allow to select which data is returned, and
  // always returns all fields
  auto objects = bucket.list_objects();
  EXPECT_EQ(11, objects.size());

  for (size_t index = 0; index < m_objects.size(); index++) {
    EXPECT_STREQ(m_objects[index].c_str(), objects[index].name.c_str());
    EXPECT_EQ(1, objects[index].size);
    EXPECT_FALSE(objects[index].etag.empty());
    EXPECT_FALSE(objects[index].time_created.empty());
  }

  // NOT RECURSIVE: Gets only objects in the current "folder", i.e. nothing
  // after next /
  objects = bucket.list_objects("", 0, false, Object_details::NAME, &prefixes);

  // Validates the returned files
  EXPECT_EQ(5, objects.size());
  EXPECT_STREQ("sakila.sql", objects[0].name.c_str());
  EXPECT_STREQ("sakila_metadata.txt", objects[1].name.c_str());
  EXPECT_STREQ("sakila_tables.txt", objects[2].name.c_str());
  EXPECT_STREQ("uncommon%25%name.txt", objects[3].name.c_str());
  EXPECT_STREQ("uncommon's name.txt", objects[4].name.c_str());

  // Validates the returned prefixes (i.e. 'subdirectories')
  EXPECT_EQ(1, prefixes.size());
  EXPECT_STREQ("sakila/", prefixes.begin()->c_str());
  prefixes.clear();

  // PREFIX FILTER: Gets files starting with....
  objects = bucket.list_objects("sakila/actor", 0, true, Object_details::NAME,
                                &prefixes);
  EXPECT_EQ(2, objects.size());
  EXPECT_STREQ("sakila/actor.csv", objects[0].name.c_str());
  EXPECT_STREQ("sakila/actor_metadata.txt", objects[1].name.c_str());
  EXPECT_TRUE(prefixes.empty());
  prefixes.clear();

  clean_bucket(bucket);
}

TEST_P(Bucket_test, multipart_uploads) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  S3_bucket bucket(get_config());

  // ACTIVE MULTIPART UPLOADS
  auto first = bucket.create_multipart_upload("sakila.sql");

  auto mp_uploads = bucket.list_multipart_uploads();
  EXPECT_EQ(1, mp_uploads.size());
  EXPECT_STREQ(first.name.c_str(), mp_uploads[0].name.c_str());
  EXPECT_STREQ(first.upload_id.c_str(), mp_uploads[0].upload_id.c_str());

  auto second = bucket.create_multipart_upload("sakila_metadata.txt");

  mp_uploads = bucket.list_multipart_uploads(1);
  EXPECT_EQ(1, mp_uploads.size());
  EXPECT_STREQ(first.name.c_str(), mp_uploads[0].name.c_str());
  EXPECT_STREQ(first.upload_id.c_str(), mp_uploads[0].upload_id.c_str());

  mp_uploads = bucket.list_multipart_uploads();
  EXPECT_EQ(2, mp_uploads.size());
  EXPECT_STREQ(first.name.c_str(), mp_uploads[0].name.c_str());
  EXPECT_STREQ(first.upload_id.c_str(), mp_uploads[0].upload_id.c_str());
  EXPECT_STREQ(second.name.c_str(), mp_uploads[1].name.c_str());
  EXPECT_STREQ(second.upload_id.c_str(), mp_uploads[1].upload_id.c_str());

  // ABORT MULTIPART UPLOADS
  for (const auto &mp_upload : mp_uploads) {
    bucket.abort_multipart_upload(mp_upload);
  }

  mp_uploads = bucket.list_multipart_uploads();
  EXPECT_TRUE(mp_uploads.empty());

  // COMMIT MULTIPART UPLOAD
  const auto data = multipart_file_data();
  const auto hash = shcore::ssl::restricted::md5(data);

  auto mp_object = bucket.create_multipart_upload("sakila.sql");
  std::vector<Multipart_object_part> parts;
  std::size_t offset = 0;
  std::size_t part_number = 0;

  while (offset < k_multipart_file_size) {
    parts.push_back(bucket.upload_part(
        mp_object, ++part_number, data.c_str() + offset,
        std::min(k_min_part_size, k_multipart_file_size - offset)));
    offset += k_min_part_size;
  }

  bucket.commit_multipart_upload(mp_object, parts);

  auto objects = bucket.list_objects("", 0, true, Object_details::NAME);
  EXPECT_EQ(1, objects.size());
  EXPECT_STREQ("sakila.sql", objects[0].name.c_str());

  mp_uploads = bucket.list_multipart_uploads();
  EXPECT_TRUE(mp_uploads.empty());

  rest::String_buffer buffer{k_multipart_file_size};
  bucket.get_object("sakila.sql", &buffer);
  EXPECT_EQ(hash, shcore::ssl::restricted::md5({buffer.data(), buffer.size()}));

  // copy object multipart
  bucket.copy_object_multipart("sakila.sql", "zakila.sql",
                               k_multipart_file_size, k_min_part_size);

  objects = bucket.list_objects("", 0, true, Object_details::NAME);
  EXPECT_EQ(2, objects.size());
  EXPECT_STREQ("sakila.sql", objects[0].name.c_str());
  EXPECT_STREQ("zakila.sql", objects[1].name.c_str());

  mp_uploads = bucket.list_multipart_uploads();
  EXPECT_TRUE(mp_uploads.empty());

  buffer.clear();
  bucket.get_object("zakila.sql", &buffer);
  EXPECT_EQ(hash, shcore::ssl::restricted::md5({buffer.data(), buffer.size()}));

  bucket.delete_object("sakila.sql");
  bucket.delete_object("zakila.sql");
}

TEST_P(Bucket_test, object_operations) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  S3_bucket bucket(get_config());

  // PUT
  bucket.put_object("sakila.txt", "0123456789", 10);

  // GET Using different byte ranges
  {
    mysqlshdk::rest::String_buffer data;
    auto read = bucket.get_object("sakila.txt", &data);
    EXPECT_EQ(10, read);
    EXPECT_STREQ("0123456789", data.data());
  }

  {
    mysqlshdk::rest::String_buffer data;
    auto read = bucket.get_object("sakila.txt", &data, 0, 3);
    EXPECT_EQ(4, read);
    EXPECT_STREQ("0123", data.data());
  }

  {
    mysqlshdk::rest::String_buffer data;
    auto read = bucket.get_object("sakila.txt", &data, 5, 5);
    EXPECT_EQ(1, read);
    EXPECT_STREQ("5", data.data());
  }

  {
    mysqlshdk::rest::String_buffer data;
    auto read = bucket.get_object("sakila.txt", &data, 8, 15);
    EXPECT_EQ(2, read);
    EXPECT_STREQ("89", data.data());
  }

  {
    mysqlshdk::rest::String_buffer data;
    auto read = bucket.get_object("sakila.txt", &data, 4);
    EXPECT_EQ(6, read);
    EXPECT_STREQ("456789", data.data());
  }

  {
    mysqlshdk::rest::String_buffer data;
    auto read = bucket.get_object("sakila.txt", &data, 4);
    EXPECT_EQ(6, read);
    EXPECT_STREQ("456789", data.data());
  }

  {
    mysqlshdk::rest::String_buffer data;
    auto read = bucket.get_object("sakila.txt", &data, {}, 4);
    EXPECT_EQ(5, read);
    EXPECT_STREQ("01234", data.data());
  }

  {
    mysqlshdk::rest::String_buffer buffer;
    auto read = bucket.get_object("sakila.txt", &buffer, 5, 5);
    EXPECT_EQ(1, read);
    EXPECT_STREQ("5", buffer.data());
  }

  {
    mysqlshdk::rest::String_buffer buffer;
    auto read = bucket.get_object("sakila.txt", &buffer, 8, 15);
    EXPECT_EQ(2, read);
    EXPECT_STREQ("89", buffer.data());
  }
  {
    mysqlshdk::rest::String_buffer buffer;
    auto read = bucket.get_object("sakila.txt", &buffer, 4);
    EXPECT_EQ(6, read);
    EXPECT_STREQ("456789", buffer.data());
  }
  {
    mysqlshdk::rest::String_buffer buffer;
    auto read = bucket.get_object("sakila.txt", &buffer, 4);
    EXPECT_EQ(6, read);
    EXPECT_STREQ("456789", buffer.data());
  }
  {
    mysqlshdk::rest::String_buffer buffer;
    auto read = bucket.get_object("sakila.txt", &buffer,
                                  std::optional<size_t>{}, std::optional{4});
    EXPECT_EQ(4, read);
    EXPECT_STREQ("6789", buffer.data());
  }

  // COPY
  {
    bucket.copy_object("sakila.txt", "sakila/metadata.txt");
    mysqlshdk::rest::String_buffer buffer;
    auto read = bucket.get_object("sakila/metadata.txt", &buffer);
    EXPECT_EQ(10, read);
    EXPECT_STREQ("0123456789", buffer.data());
    bucket.delete_object("sakila/metadata.txt");
  }

  // RENAME
  {
    bucket.rename_object("sakila.txt", "sakila/metadata.txt");
    mysqlshdk::rest::String_buffer buffer;
    auto read = bucket.get_object("sakila/metadata.txt", &buffer);
    EXPECT_EQ(10, read);
    EXPECT_STREQ("0123456789", buffer.data());
    bucket.delete_object("sakila/metadata.txt");
  }
}

TEST_P(Bucket_test, error_conditions) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  {
    S3_bucket unexisting(get_config("unexisting"));

    EXPECT_THROW_LIKE(unexisting.put_object("sample.txt", "data", 4),
                      Response_error, "Failed to put object 'sample.txt': ");
  }

  const auto config = get_config();
  S3_bucket bucket(config);

  // DELETE: Unexisting Object - this always succeeds in AWS

  // HEAD: Unexisting Object
  EXPECT_THROW_LIKE(bucket.head_object("sample.txt"), Response_error,
                    "Failed to get summary for object 'sample.txt': Not Found");

  // GET: Unexisting Object
  EXPECT_THROW_LIKE(bucket.get_object("sample.txt", nullptr), Response_error,
                    "Failed to get object 'sample.txt': ");

  // RENAME: Unexisting Object
  EXPECT_THROW_LIKE(bucket.rename_object("sample.txt", "other.txt"),
                    Response_error,
                    "Failed to rename object 'sample.txt' to 'other.txt': "
                    "Failed to get summary for object 'sample.txt': Not Found");

  Multipart_object object;
  object.name = "Sample.txt";
  object.upload_id = "SOME-WEIRD-UPLOAD-ID";
  EXPECT_THROW_LIKE(bucket.list_multipart_uploaded_parts(object),
                    Response_error,
                    "Failed to list uploaded parts for object 'Sample.txt': ");

  EXPECT_THROW_LIKE(bucket.upload_part(object, 1, "DATA", 4), Response_error,
                    "Failed to upload part 1 for object 'Sample.txt': ");

  EXPECT_THROW_LIKE(
      bucket.commit_multipart_upload(object, {}), Response_error,
      "Failed to commit multipart upload for object 'Sample.txt': ");

  EXPECT_THROW_LIKE(bucket.copy_object_multipart("from", "to", 1, 2),
                    shcore::Exception,
                    "Total size has to be greater than part size");

  EXPECT_THROW_LIKE(bucket.copy_object_multipart(
                        "from", "to", 2 * k_min_part_size, k_min_part_size - 1),
                    shcore::Exception,
                    "Part size has to be a value between 5242880 and " +
                        std::to_string(k_max_part_size));

  if (k_max_part_size < std::numeric_limits<std::size_t>::max()) {
    EXPECT_THROW_LIKE(
        bucket.copy_object_multipart("from", "to", k_max_part_size + 1,
                                     k_max_part_size + 1),
        shcore::Exception,
        "Part size has to be a value between 5242880 and " +
            std::to_string(k_max_part_size));
  }
}

INSTANTIATE_TEST_SUITE_P(Aws_s3, Bucket_test, suffixes(), format_suffix);

}  // namespace aws
}  // namespace mysqlshdk
