/* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#ifdef HAVE_PYTHON
#include "mysqlshdk/include/scripting/python_utils.h"
#endif

#include <mysql.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "shellcore/interrupt_handler.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

#ifdef WIN32
#define putenv _putenv
#endif

extern "C" {
const char *g_argv0 = nullptr;
}
char *g_mppath = nullptr;

std::vector<std::pair<std::string, std::string> > g_skipped_tests;
std::vector<std::pair<std::string, std::string> > g_pending_fixes;

static std::pair<std::string, std::string> query_sockets(int port,
                                                         const char *pwd) {
  std::string socket, xsocket;
  MYSQL mysql;
  mysql_init(&mysql);
  unsigned int tcp = MYSQL_PROTOCOL_TCP;
  mysql_options(&mysql, MYSQL_OPT_PROTOCOL, &tcp);
  // if connect succeeds or error is a server error, then there's a server
  if (mysql_real_connect(&mysql, "localhost", "root", pwd, NULL, port, NULL,
                         0)) {
    const char *query = "show variables like '%socket'";
    if (mysql_real_query(&mysql, query, strlen(query)) == 0) {
      MYSQL_RES *res = mysql_store_result(&mysql);
      while (MYSQL_ROW row = mysql_fetch_row(res)) {
        if (strcmp(row[0], "socket") == 0 && row[1])
          socket = row[1];
        if (strcmp(row[0], "mysqlx_socket") == 0 && row[1])
          xsocket = row[1];
      }
      mysql_free_result(res);
    }
  }
  mysql_close(&mysql);
  return {socket, xsocket};
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

  MYSQL mysql;
  mysql_init(&mysql);
  unsigned int tcp = MYSQL_PROTOCOL_TCP;
  mysql_options(&mysql, MYSQL_OPT_PROTOCOL, &tcp);
  // if connect succeeds or error is a server error, then there's a server
  if (mysql_real_connect(&mysql, "localhost", "root", "", NULL, sport1, NULL,
                         0) ||
      mysql_errno(&mysql) < 2000 || mysql_errno(&mysql) >= 3000) {
    std::cout << mysql_error(&mysql) << "  " << mysql_errno(&mysql) << "\n";
    std::cout << "Server already running on port " << sport1 << "\n";
    have_zombies = true;
  }
  mysql_close(&mysql);
  mysql_init(&mysql);
  mysql_options(&mysql, MYSQL_OPT_PROTOCOL, &tcp);
  if (mysql_real_connect(&mysql, "localhost", "root", "", NULL, sport2, NULL,
                         0) ||
      mysql_errno(&mysql) < 2000 || mysql_errno(&mysql) >= 3000) {
    std::cout << mysql_error(&mysql) << "  " << mysql_errno(&mysql) << "\n";
    std::cout << "Server already running on port " << sport2 << "\n";
    have_zombies = true;
  }
  mysql_close(&mysql);
  mysql_init(&mysql);
  mysql_options(&mysql, MYSQL_OPT_PROTOCOL, &tcp);
  if (mysql_real_connect(&mysql, "localhost", "root", "", NULL, sport3, NULL,
                         0) ||
      mysql_errno(&mysql) < 2000 || mysql_errno(&mysql) >= 3000) {
    std::cout << mysql_error(&mysql) << "  " << mysql_errno(&mysql) << "\n";
    std::cout << "Server already running on port " << sport3 << "\n";
    have_zombies = true;
  }

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

  const char *tmpdir = getenv("TMPDIR");
  if (tmpdir) {
    std::string zombies;
    std::string d;
    d = tmpdir;
    d.append("/").append(std::to_string(sport1));
    if (shcore::file_exists(d)) {
      zombies.append(d).append("\n");
    }
    d = tmpdir;
    d.append("/").append(std::to_string(sport2));
    if (shcore::file_exists(d)) {
      zombies.append(d).append("\n");
    }
    d = tmpdir;
    d.append("/").append(std::to_string(sport3));
    if (shcore::file_exists(d)) {
      zombies.append(d).append("\n");
    }
    if (!zombies.empty()) {
      std::cout << "The following sandbox directories seem to be leftover and "
                   "must be deleted:\n";
      std::cout << zombies;
      exit(1);
    }
  }
}


int main(int argc, char **argv) {
  // Ignore broken pipe signal from broken connections
#ifndef _WIN32
  signal(SIGPIPE, SIG_IGN);
#endif

#ifdef __APPLE__
  struct rlimit rlp;
  getrlimit(RLIMIT_NOFILE, &rlp);
  if (rlp.rlim_cur < 10000) {
    std::cout << "Increasing open files limit" << std::endl;
    std::cout << "before: " << rlp.rlim_cur << " - " << rlp.rlim_max
              << std::endl;
    rlp.rlim_cur = 10000;
    setrlimit(RLIMIT_NOFILE, &rlp);
    getrlimit(RLIMIT_NOFILE, &rlp);
    std::cout << "after: " << rlp.rlim_cur << " - " << rlp.rlim_max
              << std::endl;
  }
#endif

  g_argv0 = argv[0];
#ifdef HAVE_V8
  extern void JScript_context_init();

  JScript_context_init();
#endif
  // init the ^C handler, so it knows what's the main thread
  shcore::Interrupts::init(nullptr);

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

  std::pair<std::string, std::string> sockets =
      query_sockets(atoi(getenv("MYSQL_PORT")), "");
  if (!getenv("MYSQL_SOCKET")) {
    static char path[1024];
    snprintf(path, sizeof(path), "MYSQL_SOCKET=%s", sockets.first.c_str());
    if (putenv(path) != 0) {
      std::cerr << "MYSQL_SOCKET was not set and putenv failed to set it\n";
      exit(1);
    }
  }
  if (!getenv("MYSQLX_SOCKET")) {
    static char path[1024];
    snprintf(path, sizeof(path), "MYSQLX_SOCKET=%s", sockets.second.c_str());
    if (putenv(path) != 0) {
      std::cerr << "MYSQLX_SOCKET was not set and putenv failed to set it\n";
      exit(1);
    }
  }

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
    std::cout << "Set default " << hostname << "\n";
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

// Override the configuration home for tests, to not mess with custom data
#ifdef WIN32
  _putenv_s("MYSQLSH_USER_CONFIG_HOME", ".");
#else
  setenv("MYSQLSH_USER_CONFIG_HOME", ".", 1);
#endif
  bool got_filter = false;

  for(auto index = 0; index < argc; index++) {
    if (shcore::str_beginswith(argv[index], "--gtest_filter"))
      got_filter = true;
  }

  ::testing::InitGoogleTest(&argc, argv);

  std::string filter = ::testing::GTEST_FLAG(filter);
  std::string new_filter = filter;

  if (!got_filter) {
    // We calculate the default test group to be executed as follows
    // On PB2, the next branches would execute the full test suite:
    // - mysql-shell
    // - mysql-shell-staging
    // - mysql-shell-<version>-release
    //
    // In Any other case, we would exclude SYSTEM tests from execution
    // If a specific filter is passed on the system call, we honor it
    const char *branch_name = getenv("BRANCH_NAME");

    new_filter = "NOSYSTEM";
    if (branch_name) {
      if ((strcmp(branch_name, "mysql-shell") == 0) ||
        (shcore::str_beginswith(branch_name, "mysql-shell") &&
        (shcore::str_endswith(branch_name, "release") ||
          shcore::str_endswith(branch_name, "staging")))) {
        std::cout << "Detected BRANCH_NAME=" << branch_name << ", executing "
          "the full test suite." << std::endl;
        new_filter = "*";
      } else {
        std::cout << "Detected BRANCH_NAME=" << branch_name << ", excluding "
          "the SYSTEM tests" << std::endl;
      }
    } else {
      std::cout << "Unable to detect BRANCH_NAME, excluding the SYSTEM "
        "tests." << std::endl;
    }
  } else {
    std::cout << "Executing defined filter "
      ": " << new_filter.c_str() << "." << std::endl;
  }


  // System tests include the JS System Tests only
  // Decision was made to remove the python tests which
  // Exercise exactly the same functionalify
  if (new_filter == "SYSTEM")
    new_filter = "Shell_js_dba_tests.*";

  // non system tests include everything except
  // PY/JS System Tests
  else if (new_filter == "NOSYSTEM")
    new_filter = "*:-Shell_py_dba_tests.*:Shell_js_dba_tests.*";

  if (new_filter != filter)
    ::testing::GTEST_FLAG(filter) = new_filter.c_str();

  const char *generate_option = "--generate_test_groups=";
  if (argc > 1 &&
      strncmp(argv[1], generate_option, strlen(generate_option)) == 0) {
    const char *path = strchr(argv[1], '=') + 1;
    std::ofstream f(path);

    std::cout << "Updating " << path << "...\n";
    f << "# Automatically generated, use make testgroups to update\n";

    ::testing::UnitTest *ut = ::testing::UnitTest::GetInstance();
    for (int i = 0; i < ut->total_test_case_count(); i++) {
      const char *name = ut->GetTestCase(i)->name();
      f << "add_test(" << name << " run_unit_tests --gtest_filter=" << name
        << ".*)\n";
    }
    return 0;
  }

  // Check for leftover sandbox servers
  if (!getenv("TEST_SKIP_ZOMBIE_CHECK")) {
    check_zombie_sandboxes();
  }
  std::string mppath;
  char *p = strrchr(argv[0], '/');
  if (p) {
    mppath = std::string(argv[0], p - argv[0]);
  } else {
    p = strrchr(argv[0], '\\');
    mppath = std::string(argv[0], p - argv[0]);
  }
#ifndef _WIN32
  // On linux, we need to tell the UTs where the mysqlprovision executable is
  mppath.append("/../mysqlprovision");
#else
  mppath.append("\\mysqlprovision.cmd");
#endif

  g_mppath = strdup(mppath.c_str());

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
      std::cout << makeyellow("[  FIXME   ]") << " at " << t.first << "\n";
      std::cout << "\tNote: " << t.second << "\n";
    }
  }

#ifndef NDEBUG
  if (getenv("DEBUG_OBJ"))
    shcore::debug::debug_object_dump_report(false);
#endif

  return ret_val;
}
