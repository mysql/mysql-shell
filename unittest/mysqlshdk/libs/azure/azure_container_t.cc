/* Copyright (c) 2020, 2022, Oracle and/or its affiliates.

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

#include <optional>

#include "mysqlshdk/libs/rest/response.h"
#include "mysqlshdk/libs/storage/backend/object_storage_bucket.h"
#include "mysqlshdk/libs/utils/ssl_keygen.h"
#include "mysqlshdk/libs/utils/utils_time.h"
#include "unittest/mysqlshdk/libs/azure/azure_tests.h"

namespace mysqlshdk {
namespace azure {

using mysqlshdk::azure::Blob_container;
using mysqlshdk::rest::Response_error;
using mysqlshdk::storage::backend::object_storage::Multipart_object;
using mysqlshdk::storage::backend::object_storage::Multipart_object_part;
using mysqlshdk::storage::backend::object_storage::Object_details;

class Azure_container_tests : public Azure_tests {
 public:
  static std::string s_container_name;
  static void SetUpTestCase() { testing::create_container(s_container_name); }
  static void TearDownTestCase() {
    testing::delete_container(s_container_name);
  }
  std::string container_name() override { return s_container_name; }
};

std::string Azure_container_tests::s_container_name = "containerut";

TEST_F(Azure_container_tests, container_create_and_delete) {
  SKIP_IF_NO_AZURE_CONFIGURATION;

  // Ensures the container is empty
  Blob_container container(get_config());
  auto objects = container.list_objects();
  EXPECT_TRUE(objects.empty());

  // Adds an object
  container.put_object("sample.txt", "Sample Content", 14);
  objects = container.list_objects("", 0, true /*, Object_details::NAME*/);
  EXPECT_EQ(1, objects.size());
  EXPECT_STREQ("sample.txt", objects[0].name.c_str());

  container.delete_object("sample.txt");
  objects = container.list_objects();
  EXPECT_TRUE(objects.empty());
}

TEST_F(Azure_container_tests, container_list_objects) {
  SKIP_IF_NO_AZURE_CONFIGURATION;

  Blob_container container(get_config());
  std::unordered_set<std::string> prefixes;

  create_objects(container);

  // DEFAULT LIST: returns name and size only
  auto objects = container.list_objects();
  EXPECT_EQ(11, objects.size());

  for (size_t index = 0; index < m_objects.size(); index++) {
    EXPECT_STREQ(m_objects[index].c_str(), objects[index].name.c_str());
    EXPECT_EQ(1, objects[index].size);
    EXPECT_FALSE(objects[index].etag.empty());
    EXPECT_FALSE(objects[index].time_created.empty());
  }

  // FIELD FILTER: just names
  objects = container.list_objects("", 0, true, Object_details::NAME);
  EXPECT_EQ(11, objects.size());

  for (size_t index = 0; index < m_objects.size(); index++) {
    EXPECT_STREQ(m_objects[index].c_str(), objects[index].name.c_str());
    EXPECT_EQ(1, objects[index].size);
    EXPECT_FALSE(objects[index].etag.empty());
    EXPECT_FALSE(objects[index].time_created.empty());
  }

  // FIELD FILTER: all fields
  objects = container.list_objects("", 0, true, Object_details::ALL_FIELDS);
  EXPECT_EQ(11, objects.size());

  for (size_t index = 0; index < m_objects.size(); index++) {
    EXPECT_STREQ(m_objects[index].c_str(), objects[index].name.c_str());
    EXPECT_EQ(1, objects[index].size);
    EXPECT_FALSE(objects[index].etag.empty());
    EXPECT_FALSE(objects[index].time_created.empty());
  }

  // NOT RECURSIVE: Gets only objects in the current "folder", i.e. nothing
  // after next /
  objects =
      container.list_objects("", 0, false, Object_details::NAME, &prefixes);

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
  objects = container.list_objects("sakila/actor", 0, true,
                                   Object_details::NAME, &prefixes);
  EXPECT_EQ(2, objects.size());
  EXPECT_STREQ("sakila/actor.csv", objects[0].name.c_str());
  EXPECT_STREQ("sakila/actor_metadata.txt", objects[1].name.c_str());
  EXPECT_TRUE(prefixes.empty());
  prefixes.clear();
}

TEST_F(Azure_container_tests, multipart_uploads) {
  SKIP_IF_NO_AZURE_CONFIGURATION;

  Blob_container container(get_config());

  // ACTIVE MULTIPART UPLOADS
  std::vector<Multipart_object_part> parts;
  auto sakila = container.create_multipart_upload("sakila.sql");

  auto mp_uploads = container.list_multipart_uploads();
  EXPECT_EQ(1, mp_uploads.size());
  EXPECT_STREQ(sakila.name.c_str(), mp_uploads[0].name.c_str());

  auto sakila_metadata =
      container.create_multipart_upload("sakila_metadata.txt");

  mp_uploads = container.list_multipart_uploads(1);
  EXPECT_EQ(1, mp_uploads.size());
  EXPECT_STREQ(sakila.name.c_str(), mp_uploads[0].name.c_str());

  mp_uploads = container.list_multipart_uploads();
  EXPECT_EQ(2, mp_uploads.size());
  EXPECT_STREQ(sakila.name.c_str(), mp_uploads[0].name.c_str());
  EXPECT_STREQ(sakila_metadata.name.c_str(), mp_uploads[1].name.c_str());

  // ABORT MULTIPART UPLOADS: NO-OP in Azure
  for (const auto &mp_upload : mp_uploads) {
    container.abort_multipart_upload(mp_upload);
  }

  mp_uploads = container.list_multipart_uploads();
  EXPECT_TRUE(mp_uploads.empty());

  // COMMIT MULTIPART UPLOAD
  const auto data = multipart_file_data();
  const auto hash = shcore::ssl::md5(data.c_str(), data.size());

  auto mp_object = container.create_multipart_upload("sakila.sql");
  std::size_t offset = 0;
  std::size_t part_number = 0;

  while (offset < k_multipart_file_size) {
    parts.push_back(container.upload_part(
        mp_object, ++part_number, data.c_str() + offset,
        std::min(k_min_part_size, k_multipart_file_size - offset)));
    offset += k_min_part_size;
  }

  container.commit_multipart_upload(mp_object, parts);

  auto objects = container.list_objects("", 0, true, Object_details::NAME);
  EXPECT_EQ(1, objects.size());
  EXPECT_STREQ(sakila.name.c_str(), objects[0].name.c_str());

  mp_uploads = container.list_multipart_uploads();
  EXPECT_TRUE(mp_uploads.empty());

  rest::String_buffer buffer{k_multipart_file_size};
  container.get_object("sakila.sql", &buffer);
  EXPECT_EQ(hash, shcore::ssl::md5(buffer.data(), buffer.size()));
}

TEST_F(Azure_container_tests, container_object_operations) {
  SKIP_IF_NO_AZURE_CONFIGURATION;

  Blob_container container(get_config());

  // PUT
  container.put_object("sakila.txt", "0123456789", 10);

  // GET Using different byte ranges
  {
    mysqlshdk::rest::String_buffer data;
    auto read = container.get_object("sakila.txt", &data);
    EXPECT_EQ(10, read);
    EXPECT_STREQ("0123456789", data.data());
  }

  {
    mysqlshdk::rest::String_buffer data;
    auto read = container.get_object("sakila.txt", &data, 0, 3);
    EXPECT_EQ(4, read);
    EXPECT_STREQ("0123", data.data());
  }

  {
    mysqlshdk::rest::String_buffer data;
    auto read = container.get_object("sakila.txt", &data, 5, 5);
    EXPECT_EQ(1, read);
    EXPECT_STREQ("5", data.data());
  }

  {
    mysqlshdk::rest::String_buffer data;
    auto read = container.get_object("sakila.txt", &data, 8, 15);
    EXPECT_EQ(2, read);
    EXPECT_STREQ("89", data.data());
  }

  {
    mysqlshdk::rest::String_buffer data;
    auto read = container.get_object("sakila.txt", &data, 4);
    EXPECT_EQ(6, read);
    EXPECT_STREQ("456789", data.data());
  }

  {
    mysqlshdk::rest::String_buffer data;
    auto read = container.get_object("sakila.txt", &data, 4);
    EXPECT_EQ(6, read);
    EXPECT_STREQ("456789", data.data());
  }

  {
    mysqlshdk::rest::String_buffer data;
    auto read = container.get_object("sakila.txt", &data, {}, 4);
    EXPECT_EQ(5, read);
    EXPECT_STREQ("01234", data.data());
  }

  {
    mysqlshdk::rest::String_buffer buffer;
    auto read = container.get_object("sakila.txt", &buffer, 5, 5);
    EXPECT_EQ(1, read);
    EXPECT_STREQ("5", buffer.data());
  }

  {
    mysqlshdk::rest::String_buffer buffer;
    auto read = container.get_object("sakila.txt", &buffer, 8, 15);
    EXPECT_EQ(2, read);
    EXPECT_STREQ("89", buffer.data());
  }
  {
    mysqlshdk::rest::String_buffer buffer;
    auto read = container.get_object("sakila.txt", &buffer, 4);
    EXPECT_EQ(6, read);
    EXPECT_STREQ("456789", buffer.data());
  }
  {
    mysqlshdk::rest::String_buffer buffer;
    auto read = container.get_object("sakila.txt", &buffer, 4);
    EXPECT_EQ(6, read);
    EXPECT_STREQ("456789", buffer.data());
  }
  {
    mysqlshdk::rest::String_buffer buffer;
    EXPECT_THROW_MSG(
        container.get_object("sakila.txt", &buffer, std::optional<size_t>{},
                             std::optional{4}),
        std::invalid_argument,
        "Retrieving partial object requires starting offset.");
  }
}

TEST_F(Azure_container_tests, container_error_conditions) {
  SKIP_IF_NO_AZURE_CONFIGURATION;

  {
    Blob_container unexisting(get_config("unexisting"));

    EXPECT_THROW_MSG_CONTAINS(
        unexisting.put_object("sample.txt", "data", 4), Response_error,
        "Failed to put object 'sample.txt': The specified "
        "container does not exist.");
  }

  const auto config = get_config();
  Blob_container container(config);

  // DELETE: Unexisting Object
  EXPECT_THROW_MSG_CONTAINS(
      container.delete_object("sample.txt"), Response_error,
      "Failed to delete object 'sample.txt': The specified blob "
      "does not exist.");

  // HEAD: Unexisnting Object
  EXPECT_THROW_MSG_CONTAINS(
      container.head_object("sample.txt"), Response_error,
      "Failed to get summary for object 'sample.txt': Not Found");

  // GET: Rename is not supported
  EXPECT_THROW_MSG_CONTAINS(
      container.rename_object("sample.txt", "other.txt"), std::runtime_error,
      "The rename_object operation is not supported in Azure.");

  Multipart_object object;
  object.name = "Sample.txt";
  EXPECT_THROW_MSG_CONTAINS(
      container.list_multipart_uploaded_parts(object), Response_error,
      "Failed to list uploaded parts for object 'Sample.txt': "
      "The specified blob does not exist.");

  // NOTE: in Azure, this test does not apply, an object is created as soon as
  // it's first part is uploaded, there's no specific message to initiate a
  // multipart upload, on this case, it will succeed EXPECT_THROW_MSG_CONTAINS(
  //     container.upload_part(object, 1, "DATA", 4), Response_error,
  //     "Failed to upload part 1 for object 'Sample.txt': No such upload");
  Multipart_object_part part;
  part.part_num = 0;
  part.size = 0;

  EXPECT_THROW_MSG_CONTAINS(
      container.commit_multipart_upload(object, {part}), Response_error,
      "Failed to commit multipart upload for object "
      "'Sample.txt': The specified block list is invalid.");
}
}  // namespace azure
}  // namespace mysqlshdk
