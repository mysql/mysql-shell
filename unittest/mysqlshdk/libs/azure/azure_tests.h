/* Copyright (c) 2022, 2024, Oracle and/or its affiliates.

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

#ifndef UNITTEST_MYSQLSHDK_LIBS_AZURE_AZURE_TESTS_H_
#define UNITTEST_MYSQLSHDK_LIBS_AZURE_AZURE_TESTS_H_

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

#include "mysqlshdk/libs/azure/blob_storage_config.h"
#include "mysqlshdk/libs/azure/blob_storage_container.h"
#include "mysqlshdk/libs/azure/blob_storage_options.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "unittest/mysqlshdk/libs/azure/test_utils.h"

namespace mysqlshdk {
namespace azure {

using mysqlshdk::azure::Blob_container;
using mysqlshdk::azure::Blob_storage_config;
using mysqlshdk::azure::Blob_storage_options;
using mysqlshdk::azure::Storage_config_ptr;

#define SKIP_IF_NO_AZURE_CONFIGURATION \
  do {                                 \
    if (should_skip()) {               \
      SKIP_TEST(skip_reason());        \
    }                                  \
  } while (false)

class Azure_tests : public Shell_core_test_wrapper {
 public:
  void SetUp() override {
    Shell_core_test_wrapper::SetUp();

    // Note that these object names are in alphabetical order on purpose
    m_objects = {"sakila.sql",
                 "sakila/actor.csv",
                 "sakila/actor_metadata.txt",
                 "sakila/address.csv",
                 "sakila/address_metadata.txt",
                 "sakila/category.csv",
                 "sakila/category_metadata.txt",
                 "sakila_metadata.txt",
                 "sakila_tables.txt",
                 "uncommon%25%name.txt",
                 "uncommon's name.txt"};

    auto config = testing::get_config(container_name());
    if (!config || !config->valid()) {
      skip("Missing or invalid Azure Storage Connection settings");
    }
  }

  // ===================== IMPLEMENTATION NOTES =====================
  // In Azure, the deletion of a container is not synchronous, based on the
  // Delete Container documentation
  // - When a container is deleted, a container with the same name can't be
  //   created for at least 30 seconds.
  // - While the container is being deleted, attempts to create a container of
  //   the same name fail with status code 409 (Conflict).
  // - All other operations, including operations on any blobs under the
  //   container, fail with status code 404 (Not Found) while the container is
  //   being deleted.
  //
  // Which means the container simply can not be created/deleted in SetUp as
  // it will cause all the tests after the first one to fail as indicated
  // above.
  //
  // Moving the creation/deletion to SetUpTestCase/TearDownTestCase is not
  // enough as it will cause the first test suite to succeed, but next test
  // suites may fail if they starts while the container deletion of the
  // previous test is in progress (unless a different container name is used
  // for each test).
  //
  // For this reason, the following functions MUST be implemented by the child
  // classes, ensuring all of them use a unique container name:
  //
  // static std::string s_container_name;
  // static void SetUpTestCase() {
  // testing::create_container(s_container_name); } static void
  // TearDownTestCase() { testing::delete_container(s_container_name); }
  // std::string container_name() override {return s_container_name;}
  // std::string <class-name>::s_container_name = "<unique-container-name>";
  virtual std::string container_name() = 0;

  void TearDown() override {
    if (should_skip()) return;

    Blob_container container(testing::get_config(container_name()));

    testing::clean_container(container);
  }

 protected:
  std::string multipart_file_data() const {
    return shcore::get_random_string(k_multipart_file_size, "0123456789ABCDEF");
  }

  void create_objects(Blob_container &container) {
    for (const auto &name : m_objects) {
      container.put_object(name, "0", 1);
    }
  }

  std::shared_ptr<Blob_storage_config> get_config(
      const std::string &name = "",
      mysqlshdk::azure::Blob_storage_options::Operation operation =
          mysqlshdk::azure::Blob_storage_options::Operation::WRITE) {
    return testing::get_config(name.empty() ? container_name() : name,
                               operation);
  }

  void skip(const std::string &reason) { m_skip_reasons.emplace_back(reason); }

  bool should_skip() const { return !m_skip_reasons.empty(); }

  std::string skip_reason() const {
    return shcore::str_join(m_skip_reasons, "\n");
  }

  std::vector<std::string> m_skip_reasons;
  static constexpr std::size_t k_min_part_size = 5242880;  // 5 MiB
  static constexpr std::size_t k_multipart_file_size = k_min_part_size + 1024;
  static constexpr std::size_t k_max_part_size = std::min<uint64_t>(
      UINT64_C(5368709120), std::numeric_limits<std::size_t>::max());

  std::vector<std::string> m_objects;
};

}  // namespace azure
}  // namespace mysqlshdk

#endif  // UNITTEST_MYSQLSHDK_LIBS_AZURE_AZURE_TESTS_H_
