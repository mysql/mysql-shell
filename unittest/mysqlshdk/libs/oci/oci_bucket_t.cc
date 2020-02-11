/* Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2.0,
 as published by the Free Software Foundation.

 This program is also distributed with certain software (including
 but not limited to OpenSSL) that is licensed under separate terms, as
 designated in a particular file or component or in included license
 documentation.  The authors of MySQL hereby grant you an additional
 permission to link the program and your derivative works with the
 separately licensed software that they have included with MySQL.
 This program is distributed in the hope that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 the GNU General Public License, version 2.0, for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include "unittest/mysqlshdk/libs/oci/oci_tests.h"

namespace testing {

TEST_F(Oci_os_tests, bucket_create_and_delete) {
  SKIP_IF_NO_OCI_CONFIGURATION

  // Ensures the bucket is empty
  Bucket bucket(get_options(PRIVATE_BUCKET));
  auto objects = bucket.list_objects();
  EXPECT_TRUE(objects.empty());

  // Adds an object
  bucket.put_object("sample.txt", "Sample Content", 14);
  objects = bucket.list_objects("", "", "", 0, true,
                                mysqlshdk::oci::object_fields::kName);
  EXPECT_EQ(1, objects.size());
  EXPECT_STREQ("sample.txt", objects[0].name.c_str());

  bucket.delete_object("sample.txt");
  objects = bucket.list_objects();
  EXPECT_TRUE(objects.empty());
}

TEST_F(Oci_os_tests, bucket_list_objects) {
  SKIP_IF_NO_OCI_CONFIGURATION

  Bucket bucket(get_options(PRIVATE_BUCKET));
  std::vector<std::string> prefixes;
  std::string next_start;

  create_objects(bucket);

  // DEFAULT LIST: returns name and size only
  auto objects = bucket.list_objects();
  EXPECT_EQ(9, objects.size());

  for (size_t index = 0; index < m_objects.size(); index++) {
    EXPECT_STREQ(m_objects[index].c_str(), objects[index].name.c_str());
    EXPECT_EQ(1, objects[index].size);
    EXPECT_TRUE(objects[index].etag.empty());
    EXPECT_TRUE(objects[index].md5.empty());
    EXPECT_TRUE(objects[index].time_created.empty());
  }

  // FIELD FILTER: just names
  objects = bucket.list_objects("", "", "", 0, true,
                                mysqlshdk::oci::object_fields::kName);
  EXPECT_EQ(9, objects.size());

  for (size_t index = 0; index < m_objects.size(); index++) {
    EXPECT_STREQ(m_objects[index].c_str(), objects[index].name.c_str());
    EXPECT_EQ(0, objects[index].size);
    EXPECT_TRUE(objects[index].etag.empty());
    EXPECT_TRUE(objects[index].md5.empty());
    EXPECT_TRUE(objects[index].time_created.empty());
  }

  // FIELD FILTER: all fields
  objects = bucket.list_objects("", "", "", 0, true,
                                mysqlshdk::oci::Object_fields_mask::all());
  EXPECT_EQ(9, objects.size());

  for (size_t index = 0; index < m_objects.size(); index++) {
    EXPECT_STREQ(m_objects[index].c_str(), objects[index].name.c_str());
    EXPECT_EQ(1, objects[index].size);
    EXPECT_FALSE(objects[index].etag.empty());
    EXPECT_FALSE(objects[index].md5.empty());
    EXPECT_FALSE(objects[index].time_created.empty());
  }

  // NOT RECURSIVE: Gets only objects in the current "folder", i.e. nothing
  // after next /
  objects = bucket.list_objects(
      "", "", "", 0, false, mysqlshdk::oci::object_fields::kName, &prefixes);

  // Validates the returned files
  EXPECT_EQ(3, objects.size());
  EXPECT_STREQ("sakila.sql", objects[0].name.c_str());
  EXPECT_STREQ("sakila_metadata.txt", objects[1].name.c_str());
  EXPECT_STREQ("sakila_tables.txt", objects[2].name.c_str());

  // Validates the returned prefixes (i.e. 'subdirectories')
  EXPECT_EQ(1, prefixes.size());
  EXPECT_STREQ("sakila/", prefixes[0].c_str());
  prefixes.clear();
  next_start.clear();

  // PREFIX FILTER: Gets files starting with....
  objects = bucket.list_objects("sakila/actor", "", "", 0, true,
                                mysqlshdk::oci::object_fields::kName, &prefixes,
                                &next_start);
  EXPECT_EQ(2, objects.size());
  EXPECT_STREQ("sakila/actor.csv", objects[0].name.c_str());
  EXPECT_STREQ("sakila/actor_metadata.txt", objects[1].name.c_str());
  EXPECT_TRUE(next_start.empty());
  EXPECT_TRUE(prefixes.empty());
  prefixes.clear();
  next_start.clear();

  // START FILTER: Gets files starting with one named...
  for (size_t index = 0; index < objects.size(); index++) {
    objects = bucket.list_objects("", next_start, "", 1, true,
                                  mysqlshdk::oci::object_fields::kName,
                                  &prefixes, &next_start);

    EXPECT_EQ(1, objects.size());
    EXPECT_STREQ(m_objects[index].c_str(), objects[0].name.c_str());
    EXPECT_TRUE(prefixes.empty());

    if (index == (m_objects.size() - 1))
      EXPECT_TRUE(next_start.empty());
    else
      EXPECT_STREQ(m_objects[index + 1].c_str(), next_start.c_str());
  }

  clean_bucket(bucket);
}

TEST_F(Oci_os_tests, bucket_multipart_uploads) {
  SKIP_IF_NO_OCI_CONFIGURATION

  Bucket bucket(get_options(PRIVATE_BUCKET));

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
  auto mp_object = bucket.create_multipart_upload("sakila.sql");
  std::vector<Multipart_object_part> parts;
  parts.push_back(bucket.upload_part(mp_object, 1, "Testing Content", 15));
  parts.push_back(bucket.upload_part(mp_object, 2, "Even More", 9));
  bucket.commit_multipart_upload(mp_object, parts);
  auto objects = bucket.list_objects("", "", "", 0, true,
                                     mysqlshdk::oci::object_fields::kName);
  EXPECT_EQ(1, objects.size());
  EXPECT_STREQ("sakila.sql", objects[0].name.c_str());

  mp_uploads = bucket.list_multipart_uploads();
  EXPECT_TRUE(mp_uploads.empty());

  bucket.delete_object("sakila.sql");
}

TEST_F(Oci_os_tests, bucket_object_operations) {
  SKIP_IF_NO_OCI_CONFIGURATION

  Bucket bucket(get_options(PRIVATE_BUCKET));

  // PUT
  bucket.put_object("sakila.txt", "0123456789", 10);

  // GET Using different byte ranges
  char buffer[10];
  size_t read = bucket.get_object("sakila.txt", buffer);
  EXPECT_EQ(10, read);
  std::string data(buffer, read);
  EXPECT_STREQ("0123456789", data.c_str());

  read = bucket.get_object("sakila.txt", buffer, 0, 3);
  EXPECT_EQ(4, read);
  data.assign(buffer, read);
  EXPECT_STREQ("0123", data.c_str());

  read = bucket.get_object("sakila.txt", buffer, 5, 5);
  EXPECT_EQ(1, read);
  data.assign(buffer, read);
  EXPECT_STREQ("5", data.c_str());

  read = bucket.get_object("sakila.txt", buffer, 8, 15);
  EXPECT_EQ(2, read);
  data.assign(buffer, read);
  EXPECT_STREQ("89", data.c_str());

  read = bucket.get_object("sakila.txt", buffer, 4);
  EXPECT_EQ(6, read);
  data.assign(buffer, read);
  EXPECT_STREQ("456789", data.c_str());

  read = bucket.get_object("sakila.txt", buffer, 4);
  EXPECT_EQ(6, read);
  data.assign(buffer, read);
  EXPECT_STREQ("456789", data.c_str());

  read = bucket.get_object("sakila.txt", buffer, {}, 4);
  EXPECT_EQ(4, read);
  data.assign(buffer, read);
  EXPECT_STREQ("6789", data.c_str());

  // RENAME
  bucket.rename_object("sakila.txt", "sakila/metadata.txt");
  std::string final_data;
  read = bucket.get_object("sakila/metadata.txt", buffer);
  data.assign(buffer, read);
  EXPECT_STREQ("0123456789", data.c_str());

  bucket.delete_object("sakila/metadata.txt");
}

TEST_F(Oci_os_tests, bucket_error_conditions) {
  SKIP_IF_NO_OCI_CONFIGURATION
  EXPECT_THROW_LIKE(Bucket(get_options("unexisting")), Oci_error,
                    "The bucket 'unexisting' does not exist in namespace");

  Bucket bucket(get_options(PUBLIC_BUCKET));

  // PUT_OBJECT: Override disabled
  bucket.put_object("sample.txt", "data", 4);
  auto list = bucket.list_objects();
  EXPECT_EQ(1, list.size());
  EXPECT_STREQ("sample.txt", list[0].name.c_str());
  EXPECT_THROW_LIKE(
      bucket.put_object("sample.txt", "data", 4, false), Oci_error,
      "The If-None-Match header is '*' but there is an existing entity");
  bucket.delete_object("sample.txt");

  // DELETE: Unexsinting Object
  EXPECT_THROW_LIKE(bucket.delete_object("sample.txt"), Oci_error,
                    "The object 'sample.txt' does not exist in bucket '" +
                        PUBLIC_BUCKET + "' with namespace '" +
                        bucket.get_namespace() + "'");

  // HEAD: Unexsinting Object
  EXPECT_THROW_LIKE(bucket.head_object("sample.txt"), Oci_error, "Not Found");

  // GET: Unexsinting Object
  EXPECT_THROW_LIKE(bucket.get_object("sample.txt", nullptr), Oci_error,
                    "The object 'sample.txt' was not found in the bucket '" +
                        PUBLIC_BUCKET + "'");

  // RENAME: Unexsinting Object
  EXPECT_THROW_LIKE(bucket.rename_object("sample.txt", "other.txt"), Oci_error,
                    "Not Found");

  Multipart_object object;
  object.name = "Sample.txt";
  object.upload_id = "SOME-WEIRD-UPLOAD-ID";
  EXPECT_THROW_LIKE(bucket.list_multipart_upload_parts(object), Oci_error,
                    "No such upload");

  EXPECT_THROW_LIKE(bucket.upload_part(object, 1, "DATA", 4), Oci_error,
                    "No such upload");

  EXPECT_THROW_LIKE(bucket.commit_multipart_upload(object, {}), Oci_error,
                    "There are no parts to commit");
}
}  // namespace testing