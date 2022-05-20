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

#include "mysqlshdk/libs/aws/s3_bucket.h"

#include <set>
#include <string>

#include "mysqlshdk/libs/utils/ssl_keygen.h"

#include "unittest/mysqlshdk/libs/aws/aws_tests.h"

namespace mysqlshdk {
namespace aws {

using rest::Response_error;

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

class Bucket_test : public Aws_s3_tests {};

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
  const auto hash = shcore::ssl::md5(data.c_str(), data.size());

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
  EXPECT_EQ(hash, shcore::ssl::md5(buffer.data(), buffer.size()));

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
  EXPECT_EQ(hash, shcore::ssl::md5(buffer.data(), buffer.size()));

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
