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
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_process.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "unittest/gtest_clean.h"

#include "modules/adminapi/mod_dba.h"
#include "modules/adminapi/mod_dba_cluster.h"

// TODO(anyone)
// - make deploySandbox() reuse sandboxes
// - make destroySandbox() expect that the final state of the sandbox is the
//   same as in the beginning
// - make a deployCluster() with reusable cluster
// - make a destroyCluster() that expects the cluster is in the same state
//   as original
// - add a wait_new_member() which does wait_slave_state() but with shorter
//   check interval/delay in replay mode (for faster execution)

namespace tests {

constexpr int k_wait_member_timeout = 60;
constexpr int k_max_start_sandbox_retries = 5;

static void print(void *, const char *s) {
  std::cout << s << "\n";
}

Testutils::Testutils(const std::string &sandbox_dir, bool dummy_mode,
                     const std::vector<int> &default_sandbox_ports,
                     std::shared_ptr<mysqlsh::Mysql_shell> shell)
    : _shell(shell), _default_sandbox_ports(default_sandbox_ports) {
  _sandbox_dir = sandbox_dir;
  _dummy_sandboxes = dummy_mode;
  if (getenv("TEST_DEBUG") && dummy_mode)
    std::cerr << "tetutils using dummy sandboxes\n";

  expose("deploySandbox", &Testutils::deploy_sandbox, "port", "rootpass");
  expose("destroySandbox", &Testutils::destroy_sandbox, "port");
  expose("startSandbox", &Testutils::start_sandbox, "port");
  expose("stopSandbox", &Testutils::stop_sandbox, "port", "rootpass");
  expose("killSandbox", &Testutils::kill_sandbox, "port");
  expose("restartSandbox", &Testutils::restart_sandbox, "port", "rootpass");

  expose("snapshotSandboxConf", &Testutils::snapshot_sandbox_conf, "port");
  expose("beginSnapshotSandboxErrorLog",
         &Testutils::begin_snapshot_sandbox_error_log, "port");
  expose("endSnapshotSandboxErrorLog",
         &Testutils::end_snapshot_sandbox_error_log, "port");
  expose("changeSandboxConf", &Testutils::change_sandbox_conf, "port",
         "option");
  expose("removeFromSandboxConf", &Testutils::remove_from_sandbox_conf, "port",
         "option");
  expose("getSandboxConfPath", &Testutils::get_sandbox_conf_path, "port");

  expose("waitMemberState", &Testutils::wait_member_state, "port", "states");

  expose("expectPrompt", &Testutils::expect_prompt, "prompt", "value");
  expose("expectPassword", &Testutils::expect_password, "prompt", "value");

  expose("makeFileReadOnly", &Testutils::make_file_readonly, "path");

  expose("isReplaying", &Testutils::is_replaying);
  expose("fail", &Testutils::fail, "context");

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

std::string Testutils::get_sandbox_conf_path(int port) {
  return shcore::path::join_path(
      {_sandbox_dir, std::to_string(port), "my.cnf"});
}

std::string Testutils::get_sandbox_log_path(int port) {
  return shcore::path::join_path(
      {_sandbox_dir, std::to_string(port), "sandboxdata", "error.log"});
}

bool Testutils::is_replaying() {
  return mysqlshdk::db::replay::g_replay_mode ==
         mysqlshdk::db::replay::Mode::Replay;
}

void Testutils::fail(const std::string &context) {
  ADD_FAILURE_AT(_test_file.c_str(), _test_line) << context << "\n";
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
      if (getenv("TEST_DEBUG"))
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
    if (getenv("TEST_DEBUG"))
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

void Testutils::deploy_sandbox(int port, const std::string &rootpass) {
  mysqlshdk::db::replay::No_replay dont_record;
  if (!_dummy_sandboxes) {
    if (true) {
      // Sandbox from a boilerplate
      if (!_boilerplate_rootpass.empty() && _boilerplate_rootpass == rootpass &&
          !_expected_boilerplate_version.empty()) {
        if (!deploy_sandbox_from_boilerplate(port)) {
          prepare_sandbox_boilerplate(rootpass);
          if (!deploy_sandbox_from_boilerplate(port)) {
            std::cerr << "Unable to deploy boilerplate sandbox\n";
            abort();
          }
        }
      } else {
        prepare_sandbox_boilerplate(rootpass);
        _boilerplate_rootpass = rootpass;

        deploy_sandbox_from_boilerplate(port);
      }
    } else {
      // Sandbox from scratch
      shcore::Value::Array_type_ref errors;
      shcore::Value mycnf_options = shcore::Value::new_array();
      mycnf_options.as_array()->push_back(
          shcore::Value("innodb_log_file_size=4M"));
      _mp->set_verbose(_debug);
      _mp->create_sandbox(port, port * 10, _sandbox_dir, rootpass,
                          mycnf_options, true, true, &errors);
      if (errors && !errors->empty())
        std::cerr << "During deploy of " << port << ": "
                  << shcore::Value(errors).descr() << "\n";
    }
  }
}

void Testutils::destroy_sandbox(int port) {
  mysqlshdk::db::replay::No_replay dont_record;
  kill_sandbox(port);
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
        if (retries == 0 || getenv("TEST_DEBUG")) {
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

void Testutils::restart_sandbox(int port, const std::string &rootpass) {
  stop_sandbox(port, rootpass);
  start_sandbox(port);
}

void Testutils::kill_sandbox(int port) {
  if (!_dummy_sandboxes) {
    shcore::Value::Array_type_ref errors;
    _mp->kill_sandbox(port, _sandbox_dir, &errors);
    if (errors && !errors->empty())
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

// Delete lines with the option from the given config file.
void Testutils::remove_from_sandbox_conf(int port, const std::string &option) {
  std::string cfgfile_path = get_sandbox_conf_path(port);
  std::string new_cfgfile_path = cfgfile_path + ".new";
  std::ofstream new_cfgfile(new_cfgfile_path);
  std::ifstream cfgfile(cfgfile_path);
  std::string line;
  while (std::getline(cfgfile, line)) {
    if (line.find(option) == std::string::npos)
      new_cfgfile << line << std::endl;
  }
  cfgfile.close();
  new_cfgfile.close();
  shcore::delete_file(cfgfile_path);
  shcore::rename_file(new_cfgfile_path, cfgfile_path);
}

// Change sandbox config option and add it if it's not in the my.cnf yet
void Testutils::change_sandbox_conf(int port, const std::string &option) {
  std::string cfgfile_path = get_sandbox_conf_path(port);
  std::string new_cfgfile_path = cfgfile_path + ".new";
  std::ofstream new_cfgfile(new_cfgfile_path);
  std::ifstream cfgfile(cfgfile_path);
  std::string line;
  bool found_mysqld = false;
  bool found_option = false;

  auto sep = option.find('=');
  if (sep != std::string::npos)
    ++sep;
  while (std::getline(cfgfile, line)) {
    if (found_mysqld) {
      if (line.compare(0, sep, option, 0, sep) == 0) {
        found_option = true;
        new_cfgfile << option << std::endl;
        continue;
      }
    }
    if (line == "[mysqld]")
      found_mysqld = true;
    new_cfgfile << line << std::endl;
  }
  if (!found_option)
    new_cfgfile << option << std::endl;
  cfgfile.close();
  new_cfgfile.close();
  shcore::delete_file(cfgfile_path);
  shcore::rename_file(new_cfgfile_path, cfgfile_path);
}

// InnoDB cluster stuff

int Testutils::wait_member_state(int member_port, const std::string &states) {
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
    while (curtime < k_wait_member_timeout) {
      auto result = session->query(
          "SELECT member_state FROM "
          "performance_schema.replication_group_members "
          "WHERE member_port = " +
          std::to_string(member_port));
      std::string current_state = "(MISSING)";
      if (auto row = result->fetch_one()) {
        current_state = row->get_string(0);
      }
      if (states.find(current_state) != std::string::npos)
        return 0;

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
        "Timeout while waiting for cluster member to become one of " + states);
  } else {
    throw std::logic_error("Lost reference to shell object");
  }
  return -1;
}

// Misc Utilities

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

void Testutils::expect_prompt(const std::string &prompt,
                              const std::string &text) {
  _feed_prompt(prompt, text);
}

void Testutils::expect_password(const std::string &prompt,
                                const std::string &text) {
  _feed_password(prompt, text);
}

void Testutils::prepare_sandbox_boilerplate(const std::string &rootpass) {
  if (_debug)
    std::cerr << "Preparing sandbox boilerplate...\n";

  std::string boilerplate =
      shcore::path::join_path(_sandbox_dir, "myboilerplate");
  if (shcore::is_folder(boilerplate) &&
      !_expected_boilerplate_version.empty() &&
      getenv("TEST_REUSE_SANDBOX_BOILERPLATE")) {
    if (_debug)
      std::cerr << "Reusing existing sandbox boilerplate as requested\n";

    return;
  }

  // Create a sandbox, shut it down and then keep a copy of its basedir
  // to be reused for future deployments
  int port =
      _default_sandbox_ports.empty() ? 3300 : _default_sandbox_ports.front();

  shcore::Value::Array_type_ref errors;
  shcore::Value mycnf_options = shcore::Value::new_array();
  mycnf_options.as_array()->push_back(shcore::Value("innodb_log_file_size=1M"));
  mycnf_options.as_array()->push_back(
      shcore::Value("innodb_log_buffer_size=1M"));
  mycnf_options.as_array()->push_back(
      shcore::Value("innodb_data_file_path=ibdata1:10M:autoextend"));

  _mp->set_verbose(_debug);
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
    shcore::create_file(shcore::path::join_path(boilerplate, "version.txt"),
                        version);
  }

  _mp->stop_sandbox(port, _sandbox_dir, rootpass, &errors);
  if (errors && !errors->empty()) {
    for (const auto &e : *errors) {
      if ((*e.as_map())["type"].descr() != "WARNING")
        std::cerr << "During stop of boilerplate sandbox " << port << ": "
        << e.descr() << "\n";
    }
  }
  remove_from_sandbox_conf(port, "port");
  remove_from_sandbox_conf(port, "server_id");
  remove_from_sandbox_conf(port, "datadir");
  remove_from_sandbox_conf(port, "log_error");
  remove_from_sandbox_conf(port, "pid_file");
  remove_from_sandbox_conf(port, "secure_file_priv");
  remove_from_sandbox_conf(port, "loose_mysqlx_port");
  remove_from_sandbox_conf(port, "report_port");

  if (shcore::is_folder(boilerplate)) {
    shcore::remove_directory(boilerplate);
  }
  shcore::rename_file(
      shcore::path::join_path(_sandbox_dir, std::to_string(port)), boilerplate);

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
}

bool Testutils::deploy_sandbox_from_boilerplate(int port) {
  if (_debug)
    std::cerr << "Deploying sandbox " << port << " from boilerplate\n";
  std::string boilerplate =
      shcore::path::join_path(_sandbox_dir, "myboilerplate");

  std::string basedir =
      shcore::path::join_path(_sandbox_dir, std::to_string(port));

  // Copy basics
  try {
    shcore::copy_dir(boilerplate, basedir);
  } catch (std::exception &e) {
    std::cerr << "Error copying boilerplate for sandbox " << port << ": "
              << e.what() << "\n";
    throw std::logic_error("During deployment of sandbox " +
                           std::to_string(port) + ": " + e.what());
  }
  // Customize
  change_sandbox_conf(port, "port=" + std::to_string(port));
  change_sandbox_conf(port, "server_id=" + std::to_string(port + 12345));
  change_sandbox_conf(
      port, "datadir=" +
      shcore::str_replace(shcore::path::join_path(basedir, "sandboxdata"),
                          "\\", "/"));
  change_sandbox_conf(
      port,
      "log_error=" +
      shcore::str_replace(shcore::path::join_path(basedir, "sandboxdata",
                                                  "error.log"), "\\", "/"));
  change_sandbox_conf(
      port, "pid_file=" +
      shcore::str_replace(shcore::path::join_path(
                          basedir, std::to_string(port) + ".pid"), "\\", "/"));
  change_sandbox_conf(port, "secure_file_priv=" + shcore::path::join_path(
                                                      basedir, "mysql-files"));
  change_sandbox_conf(port, "loose_mysqlx_port=" + std::to_string(port * 10));
  change_sandbox_conf(port, "report_port=" + std::to_string(port));
  change_sandbox_conf(port, "general_log=1");

  start_sandbox(port);

  if (!_boilerplate_checked) {
    _boilerplate_checked = true;
    try {
      std::string bversion = shcore::get_text_file(
          shcore::path::join_path(boilerplate, "version.txt"));
      if (bversion != _expected_boilerplate_version) {
        std::cerr << "WARNING: Boilerplate instance was created for a "
                     "different MySQL "
                     "version than current and will be recreated\n";
        destroy_sandbox(port);
        return false;
      }
    } catch (std::exception &e) {
      std::cerr << "Error checking boilerplate version: " << e.what() << "\n";
      return false;
    }
  }
  return true;
}

}  // namespace tests
