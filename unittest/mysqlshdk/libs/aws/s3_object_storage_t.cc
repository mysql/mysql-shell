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

#include "mysqlshdk/libs/storage/backend/object_storage.h"

#include "unittest/mysqlshdk/libs/aws/aws_tests.h"

using mysqlshdk::rest::Response_error;
using mysqlshdk::storage::Mode;
using mysqlshdk::storage::backend::object_storage::Directory;

namespace mysqlshdk {
namespace aws {

class Object_storage_test : public Aws_s3_tests {};

TEST_P(Object_storage_test, directory_list_files) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  const auto config = get_config();
  S3_bucket bucket(config);
  Directory root_directory(config);
  Directory sakila(config, "sakila");

  // The root directory exists for sure
  EXPECT_TRUE(root_directory.exists());

  // A subdirectory only exist if created or if there are files/multipart
  // uploads
  EXPECT_FALSE(sakila.exists());

  bucket.create_multipart_upload("multipart_object.txt");
  bucket.create_multipart_upload("sakila/sakila_multipart_object.txt");

  // The directories exist if there's files or multipart uploads
  EXPECT_TRUE(root_directory.exists());
  EXPECT_TRUE(sakila.exists());

  // Normal listing doesn't include multipart uploads
  auto root_files = root_directory.list_files();
  auto expected_files = root_files;
  EXPECT_TRUE(root_files.empty());

  // With hidden files we get active multipart uploads
  root_files = root_directory.list_files(true);
  expected_files = {{"multipart_object.txt"}};
  EXPECT_EQ(expected_files, root_files);

  create_objects(bucket);

  EXPECT_STREQ("", root_directory.full_path().real().c_str());

  root_files = root_directory.list_files();
  expected_files = {{"sakila.sql"},
                    {"sakila_metadata.txt"},
                    {"sakila_tables.txt"},
                    {"uncommon%25%name.txt"},
                    {"uncommon's name.txt"}};
  EXPECT_EQ(expected_files, root_files);

  root_files = root_directory.list_files(true);
  expected_files = {{"sakila.sql"},          {"sakila_metadata.txt"},
                    {"sakila_tables.txt"},   {"uncommon%25%name.txt"},
                    {"uncommon's name.txt"}, {"multipart_object.txt"}};
  EXPECT_EQ(expected_files, root_files);

  EXPECT_STREQ("sakila", sakila.full_path().real().c_str());

  auto files = sakila.list_files();
  expected_files = {{"actor.csv"},    {"actor_metadata.txt"},
                    {"address.csv"},  {"address_metadata.txt"},
                    {"category.csv"}, {"category_metadata.txt"}};
  EXPECT_EQ(expected_files, files);

  files = sakila.list_files(true);
  expected_files = {{"actor.csv"},
                    {"actor_metadata.txt"},
                    {"address.csv"},
                    {"address_metadata.txt"},
                    {"category.csv"},
                    {"category_metadata.txt"},
                    {"sakila_multipart_object.txt"}};
  EXPECT_EQ(expected_files, files);

  {
    auto filtered = root_directory.filter_files("*");
    expected_files = {{"sakila.sql"},
                      {"sakila_metadata.txt"},
                      {"sakila_tables.txt"},
                      {"uncommon%25%name.txt"},
                      {"uncommon's name.txt"}};
    EXPECT_EQ(expected_files, filtered);
  }
  {
    auto filtered = root_directory.filter_files("sakila*");
    expected_files = {
        {"sakila.sql"}, {"sakila_metadata.txt"}, {"sakila_tables.txt"}};
    EXPECT_EQ(expected_files, filtered);
  }
  {
    auto filtered = sakila.filter_files("*");
    expected_files = {{"actor.csv"},    {"actor_metadata.txt"},
                      {"address.csv"},  {"address_metadata.txt"},
                      {"category.csv"}, {"category_metadata.txt"}};
    EXPECT_EQ(expected_files, filtered);
  }

  Directory unexisting(config, "unexisting");
  EXPECT_FALSE(unexisting.exists());
  unexisting.create();
  EXPECT_TRUE(unexisting.exists());

  clean_bucket(bucket);
}

TEST_P(Object_storage_test, file_errors) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  const auto config = get_config();
  S3_bucket bucket(config);
  Directory root(config, "prefix");

  auto file = root.file("sample.txt");

  // Attempt to rename unexisting file
  EXPECT_THROW_LIKE(
      file->rename("other.txt"), shcore::Exception,
      "Failed to rename object 'prefix/sample.txt' to 'prefix/other.txt': "
      "Failed to get summary for object 'prefix/sample.txt': Not Found (404)");

  // Attempt to open unexisting file for read
  EXPECT_THROW_LIKE(file->open(Mode::READ), shcore::Exception,
                    "Failed opening object 'prefix/sample.txt' in READ mode: "
                    "Failed to get summary for object 'prefix/sample.txt': "
                    "Not Found (404)");
}

TEST_P(Object_storage_test, file_write_simple_upload) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  const auto config = get_config();
  S3_bucket bucket(config);
  clean_bucket(bucket);
  Directory root(config);

  auto file = root.file("sample.txt");
  file->open(Mode::WRITE);
  file->write("01234", 5);
  file->write("56789", 5);
  file->write("ABCDE", 5);

  auto in_progress_uploads = bucket.list_multipart_uploads();
  EXPECT_TRUE(in_progress_uploads.empty());
  file->close();

  file->open(Mode::READ);
  char buffer[20];
  size_t read = file->read(buffer, 20);
  EXPECT_EQ(15, read);
  std::string data(buffer, read);
  EXPECT_STREQ("0123456789ABCDE", data.c_str());
  file->close();

  bucket.delete_object("sample.txt");
}

TEST_P(Object_storage_test, file_write_multipart_upload) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  auto config = get_config();
  config->set_part_size(k_min_part_size);
  S3_bucket bucket(config);
  Directory root(config, "test");

  auto file = root.file("sample\".txt");

  const auto data = multipart_file_data();
  size_t offset = 0;
  int writes = 0;

  file->open(Mode::WRITE);

  while (offset < k_multipart_file_size) {
    ++writes;
    offset += file->write(
        data.data() + offset,
        std::min(k_min_part_size + 1, k_multipart_file_size - offset));
  }

  auto uploads = bucket.list_multipart_uploads();
  EXPECT_EQ(1, uploads.size());
  EXPECT_STREQ("test/sample\".txt", uploads[0].name.c_str());
  auto parts = bucket.list_multipart_uploaded_parts(uploads[0]);
  EXPECT_EQ(writes - 1, parts.size());  // Last part is still on the buffer

  file->close();
  EXPECT_THROW_LIKE(
      bucket.list_multipart_uploaded_parts(uploads[0]), Response_error,
      "Failed to list uploaded parts for object 'test/sample\".txt': ");
  uploads = bucket.list_multipart_uploads();
  EXPECT_TRUE(uploads.empty());

  file->open(Mode::READ);
  std::string buffer;
  buffer.resize(k_multipart_file_size + 5);
  size_t read = file->read(buffer.data(), buffer.size());
  EXPECT_EQ(k_multipart_file_size, read);
  buffer.resize(read);
  EXPECT_EQ(data, buffer);
  file->close();

  bucket.delete_object("test/sample\".txt");
}

TEST_P(Object_storage_test, file_append_new_file) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  const auto config = get_config();
  S3_bucket bucket(config);
  Directory root(config);

  auto file = root.file("sample.txt");
  file->open(Mode::APPEND);
  file->write("01234", 5);
  file->write("56789", 5);
  file->write("ABCDE", 5);

  auto in_progress_uploads = bucket.list_multipart_uploads();
  EXPECT_TRUE(in_progress_uploads.empty());
  file->close();

  file->open(Mode::READ);
  char buffer[20];
  size_t read = file->read(buffer, 20);
  EXPECT_EQ(15, read);
  std::string data(buffer, read);
  EXPECT_STREQ("0123456789ABCDE", data.c_str());
  file->close();

  bucket.delete_object("sample.txt");
}

TEST_P(Object_storage_test, file_append_resume_interrupted_upload) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  auto config = get_config();
  config->set_part_size(k_min_part_size);
  S3_bucket bucket(config);
  Directory root(config);

  auto initial_file = root.file("sample.txt");

  const auto data = multipart_file_data();
  size_t offset = 0;
  int writes = 0;

  initial_file->open(Mode::WRITE);

  while (offset + k_min_part_size + 1 < k_multipart_file_size) {
    ++writes;
    offset += initial_file->write(data.data() + offset, k_min_part_size + 1);
  }

  auto uploads = bucket.list_multipart_uploads();
  EXPECT_EQ(1, uploads.size());
  EXPECT_STREQ("sample.txt", uploads[0].name.c_str());
  auto parts = bucket.list_multipart_uploaded_parts(uploads[0]);
  EXPECT_EQ(writes, parts.size());

  // INTERRUPTION: We stop writing to initial_file as it got interrupted
  // At this point the file is an active multipart upload:
  // Sent Parts: part1, ..., partN
  // Buffered (lost): data which did not fit into a part

  // RESUME THE UPLOAD
  auto final_file = root.file("sample.txt");

  final_file->open(Mode::APPEND);
  offset = final_file->file_size();

  while (offset < k_multipart_file_size) {
    ++writes;
    offset += final_file->write(
        data.data() + offset,
        std::min(k_min_part_size + 1, k_multipart_file_size - offset));
  }

  uploads = bucket.list_multipart_uploads();
  EXPECT_EQ(1, uploads.size());
  EXPECT_STREQ("sample.txt", uploads[0].name.c_str());
  parts = bucket.list_multipart_uploaded_parts(uploads[0]);
  EXPECT_EQ(writes - 1, parts.size());  // Last part is still on the buffer

  final_file->close();
  EXPECT_THROW_LIKE(bucket.list_multipart_uploaded_parts(uploads[0]),
                    Response_error,
                    "Failed to list uploaded parts for object 'sample.txt': ");
  uploads = bucket.list_multipart_uploads();
  EXPECT_TRUE(uploads.empty());

  final_file->open(Mode::READ);
  std::string buffer;
  buffer.resize(k_multipart_file_size + 5);
  size_t read = final_file->read(buffer.data(), buffer.size());
  EXPECT_EQ(k_multipart_file_size, read);
  buffer.resize(read);
  EXPECT_EQ(data, buffer);
  final_file->close();

  bucket.delete_object("sample.txt");
}

TEST_P(Object_storage_test, file_append_existing_file) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  const auto config = get_config();
  S3_bucket bucket(config);
  Directory root(config);

  auto file = root.file("sample.txt");
  file->open(Mode::WRITE);
  file->write("01234", 5);
  file->close();

  // APPEND is forbidden here as the file exist and there' no active multipart
  // upload
  EXPECT_THROW_LIKE(file->open(Mode::APPEND), std::invalid_argument,
                    "Object Storage only supports APPEND mode for in-progress "
                    "multipart uploads or new files.");

  // Now APPEND should be allowed
  bucket.create_multipart_upload("sample.txt");
  file->open(Mode::APPEND);
  file->write("67890", 5);
  file->close();

  file->open(Mode::READ);
  char buffer[10];
  size_t read = file->read(buffer, 10);
  EXPECT_EQ(5, read);
  std::string final_data(buffer, read);
  EXPECT_STREQ("67890", final_data.c_str());
  file->close();

  bucket.delete_object("sample.txt");
}

TEST_P(Object_storage_test, file_write_multipart_errors) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  auto config = get_config();
  config->set_part_size(3);
  S3_bucket bucket(config);
  Directory root(config);
  // Now APPEND should be allowed
  auto mpo1 = bucket.create_multipart_upload("sample.txt");
  auto file = root.file("sample.txt");

  file->open(Mode::APPEND);

  bucket.abort_multipart_upload(mpo1);

  EXPECT_THROW_LIKE(file->write("67890", 5), shcore::Exception,
                    "Failed to upload part 1 for object 'sample.txt': ");

  EXPECT_THROW_LIKE(
      file->close(), shcore::Exception,
      "Failed to commit multipart upload for object 'sample.txt': ");
}

TEST_P(Object_storage_test, file_writing) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  const auto config = get_config();

  Directory root(config);

  auto file = root.file("sample.txt");

  EXPECT_FALSE(file->exists());
  file->open(mysqlshdk::storage::Mode::WRITE);
  file->write("SOME", 4);
  std::string test;
  test.append(15, 'a');
  file->write(test.data(), test.size());
  file->write("END", 3);
  file->close();
  EXPECT_TRUE(file->exists());

  auto another = root.file("sample.txt");
  EXPECT_TRUE(another->exists());

  char buffer[30];
  another->open(Mode::READ);
  size_t read = another->read(&buffer, 4);
  EXPECT_EQ(4, read);
  std::string data(buffer, read);
  EXPECT_STREQ("SOME", data.c_str());
  read = another->read(&buffer, 15);
  EXPECT_EQ(15, read);
  data.assign(buffer, read);
  EXPECT_STREQ(test.c_str(), data.c_str());
  read = another->read(&buffer, 3);
  EXPECT_EQ(3, read);
  data.assign(buffer, read);
  EXPECT_STREQ("END", data.c_str());

  another->seek(2);
  read = another->read(&buffer, 5);
  EXPECT_EQ(5, read);
  data.assign(buffer, read);
  EXPECT_STREQ("MEaaa", data.c_str());

  another->seek(0);
  read = another->read(&buffer, 30);
  EXPECT_EQ(22, read);
  data.assign(buffer, read);
  EXPECT_STREQ("SOMEaaaaaaaaaaaaaaaEND", data.c_str());

  another->seek(22);
  read = another->read(&buffer, 30);
  EXPECT_EQ(0, read);

  S3_bucket bucket(config);
  bucket.delete_object("sample.txt");
}

TEST_P(Object_storage_test, file_rename) {
  SKIP_IF_NO_AWS_CONFIGURATION;

  const auto config = get_config();
  S3_bucket bucket(config);
  Directory root(config);

  auto file = root.file("sample.txt");

  file->open(Mode::WRITE);
  file->write("SOME CONTENT", 12);
  file->close();

  EXPECT_STREQ("sample.txt", file->filename().c_str());
  EXPECT_STREQ("sample.txt", file->full_path().real().c_str());

  file->rename("testing.txt");
  EXPECT_STREQ("testing.txt", file->filename().c_str());
  EXPECT_STREQ("testing.txt", file->full_path().real().c_str());

  auto files = root.list_files();
  auto expected_files = files;
  expected_files = {{"testing.txt"}};
  EXPECT_EQ(expected_files, files);

  file->rename("`test`");
  EXPECT_STREQ("`test`", file->filename().c_str());
  EXPECT_STREQ("`test`", file->full_path().real().c_str());

  file->rename("_test1~");
  EXPECT_STREQ("_test1~", file->filename().c_str());
  EXPECT_STREQ("_test1~", file->full_path().real().c_str());

  file->rename("\"ois\"");
  EXPECT_STREQ("\"ois\"", file->filename().c_str());
  EXPECT_STREQ("\"ois\"", file->full_path().real().c_str());

  bucket.delete_object("\"ois\"");

  Directory other(config, "other");
  file = other.file("sample.txt");
  file->open(Mode::WRITE);
  file->write("SOME CONTENT", 12);
  file->close();

  EXPECT_STREQ("sample.txt", file->filename().c_str());
  EXPECT_STREQ("other/sample.txt", file->full_path().real().c_str());

  file->rename("testing.txt");
  EXPECT_STREQ("testing.txt", file->filename().c_str());
  EXPECT_STREQ("other/testing.txt", file->full_path().real().c_str());

  files = other.list_files();
  EXPECT_EQ(expected_files, files);

  bucket.delete_object("other/testing.txt");
}

INSTANTIATE_TEST_SUITE_P(Aws_s3, Object_storage_test, suffixes(),
                         format_suffix);

}  // namespace aws
}  // namespace mysqlshdk
