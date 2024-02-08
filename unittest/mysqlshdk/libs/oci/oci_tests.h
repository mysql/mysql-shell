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

#ifndef UNITTEST_MYSQLSHDK_LIBS_OCI_OCI_TESTS_H_
#define UNITTEST_MYSQLSHDK_LIBS_OCI_OCI_TESTS_H_

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

#include "modules/util/dump/dump_manifest_config.h"
#include "modules/util/dump/dump_manifest_options.h"
#include "mysqlshdk/include/scripting/common.h"
#include "mysqlshdk/include/scripting/lang_base.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/scripting/types_cpp.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/oci/oci_bucket.h"
#include "mysqlshdk/libs/oci/oci_bucket_options.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_string.h"

extern "C" const char *g_oci_config_path;

namespace testing {

using mysqlsh::dump::Dump_manifest_options;

#define SKIP_IF_NO_OCI_CONFIGURATION \
  do {                               \
    if (should_skip()) {             \
      SKIP_TEST(skip_reason());      \
    }                                \
  } while (false)

class Oci_os_tests : public Shell_core_test_wrapper {
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

    {
      const auto oci_config_path =
          mysqlsh::current_shell_options()->get().oci_config_file;

      if (!shcore::path_exists(oci_config_path)) {
        skip("OCI Configuration does not exist: " + oci_config_path);
      }
    }

    const auto read_var = [this](const char *name, std::string *out,
                                 bool required = true) {
      const auto var = getenv(name);

      if (var) {
        *out = var;
      } else if (required) {
        skip("Missing environment variable: " + std::string(name));
      }
    };

    read_var("OS_NAMESPACE", &m_os_namespace, false);
    read_var("OCI_COMPARTMENT_ID", &m_oci_compartment_id);
    read_var("OS_BUCKET_NAME", &m_os_bucket_name);

    create_bucket();
  }

  void TearDown() override { delete_bucket(); }

 protected:
  std::vector<std::string> m_objects;
  std::string m_os_namespace;
  std::string m_os_bucket_name;
  std::string m_oci_compartment_id;

  void create_objects(mysqlshdk::oci::Oci_bucket &bucket) {
    for (const auto &name : m_objects) {
      bucket.put_object(name, "0", 1);
    }
  }

  // NOTE: this returns Dump_manifest_write_config, so it can be used in the
  // Dump_manifest tests, but can be upcast to Oci_bucket_config, and used to
  // initialize an OCI bucket
  std::shared_ptr<mysqlsh::dump::Dump_manifest_write_config> get_config(
      const std::string &bucket = {}) {
    const auto options =
        shcore::make_dict(Dump_manifest_options::bucket_name_option(),
                          bucket.empty() ? m_os_bucket_name : bucket,
                          Dump_manifest_options::par_manifest_option(), true);

    if (!m_os_namespace.empty()) {
      options->set(Dump_manifest_options::namespace_option(),
                   shcore::Value{m_os_namespace});
    }

    Dump_manifest_options parsed_options;
    Dump_manifest_options::options().unpack(options, &parsed_options);

    return std::make_shared<mysqlsh::dump::Dump_manifest_write_config>(
        parsed_options);
  }

  void clean_bucket(mysqlshdk::oci::Oci_bucket &bucket,
                    bool clean_uploads = true, bool clean_pars = true) {
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

    if (clean_pars) {
      auto pars = bucket.list_preauthenticated_requests();
      if (!pars.empty()) {
        for (const auto &par : pars) {
          bucket.delete_pre_authenticated_request(par.id);
        }

        pars = bucket.list_preauthenticated_requests();
        EXPECT_TRUE(pars.empty());
      }
    }
  }

  void skip(const std::string &reason) { m_skip_reasons.emplace_back(reason); }

  bool should_skip() const { return !m_skip_reasons.empty(); }

  std::string skip_reason() const {
    return shcore::str_join(m_skip_reasons, "\n");
  }

 private:
  void create_bucket() {
    if (!should_skip()) {
      mysqlshdk::oci::Oci_bucket bucket(get_config());

      if (bucket.exists()) {
        clean_bucket(bucket);
      } else {
        bucket.create(m_oci_compartment_id);
      }
    }
  }

  void delete_bucket() {
    if (!should_skip()) {
      mysqlshdk::oci::Oci_bucket bucket(get_config());

      clean_bucket(bucket);
      bucket.delete_();
    }
  }

  std::vector<std::string> m_skip_reasons;
};

}  // namespace testing

#endif  // UNITTEST_MYSQLSHDK_LIBS_OCI_OCI_TESTS_H_
