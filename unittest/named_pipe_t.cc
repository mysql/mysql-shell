/*
 * Copyright (c) 2018, 2022 Oracle and/or its affiliates.
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

#ifdef _WIN32

#include <string>
#include <vector>

#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "unittest/gtest_clean.h"
#include "unittest/test_utils/command_line_test.h"

namespace tests {

class Named_pipe_test : public Command_line_test {
 public:
  static void SetUpTestCase() {
    mysqlshdk::db::Connection_options options;
    options.set_host(_host);
    options.set_port(std::stoi(_mysql_port));
    options.set_user(_user);
    options.set_password(_pwd);

    auto session = mysqlshdk::db::mysql::Session::create();
    session->connect(options);

    auto result = session->query("show variables like 'named_pipe'");
    auto row = result->fetch_one();

    s_named_pipe_enabled = row && row->get_as_string(1) == "ON";

    if (s_named_pipe_enabled) {
      result = session->query("show variables like 'socket'");
      row = result->fetch_one();

      if (row) {
        s_named_pipe = row->get_as_string(1);
        s_is_default_named_pipe = shcore::str_caseeq(s_named_pipe, "MySQL");
      } else {
        s_named_pipe_enabled = false;
      }
    }

    session->close();
  }

 protected:
  static std::string user() { return "--user=" + _user; }

  static std::string pwd() { return "--password=" + _pwd; }

  static std::string host() { return "--host=."; }

  static std::string localhost() { return "--host=localhost"; }

  static std::string socket() { return "--socket=" + s_named_pipe; }

  static std::string uri_userinfo() { return _user + ":" + _pwd + "@"; }

  static std::string pipe() { return s_named_pipe; }

  static std::string encoded_pipe() {
    return shcore::str_replace(
        shcore::str_replace(shcore::str_replace(pipe(), ":", "%3A"), "\\",
                            "%5C"),
        "/", "%2F");
  }

  // Named pipe as listed in status() is limitted to 32 characters
  static std::string expected_pipe() { return s_named_pipe.substr(0, 32); }

  int status(const std::vector<std::string> &v) {
    return execute(v, "shell.status()");
  }

  int connect_and_status(const std::string &uri) {
    return execute({}, "shell.connect(\"" + uri + "\");shell.status()");
  }

  int connect_and_status(const std::string &host, const std::string &pipe) {
    return execute(
        {}, "shell.connect({\"user\":\"" + _user + "\",\"host\":\"" + host +
                "\",\"password\":\"" + _pwd + "\"" +
                (pipe.empty() ? "" : ",\"socket\":\"" + pipe + "\"") +
                "});shell.status()");
  }

  static bool s_named_pipe_enabled;
  static bool s_is_default_named_pipe;

 private:
  int execute(const std::vector<std::string> &v, const std::string &c) {
    std::vector<const char *> args;

    args.emplace_back(_mysqlsh);

    for (const auto &a : v) {
      args.emplace_back(a.c_str());
    }

    args.emplace_back("-e");
    args.emplace_back(c.c_str());
    args.emplace_back(nullptr);

    return Command_line_test::execute(args);
  }

  static std::string s_named_pipe;
};

bool Named_pipe_test::s_named_pipe_enabled = false;
bool Named_pipe_test::s_is_default_named_pipe = false;
std::string Named_pipe_test::s_named_pipe;

#define DEFAULT_PIPE_TEST(name, call)                                         \
  TEST_F(Named_pipe_test, name) {                                             \
    if (!s_named_pipe_enabled) {                                              \
      FAIL() << "Named Pipe connections are disabled, they must be enabled."; \
    } else {                                                                  \
      const auto code = call;                                                 \
                                                                              \
      if (s_is_default_named_pipe) {                                          \
        EXPECT_EQ(0, code);                                                   \
        MY_EXPECT_CMD_OUTPUT_CONTAINS(                                        \
            "Connection:                   Named pipe: " + expected_pipe());  \
        MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("Unix socket:");                    \
      } else {                                                                \
        EXPECT_EQ(1, code);                                                   \
        MY_EXPECT_CMD_OUTPUT_CONTAINS(                                        \
            "Can't open named pipe to host: .  pipe: MySQL");                 \
      }                                                                       \
    }                                                                         \
  }

#define PIPE_TEST(name, call)                                                 \
  TEST_F(Named_pipe_test, name) {                                             \
    if (!s_named_pipe_enabled) {                                              \
      FAIL() << "Named Pipe connections are disabled, they must be enabled."; \
    } else {                                                                  \
      const auto code = call;                                                 \
                                                                              \
      EXPECT_EQ(0, code);                                                     \
      MY_EXPECT_CMD_OUTPUT_CONTAINS(                                          \
          "Connection:                   Named pipe: " + expected_pipe());    \
      MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("Unix socket:");                      \
    }                                                                         \
  }

DEFAULT_PIPE_TEST(cmdline_default_named_pipe, status({user(), pwd(), host()}));
DEFAULT_PIPE_TEST(cmdline_default_named_pipe_uri,
                  status({uri_userinfo() + "."}));

PIPE_TEST(cmdline_socket, status({user(), pwd(), host(), socket()}));
PIPE_TEST(cmdline_localhost_socket,
          status({user(), pwd(), localhost(), socket()}));
PIPE_TEST(cmdline_uri_socket, status({uri_userinfo() + ".", socket()}));
PIPE_TEST(cmdline_uri, status({uri_userinfo() + "(\\\\.\\" + pipe() + ")"}));
PIPE_TEST(cmdline_encoded_uri,
          status({uri_userinfo() + "\\\\.\\" + encoded_pipe()}));

DEFAULT_PIPE_TEST(default_named_pipe_uri,
                  connect_and_status(uri_userinfo() + "."));
DEFAULT_PIPE_TEST(default_named_pipe_options, connect_and_status(".", ""));

PIPE_TEST(options_socket, connect_and_status(".", pipe()));
PIPE_TEST(options_localhost_socket, connect_and_status("localhost", pipe()));

PIPE_TEST(uri,
          connect_and_status(uri_userinfo() + "(\\\\\\\\.\\\\" + pipe() + ")"));
PIPE_TEST(encoded_uri, connect_and_status(uri_userinfo() + "\\\\\\\\.\\\\" +
                                          encoded_pipe()));

}  // namespace tests

#endif  // _WIN32
