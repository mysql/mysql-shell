/* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms, as
   designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.
   This program is distributed in the hope that it will be useful,  but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
   the GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#ifdef HAVE_PYTHON
#include "mysqlshdk/include/scripting/python_utils.h"
#endif

#ifdef _WIN32
#include <Windows.h>
#else
#include <signal.h>
#endif

#include <mysql.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_stacktrace.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "shellcore/interrupt_handler.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mod_testutils.h"
#include "utils/utils_path.h"
#include "utils/utils_string.h"

using Version = mysqlshdk::utils::Version;

// Begin test configuration block

// TODO(.) remove Interrupt_ from the filter, delete the deprecated Python tests
const char *k_default_test_filter = "*:-Shell_py_dba_tests.*";


// Default execution mode for replayable tests
mysqlshdk::db::replay::Mode g_test_recording_mode =
    mysqlshdk::db::replay::Mode::Replay;

bool g_generate_validation_file = false;

int g_test_trace_scripts = 0;
int g_test_trace_sql = 0;
bool g_test_color_output = false;

// Default trace set (MySQL version) to be used for replay mode
mysqlshdk::utils::Version g_target_server_version = Version("8.0.4");
mysqlshdk::utils::Version g_highest_tls_version = Version();

// End test configuration block


#ifdef _WIN32
#define putenv _putenv
#endif

extern "C" {
const char *g_test_home = nullptr;
const char *g_mysqlsh_bin_folder = nullptr;
}
const char *g_mysqlsh_argv0;
char *g_mppath = nullptr;

std::string g_test_home_value;  // NOLINT

std::vector<std::pair<std::string, std::string> > g_skipped_tests;
std::vector<std::pair<std::string, std::string> > g_pending_fixes;

static std::string make_socket_absolute_path(const std::string &datadir,
                                             const std::string &socket) {
  if (socket.empty()) {
    return std::string{};
  }
#ifdef _WIN32
  return socket;
#else
  return shcore::path::normalize(
      shcore::path::join_path(std::vector<std::string>{datadir, socket}));
#endif
}

static void detect_mysql_environment(int port, const char *pwd) {
  std::string socket, xsocket, datadir;
  std::string hostname;
  int xport = 0;
  int server_id = 0;
  bool have_ssl = false;
  MYSQL *mysql;
  mysql = mysql_init(nullptr);
  unsigned int tcp = MYSQL_PROTOCOL_TCP;
  mysql_options(mysql, MYSQL_OPT_PROTOCOL, &tcp);
  // if connect succeeds or error is a server error, then there's a server
  if (mysql_real_connect(mysql, "localhost", "root", pwd, NULL, port, NULL,
                         0)) {
    const char *query = "show variables like '%socket'";
    if (mysql_real_query(mysql, query, strlen(query)) == 0) {
      MYSQL_RES *res = mysql_store_result(mysql);
      while (MYSQL_ROW row = mysql_fetch_row(res)) {
        auto lengths = mysql_fetch_lengths(res);
        if (strcmp(row[0], "socket") == 0 && row[1])
          socket = std::string(row[1], lengths[1]);
        if (strcmp(row[0], "mysqlx_socket") == 0 && row[1])
          xsocket = std::string(row[1], lengths[1]);
      }
      mysql_free_result(res);
    }

    {
      const char *query = "show variables like 'datadir'";
      if (mysql_real_query(mysql, query, strlen(query)) == 0) {
        MYSQL_RES *res = mysql_store_result(mysql);
        if (MYSQL_ROW row = mysql_fetch_row(res)) {
          auto lengths = mysql_fetch_lengths(res);
          datadir = std::string(row[1], lengths[1]);
        }
        mysql_free_result(res);
      }
    }

    {
      const char *query = "show variables like 'hostname'";
      if (mysql_real_query(mysql, query, strlen(query)) == 0) {
        MYSQL_RES *res = mysql_store_result(mysql);
        if (MYSQL_ROW row = mysql_fetch_row(res)) {
          hostname = row[1];
        }
        mysql_free_result(res);
      }
    }

    {
      const char *query =
          "select @@version, (@@have_ssl = 'YES' or @@have_openssl = 'YES'), "
          "@@mysqlx_port, @@server_id";
      if (mysql_real_query(mysql, query, strlen(query)) == 0) {
        MYSQL_RES *res = mysql_store_result(mysql);
        if (MYSQL_ROW row = mysql_fetch_row(res)) {
          g_target_server_version = mysqlshdk::utils::Version(row[0]);
          if (row[1] && strcmp(row[1], "1") == 0)
            have_ssl = true;
          if (row[2])
            xport = atoi(row[2]);
          server_id = atoi(row[3]);
        }
        mysql_free_result(res);
      }
    }

    {
      char const *const query = "SELECT @@tls_version";
      if (mysql_real_query(mysql, query, strlen(query)) == 0) {
        MYSQL_RES *res = mysql_store_result(mysql);
        if (MYSQL_ROW row = mysql_fetch_row(res)) {
          auto tls_versions = shcore::str_split(row[0], ",");
          for (auto i = tls_versions.crbegin(); i != tls_versions.crend();
               i++) {
            if (shcore::str_beginswith(tls_versions.back(), "TLSv")) {
              std::string ver((*i).begin() + 4, (*i).end());
              g_highest_tls_version = Version(ver);
              break;
            }
          }
        }
      }
    }
  }
  mysql_close(mysql);

  if (!xport) {
    std::cerr << "Could not query mysqlx_port. X plugin not installed?\n";
    exit(1);
  }

  const std::string socket_absolute =
      make_socket_absolute_path(datadir, socket);
  const std::string xsocket_absolute =
      make_socket_absolute_path(datadir, xsocket);

  std::string hostname_ip = mysqlshdk::utils::resolve_hostname_ipv4(hostname);

  std::cout << "Target MySQL server:\n";
  std::cout << "version=" << g_target_server_version.get_full() << "\n";
  std::cout << "hostname=" << hostname << ", ip=" << hostname_ip << "\n";
  std::cout << "server_id=" << server_id << ", ssl=" << have_ssl
            << ", highest_tls_version=" << g_highest_tls_version.get_full()
            << "\n";

  std::cout << "Classic protocol:\n";
  std::cout << "  port=" << port << '\n';
  std::cout << "  socket=" << socket;
  std::cout << ((socket != socket_absolute) ? " (" + socket_absolute + ")\n"
                                            : "\n");

  std::cout << "X protocol:\n";
  std::cout << "  xport=" << xport << '\n';
  std::cout << "  xsocket=" << xsocket;
  std::cout << ((xsocket != xsocket_absolute) ? " (" + xsocket_absolute + ")\n"
                                              : "\n");

  if (!getenv("MYSQL_SOCKET")) {
    static char path[1024];
    snprintf(path, sizeof(path), "MYSQL_SOCKET=%s", socket_absolute.c_str());
    if (putenv(path) != 0) {
      std::cerr << "MYSQL_SOCKET was not set and putenv failed to set it\n";
      exit(1);
    }
  }

  {
    // This environment variable makes libmysqlclient override the default
    // compiled-in socket path with the actual path in use
    static char path[1024];
    snprintf(path, sizeof(path), "MYSQL_UNIX_PORT=%s", socket_absolute.c_str());
    putenv(path);
  }

  if (!getenv("MYSQLX_SOCKET")) {
    static char path[1024];
    snprintf(path, sizeof(path), "MYSQLX_SOCKET=%s", xsocket_absolute.c_str());
    if (putenv(path) != 0) {
      std::cerr << "MYSQLX_SOCKET was not set and putenv failed to set it\n";
      exit(1);
    }
  }

  {
    static char my_hostname[1024];
    snprintf(my_hostname, sizeof(my_hostname), "MYSQL_HOSTNAME=%s",
             hostname.c_str());
    if (putenv(my_hostname) != 0) {
      std::cerr << "MYSQL_HOSTNAME could not be set\n";
      exit(1);
    }
  }
}

static bool delete_sandbox(int port) {
  MYSQL *mysql;
  mysql = mysql_init(nullptr);

  unsigned int tcp = MYSQL_PROTOCOL_TCP;
  mysql_options(mysql, MYSQL_OPT_PROTOCOL, &tcp);
  // if connect succeeds or error is a server error, then there's a server
  if (mysql_real_connect(mysql, "localhost", "root", "root", NULL, port, NULL,
                         0)) {
    std::cout << "Sandbox server running at " << port
              << ", shutting down and deleting\n";
    mysql_real_query(mysql, "shutdown", strlen("shutdown"));
  } else if (mysql_errno(mysql) < 2000 || mysql_errno(mysql) >= 3000) {
    std::cout << mysql_error(mysql) << "  " << mysql_errno(mysql) << "\n";
    std::cout << "Server already running on port " << port
              << " but can't shut it down\n";
    mysql_close(mysql);
    return false;
  }
  mysql_close(mysql);

  const char *tmpdir = getenv("TMPDIR");
  if (tmpdir) {
    std::string d;
    d = shcore::path::join_path(tmpdir, std::to_string(port));
    if (shcore::is_folder(d)) {
      try {
        shcore::remove_directory(d, true);
        std::cerr << "Deleted leftover sandbox dir " << d << "\n";
      } catch (std::exception &e) {
        std::cerr << "Error deleting sandbox dir " << d << ": " << e.what()
                  << "\n";
        return false;
      }
    }
  }
  return true;
}


static void check_zombie_sandboxes() {
  int port = 3306;
  if (getenv("MYSQL_PORT")) {
    port = atoi(getenv("MYSQL_PORT"));
  }
  int sport1, sport2, sport3;

  const char *sandbox_port1 = getenv("MYSQL_SANDBOX_PORT1");
  if (sandbox_port1) {
    sport1 = atoi(getenv("MYSQL_SANDBOX_PORT1"));
  } else {
    sport1 = port + 10;
  }
  const char *sandbox_port2 = getenv("MYSQL_SANDBOX_PORT2");
  if (sandbox_port2) {
    sport2 = atoi(getenv("MYSQL_SANDBOX_PORT2"));
  } else {
    sport2 = port + 20;
  }
  const char *sandbox_port3 = getenv("MYSQL_SANDBOX_PORT3");
  if (sandbox_port3) {
    sport3 = atoi(getenv("MYSQL_SANDBOX_PORT3"));
  } else {
    sport3 = port + 30;
  }

  bool have_zombies = false;

  have_zombies |= !delete_sandbox(sport1);
  have_zombies |= !delete_sandbox(sport2);
  have_zombies |= !delete_sandbox(sport3);

  if (have_zombies) {
    std::cout << "WARNING: mysqld running on port reserved for sandbox tests\n";
    std::cout << "Sandbox ports: " << sport1 << ", " << sport2 << ", " << sport3
              << "\n";
    std::cout << "If they're left from a previous run, terminate them first\n";
    std::cout << "Or setenv TEST_SKIP_ZOMBIE_CHECK to skip this check\n";
    std::cout << "Or setenv MYSQL_SANDBOX_PORT1..3 to pick different ports for "
                 "test sandboxes\n";
    exit(1);
  }
}

#ifndef _WIN32
static void catch_segv(int sig) {
  mysqlshdk::utils::print_stacktrace();
  signal(sig, SIG_DFL);
  kill(getpid(), sig);
}
#endif

int main(int argc, char **argv) {
  mysqlshdk::utils::init_stacktrace();

#if defined(WIN32)
  g_test_color_output = _isatty(_fileno(stdout));
#else
  g_test_color_output = isatty(STDOUT_FILENO) != 0;
#endif

  // Ignore broken pipe signal from broken connections
#ifndef _WIN32
  signal(SIGPIPE, SIG_IGN);
  signal(SIGSEGV, catch_segv);
  signal(SIGABRT, catch_segv);
#endif

#ifdef _WIN32
  // Try to enable VT100 escapes... if it doesn't work,
  // then it disables the ansi escape sequences
  // Supported in Windows 10 command window and some other terminals
  HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode;
  GetConsoleMode(handle, &mode);

  // ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(handle, mode | 0x004);
#endif

  // Allows customizing the path where the test data files are, if customized
  // it should be a path that contains the next folders:
  // - scripts: with all the tests that go through the Shell_script_tester
  // - data: additional test files
  // - traces: tre recorded trace files
  // - prompt files
  // If not customized it will point to the unit test folder in the source code
  g_test_home = getenv("MYSQLSH_TEST_HOME");
  if (!g_test_home) {
    g_test_home_value =
        shcore::path::join_path(MYSQLX_SOURCE_HOME, "unittest").c_str();
    g_test_home = g_test_home_value.c_str();
  }

  // This variable holds the path to the mysqlsh container, in case the tests
  // are being executed vs a shell package rather than the build directory
  g_mysqlsh_bin_folder = getenv("MYSQLSH_BIN_FOLDER");

  #ifdef __APPLE__
  struct rlimit rlp;
  getrlimit(RLIMIT_NOFILE, &rlp);
  if (rlp.rlim_cur < 10000) {
    std::cout << "Increasing open files limit FROM: "
              << rlp.rlim_cur;
    rlp.rlim_cur = 10000;
    setrlimit(RLIMIT_NOFILE, &rlp);
    getrlimit(RLIMIT_NOFILE, &rlp);
    std::cout << "  TO: " << rlp.rlim_cur
              << std::endl;
  }
#endif

#ifdef HAVE_V8
  extern void JScript_context_init();

  JScript_context_init();
#endif
  // init the ^C handler, so it knows what's the main thread
  shcore::Interrupts::init(nullptr);

  // Check TMPDIR environment variable for windows and other platforms without
  // TMPDIR defined.
  // NOTE: Required to be used as location for sandbox deployment.
  const char *tmpdir = getenv("TMPDIR");
  if (tmpdir == nullptr || strlen(tmpdir) == 0) {
    tmpdir = getenv("TEMP");  // TEMP usually used on Windows.
    static std::string temp("TMPDIR=");
    if (tmpdir == nullptr || strlen(tmpdir) == 0) {
      // Use the binary folder as default for the TMPDIR.
      std::string bin_folder = shcore::get_binary_folder();
      temp.append(bin_folder);
      std::cout << std::endl
                << "WARNING: TMPDIR environment variable is "
                   "empty or not defined. It will be set with the binary "
                   "folder path: "
                << temp << std::endl << std::endl;
    } else {
      temp.append(tmpdir);
    }
    putenv(&temp[0]);
  }

  bool show_help = false;
  if (const char *uri = getenv("MYSQL_URI")) {
    if (strcmp(uri, "root@localhost") != 0 &&
        strcmp(uri, "root@127.0.0.1") != 0) {
      std::cerr << "MYSQL_URI is set to " << getenv("MYSQL_URI") << "\n";
      std::cerr << "MYSQL_URI environment variable is no longer supported.\n";
      std::cerr << "Tests must run against local server using root user.\n";
      show_help = true;
    }
  }

  if (show_help) {
    std::cerr
        << "The following environment variables are available:\n"
        << "MYSQL_PORT classic protocol port for local MySQL (default 3306)\n"
        << "MYSQLX_PORT X protocol port for local MySQL (default 33060)\n"
        << "MYSQL_PWD root password for local MySQL server (default "
           ")\n"
        // << "MYSQL_REMOTE_HOST for tests against remove MySQL (default not
        // set)\n"
        // << "MYSQL_REMOTE_PWD root password for remote MySQL server (default
        // "")\n"
        // << "MYSQL_REMOTE_PORT classic port for remote MySQL (default 3306)\n"
        // << "MYSQLX_REMOTE_PORT X port for remote MySQL (default 33060)\n\n"
        << "MYSQL_SANDBOX_PORT1, MYSQL_SANDBOX_PORT2, MYSQL_SANDBOX_PORT3\n"
        << "    ports to use for test sandbox instances. X protocol will use\n"
        << "    MYSQL_SANDBOX_PORT1 * 10\n";
    exit(1);
  }

  if (!getenv("MYSQL_PORT")) {
    if (putenv(const_cast<char *>("MYSQL_PORT=3306")) != 0) {
      std::cerr << "MYSQL_PORT was not set and putenv failed to set it\n";
      exit(1);
    }
  }

  if (!getenv("MYSQLX_PORT")) {
    if (putenv(const_cast<char *>("MYSQLX_PORT=33060")) != 0) {
      std::cerr << "MYSQLX_PORT was not set and putenv failed to set it\n";
      exit(1);
    }
  }

  detect_mysql_environment(atoi(getenv("MYSQL_PORT")), "");

  if (!getenv("MYSQL_REMOTE_HOST")) {
    static char hostname[1024] = "MYSQL_REMOTE_HOST=";
    if (gethostname(hostname + strlen(hostname),
                    sizeof(hostname) - strlen(hostname)) != 0) {
      std::cerr << "gethostname() returned error: " << strerror(errno) << "\n";
      std::cerr << "Set MYSQL_REMOTE_HOST\n";
      // exit(1); this option is not used for now, so no need to fail
    }
    if (putenv(hostname) != 0) {
      std::cerr
          << "MYSQL_REMOTE_HOST was not set and putenv failed to set it\n";
      // exit(1);
    }
  }

  if (!getenv("MYSQL_REMOTE_PORT")) {
    if (putenv(const_cast<char *>("MYSQL_REMOTE_PORT=3306")) != 0) {
      std::cerr
          << "MYSQL_REMOTE_PORT was not set and putenv failed to set it\n";
      // exit(1);
    }
  }

  if (!getenv("MYSQLX_REMOTE_PORT")) {
    if (putenv(const_cast<char *>("MYSQLX_REMOTE_PORT=33060")) != 0) {
      std::cerr
          << "MYSQLX_REMOTE_PORT was not set and putenv failed to set it\n";
      // exit(1);
    }
  }

  // Reset these environment vars to start with a clean environment
  putenv(const_cast<char*>("MYSQLSH_RECORDER_PREFIX="));
  putenv(const_cast<char*>("MYSQLSH_RECORDER_MODE="));

// Override the configuration home for tests, to not mess with custom data
#ifdef WIN32
  _putenv_s("MYSQLSH_USER_CONFIG_HOME", ".");
#else
  setenv("MYSQLSH_USER_CONFIG_HOME", ".", 1);
#endif
  // Setup logger with default configs
  std::string log_path = shcore::path::join_path(shcore::get_user_config_path(),
        "mysqlsh.log");
  if (shcore::file_exists(log_path)) {
    std::cerr << "Deleting old " << log_path << " file\n";
    shcore::delete_file(log_path);
  }
  ngcommon::Logger::setup_instance(log_path.c_str(), false);

  bool got_filter = false;
  std::string target_version = g_target_server_version.get_base();
  const char *target = target_version.c_str();

  for (int index = 1; index < argc; index++) {
    if (shcore::str_beginswith(argv[index], "--gtest_filter")) {
      got_filter = true;
    } else if (shcore::str_caseeq(argv[index], "--direct")) {
      g_test_recording_mode = mysqlshdk::db::replay::Mode::Direct;
    } else if (shcore::str_beginswith(argv[index], "--record")) {
      g_test_recording_mode = mysqlshdk::db::replay::Mode::Record;
      char *p = strchr(argv[index], '=');
      if (!p) {
        std::cerr << "--record= option requires target name to be specified\n";
        exit(1);
      }
      target = p + 1;
    } else if (shcore::str_beginswith(argv[index], "--replay")) {
      g_test_recording_mode = mysqlshdk::db::replay::Mode::Replay;
      char *p = strchr(argv[index], '=');
      if (p) {
        target = p + 1;
      } else {
        target = target_version.c_str();
      }
    } else if (shcore::str_caseeq(argv[index], "--generate-validation-file")) {
      g_generate_validation_file = true;
    } else if (strcmp(argv[index], "--trace-no-stop") == 0) {
      // continue executing script until the end on failure
      g_test_trace_scripts = 1;
    } else if (strcmp(argv[index], "--trace") == 0) {
      // stop executing script on failure
      g_test_trace_scripts = 2;
    } else if (strcmp(argv[index], "--trace-sql") == 0) {
      g_test_trace_sql = 1;
    } else if (strcmp(argv[index], "--trace-all-sql") == 0) {
      g_test_trace_sql = 2;
    } else if (!shcore::str_beginswith(argv[index], "--gtest_") &&
               strcmp(argv[index], "--help") != 0) {
      std::cerr << "Invalid option " << argv[index] << "\n";
      exit(1);
    }
  }

  if (g_test_recording_mode != mysqlshdk::db::replay::Mode::Direct) {
    std::string tracedir =
        shcore::path::join_path(g_test_home, "traces", target, "");
    mysqlshdk::db::replay::set_recording_path_prefix(tracedir);
  }

  if (g_test_recording_mode == mysqlshdk::db::replay::Mode::Record) {
    shcore::ensure_dir_exists(mysqlshdk::db::replay::g_recording_path_prefix);
  }

  ::testing::InitGoogleTest(&argc, argv);

  std::string filter = ::testing::GTEST_FLAG(filter);
  std::string new_filter = filter;
  if (!got_filter)
    new_filter = k_default_test_filter;
  if (new_filter != filter) {
    std::cout << "Executing defined filter: " << new_filter.c_str()
              << std::endl;
    ::testing::GTEST_FLAG(filter) = new_filter.c_str();
  }
  std::string mppath;
  // If using a custom shell package, initial path is the binary folder
  if (g_mysqlsh_bin_folder) {
    mppath.assign(g_mysqlsh_bin_folder);
  } else {
  // If running on the build dir then initial path is the tests binary folder
    mppath = shcore::get_binary_folder();
  }

#ifndef _WIN32
  // On linux, we need to tell the UTs where the mysqlprovision executable is
  mppath = shcore::path::dirname(mppath);
  mppath = shcore::path::join_path(mppath, "share", "mysqlsh");
#endif
  mppath = shcore::path::join_path(mppath, "mysqlprovision.zip");

  std::string mysqlsh_path;

  // If using the test package g_test_home_value is empty but g_test_home
  // points to the test package folder and it contains mysqlshrec
  if (g_test_home_value.empty()) {
    mysqlsh_path = shcore::path::join_path(g_test_home, "mysqlshrec");
  } else {
    // On this case we are on the build folders, the path is calculated
    // from the the binary folder
    mysqlsh_path = shcore::get_binary_folder();
#ifndef _WIN32
    mysqlsh_path = shcore::path::dirname(mysqlsh_path);
#endif
    mysqlsh_path = shcore::path::join_path(mysqlsh_path, "mysqlshrec");
  }

#ifdef _WIN32
  mysqlsh_path.append(".exe");
#endif

  g_mysqlsh_argv0 = mysqlsh_path.c_str();

  g_mppath = strdup(mppath.c_str());

  // Check for leftover sandbox servers
  if (!getenv("TEST_SKIP_ZOMBIE_CHECK")) {
    check_zombie_sandboxes();
  }

  tests::Testutils::validate_boilerplate(getenv("TMPDIR"),
                                         g_target_server_version.get_full());

  if (!g_test_home_value.empty())
    std::cout << "Testing: Shell Build." << std::endl;
  else
    std::cout << "Testing: Shell Package." << std::endl;
  std::cout << "Shell Binary: " << g_mysqlsh_argv0 << std::endl;
  std::cout << "MySQL Provision: " << g_mppath << std::endl;
  std::cout << "Test Data Home: " << g_test_home << std::endl;

  switch (g_test_recording_mode) {
    case mysqlshdk::db::replay::Mode::Direct:
      std::cout << "Session replay not enabled.\n";
      break;
    case mysqlshdk::db::replay::Mode::Record:
      std::cout
          << "Session RECORDING mode enabled. Session traces will be saved to "
          << mysqlshdk::db::replay::g_recording_path_prefix << "\n";
      break;
    case mysqlshdk::db::replay::Mode::Replay:
      std::cout
          << "Session REPLAY mode enabled. Sessions will replay traces from "
          << mysqlshdk::db::replay::g_recording_path_prefix << "\n";
      break;
  }

  std::cout << "-=-\n";  // begin test marker fpr rebuild_traces
  int ret_val = RUN_ALL_TESTS();

  if (!g_skipped_tests.empty()) {
    std::cout << makeyellow("The following tests were SKIPPED:") << "\n";
    for (auto &t : g_skipped_tests) {
      std::cout << makeyellow("[  SKIPPED ]") << " " << t.first << "\n";
      std::cout << "\tNote: " << t.second << "\n";
    }
  }
  if (!g_pending_fixes.empty()) {
    std::cout << makeyellow("Tests for unfixed bugs:") << "\n";
    for (auto &t : g_pending_fixes) {
      std::cout << makeyellow("[  FIXME   ]") << " at " << t.first
                << "\n";
      std::cout << "\tNote: " << t.second << "\n";
    }
  }

#ifndef NDEBUG
  if (getenv("DEBUG_OBJ"))
    shcore::debug::debug_object_dump_report(false);
#endif

  return ret_val;
}
