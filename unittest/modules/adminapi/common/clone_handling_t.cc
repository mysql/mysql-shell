/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#include <string>
#include "unittest/gprod_clean.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

#include "modules/adminapi/common/clone_progress.h"
#include "modules/adminapi/common/common.h"
#include "mysqlshdk/libs/mysql/clone.h"

namespace mysqlsh {
namespace dba {

class Clone_progress_test : public Shell_core_test_wrapper {};

// It's not guaranteed that P_S will have the information for the 4 stages
// we want to monitor. If the dataset is very small it's possible that
// only 1 or 2 stages are displayed in P_S.
// Test that the function does not segfault in that scenario (BUG#31545728)
TEST_F(Clone_progress_test, update_transfer) {
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

}  // namespace dba
}  // namespace mysqlsh
