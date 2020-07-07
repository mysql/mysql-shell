/* Copyright (c) 2020, Oracle and/or its affiliates.

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

#include "modules/util/dump/dump_manifest.h"
#include "mysqlshdk/libs/utils/utils_time.h"

namespace testing {

using mysqlsh::dump::Dump_manifest;

struct Object {
  std::string name;
  std::string content;
};

const std::vector<Object> k_objects = {
    {"first.txt", "FIRST"}, {"second.txt", "SECOND"}, {"@.done.json", "DONE"}};

enum Obj_index { FIRST, SECOND, DONE };

void write_object(Dump_manifest *manifest, Obj_index index) {
  const auto &obj = k_objects[index];

  auto file = manifest->file(obj.name);
  file->open(Mode::WRITE);
  file->write(obj.content.data(), obj.content.size());
  EXPECT_NO_THROW(file->close());
}

bool bucket_object_exists(
    const std::string &name, ssize_t size,
    const std::vector<mysqlshdk::oci::Object_details> &objects) {
  for (const auto &bo : objects) {
    if (name == bo.name && (size == -1 || static_cast<size_t>(size) == bo.size))
      return true;
  }
  return false;
}

bool bucket_object_exists(
    Obj_index index,
    const std::vector<mysqlshdk::oci::Object_details> &objects) {
  return bucket_object_exists(k_objects[index].name,
                              k_objects[index].content.size(), objects);
}

shcore::Array_t get_manifest_content(mysqlshdk::oci::Bucket *bucket) {
  mysqlshdk::rest::String_buffer manifest_data;
  bucket->get_object("@.manifest.json", &manifest_data);
  auto manifest_map = shcore::Value::parse(manifest_data.data()).as_map();
  auto contents = manifest_map->get_array("contents");
  return contents;
}

bool manifest_object_exists(Obj_index index,
                            const shcore::Array_t &manifest_content) {
  for (const auto &mo : *manifest_content) {
    auto object = mo.as_map();

    if (k_objects[index].name == object->get_string("objectName") &&
        static_cast<uint64_t>(k_objects[index].content.size()) ==
            object->get_uint("objectSize") &&
        object->has_key("parUrl") && object->has_key("parId"))
      return true;
  }
  return false;
}

bool manifest_object_exists(
    Obj_index index,
    const std::vector<mysqlshdk::storage::IDirectory::File_info> &files) {
  for (const auto &file : files) {
    if (k_objects[index].name == file.name &&
        k_objects[index].content.size() == file.size)
      return true;
  }
  return false;
}

TEST_F(Oci_os_tests, dump_manifest_write_complete) {
  SKIP_IF_NO_OCI_CONFIGURATION

  // Ensures the bucket is empty
  Dump_manifest manifest(Dump_manifest::Mode::WRITE,
                         get_options(PRIVATE_BUCKET));
  Bucket bucket(get_options(PRIVATE_BUCKET));

  shcore::Scoped_callback cleanup([this, &bucket]() { clean_bucket(bucket); });

  write_object(&manifest, Obj_index::FIRST);

  // Manifest for the first object is written right away
  // We just wait as a safety measure
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  auto objects = bucket.list_objects("@.manifest.json");
  EXPECT_STREQ("@.manifest.json", objects[0].name.c_str());
  auto first_manifest = objects[0];

  write_object(&manifest, Obj_index::SECOND);

  // Second object is not written, but cached for 5 seconds so second manifest
  // is the same as the first (comparing size)
  objects = bucket.list_objects("@.manifest.json");
  auto second_manifest = objects[0];
  EXPECT_EQ(first_manifest.size, second_manifest.size);

  // After 5 seconds the manifest will be flushed so the new manifest will be
  // even bigger
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  objects = bucket.list_objects("@.manifest.json");
  second_manifest = objects[0];
  EXPECT_LT(first_manifest.size, second_manifest.size);

  // Writing the last expected object will also trigger the manifest flush
  write_object(&manifest, Obj_index::DONE);

  EXPECT_NO_THROW(manifest.close());
  objects = bucket.list_objects("@.manifest.json");
  auto third_manifest = objects[0];
  EXPECT_LT(second_manifest.size, third_manifest.size);

  // Verifies all the objects were written
  objects = bucket.list_objects();
  EXPECT_EQ(4, objects.size());
  EXPECT_TRUE(bucket_object_exists("@.manifest.json", -1, objects));
  EXPECT_TRUE(bucket_object_exists(Obj_index::FIRST, objects));
  EXPECT_TRUE(bucket_object_exists(Obj_index::SECOND, objects));
  EXPECT_TRUE(bucket_object_exists(Obj_index::DONE, objects));

  // Verifies what was stored on the manifest
  auto content = get_manifest_content(&bucket);
  EXPECT_EQ(3, content->size());
  EXPECT_TRUE(manifest_object_exists(Obj_index::FIRST, content));
  EXPECT_TRUE(manifest_object_exists(Obj_index::SECOND, content));
  EXPECT_TRUE(manifest_object_exists(Obj_index::DONE, content));
}

TEST_F(Oci_os_tests, dump_manifest_write_incomplete) {
  SKIP_IF_NO_OCI_CONFIGURATION

  // Ensures the bucket is empty
  Dump_manifest manifest(Dump_manifest::Mode::WRITE,
                         get_options(PRIVATE_BUCKET));
  Bucket bucket(get_options(PRIVATE_BUCKET));

  shcore::Scoped_callback cleanup([this, &bucket]() { clean_bucket(bucket); });

  write_object(&manifest, Obj_index::FIRST);

  // Manifest for the first object is written right away
  // We just wait as a safety measure
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  auto objects = bucket.list_objects("@.manifest.json");
  EXPECT_STREQ("@.manifest.json", objects[0].name.c_str());
  auto first_manifest = objects[0];

  write_object(&manifest, Obj_index::SECOND);

  // Second object is not written, but cached for 5 seconds so second manifest
  // is the same as the first (comparing size)
  objects = bucket.list_objects("@.manifest.json");
  auto second_manifest = objects[0];
  EXPECT_EQ(first_manifest.size, second_manifest.size);

  // Closing the manifest will flush cached PARs
  EXPECT_NO_THROW(manifest.close());
  objects = bucket.list_objects("@.manifest.json");
  second_manifest = objects[0];
  EXPECT_EQ(first_manifest.size, first_manifest.size);

  // Verifies all the objects were written
  objects = bucket.list_objects();
  EXPECT_EQ(3, objects.size());
  EXPECT_TRUE(bucket_object_exists("@.manifest.json", -1, objects));
  EXPECT_TRUE(bucket_object_exists(Obj_index::FIRST, objects));
  EXPECT_TRUE(bucket_object_exists(Obj_index::SECOND, objects));

  // Verifies what was stored on the manifest
  auto content = get_manifest_content(&bucket);
  EXPECT_EQ(2, content->size());
  EXPECT_TRUE(manifest_object_exists(Obj_index::FIRST, content));
}

TEST_F(Oci_os_tests, dump_manifest_read_mode) {
  SKIP_IF_NO_OCI_CONFIGURATION

  // Ensures the bucket is empty
  Dump_manifest write_manifest(Dump_manifest::Mode::WRITE,
                               get_options(PRIVATE_BUCKET));
  Bucket bucket(get_options(PRIVATE_BUCKET));

  shcore::Scoped_callback cleanup([this, &bucket]() { clean_bucket(bucket); });

  write_object(&write_manifest, Obj_index::FIRST);
  write_object(&write_manifest, Obj_index::SECOND);
  write_object(&write_manifest, Obj_index::DONE);

  write_manifest.close();

  auto time = shcore::future_time_rfc3339(std::chrono::hours(24));

  auto manifest_par = bucket.create_pre_authenticated_request(
      mysqlshdk::oci::PAR_access_type::OBJECT_READ, time, "par-manifest",
      "@.manifest.json");

  Dump_manifest read_manifest(
      Dump_manifest::Mode::READ, get_options(PRIVATE_BUCKET),
      bucket.get_rest_service()->end_point() + manifest_par.access_uri);

  auto manifest_files = read_manifest.list_files();
  EXPECT_EQ(3, manifest_files.size());
  EXPECT_TRUE(manifest_object_exists(Obj_index::FIRST, manifest_files));
  EXPECT_TRUE(manifest_object_exists(Obj_index::SECOND, manifest_files));
  EXPECT_TRUE(manifest_object_exists(Obj_index::DONE, manifest_files));

  // Tests a file from the manifest is readable
  auto first_file = read_manifest.file(k_objects[Obj_index::FIRST].name);

  // Being a file in the manifest it MUST exist
  EXPECT_TRUE(first_file->exists());

  // The file in the manifest must match the expected size
  EXPECT_EQ(k_objects[Obj_index::FIRST].content.size(),
            first_file->file_size());

  first_file->open(mysqlshdk::storage::Mode::READ);

  char buffer[20];
  auto read =
      first_file->read(buffer, k_objects[Obj_index::FIRST].content.size());
  std::string final_data(buffer, read);
  first_file->close();
  EXPECT_STREQ(k_objects[Obj_index::FIRST].content.c_str(), final_data.c_str());

  // Test the read manifest allows writing to file using Read/Write PAR
  auto rw_par = bucket.create_pre_authenticated_request(
      mysqlshdk::oci::PAR_access_type::OBJECT_READ_WRITE, time, "par-progress",
      "@.load.progress.json");
  auto new_progress_file = read_manifest.file(
      bucket.get_rest_service()->end_point() + rw_par.access_uri);

  // Being a new file created with PAR it should NOT exist
  EXPECT_FALSE(new_progress_file->exists());
  EXPECT_THROW_LIKE(new_progress_file->file_size(), std::logic_error,
                    "Size is not available for objects created using PAR: ");

  new_progress_file->open(mysqlshdk::storage::Mode::WRITE);
  new_progress_file->write("MY PROGRESS DATA", 16);
  new_progress_file->close();

  auto existing_progress_file = read_manifest.file(
      bucket.get_rest_service()->end_point() + rw_par.access_uri);

  // Being an existing file created with PAR it should exist
  EXPECT_TRUE(existing_progress_file->exists());

  existing_progress_file->open(mysqlshdk::storage::Mode::READ);

  read = existing_progress_file->read(&buffer, 16);
  std::string progress_data(buffer, read);
  existing_progress_file->close();
  EXPECT_STREQ("MY PROGRESS DATA", progress_data.c_str());

  EXPECT_THROW_LIKE(
      Dump_manifest(Dump_manifest::Mode::READ, get_options(PRIVATE_BUCKET),
                    bucket.get_rest_service()->end_point() + rw_par.access_uri),
      std::runtime_error, "Invalid manifest PAR: ");
}

}  // namespace testing
