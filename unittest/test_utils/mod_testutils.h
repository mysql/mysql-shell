/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef UNITTEST_TEST_UTILS_MOD_TESTUTILS_H_
#define UNITTEST_TEST_UTILS_MOD_TESTUTILS_H_

#include <memory>
#include <string>
#include <vector>
#include "modules/adminapi/mod_dba_provisioning_interface.h"
#include "scripting/types_cpp.h"

namespace tests {

// C++ utility routines for use within scripted tests

class Testutils : public shcore::Cpp_object_bridge {
 private:
  virtual std::string class_name() const {
    return "Testutils";
  }

 public:
  Testutils(const std::string &sandbox_dir, bool dummy_mode);

  void set_sandbox_snapshot_dir(const std::string &dir);

 public:
  // Sandbox routines
  int deploy_sandbox(int port, const std::string &rootpass);
  int destroy_sandbox(int port);

  int start_sandbox(int port);
  int stop_sandbox(int port, const std::string &rootpass);
  int kill_sandbox(int port);

  int snapshot_sandbox_conf(int port);
  int begin_snapshot_sandbox_error_log(int port);
  int end_snapshot_sandbox_error_log(int port);

  int add_to_sandbox_conf(int port, const std::string &option);
  int remove_from_sandbox_conf(int port, const std::string &option);
  std::string get_sandbox_conf_path(int port);
  std::string get_sandbox_log_path(int port);

 public:
  // InnoDB cluster routines
  int wait_member_state(int query_port, const std::string &rootpass,
                        int member_port, const std::string &states);

 public:
  // Misc utility stuff
  int make_file_readonly(const std::string &path);

 public:
  // These should produce test failure output similar to that of gtest,
  // possibly including a stacktrace in the target language
  // set_current_test_case(::testing::Test);

 private:
  // Testing stuff
  bool is_replaying();

  // void expect_true(bool value);
  // void expect_false(bool value);
  // void expect_eqs(const std::string &expected, const std::string &actual);
  // void expect_eqi(int64_t expected, int64_t actual);

  // Sets the text to return next time an interactive prompt is shown.
  // if expected_prompt_text is not "", it will match the prompt text and fail
  // the test if it is different
  // void next_prompted_text(const std::string &text,
  //            const std::string &expected_prompt_text);

  // void next_prompted_password(const std::string &text,
  //            const std::string &expected_prompt_text);

 private:
  shcore::Interpreter_delegate _delegate;
  std::unique_ptr<mysqlsh::dba::ProvisioningInterface> _mp;
  std::string _sandbox_dir;
  std::string _sandbox_snapshot_dir;
  bool _debug = false;
  bool _dummy_sandboxes = false;
  int _snapshot_log_index = 0;

  void wait_sandbox_dead(int port);
};

}  // namespace tests

#endif  // UNITTEST_TEST_UTILS_MOD_TESTUTILS_H_
