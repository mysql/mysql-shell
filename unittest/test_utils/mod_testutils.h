/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef UNITTEST_TEST_UTILS_MOD_TESTUTILS_H_
#define UNITTEST_TEST_UTILS_MOD_TESTUTILS_H_

#include <memory>
#include <string>
#include <vector>
#include "modules/adminapi/mod_dba_provisioning_interface.h"
#include "scripting/types_cpp.h"
#include "src/mysqlsh/mysql_shell.h"
#include "mysqlshdk/libs/db/connection_options.h"

namespace tests {

/**
 * \ingroup Testutils
 * Class with test utilities
*/
class Testutils : public shcore::Cpp_object_bridge {
 private:
  virtual std::string class_name() const {
    return "Testutils";
  }

 public:
#if DOXYGEN_JS
  Undefined deploySandbox(Integer port, String pwd, Dictionary options);
  Undefined destroySandbox(Integer port);
  Undefined startSandbox(Integer port);
  Undefined stopSandbox(Integer port);
  Undefined killSandbox(Integer port);
  Undefined restartSandbox(Integer port);
  Undefined changeSandboxConf(Integer port, String option, String value,
                              String section);
  Undefined removeFromSandboxConf(Integer port, String option, String section);
  String getSandboxConfPath(Integer port);
  String getSandboxLogPath(Integer port);
  String getSandboxPath(Integer port, String name);
  String getShellLogPath();
  Undefined dumpData(String uri, String path, Array schemaList);
  Undefined importData(String uri, String path, String defaultSchema);
  String waitMemberState(Integer port, String[] states);
  Undefined waitForDelayedGRStart(Integer port, String rootpass,
                                  Integer timeout = 60);
  Undefined expectPrompt(String prompt, String answer);
  Undefined expectPassword(String prompt, String password);
  Integer makeFileReadonly(String path);
  List grepFile(String path, String pattern);
  Bool isReplying();
  Undefined fail();
  Boolean versionCheck(String v1, String op, String v2);
  Undefined mkdir(String path, Boolean recursive);
  Undefined rmdir(String path, Boolean recursive);
  Integer chmod(String path, Integer mode);
  Undefined cpfile(String source, String target);
  Undefined rmfile(String path);
  List catFile(String path);
  List wipeFileContents(String path);
#elif DOXYGEN_PY
  None deploy_sandbox(int port, str pwd, Dictionary options);
  None destroy_sandbox(int port);
  None start_sandbox(int port);
  None stop_sandbox(int port);
  None kill_sandbox(int port);
  None restart_sandbox(int port);
  None change_sandbox_conf(int port, str option, str value, str section);
  None remove_from_sandbox_conf(int port, str option, str section);
  str get_sandbox_conf_path(int port);
  str get_sandbox_log_path(int port);
  str get_sandbox_path(int port, str name);
  str get_shell_log_path();
  None dump_data(str uri, str path, list schemaList);
  None import_data(str uri, str path, str defaultSchema);
  str wait_member_state(int port, str[] states);
  None wait_for_delayed_gr_start(int port, str rootpass,
                                 int timeout = 60);
  None expect_prompt(str prompt, str answer);
  None expect_password(str prompt, str password);
  int make_file_readonly(str path);
  list grep_file(str path, str pattern);
  bool is_replying();
  None fail();
  bool version_check(str v1, str op, str v2);
  None mkdir(str path, bool recursive);
  None rmdir(str path, bool recursive);
  int chmod(str path, int mode);
  None cpfile(str source, str target);
  None rmfile(str path);
  list cat_file(str path);
  list wipe_file_contents(str path);
#endif

  Testutils(const std::string &sandbox_dir, bool dummy_mode,
            std::shared_ptr<mysqlsh::Mysql_shell> shell = {},
            const std::string &mysqlsh_path = "");

  void set_sandbox_snapshot_dir(const std::string &dir);

  static void validate_boilerplate(const std::string &sandbox_dir,
                                   const std::string &version);

  using Input_fn =
      std::function<void(const std::string &, const std::string &)>;

  using Output_fn = std::function<std::string(bool)>;

  void set_test_callbacks(Input_fn feed_prompt, Input_fn feed_password,
                          Output_fn fetch_stdout, Output_fn fetch_stderr) {
    _feed_prompt = feed_prompt;
    _feed_password = feed_password;
    _fetch_stdout = fetch_stdout;
    _fetch_stderr = fetch_stderr;
  }

  void set_test_execution_context(const std::string &file, int line);

  static mysqlshdk::db::Connection_options sandbox_connection_options(
      int port, const std::string &password);

  bool test_skipped() const {
    return !_test_skipped.empty();
  }

  const std::string &test_skip_reason() const {
    return _test_skipped;
  }

 public:
  // Sandbox routines
  void deploy_sandbox(int port, const std::string &rootpass,
                      const shcore::Dictionary_t &opts = {});
  void deploy_raw_sandbox(int port, const std::string &rootpass,
                      const shcore::Dictionary_t &opts = {});
  void destroy_sandbox(int port, bool quiet_kill = false);

  void start_sandbox(int port);
  void stop_sandbox(int port, const std::string &pass = "");
  void kill_sandbox(int port, bool quiet = false);

  void restart_sandbox(int port);

  void snapshot_sandbox_conf(int port);
  void begin_snapshot_sandbox_error_log(int port);
  void end_snapshot_sandbox_error_log(int port);

  void change_sandbox_conf(int port, const std::string &option,
                           const std::string& value,
                           const std::string& section = "");
  void remove_from_sandbox_conf(int port, const std::string &option,
                                const std::string &section = "");
  std::string get_sandbox_conf_path(int port);
  std::string get_sandbox_log_path(int port);
  std::string get_sandbox_path(int port = 0, const std::string& file = "");

  void dump_data(const std::string &uri, const std::string &path,
                 const std::vector<std::string> &schemas);
  void import_data(const std::string &uri, const std::string &path,
                   const std::string &schema = "");
  void wait_for_delayed_gr_start(int port, const std::string &root_pass,
                                 int timeout = 100);
 public:
  // InnoDB cluster routines
  std::string wait_member_state(int member_port, const std::string &states,
                                bool direct_connection);

 public:
  // Misc utility stuff
  int make_file_readonly(const std::string &path);
  void mk_dir(const std::string& path, bool recursive = false);
  void rm_dir(const std::string& path, bool recursive = false);
  int ch_mod(const std::string& path, int mode);
  void cp_file(const std::string &source, const std::string& target);
  void rm_file(const std::string& target);

  std::string get_shell_log_path();

  shcore::Array_t grep_file(const std::string &path,
                            const std::string &pattern);
  std::string cat_file(const std::string &path);

  void wipe_file_contents(const std::string &path);

 public:
  // These should produce test failure output similar to that of gtest,
  // possibly including a stacktrace in the target language
  // set_current_test_case(::testing::Test);
  int call_mysqlsh(const shcore::Array_t &args);

  // Sets the text to return next time an interactive prompt is shown.
  // if expected_prompt_text is not "", it will match the prompt text and fail
  // the test if it is different
  void expect_prompt(const std::string &prompt, const std::string &text);
  void expect_password(const std::string &prompt, const std::string &text);

  bool version_check(const std::string &v1,
                     const std::string &op,
                     const std::string &v2);

 private:
  // Testing stuff
  bool is_replaying();

  void skip(const std::string &reason);
  void fail(const std::string &context);

  std::string fetch_captured_stdout(bool eat_one);
  std::string fetch_captured_stderr(bool eat_one);

 private:
  std::weak_ptr<mysqlsh::Mysql_shell> _shell;
  std::string _mysqlsh_path;
  shcore::Interpreter_delegate _delegate;
  std::unique_ptr<mysqlsh::dba::ProvisioningInterface> _mp;
  std::map<int, std::string> _passwords;
  std::string _sandbox_dir;
  std::string _sandbox_snapshot_dir;
  bool _dummy_sandboxes = false;
  bool _use_boilerplate = false;
  std::string _test_skipped;
  int _snapshot_log_index = 0;
  int _snapshot_conf_serial = 0;
  Input_fn _feed_prompt;
  Input_fn _feed_password;
  Output_fn _fetch_stdout;
  Output_fn _fetch_stderr;
  std::string _test_file;
  int _test_line = 0;

  void wait_sandbox_dead(int port);

  void prepare_sandbox_boilerplate(const std::string &rootpass, int port);
  bool deploy_sandbox_from_boilerplate(int port,
                                       const shcore::Dictionary_t &opts,
                                       bool raw = false);
  void change_sandbox_uuid(int port, const std::string &server_uuid);
  std::string get_sandbox_datadir(int port);
  void try_rename(const std::string& source, const std::string& target);
};

}  // namespace tests

#endif  // UNITTEST_TEST_UTILS_MOD_TESTUTILS_H_
