/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_CLONE_PROGRESS_H_
#define MODULES_ADMINAPI_COMMON_CLONE_PROGRESS_H_

#include "modules/adminapi/common/common.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/textui/progress.h"
#include "mysqlshdk/libs/textui/textui.h"

namespace mysqlsh {
namespace dba {

class Clone_progress {
 public:
  explicit Clone_progress(Recovery_progress_style style);
  ~Clone_progress();

  void on_restart();
  void on_error();
  void update(const mysqlshdk::mysql::Clone_status &status);

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(Admin_api_clone_test, update_transfer);
#endif

  void update_stage(const mysqlshdk::mysql::Clone_status &status,
                    int first_stage, int last_stage);
  void update_transfer(const mysqlshdk::mysql::Clone_status &status);

 private:
  std::unique_ptr<mysqlshdk::textui::Progress_vt100> m_progress;
  std::unique_ptr<mysqlshdk::textui::Spinny_stick> m_spinner;
  // Clone stages:
  //
  // - 0 None
  // - 1 DROP DATA
  // - 2 FILE COPY
  // - 3 PAGE COPY
  // - 4 REDO COPY
  // - 5 FILE SYNC
  // - 6 RESTART
  // - 7 RECOVERY
  int m_current_stage;
  Recovery_progress_style m_style;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_CLONE_PROGRESS_H_
