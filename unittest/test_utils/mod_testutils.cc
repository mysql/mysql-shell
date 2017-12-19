/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "unittest/test_utils/mod_testutils.h"

#include <cassert>
#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <system_error>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/process_launcher.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_process.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/gtest_clean.h"
#include "mysqlshdk/libs/utils/version.h"

#include "modules/adminapi/mod_dba.h"
#include "modules/adminapi/mod_dba_cluster.h"

// TODO(anyone)
// - make deploySandbox() reuse sandboxes
// - make destroySandbox() expect that the final state of the sandbox is the
//   same as in the beginning
// - make a deployCluster() with reusable cluster
// - make a destroyCluster() that expects the cluster is in the same state
//   as original

extern int g_test_trace_scripts;
extern bool g_test_color_output;

namespace tests {

constexpr int k_wait_member_timeout = 60;
constexpr int k_max_start_sandbox_retries = 5;

static void print(void *, const char *s) {
  std::cout << s << "\n";
}

Testutils::Testutils(const std::string &sandbox_dir, bool dummy_mode,
                     const std::vector<int> &default_sandbox_ports,
                     std::shared_ptr<mysqlsh::Mysql_shell> shell,
                     const std::string &mysqlsh_path)
    : _shell(shell),
      _mysqlsh_path(mysqlsh_path),
      _default_sandbox_ports(default_sandbox_ports) {
  _use_boilerplate = true;
  _sandbox_dir = sandbox_dir;
  _dummy_sandboxes = dummy_mode;
  if (g_test_trace_scripts > 0 && dummy_mode)
    std::cerr << "tetutils using dummy sandboxes\n";

expose("deploySandbox", &Testutils::deploy_sandbox, "port", "rootpass",
       "?options");
  expose("destroySandbox", &Testutils::destroy_sandbox, "port", "?quiet_kill",
         false);
  expose("startSandbox", &Testutils::start_sandbox, "port");
  expose("stopSandbox", &Testutils::stop_sandbox, "port", "rootpass");
  expose("killSandbox", &Testutils::kill_sandbox, "port", "?quiet", false);
  expose("restartSandbox", &Testutils::restart_sandbox, "port", "rootpass");

  expose("snapshotSandboxConf", &Testutils::snapshot_sandbox_conf, "port");
  expose("beginSnapshotSandboxErrorLog",
         &Testutils::begin_snapshot_sandbox_error_log, "port");
  expose("endSnapshotSandboxErrorLog",
         &Testutils::end_snapshot_sandbox_error_log, "port");
  expose("changeSandboxConf", &Testutils::change_sandbox_conf, "port",
         "option", "value", "?section");
  expose("removeFromSandboxConf", &Testutils::remove_from_sandbox_conf, "port",
         "option", "?section");
  expose("getSandboxConfPath", &Testutils::get_sandbox_conf_path, "port");
  expose("getSandboxLogPath", &Testutils::get_sandbox_log_path, "port");
  expose("getSandboxPath", &Testutils::get_sandbox_path, "?port",
         "?filename");

  expose("getShellLogPath", &Testutils::get_shell_log_path);

  expose("waitMemberState", &Testutils::wait_member_state, "port", "states");

  expose("expectPrompt", &Testutils::expect_prompt, "prompt", "value");
  expose("expectPassword", &Testutils::expect_password, "prompt", "value");
  expose("fetchCapturedStdout", &Testutils::fetch_captured_stdout);
  expose("fetchCapturedStderr", &Testutils::fetch_captured_stderr);

  expose("callMysqlsh", &Testutils::call_mysqlsh, "args");

  expose("makeFileReadOnly", &Testutils::make_file_readonly, "path");
  expose("grepFile", &Testutils::grep_file, "path", "pattern");

  expose("isReplaying", &Testutils::is_replaying);
  expose("fail", &Testutils::fail, "context");
  expose("versionCheck", &Testutils::version_check, "v1", "op", "v2");

  _delegate.print = print;
  _delegate.print_error = print;
  _mp.reset(new mysqlsh::dba::ProvisioningInterface(&_delegate));
}

void Testutils::set_sandbox_snapshot_dir(const std::string &dir) {
  _sandbox_snapshot_dir = dir;
}

void Testutils::set_test_execution_context(const std::string &file, int line) {
  _test_file = file;
  _test_line = line;
}

//!<  @name Sandbox Operations
///@{
/**
 * Gets the path to the configuration file for the specific sandbox.
 * @param port The port of the sandbox owning the configuration file being searched.
 *
 * This function will return the path to the configuration file for the sandbox
 * listening at the specified port.
 */
#if DOXYGEN_JS
  String Testutils::getSandboxConfPath(Integer port);
#elif DOXYGEN_PY
  str Testutils::get_sandbox_conf_path(int port);
#endif
///@}
std::string Testutils::get_sandbox_conf_path(int port) {
  return shcore::path::join_path(
      {_sandbox_dir, std::to_string(port), "my.cnf"});
}

//!<  @name Sandbox Operations
///@{
/**
 * Gets the path to the error log for the specific sandbox.
 * @param port The port of the sandbox which error log file path will be retrieved.
 *
 * This function will return the path to the error log for the sandbox
 * listening at the specified port.
 */
#if DOXYGEN_JS
  String Testutils::getSandboxLogPath(Integer port);
#elif DOXYGEN_PY
  str Testutils::get_sandbox_log_path(int port);
#endif
///@}
std::string Testutils::get_sandbox_log_path(int port) {
  return shcore::path::join_path(
      {_sandbox_dir, std::to_string(port), "sandboxdata", "error.log"});
}

//!<  @name Sandbox Operations
///@{
/**
 * Gets the path to any file contained on the sandbox datadir.
 * @param port Optional port of the sandbox for which a path will be retrieved.
 * @param port Optional name of the file path to be retrieved.
 *
 * This function will return path related to the sandboxes.
 *
 * If port is NOT specified, it will retrieve the path to the sandbox home
 * folder.
 *
 * If port is specified it will retrieve a path for that specific sandbox.
 *
 * If file is not specified, it will retrieve the path to the sandbox folder.
 *
 * If file is specified it will retrieve the path to that file only if it is a
 * a valid file:
 * - my.cnf is a valid file.
 * - Any other file, will be valid if it exists on the sandboxdata dir.
 */
#if DOXYGEN_JS
  String Testutils::getSandboxPath(Integer port, String name);
#elif DOXYGEN_PY
  str Testutils::get_sandbox_path(int port, str name);
#endif
///@}
std::string Testutils::get_sandbox_path(int port,
                                             const std::string& file) {
  std::string path;

  if (port == 0)
    path = _sandbox_dir;
  else {
    if (file.empty()) {
      path = shcore::path::join_path({_sandbox_dir, std::to_string(port)});
    } else {
      if (file == "my.cnf") {
        path = shcore::path::join_path(
          {_sandbox_dir, std::to_string(port), "my.cnf"});
      } else {
        path = shcore::path::join_path(
          {_sandbox_dir, std::to_string(port), "sandboxdata", file});
      }
    }
  }

  if (!shcore::path::exists(path))
    path.clear();

  return path;
}

//!<  @name Misc Utilities
///@{
/**
 * Gets the path to the shell log.
 */
#if DOXYGEN_JS
  String Testutils::getShellLogPath();
#elif DOXYGEN_PY
  str Testutils::get_shell_log_path();
#endif
///@}
std::string Testutils::get_shell_log_path() {
  return ngcommon::Logger::singleton()->logfile_name();
}

//!<  @name Testing Utilities
///@{
/**
 * Identifies if the test suite is being executed in reply mode.
 */
#if DOXYGEN_JS
  Bool Testutils::isReplying();
#elif DOXYGEN_PY
  bool Testutils::is_replying();
#endif
///@}
bool Testutils::is_replaying() {
  return mysqlshdk::db::replay::g_replay_mode ==
         mysqlshdk::db::replay::Mode::Replay;
}

//!<  @name Testing Utilities
///@{
/**
 * Causes the test to fail.
 *
 * This function can be used directly on the test script to cause a failure,
 * it is useful if the test validation should be done using pure code rather
 * than falling to the standard validation methods.
 */
#if DOXYGEN_JS
  Undefined Testutils::fail();
#elif DOXYGEN_PY
  None Testutils::fail();
#endif
///@}
void Testutils::fail(const std::string &context) {
  // TODO(alfredo) make and replace with a markup text processor
  std::string text = context;

  if (g_test_color_output) {
    text = shcore::str_replace(text, "<red>", "\x1b[31m");
    text = shcore::str_replace(text, "</red>", "\x1b[0m");
    text = shcore::str_replace(text, "<yellow>", "\x1b[33m");
    text = shcore::str_replace(text, "</yellow>", "\x1b[0m");
  } else {
    text = shcore::str_replace(text, "<red>", "");
    text = shcore::str_replace(text, "</red>", "");
    text = shcore::str_replace(text, "<yellow>", "");
    text = shcore::str_replace(text, "</yellow>", "");
  }
  ADD_FAILURE_AT(_test_file.c_str(), _test_line) << text << "\n";
}

void Testutils::snapshot_sandbox_conf(int port) {
  if (mysqlshdk::db::replay::g_replay_mode !=
      mysqlshdk::db::replay::Mode::Direct) {
    if (_sandbox_snapshot_dir.empty()) {
      throw std::logic_error("set_sandbox_snapshot_dir() not called");
    }
    std::string sandbox_cnf_path = get_sandbox_conf_path(port);
    std::string sandbox_cnf_bkpath =
        _sandbox_snapshot_dir + "/sandbox_" + std::to_string(port) + "_my.cnf";
    if (!_dummy_sandboxes) {
      // copy the config file from the sandbox dir to the snapshot dir
      shcore::copy_file(sandbox_cnf_path, sandbox_cnf_bkpath);
    } else {
      // copy the config file from the snapshot dir to the sandbox dir, creating
      // it if needed
      shcore::create_directory(shcore::path::dirname(sandbox_cnf_path));
      shcore::copy_file(sandbox_cnf_bkpath, sandbox_cnf_path);
      if (g_test_trace_scripts)
        std::cerr << "Copied " << sandbox_cnf_bkpath << " to "
                  << sandbox_cnf_path << "\n";
    }
  }
}

void Testutils::begin_snapshot_sandbox_error_log(int port) {
  if (_sandbox_snapshot_dir.empty()) {
    if (mysqlshdk::db::replay::g_replay_mode !=
        mysqlshdk::db::replay::Mode::Direct)
      throw std::logic_error("set_sandbox_snapshot_dir() not called");
    return;
  }
  std::string sandbox_log_path = get_sandbox_log_path(port);
  std::string sandbox_log_bkpath = shcore::path::join_path(
      _sandbox_snapshot_dir, "sandbox_" + std::to_string(port) + "_" +
                                 std::to_string(_snapshot_log_index) +
                                 "_error.log");
  if (_dummy_sandboxes) {
    _snapshot_log_index++;
    // copy the log file from the snapshot dir to the sandbox dir, creating
    // it if needed
    shcore::create_directory(shcore::path::dirname(sandbox_log_path));
    shcore::copy_file(sandbox_log_bkpath, sandbox_log_path);
    if (g_test_trace_scripts)
      std::cerr << "Copied " << sandbox_log_bkpath << " to " << sandbox_log_path
                << "\n";
  }
}

void Testutils::end_snapshot_sandbox_error_log(int port) {
  if (_sandbox_snapshot_dir.empty()) {
    if (mysqlshdk::db::replay::g_replay_mode !=
        mysqlshdk::db::replay::Mode::Direct)
      throw std::logic_error("set_sandbox_snapshot_dir() not called");
    return;
  }
  std::string sandbox_log_path = get_sandbox_log_path(port);
  std::string sandbox_log_bkpath = shcore::path::join_path(
      _sandbox_snapshot_dir, "sandbox_" + std::to_string(port) + "_" +
                                 std::to_string(_snapshot_log_index) +
                                 "_error.log");
  if (!_dummy_sandboxes) {
    _snapshot_log_index++;
    // copy the log file from the sandbox dir to the snapshot dir
    shcore::copy_file(sandbox_log_path, sandbox_log_bkpath);
  }
}

//!<  @name Sandbox Operations
///@{
/**
 * Deploys a sandbox using the indicated password and port
 * @param port The port where the sandbox wlil be listening for mysql protocol
 * connections.
 * @param pwd The password to be assigned to the root user.
 * @param options Additional options to be set on the sandbox configuration
 * file.
 *
 * This functions works when using either --record or --direct mode of the test
 * suite. It is an improved version of the deploySandboxInstance function of the
 * Admin API which will speed up the process of deploying a new sandbox.
 *
 * First time it is called, it will create a boilerplate sandbox using the normal
 * sandbox deployment procedure.
 *
 * It creates a new sandbox by copying the data on the boilerplate sandbox.
 *
 * When using --replay mode, the function does nothing.
 */
#if DOXYGEN_JS
  Undefined Testutils::deploySandbox(Integer port, String pwd,
                                     Dictionary options);
#elif DOXYGEN_PY
  None Testutils::deploy_sandbox(int port, str pwd, Dictionary options);
#endif
///@}
void Testutils::deploy_sandbox(int port, const std::string &rootpass,
                               const shcore::Dictionary_t &opts) {
  mysqlshdk::db::replay::No_replay dont_record;
  if (!_dummy_sandboxes) {
    // Sandbox from a boilerplate
    if (!_boilerplate_rootpass.empty() && _boilerplate_rootpass == rootpass) {
      if (!deploy_sandbox_from_boilerplate(port, opts)) {
        prepare_sandbox_boilerplate(rootpass, port);
        if (!deploy_sandbox_from_boilerplate(port, opts)) {
          std::cerr << "Unable to deploy boilerplate sandbox\n";
          abort();
        }
      }
    } else {
      prepare_sandbox_boilerplate(rootpass, port);
      _boilerplate_rootpass = rootpass;

      deploy_sandbox_from_boilerplate(port, opts);
    }
  }
}

//!<  @name Sandbox Operations
///@{
/**
 * Destroys the sandbox listening at the indicated port
 * @param port The port where the sandbox is listening for mysql protocol
 * connections.
 *
 * This function also works when using the --direct and --record modes of the
 * test suite.
 *
 * It will delete the sandbox listening at the indicated port. This function
 * must be called after stopping or killing the sandbox.
 *
 * When using --replay mode, the function does nothing.
 */
#if DOXYGEN_JS
  Undefined Testutils::destroySandbox(Integer port);
#elif DOXYGEN_PY
  None Testutils::destroy_sandbox(int port);
#endif
///@}
void Testutils::destroy_sandbox(int port, bool quiet_kill) {
  mysqlshdk::db::replay::No_replay dont_record;
  kill_sandbox(port, quiet_kill);
#ifdef _WIN32
  // Make config file (and backups) readable in case it was made RO by some test
  std::string dirname = shcore::path::dirname(get_sandbox_conf_path(port));
  if (shcore::path::exists(dirname)) {
    for (const std::string &name : shcore::listdir(dirname)) {
      std::string path = shcore::path::join_path({dirname, name});
      auto dwAttrs = GetFileAttributes(path.c_str());
      if (dwAttrs != INVALID_FILE_ATTRIBUTES) {
        dwAttrs &= ~FILE_ATTRIBUTE_READONLY;
        SetFileAttributes(path.c_str(), dwAttrs);
      }
    }
  }
#endif
  if (!_dummy_sandboxes) {
    shcore::Value::Array_type_ref errors;
    _mp->delete_sandbox(port, _sandbox_dir, &errors);
    if (errors && !errors->empty())
      std::cerr << "During delete of " << port << ": "
                << shcore::Value(errors).descr() << "\n";
  } else {
    if (!_sandbox_dir.empty()) {
      std::string sandbox_path =
          shcore::path::dirname(get_sandbox_conf_path(port));
      if (shcore::is_folder(sandbox_path))
        shcore::remove_directory(sandbox_path, true);
    }
  }
}

//!<  @name Sandbox Operations
///@{
/**
 * Starts the sandbox created at the indicated port
 * @param port The port where the sandbox listens for mysql protocol connections.
 *
 * This function also works when using the --direct and --record modes of the
 * test suite.
 *
 * It will retry up to 5 times starting the sandbox at the indicated port,
 * improving the success rate of the operation.
 *
 * This function will verify that any other sandbox running previously at the
 * same port is completely dead before each start attempt.
 *
 * When using --replay mode, the function does nothing.
 */
#if DOXYGEN_JS
  Undefined Testutils::startSandbox(Integer port);
#elif DOXYGEN_PY
  None Testutils::start_sandbox(int port);
#endif
///@}
void Testutils::start_sandbox(int port) {
  int retries = k_max_start_sandbox_retries;
  if (!_dummy_sandboxes) {
    bool failed = true;

    wait_sandbox_dead(port);

    while (retries-- > 0) {
      shcore::Value::Array_type_ref errors;
      _mp->start_sandbox(port, _sandbox_dir, &errors);
      if (errors && !errors->empty()) {
        int num_errors = 0;
        for (auto err : *errors) {
          if ((*err.as_map())["type"].as_string() == "ERROR") {
            num_errors++;
          }
        }
        if (num_errors == 0) {
          failed = false;
          break;
        }
        if (retries == 0 || g_test_trace_scripts) {
          std::cerr << "During start of " << port << ": "
                    << shcore::Value(errors).descr() << "\n";
          std::cerr << "Retried " << k_max_start_sandbox_retries << " times\n";
        }
        shcore::sleep_ms(1000);
      } else {
        failed = false;
        break;
      }
    }
    if (failed) {
      throw std::runtime_error("Could not start sandbox instance " +
                               std::to_string(port));
    }
  }
}


//!<  @name Sandbox Operations
///@{
/**
 * Stops the sandbox listening at the indicated port
 * @param port The port where the sandbox listens for mysql protocol connections.
 *
 * This function works when using the --direct and --record modes of the test
 * suite.
 *
 * This function performs the normal stop sandbox operation of the Admin API
 *
 * When using --replay mode, the function does nothing.
 */
#if DOXYGEN_JS
  Undefined Testutils::stopSandbox(Integer port);
#elif DOXYGEN_PY
  None Testutils::stop_sandbox(int port);
#endif
///@}
void Testutils::stop_sandbox(int port, const std::string &rootpass) {
  mysqlshdk::db::replay::No_replay dont_record;
  if (!_dummy_sandboxes) {
    shcore::Value::Array_type_ref errors;
    _mp->stop_sandbox(port, _sandbox_dir, rootpass, &errors);
    if (errors && !errors->empty())
      std::cerr << "During stop of " << port << ": "
                << shcore::Value(errors).descr() << "\n";
  }
}

//!<  @name Sandbox Operations
///@{
/**
 * Restarts the sandbox listening at the specified port.
 * @param port The port where the sandbox listens for mysql protocol connections.
 *
 * This function works when using the --direct and --record modes of the test
 * suite.
 *
 * This function executes the stop sandbox operation of this module followed
 * by the start sandbox operation.
 *
 * When using --replay mode, the function does nothing.
 */
#if DOXYGEN_JS
  Undefined Testutils::restartSandbox(Integer port);
#elif DOXYGEN_PY
  None Testutils::restart_sandbox(int port);
#endif
///@}
void Testutils::restart_sandbox(int port, const std::string &rootpass) {
  stop_sandbox(port, rootpass);
  start_sandbox(port);
}

//!<  @name Sandbox Operations
///@{
/**
 * Kills the sandbox listening at the indicated port
 * @param port The port where the sandbox listens for mysql protocol connections.
 *
 * This function works when using the --direct and --record modes of the test
 * suite.
 *
 * This function performs the normal kill sandbox operation of the Admin API but
 * also verifies that the sandbox is completely dead.
 *
 * When using --replay mode, the function does nothing.
 */
#if DOXYGEN_JS
  Undefined Testutils::killSandbox(Integer port);
#elif DOXYGEN_PY
  None Testutils::kill_sandbox(int port);
#endif
///@}
void Testutils::kill_sandbox(int port, bool quiet) {
  if (!_dummy_sandboxes) {
    shcore::Value::Array_type_ref errors;
    _mp->kill_sandbox(port, _sandbox_dir, &errors);
    // Only output errors to stderr if quiet mode is disabled (default).
    if (!quiet && errors && !errors->empty())
      std::cerr << "During kill of " << port << ": "
                << shcore::Value(errors).descr() << "\n";
    wait_sandbox_dead(port);
  }
}

#ifndef WIN32
static int os_file_lock(int fd) {
  struct flock lk;

  lk.l_type = F_WRLCK;
  lk.l_whence = SEEK_SET;
  lk.l_start = lk.l_len = 0;

  if (fcntl(fd, F_SETLK, &lk) == -1) {
    if (errno == EAGAIN || errno == EACCES) {
      return 1;  // already locked
    }
    return (-1);  // error
  }
  return (0);  // not locked
}
#endif

void Testutils::wait_sandbox_dead(int port) {
#ifdef _WIN32
  // In Windows, it should be enough to see if the ibdata file is locked
  std::string ibdata =
      _sandbox_dir + "/" + std::to_string(port) + "/sandboxdata/ibdata1";
  while (true) {
    FILE *f = fopen(ibdata.c_str(), "a");
    if (f) {
      fclose(f);
      break;
    }
    if (errno == ENOENT)
      break;
    shcore::sleep_ms(500);
  }
#else
  while (mysqlshdk::utils::check_lock_file(_sandbox_dir + "/" +
                                           std::to_string(port) +
                                           "/sandboxdata/mysqld.sock.lock")) {
    shcore::sleep_ms(500);
  }
  // wait for innodb to release lock from ibdata file
  int ibdata_fd =
      open((_sandbox_dir + "/" + std::to_string(port) + "/sandboxdata/ibdata1")
               .c_str(),
           O_RDONLY);
  if (ibdata_fd > 0) {
    while (os_file_lock(ibdata_fd) > 0) {
      shcore::sleep_ms(1000);
    }
    ::close(ibdata_fd);
  }
#endif
}

bool is_configuration_option(const std::string &option,
                             const std::string& line) {
  std::string normalized = shcore::str_replace(line, " ", "");
  return (normalized == option || normalized.find(option + "=") == 0);
}

bool is_section_line(const std::string& line, const std::string &section = "") {
  bool ret_val = false;

  if (!line.empty()) {
    std::string normalized = shcore::str_strip(line);
    if (normalized[0] == '[' && normalized[normalized.length()-1] == ']') {
      ret_val = true;

      if (!section.empty())
        ret_val = normalized == "[" + section + "]";
    }
  }

  return ret_val;
}

//!<  @name Sandbox Operations
///@{
/**
 * Delete lines with the option from the given config file.
 * @param port The port of the sandbox where the configuration will be updated.
 * @param option The option name that will be removed from the configuration file.
 * @param section The section from which the option will be removed.
 *
 * This function will remove any occurrence of the configuration option on the
 * indicated section.
 *
 * If the section is not specified the operation will be done on every section
 * of the file.
 */
#if DOXYGEN_JS
  Undefined Testutils::removeFromSandboxConf(Integer port, String option, String section);
#elif DOXYGEN_PY
  None Testutils::remove_from_sandbox_conf(int port, str option, str section);
#endif
///@}
void Testutils::remove_from_sandbox_conf(int port, const std::string &option,
                                         const std::string &section) {
  if (_dummy_sandboxes)
    return;

  std::string cfgfile_path = get_sandbox_conf_path(port);
  std::string new_cfgfile_path = cfgfile_path + ".new";
  std::ofstream new_cfgfile(new_cfgfile_path);
  std::ifstream cfgfile(cfgfile_path);
  std::string line;

  bool in_section = false;
  while (std::getline(cfgfile, line)) {

    if (is_section_line(line, section))
      in_section = true;
    else if (is_section_line(line))
      in_section = false;

    // If we are in the right section and the option is found, the line is
    // removed from the cfg file
    if (in_section) {
      if (is_configuration_option(option, line))
        continue;
    }

    new_cfgfile << line << std::endl;
  }
  cfgfile.close();
  new_cfgfile.close();
  shcore::delete_file(cfgfile_path);
  shcore::rename_file(new_cfgfile_path, cfgfile_path);
}

//!<  @name Sandbox Operations
///@{
/**
 * Change sandbox config option and add it if it's not in the my.cnf yet
 * @param port The port of the sandbox where the configuration will be updated.
 * @param option The option to be updated or added.
 * @param value The new value for the option.
 * @param section The section on which the option will be added or updated.
 *
 * This function will replace the value of the configuration option from the
 * indicated section of the configuration file.
 *
 * If the option does not exist it will be added.
 *
 * If the section is not indicated, the operation will be done on every section
 * of the configuration file.
 */
#if DOXYGEN_JS
  Undefined Testutils::changeSandboxConf(Integer port, String option,
                                         String value, String section);
#elif DOXYGEN_PY
  None Testutils::change_sandbox_conf(int port, str option, str value,
                                      str section);
#endif
///@}
void Testutils::change_sandbox_conf(int port, const std::string &option,
                                    const std::string& value,
                                    const std::string &section) {
  if (_dummy_sandboxes)
    return;

  std::string cfgfile_path = get_sandbox_conf_path(port);
  std::string new_cfgfile_path = cfgfile_path + ".new";
  std::ofstream new_cfgfile(new_cfgfile_path);
  std::ifstream cfgfile(cfgfile_path);
  std::string line;


  bool in_section = false;
  while (std::getline(cfgfile, line)) {

    if (is_section_line(line, section)) {
      // As soon as the section is found adds the option with the new value
      in_section = true;
      new_cfgfile << line << std::endl;
      new_cfgfile << option << " = " << value << std::endl;
      continue;
    } else if (is_section_line(line)) {
      in_section = false;
    }

    // If we are in the right section and the option is found, it is
    // removed since it will be the old value
    if (in_section) {
      if (is_configuration_option(option, line))
        continue;
    }

    new_cfgfile << line << std::endl;
  }

  cfgfile.close();
  new_cfgfile.close();
  shcore::delete_file(cfgfile_path);
  shcore::rename_file(new_cfgfile_path, cfgfile_path);
}

//!<  @name InnoDB Cluster Utilities
///@{
/**
 * Waits until a cluster member reaches one of the specified states.
 * @param port The port of the instance to be polled listens for MySQL connections.
 * @param states An array containing the states that would cause the poll cycle to finish.
 * @returns The state of the member.
 *
 * This function is to be used with the members of a cluster.
 *
 * It will start a polling cycle verifying the member state, the cycle will end
 * when one of the expected states is reached or if the timeout of 60 seconds
 * occurs.
 */
#if DOXYGEN_JS
String Testutils::waitMemberState(Integer port, String[] states);
#elif DOXYGEN_PY
str Testutils::wait_member_state(int port, str[] states);
#endif
///@}
std::string Testutils::wait_member_state(int member_port,
                                         const std::string &states) {
  if (states.empty())
    throw std::invalid_argument(
        "states argument for wait_member_state() can't be empty");

  // Use the shell's active session
  if (auto shell = _shell.lock()) {
    if (!shell->shell_context()->get_dev_session())
      throw std::runtime_error("No active shell session");
    auto session =
        shell->shell_context()->get_dev_session()->get_core_session();

    int curtime = 0;
    std::string current_state;
    while (curtime < k_wait_member_timeout) {
      auto result = session->query(
          "SELECT member_state FROM "
          "performance_schema.replication_group_members "
          "WHERE member_port = " +
          std::to_string(member_port));
      current_state = "(MISSING)";
      if (auto row = result->fetch_one()) {
        current_state = row->get_string(0);
      }
      if (states.find(current_state) != std::string::npos)
        return current_state;

      if (mysqlshdk::db::replay::g_replay_mode ==
          mysqlshdk::db::replay::Mode::Replay) {
        // in replay mode we can wait much less (or not at all)
        shcore::sleep_ms(1);
      } else {
        shcore::sleep_ms(1000);
      }
      curtime++;
    }
    throw std::runtime_error(
        "Timeout while waiting for cluster member to become one of " + states +
        ": seems to be stuck as " + current_state);
  } else {
    throw std::logic_error("Lost reference to shell object");
  }
}

//!<  @name Misc Utilities
///@{
/**
 * Changes access attributes to a file to be read only.
 * @param path The path to the file to be made read only.
 * @returns 0 on success, -1 on failure
 */
#if DOXYGEN_JS
  Integer Testutils::makeFileReadonly(String path);
#elif DOXYGEN_PY
  int Testutils::make_file_readonly(str path);
#endif
///@}
int Testutils::make_file_readonly(const std::string &path) {
#ifndef _WIN32
  // Set permissions on configuration file to 444 (chmod only works on
  // unix systems).
  return chmod(path.c_str(), S_IRUSR | S_IRGRP | S_IROTH);
#else
  auto dwAttrs = GetFileAttributes(path.c_str());
  // set permissions on configuration file to read only
  if (SetFileAttributes(path.c_str(), dwAttrs | FILE_ATTRIBUTE_READONLY))
    return 0;
  return -1;
#endif
}

//!<  @name Misc Utilities
///@{
/**
 * Search for a pattern on a file.
 * @param path The path to the file where the pattern will be searched.
 * @param pattern The pattern to be searched on the file.
 * @returns Array containing the matching lines.
 *
 * This function will read each line of the file and match it using the provided
 * glob-like pattern using backtracking.
 *
 * Note: works with ASCII only, no utf8 support.
 *
 * This function will return all the lines that matched the given pattern.
 */
#if DOXYGEN_JS
  List Testutils::grepFile(String path, String pattern);
#elif DOXYGEN_PY
  list Testutils::grep_file(str path, str pattern);
#endif
///@}
shcore::Array_t Testutils::grep_file(const std::string &path,
                                     const std::string &pattern) {
  std::ifstream f(path);
  if (!f.good())
    throw std::runtime_error("grep error: " + path + ": " + strerror(errno));
  shcore::Array_t result = shcore::make_array();
  while (!f.eof()) {
    std::string line;
    std::getline(f, line);
    if (shcore::match_glob("*" + pattern + "*", line))
      result->push_back(shcore::Value(line));
  }
  return result;
}

//!<  @name Testing Utilities
///@{
/**
 * Sets an expected prompt as well as the response to be given.
 * @param prompt The prompt to be expected.
 * @param answer The answer to be given when the prompt is received.
 *
 * Some of the interative functions of the shell require information from the
 * user, this is done through prompts.
 *
 * This function can be used to identify an expected prompt as well as defining
 * the response that should be given to that prompt.
 *
 * If something different than the expected is prompted, will cause the test to
 * fail.
 *
 * If the prompt matches the expected prompt, the answer associated to the
 * prompt will be given.
 *
 * Use * on the prompt to cause any prompt to be valid (bypass the expected
 * prompt validation).
 */
#if DOXYGEN_JS
  Undefined Testutils::expectPrompt(String prompt, String answer);
#elif DOXYGEN_PY
  None Testutils::expect_prompt(str prompt, str answer);
#endif
///@}
void Testutils::expect_prompt(const std::string &prompt,
                              const std::string &text) {
  _feed_prompt(prompt, text);
}

//!<  @name Testing Utilities
///@{
/**
 * Sets an expected password prompt as well as the password to be given as response.
 * @param prompt The prompt to be expected.
 * @param password The password to be given when the password prompt is received.
 *
 * Some of the interative functions of the shell require a password from the
 * user, this is done through password prompts.
 *
 * This function can be used to identify an expected password prompt as well as
 * defining the password that should be given to that prompt.
 *
 * If something different than the expected is prompted, will cause the test to
 * fail.
 *
 * If the password prompt matches the expected password prompt, the password
 * associated to the prompt will be given.
 *
 * Use * on the prompt to cause any password prompt to be valid (bypass the
 * expected password prompt validation).
 */
#if DOXYGEN_JS
  Undefined Testutils::expectPassword(String prompt, String password);
#elif DOXYGEN_PY
  None Testutils::expect_password(str prompt, str password);
#endif
///@}
void Testutils::expect_password(const std::string &prompt,
                                const std::string &text) {
  _feed_password(prompt, text);
}

std::string Testutils::fetch_captured_stdout() {
  return _fetch_stdout();
}

std::string Testutils::fetch_captured_stderr() {
  return _fetch_stderr();
}

void Testutils::prepare_sandbox_boilerplate(const std::string &rootpass,
                                            int port) {
  if (g_test_trace_scripts)
    std::cerr << "Preparing sandbox boilerplate...\n";

  std::string boilerplate =
      shcore::path::join_path(_sandbox_dir, "myboilerplate");
  if (shcore::is_folder(boilerplate) && _use_boilerplate) {
    if (g_test_trace_scripts)
      std::cerr << "Reusing existing sandbox boilerplate at " << boilerplate
                << "\n";
    return;
  }

  // Create a sandbox, shut it down and then keep a copy of its basedir
  // to be reused for future deployments

  shcore::Value::Array_type_ref errors;
  shcore::Value mycnf_options = shcore::Value::new_array();
  mycnf_options.as_array()->push_back(shcore::Value("innodb_log_file_size=1M"));
  mycnf_options.as_array()->push_back(
      shcore::Value("innodb_log_buffer_size=1M"));
  mycnf_options.as_array()->push_back(
      shcore::Value("innodb_data_file_path=ibdata1:10M:autoextend"));

  _mp->set_verbose(g_test_trace_scripts > 1);
  _mp->create_sandbox(port, port * 10, _sandbox_dir, rootpass, mycnf_options,
                      true, true, &errors);
  if (errors && !errors->empty()) {
    std::cerr << "Error deploying sandbox:\n";
    for (auto &v : *errors)
      std::cerr << v.descr() << "\n";
    throw std::runtime_error("Error deploying sandbox");
  }

  {
    mysqlshdk::db::replay::No_replay noreplay;
    auto session = mysqlshdk::db::mysql::Session::create();
    auto options = mysqlshdk::db::Connection_options("root@localhost");
    options.set_port(port);
    options.set_password(rootpass);
    session->connect(options);
    auto result = session->query("select @@version");
    std::string version = result->fetch_one()->get_string(0);
    shcore::create_file(shcore::path::join_path(
      _sandbox_dir, std::to_string(port), "version.txt"),
      mysqlshdk::utils::Version(version).get_full());
  }

  stop_sandbox(port, rootpass);

  wait_sandbox_dead(port);

  change_sandbox_conf(port, "port", "<PLACEHOLDER>");
  remove_from_sandbox_conf(port, "server_id", "mysqld");
  remove_from_sandbox_conf(port, "datadir", "mysqld");
  remove_from_sandbox_conf(port, "log_error", "mysqld");
  remove_from_sandbox_conf(port, "pid_file", "mysqld");
  remove_from_sandbox_conf(port, "secure_file_priv", "mysqld");
  remove_from_sandbox_conf(port, "loose_mysqlx_port", "mysqld");
  remove_from_sandbox_conf(port, "report_port", "mysqld");

  if (shcore::is_folder(boilerplate)) {
    shcore::remove_directory(boilerplate);
  }

#ifdef _WIN32
  // We'll wait up to ~1 minute trying to do the folder rename
  // in case the server has not stopped.
  int attempts = 30;
  bool retry = true;
  while (retry && attempts) {
    try {
      shcore::rename_file(
        shcore::path::join_path(_sandbox_dir, std::to_string(port)), boilerplate);
      retry = false;
    } catch (const std::exception& err) {
      std::cout << "Failed attempt creating boilerplate sandbox: " << err.what() << std::endl;
      std::string message = err.what();
      if (message.find("Permission denied") != std::string::npos) {
        attempts--;
        shcore::sleep_ms(2000);
      } else {
        retry = false;
      }
    }
  }
#else
  shcore::rename_file(
    shcore::path::join_path(_sandbox_dir, std::to_string(port)), boilerplate);
#endif

  shcore::delete_file(
      shcore::path::join_path(boilerplate, "sandboxdata", "auto.cnf"));
  shcore::delete_file(
      shcore::path::join_path(boilerplate, "sandboxdata", "general.log"));
  shcore::delete_file(
      shcore::path::join_path(boilerplate, "sandboxdata", "mysqld.sock"));
  shcore::delete_file(
      shcore::path::join_path(boilerplate, "sandboxdata", "mysqlx.sock"));
  shcore::delete_file(
      shcore::path::join_path(boilerplate, "sandboxdata", "error.log"));

  if (g_test_trace_scripts)
    std::cerr << "Created sandbox boilerplate at " << boilerplate << "\n";
}

void copy_boilerplate_sandbox(const std::string &from,
                                         const std::string& to) {
  shcore::create_directory(to);
  shcore::iterdir(from, [from, to](const std::string &name) {
    try {
      std::string item_from = shcore::path::join_path(from, name);
      std::string item_to = shcore::path::join_path(to, name);

      if (shcore::is_folder(item_from)) {
        copy_boilerplate_sandbox(item_from, item_to);
      } else if (shcore::file_exists(item_from)) {
#ifndef _WIN32
        if (name == "mysqld") {
          if (symlink(item_from.c_str(), item_to.c_str()) != 0) {
            throw std::runtime_error(shcore::str_format(
                "Unable to create symlink %s to %s: %s", item_to.c_str(),
                item_from.c_str(), strerror(errno)));
          }
          return true;
        }
#endif
        shcore::copy_file(item_from, item_to);
      }
    } catch (std::runtime_error &e) {
      if (errno != ENOENT)
        throw;
    }
    return true;
  });
}

void Testutils::validate_boilerplate(const std::string &sandbox_dir,
                                     const std::string &version) {
  std::string basedir =
      shcore::path::join_path(sandbox_dir, "myboilerplate");
  bool expired = false;
  if (shcore::is_folder(basedir)) {
    try {
      std::string bversion = shcore::get_text_file(
          shcore::path::join_path(basedir, "version.txt"));
      if (bversion != version) {
        std::cerr << "Sandbox boilerplate was created for " << bversion
                  << " but available mysqld is " << version << "\n";
        expired = true;
      }
    } catch (std::exception &e) {
      expired = true;
    }
    if (expired) {
      std::cout << "Deleting expired boilerplate at " << basedir << "...\n";
      shcore::remove_directory(basedir);
    } else {
      std::cerr << "Found apparently valid sandbox boilerplate at " << basedir
                << "\n";
    }
  }
}

bool Testutils::deploy_sandbox_from_boilerplate
  (int port, const shcore::Dictionary_t &opts) {
  if (g_test_trace_scripts)
    std::cerr << "Deploying sandbox " << port << " from boilerplate\n";
  std::string boilerplate =
      shcore::path::join_path(_sandbox_dir, "myboilerplate");

  std::string basedir =
      shcore::path::join_path(_sandbox_dir, std::to_string(port));

  // Copy basics
  try {
    copy_boilerplate_sandbox(boilerplate, basedir);
  } catch (std::exception &e) {
    std::cerr << "Error copying boilerplate for sandbox " << port << ": "
              << e.what() << "\n";
    throw std::logic_error("During lazy deployment of sandbox " +
                           std::to_string(port) + ": " + e.what());
  }
  // Customize
  change_sandbox_conf(port, "port", std::to_string(port));
  change_sandbox_conf(port, "server_id", std::to_string(port + 12345),
                      "mysqld");
  change_sandbox_conf(
      port, "datadir", shcore::str_replace(
                             shcore::path::join_path(basedir, "sandboxdata"),
                             "\\", "/"), "mysqld");
  change_sandbox_conf(
      port, "log_error",
                shcore::str_replace(shcore::path::join_path(
                                        basedir, "sandboxdata", "error.log"),
                                    "\\", "/"), "mysqld");
  change_sandbox_conf(
      port, "pid_file",
                shcore::str_replace(shcore::path::join_path(
                                        basedir, std::to_string(port) + ".pid"),
                                    "\\", "/"), "mysqld");
  change_sandbox_conf(port, "secure_file_priv", shcore::path::join_path(
                                                      basedir, "mysql-files"),
                                                      "mysqld");
  change_sandbox_conf(port, "loose_mysqlx_port", std::to_string(port * 10),
                      "mysqld");
  change_sandbox_conf(port, "report_port", std::to_string(port), "mysqld");
  change_sandbox_conf(port, "general_log", "1", "mysqld");

  if (opts && !opts->empty()) {
    for (const auto& option : (*opts)) {
      change_sandbox_conf(port, option.first, option.second.descr(), "mysqld");
    }
  }

  start_sandbox(port);

  return true;
}

//!<  @name Misc Utilities
///@{
/**
 * Compares two version strings.
 *
 * @param v1 the first version to be compared.
 * @param op the comparison operator to be used.
 * @param v2 the second version to be compared.
 *
 * This function performs the comparison operation at op.
 *
 * If op is empty, an == operation will be done
 */
#if DOXYGEN_JS
Boolean Testutils::versionCheck(String v1, String op, String v2);
#elif DOXYGEN_PY
bool Testutils::version_check(str v1, str op, str v2);
#endif
///@}
bool Testutils::version_check(const std::string &v1, const std::string &op,
                              const std::string &v2) {
  bool ret_val = true;

  mysqlshdk::utils::Version ver1, ver2;

  ver1 = mysqlshdk::utils::Version(v1);
  ver2 = mysqlshdk::utils::Version(v2);

  if (ret_val) {
    if (op.empty() || op == "==")
      ret_val = ver1 == ver2;
    else if (op == "!=")
      ret_val = ver1 != ver2;
    else if (op == ">=")
      ret_val = ver1 >= ver2;
    else if (op == "<=")
      ret_val = ver1 <= ver2;
    else if (op == ">")
      ret_val = ver1 > ver2;
    else if (op == "<")
      ret_val = ver1 < ver2;
    else
      throw std::logic_error(get_function_name("versionCheck") +
                             ": Invalid operator: " + op);
  }

  return ret_val;
}

/**  @name Sandbox Operations
 *
 * Utilities that provide a reliable handling of sandboxes.
 */
///@{
///@}

/**  @name InnoDB Cluster Utilities
 *
 * Utilities specific for InnoDB Cluster Tests.
 */
///@{
///@}

/**  @name Testing Utilities
 *
 * Utilities related to the testing framework.
 */
///@{
///@}

/**  @name Misc Utilities
 *
 * Other utilities.
 */
///@{
///@}

mysqlshdk::db::Connection_options Testutils::sandbox_connection_options(
    int port, const std::string &password) {
  mysqlshdk::db::Connection_options copts;
  copts.set_scheme("mysql");
  copts.set_host("localhost");
  copts.set_port(port);
  copts.set_user("root");
  copts.set_password(password);
  return copts;
}


namespace {
void setup_recorder_environment() {
  static char mode[512];
  static char prefix[512];

  snprintf(mode, sizeof(mode), "MYSQLSH_RECORDER_MODE=");
  snprintf(prefix, sizeof(prefix), "MYSQLSH_RECORDER_PREFIX=");

  // If session recording is wanted, we need to append a mysqlprovision specific
  // suffix to the output path, which also has to be different for each call
  if (mysqlshdk::db::replay::g_replay_mode !=
      mysqlshdk::db::replay::Mode::Direct) {
    if (mysqlshdk::db::replay::g_replay_mode ==
        mysqlshdk::db::replay::Mode::Record) {
      snprintf(mode, sizeof(mode), "MYSQLSH_RECORDER_MODE=record");
    } else {
      snprintf(mode, sizeof(mode), "MYSQLSH_RECORDER_MODE=replay");
    }
    snprintf(prefix, sizeof(prefix), "MYSQLSH_RECORDER_PREFIX=%s",
             mysqlshdk::db::replay::external_recording_path("mysqlsh").c_str());
  }
  putenv(mode);
  putenv(prefix);
}
}  // namespace

int Testutils::call_mysqlsh(const shcore::Array_t &args) {
  std::vector<std::string> argv = shcore::Value(args).to_string_vector();
  std::vector<const char *> full_argv;
  std::string path;

  setup_recorder_environment();

  if (mysqlshdk::db::replay::g_replay_mode !=
      mysqlshdk::db::replay::Mode::Direct) {
    // use mysqlshrec unless in direct mode
    path = shcore::path::join_path(shcore::path::dirname(_mysqlsh_path),
                                   "mysqlshrec");
    full_argv.push_back(path.c_str());
  } else {
    full_argv.push_back(_mysqlsh_path.c_str());
  }
  assert(strlen(full_argv.front()) > 0);

  for (const std::string &arg : argv) {
    full_argv.push_back(arg.c_str());
  }
  if (g_test_trace_scripts) {
    std::cerr << shcore::str_join(full_argv, " ") << "\n";
  }
  full_argv.push_back(nullptr);

  char c;
  int exit_code = 1;
  std::string output;

  shcore::Process_launcher process(&full_argv[0]);
#ifdef _WIN32
  process.set_create_process_group();
#endif
  std::shared_ptr<mysqlsh::Mysql_shell> shell(_shell.lock());
  try {
    // Starts the process
    process.start();

    // // The password should be provided when it is expected that the Shell
    // // will prompt for it, in such case, we give it on the stdin
    // if (password) {
    //   std::string pwd(password);
    //   pwd.append("\n");
    //   process->write(pwd.c_str(), pwd.size());
    // }

    // Reads all produced output, until stdout is closed
    while (process.read(&c, 1) > 0) {
      if (g_test_trace_scripts && !shell)
        std::cout << c << std::flush;
      if (c == '\r')
        continue;
      if (c == '\n') {
        if (shell)
          shell->println(output);
        output.clear();
      } else {
        output += c;
      }
    }
    if (!output.empty()) {
      if (shell)
        shell->println(output);
    }
    // Wait until it finishes
    exit_code = process.wait();
  } catch (const std::system_error &e) {
    output = e.what();
    if (shell)
      shell->println(("Exception calling mysqlsh: " + output).c_str());
    exit_code = 256;  // This error code will indicate an error happened
                      // launching the process
  }
  return exit_code;
}
}  // namespace tests
