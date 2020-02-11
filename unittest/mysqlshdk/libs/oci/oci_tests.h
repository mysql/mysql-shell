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

#ifndef UNITTESTS_MYSQLSHDK_LIBS_OCI_OCI_TESTS_H_
#define UNITTESTS_MYSQLSHDK_LIBS_OCI_OCI_TESTS_H_

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "gtest_clean.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/oci/oci_options.h"
#include "mysqlshdk/libs/storage/backend/oci_object_storage.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "scripting/common.h"
#include "scripting/lang_base.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "unittest/test_utils.h"

extern "C" const char *g_oci_config_path;
namespace testing {

using namespace mysqlshdk::storage::backend::oci;
using mysqlshdk::oci::Oci_error;
using mysqlshdk::oci::Oci_options;
using mysqlshdk::storage::Mode;

#define SKIP_IF_NO_OCI_CONFIGURATION \
  if (!m_skip_reason.empty()) {      \
    SKIP_TEST(m_skip_reason);        \
  }

class Oci_os_tests : public Shell_core_test_wrapper {
 public:
  void SetUp() override {
    Shell_core_test_wrapper::SetUp();
    m_objects = {"sakila.sql",
                 "sakila/actor.csv",
                 "sakila/actor_metadata.txt",
                 "sakila/address.csv",
                 "sakila/address_metadata.txt",
                 "sakila/category.csv",
                 "sakila/category_metadata.txt",
                 "sakila_metadata.txt",
                 "sakila_tables.txt"};

    const auto oci_config_path =
        mysqlsh::current_shell_options()->get().oci_config_file;

    if (!shcore::path_exists(oci_config_path))
      m_skip_reason = "OCI Configuration does not exist: " + oci_config_path;
  }

 protected:
  std::string m_skip_reason;
  std::vector<std::string> m_objects;
  const std::string PUBLIC_BUCKET = "shell-rut-pub";
  const std::string PRIVATE_BUCKET = "shell-rut-priv";

  void create_objects(Bucket &bucket) {
    for (const auto &name : m_objects) {
      bucket.put_object(name, "0", 1);
    }
  }

  Oci_options get_options(const std::string bucket) {
    Oci_options options;
    options.os_bucket_name = bucket;
    options.load_defaults();
    return options;
  }

  void clean_bucket(Bucket &bucket, bool clean_uploads = true) {
    auto objects = bucket.list_objects();

    if (!objects.empty()) {
      for (const auto &object : objects) {
        bucket.delete_object(object.name);
      }

      objects = bucket.list_objects();
      EXPECT_TRUE(objects.empty());
    }

    if (clean_uploads) {
      auto uploads = bucket.list_multipart_uploads();
      if (!uploads.empty()) {
        for (const auto &upload : uploads) {
          bucket.abort_multipart_upload(upload);
        }

        uploads = bucket.list_multipart_uploads();
        EXPECT_TRUE(uploads.empty());
      }
    }
  }
};

}  // namespace testing

#endif  // UNITTESTS_MYSQLSHDK_LIBS_OCI_OCI_TESTS_H_