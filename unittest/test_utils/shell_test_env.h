/*
 * Copyright (c) 2014, 2024, Oracle and/or its affiliates.
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

#ifndef UNITTEST_TEST_UTILS_SHELL_TEST_ENV_H_
#define UNITTEST_TEST_UTILS_SHELL_TEST_ENV_H_

// Environment variables only

#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/utils/version.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils/sandboxes.h"

extern "C" const char *g_argv0;
extern int g_test_trace_scripts;
extern bool g_test_fail_early;
extern int g_test_color_output;
extern mysqlshdk::db::replay::Mode g_test_recording_mode;

#define SKIP_UNLESS_DIRECT_MODE()                                   \
  if (g_test_recording_mode != mysqlshdk::db::replay::Mode::Direct) \
    SKIP_TEST("This only runs in --direct mode.");

#define ASSERT_THROW_LIKE(expr, exc, msg)                              \
  try {                                                                \
    expr;                                                              \
    FAIL() << "Expected exception of type " #exc << " but got none\n"; \
  } catch (exc & e) {                                                  \
    if (std::string(e.what()).find(msg) == std::string::npos) {        \
      FAIL() << "Expected exception with message: " << (msg)           \
             << "\nbut got: " << typeid(e).name() << e.what() << "\n"; \
    }                                                                  \
  }

#define EXPECT_THROW_LIKE(expr, exc, msg)                                   \
  try {                                                                     \
    expr;                                                                   \
    ADD_FAILURE() << "Expected exception of type " #exc                     \
                  << "\nwith message: " << msg << "\nbut got none\n";       \
  } catch (exc & e) {                                                       \
    if (std::string(e.what()).find(msg) == std::string::npos) {             \
      ADD_FAILURE() << "Expected exception with message: " << (msg)         \
                    << "\nbut got: " << e.what() << "\n";                   \
    }                                                                       \
  } catch (std::exception & e) {                                            \
    ADD_FAILURE() << "Expected exception of type " #exc                     \
                  << "\nwith message: " << (msg) << "\nbut got "            \
                  << typeid(e).name() << ": " << e.what() << "\n";          \
  } catch (...) {                                                           \
    ADD_FAILURE() << "Expected exception of type " #exc                     \
                  << "\n with message: " << (msg) << "\nbut got another\n"; \
  }

#define EXPECT_EXECUTE_CALL(obj, query) \
  EXPECT_CALL(obj, executes(StrEq(query), ::strlen(query)))

namespace tests {

/**
 * \ingroup UTFramework
 * \todo This class must be documented.
 */
class Override_row_string : public mysqlshdk::db::replay::Row_hook {
 public:
  Override_row_string(std::unique_ptr<mysqlshdk::db::IRow> source,
                      const std::vector<uint32_t> columns,
                      const std::vector<std::string> &values)
      : mysqlshdk::db::replay::Row_hook(std::move(source)),
        m_columns(columns),
        m_values(values) {
    assert(m_columns.size() == m_values.size());
  }

  std::string get_string(uint32_t index) const override {
    // if (index == _column) return _value;
    auto it = std::find(std::begin(m_columns), std::end(m_columns), index);
    if (it != std::end(m_columns)) {
      auto column_index = std::distance(std::begin(m_columns), it);
      return m_values.at(column_index);
    }
    return Row_hook::get_string(index);
  }

 private:
  std::vector<uint32_t> m_columns;
  std::vector<std::string> m_values;
};

/**
 * \ingroup UTFramework
 * Defines a base environment for unit tests.
 *
 * The attributes of this class are configured based on the environment where
 * the tests are executed, the values are available through all the test suite.
 */
class Shell_test_env : public ::testing::Test {
 public:
  Shell_test_env();

  virtual void SetUpOnce() {}

  static void SetUpTestCase();

  std::string setup_recorder(const char *sub_test_name = nullptr);
  void teardown_recorder();

  static void setup_env();
  static mysqlshdk::utils::Version get_target_server_version() {
    return _target_server_version;
  }

  std::string query_replace_hook(std::string_view sql);
  std::unique_ptr<mysqlshdk::db::IRow> set_replay_row_hook(
      const mysqlshdk::db::Connection_options &target, const std::string &sql,
      std::unique_ptr<mysqlshdk::db::IRow> source);

  virtual void debug_print(const std::string &s) {
    fprintf(stderr, "%s\n", s.c_str());
  }

  static bool check_min_version_skip_test(bool skip_test = true);

 protected:
  static std::string _host;  //!< localhost
  static std::string _port;  //!< The port for X protocol, env:MYSQLX_PORT
  static std::string _user;
  static std::string _pwd;
  static std::string s_hostname;  //!< TBD
  std::string m_hostname;
  static bool s_real_host_is_loopback;
  bool m_real_host_is_loopback;
  static std::string s_hostname_ip;  //!< TBD
  std::string m_hostname_ip;
  static std::string s_real_hostname;  //!< TBD
  std::string m_real_hostname;
  static std::string _uri;           //!< A full URI for X protocol sessions
  static std::string _uri_nopasswd;  //!< A password-less URI for X protocol
  static std::string
      _mysql_port;  //!< The port for MySQL protocol, env:MYSQL_PORT
  static std::string _mysql_uri;  //!< A full URI for MySQL protocol sessions
  static std::string
      _mysql_uri_nopasswd;  //!< A password-less URI for MySQL protocol sessions

  static std::string _socket;        //!< env:MYSQLX_SOCKET
  static std::string _mysql_socket;  //!< env:MYSQL_SOCKET

  static std::string _sandbox_dir;  //!< Path to the sandbox directory

  // these are per-test values, which can be either the same as _port and
  // _mysql_port or restored value from traces, when replaying
  std::string m_port;
  std::string m_mysql_port;

  // Overridden sandbox ports (used for replays)
  int _mysql_sandbox_ports[sandbox::k_num_ports];  //!< Ports

  static mysqlshdk::utils::Version
      _target_server_version;  //!< The version of the used MySQL Server
  static mysqlshdk::utils::Version _highest_tls_version;  //!< The highest TLS
                                                          //!< version supported
                                                          //!< by MySQL Server
 protected:
  std::string _test_context;  //!< Context for script validation engine

  std::string _current_entry_point;
  std::vector<std::string> _current_entry_point_stacktrace;
  bool _recording_enabled = false;
  bool _replaying = false;
  bool _recording = false;

  std::map<std::string, std::string>
      _output_tokens;  //!< Tokens for string resolution
  std::string resolve_string(std::string source);

  void SetUp() override;
  void TearDown() override;

  std::shared_ptr<mysqlshdk::db::mysql::Session> create_mysql_session(
      const std::string &uri = "");
  std::shared_ptr<mysqlshdk::db::mysqlx::Session> create_mysqlx_session(
      const std::string &uri = "");

 public:
  static std::string get_path_to_mysqlsh();
  static std::string get_path_to_test_dir(const std::string &file = "");

  std::string hostname() {
    return m_hostname.empty() ? s_hostname : m_hostname;
  }

  std::string hostname_ip() {
    return m_hostname_ip.empty() ? s_hostname_ip : m_hostname_ip;
  }

  std::string real_hostname() {
    return m_real_hostname.empty() ? s_real_hostname : m_real_hostname;
  }

  bool real_host_is_loopback() {
    return m_real_hostname.empty() ? s_real_host_is_loopback
                                   : m_real_host_is_loopback;
  }

  std::string mysql_sandbox_uri(int sbindex, const std::string &user = "root",
                                const std::string &pwd = "root");

  std::string mysqlx_sandbox_uri(int sbindex, const std::string &user = "root",
                                 const std::string &pwd = "root");

 private:
  struct Open_session {
    std::weak_ptr<mysqlshdk::db::ISession> session;
    std::string location;
    std::string stacktrace_at_open;
  };

  std::list<Open_session> _open_sessions;
  bool check_open_sessions();
  void on_session_connect(std::shared_ptr<mysqlshdk::db::ISession> session);
  void on_session_close(std::shared_ptr<mysqlshdk::db::ISession> session);
};
}  // namespace tests

// proto = [a]uto, [c]lassic, [x]
std::string shell_test_server_uri(int proto = 'a');

std::string random_string(std::string::size_type length);

void run_script_classic(const std::vector<std::string> &script);

// Execute the given script file from the URI. File is assumed to be in
// unittest/data/sql/
void run_test_data_sql_file(const std::string &uri,
                            const std::string &filename);

inline std::string makebold(const std::string &s) {
  if (!g_test_color_output) return s;
  return "\x1b[1m" + s + "\x1b[0m";
}

inline std::string makered(const std::string &s) {
  if (!g_test_color_output) return s;
  if (g_test_color_output == 2)
    return "\x1b[38;5;161m" + s + "\x1b[0m";
  else
    return "\x1b[31m" + s + "\x1b[0m";
}

inline std::string makelred(const std::string &s) {
  if (!g_test_color_output) return s;
  if (g_test_color_output == 2)
    return "\x1b[38;5;210m" + s + "\x1b[0m";
  else
    return "\x1b[91m" + s + "\x1b[0m";
}

inline std::string makeredbg(const std::string &s) {
  if (!g_test_color_output) return s;
  return "\x1b[41m" + s + "\x1b[0m";
}

inline std::string makeblue(const std::string &s) {
  if (!g_test_color_output) return s;
  return "\x1b[36m" + s + "\x1b[0m";
}

inline std::string makelblue(const std::string &s) {
  if (!g_test_color_output) return s;
  return "\x1b[94m" + s + "\x1b[0m";
}

inline std::string makegreen(const std::string &s) {
  if (!g_test_color_output) return s;
  return "\x1b[32m" + s + "\x1b[0m";
}

inline std::string makeyellow(const std::string &s) {
  if (!g_test_color_output) return s;
  return "\x1b[33m" + s + "\x1b[0m";
}

#endif  // UNITTEST_TEST_UTILS_SHELL_TEST_ENV_H_
