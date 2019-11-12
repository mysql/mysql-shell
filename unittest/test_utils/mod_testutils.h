/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "modules/adminapi/common/provisioning_interface.h"
#include "modules/mod_extensible_object.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "scripting/types_cpp.h"
#include "src/mysqlsh/cmdline_shell.h"

namespace tests {

class Shell_test_env;

/**
 * \ingroup Testutils
 * Class with test utilities
 */
class Testutils : public mysqlsh::Extensible_object {
 private:
  virtual std::string class_name() const { return "Testutils"; }

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
  Boolean waitMemberTransactions(Integer destPort, Integer sourcePort = 0);
  Undefined waitForDelayedGRStart(Integer port, String rootpass,
                                  Integer timeout = 60);
  Undefined waitForConnectionErrorInRecovery(Integer port, Integer errorNumber,
                                             Integer timeout = 60);
  Undefined expectPrompt(String prompt, String answer);
  Undefined expectPassword(String prompt, String password);
  Undefined assertNoPrompts();
  Integer makeFileReadonly(String path);
  List grepFile(String path, String pattern);
  Bool isTcpPortListening(String host, Integer port);
  Bool isReplaying();
  Undefined fail();
  Boolean versionCheck(String v1, String op, String v2);
  Undefined mkdir(String path, Boolean recursive);
  Undefined rmdir(String path, Boolean recursive);
  Integer chmod(String path, Integer mode);
  Undefined cpfile(String source, String target);
  Undefined rmfile(String path);
  Undefined touch(String file);
  List catFile(String path);
  List wipeFileContents(String path);
  Undefined dbugSet(String s);
  Undefined dprint(String s);
  Undefined setenv(String var, String value);
  // Undefined slowify(Integer port, Boolean start);
#elif DOXYGEN_PY
  None deploy_sandbox(int port, str pwd, Dictionary options);
  None destroy_sandbox(int port);
  None start_sandbox(int port);
  None stop_sandbox(int port, Dictionary options);
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
  bool wait_member_transactions(int destPort, int sourcePort = 0);
  None wait_for_delayed_gr_start(int port, str rootpass, int timeout = 60);
  None wait_for_connection_error_in_recovery(int port, int errorNumber,
                                             int timeout = 60);
  None expect_prompt(str prompt, str answer);
  None expect_password(str prompt, str password);
  None assert_no_prompts();
  int make_file_readonly(str path);
  list grep_file(str path, str pattern);
  bool is_tcp_port_listening(str host, int port);
  bool is_replaying();
  None fail();
  bool version_check(str v1, str op, str v2);
  None mkdir(str path, bool recursive);
  None rmdir(str path, bool recursive);
  int chmod(str path, int mode);
  None cpfile(str source, str target);
  None rmfile(str path);
  None touch(str file);
  list cat_file(str path);
  list wipe_file_contents(str path);
  None dbug_set(str s);
  None dprint(str s);
  None skip(int port, bool start);
  None setenv(str var, str value);
#endif

  Testutils(const std::string &sandbox_dir, bool dummy_mode,
            std::shared_ptr<mysqlsh::Command_line_shell> shell = {},
            const std::string &mysqlsh_path = "");

  void set_sandbox_snapshot_dir(const std::string &dir);

  static void validate_boilerplate(const std::string &sandbox_dir,
                                   bool delete_if_expired = true);
  static std::string get_mysqld_version(const std::string &mysqld_path);

  using Input_fn =
      std::function<void(const std::string &, const std::string &)>;

  using Output_fn = std::function<std::string(bool)>;
  using Assert_no_prompts_fn = std::function<void()>;

  void set_test_callbacks(Input_fn feed_prompt, Input_fn feed_password,
                          Output_fn fetch_stdout, Output_fn fetch_stderr,
                          Assert_no_prompts_fn assert_no_prompts) {
    _feed_prompt = feed_prompt;
    _feed_password = feed_password;
    _fetch_stdout = fetch_stdout;
    _fetch_stderr = fetch_stderr;
    _assert_no_prompts = assert_no_prompts;
  }

  void set_test_execution_context(const std::string &file, int line,
                                  Shell_test_env *env);

  static mysqlshdk::db::Connection_options sandbox_connection_options(
      int port, const std::string &password);

  bool test_skipped() const { return !_test_skipped.empty(); }

  const std::string &test_skip_reason() const { return _test_skipped; }

  void dbug_set(const std::string &s);

 public:
  // Sandbox routines
  void deploy_sandbox(int port, const std::string &rootpass,
                      const shcore::Dictionary_t &my_cnf_opts = {},
                      const shcore::Dictionary_t &opts = {});
  void deploy_raw_sandbox(int port, const std::string &rootpass,
                          const shcore::Dictionary_t &my_cnf_opts = {},
                          const shcore::Dictionary_t &opts = {});
  void destroy_sandbox(int port, bool quiet_kill = false);

  void start_sandbox(int port);
  void stop_sandbox(int port, const shcore::Dictionary_t &opts = {});
  void kill_sandbox(int port, bool quiet = false);

  void restart_sandbox(int port);

  void wait_sandbox_alive(int port);

  void snapshot_sandbox_conf(int port);
  void begin_snapshot_sandbox_error_log(int port);
  void end_snapshot_sandbox_error_log(int port);

  void change_sandbox_conf(int port, const std::string &option,
                           const std::string &value,
                           const std::string &section = "");
  void remove_from_sandbox_conf(int port, const std::string &option,
                                const std::string &section = "");
  std::string get_sandbox_conf_path(int port);
  std::string get_sandbox_log_path(int port);
  std::string get_sandbox_path(int port = 0, const std::string &file = "");

  void dump_data(const std::string &uri, const std::string &path,
                 const std::vector<std::string> &schemas);
  void import_data(const std::string &uri, const std::string &path,
                   const std::string &schema = "");

  bool is_tcp_port_listening(const std::string &host, int port);

 public:
  // InnoDB cluster routines
  void wait_for_delayed_gr_start(int port, const std::string &root_pass,
                                 int timeout = 100);

  void wait_for_connection_error_in_recovery(int port, int error_number,
                                             int timeout = 60);

  std::string wait_member_state(int member_port, const std::string &states,
                                bool direct_connection);

  bool wait_member_transactions(int dest_port, int source_port);

 public:
  // Misc utility stuff
  int make_file_readonly(const std::string &path);
  void mk_dir(const std::string &path, bool recursive = false);
  void rm_dir(const std::string &path, bool recursive = false);
  int ch_mod(const std::string &path, int mode);
  void cp_file(const std::string &source, const std::string &target);
  void rm_file(const std::string &target);
  void touch(const std::string &file);

  std::string get_shell_log_path();

  shcore::Array_t grep_file(const std::string &path,
                            const std::string &pattern);
  std::string cat_file(const std::string &path);

  void wipe_file_contents(const std::string &path);

  void dprint(const std::string &s);

  // void slowify(int port, bool flag);

 public:
  // These should produce test failure output similar to that of gtest,
  // possibly including a stacktrace in the target language
  // set_current_test_case(::testing::Test);
  int call_mysqlsh(const shcore::Array_t &args,
                   const std::string &std_input = std::string{},
                   const shcore::Array_t &env = nullptr);
  int call_mysqlsh_c(const std::vector<std::string> &args,
                     const std::string &std_input = "",
                     const std::vector<std::string> &env = {});

  // Sets the text to return next time an interactive prompt is shown.
  // if expected_prompt_text is not "", it will match the prompt text and fail
  // the test if it is different
  void expect_prompt(const std::string &prompt, const std::string &text);
  void expect_password(const std::string &prompt, const std::string &text);
  void assert_no_prompts();

  bool version_check(const std::string &v1, const std::string &op,
                     const std::string &v2);

  /**
   * This function is used as a demostration on how to use extensible objects
   * which allow:
   *
   * - Help data registration into the help system for objects and members.
   * - Creation of extendible objects from C++.
   *
   * This code has been let as a regression test for the extensible object
   * class (including it's test at extendible_objects_norecord.py/js)
   */
  void enable_extensible();

 private:
  // Testing stuff
  bool is_replaying();

  void skip(const std::string &reason);
  void fail(const std::string &context);

  std::string fetch_captured_stdout(bool eat_one);
  std::string fetch_captured_stderr(bool eat_one);

  void setenv(const std::string &var, const std::string &value);

 private:
  struct Slower_thread {
    std::thread thread;
    bool stop = false;
  };

  std::weak_ptr<mysqlsh::Command_line_shell> _shell;
  std::string _mysqlsh_path;
  std::unique_ptr<mysqlsh::dba::ProvisioningInterface> _mp;
  std::map<int, std::string> _passwords;
  std::map<int, std::unique_ptr<Slower_thread>> _slower_threads;
  std::string _sandbox_dir;
  std::string _sandbox_snapshot_dir;
  bool _dummy_sandboxes = false;
  bool _use_boilerplate = false;
  std::string _test_skipped;
  int _snapshot_log_index = 0;
  int _snapshot_conf_serial = 0;
  int _tcp_port_check_serial = 0;
  Input_fn _feed_prompt;
  Input_fn _feed_password;
  Output_fn _fetch_stdout;
  Output_fn _fetch_stderr;
  Assert_no_prompts_fn _assert_no_prompts;
  std::string _test_file;
  int _test_line = 0;
  Shell_test_env *_test_env = nullptr;

  void wait_sandbox_dead(int port);

  bool is_port_available_for_sandbox_to_bind(int port) const;

  void handle_remote_root_user(const std::string &rootpass, int port,
                               bool create_remote_root = true) const;
  void prepare_sandbox_boilerplate(const std::string &rootpass, int port,
                                   const std::string &mysqld_path);
  bool deploy_sandbox_from_boilerplate(int port,
                                       const shcore::Dictionary_t &opts,
                                       bool raw,
                                       const std::string &mysqld_path);
  void change_sandbox_uuid(int port, const std::string &server_uuid);
  std::string get_sandbox_datadir(int port);
  void try_rename(const std::string &source, const std::string &target);
  void make_empty_file(const std::string &path);
  void create_file(const std::string &path, const std::string &content);

  std::shared_ptr<mysqlshdk::db::ISession> connect_to_sandbox(int port);
  std::string get_user_config_path();
};

}  // namespace tests

#endif  // UNITTEST_TEST_UTILS_MOD_TESTUTILS_H_
