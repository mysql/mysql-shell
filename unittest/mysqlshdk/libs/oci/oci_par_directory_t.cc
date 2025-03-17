/* Copyright (c) 2020, 2025, Oracle and/or its affiliates.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2.0,
 as published by the Free Software Foundation.

 This program is designed to work with certain software (including
 but not limited to OpenSSL) that is licensed under separate terms,
 as designated in a particular file or component or in included license
 documentation.  The authors of MySQL hereby grant you an additional
 permission to link the program and your derivative works with the
 separately licensed software that they have either included with
 the program or referenced in the documentation.

 This program is distributed in the hope that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 the GNU General Public License, version 2.0, for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include "unittest/mysqlshdk/libs/oci/oci_tests.h"

#include "mysqlshdk/libs/storage/backend/oci_par_directory.h"
#include "mysqlshdk/libs/storage/backend/oci_par_directory_config.h"
#include "mysqlshdk/libs/utils/utils_time.h"

namespace mysqlshdk {
namespace storage {
namespace backend {
namespace oci {

using mysqlshdk::oci::Oci_bucket;
using mysqlshdk::oci::Oci_bucket_config_ptr;
using mysqlshdk::oci::PAR;

namespace {

std::shared_ptr<Oci_par_directory_config> create_config(
    const Oci_bucket_config_ptr &cfg, const PAR &par) {
  return std::make_shared<Oci_par_directory_config>(
      cfg->service_endpoint() + par.access_uri + par.object_name);
}

}  // namespace

class Oci_par_directory_tests : public mysqlshdk::oci::Oci_os_tests {};

TEST_F(Oci_par_directory_tests, list_files) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  const auto config = get_config();
  Oci_bucket bucket(config);
  std::vector<std::string> prefixes;
  std::string next_start;

  create_objects(bucket);

  auto time = shcore::future_time_rfc3339(std::chrono::hours(24));

  const auto EXPECT_LIST_FILES = [&](const std::string &prefix) {
    SCOPED_TRACE(prefix);

    std::vector<std::string> expected_objects;

    for (auto object : m_objects) {
      bool match = false;

      if (prefix.empty()) {
        match = true;
      } else {
        if (shcore::str_beginswith(object, prefix)) {
          object = object.substr(prefix.length());

          match = true;
        }
      }

      if (match && std::string::npos == object.find('/')) {
        expected_objects.emplace_back(std::move(object));
      }
    }

    auto par = bucket.create_pre_authenticated_request(
        mysqlshdk::oci::PAR_access_type::ANY_OBJECT_READ, time.c_str(),
        "sample-par", prefix, mysqlshdk::oci::PAR_list_action::LIST_OBJECTS);

    Oci_par_directory par_dir(create_config(config, par));

    const auto objects = par_dir.list().files;

    // DEFAULT LIST: returns name and size only
    ASSERT_EQ(expected_objects.size(), objects.size());

    for (const auto &object : expected_objects) {
      const auto o = objects.find(object);
      ASSERT_NE(objects.end(), o);
      EXPECT_EQ(1, o->size());
    }

    bucket.delete_pre_authenticated_request(par.id);
  };

  EXPECT_LIST_FILES("");
  EXPECT_LIST_FILES("sakila/");

  clean_bucket(bucket);
}

TEST_F(Oci_par_directory_tests, filter_files) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  const auto config = get_config();
  Oci_bucket bucket(config);
  std::vector<std::string> prefixes;
  std::string next_start;

  create_objects(bucket);

  auto time = shcore::future_time_rfc3339(std::chrono::hours(24));

  const auto EXPECT_FILTER_FILES = [&](const std::string &prefix,
                                       const std::string &filter) {
    SCOPED_TRACE(prefix);

    std::vector<std::string> expected_objects;

    for (auto object : m_objects) {
      bool match = false;

      if (prefix.empty()) {
        match = true;
      } else {
        if (shcore::str_beginswith(object, prefix)) {
          object = object.substr(prefix.length());

          match = true;
        }
      }

      if (match && std::string::npos == object.find('/') &&
          shcore::match_glob(filter, object)) {
        expected_objects.emplace_back(std::move(object));
      }
    }

    auto par = bucket.create_pre_authenticated_request(
        mysqlshdk::oci::PAR_access_type::ANY_OBJECT_READ, time.c_str(),
        "sample-par", prefix, mysqlshdk::oci::PAR_list_action::LIST_OBJECTS);

    Oci_par_directory par_dir(create_config(config, par));

    const auto objects =
        mysqlshdk::storage::filter(par_dir.list().files, filter);

    // DEFAULT LIST: returns name and size only
    ASSERT_EQ(expected_objects.size(), objects.size());

    for (const auto &object : expected_objects) {
      const auto o = objects.find(object);
      ASSERT_NE(objects.end(), o);
      EXPECT_EQ(1, o->size());
    }

    bucket.delete_pre_authenticated_request(par.id);
  };

  EXPECT_FILTER_FILES("", "sakila*");
  EXPECT_FILTER_FILES("sakila/", "category*");

  clean_bucket(bucket);
}

TEST_F(Oci_par_directory_tests, subdirectory) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  const auto create_file =
      [](const std::unique_ptr<mysqlshdk::storage::IDirectory> &d,
         const std::string &name) {
        const auto file = d->file(name);
        const auto full_path = file->full_path().real();

        file->open(mysqlshdk::storage::Mode::WRITE);
        file->write(full_path.c_str(), full_path.length());
        file->close();
      };

  mysqlshdk::storage::File_list expected_files;
  mysqlshdk::storage::Directory_list expected_dirs;

  const auto config = get_config();
  auto bucket = Oci_bucket{config};

  const auto par = bucket.create_pre_authenticated_request(
      mysqlshdk::oci::PAR_access_type::ANY_OBJECT_READ_WRITE,
      shcore::future_time_rfc3339(std::chrono::hours(24)), "sample-par", "",
      mysqlshdk::oci::PAR_list_action::LIST_OBJECTS);
  const auto par_config = create_config(config, par);

  const auto root = mysqlshdk::storage::make_directory(
      par_config->anonymized_full_url().real(), par_config);

  EXPECT_TRUE(root->is_empty());
  EXPECT_TRUE(root->list().files.empty());
  EXPECT_TRUE(root->list().directories.empty());

  create_file(root, "one.txt");
  create_file(root, "two.txt");

  EXPECT_FALSE(root->is_empty());
  expected_files = {{"one.txt"}, {"two.txt"}};
  EXPECT_EQ(expected_files, root->list().files);

  const auto first = root->directory("first");

  EXPECT_TRUE(first->is_empty());
  EXPECT_TRUE(first->list().files.empty());
  EXPECT_TRUE(first->list().directories.empty());

  create_file(first, "three.txt");
  create_file(first, "four.txt");

  EXPECT_FALSE(first->is_empty());
  expected_files = {{"three.txt"}, {"four.txt"}};
  EXPECT_EQ(expected_files, first->list().files);

  const auto second = root->directory("second");

  EXPECT_TRUE(second->is_empty());
  EXPECT_TRUE(second->list().files.empty());
  EXPECT_TRUE(second->list().directories.empty());

  create_file(second, "five.txt");
  create_file(second, "six.txt");

  EXPECT_FALSE(second->is_empty());
  expected_files = {{"five.txt"}, {"six.txt"}};
  EXPECT_EQ(expected_files, second->list().files);

  const auto third = first->directory("third");

  EXPECT_TRUE(third->is_empty());
  EXPECT_TRUE(third->list().files.empty());
  EXPECT_TRUE(third->list().directories.empty());

  create_file(third, "seven.txt");
  create_file(third, "eight.txt");

  EXPECT_FALSE(third->is_empty());
  expected_files = {{"seven.txt"}, {"eight.txt"}};
  EXPECT_EQ(expected_files, third->list().files);

  const auto fourth = first->directory("fourth");

  EXPECT_TRUE(fourth->is_empty());
  EXPECT_TRUE(fourth->list().files.empty());
  EXPECT_TRUE(fourth->list().directories.empty());

  create_file(fourth, "nine.txt");
  create_file(fourth, "ten.txt");

  EXPECT_FALSE(fourth->is_empty());
  expected_files = {{"nine.txt"}, {"ten.txt"}};
  EXPECT_EQ(expected_files, fourth->list().files);

  // directories are visible only once all files are created
  expected_dirs = {{"first"}, {"second"}};
  EXPECT_EQ(expected_dirs, root->list().directories);

  expected_dirs = {{"third"}, {"fourth"}};
  EXPECT_EQ(expected_dirs, first->list().directories);

  EXPECT_TRUE(second->list().directories.empty());
  EXPECT_TRUE(third->list().directories.empty());
  EXPECT_TRUE(fourth->list().directories.empty());

  // relative paths
  const auto fourth_again = third->directory("../fourth");

  expected_files = {{"nine.txt"}, {"ten.txt"}};
  EXPECT_EQ(expected_files, fourth_again->list().files);

  EXPECT_NO_THROW(fourth->remove());
}

TEST_F(Oci_par_directory_tests, file) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  const auto config = get_config();
  Oci_bucket bucket(config);

  auto time = shcore::future_time_rfc3339(std::chrono::hours(24));

  // PUT
  bucket.put_object("sakila.txt", "0123456789", 10);

  auto par = bucket.create_pre_authenticated_request(
      mysqlshdk::oci::PAR_access_type::ANY_OBJECT_READ, time.c_str(),
      "sample-par", "", mysqlshdk::oci::PAR_list_action::LIST_OBJECTS);

  Oci_par_directory par_dir(create_config(config, par));

  // GET Using different byte ranges
  {
    char buffer[20];
    auto file = par_dir.file("sakila.txt");
    file->open(mysqlshdk::storage::Mode::READ);
    size_t read = file->read(buffer, 20);
    file->close();

    EXPECT_EQ(10, read);
    std::string data(buffer, read);
    EXPECT_STREQ("0123456789", data.c_str());
  }

  {
    char buffer[20];
    auto file = par_dir.file("sakila.txt");
    file->open(mysqlshdk::storage::Mode::READ);
    size_t read = file->read(buffer, 2);
    EXPECT_EQ(2, read);
    std::string data1(buffer, read);
    EXPECT_STREQ("01", data1.c_str());

    read = file->read(buffer, 3);
    EXPECT_EQ(3, read);
    std::string data2(buffer, read);
    EXPECT_STREQ("234", data2.c_str());

    read = file->read(buffer, 4);
    EXPECT_EQ(4, read);
    std::string data3(buffer, read);
    EXPECT_STREQ("5678", data3.c_str());

    read = file->read(buffer, 10);
    EXPECT_EQ(1, read);
    std::string data4(buffer, read);
    EXPECT_STREQ("9", data4.c_str());
    file->close();
  }

  bucket.delete_pre_authenticated_request(par.id);
}

TEST_F(Oci_par_directory_tests, file_write_multipart_upload) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  const auto oci_config = get_config();
  Oci_bucket bucket{oci_config};

  const auto time = shcore::future_time_rfc3339(std::chrono::hours(24));
  const auto par = bucket.create_pre_authenticated_request(
      mysqlshdk::oci::PAR_access_type::ANY_OBJECT_READ_WRITE, time,
      "sample-par", "test/", mysqlshdk::oci::PAR_list_action::LIST_OBJECTS);

  auto par_config = create_config(oci_config, par);
  par_config->set_part_size(3);

  Oci_par_directory par_dir{par_config};
  const auto file = par_dir.file("sample\".txt");

  {
    const std::string data = "0123456789ABCDE";
    size_t offset = 0;

    file->open(Mode::WRITE);
    offset += file->write(data.data() + offset, 5);
    offset += file->write(data.data() + offset, 5);
    offset += file->write(data.data() + offset, 5);
    EXPECT_EQ(offset, data.size());

    file->close();
  }

  {
    file->open(Mode::READ);
    char buffer[20];
    const auto read = file->read(buffer, 20);
    EXPECT_EQ(15, read);

    const std::string final_data(buffer, read);
    EXPECT_STREQ("0123456789ABCDE", final_data.c_str());

    file->close();
  }
}

TEST_F(Oci_par_directory_tests, file_auto_cancel_multipart_upload) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  const auto oci_config = get_config();
  Oci_bucket bucket{oci_config};

  const auto time = shcore::future_time_rfc3339(std::chrono::hours(24));
  const auto par = bucket.create_pre_authenticated_request(
      mysqlshdk::oci::PAR_access_type::ANY_OBJECT_READ_WRITE, time,
      "sample-par", "test/", mysqlshdk::oci::PAR_list_action::LIST_OBJECTS);

  auto par_config = create_config(oci_config, par);
  par_config->set_part_size(3);

  Oci_par_directory par_dir{par_config};
  auto file = par_dir.file("sample@db.txt");

  {
    const std::string data = "0123456789ABCDE";
    size_t offset = 0;

    file->open(Mode::WRITE);
    offset += file->write(data.data() + offset, 5);
    offset += file->write(data.data() + offset, 5);
    offset += file->write(data.data() + offset, 5);
  }

  // release the file, simulating abnormal situation (close() was not called,
  // destructor should clean up the upload)
  file.reset();
}

}  // namespace oci
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
