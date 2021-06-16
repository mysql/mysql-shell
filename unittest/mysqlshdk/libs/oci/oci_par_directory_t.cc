/* Copyright (c) 2020, 2021, Oracle and/or its affiliates.

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

#include "mysqlshdk/libs/storage/backend/oci_par_directory.h"
#include "mysqlshdk/libs/utils/utils_time.h"

namespace testing {

class Oci_par_directory_tests : public Oci_os_tests {};

TEST_F(Oci_par_directory_tests, oci_par_directory_list_files) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  Bucket bucket(get_options());
  std::vector<std::string> prefixes;
  std::string next_start;

  create_objects(bucket);

  auto time = shcore::future_time_rfc3339(std::chrono::hours(24));

  {
    auto par = bucket.create_pre_authenticated_request(
        mysqlshdk::oci::PAR_access_type::ANY_OBJECT_READ, time.c_str(),
        "sample-par", "", mysqlshdk::oci::PAR_list_action::LIST_OBJECTS);

    Oci_par_directory par_dir(bucket.get_rest_service()->end_point() +
                              par.access_uri + par.object_name);

    auto objects = par_dir.list_files();

    // DEFAULT LIST: returns name and size only
    EXPECT_EQ(11, objects.size());

    for (size_t index = 0; index < m_objects.size(); index++) {
      EXPECT_STREQ(m_objects[index].c_str(), objects[index].name.c_str());
      EXPECT_EQ(1, objects[index].size);
    }

    bucket.delete_pre_authenticated_request(par.id);
  }

  {
    auto par = bucket.create_pre_authenticated_request(
        mysqlshdk::oci::PAR_access_type::ANY_OBJECT_READ, time.c_str(),
        "sample-par", "sakila/", mysqlshdk::oci::PAR_list_action::LIST_OBJECTS);

    Oci_par_directory par_dir(bucket.get_rest_service()->end_point() +
                              par.access_uri + par.object_name);

    auto objects = par_dir.list_files();
    const size_t PREFIX_LENGTH = 7;  // "sakila/"

    // DEFAULT LIST: returns name and size only
    EXPECT_EQ(6, objects.size());

    for (size_t index = 0; index < objects.size(); index++) {
      EXPECT_STREQ(m_objects[index + 1].substr(PREFIX_LENGTH).c_str(),
                   objects[index].name.c_str());
      EXPECT_EQ(1, objects[index].size);
    }

    bucket.delete_pre_authenticated_request(par.id);
  }

  clean_bucket(bucket);
}

TEST_F(Oci_par_directory_tests, oci_par_directory_filter_files) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  Bucket bucket(get_options());
  std::vector<std::string> prefixes;
  std::string next_start;

  create_objects(bucket);

  auto time = shcore::future_time_rfc3339(std::chrono::hours(24));

  {
    auto par = bucket.create_pre_authenticated_request(
        mysqlshdk::oci::PAR_access_type::ANY_OBJECT_READ, time.c_str(),
        "sample-par", "", mysqlshdk::oci::PAR_list_action::LIST_OBJECTS);

    Oci_par_directory par_dir(bucket.get_rest_service()->end_point() +
                              par.access_uri + par.object_name);

    auto objects = par_dir.filter_files("sakila*");

    // DEFAULT LIST: returns name and size only
    EXPECT_EQ(9, objects.size());

    for (size_t index = 0; index < objects.size(); index++) {
      EXPECT_STREQ(m_objects[index].c_str(), objects[index].name.c_str());
      EXPECT_EQ(1, objects[index].size);
    }

    bucket.delete_pre_authenticated_request(par.id);
  }

  clean_bucket(bucket);
}

TEST_F(Oci_par_directory_tests, oci_par_directory_file) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  Bucket bucket(get_options());

  auto time = shcore::future_time_rfc3339(std::chrono::hours(24));

  // PUT
  bucket.put_object("sakila.txt", "0123456789", 10);

  auto par = bucket.create_pre_authenticated_request(
      mysqlshdk::oci::PAR_access_type::ANY_OBJECT_READ, time.c_str(),
      "sample-par", "", mysqlshdk::oci::PAR_list_action::LIST_OBJECTS);

  Oci_par_directory par_dir(bucket.get_rest_service()->end_point() +
                            par.access_uri + par.object_name);

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
