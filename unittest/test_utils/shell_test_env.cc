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

#include "unittest/test_utils/shell_test_env.h"
#include <fstream>
#include <random>
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/db/uri_encoder.h"
#include "mysqlshdk/libs/utils/trandom.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/utils_net.h"

extern mysqlshdk::db::replay::Mode g_test_recording_mode;

namespace tests {

class My_random : public mysqlshdk::utils::Random {
 public:
  virtual std::string get_time_string() {
    return "000000000" + std::to_string(++ts);
  }

 private:
  int ts = 0;
};

#ifdef _WIN32
std::string Shell_test_env::_path_splitter = "\\";
#else
std::string Shell_test_env::_path_splitter = "/";
#endif

Shell_test_env::Shell_test_env() {
  const char *uri = getenv("MYSQL_URI");
  if (uri == NULL)
    throw std::runtime_error(
        "MYSQL_URI environment variable has to be defined for tests");

  // Creates connection data and recreates URI, this will fix URI if no
  // password is defined So the UT don't prompt for password ever
  auto data = shcore::get_connection_options(uri);

  _host = data.get_host();
  _user = data.get_user();

  const char *pwd = getenv("MYSQL_PWD");
  if (pwd) {
    _pwd.assign(pwd);
    data.set_password(_pwd);
  } else {
    data.set_password("");
  }

  _uri = data.as_uri(mysqlshdk::db::uri::formats::full());
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

  _hostname = getenv("MYSQL_HOSTNAME");
  _hostname_ip = mysqlshdk::utils::resolve_hostname_ipv4(_hostname);

  _mysql_uri_nopasswd = shcore::strip_password(_mysql_uri);

  const char *sandbox_port1 = getenv("MYSQL_SANDBOX_PORT1");
  if (sandbox_port1)
    _mysql_sandbox_port1.assign(sandbox_port1);
  else
    _mysql_sandbox_port1 = std::to_string(atoi(_mysql_port.c_str()) + 10);

  _mysql_sandbox_nport1 = std::stoi(_mysql_sandbox_port1);

  const char *sandbox_port2 = getenv("MYSQL_SANDBOX_PORT2");
  if (sandbox_port2)
    _mysql_sandbox_port2.assign(sandbox_port2);
  else
    _mysql_sandbox_port2 = std::to_string(atoi(_mysql_port.c_str()) + 20);

  _mysql_sandbox_nport2 = std::stoi(_mysql_sandbox_port2);

  const char *sandbox_port3 = getenv("MYSQL_SANDBOX_PORT3");
  if (sandbox_port3)
    _mysql_sandbox_port3.assign(sandbox_port3);
  else
    _mysql_sandbox_port3 = std::to_string(atoi(_mysql_port.c_str()) + 30);

  _mysql_sandbox_nport3 = std::stoi(_mysql_sandbox_port3);

  const char *tmpdir = getenv("TMPDIR");
  if (tmpdir) {
    _sandbox_dir.assign(tmpdir);
  } else {
    // If not specified, the tests will create the sandboxes on the
    // binary folder
    _sandbox_dir = shcore::get_binary_folder();
  }

  std::vector<std::string> path_components = {_sandbox_dir,
                                              _mysql_sandbox_port1, "my.cnf"};
  _sandbox_cnf_1 = shcore::str_join(path_components, _path_splitter);

  path_components[1] = _mysql_sandbox_port2;
  _sandbox_cnf_2 = shcore::str_join(path_components, _path_splitter);

  path_components[1] = _mysql_sandbox_port3;
  _sandbox_cnf_3 = shcore::str_join(path_components, _path_splitter);

  std::vector<std::string> backup_path = {_sandbox_dir + _path_splitter + "my",
                                          _mysql_sandbox_port1, "cnf"};

  _sandbox_cnf_1_bkp = shcore::str_join(backup_path, ".");

  backup_path[1] = _mysql_sandbox_port2;
  _sandbox_cnf_2_bkp = shcore::str_join(backup_path, ".");

  backup_path[1] = _mysql_sandbox_port3;
  _sandbox_cnf_3_bkp = shcore::str_join(backup_path, ".");
}

static bool g_initialized_test = false;

void Shell_test_env::SetUp() {
  if (!g_initialized_test) {
    SetUpOnce();
    g_initialized_test = true;
  }
}

void Shell_test_env::TearDown() {
  if (_recording_enabled) {
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
    mysqlshdk::db::replay::set_mode(mysqlshdk::db::replay::Mode::Direct);
    mysqlshdk::utils::Random::set(nullptr);
    mysqlshdk::db::replay::set_replay_row_hook({});
  }
}

void Shell_test_env::SetUpTestCase() {
  g_initialized_test = false;
}

std::string Shell_test_env::setup_recorder() {
  mysqlshdk::db::replay::set_mode(g_test_recording_mode);
  _recording_enabled = true;

  bool is_recording =
      (g_test_recording_mode == mysqlshdk::db::replay::Mode::Record);

  std::string tracedir;

  const ::testing::TestInfo *const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  std::string context;
  context.append(test_info->test_case_name());
  context.append("/");
  context.append(test_info->name());

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

  mysqlshdk::db::replay::set_recording_context(context);

  if (is_recording) {
    if (shcore::is_folder(mysqlshdk::db::replay::current_recording_dir()) &&
        !shcore::file_exists(mysqlshdk::db::replay::current_recording_dir() +
                             "/FAILED")) {
      ADD_FAILURE() << "Test running in record mode, but trace directory "
                       "already exists (and is not FAILED). Delete directory "
                       "to re-record it.\n";
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
  }

  if (g_test_recording_mode == mysqlshdk::db::replay::Mode::Replay) {
    // Some environmental or random data can change between recording and
    // replay time. Such data must be ensured to match between both.
    try {
      std::map<std::string, std::string> info =
          mysqlshdk::db::replay::load_test_case_info();

      // Override environment dependant data with the same values that were
      // used and saved during recording
      _mysql_sandbox_port1 = info["sandbox_port1"];
      _mysql_sandbox_nport1 = std::stoi(_mysql_sandbox_port1);
      _mysql_sandbox_port2 = info["sandbox_port2"];
      _mysql_sandbox_nport2 = std::stoi(_mysql_sandbox_port2);
      _mysql_sandbox_port3 = info["sandbox_port3"];
      _mysql_sandbox_nport3 = std::stoi(_mysql_sandbox_port3);

      _hostname = info["hostname"];
      _hostname_ip = info["hostname_ip"];
    } catch (std::exception &e) {
      // std::cout << e.what() << "\n";
      _mysql_sandbox_port1 = "3316";
      _mysql_sandbox_nport1 = 3316;
      _mysql_sandbox_port2 = "3326";
      _mysql_sandbox_nport2 = 3326;
      _mysql_sandbox_port3 = "3336";
      _mysql_sandbox_nport3 = 3336;
    }
  } else if (g_test_recording_mode == mysqlshdk::db::replay::Mode::Record) {
    std::map<std::string, std::string> info;

    info["sandbox_port1"] = _mysql_sandbox_port1;
    info["sandbox_port2"] = _mysql_sandbox_port2;
    info["sandbox_port3"] = _mysql_sandbox_port3;
    info["hostname"] = _hostname;
    info["hostname_ip"] = _hostname_ip;

    mysqlshdk::db::replay::save_test_case_info(info);
  }

  if (g_test_recording_mode != mysqlshdk::db::replay::Mode::Direct) {
    // Ensure randomly generated strings are not so random when
    // recording/replaying
    mysqlshdk::utils::Random::set(new My_random());
  }

  return tracedir;
}

std::string Shell_test_env::get_path_to_mysqlsh() {
  std::string command;

#ifdef _WIN32
  // For now, on windows the executable is expected to be on the same path as
  // the unit tests
  char buf[MAX_PATH];
  GetModuleFileNameA(NULL, buf, MAX_PATH);
  command = buf;
  command.resize(command.rfind('\\') + 1);
  command += "mysqlsh.exe";
#else
  std::string prefix = g_argv0;
  // strip unittest/run_unit_tests
  size_t pos = prefix.rfind('/');
  prefix = prefix.substr(0, pos);
  pos = prefix.rfind('/');
  if (pos == std::string::npos)
    prefix = ".";
  else
    prefix = prefix.substr(0, pos);

  command = prefix + "/mysqlsh";
#endif

  return command;
}

std::string Shell_test_env::resolve_string(const std::string &source) {
  std::string updated(source);

  size_t start;
  size_t end;

  start = updated.find("<<<");
  while (start != std::string::npos) {
    end = updated.find(">>>", start);

    std::string token = updated.substr(start + 3, end - start - 3);

    std::string value;
    // If the token was registered in C++ uses it
    if (_output_tokens.count(token)) {
      value = _output_tokens[token];
    }

    updated.replace(start, end - start + 3, value);

    start = updated.find("<<<");
  }

  return updated;
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
  while (length--)
    result += alphanum[dist(rng)];

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

std::string shell_test_server_uri(int proto) {
  const char *uri = getenv("MYSQL_URI");
  if (!uri)
    uri = "root@localhost";

  // Creates connection data and recreates URI, fixes URI if no pwd defined
  // So the UT don't prompt for password ever
  auto data = shcore::get_connection_options(uri);

  const char *pwd = getenv("MYSQL_PWD");
  if (pwd)
    data.set_password(pwd);
  else
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
      .append(MYSQLX_SOURCE_HOME)
      .append("/unittest/data/sql/")
      .append(filename);
  int rc = system(cmd.c_str());
  ASSERT_EQ(0, rc);
}
