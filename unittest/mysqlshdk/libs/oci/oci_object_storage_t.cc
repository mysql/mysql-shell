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
TEST_F(Oci_os_tests, directory_list_files) {
  SKIP_IF_NO_OCI_CONFIGURATION

  Oci_options options{get_options(PRIVATE_BUCKET)};
  Bucket bucket(options);
  create_objects(bucket);

  Directory root_directory(options);
  EXPECT_TRUE(root_directory.exists());
  EXPECT_STREQ("", root_directory.full_path().c_str());

  auto root_files = root_directory.list_files();
  EXPECT_EQ(3, root_files.size());
  EXPECT_STREQ("sakila.sql", root_files[0].name.c_str());
  EXPECT_STREQ("sakila_metadata.txt", root_files[1].name.c_str());
  EXPECT_STREQ("sakila_tables.txt", root_files[2].name.c_str());

  Directory sakila(options, "sakila");
  EXPECT_TRUE(sakila.exists());
  EXPECT_STREQ("sakila", sakila.full_path().c_str());

  auto files = sakila.list_files();
  EXPECT_EQ(6, files.size());
  EXPECT_STREQ("actor.csv", files[0].name.c_str());
  EXPECT_STREQ("actor_metadata.txt", files[1].name.c_str());
  EXPECT_STREQ("address.csv", files[2].name.c_str());
  EXPECT_STREQ("address_metadata.txt", files[3].name.c_str());
  EXPECT_STREQ("category.csv", files[4].name.c_str());
  EXPECT_STREQ("category_metadata.txt", files[5].name.c_str());

  Directory unexisting(options, "unexisting");
  EXPECT_FALSE(unexisting.exists());
  unexisting.create();
  EXPECT_TRUE(unexisting.exists());

  clean_bucket(bucket);
}

TEST_F(Oci_os_tests, file_write_simple_upload) {
  SKIP_IF_NO_OCI_CONFIGURATION

  Oci_options options{get_options(PRIVATE_BUCKET)};
  Bucket bucket(options);
  Directory root(options, "");

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

TEST_F(Oci_os_tests, file_write_multipart_upload) {
  SKIP_IF_NO_OCI_CONFIGURATION

  Oci_options options{get_options(PRIVATE_BUCKET)};
  Bucket bucket(options);
  Directory root(options, "");

  auto file = root.file("sample.txt");
  auto oci_file =
      dynamic_cast<mysqlshdk::storage::backend::oci::Object *>(file.get());
  oci_file->set_max_part_size(3);

  std::string data = "0123456789ABCDE";
  size_t offset = 0;

  file->open(Mode::WRITE);
  offset += file->write(data.data() + offset, 5);
  offset += file->write(data.data() + offset, 5);
  offset += file->write(data.data() + offset, 5);

  auto uploads = bucket.list_multipart_uploads();
  EXPECT_EQ(1, uploads.size());
  EXPECT_STREQ("sample.txt", uploads[0].name.c_str());
  auto parts = bucket.list_multipart_upload_parts(uploads[0]);
  EXPECT_EQ(4, parts.size());  // Last part is still on the buffer

  file->close();
  EXPECT_THROW_LIKE(bucket.list_multipart_upload_parts(uploads[0]),
                    Response_error, "No such upload");
  uploads = bucket.list_multipart_uploads();
  EXPECT_TRUE(uploads.empty());

  file->open(Mode::READ);
  char buffer[20];
  size_t read = file->read(buffer, 20);
  EXPECT_EQ(15, read);
  std::string final_data(buffer, read);
  EXPECT_STREQ("0123456789ABCDE", final_data.c_str());
  file->close();

  bucket.delete_object("sample.txt");
}

TEST_F(Oci_os_tests, file_write_resume_interrupted_upload) {
  SKIP_IF_NO_OCI_CONFIGURATION

  Oci_options options{get_options(PRIVATE_BUCKET)};
  Bucket bucket(options);
  Directory root(options, "");

  auto initial_file = root.file("sample.txt");
  auto oci_file = dynamic_cast<mysqlshdk::storage::backend::oci::Object *>(
      initial_file.get());
  oci_file->set_max_part_size(3);

  std::string data = "0123456789ABCDE";
  size_t offset = 0;

  initial_file->open(Mode::WRITE);
  offset += initial_file->write(data.data() + offset, 5);
  offset += initial_file->write(data.data() + offset, 5);

  // INTERRUPTION: We stop writing to initial_file as it got interrupted
  // At this point the file is an active multipart upload:
  // Sent Parts: 012, 345, 678
  // Buffered (lost): 9

  // RESUME THE UPLOAD
  auto final_file = root.file("sample.txt");
  oci_file = dynamic_cast<mysqlshdk::storage::backend::oci::Object *>(
      final_file.get());
  oci_file->set_max_part_size(3);

  final_file->open(Mode::APPEND);
  offset = final_file->file_size();
  offset += final_file->write(data.data() + offset, 5);
  auto uploads = bucket.list_multipart_uploads();
  EXPECT_EQ(1, uploads.size());
  EXPECT_STREQ("sample.txt", uploads[0].name.c_str());
  auto parts = bucket.list_multipart_upload_parts(uploads[0]);
  EXPECT_EQ(4, parts.size());

  offset += final_file->write(data.data() + offset, data.size() - offset);

  final_file->close();
  EXPECT_THROW_LIKE(bucket.list_multipart_upload_parts(uploads[0]),
                    Response_error, "No such upload");
  uploads = bucket.list_multipart_uploads();
  EXPECT_TRUE(uploads.empty());

  final_file->open(Mode::READ);
  char buffer[20];
  size_t read = final_file->read(buffer, 20);
  EXPECT_EQ(15, read);
  std::string final_data(buffer, read);
  EXPECT_STREQ("0123456789ABCDE", final_data.c_str());
  final_file->close();

  bucket.delete_object("sample.txt");
}

TEST_F(Oci_os_tests, file_writing) {
  SKIP_IF_NO_OCI_CONFIGURATION

  Oci_options options{get_options(PRIVATE_BUCKET)};

  Directory root(options);

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

  Bucket bucket(options);
  bucket.delete_object("sample.txt");
}

TEST_F(Oci_os_tests, file_rename) {
  SKIP_IF_NO_OCI_CONFIGURATION

  Oci_options options{get_options(PRIVATE_BUCKET)};
  Bucket bucket(options);
  bucket.put_object("sample.txt", "SOME CONTENT", 12);

  Directory root(options, "");
  auto file = root.file("sample.txt");

  file->rename("testing.txt");

  auto files = root.list_files();
  EXPECT_EQ(1, files.size());
  EXPECT_STREQ("testing.txt", files[0].name.c_str());

  bucket.delete_object("testing.txt");
}
}  // namespace testing