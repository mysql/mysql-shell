/*
 * Copyright (c) 2017, 2025, Oracle and/or its affiliates.
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

#ifndef UNITTEST_TEST_UTILS_MOD_TESTUTILS_H_
#define UNITTEST_TEST_UTILS_MOD_TESTUTILS_H_

#include <functional>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "modules/adminapi/common/provisioning_interface.h"
#include "modules/mod_extensible_object.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/utils/process_launcher.h"
#include "src/mysqlsh/cmdline_shell.h"

namespace tests {

class Shell_test_env;

/**
 * \ingroup Testutils
 * Class with test utilities
 */
class Testutils : public mysqlsh::Extensible_object {
 private:
  std::string class_name() const override { return "Testutils"; }

 public:
#if DOXYGEN_JS
  Undefined deploySandbox(Integer port, String pwd, Dictionary options);
  Undefined destroySandbox(Integer port);
  Undefined startSandbox(Integer port, Dictionary options);
  Undefined stopSandbox(Integer port, Dictionary options);
  Undefined killSandbox(Integer port);
  Undefined restartSandbox(Integer port);
  Undefined waitSandboxAlive(Integer port);
  Undefined waitSandboxAlive(String uri);
  Undefined waitSandboxAlive(ConnectionData data);
  Undefined changeSandboxConf(Integer port, String option, String value,
                              String section);
  String getSandboxConf(Integer port, String option);
  Undefined upgradeSandbox(Integer port);
  Undefined removeFromSandboxConf(Integer port, String option, String section);
  String getSandboxConfPath(Integer port);
  String getSandboxLogPath(Integer port);
  String getSandboxPath(Integer port, String name);
  List readGeneralLog(Integer port, String startingTimestamp = "");
  String getShellLogPath();
  Undefined dumpData(String uri, String path, Array schemaList);
  Undefined importData(String uri, String path, String defaultSchema,
                       String importCharset);
  String waitMemberState(Integer port, String[] states);
  String waitReadReplicaState(Integer port, String[] states);
  Boolean waitMemberTransactions(Integer destPort, Integer sourcePort = 0);
  Undefined waitForDelayedGRStart(Integer port, String rootpass,
                                  Integer timeout = 60);
  Integer waitForReplConnectionError(
      Integer port, String channel = "group_replication_recovery");
  Undefined Testutils::waitReplicationChannelState(Integer port,
                                                   String channelName,
                                                   String[] states);
  Undefined expectPrompt(String prompt, String answer, Dictionary options);
  Undefined expectPassword(String prompt, String password, Dictionary options);
  Undefined assertNoPrompts();
  Integer makeFileReadonly(String path);
  List grepFile(String path, String pattern);
  Bool isTcpPortListening(String host, Integer port);
  Bool isReplaying();
  Undefined fail();
  Boolean versionCheck(String v1, String op, String v2);
  Undefined mkdir(String path, Boolean recursive);
  Undefined rmdir(String path, Boolean recursive);
  Undefined rename(String path, String newpath);
  Integer chmod(String path, Integer mode);
  Undefined cpfile(String source, String target);
  Undefined rmfile(String path);
  Undefined touch(String file);
  List catFile(String path);
  List wipeFileContents(String path);
  Undefined preprocessFile(String inPath, Array variables, String outPath);
  Undefined dbugSet(String s);
  Undefined dprint(String s);
  Undefined setenv(String var, String value);
  String sslCreateCA(String name, String issuer);
  String sslCreateCert(String name, String caname, String subj, Integer sbport);
  Undefined setTrap(String type, Array conditions, Dictionary options);
  None clearTraps(String type);
  None resetTraps(String type);
  Undefined wipeAllOutput();
  String getCurrentMetadataVersion();
  String getInstalledMetadataVersion();
  // Undefined slowify(Integer port, Boolean start);
  Undefined traceSyslog(String file);
  Undefined stopTracingSyslog();
  String yaml(Any value);
  Undefined serExecutionContext(String context);
#elif DOXYGEN_PY
  None deploy_sandbox(int port, str pwd, Dictionary options);
  None destroy_sandbox(int port);
  None start_sandbox(int port, Dictionary options);
  None stop_sandbox(int port, Dictionary options);
  None kill_sandbox(int port);
  None restart_sandbox(int port);
  None wait_sandbox_alive(int port);
  None wait_sandbox_alive(str uri);
  None wait_sandbox_alive(ConnectionData data);
  None change_sandbox_conf(int port, str option, str value, str section);
  str get_sandbox_conf(int port, str option);
  None upgrade_sandbox(int port);
  None remove_from_sandbox_conf(int port, str option, str section);
  str get_sandbox_conf_path(int port);
  str get_sandbox_log_path(int port);
  str get_sandbox_path(int port, str name);
  list read_general_log(int port, str startingTimestamp = "");
  str get_shell_log_path();
#ifndef ENABLE_SESSION_RECORDING
  list fetch_sql_log(bool flush);
#endif
  None dump_data(str uri, str path, list schemaList);
  None import_data(str uri, str path, str defaultSchema, str defaultCharset);
  str wait_member_state(int port, str[] states);
  str wait_read_replica_state(int port, str[] states);
  bool wait_member_transactions(int destPort, int sourcePort = 0);
  None wait_for_delayed_gr_start(int port, str rootpass, int timeout = 60);
  int wait_for_repl_connection_error(
      int port, str channel = "group_replication_recovery");
  None Testutils::wait_replication_channel_state(int port, str channel_name,
                                                 str[] states);
  None expect_prompt(str prompt, str answer, dict options);
  None expect_password(str prompt, str password, dict options);
  None assert_no_prompts();
  int make_file_readonly(str path);
  list grep_file(str path, str pattern);
  bool is_tcp_port_listening(str host, int port);
  bool is_replaying();
  None fail();
  bool version_check(str v1, str op, str v2);
  None mkdir(str path, bool recursive);
  None rmdir(str path, bool recursive);
  None rename(str path, str newpath);
  int chmod(str path, int mode);
  None cpfile(str source, str target);
  None rmfile(str path);
  None touch(str file);
  list cat_file(str path);
  list wipe_file_contents(str path);
  None preprocess_file(str inPath, list variables, str outPath);
  None dbug_set(str s);
  None dprint(str s);
  None skip(int port, bool start);
  None setenv(str var, str value);
  str ssl_create_ca(str name, str issuer);
  str ssl_create_cert(str name, str caname, str subj, int sbport);
  None set_trap(str type, list conditions, dict options);
  None clear_traps(str type);
  None reset_traps(str type);
  None wipe_all_output();
  str get_current_metadata_version();
  str get_installed_metadata_version();
  None trace_syslog(str file);
  None stop_tracing_syslog();
  str yaml(any value);
  None set_execution_context(str context);
#endif

  Testutils(const std::string &sandbox_dir, bool dummy_mode,
            std::shared_ptr<mysqlsh::Command_line_shell> shell = {},
            const std::string &mysqlsh_path = "");

  ~Testutils() override;

  void set_sandbox_snapshot_dir(const std::string &dir);

  static void validate_boilerplate(const std::string &sandbox_dir,
                                   bool delete_if_expired = true);
  static std::string get_mysqld_version(const std::string &mysqld_path);

  using Input_fn = std::function<void(const std::string &, const std::string &,
                                      const shcore::prompt::Prompt_options &)>;

  using Output_fn = std::function<std::string(bool)>;
  using Simple_callback = std::function<void()>;

  void set_test_callbacks(Input_fn feed_prompt, Input_fn feed_password,
                          Output_fn fetch_stdout, Output_fn fetch_stderr,
                          Simple_callback assert_no_prompts,
                          Simple_callback wipe_all_output) {
    _feed_prompt = feed_prompt;
    _feed_password = feed_password;
    _fetch_stdout = fetch_stdout;
    _fetch_stderr = fetch_stderr;
    _assert_no_prompts = assert_no_prompts;
    _wipe_all_output = wipe_all_output;
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

  void start_sandbox(int port, const shcore::Dictionary_t &opts = {});
  void stop_sandbox(int port, const shcore::Dictionary_t &opts = {});
  void kill_sandbox(int port, bool quiet = false);

  void restart_sandbox(int port);

  void stop_group(const shcore::Array_t &ports,
                  const std::string &root_pass = {});

  void wait_sandbox_alive(const shcore::Value &port_or_uri);
  void wait_sandbox_alive(
      const std::function<std::shared_ptr<mysqlshdk::db::ISession>()> &connect,
      const std::string &context);

  void snapshot_sandbox_conf(int port);
  void begin_snapshot_sandbox_error_log(int port);
  void end_snapshot_sandbox_error_log(int port);

  void change_sandbox_conf(int port, const std::string &option,
                           const std::string &value,
                           const std::string &section = "");
  std::string get_sandbox_conf(int port, const std::string &option);

  void upgrade_sandbox(int port);
  void remove_from_sandbox_conf(int port, const std::string &option,
                                const std::string &section = "");
  std::string get_sandbox_conf_path(int port);
  std::string get_sandbox_log_path(int port);
  std::string get_sandbox_path(int port = 0, const std::string &file = "");

  void dump_data(const std::string &uri, const std::string &path,
                 const std::vector<std::string> &schemas,
                 const shcore::Dictionary_t &options);
  void import_data(const std::string &uri, const std::string &path,
                   const std::string &schema = "",
                   const std::string &default_charset = "");

  bool is_tcp_port_listening(const std::string &host, int port);

  std::string get_current_metadata_version_string();
  std::string get_installed_metadata_version_string();

  shcore::Array_t read_general_log(int port,
                                   const std::string &starting_timestamp = "");

 public:
  // InnoDB cluster routines
  void wait_for_delayed_gr_start(int port, const std::string &root_pass,
                                 int timeout = 100);

  void wait_replication_channel_state(int port, const std::string &channel_name,
                                      const std::string &states);

  int wait_for_repl_connection_error(int port, const std::string &channel);

  int wait_for_repl_applier_error(int port, const std::string &channel);

  std::string wait_member_state(int member_port, const std::string &states,
                                bool direct_connection);

  std::string wait_read_replica_state(int rr_port, const std::string &states);

  bool wait_member_transactions(int dest_port, int source_port);

  void inject_gtid_set(int dest_port, const std::string &gtid_set);

 public:
  // Misc utility stuff
  int make_file_readonly(const std::string &path);
  void mk_dir(const std::string &path, bool recursive = false);
  void rm_dir(const std::string &path, bool recursive = false);
  void rename(const std::string &path, const std::string &newpath);
  int ch_mod(const std::string &path, int mode);
  void cp_file(const std::string &source, const std::string &target);
  void rm_file(const std::string &target);
  void touch(const std::string &file);

  std::string get_shell_log_path();
#ifndef ENABLE_SESSION_RECORDING
  shcore::Array_t fetch_sql_log(bool flush);
#endif

  shcore::Array_t grep_file(const std::string &path,
                            const std::string &pattern);
  std::string cat_file(const std::string &path);

  std::string get_executable_path(const std::string &exec_name);

  void wipe_file_contents(const std::string &path);

  void preprocess_file(const std::string &in_path,
                       const std::vector<std::string> &vars,
                       const std::string &out_path,
                       const std::vector<std::string> &skip_sections = {});

  void dprint(const std::string &s);

  void set_trap(const std::string &type, const shcore::Array_t &conditions,
                const shcore::Dictionary_t &options);
  void reset_traps(const std::string &type);
  void clear_traps(const std::string &type);

  void bp(bool flag);

  std::string ssl_create_ca(const std::string &name, const std::string &issuer);
  std::string ssl_create_cert(const std::string &name,
                              const std::string &caname,
                              const std::string &subj, int sbport);

  void get_exclusive_lock(const shcore::Value &classic_session,
                          const std::string name_space, const std::string name,
                          unsigned int timeout = 0);
  void get_shared_lock(const shcore::Value &classic_session,
                       const std::string name_space, const std::string name,
                       unsigned int timeout = 0);
  void release_locks(const shcore::Value &classic_session,
                     const std::string name_space);

 public:
  // These should produce test failure output similar to that of gtest,
  // possibly including a stacktrace in the target language
  // set_current_test_case(::testing::Test);
  int call_mysqlsh(const shcore::Array_t &args,
                   const std::string &std_input = std::string{},
                   const shcore::Array_t &env = nullptr,
                   const std::string &executable_path = "");
  int call_mysqlsh_c(const std::vector<std::string> &args,
                     const std::string &std_input = "",
                     const std::vector<std::string> &env = {},
                     const std::string &executable_path = "");

  int call_mysqlsh_async(const std::vector<std::string> &args,
                         const std::string &std_input = {},
                         const std::vector<std::string> &env = {},
                         const std::string &executable_path = {});
  int wait_mysqlsh_async(int id, int seconds = 60);

  // Sets the text to return next time an interactive prompt is shown.
  // if expected_prompt_text is not "", it will match the prompt text and fail
  // the test if it is different
  void expect_prompt(const std::string &prompt, const std::string &text,
                     const shcore::prompt::Prompt_options &options = {});
  void expect_password(const std::string &prompt, const std::string &text,
                       const shcore::prompt::Prompt_options &options = {});
  void assert_no_prompts();
  void wipe_all_output();

  bool version_check(const std::string &v1, const std::string &op,
                     const std::string &v2);

  /**
   * This function is used as a demonstration on how to use extensible objects
   * which allow:
   *
   * - Help data registration into the help system for objects and members.
   * - Creation of extensible objects from C++.
   *
   * This code has been let as a regression test for the extensible object
   * class (including it's test at extensible_objects_norecord.py/js)
   */
  void enable_extensible();

  void trace_syslog(const std::string &file);

  void stop_tracing_syslog();

  std::string yaml(const shcore::Value &v) const;

  void set_execution_context(const std::string &context);

  int test_script_timeout() const;

  void set_test_script_timeout(int timeout) const;

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

  class Async_mysqlsh_run {
   public:
    Async_mysqlsh_run(const std::vector<std::string> &cmdline_,
                      const std::string &stdin_,
                      const std::vector<std::string> &env_,
                      const std::string &mysqlsh_path,
                      const std::string &executable_path);

    int wait(int seconds);

    const std::string &cmdline() const noexcept { return m_process_cmdline; }

    const std::string &output() const noexcept { return m_output; }

   private:
    std::unique_ptr<shcore::Process_launcher> m_process;
    std::string m_process_cmdline;
    std::string m_output;
  };

  std::vector<std::unique_ptr<Async_mysqlsh_run>> m_shell_runs;

  std::weak_ptr<mysqlsh::Command_line_shell> _shell;
  std::string _mysqlsh_path;
  mysqlsh::dba::ProvisioningInterface _mp;
  std::map<int, std::string> _passwords;
  std::map<int, std::string> _general_log_files;
  std::map<int, std::unique_ptr<Slower_thread>> _slower_threads;
  std::string _sandbox_dir;
  std::string _sandbox_snapshot_dir;
  bool _skip_server_interaction = false;
  bool _use_boilerplate = false;
  std::string _test_skipped;
  int _snapshot_log_index = 0;
  int _snapshot_conf_serial = 0;
  int _tcp_port_check_serial = 0;
  Input_fn _feed_prompt;
  Input_fn _feed_password;
  Output_fn _fetch_stdout;
  Output_fn _fetch_stderr;
  Simple_callback _assert_no_prompts;
  Simple_callback _wipe_all_output;
  std::string _test_file;
  int _test_line = 0;
  Shell_test_env *_test_env = nullptr;

  std::ofstream m_syslog_trace;

  void wait_sandbox_dead(int port);

  bool is_port_available_for_sandbox_to_bind(int port) const;

  void wait_until_file_lock_released(const std::string &filepath,
                                     int timeout) const;

  void handle_sandbox_encryption(const std::string &path) const;

  void prepare_sandbox_boilerplate(int port, const std::string &mysqld_path);
  void deploy_sandbox_from_boilerplate(int port,
                                       const shcore::Dictionary_t &opts,
                                       bool raw, const std::string &mysqld_path,
                                       int timeout = -1);
  void change_sandbox_uuid(int port, const std::string &server_uuid);
  std::string get_sandbox_datadir(int port);
  void try_rename(const std::string &source, const std::string &target);
  void make_empty_file(const std::string &path);
  void create_file(const std::string &path, const std::string &content);

  std::shared_ptr<mysqlshdk::db::ISession> connect_to_sandbox(
      int port, const std::optional<std::string> &rootpass = {});
  std::string get_user_config_path();

  bool validate_oci_config();
  shcore::Dictionary_t get_oci_config();
  void upload_oci_object(const std::string &bucket, const std::string &name,
                         const std::string &path);
  void download_oci_object(const std::string &ns, const std::string &bucket,
                           const std::string &name, const std::string &path);
  void create_oci_object(const std::string &bucket, const std::string &name,
                         const std::string &content);
  void delete_oci_object(const std::string &bucket, const std::string &name);

  void anycopy(const shcore::Value &from, const shcore::Value &to);

  bool clean_s3_bucket(const shcore::Dictionary_t &opts);

  void delete_s3_object(const std::string &name,
                        const shcore::Dictionary_t &opts);

  void delete_s3_objects(const std::vector<std::string> &names,
                         const shcore::Dictionary_t &opts);

  bool clean_azure_container(const shcore::Dictionary_t &opts);
};

}  // namespace tests

#endif  // UNITTEST_TEST_UTILS_MOD_TESTUTILS_H_
