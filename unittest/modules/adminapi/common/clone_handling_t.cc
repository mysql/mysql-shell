/*
 * Copyright (c) 2020, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <string>

#include "unittest/gprod_clean.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

#include "modules/adminapi/common/base_cluster_impl.h"
#include "modules/adminapi/common/clone_progress.h"
#include "modules/adminapi/common/common.h"
#include "mysqlshdk/libs/mysql/clone.h"

namespace mysqlsh {
namespace dba {

class Admin_api_clone_test : public Shell_core_test_wrapper {
 public:
  static std::shared_ptr<mysqlshdk::db::ISession> create_session(
      int port, std::string user = "root") {
    auto session = mysqlshdk::db::mysql::Session::create();

    auto connection_options = shcore::get_connection_options(
        user + ":root@localhost:" + std::to_string(port), false);
    session->connect(connection_options);

    return session;
  }
};

// It's not guaranteed that P_S will have the information for the 4 stages
// we want to monitor. If the dataset is very small it's possible that
// only 1 or 2 stages are displayed in P_S.
// Test that the function does not segfault in that scenario (BUG#31545728)
TEST_F(Admin_api_clone_test, update_transfer) {
  reset_shell();
  Recovery_progress_style wait_recovery = Recovery_progress_style::PROGRESSBAR;
  Clone_progress clone_progress(wait_recovery);

  mysqlshdk::mysql::Clone_status status;

  // Add some data to the status
  status.state = "STATE_TEST";
  status.begin_time = "";
  status.end_time = "";
  status.seconds_elapsed = 0.0;
  status.source = "SOURCE_TEST";
  status.error_n = 0;
  status.error = "ERROR_MSG_TEST";

  for (int i = 0; i < 5; i++) {
    mysqlshdk::mysql::Clone_status::Stage_info stage;
    stage.stage = "STAGE_TEST" + std::to_string(i);
    stage.state = "STATE_TEST" + std::to_string(i);
    stage.seconds_elapsed = 0;
    stage.work_estimated = 0;
    stage.work_completed = 0;

    status.stages.push_back(stage);

    ASSERT_NO_FATAL_FAILURE(clone_progress.update_transfer(status));
  }
}

TEST_F(Admin_api_clone_test, check_clone_version_compatibility) {
  using namespace mysqlshdk::utils;

  // anything below 8.0.16 can't be used
  EXPECT_FALSE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(8, 0, 16), Version(8, 0, 17)));
  EXPECT_FALSE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(8, 0, 17), Version(8, 0, 16)));
  EXPECT_FALSE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(8, 0, 16), Version(8, 0, 16)));
  EXPECT_TRUE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(8, 0, 17), Version(8, 0, 17)));

  EXPECT_TRUE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(9, 1, 4), Version(9, 1, 4)));
  EXPECT_TRUE(Base_cluster_impl::verify_compatible_clone_versions(
      Version("8.0.28-debug"), Version("8.0.28-debug")));

  EXPECT_FALSE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(10, 7, 3), Version(9, 7, 4)));
  EXPECT_FALSE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(9, 7, 4), Version(9, 6, 4)));
  EXPECT_FALSE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(9, 6, 4), Version(10, 7, 3)));

  EXPECT_TRUE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(9, 6, 4), Version("9.6.53-release")));
  EXPECT_TRUE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(10, 7, 36), Version(10, 7, 16)));
  EXPECT_TRUE(Base_cluster_impl::verify_compatible_clone_versions(
      Version("8.4.1-release"), Version(8, 4, 0)));
  EXPECT_TRUE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(9, 0, 0), Version(9, 0, 2)));

  EXPECT_TRUE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(8, 0, 31), Version(8, 0, 31)));
  EXPECT_TRUE(Base_cluster_impl::verify_compatible_clone_versions(
      Version("8.0.35-release"), Version(8, 0, 35)));
  EXPECT_TRUE(Base_cluster_impl::verify_compatible_clone_versions(
      Version("8.0.37-release"), Version(8, 0, 37)));

  EXPECT_FALSE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(8, 0, 33), Version(8, 0, 35)));
  EXPECT_FALSE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(8, 0, 35), Version(8, 0, 30)));
  EXPECT_FALSE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(8, 0, 35), Version(8, 0, 37)));
  EXPECT_FALSE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(8, 0, 37), Version(8, 0, 34)));
  EXPECT_FALSE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(8, 0, 0), Version(8, 0, 37)));
  EXPECT_FALSE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(8, 0, 0), Version(8, 0, 34)));

  EXPECT_TRUE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(8, 0, 37), Version(8, 0, 38)));
  EXPECT_TRUE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(8, 0, 38), Version(8, 0, 37)));

  EXPECT_FALSE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(8, 3, 1), Version(8, 3, 0)));
  EXPECT_TRUE(Base_cluster_impl::verify_compatible_clone_versions(
      Version(8, 4, 1), Version(8, 4, 1)));
  EXPECT_FALSE(Base_cluster_impl::verify_compatible_clone_versions(
      Version("8.0.37-release"), Version("8.0.36-release")));
}

}  // namespace dba
}  // namespace mysqlsh
