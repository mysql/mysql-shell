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

#include <gtest/gtest.h>
#include <algorithm>
#include "unittest/test_utils.h"
#include "unittest/test_utils/command_line_test.h"

class Mysqlsh_ssl : public tests::Command_line_test {
 public:
  static void SetUpTestCase() {
    // ensure that the default server has proper SSL settings

    // deploy sandbox instances with different SSL settings
  }

  static void TearDownTestCase() {
    // delete sandbox instances
  }

  enum class Ssl_config {
    Off,            // SSL disabled
    Tls_required,   // SSL enabled and with require_secure_transport
    Identity_good,  // SSL enabled with a proper host certificate
    Identity_bad    // SSL enabled but with wrong host certificate
  };

  void reconfigure_sandbox(Ssl_config sslconf) {
    // TODO(alfredo)
  }

  enum class Proto {
    X,       // --node explicit port
    C,       // --classic explicit port
    X_dflt,  // --node default port
    C_dflt,  // --classic default port
    X_auto,  // X protocol port but without --node
    C_auto   // classic port but without --classic
  };
  enum class Ssl { Dflt = -1, Disab = 0, Pref, Req, Ver_ca, Ver_id };

  enum class Srv { Main, Alt };

  enum class Usr { Root, SRoot };

  enum class Expect {
    Fail,     // Connection fail
    Ssl,      // OK_ssl
    Ssl_,     // OK_ssl or Fail_socket
    Tcp,      // OK_tcp
    Sok,      // OK_socket
    Sok_,     // OK_socket or Fail_socket
  };

  enum class Result {
    Fail,         // Connection failed
    Fail_socket,  // Connection failed because of socket path error
    OK_ssl,       // Connection OK, with ssl
    OK_socket,    // Connection OK, with socket
    OK_tcp,       // Connection OK, no ssl
    OK_err        // Connection OK, but got error in checks
  };

  struct Combination {
    Expect expected;

    Ssl ssl_mode;
    Proto protocol;
    Srv server;
    Usr account;
    std::vector<const char *> options;
  };

  Result test_connection(Proto protocol, Srv server, Usr account, Ssl ssl_mode,
                         const std::vector<const char *> &options,
                         std::string *out_cmd, std::string *out_output) {
    std::vector<const char *> argv;

    argv.push_back(_mysqlsh);
    argv.push_back("--sql");

    switch (protocol) {
      case Proto::X:
        argv.push_back("--node");
      case Proto::X_auto:
        switch (server) {
          case Srv::Main:
            argv.push_back("-P");
            argv.push_back(_port.c_str());
            break;
          case Srv::Alt:
            argv.push_back("-P");
            argv.push_back(_sandbox_xport.c_str());
            break;
        }
        break;
      case Proto::C:
        argv.push_back("--classic");
      case Proto::C_auto:
        switch (server) {
          case Srv::Main:
            argv.push_back("-P");
            argv.push_back(_mysql_port.c_str());
            break;
          case Srv::Alt:
            argv.push_back("-P");
            argv.push_back(_sandbox_port.c_str());
            break;
        }
        break;
      case Proto::X_dflt:
        argv.push_back("--node");
        break;
      case Proto::C_dflt:
        argv.push_back("--classic");
        break;
    }

    switch (account) {
      case Usr::Root:
        argv.push_back("-uroot");
        argv.push_back("--password=");
        break;
      case Usr::SRoot:
        argv.push_back("-urootssl");
        argv.push_back("--password=");
        break;
    }
    argv.push_back("localhost");
    switch (ssl_mode) {
      case Ssl::Dflt:
        break;
      case Ssl::Disab:
        argv.push_back("--ssl-mode=DISABLED");
        break;
      case Ssl::Pref:
        argv.push_back("--ssl-mode=PREFERRED");
        break;
      case Ssl::Req:
        argv.push_back("--ssl-mode=REQUIRED");
        break;
      case Ssl::Ver_ca:
        argv.push_back("--ssl-mode=VERIFY_CA");
        break;
      case Ssl::Ver_id:
        argv.push_back("--ssl-mode=VERIFY_IDENTITY");
        break;
    }
    std::copy(options.begin(), options.end(), std::back_inserter(argv));

    argv.push_back("-e");
    size_t cmd_index = argv.size();
    argv.push_back("");
    argv.push_back(nullptr);

    argv[cmd_index] =
        "select connection_type from performance_schema.threads"
        " where processlist_id = connection_id()";

    for (const char *s : argv) {
      if (s) {
        out_cmd->append(s).append(" ");
      }
    }

    bool test_debug = getenv("TEST_DEBUG") != nullptr;

    if (test_debug)
      std::cout << "execute: " << *out_cmd << "\n";

    // check if connection succeeds and whether SSL gets enabled
    int rc = execute(argv);
    if (test_debug)
      std::cout << _output << "\n\n";

    *out_output = _output;

    if (rc == 0) {
      Result result = Result::Fail;
      if (_output.find("TCP/IP") != std::string::npos)
        result = Result::OK_tcp;
      else if (_output.find("SSL/TLS") != std::string::npos)
        result = Result::OK_ssl;
      else if (_output.find("Socket") != std::string::npos)
        result = Result::OK_socket;

      // Perform some additional checks
      if (result != Result::Fail) {
        // query with small results
        argv[cmd_index] = "select 'hello'";
        _output.clear();
        rc = execute(argv);
        if (test_debug)
          std::cout << _output << "\n\n";
        if (rc != 0) {
          std::cout << "Small result test fail (rc):" << rc << "\t" << _output
                    << "\n";
          return Result::OK_err;
        }
        if (_output.find("ERROR") != std::string::npos) {
          std::cout << "Small result test fail (error):" << _output << "\n";
          return Result::OK_err;
        }
        if (_output.find("hello") == std::string::npos) {
          std::cout << "Small result test fail (content):" << _output << "\n";
          return Result::OK_err;
        }

        // artificial query with big results
        argv[cmd_index] = "select * from performance_schema.setup_instruments";
        _output.clear();
        rc = execute(argv);
        if (test_debug)
          std::cout << _output << "\n\n";
        if (rc != 0) {
          std::cout << "Big result test fail (rc):" << rc << "\t" << _output
                    << "\n";
          return Result::OK_err;
        }
        // regression test for bug#26431908
        if (_output.find("ERROR") != std::string::npos) {
          std::cout << "Big result test fail (ERROR):" << _output << "\n";
          return Result::OK_err;
        }
        if (_output.find("memory/mysqlx/send_buffer") == std::string::npos) {
          std::cout << "Big result test fail (content):" << _output << "\n";
          return Result::OK_err;
        }
        return result;
      }

    } else {
      _output = "exit_status = " + std::to_string(rc) + "\n" + _output;
      if (_output.find("server through socket") != std::string::npos)
        return Result::Fail_socket;
    }
    return Result::Fail;
  }

  std::string _sandbox_port;
  std::string _sandbox_xport;
};

void PrintTo(Mysqlsh_ssl::Expect r, ::std::ostream *os) {
  switch (r) {
    case Mysqlsh_ssl::Expect::Fail:
      *os << "Expect::Fail";
      break;
    case Mysqlsh_ssl::Expect::Ssl:
      *os << "Expect::Ssl";
      break;
    case Mysqlsh_ssl::Expect::Ssl_:
      *os << "Expect::Ssl_";
      break;
    case Mysqlsh_ssl::Expect::Sok:
      *os << "Expect::Sok";
      break;
    case Mysqlsh_ssl::Expect::Sok_:
      *os << "Expect::Sok_or fail";
      break;
    case Mysqlsh_ssl::Expect::Tcp:
      *os << "Expect::Tcp";
      break;
  }
}

void PrintTo(Mysqlsh_ssl::Result r, ::std::ostream *os) {
  switch (r) {
    case Mysqlsh_ssl::Result::Fail:
      *os << "Result::Fail";
      break;
    case Mysqlsh_ssl::Result::Fail_socket:
      *os << "Result::Fail_socket";
      break;
    case Mysqlsh_ssl::Result::OK_ssl:
      *os << "Result::OK_ssl";
      break;
    case Mysqlsh_ssl::Result::OK_tcp:
      *os << "Result::OK_tcp";
      break;
    case Mysqlsh_ssl::Result::OK_socket:
      *os << "Result::OK_socket";
      break;
    case Mysqlsh_ssl::Result::OK_err:
      *os << "Result::OK_err";
      break;
  }
}

void PrintTo(Mysqlsh_ssl::Proto r, ::std::ostream *os) {
  switch (r) {
    case Mysqlsh_ssl::Proto::X:
      *os << "Proto::X";
      break;
    case Mysqlsh_ssl::Proto::C:
      *os << "Proto::C";
      break;
    case Mysqlsh_ssl::Proto::X_dflt:
      *os << "Proto::X_dflt";
      break;
    case Mysqlsh_ssl::Proto::C_dflt:
      *os << "Proto::C_dflt";
      break;
    case Mysqlsh_ssl::Proto::X_auto:
      *os << "Proto::X_auto";
      break;
    case Mysqlsh_ssl::Proto::C_auto:
      *os << "Proto::C_auto";
      break;
  }
}

void PrintTo(Mysqlsh_ssl::Ssl r, ::std::ostream *os) {
  switch (r) {
    case Mysqlsh_ssl::Ssl::Dflt:
      *os << "Ssl::Dflt";
      break;
    case Mysqlsh_ssl::Ssl::Disab:
      *os << "Ssl::Disab";
      break;
    case Mysqlsh_ssl::Ssl::Pref:
      *os << "Ssl::Pref";
      break;
    case Mysqlsh_ssl::Ssl::Req:
      *os << "Ssl::Req";
      break;
    case Mysqlsh_ssl::Ssl::Ver_ca:
      *os << "Ssl::Ver_ca";
      break;
    case Mysqlsh_ssl::Ssl::Ver_id:
      *os << "Ssl::Ver_id";
      break;
  }
}

void PrintTo(Mysqlsh_ssl::Srv r, ::std::ostream *os) {
  switch (r) {
    case Mysqlsh_ssl::Srv::Main:
      *os << "Srv::Main";
      break;
    case Mysqlsh_ssl::Srv::Alt:
      *os << "Srv::Alt";
      break;
  }
}

void PrintTo(Mysqlsh_ssl::Usr r, ::std::ostream *os) {
  switch (r) {
    case Mysqlsh_ssl::Usr::Root:
      *os << "Usr::Root";
      break;
    case Mysqlsh_ssl::Usr::SRoot:
      *os << "Usr::SRoot";
      break;
  }
}

#define TRY_COMBINATIONS(combos)                                              \
  do {                                                                        \
    for (auto combo : combos) {                                               \
      Mysqlsh_ssl::Result result;                                             \
      std::string cmd;                                                        \
      std::string output;                                                     \
      std::stringstream ss;                                                   \
      PrintTo(combo.ssl_mode, &ss);                                           \
      ss << ", ";                                                             \
      PrintTo(combo.protocol, &ss);                                           \
      ss << ", ";                                                             \
      PrintTo(combo.server, &ss);                                             \
      ss << ", ";                                                             \
      PrintTo(combo.account, &ss);                                            \
      std::cout << ss.str() << "\n";                                          \
      result = test_connection(combo.protocol, combo.server, combo.account,   \
                               combo.ssl_mode, combo.options, &cmd, &output); \
      SCOPED_TRACE(ss.str() + ":\n\t" + cmd);                                 \
      switch (combo.expected) {                                               \
        case Mysqlsh_ssl::Expect::Fail:                                       \
          EXPECT_EQ(Mysqlsh_ssl::Result::Fail, result);                       \
          break;                                                              \
        case Mysqlsh_ssl::Expect::Ssl:                                        \
          EXPECT_EQ(Mysqlsh_ssl::Result::OK_ssl, result);                     \
          break;                                                              \
        case Mysqlsh_ssl::Expect::Ssl_:                                       \
          EXPECT_TRUE(Mysqlsh_ssl::Result::Fail_socket == result ||           \
                      Mysqlsh_ssl::Result::OK_ssl == result);                 \
          break;                                                              \
        case Mysqlsh_ssl::Expect::Tcp:                                        \
          EXPECT_EQ(Mysqlsh_ssl::Result::OK_tcp, result);                     \
          break;                                                              \
        case Mysqlsh_ssl::Expect::Sok:                                        \
          EXPECT_EQ(Mysqlsh_ssl::Result::OK_socket, result);                  \
          break;                                                              \
        case Mysqlsh_ssl::Expect::Sok_:                                       \
          EXPECT_TRUE(Mysqlsh_ssl::Result::Fail_socket == result ||           \
                      Mysqlsh_ssl::Result::OK_socket == result);              \
          break;                                                              \
      }                                                                       \
    }                                                                         \
  } while (0)

TEST_F(Mysqlsh_ssl, ssl_basic) {
  // Test basic SSL support:
  // ssl-mode: default, disabled, required, preferred
  // X and classic protocols
  // normal server with SSL
  // normal account

  std::vector<Combination> combos{
      {Expect::Ssl, Ssl::Dflt, Proto::X, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::C, Srv::Main, Usr::Root, {}},
#ifndef _WIN32
      //{Expect::Ssl, Ssl::Dflt, Proto::X_dflt, Srv::Main, Usr::Root, {}},
      // default connection method when port is not given is socket
      {Expect::Sok_, Ssl::Dflt, Proto::C_dflt, Srv::Main, Usr::Root, {}},
#endif
      {Expect::Ssl, Ssl::Dflt, Proto::X_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::C_auto, Srv::Main, Usr::Root, {}},
      {Expect::Tcp, Ssl::Disab, Proto::X, Srv::Main, Usr::Root, {}},
      {Expect::Tcp, Ssl::Disab, Proto::C, Srv::Main, Usr::Root, {}},
#ifndef _WIN32
      //{Expect::Tcp, Ssl::Disab, Proto::X_dflt, Srv::Main, Usr::Root, {}},
      {Expect::Sok_, Ssl::Disab, Proto::C_dflt, Srv::Main, Usr::Root, {}},
#endif
      {Expect::Tcp, Ssl::Disab, Proto::X_auto, Srv::Main, Usr::Root, {}},
      {Expect::Tcp, Ssl::Disab, Proto::C_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Pref, Proto::X, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Pref, Proto::C, Srv::Main, Usr::Root, {}},
#ifndef _WIN32
      //{Expect::Ssl, Ssl::Pref, Proto::X_dflt, Srv::Main, Usr::Root, {}},
      {Expect::Sok_, Ssl::Pref, Proto::C_dflt, Srv::Main, Usr::Root, {}},
#endif
      {Expect::Ssl, Ssl::Pref, Proto::X_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Pref, Proto::C_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Req, Proto::X, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Req, Proto::C, Srv::Main, Usr::Root, {}},
#ifndef _WIN32
      //{Expect::Ssl, Ssl::Req, Proto::X_dflt, Srv::Main, Usr::Root, {}},
      {Expect::Ssl_, Ssl::Req, Proto::C_dflt, Srv::Main, Usr::Root, {}},
#endif
      {Expect::Ssl, Ssl::Req, Proto::X_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Req, Proto::C_auto, Srv::Main, Usr::Root, {}}};
  TRY_COMBINATIONS(combos);
}

#if 0
TEST_F(Mysqlsh_ssl, all_combos) {
  const std::vector<Combination> combos{
      {Expect::Ssl, Ssl::Dflt, Proto::X, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::X, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::X, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::X, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::C, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::C, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::C, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::C, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::X_dflt, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::X_dflt, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::X_dflt, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::X_dflt, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::C_dflt, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::C_dflt, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::C_dflt, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::C_dflt, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::X_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::X_auto, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::X_auto, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::X_auto, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::C_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::C_auto, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::C_auto, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Dflt, Proto::C_auto, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Disab, Proto::X, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Disab, Proto::X, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Disab, Proto::X, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Disab, Proto::X, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Disab, Proto::C, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Disab, Proto::C, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Disab, Proto::C, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Disab, Proto::C, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Disab, Proto::X_dflt, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Disab, Proto::X_dflt, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Disab, Proto::X_dflt, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Disab, Proto::X_dflt, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Disab, Proto::C_dflt, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Disab, Proto::C_dflt, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Disab, Proto::C_dflt, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Disab, Proto::C_dflt, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Disab, Proto::X_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Disab, Proto::X_auto, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Disab, Proto::X_auto, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Disab, Proto::X_auto, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Disab, Proto::C_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Disab, Proto::C_auto, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Disab, Proto::C_auto, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Disab, Proto::C_auto, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Pref, Proto::X, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Pref, Proto::X, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Pref, Proto::X, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Pref, Proto::X, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Pref, Proto::C, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Pref, Proto::C, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Pref, Proto::C, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Pref, Proto::C, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Pref, Proto::X_dflt, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Pref, Proto::X_dflt, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Pref, Proto::X_dflt, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Pref, Proto::X_dflt, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Pref, Proto::C_dflt, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Pref, Proto::C_dflt, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Pref, Proto::C_dflt, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Pref, Proto::C_dflt, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Pref, Proto::X_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Pref, Proto::X_auto, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Pref, Proto::X_auto, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Pref, Proto::X_auto, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Pref, Proto::C_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Pref, Proto::C_auto, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Pref, Proto::C_auto, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Pref, Proto::C_auto, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Req, Proto::X, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Req, Proto::X, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Req, Proto::X, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Req, Proto::X, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Req, Proto::C, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Req, Proto::C, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Req, Proto::C, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Req, Proto::C, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Req, Proto::X_dflt, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Req, Proto::X_dflt, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Req, Proto::X_dflt, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Req, Proto::X_dflt, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Req, Proto::C_dflt, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Req, Proto::C_dflt, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Req, Proto::C_dflt, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Req, Proto::C_dflt, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Req, Proto::X_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Req, Proto::X_auto, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Req, Proto::X_auto, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Req, Proto::X_auto, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Req, Proto::C_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Req, Proto::C_auto, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Req, Proto::C_auto, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Req, Proto::C_auto, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::X, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::X, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::X, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::X, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::C, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::C, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::C, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::C, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::X_dflt, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::X_dflt, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::X_dflt, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::X_dflt, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::C_dflt, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::C_dflt, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::C_dflt, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::C_dflt, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::X_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::X_auto, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::X_auto, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::X_auto, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::C_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::C_auto, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::C_auto, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_ca, Proto::C_auto, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::X, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::X, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::X, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::X, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::C, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::C, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::C, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::C, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::X_dflt, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::X_dflt, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::X_dflt, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::X_dflt, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::C_dflt, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::C_dflt, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::C_dflt, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::C_dflt, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::X_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::X_auto, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::X_auto, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::X_auto, Srv::Alt, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::C_auto, Srv::Main, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::C_auto, Srv::Main, Usr::SRoot, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::C_auto, Srv::Alt, Usr::Root, {}},
      {Expect::Ssl, Ssl::Ver_id, Proto::C_auto, Srv::Alt, Usr::SRoot, {}}};
}
#endif
