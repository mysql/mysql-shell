/*
 * Copyright (c) 2015, 2024, Oracle and/or its affiliates.
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

#ifdef HAVE_PYTHON
#include "mysqlshdk/include/scripting/python_context.h"
#include "mysqlshdk/include/scripting/python_utils.h"
#endif

#ifdef _WIN32
#include <Windows.h>
#else
#include <signal.h>
#endif

#include <my_alloc.h>
#include <my_default.h>
#include <mysql.h>
#include <stdlib.h>
#include <clocale>
#include <fstream>
#include <iostream>
#include <set>

#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_stacktrace.h"
#include "shellcore/interrupt_handler.h"
#include "shellcore/shell_init.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mod_testutils.h"
#include "utils/utils_path.h"
#include "utils/utils_process.h"
#include "utils/utils_string.h"

// Begin test configuration block

// Default execution mode for replayable tests
mysqlshdk::db::replay::Mode g_test_recording_mode =
    mysqlshdk::db::replay::Mode::Replay;

bool g_generate_validation_file = false;
bool g_test_parallel_execution = false;

int g_test_trace_scripts = 0;
int g_test_default_verbosity = 0;
bool g_test_fail_early = false;
int g_test_color_output = 0;
// timeout for execution of a single statement of a test script (seconds)
int g_test_script_timeout = 10 * 60;
bool g_bp = false;
std::set<int> g_break;

// Default trace set (MySQL version) to be used for replay mode
mysqlshdk::utils::Version g_target_server_version{"8.0.16"};
// Highest supported TLS version by MySQL Server
mysqlshdk::utils::Version g_highest_server_tls_version{};
// Highest common (server<->client) TLS supported version
mysqlshdk::utils::Version g_highest_tls_version{};

// End test configuration block

extern "C" {
const char *g_test_home = nullptr;
}
const char *g_mysqlsh_path;
const char *g_mysqld_path_variables;
int g_profile_test_scripts = 0;

std::vector<std::pair<std::string, std::string>> g_skipped_tests;
std::vector<std::pair<std::string, std::string>> g_skipped_chunks;
std::vector<std::pair<std::string, std::string>> g_skipped_validations;
std::vector<std::pair<std::string, std::string>> g_pending_fixes;

namespace {

struct Target_server {
  std::string path;
  mysqlshdk::utils::Version version;
};

const char *k_default_test_filter = "*";
constexpr int k_wait_sandbox_shutdown = 60;

std::string make_socket_absolute_path(const std::string &datadir,
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

void detect_mysql_environment(int port, const char *pwd) {
  std::string socket, xsocket, datadir;
  std::string hostname, report_host;
  int xport = 0;
  int server_id = 0;
  bool have_ssl = false;
  bool have_openssl = false;
  MYSQL *mysql;
  mysql = mysql_init(nullptr);
  unsigned int tcp = MYSQL_PROTOCOL_TCP;
  mysql_options(mysql, MYSQL_OPT_PROTOCOL, &tcp);

  if (const auto dir = shcore::get_default_mysql_plugin_dir(); !dir.empty()) {
    mysql_options(mysql, MYSQL_PLUGIN_DIR, dir.c_str());
  }

  // if connect succeeds or error is a server error, then there's a server
  if (mysql_real_connect(mysql, "127.0.0.1", "root", pwd, NULL, port, NULL,
                         0)) {
    {
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
      const char *query = "select @@hostname, @@report_host";
      if (mysql_real_query(mysql, query, strlen(query)) == 0) {
        MYSQL_RES *res = mysql_store_result(mysql);
        if (MYSQL_ROW row = mysql_fetch_row(res)) {
          hostname = row[0];
          report_host = row[1] ? row[1] : "";
        }
        mysql_free_result(res);
      }
    }

    {
      const char *query = "select @@version, @@server_id";
      if (mysql_real_query(mysql, query, strlen(query)) == 0) {
        MYSQL_RES *res = mysql_store_result(mysql);
        if (MYSQL_ROW row = mysql_fetch_row(res)) {
          g_target_server_version = mysqlshdk::utils::Version(row[0]);
          server_id = atoi(row[1]);
        }
        mysql_free_result(res);
      }
    }

    if (g_target_server_version < mysqlshdk::utils::Version(8, 0, 24)) {
      const char *query = "select @@have_ssl = 'YES', @@have_openssl = 'YES'";
      if (mysql_real_query(mysql, query, strlen(query)) == 0) {
        MYSQL_RES *res = mysql_store_result(mysql);
        if (MYSQL_ROW row = mysql_fetch_row(res)) {
          if (row[0] && strcmp(row[0], "1") == 0) have_ssl = true;
          if (row[1] && strcmp(row[1], "1") == 0) have_openssl = true;
        }
        mysql_free_result(res);
      }
    } else {
      have_ssl = true;
      have_openssl = true;
    }

    {
      const char *query = "select @@mysqlx_port";
      if (mysql_real_query(mysql, query, strlen(query)) == 0) {
        MYSQL_RES *res = mysql_store_result(mysql);
        if (MYSQL_ROW row = mysql_fetch_row(res)) {
          if (row[0]) xport = atoi(row[0]);
        }
        mysql_free_result(res);
      }
    }

    // highest server tls version
    {
      char const *const query = "SELECT @@tls_version";
      if (mysql_real_query(mysql, query, strlen(query)) == 0) {
        MYSQL_RES *res = mysql_store_result(mysql);
        if (MYSQL_ROW row = mysql_fetch_row(res)) {
          auto tls_versions = shcore::str_split(row[0], ",");
          // we assume that highest tls version string is last
          for (auto i = tls_versions.crbegin(); i != tls_versions.crend();
               i++) {
            if (shcore::str_beginswith(tls_versions.back(), "TLSv")) {
              std::string ver((*i).begin() + 4, (*i).end());
              g_highest_server_tls_version = g_highest_tls_version =
                  mysqlshdk::utils::Version(ver);
              break;
            }
          }
        }
        mysql_free_result(res);
      }
    }

    // highest client tls version. Might be empty if user do not use TLS
    // connection.
    // Update common (server<->user) highest supported TLS version.
    {
      char const *const query = "show status like 'Ssl_version'";
      if (mysql_real_query(mysql, query, strlen(query)) == 0) {
        MYSQL_RES *res = mysql_store_result(mysql);
        if (MYSQL_ROW row = mysql_fetch_row(res)) {
          const std::string tls_version = row[1];
          if (shcore::str_beginswith(tls_version, "TLSv")) {
            std::string ver(tls_version.begin() + 4, tls_version.end());
            auto client_tls_version = mysqlshdk::utils::Version(ver);
            g_highest_tls_version =
                std::min(client_tls_version, g_highest_server_tls_version);
          }
        }
        mysql_free_result(res);
      }
    }

  } else {
    std::cerr << "Cannot connect to MySQL server at " << port << ": "
              << mysql_error(mysql) << "\n";
    exit(1);
  }
  mysql_close(mysql);

  if (!xport && g_target_server_version >= mysqlshdk::utils::Version(5, 7, 0)) {
    std::cerr << "Could not query mysqlx_port. X plugin not installed?\n";
    exit(1);
  }

  const std::string socket_absolute =
      make_socket_absolute_path(datadir, socket);
  const std::string xsocket_absolute =
      make_socket_absolute_path(datadir, xsocket);

  std::string hostname_ip;
  try {
    hostname_ip = mysqlshdk::utils::Net::resolve_hostname_ipv4(hostname);
  } catch (std::exception &e) {
    std::cerr << "Error resolving hostname of target server: " << e.what()
              << "\n";
    exit(1);
  }

  std::cout << "Target MySQL server:\n";
  std::cout << "version=" << g_target_server_version.get_full() << "\n";
  std::cout << "hostname=" << hostname << ", ip=" << hostname_ip << "\n";
  std::cout << "report_host=" << report_host << "\n";
  std::cout << "server_id=" << server_id << ", ssl=" << have_ssl
            << ", openssl=" << have_openssl << ", highest_server_tls_version="
            << g_highest_server_tls_version.get_full()
            << ", highest_common_tls_version="
            << g_highest_tls_version.get_full() << "\n";

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

  {
    if (!shcore::setenv("MYSQL_SOCKET", socket_absolute)) {
      std::cerr << "MYSQL_SOCKET putenv failed to set it\n";
      exit(1);
    }
  }

  {
    // This environment variable makes libmysqlclient override the default
    // compiled-in socket path with the actual path in use
    shcore::setenv("MYSQL_UNIX_PORT", socket_absolute);
  }
  {
    // This environment variable makes libmysqlclient override the default
    // compiled-in TCP port with the actual one in use
    if (!shcore::setenv("MYSQL_TCP_PORT", std::to_string(port))) {
      std::cerr << "MYSQL_TCP_PORT putenv failed to set it\n";
      exit(1);
    }
  }

  {
    // does not affect libmysqlx
    if (!shcore::setenv("MYSQLX_SOCKET", xsocket_absolute)) {
      std::cerr << "MYSQLX_SOCKET putenv failed to set it\n";
      exit(1);
    }
  }
  {
    // does not affect libmysqlx
    if (!shcore::setenv("MYSQLX_PORT", std::to_string(xport))) {
      std::cerr << "MYSQLX_PORT putenv failed to set it\n";
      exit(1);
    }
  }

  if (getenv("PARATRACE")) {
    g_test_parallel_execution = true;
  }

  // MYSQL_HOSTNAME corresponds to whatever is returned by gethostbyname,
  // although it's fetched from @@hostname in the test MySQL server.
  // In some systems (Debian, Ubuntu) it resolves to 127.0.1.1, which is
  // unusable for setting up GR. In these hosts, MYSQL_HOSTNAME must be set
  // to an address that can be reached from outside.
  // OTOH MYSQL_REAL_HOSTNAME will always have @@hostname
  if (!getenv("MYSQL_HOSTNAME")) {
    if (!shcore::setenv("MYSQL_HOSTNAME", hostname)) {
      std::cerr << "MYSQL_HOSTNAME could not be set\n";
      exit(1);
    }
  }

  {
    if (!shcore::setenv("MYSQL_REAL_HOSTNAME", hostname)) {
      std::cerr << "MYSQL_REAL_HOSTNAME could not be set\n";
      exit(1);
    }
  }
}

std::string verify_target_server(
    const std::string &variable,
    const mysqlshdk::utils::Version &expected_version,
    std::vector<Target_server> &path_variables) {
  const char *path = getenv(variable.c_str());

  if (!path) {
    return shcore::str_format(
        "The environment variable %s must be defined with the path to the "
        "MySQL Server %s binary",
        variable.c_str(), expected_version.get_base().c_str());
  } else {
    mysqlshdk::utils::Version actual_version(
        tests::Testutils::get_mysqld_version(path));
    // if patch version is not used, accept any patch version as long as major
    // and minor versions match
    if (expected_version == actual_version ||
        (expected_version.get_patch() == 0 &&
         expected_version.get_major() == actual_version.get_major() &&
         expected_version.get_minor() == actual_version.get_minor())) {
      path_variables.push_back(
          Target_server{std::move(variable), actual_version});
    } else {
      return shcore::str_format(
          "The environment variable %s must be defined with the path to the "
          "MySQL Server %s binary, it points to a MySQL Server %s",
          variable.c_str(), expected_version.get_base().c_str(),
          actual_version.get_base().c_str());
    }
  }
  return "";
}

/*
 * Verifies the target servers to be used on the test session.
 * returns the base server version.
 */
std::string verify_target_servers(const std::string &target,
                                  std::string &path_variables) {
  std::vector<Target_server> target_server_list;
  std::string new_target;
  if (target.empty()) {
    new_target = g_target_server_version.get_base();
  } else {
    auto versions = shcore::str_split(target, "+");

    // The base server version is the first one
    new_target = versions[0];

    // The rest must have their paths defined in MYSQLD<M><m>[r]_PATH env
    // variables
    auto count = versions.size();
    std::vector<std::string> errors;
    // if just 2 versions are given, a MYSQLDMm_PATH env var should be required
    // with the specific server version (only if the major version differs from
    // the major version of the base server)
    if (count == 2) {
      mysqlshdk::utils::Version secondary(versions[1]);

      if (g_target_server_version.get_major() != secondary.get_major() ||
          g_target_server_version.get_minor() != secondary.get_minor()) {
        std::string variable = shcore::str_format(
            "MYSQLD%d%d_PATH", secondary.get_major(), secondary.get_minor());

        std::string error =
            verify_target_server(variable, secondary, target_server_list);
        if (!error.empty()) errors.push_back(error);
      }
    }

    // If no target variables yet and server count is more than 1 then specific
    // MYSQLDMmp_PATH variables should be defined
    if (count > 1 && target_server_list.empty() && errors.empty()) {
      for (size_t index = 1; index < versions.size(); index++) {
        mysqlshdk::utils::Version secondary(versions[index]);

        std::string variable =
            shcore::str_format("MYSQLD%d%d%d_PATH", secondary.get_major(),
                               secondary.get_minor(), secondary.get_patch());

        std::string error =
            verify_target_server(variable, secondary, target_server_list);
        if (!error.empty()) errors.push_back(error);
      }
    }

    if (!errors.empty()) {
      std::cerr << "Please correct the following errors regarding the target "
                   "servers for the test execution:\n"
                << shcore::str_join(errors, "\n").c_str() << std::endl;
      exit(1);
    } else {
      for (const auto &server : target_server_list) {
        path_variables.append(server.version.get_full())
            .append(";")
            .append(server.path)
            .append(",");
      }
      if (!path_variables.empty()) {
        path_variables.erase(path_variables.size() - 1);
      }
    }
  }

  return new_target;
}

bool delete_sandbox(int port) {
  MYSQL *mysql;
  mysql = mysql_init(nullptr);

  unsigned int tcp = MYSQL_PROTOCOL_TCP;
  mysql_options(mysql, MYSQL_OPT_PROTOCOL, &tcp);
  // if connect succeeds, attempt to shutdown the server with SHUTDOWN
  if (mysql_real_connect(mysql, "127.0.0.1", "root", "root", NULL, port, NULL,
                         0)) {
    std::cout << "Sandbox server running at " << port
              << ", shutting down and deleting\n";

    if (mysql_real_query(mysql, "shutdown", strlen("shutdown"))) {
      auto e = mysqlshdk::db::Error(mysql_error(mysql), mysql_errno(mysql),
                                    mysql_sqlstate(mysql));

      std::cout << "Error shutting down sandbox running on port " << port
                << ": " << e.what() << ": " << e.code() << "\n";

      mysql_close(mysql);
      return false;
    }
  } else if (mysql_errno(mysql) < 2000 || mysql_errno(mysql) >= 3000) {
    // If the connection fails with a server error, then there's a server
    // running but we cannot shut it down with SHUTDOWN
    std::cout << mysql_error(mysql) << "  " << mysql_errno(mysql) << "\n";
    std::cout << "Server already running on port " << port
              << " but can't shut it down\n";
    mysql_close(mysql);
    return false;
  }

  mysql_close(mysql);

  const char *tmpdir = getenv("TMPDIR");
  if (tmpdir == nullptr || strlen(tmpdir) == 0) {
    tmpdir = getenv("TEMP");  // TEMP usually used on Windows.
  }

  // Wait for the running server to complete the shutdown sequence

  std::cout << "Waiting for server running at " << port << " to shut down..."
            << std::endl;

  int retries = k_wait_sandbox_shutdown;

  const auto is_port_listening = [](const std::string &host, int tcp_port) {
    int ret = false;
    try {
      ret = mysqlshdk::utils::Net::is_port_listening(host, tcp_port);
    } catch (...) {
      // Ignore errors
    }
    return ret;
  };

  // Wait for the ports (classic, x and xcom) to be freed
  while (is_port_listening("127.0.0.1", port)) {
    if (--retries < 0) {
      std::cout << "Timeout waiting for port " << port << " to be free."
                << std::endl;
      return false;
    }
    shcore::sleep_ms(1000);
  }

  int x_port = port * 10;
  retries = k_wait_sandbox_shutdown;
  while (is_port_listening("127.0.0.1", x_port)) {
    if (--retries < 0) {
      std::cout << "Timeout waiting for port " << x_port << " to be free."
                << std::endl;
      return false;
    }
    shcore::sleep_ms(1000);
  }

  int xcom_port = port * 10 + 1;
  retries = k_wait_sandbox_shutdown;
  while (is_port_listening("127.0.0.1", xcom_port)) {
    if (--retries < 0) {
      std::cout << "Timeout waiting for port " << xcom_port << " to be free."
                << std::endl;
      return false;
    }
    shcore::sleep_ms(1000);
  }

  // Wait for the pid file to be deleted, meaning shutdown is complete
  if (tmpdir) {
#ifndef _WIN32
    // Wait for the lock files to be deleted too when not running in Windows
    retries = k_wait_sandbox_shutdown;
    std::string lock_file = shcore::path::join_path(
        tmpdir, std::to_string(port), "data", "mysqld.sock.lock");
    while (mysqlshdk::utils::check_lock_file(lock_file)) {
      if (--retries < 0) {
        std::cout << "Timeout waiting for sandbox lock file " << lock_file
                  << " to be deleted after shutdown." << std::endl;
        return false;
      }
      shcore::sleep_ms(1000);
    }

    retries = k_wait_sandbox_shutdown;
    std::string xlock_file = shcore::path::join_path(
        tmpdir, std::to_string(port), "data", "mysqlx.sock.lock");
    while (mysqlshdk::utils::check_lock_file(xlock_file, "X%zd")) {
      if (--retries < 0) {
        std::cout << "Timeout waiting for sandbox lock file " << xlock_file
                  << " to be deleted after shutdown." << std::endl;
        return false;
      }
      shcore::sleep_ms(1000);
    }

#endif
    retries = k_wait_sandbox_shutdown;
    std::string pidfile = shcore::path::join_path(
        tmpdir, std::to_string(port), std::to_string(port) + ".pid");

    while (shcore::path::exists(pidfile)) {
      if (--retries < 0) {
        std::cout << "Timeout waiting for sandbox pid file " << pidfile
                  << " to be deleted after shutdown." << std::endl;
        return false;
      }
      shcore::sleep_ms(1000);
    }

    std::string d;
    d = shcore::path::join_path(tmpdir, std::to_string(port));
    if (shcore::is_folder(d)) {
      try {
        shcore::remove_directory(d, true);
        std::cout << "Deleted leftover sandbox dir " << d << std::endl;
      } catch (std::exception &e) {
        std::cout << "Error deleting sandbox dir " << d << ": " << e.what()
                  << std::endl;
        return false;
      }
    }
  }
#ifndef _WIN32
  else {
    // If the tmpdir does not exist, it means it was removed before so we must
    // kill the process
    std::string s =
        "pkill -9 -f " +
        shcore::path::join_path(tmpdir, std::to_string(port), "bin", "mysqld");

    std::cout << "Terminating sandbox process\n";

    if (system(s.c_str()) != 0) {
      std::cout << "Error: unable to terminate sandbox process." << std::endl;
    }
  }
#endif

  return true;
}

void check_zombie_sandboxes(int sports[tests::sandbox::k_num_ports]) {
  bool have_zombies = false;
  std::string ports;

  for (int i = 0; i < tests::sandbox::k_num_ports; ++i) {
    have_zombies |= !delete_sandbox(sports[i]);
    if (!ports.empty()) ports.append(", ");
    ports.append(std::to_string(sports[i]));
  }

  if (have_zombies) {
    std::cout << "WARNING: mysqld running on port reserved for sandbox tests\n";
    std::cout << "Sandbox ports: " << ports << "\n";
    std::cout << "If they're left from a previous run, terminate them first\n";
    std::cout << "Or setenv TEST_SKIP_ZOMBIE_CHECK to skip this check\n";
    std::cout << "Or setenv MYSQL_SANDBOX_PORT1.."
              << tests::sandbox::k_num_ports + 1
              << " to pick different ports for "
                 "test sandboxes\n";
    exit(1);
  }
}

#ifndef _WIN32
void catch_segv(int sig) {
  mysqlshdk::utils::print_stacktrace();
  signal(sig, SIG_DFL);
  kill(getpid(), sig);
}
#endif

#ifdef __APPLE__
std::string get_test_keychain() {
  static constexpr auto k_keychain = "mysqlsh-test-keychain";
  return shcore::path::join_path(shcore::get_user_config_path(), k_keychain);
}

void setup_test_keychain() {
  const auto keychain = get_test_keychain();

  std::cout << "Using keychain: " << keychain << std::endl;
  std::cout << "Deleting old keychain (may fail): "
            << system(("security delete-keychain " + keychain).c_str())
            << std::endl;
  std::cout << "Creating keychain: "
            << system(("security create-keychain -p pass " + keychain).c_str())
            << std::endl;
  std::cout << "Disabling timeout: "
            << system(("security set-keychain-settings " + keychain).c_str())
            << std::endl;

  shcore::setenv("MYSQLSH_CREDENTIAL_STORE_KEYCHAIN", keychain);
}

void remove_test_keychain() {
  const auto keychain = get_test_keychain();
  std::cout << "Deleting test keychain: "
            << system(("security delete-keychain " + keychain).c_str())
            << std::endl;
}
#endif  // __APPLE__

}  // namespace
namespace testing {
class Fail_logger : public EmptyTestEventListener {
 public:
  Fail_logger() : EmptyTestEventListener() {
    m_pass_log_file.open("passed_tests", std::ofstream::app);
  }

  // Called before a test starts.
  void OnTestStart(const ::testing::TestInfo & /* test_info */) override {
    m_failed = false;
  }

  // Called after a failed assertion or a SUCCESS().
  void OnTestPartResult(
      const ::testing::TestPartResult &test_part_result) override {
    if (test_part_result.failed()) {
      m_failed = true;
    }
  }

  // Called after a test ends.
  void OnTestEnd(const ::testing::TestInfo &test_info) override {
    if (!m_failed) {
      m_pass_log_file << ":" << test_info.test_case_name() << "."
                      << test_info.name();
    }
  }

 private:
  std::ofstream m_pass_log_file;
  bool m_failed;
};
}  // namespace testing

static void setup_test_skipper() {
  std::string passed_tests;

  // Given the original filter, we want to only run the tests from that
  // set that either failed when running or didn't run at all.
  // Additionally, we don't want to run previously passed tests even when
  // a re-execution is interrupted.
  if (shcore::load_text_file("passed_tests", passed_tests) &&
      !passed_tests.empty()) {
    if (::testing::GTEST_FLAG(filter).find('-') == std::string::npos)
      ::testing::GTEST_FLAG(filter) += ":-" + passed_tests.substr(1);
    else
      ::testing::GTEST_FLAG(filter) += ":" + passed_tests.substr(1);
  }

  ::testing::TestEventListeners &listeners =
      ::testing::UnitTest::GetInstance()->listeners();
  // Adds a listener to the end.  googletest takes the ownership.
  listeners.Append(new testing::Fail_logger());
}

std::shared_ptr<shcore::Logger> setup_logger() {
  // Set HOME to same as TMPDIR
  {
    if (!shcore::setenv("HOME", getenv("TMPDIR"))) {
      std::cerr << "HOME could not be overriden\n";
      exit(1);
    }
  }

  if (!getenv("MYSQLSH_USER_CONFIG_HOME")) {
    // Override the configuration home for tests, to not mess with custom data
    if (!shcore::setenv("MYSQLSH_USER_CONFIG_HOME",
                        shcore::get_binary_folder())) {
      std::cerr << "MYSQLSH_USER_CONFIG_HOME could not be set with putenv\n";
    }
  }

  // Setup logger with default configs
  std::string log_path =
      shcore::path::join_path(shcore::get_user_config_path(), "mysqlsh.log");
  if (shcore::path_exists(log_path)) {
    std::cerr << "Deleting old " << log_path << " file\n";
    shcore::delete_file(log_path);
  }

  shcore::Logger::LOG_LEVEL log_level = shcore::Logger::LOG_LEVEL::LOG_INFO;
  const char *rut_log_level = getenv("RUT_LOG_LEVEL");
  if (rut_log_level) {
    try {
      log_level = shcore::Logger::parse_log_level(rut_log_level);
    } catch (const std::invalid_argument &err) {
      std::cerr << err.what() << std::endl;
      std::cerr << "Using default log level: info" << std::endl;
    }
  }

  return shcore::Logger::create_instance(log_path.c_str(), false, log_level);
}

void setup_test_environment() {
  if (!getenv("MYSQL_PORT")) {
    if (!shcore::setenv("MYSQL_PORT", "3306")) {
      std::cerr << "MYSQL_PORT was not set and putenv failed to set it\n";
      exit(1);
    }
  }

  detect_mysql_environment(atoi(getenv("MYSQL_PORT")), "");

  if (!getenv("MYSQL_REMOTE_HOST")) {
    char hostname[1024] = {0};
    if (gethostname(hostname, sizeof(hostname)) != 0) {
      std::cerr << "gethostname() returned error: " << strerror(errno) << "\n";
      std::cerr << "Set MYSQL_REMOTE_HOST\n";
      // exit(1); this option is not used for now, so no need to fail
    }
    if (!shcore::setenv("MYSQL_REMOTE_HOST", hostname)) {
      std::cerr
          << "MYSQL_REMOTE_HOST was not set and putenv failed to set it\n";
      // exit(1);
    }
  }

  if (!getenv("MYSQL_REMOTE_PORT")) {
    if (!shcore::setenv("MYSQL_REMOTE_PORT", "3306")) {
      std::cerr
          << "MYSQL_REMOTE_PORT was not set and putenv failed to set it\n";
      // exit(1);
    }
  }

  // Check TMPDIR environment variable for windows and other platforms without
  // TMPDIR defined.
  // NOTE: Required to be used as location for sandbox deployment.
  const char *tmpdir = getenv("TMPDIR");
  if (tmpdir == nullptr || strlen(tmpdir) == 0) {
    tmpdir = getenv("TEMP");  // TEMP usually used on Windows.
    std::string temp;
    if (tmpdir == nullptr || strlen(tmpdir) == 0) {
      // Use the binary folder as default for the TMPDIR.
      temp = shcore::get_binary_folder();
      std::cout << std::endl
                << "WARNING: TMPDIR environment variable is "
                   "empty or not defined. It will be set with the binary "
                   "folder path: "
                << temp << std::endl
                << std::endl;
    } else {
      temp = tmpdir;
    }
    if (!shcore::setenv("TMPDIR", temp)) {
      std::cerr << "TMPDIR was not set and putenv failed to set it\n";
      exit(1);
    }
  }

  tests::sandbox::initialize();

  // Check for leftover sandbox servers
  if (!getenv("TEST_SKIP_ZOMBIE_CHECK")) {
    check_zombie_sandboxes(tests::sandbox::k_ports);
  }
  tests::Shell_test_env::setup_env();

  tests::Testutils::validate_boilerplate(getenv("TMPDIR"));

  shcore::setenv("MYSQLSH_CREDENTIAL_STORE_HELPER", "<disabled>");

  if (!getenv("MYSQL_TEST_LOGIN_FILE")) {
    shcore::setenv("MYSQL_TEST_LOGIN_FILE",
                   shcore::path::join_path(getenv("TMPDIR"), ".mylogin.cnf"));
  }

#ifdef __APPLE__
  setup_test_keychain();
#endif  // __APPLE__

  // disable PAGER so it doesn't break the tests
  shcore::unsetenv("PAGER");

  // disable \edit-related variables
  shcore::unsetenv("EDITOR");
  shcore::unsetenv("VISUAL");
}

int main(int argc, char **argv) {
  // Has to be called once in main so internal static variable is properly set
  // with the main thread id.
  mysqlshdk::utils::in_main_thread();

  mysqlshdk::utils::init_stacktrace();
#ifdef WIN32
  UINT origcp = GetConsoleCP();
  UINT origocp = GetConsoleOutputCP();

  // Enable UTF-8 input and output
  SetConsoleCP(CP_UTF8);
  SetConsoleOutputCP(CP_UTF8);

  auto restore_cp = shcore::on_leave_scope([origcp, origocp]() {
    // Restore original codepage
    SetConsoleCP(origcp);
    SetConsoleOutputCP(origocp);
  });

  // init Winsock
  {
    WSADATA wsa;
    if (int err = WSAStartup(MAKEWORD(2, 2), &wsa); err != 0) {
      std::cerr << "Unable to load Winsock: " << err << "\n";
      exit(1);
    }
  }

#else
  auto locale = std::setlocale(LC_ALL, "en_US.UTF-8");
  if (!locale) {
    // logger is not yet initialized here
    std::cerr << "Failed to set LC_ALL locale to en_US.UTF-8: "
              << strerror(errno) << '\n';
  }
  shcore::setenv("LC_ALL", "en_US.UTF-8");
#endif

#if defined(WIN32)
  g_test_color_output = _isatty(_fileno(stdout));
#else
  g_test_color_output = isatty(STDOUT_FILENO) != 0;
  if (g_test_color_output && getenv("TERM") &&
      strcmp(getenv("TERM"), "xterm-256color") == 0) {
    g_test_color_output = 2;
  }
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

  mysqlsh::global_init();

  // Allows customizing the path where the test data files are, if customized
  // it should be a path that contains the next folders:
  // - scripts: with all the tests that go through the Shell_script_tester
  // - data: additional test files
  // - traces: tre recorded trace files
  // - prompt files
  // If not customized it will point to the unit test folder in the source code
  g_test_home = getenv("MYSQLSH_TEST_HOME");
  if (!g_test_home) {
    static std::string test_home =
        shcore::path::join_path(MYSQLX_SOURCE_HOME, "unittest");
    g_test_home = test_home.c_str();
  }

#ifdef __APPLE__
  struct rlimit rlp;
  getrlimit(RLIMIT_NOFILE, &rlp);
  if (rlp.rlim_cur < 10000) {
    std::cout << "Increasing open files limit FROM: " << rlp.rlim_cur;
    rlp.rlim_cur = 10000;
    setrlimit(RLIMIT_NOFILE, &rlp);
    getrlimit(RLIMIT_NOFILE, &rlp);
    std::cout << "  TO: " << rlp.rlim_cur << std::endl;
  }
#endif
  mysqlsh::Scoped_interrupt interrupt_handler(
      shcore::Interrupts::create(nullptr));

  // Disable colors for tests
  mysqlshdk::textui::set_color_capability(mysqlshdk::textui::No_color);

  // Reset these environment vars to start with a clean environment
  shcore::unsetenv("MYSQLSH_RECORDER_PREFIX");
  shcore::unsetenv("MYSQLSH_RECORDER_MODE");

  bool listing_tests = false;
  bool show_all_skipped = false;
  bool got_filter = false;
  bool tdb = false;
  bool only_failures = false;
  bool tdb_step = false;
  std::string tracedir;
  std::string target;

  for (int index = 1; index < argc; index++) {
    if (shcore::str_beginswith(argv[index], "--gtest_list_tests")) {
      listing_tests = true;
    } else if (shcore::str_beginswith(argv[index], "--gtest_filter")) {
      got_filter = true;
    } else if (shcore::str_beginswith(argv[index], "--direct")) {
      g_test_recording_mode = mysqlshdk::db::replay::Mode::Direct;
      char *p = strchr(argv[index], '=');
      if (p) {
        target = p + 1;
      }
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
      }
    } else if (shcore::str_beginswith(argv[index], "--tracedir")) {
      char *p = strchr(argv[index], '=');
      if (!p) {
        std::cerr << "--tracedir= option requires directory argument\n";
        exit(1);
      }
      tracedir = p + 1;
    } else if (shcore::str_caseeq(argv[index], "--generate-validation-file") ||
               strcmp(argv[index], "-g") == 0) {
      g_generate_validation_file = true;
    } else if (shcore::str_beginswith(argv[index], "--debug=")) {
      DBUG_SET_INITIAL(argv[index] + strlen("--debug="));
    } else if (strcmp(argv[index], "--trace-no-stop") == 0 ||
               strcmp(argv[index], "-T") == 0) {
      // continue executing script until the end on failure
      g_test_trace_scripts = 1;
    } else if (strcmp(argv[index], "--trace") == 0 ||
               strcmp(argv[index], "-t") == 0) {
      // stop executing script on failure
      g_test_trace_scripts = 2;
      g_test_fail_early = true;
    } else if (shcore::str_beginswith(argv[index], "--verbose=")) {
      g_test_default_verbosity = std::stod(strchr(argv[index], '=') + 1);
    } else if (strcmp(argv[index], "--verbose") == 0) {
      g_test_default_verbosity = 1;
    } else if (strcmp(argv[index], "--trace-sql") == 0) {
      DBUG_SET_INITIAL("+d,sql");
    } else if (strcmp(argv[index], "--trace-all-sql") == 0) {
      DBUG_SET_INITIAL("+d,sql,sqlall");
    } else if (strcmp(argv[index], "--stop-on-fail") == 0) {
      g_test_fail_early = true;
    } else if (strcmp(argv[index], "--tdb") == 0) {
      tdb = true;
      g_test_trace_scripts = 1;
      g_test_fail_early = true;
    } else if (strcmp(argv[index], "--tdb-step") == 0) {
      tdb = true;
      tdb_step = true;
      g_test_trace_scripts = 1;
      g_test_fail_early = true;
    } else if (shcore::str_beginswith(argv[index], "--break=")) {
      g_break.insert(std::stod(strchr(argv[index], '=') + 1));
    } else if (strcmp(argv[index], "--profile-scripts") == 0 ||
               strcmp(argv[index], "-p") == 0) {
      g_profile_test_scripts = 1;
    } else if (strcmp(argv[index], "-P") == 0) {
      g_profile_test_scripts = 2;
    } else if (shcore::str_beginswith(argv[index], "--timeout=")) {
      // timeout for script tests in seconds
      g_test_script_timeout = std::stod(strchr(argv[index], '=') + 1);
    } else if (strcmp(argv[index], "--show-skipped") == 0) {
      show_all_skipped = true;
    } else if (strcmp(argv[index], "--only-failures") == 0) {
      only_failures = true;
    } else if (strcmp(argv[index], "--gtest_color=yes") == 0) {
      g_test_color_output = 1;
    } else if (!shcore::str_beginswith(argv[index], "--gtest_") &&
               strcmp(argv[index], "--help") != 0) {
      std::cerr << "Invalid option " << argv[index] << "\n";
      exit(1);
    }
  }

  std::string mysqld_path_variables;
  mysqlsh::Scoped_ssh_manager ssh_manager(
      std::make_shared<mysqlshdk::ssh::Ssh_manager>());
  mysqlsh::Scoped_sql_processor sql_processor(
      std::make_shared<shcore::Sql_handler_registry>());
  mysqlsh::Scoped_logger sclogger(setup_logger());
  mysqlsh::Scoped_log_sql sclogsql(std::make_shared<shcore::Log_sql>());
  shcore::current_log_sql()->push("testmain");

  if (!listing_tests) {
    setup_test_environment();

    if (tdb) {
      extern void enable_tdb(bool);
      enable_tdb(tdb_step);
    }

    // The call to the verify_target_servers is needed so the path to the
    // different mysqld servers gets in place for every execution mode
    if (!target.empty())
      target = verify_target_servers(target, mysqld_path_variables);
    g_mysqld_path_variables = mysqld_path_variables.c_str();

    if (g_test_recording_mode != mysqlshdk::db::replay::Mode::Direct) {
      if (target.empty()) target = g_target_server_version.get_base();

      if (tracedir.empty())
        tracedir = shcore::path::join_path(g_test_home, "traces", target, "");
      mysqlshdk::db::replay::set_recording_path_prefix(tracedir);
    }

    if (g_test_recording_mode == mysqlshdk::db::replay::Mode::Record) {
      shcore::ensure_dir_exists(mysqlshdk::db::replay::g_recording_path_prefix);
    }
  }

  ::testing::InitGoogleTest(&argc, argv);

  std::string filter = ::testing::GTEST_FLAG(filter);
  std::string new_filter = filter;
  if (!got_filter) new_filter = k_default_test_filter;

  if (new_filter != filter) {
    std::cout << "Executing defined filter: " << new_filter.c_str()
              << std::endl;
    ::testing::GTEST_FLAG(filter) = new_filter.c_str();
  }

  std::string mysqlsh_path;
  // mysqlshrec is supposed to be in the same dir as run_unit_tests
  mysqlsh_path =
      shcore::path::join_path(shcore::get_binary_folder(), "mysqlshrec");
#ifdef _WIN32
  mysqlsh_path.append(".exe");
#endif

  g_mysqlsh_path = mysqlsh_path.c_str();

  if (!getenv("MYSQLSH_HOME"))
    std::cout << "Testing: Shell Build." << std::endl;
  else
    std::cout << "Testing: Shell Package." << std::endl;
  std::cout << "Shell Binary: " << g_mysqlsh_path << std::endl;
  std::cout << "Shell Home: " << shcore::get_mysqlx_home_path() << std::endl;
  std::cout << "Test Data Home: " << g_test_home << std::endl;
  if (!listing_tests) {
    std::cout << "Effective Hostname (external address of this host): "
              << getenv("MYSQL_HOSTNAME") << std::endl;
    std::cout << "Real Hostname (as returned by gethostbyname): "
              << getenv("MYSQL_REAL_HOSTNAME") << std::endl;
    if (mysqlshdk::utils::Net::is_loopback(getenv("MYSQL_REAL_HOSTNAME"))) {
      std::cout << "Note: " << getenv("MYSQL_REAL_HOSTNAME")
                << " resolves to a loopback\n";
      if (!mysqlshdk::utils::Net::is_externally_addressable(
              getenv("MYSQL_HOSTNAME"))) {
        std::cout
            << "Set the MYSQL_HOSTNAME to an externally addressable hostname "
               "or IP when executing AdminAPI tests in this host.\n";
      }
    } else {
      if (strcmp(getenv("MYSQL_HOSTNAME"), getenv("MYSQL_REAL_HOSTNAME")) !=
          0) {
        std::cout
            << "WARNING: " << getenv("MYSQL_REAL_HOSTNAME")
            << " does not resolve to a loopback but "
               "MYSQL_HOSTNAME and MYSQL_REAL_HOSTNAME have different values. "
               "You can leave MYSQL_HOSTNAME unset unless you're "
               "in a system where the default hostname "
               "is a loopback (like Ubuntu/Debian).\n";
      }
    }
  }
  {
    std::cout << "mycnf defaults: "
              << system((mysqlsh_path + " --print-defaults").c_str()) << "\n";
  }

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

  if (only_failures) setup_test_skipper();

  std::cout << "-=-\n";  // begin test marker fpr rebuild_traces
  int ret_val = RUN_ALL_TESTS();

  if (!g_skipped_tests.empty()) {
    std::cout << makeyellow("The following tests were SKIPPED:") << "\n";
    for (auto &t : g_skipped_tests) {
      std::cout << makeyellow("[  SKIPPED ]") << " " << t.first << "\n";
      std::cout << "\tNote: " << t.second << "\n";
    }
  }

  if (show_all_skipped) {
    if (!g_skipped_chunks.empty()) {
      std::cout << makeyellow("The following test chunks were SKIPPED:")
                << "\n";
      for (auto &t : g_skipped_chunks) {
        std::cout << makeyellow("[  SKIPPED ]") << " " << t.first << "\n";
        std::cout << "\tChunk: " << t.second << "\n";
      }
    }
    if (!g_skipped_validations.empty()) {
      std::cout << makeyellow(
                       "The following test chunk validations were SKIPPED:")
                << "\n";
      for (auto &t : g_skipped_validations) {
        std::cout << makeyellow("[  SKIPPED ]") << " " << t.first << "\n";
        std::cout << "\tValidation: " << t.second << "\n";
      }
    }
  } else {
    if (g_skipped_chunks.size() > 0 || g_skipped_validations.size() > 0) {
      std::cout << makeyellow(std::to_string(g_skipped_chunks.size()) +
                              " chunks and " +
                              std::to_string(g_skipped_validations.size()) +
                              " chunk validations were skipped. Use "
                              "--show-skipped to show them.")
                << "\n";
    }
  }
  if (!g_pending_fixes.empty()) {
    std::cout << makeyellow("Tests for unfixed bugs:") << "\n";
    for (auto &t : g_pending_fixes) {
      std::cout << makeyellow("[  FIXME   ]") << " at " << t.first << "\n";
      std::cout << "\tNote: " << t.second << "\n";
    }
  }
  {
    extern void fini_tdb();
    fini_tdb();
  }
#ifndef NDEBUG
  if (getenv("DEBUG_OBJ")) shcore::debug::debug_object_dump_report(false);
#endif

  // We need to ensure the callbacks registered in an SQL handler get released
  // before the python/js contexts get destroyed.
  sql_processor.get()->clear();

  mysqlsh::global_end();

#ifdef __APPLE__
  remove_test_keychain();
#endif  // __APPLE__

#ifdef HAVE_PYTHON
  shcore::Python_init_singleton::destroy_python();
#endif

#ifdef WIN32
  WSACleanup();
#endif

  return ret_val;
}
