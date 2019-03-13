/* Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.

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

#include "unittest/test_utils/shell_test_env.h"
#include <fstream>
#include <random>
#include <tuple>
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/db/replay/trace.h"
#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/trandom.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_stacktrace.h"
#include "mysqlshdk/libs/utils/utils_string.h"

extern mysqlshdk::db::replay::Mode g_test_recording_mode;
extern mysqlshdk::utils::Version g_target_server_version;
extern mysqlshdk::utils::Version g_highest_tls_version;
extern "C" const char *g_test_home;

namespace tests {
namespace {

class My_random : public mysqlshdk::utils::Random {
 public:
  virtual std::string get_time_string() {
    return "000000000" + std::to_string(++ts);
  }

 private:
  int ts = 0;
};

/**
 * Network utilities used by the unit tests.
 */
class Test_net_utilities : public mysqlshdk::utils::Net {
 public:
  ~Test_net_utilities() override { remove(); }

  /**
   * Injects this instance of network utilities, replacing the default
   * behaviour.
   */
  void inject(const std::string &hostname, const std::string &hostname_ip,
              const std::string &real_hostname, bool real_host_is_loopback) {
    m_hostname = hostname;
    m_hostname_ip = hostname_ip;
    m_real_hostname = real_hostname;
    m_real_host_is_loopback = real_host_is_loopback;
    set(this);
  }

  /**
   * Removes the injected instance, restoring the default behaviour.
   */
  void remove() {
    if (get() == this) set(nullptr);
  }

  void inject_port_check_result(const std::string &host, int port,
                                bool result) {
    m_injected_port_checks.emplace_back(host, port, result);
  }

 protected:
  std::string m_hostname;
  std::string m_hostname_ip;
  std::string m_real_hostname;
  bool m_real_host_is_loopback = false;
  mutable std::list<std::tuple<std::string, int, bool>> m_injected_port_checks;

  /**
   * Allows to resolve the hostname stored by the shell test environment.
   */
  std::vector<std::string> resolve_hostname_ipv4_all_impl(
      const std::string &name) const override {
    if (name == m_hostname || name == m_real_hostname)
      return {m_hostname_ip};
    else
      return Net::resolve_hostname_ipv4_all_impl(name);
  }

  bool is_local_address_impl(const std::string &address) const override {
    if (address == m_hostname || address == m_hostname_ip ||
        address == m_real_hostname)
      return true;
    else
      return Net::is_local_address_impl(address);
  }

  bool is_loopback_impl(const std::string &address) const override {
    if (address == m_real_hostname && m_real_host_is_loopback) return true;
    return Net::is_loopback_impl(address);
  }

  std::string get_hostname_impl() const override { return m_hostname; }

  bool is_port_listening_impl(const std::string &address,
                              int port) const override {
    if (m_injected_port_checks.empty()) {
      return Net::is_port_listening_impl(address, port);
    }

    std::string ex_addr;
    int ex_port;
    bool ex_result;
    std::tie(ex_addr, ex_port, ex_result) = m_injected_port_checks.front();
    m_injected_port_checks.pop_front();

    if (ex_addr != address || ex_port != port) {
      ADD_FAILURE() << "Unexpected port check for " << address << ":" << port
                    << "\nwas expecting " << ex_addr << ":" << ex_port << "\n";
      throw std::logic_error("Unexpected port check");
    }
    return ex_result;
  }
};

Test_net_utilities test_net_utilities;

}  // namespace

std::string Shell_test_env::_host;
std::string Shell_test_env::_port;
std::string Shell_test_env::_user;
std::string Shell_test_env::_pwd;
int Shell_test_env::_port_number;
std::string Shell_test_env::s_hostname;
bool Shell_test_env::s_real_host_is_loopback;
std::string Shell_test_env::s_real_hostname;
std::string Shell_test_env::s_hostname_ip;
std::string Shell_test_env::_uri;
std::string Shell_test_env::_uri_nopasswd;
std::string Shell_test_env::_mysql_port;
int Shell_test_env::_mysql_port_number;
std::string Shell_test_env::_mysql_uri;
std::string Shell_test_env::_mysql_uri_nopasswd;

std::string Shell_test_env::_socket;
std::string Shell_test_env::_mysql_socket;

std::string Shell_test_env::_sandbox_dir;

int Shell_test_env::_def_mysql_sandbox_port1 = 0;
int Shell_test_env::_def_mysql_sandbox_port2 = 0;
int Shell_test_env::_def_mysql_sandbox_port3 = 0;

mysqlshdk::utils::Version Shell_test_env::_target_server_version;
mysqlshdk::utils::Version Shell_test_env::_highest_tls_version;

void Shell_test_env::setup_env(int sandbox_port1, int sandbox_port2,
                               int sandbox_port3) {
  // All tests expect the following standard test server definitions
  // Tests that need something else should create a sandbox with the required
  // configurations
  _host = "localhost";
  _user = "root";
  _pwd = "";
  _uri = "root:@localhost";
  _mysql_uri = _uri;

  const char *xport = getenv("MYSQLX_PORT");
  if (xport) {
    _port_number = atoi(xport);
    _port.assign(xport);
    _uri += ":" + _port;
  }
  _uri_nopasswd = shcore::strip_password(_uri);

  const char *port = getenv("MYSQL_PORT");
  if (port) {
    _mysql_port_number = atoi(port);
    _mysql_port.assign(port);
    _mysql_uri += ":" + _mysql_port;
  }

  const char *xsock = getenv("MYSQLX_SOCKET");
  if (xsock) {
    _socket = xsock;
  }

  const char *sock = getenv("MYSQL_SOCKET");
  if (sock) {
    _mysql_socket = sock;
  }

  s_hostname = getenv("MYSQL_HOSTNAME");
  s_real_hostname = getenv("MYSQL_REAL_HOSTNAME");
  s_real_host_is_loopback = mysqlshdk::utils::Net::is_loopback(s_real_hostname);
  s_hostname_ip = mysqlshdk::utils::Net::resolve_hostname_ipv4(s_hostname);
  assert(!s_real_hostname.empty());

  _mysql_uri_nopasswd = shcore::strip_password(_mysql_uri);

  _def_mysql_sandbox_port1 = sandbox_port1;
  _def_mysql_sandbox_port2 = sandbox_port2;
  _def_mysql_sandbox_port3 = sandbox_port3;

  const char *tmpdir = getenv("TMPDIR");
  if (tmpdir) {
    _sandbox_dir.assign(tmpdir);
  } else {
    // If not specified, the tests will create the sandboxes on the
    // binary folder
    _sandbox_dir = shcore::get_binary_folder();
  }

  // Enabling test context for expectations, the default context is the server
  // version
  _target_server_version = g_target_server_version;
  _highest_tls_version = g_highest_tls_version;
}

/**
 * Initializes the variables that serve as base environment for the Unit Tests.
 *
 * The Unit Tests have the following requirements:
 * @li A MySQL Server setup and running.
 * @li PATH must include the path to the binary folder of the MySQL Server that
 * will be used for sandbox operations (if required).
 * @li A set of environment variables that configure the way the Unit Tests will
 * be executed.
 *
 * At least the following environment variables are required:
 * @li MYSQL_PORT The port where the base MySQL Server listens for connections
 * through the MySQL protocol.
 * @li MYSQLX_PORT The port where the base MySQL Server listens for connections
 * through the X protocol.
 *
 * @li MYSQL_SANDBOX_PORT1 The port to be used for the first sandbox on tests
 * that require it, if not specified it will be calculated as the value of
 * MYSQL_PORT + 10
 * @li MYSQL_SANDBOX_PORT2 The port to be used for the first sandbox on tests
 * that require it, if not specified it will be calculated as the value of
 * MYSQL_PORT + 20
 * @li MYSQL_SANDBOX_PORT3 The port to be used for the first sandbox on tests
 * that require it, if not specified it will be calculated as the value of
 * MYSQL_PORT + 30
 * @li TMPDIR The path that will be used to create sandboxes if required, if not
 * specified the path where run_unit_tests binary will be used.
 */
Shell_test_env::Shell_test_env() {
  _mysql_sandbox_port1 = _def_mysql_sandbox_port1;
  _mysql_sandbox_port2 = _def_mysql_sandbox_port2;
  _mysql_sandbox_port3 = _def_mysql_sandbox_port3;

  _test_context = _target_server_version.get_base();
}

std::string Shell_test_env::mysql_sandbox_uri1(const std::string &user,
                                               const std::string &pwd) {
  return "mysql://" + user + ":" + pwd +
         "@localhost:" + std::to_string(_mysql_sandbox_port1);
}

std::string Shell_test_env::mysql_sandbox_uri2(const std::string &user,
                                               const std::string &pwd) {
  return "mysql://" + user + ":" + pwd +
         "@localhost:" + std::to_string(_mysql_sandbox_port2);
}

std::string Shell_test_env::mysql_sandbox_uri3(const std::string &user,
                                               const std::string &pwd) {
  return "mysql://" + user + ":" + pwd +
         "@localhost:" + std::to_string(_mysql_sandbox_port3);
}

std::string Shell_test_env::mysqlx_sandbox_uri1(const std::string &user,
                                                const std::string &pwd) {
  return "mysqlx://" + user + ":" + pwd +
         "@localhost:" + std::to_string(_mysql_sandbox_port1 * 10);
}

std::string Shell_test_env::mysqlx_sandbox_uri2(const std::string &user,
                                                const std::string &pwd) {
  return "mysqlx://" + user + ":" + pwd +
         "@localhost:" + std::to_string(_mysql_sandbox_port2 * 10);
}

std::string Shell_test_env::mysqlx_sandbox_uri3(const std::string &user,
                                                const std::string &pwd) {
  return "mysqlx://" + user + ":" + pwd +
         "@localhost:" + std::to_string(_mysql_sandbox_port3 * 10);
}

static bool g_initialized_test = false;

void Shell_test_env::SetUp() {
  if (!g_initialized_test) {
    SetUpOnce();
    g_initialized_test = true;
  }
}

void Shell_test_env::TearDown() {
  // ensure colors don't leak into other tests
  mysqlshdk::textui::set_color_capability(mysqlshdk::textui::No_color);

  if (_recording_enabled) {
    const ::testing::TestInfo *const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    // Don't bother with checking session leaks if the test itself failed
    if (!test_info->result()->Failed() && !check_open_sessions()) {
      ADD_FAILURE() << "Leaky sessions detected\n";
    }

    teardown_recorder();

    mysqlshdk::utils::Random::set(nullptr);
  }
}

void Shell_test_env::SetUpTestCase() { g_initialized_test = false; }

std::string Shell_test_env::setup_recorder(const char *sub_test_name) {
  mysqlshdk::db::replay::set_mode(g_test_recording_mode, g_test_trace_sql);
  _recording_enabled = true;

  bool is_recording =
      (g_test_recording_mode == mysqlshdk::db::replay::Mode::Record);

  std::string tracedir;

  const ::testing::TestInfo *const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  std::string context;
  context.append(test_info->test_case_name());
  context.append("/");
  if (sub_test_name) {
    // test_info->name() can be in the form test/<index> if this is a
    // parameterized test. For such cases, we replace the index with the
    // given sub_test_name, so that we can have a stable test name that doesn't
    // change if the execution order changes
    context.append(shcore::path::dirname(test_info->name()) + "/" +
                   sub_test_name);
  } else {
    context.append(test_info->name());
  }

  tracedir = shcore::path::join_path(
      {mysqlshdk::db::replay::g_recording_path_prefix, context});
  context.append("/");

  if (g_test_recording_mode == mysqlshdk::db::replay::Mode::Replay &&
      !shcore::is_folder(tracedir)) {
    ADD_FAILURE()
        << "Test running in replay mode but trace directory is missing: "
        << tracedir << "\n";
    throw std::logic_error("Missing tracedir");
  }

  mysqlshdk::db::replay::begin_recording_context(context);

  // Reset per-test network environment vars. overriden below if replaying
  m_hostname.clear();
  m_hostname_ip.clear();
  m_real_hostname.clear();
  m_real_host_is_loopback = false;

  if (is_recording) {
    if (shcore::is_folder(mysqlshdk::db::replay::current_recording_dir()) &&
        !shcore::is_file(mysqlshdk::db::replay::current_recording_dir() +
                         "/FAILED")) {
      ADD_FAILURE() << "Test running in record mode, but trace directory "
                       "already exists (and is not FAILED). Delete directory "
                       "to re-record it: "
                    << mysqlshdk::db::replay::current_recording_dir() << "\n";
      throw std::logic_error("Tracedir already exists");
    }

    try {
      // Delete old data
      shcore::remove_directory(tracedir, true);
    } catch (...) {
    }
    shcore::create_directory(tracedir);
    shcore::create_file(
        mysqlshdk::db::replay::current_recording_dir() + "/FAILED", "");

    // Set up hooks for keeping track of opened sessions
    mysqlshdk::db::replay::on_recorder_connect_hook = std::bind(
        &Shell_test_env::on_session_connect, this, std::placeholders::_1);
    mysqlshdk::db::replay::on_recorder_close_hook = std::bind(
        &Shell_test_env::on_session_close, this, std::placeholders::_1);

    // Set up hook to replace (non-deterministic) queries.
    mysqlshdk::db::replay::on_recorder_query_replace_hook = std::bind(
        &Shell_test_env::query_replace_hook, this, std::placeholders::_1);
  }

  if (g_test_recording_mode == mysqlshdk::db::replay::Mode::Replay) {
    _replaying = true;
    // Some environmental or random data can change between recording and
    // replay time. Such data must be ensured to match between both.
    try {
      std::map<std::string, std::string> info =
          mysqlshdk::db::replay::load_test_case_info();

      // Note: these ports are static vars, we assume only one test will
      // be active at a time.

      // Override environment dependant data with the same values that were
      // used and saved during recording
      _mysql_sandbox_port1 = std::stoi(info["sandbox_port1"]);
      _mysql_sandbox_port2 = std::stoi(info["sandbox_port2"]);
      _mysql_sandbox_port3 = std::stoi(info["sandbox_port3"]);

      m_hostname = info["hostname"];
      m_hostname_ip = info["hostname_ip"];
      m_real_hostname = info["real_hostname"];
      m_real_host_is_loopback = info["real_host_is_loopback"] == "1";
    } catch (std::exception &e) {
      // std::cout << e.what() << "\n";
      _mysql_sandbox_port1 = 3316;
      _mysql_sandbox_port2 = 3326;
      _mysql_sandbox_port3 = 3336;
    }

    // Inject the recorded environment, so that tests that call things like
    // Net::is_loopback() will replay with the same environment as where
    // traces were recorded
    test_net_utilities.inject(m_hostname, m_hostname_ip, m_real_hostname,
                              m_real_host_is_loopback);
  } else if (g_test_recording_mode == mysqlshdk::db::replay::Mode::Record) {
    _recording = true;
    std::map<std::string, std::string> info;

    info["sandbox_port1"] = std::to_string(_mysql_sandbox_port1);
    info["sandbox_port2"] = std::to_string(_mysql_sandbox_port2);
    info["sandbox_port3"] = std::to_string(_mysql_sandbox_port3);
    info["hostname"] = s_hostname;
    info["hostname_ip"] = s_hostname_ip;
    info["real_hostname"] = s_real_hostname;
    info["real_host_is_loopback"] = s_real_host_is_loopback ? "1" : "0";

    mysqlshdk::db::replay::save_test_case_info(info);
  }

  if (g_test_recording_mode != mysqlshdk::db::replay::Mode::Direct) {
    // Ensure randomly generated strings are not so random when
    // recording/replaying
    mysqlshdk::utils::Random::set(new My_random());
  }

  return tracedir;
}

void Shell_test_env::teardown_recorder() {
  mysqlshdk::db::replay::on_recorder_connect_hook = {};
  mysqlshdk::db::replay::on_recorder_close_hook = {};

  if (g_test_recording_mode == mysqlshdk::db::replay::Mode::Record) {
    // If the test failed during recording mode, put a fail marker in the
    // trace dir
    const ::testing::TestInfo *const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    if (!test_info->result()->Failed()) {
      shcore::delete_file(mysqlshdk::db::replay::current_recording_dir() +
                          "/FAILED");
    }
  }

  test_net_utilities.remove();

  mysqlshdk::db::replay::end_recording_context();
}

/**
 * Returns the path to the mysqlsh binary being used on the tests.
 */
std::string Shell_test_env::get_path_to_mysqlsh() {
// MYSQLSH_HOME will be honored during execution of the tests
#ifdef _WIN32
  return shcore::path::join_path(shcore::get_mysqlx_home_path(), "bin",
                                 "mysqlsh.exe");
#else
  return shcore::path::join_path(shcore::get_mysqlx_home_path(), "bin",
                                 "mysqlsh");
#endif
}

std::string Shell_test_env::get_path_to_test_dir(const std::string &file) {
  if (file.empty()) return g_test_home;
  return shcore::path::join_path(g_test_home, file);
}

void Shell_test_env::inject_port_check_result(const std::string &host, int port,
                                              bool result) {
  test_net_utilities.inject_port_check_result(host, port, result);
}

size_t find_token(const std::string &source, const std::string &find,
                  const std::string &is_not, size_t start_pos) {
  size_t ret_val = std::string::npos;

  while (true) {
    size_t found = source.find(find, start_pos);
    size_t proto = source.find(is_not, start_pos);

    if (found != std::string::npos && found == proto)
      start_pos += proto + is_not.size();
    else {
      ret_val = found;
      break;
    }
  }

  return ret_val;
}

/**
 * Dynamically resolves strings based on predefined output tokens.
 *
 * Sometimes the expected strings do not contain fixed values, but contain
 * information that change depending on the environment being used to execute
 * the tests.
 *
 * It is possible to dynamically define a sort "variables" with thos values
 * that depend on the environment while executing the test, this is done by
 * adding such variable on the _output_tokens as:
 *
 * \code
 * _output_tokens[<variable>]=<value>
 * \endcode
 *
 * Those variables can be used on string expectations as follows:
 *
 * \code
 * This is <<<variable>>> defined at runtime.
 * \endcode
 *
 * If we assume that an output token was defined as:
 *
 * \code
 * _output_tokens["variable"] = "some value";
 * \endcode
 *
 * The string above will be converted to:
 *
 * \code
 * This is some value defined at runtime.
 * \endcode
 *
 * And the resolved string can then be used as the final expectation.
 */
std::string Shell_test_env::resolve_string(const std::string &source) {
  std::string updated(source);

  size_t start = find_token(updated, "<<<", "<<<< RECEIVE", 0);
  size_t end;

  while (start != std::string::npos) {
    end = find_token(updated, ">>>", ">>>> SEND", start);

    std::string token = updated.substr(start + 3, end - start - 3);

    std::string value;
    // If the token was registered in C++ uses it
    if (_output_tokens.count(token)) {
      value = _output_tokens[token];
    }

    updated.replace(start, end - start + 3, value);

    start = find_token(updated, "<<<", "<<<< RECEIVE", 0);
  }

  return updated;
}

bool Shell_test_env::check_open_sessions() {
  // check that there aren't any sessions still open
  size_t leaks = 0;
  for (const auto &s : _open_sessions) {
    if (auto session = s.session.lock()) {
      std::cerr << makered("Unclosed/leaked session at") << makeblue(s.location)
                << "\n";
      std::cerr << s.stacktrace_at_open << "\n\n";
      session->close();
      leaks++;
    }
  }
  if (leaks > 0) {
    std::cerr << makered("SESSION LEAK CHECK ERROR:") << " There are " << leaks
              << " sessions still open at the end of the test\n";
  }
  _open_sessions.clear();
  return leaks == 0;
}

void Shell_test_env::on_session_connect(
    std::shared_ptr<mysqlshdk::db::ISession> session) {
  // called by session recorder classes when connect is called
  // adds a weak ptr to the session object along with the stack trace
  // to a list of open sessions, which will be checked when the test finishes
  _open_sessions.push_back(
      {session, _current_entry_point,
       "\t" + shcore::str_join(mysqlshdk::utils::get_stacktrace(), "\n\t")});
}

void Shell_test_env::on_session_close(
    std::shared_ptr<mysqlshdk::db::ISession> session) {
  assert(session);
  // called by session recorder classes when close is called
  for (auto iter = _open_sessions.begin(); iter != _open_sessions.end();
       ++iter) {
    auto ptr = iter->session.lock();
    if (ptr && ptr.get() == session.get()) {
      _open_sessions.erase(iter);
      return;
    }
  }
  assert(0);
}

std::string Shell_test_env::query_replace_hook(const std::string &sql) {
  // Replace query to set the server_id in order to ignore its (random) value.
  // Otherwise, it will fail during replay because the random value will not
  // match the one recorded.
  if (shcore::str_beginswith(sql, "SET PERSIST_ONLY `server_id` = ")) {
    return "SET PERSIST_ONLY `server_id` = /*(*/" + sql.substr(31) + "/*)*/";
  } else {
    return sql;
  }
}

static int find_column_in_select_stmt(const std::string &sql,
                                      const std::string &column) {
  std::string s = shcore::str_lower(sql);
  // sanity checks for things we don't support
  assert(s.find(" from ") == std::string::npos);
  assert(s.find(" where ") == std::string::npos);
  assert(s.find("select ") == 0);
  assert(column.find(" ") == std::string::npos);
  // other not supported things...: ``, column names with special chars,
  // aliases, stuff in comments etc
  s = s.substr(strlen("select "));

  // trim out stuff inside parenthesis which can confuse the , splitter and
  // are not supported anyway, like:
  // select (select a, b from something), c
  // select concat(a, b), c
  std::string::size_type p = s.find("(");
  while (p != std::string::npos) {
    std::string::size_type pp = s.find(")", p);
    if (pp != std::string::npos) {
      s = s.substr(0, p + 1) + s.substr(pp);
    } else {
      break;
    }
    p = s.find("(", pp);
  }

  auto pos = s.find(";");
  if (pos != std::string::npos) s = s.substr(0, pos);

  std::vector<std::string> columns(shcore::str_split(s, ","));
  // the last column name can contain other stuff
  columns[columns.size() - 1] =
      shcore::str_split(shcore::str_strip(columns.back()), " \t\n\r").front();

  int i = 0;
  for (const auto &c : columns) {
    if (shcore::str_strip(c) == column) return i;
    ++i;
  }
  return -1;
}

std::unique_ptr<mysqlshdk::db::IRow> Shell_test_env::set_replay_row_hook(
    const mysqlshdk::db::Connection_options &target, const std::string &sql,
    std::unique_ptr<mysqlshdk::db::IRow> source) {
  int datadir_column = -1;

  if (sql.find("@@datadir") != std::string::npos &&
      shcore::str_ibeginswith(sql, "select ")) {
    // find the index for @@datadir in the query
    datadir_column = find_column_in_select_stmt(sql, "@@datadir");
    assert(datadir_column >= 0);
  } else {
    assert(sql.find("@@datadir") == std::string::npos);
  }

  // replace sandbox @@datadir from results with actual datadir
  if (datadir_column >= 0) {
    std::string prefix = shcore::path::dirname(
        shcore::path::dirname(source->get_string(datadir_column)));
    std::string suffix =
        source->get_string(datadir_column).substr(prefix.length() + 1);
    std::string datadir = shcore::path::join_path(_sandbox_dir, suffix);
#ifdef _WIN32
    datadir = shcore::str_replace(datadir, "/", "\\");
#endif
    return std::unique_ptr<mysqlshdk::db::IRow>{new tests::Override_row_string(
        std::move(source), std::vector<uint32_t>{(uint32_t)datadir_column},
        std::vector<std::string>{datadir})};
  }

#ifdef __sun
  return std::move(source);
#else
  return source;
#endif
}

std::shared_ptr<mysqlshdk::db::mysql::Session>
Shell_test_env::create_mysql_session(const std::string &uri) {
  mysqlshdk::db::Connection_options cnx_opt;
  if (uri.empty()) {
    cnx_opt.set_user("root");
    cnx_opt.set_password("");
    cnx_opt.set_host("127.0.0.1");
    cnx_opt.set_port(_mysql_port_number);
  } else {
    cnx_opt = mysqlshdk::db::Connection_options(uri);
  }
  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(cnx_opt);
  return session;
}

std::shared_ptr<mysqlshdk::db::mysqlx::Session>
Shell_test_env::create_mysqlx_session(const std::string &uri) {
  mysqlshdk::db::Connection_options cnx_opt;
  if (uri.empty()) {
    cnx_opt.set_user("root");
    cnx_opt.set_password("");
    cnx_opt.set_host("127.0.0.1");
    cnx_opt.set_port(_port_number);
  } else {
    cnx_opt = mysqlshdk::db::Connection_options(uri);
  }
  auto session = mysqlshdk::db::mysqlx::Session::create();
  session->connect(cnx_opt);
  return session;
}

}  // namespace tests

std::string random_string(std::string::size_type length) {
  std::string alphanum =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  std::random_device seed;
  std::mt19937 rng{seed()};
  std::uniform_int_distribution<std::string::size_type> dist(
      0, alphanum.size() - 1);

  std::string result;
  result.reserve(length);
  while (length--) result += alphanum[dist(rng)];

  return result;
}

void run_script_classic(const std::vector<std::string> &sql) {
  auto session(mysqlshdk::db::mysql::Session::create());

  session->connect(
      mysqlshdk::db::Connection_options(shell_test_server_uri('c')));

  for (const auto &s : sql) {
    try {
      session->execute(s);
    } catch (std::exception &e) {
      std::cerr << "EXCEPTION DURING SETUP: " << e.what() << "\n";
      std::cerr << "QUERY: " << s << "\n";
      throw;
    }
  }
  session->close();
}

/**
 * Returns a URI string for the given protocol.
 * @param proto Identifies the protocol for which the URI is required.
 *
 * This function create a URI for the given protocol, use either 'x' or 'c' for
 * the protocol type.
 *
 * The URI is created reading MYSQL_URI and the corresponding MYSQL_PORT or
 * MYSQLX_PORT environment variables.
 *
 * I MYSQL_URI is not defined, it will use 'root@localhost'.
 */
std::string shell_test_server_uri(int proto) {
  const char *uri = "root@localhost";

  // Creates connection data and recreates URI, fixes URI if no pwd defined
  // So the UT don't prompt for password ever
  auto data = shcore::get_connection_options(uri);
  data.set_password("");

  std::string _uri;
  _uri = mysqlshdk::db::uri::Uri_encoder().encode_uri(
      data, mysqlshdk::db::uri::formats::full());

  if (proto == 'x') {
    const char *xport = getenv("MYSQLX_PORT");
    if (xport) {
      _uri.append(":").append(xport);
    }
  } else if (proto == 'c') {
    const char *port = getenv("MYSQL_PORT");
    if (port) {
      _uri.append(":").append(port);
    }
  }
  return _uri;
}

void run_test_data_sql_file(const std::string &uri,
                            const std::string &filename) {
  std::string cmd = tests::Shell_test_env::get_path_to_mysqlsh();
  cmd.append(" ").append(uri);
  cmd.append(" --sql -f ")
      .append(shcore::path::join_path(g_test_home, "data", "sql", filename));
  int rc = system(cmd.c_str());
  ASSERT_EQ(0, rc);
}
