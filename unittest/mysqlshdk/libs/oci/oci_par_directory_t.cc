/* Copyright (c) 2020, 2024, Oracle and/or its affiliates.

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

namespace testing {

using mysqlshdk::oci::Oci_bucket;
using mysqlshdk::oci::Oci_bucket_config_ptr;
using mysqlshdk::oci::PAR;
using mysqlshdk::storage::backend::oci::Oci_par_directory;
using mysqlshdk::storage::backend::oci::Oci_par_directory_config;
using mysqlshdk::storage::backend::oci::Oci_par_directory_config_ptr;

namespace {

Oci_par_directory_config_ptr create_config(const Oci_bucket_config_ptr &cfg,
                                           const PAR &par) {
  return std::make_shared<Oci_par_directory_config>(
      cfg->service_endpoint() + par.access_uri + par.object_name);
}

}  // namespace

class Oci_par_directory_tests : public Oci_os_tests {};

TEST_F(Oci_par_directory_tests, oci_par_directory_list_files) {
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

    const auto objects = par_dir.list_files();

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

TEST_F(Oci_par_directory_tests, oci_par_directory_filter_files) {
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

    const auto objects = par_dir.filter_files(filter);

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

TEST_F(Oci_par_directory_tests, oci_par_directory_file) {
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

}  // namespace testing
