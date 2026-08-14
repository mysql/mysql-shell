/*
 * Copyright (c) 2017, 2026, Oracle and/or its affiliates.
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

#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/mod_mysql_resultset.h"
#include "modules/mod_mysql_session.h"
#include "modules/mod_shell.h"
#include "modules/mod_shell_context.h"
#include "mysqlshdk/include/scripting/types/cpp.h"
#include "scripting/types.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/gmock_clean.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_mysql_session.h"

namespace testing {

namespace {

std::shared_ptr<Mock_result> make_status_result(
    const std::optional<std::string> &value, const std::string &variable) {
  auto result = std::make_shared<Mock_result>();
  std::vector<std::vector<std::string>> rows;

  if (value) rows.push_back({variable, *value});

  result->add_result({"Variable_name", "Value"},
                     {mysqlshdk::db::Type::String, mysqlshdk::db::Type::String},
                     rows);
  return result;
}

std::shared_ptr<Mock_result> make_string_result(
    size_t columns, const std::vector<std::vector<std::string>> &rows = {}) {
  auto result = std::make_shared<Mock_result>();
  std::vector<std::string> names;
  names.reserve(columns);

  for (size_t i = 0; i < columns; ++i)
    names.emplace_back("column" + std::to_string(i));

  result->add_result(
      names,
      std::vector<mysqlshdk::db::Type>(columns, mysqlshdk::db::Type::String),
      rows);
  return result;
}

class Mock_mysqlx_session : public mysqlshdk::db::mysqlx::Session {
 public:
  MOCK_CONST_METHOD0(get_connection_id, uint64_t());
  MOCK_CONST_METHOD0(get_ssl_cipher, const char *());
  MOCK_CONST_METHOD0(get_connection_options,
                     const mysqlshdk::db::Connection_options &());
  MOCK_CONST_METHOD0(get_connection_info, const std::string &());
  MOCK_CONST_METHOD0(is_open, bool());
};

class Mock_mysqlx_shell_session : public mysqlsh::mysqlx::Session {
 public:
  using Query_handler = std::function<std::shared_ptr<mysqlshdk::db::IResult>(
      const std::string &)>;

  explicit Mock_mysqlx_shell_session(
      std::shared_ptr<mysqlshdk::db::mysqlx::Session> session)
      : mysqlsh::mysqlx::Session(std::move(session)) {}

  void set_query_handler(Query_handler handler) {
    m_query_handler = std::move(handler);
  }

 private:
  std::shared_ptr<mysqlshdk::db::IResult> do_execute_sql(
      std::string_view query, [[maybe_unused]] const shcore::Array_t &args,
      [[maybe_unused]] const std::vector<mysqlshdk::db::Query_attribute>
          &query_attributes) override {
    return m_query_handler(std::string{query});
  }

  Query_handler m_query_handler;
};

constexpr auto k_tls_kex_status_query =
    "SHOW SESSION STATUS LIKE 'Tls_key_exchange_algorithm'";
constexpr auto k_tls_sign_status_query =
    "SHOW SESSION STATUS LIKE 'Tls_sign_algorithm'";

void configure_classic_status_results(Mock_mysql_session *session,
                                      std::optional<std::string> *tls_kex,
                                      std::optional<std::string> *tls_sign,
                                      std::vector<std::string> *queries) {
  session->set_query_handler(
      [tls_kex, tls_sign, queries](const std::string &sql) {
        queries->push_back(sql);

        if (sql == "select DATABASE(), USER() limit 1")
          return make_string_result(2);

        if (shcore::str_beginswith(sql, "select @@character_set_client"))
          return make_string_result(9);

        if (sql == k_tls_kex_status_query)
          return make_status_result(*tls_kex, "Tls_key_exchange_algorithm");

        if (sql == k_tls_sign_status_query)
          return make_status_result(*tls_sign, "Tls_sign_algorithm");

        ADD_FAILURE() << "Unexpected query: " << sql;
        return make_string_result(0);
      });
}

void configure_x_status_results(Mock_mysqlx_shell_session *session,
                                std::optional<std::string> *tls_kex,
                                std::optional<std::string> *tls_sign,
                                std::vector<std::string> *queries) {
  session->set_query_handler(
      [tls_kex, tls_sign, queries](const std::string &sql) {
        queries->push_back(sql);

        if (sql == "select DATABASE(), USER() limit 1")
          return make_string_result(2, {{"", "user@host"}});

        if (shcore::str_beginswith(sql, "select @@character_set_client"))
          return make_string_result(9);

        if (shcore::str_beginswith(sql, "show status where variable_name in "))
          return make_string_result(2);

        if (sql == "SHOW GLOBAL STATUS LIKE 'Uptime';")
          return make_string_result(2, {{"Uptime", "10"}});

        if (sql == k_tls_kex_status_query)
          return make_status_result(*tls_kex, "Tls_key_exchange_algorithm");

        if (sql == k_tls_sign_status_query)
          return make_status_result(*tls_sign, "Tls_sign_algorithm");

        ADD_FAILURE() << "Unexpected query: " << sql;
        return make_string_result(0);
      });
}

}  // namespace

class Mock_mysql_shell : public mysqlsh::Mysql_shell {
 public:
  Mock_mysql_shell(std::shared_ptr<mysqlsh::Shell_options> options,
                   shcore::Interpreter_delegate *custom_delegate)
      : mysqlsh::Mysql_shell(options, custom_delegate) {}

  MOCK_METHOD4(
      connect,
      std::shared_ptr<mysqlsh::ShellBaseSession>(
          const mysqlshdk::db::Connection_options &, bool,
          std::function<void(std::shared_ptr<mysqlshdk::db::ISession>)>, bool));
  MOCK_METHOD1(cmd_disconnect, bool(const std::vector<std::string> &args));
};

class mod_shell_test : public Shell_core_test_wrapper {
  void SetUp() override {
    Shell_core_test_wrapper::SetUp();

    _backend.reset(new Mock_mysql_shell(get_options(), &output_handler.deleg));
    _shell.reset(new mysqlsh::Shell(_backend.get()));
  }

  void TearDown() override {
    _shell.reset();
    _backend.reset();

    Shell_core_test_wrapper::TearDown();
  }

 protected:
  std::shared_ptr<Mock_mysql_shell> _backend;
  std::shared_ptr<mysqlsh::Shell> _shell;
};

// WL17311-AUTO-07
TEST_F(mod_shell_test, classic_pqc_status_collection) {
  mysqlshdk::db::Connection_options connection_options;
  auto driver = std::make_shared<NiceMock<Mock_mysql_session>>();
  ON_CALL(*driver, get_connection_options())
      .WillByDefault(ReturnRef(connection_options));
  ON_CALL(*driver, get_ssl_cipher())
      .WillByDefault(Return(static_cast<const char *>(nullptr)));

  std::optional<std::string> tls_kex{"X25519MLKEM768"};
  std::optional<std::string> tls_sign{""};
  std::vector<std::string> queries;
  configure_classic_status_results(driver.get(), &tls_kex, &tls_sign, &queries);

  auto session = std::make_shared<mysqlsh::mysql::ClassicSession>(driver);
  auto status = session->get_status();

  ASSERT_TRUE(status->has_key("TLS_KEY_EXCHANGE_ALGORITHM"));
  ASSERT_TRUE(status->has_key("TLS_SIGNATURE_ALGORITHM"));
  EXPECT_EQ("X25519MLKEM768", status->get_string("TLS_KEY_EXCHANGE_ALGORITHM"));
  EXPECT_EQ("", status->get_string("TLS_SIGNATURE_ALGORITHM"));
  EXPECT_NE(queries.end(),
            std::find(queries.begin(), queries.end(), k_tls_kex_status_query));
  EXPECT_NE(queries.end(),
            std::find(queries.begin(), queries.end(), k_tls_sign_status_query));

  tls_kex.reset();
  tls_sign.reset();
  status = session->get_status();

  EXPECT_FALSE(status->has_key("TLS_KEY_EXCHANGE_ALGORITHM"));
  EXPECT_FALSE(status->has_key("TLS_SIGNATURE_ALGORITHM"));
}

// WL17311-AUTO-07
TEST_F(mod_shell_test, x_pqc_status_collection) {
  mysqlshdk::db::Connection_options connection_options;
  auto driver = std::make_shared<NiceMock<Mock_mysqlx_session>>();
  ON_CALL(*driver, get_connection_options())
      .WillByDefault(ReturnRef(connection_options));
  ON_CALL(*driver, get_ssl_cipher())
      .WillByDefault(Return(static_cast<const char *>(nullptr)));

  std::optional<std::string> tls_kex{"SecP384r1MLKEM1024"};
  std::optional<std::string> tls_sign{""};
  std::vector<std::string> queries;
  auto session = std::make_shared<Mock_mysqlx_shell_session>(driver);
  configure_x_status_results(session.get(), &tls_kex, &tls_sign, &queries);
  auto status = session->get_status();

  ASSERT_TRUE(status->has_key("TLS_KEY_EXCHANGE_ALGORITHM"));
  ASSERT_TRUE(status->has_key("TLS_SIGNATURE_ALGORITHM"));
  EXPECT_EQ("SecP384r1MLKEM1024",
            status->get_string("TLS_KEY_EXCHANGE_ALGORITHM"));
  EXPECT_EQ("", status->get_string("TLS_SIGNATURE_ALGORITHM"));
  EXPECT_NE(queries.end(),
            std::find(queries.begin(), queries.end(), k_tls_kex_status_query));
  EXPECT_NE(queries.end(),
            std::find(queries.begin(), queries.end(), k_tls_sign_status_query));

  tls_kex.reset();
  tls_sign.reset();
  status = session->get_status();

  EXPECT_FALSE(status->has_key("TLS_KEY_EXCHANGE_ALGORITHM"));
  EXPECT_FALSE(status->has_key("TLS_SIGNATURE_ALGORITHM"));
}

// WL17311-AUTO-07
TEST_F(mod_shell_test, pqc_status_output) {
  mysqlshdk::db::Connection_options connection_options;
  auto driver = std::make_shared<NiceMock<Mock_mysql_session>>();
  ON_CALL(*driver, get_connection_options())
      .WillByDefault(ReturnRef(connection_options));
  ON_CALL(*driver, get_ssl_cipher())
      .WillByDefault(Return(static_cast<const char *>(nullptr)));
  ON_CALL(*driver, is_open()).WillByDefault(Return(true));

  std::optional<std::string> tls_kex{"X25519MLKEM768"};
  std::optional<std::string> tls_sign{"rsa_pss_rsae_sha256"};
  std::vector<std::string> queries;
  configure_classic_status_results(driver.get(), &tls_kex, &tls_sign, &queries);

  auto session = std::make_shared<mysqlsh::mysql::ClassicSession>(driver);
  _backend->shell_context()->set_dev_session(session);

  wipe_all();
  _shell->status();
  MY_EXPECT_STDOUT_CONTAINS("TLS key exchange algorithm:   X25519MLKEM768");
  MY_EXPECT_STDOUT_CONTAINS(
      "TLS signature algorithm:      rsa_pss_rsae_sha256");

  tls_kex.reset();
  tls_sign.reset();
  wipe_all();
  _shell->status();
  MY_EXPECT_STDOUT_NOT_CONTAINS("TLS key exchange algorithm:");
  MY_EXPECT_STDOUT_NOT_CONTAINS("TLS signature algorithm:");

  _backend->shell_context()->set_dev_session({});
}

TEST_F(mod_shell_test, context_delegates_json_attributes) {
  std::string print_text;
  std::string print_source;
  std::string prompt_text;
  std::string prompt_type;
  std::string prompt_source;
  std::exception_ptr error;

  std::thread worker([&] {
    try {
      const auto print_delegate = shcore::Cpp_function::create(
          "print",
          [&print_text, &print_source](const shcore::Argument_list &arguments) {
            print_text = arguments[0].as_string();
            print_source = arguments[1].as_map()->get_string("source");
            return shcore::Value();
          },
          {{"text", shcore::String}, {"attributes", shcore::Map}});
      const auto prompt_delegate = shcore::Cpp_function::create(
          "prompt",
          [&prompt_text, &prompt_type,
           &prompt_source](const shcore::Argument_list &arguments) {
            prompt_text = arguments[0].as_string();
            prompt_type = arguments[1].as_map()->get_string("type");
            prompt_source = arguments[2].as_map()->get_string("source");
            return shcore::Value(shcore::make_array(true, "answer"));
          },
          {{"text", shcore::String},
           {"options", shcore::Map},
           {"attributes", shcore::Map}});
      const auto options =
          shcore::make_dict("errorDelegate", shcore::Value(print_delegate),
                            "promptDelegate", shcore::Value(prompt_delegate));
      mysqlsh::Shell_context_wrapper_options callbacks;
      mysqlsh::Shell_context_wrapper_options::options().unpack(options,
                                                               &callbacks);
      const auto context = _shell->create_context(callbacks);
      const auto attributes = shcore::make_dict("source", "test");

      mysqlsh::current_console()->print_info("output", attributes);

      std::string result;
      EXPECT_EQ(shcore::Prompt_result::Ok,
                mysqlsh::current_console()->prompt(
                    "prompt", &result, nullptr,
                    shcore::prompt::Prompt_type::TEXT, "", {}, "", attributes));
      EXPECT_EQ("answer", result);
    } catch (...) {
      error = std::current_exception();
    }
  });
  worker.join();

  if (error) std::rethrow_exception(error);

  EXPECT_EQ("output\n", print_text);
  EXPECT_EQ("test", print_source);
  EXPECT_EQ("prompt", prompt_text);
  EXPECT_EQ("text", prompt_type);
  EXPECT_EQ("test", prompt_source);
}

// clang-format off
TEST_F(mod_shell_test, parse_uri) {
  {
    std::string args;
    args =
        "mysql://user:password@localhost:1234/schema"
        "?Ssl-MoDe=REQUIRED&auth-method=PLAIN";

    auto dict = _shell->parse_uri(args);

    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kScheme));
    EXPECT_STREQ("mysql", dict->get_string(mysqlshdk::db::kScheme).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kUser));
    EXPECT_STREQ("user", dict->get_string(mysqlshdk::db::kUser).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kPassword));
    EXPECT_STREQ("password",
                 dict->get_string(mysqlshdk::db::kPassword).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kHost));
    EXPECT_STREQ("localhost", dict->get_string(mysqlshdk::db::kHost).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kPort));
    EXPECT_EQ(1234, dict->get_int(mysqlshdk::db::kPort));
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSchema));
    EXPECT_STREQ("schema", dict->get_string(mysqlshdk::db::kSchema).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSslMode));
    EXPECT_STREQ("REQUIRED", dict->get_string(mysqlshdk::db::kSslMode).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kAuthMethod));
    EXPECT_STREQ("PLAIN", dict->get_string(mysqlshdk::db::kAuthMethod).c_str());

    EXPECT_EQ(
        "mysql://user:password@localhost:1234/"
        "schema?ssl-mode=REQUIRED&auth-method=PLAIN",
        _shell->unparse_uri(dict));
  }

  // WL17311-AUTO-02
  {
    const auto dict = _shell->parse_uri(
        "mysql://user@host?tls-kex=X25519MLKEM768%3AX25519&"
        "force-pqc=true&use-pqc-sign=false");

    EXPECT_EQ("X25519MLKEM768:X25519",
              dict->get_string(mysqlshdk::db::kSslTlsKex));
    EXPECT_EQ(shcore::Bool,
              dict->get_type(mysqlshdk::db::kSslForcePqc));
    EXPECT_EQ(shcore::Bool,
              dict->get_type(mysqlshdk::db::kSslUsePqcSign));
    EXPECT_TRUE(dict->get_bool(mysqlshdk::db::kSslForcePqc));
    EXPECT_FALSE(dict->get_bool(mysqlshdk::db::kSslUsePqcSign));
  }

#ifdef _WIN32  // TODO(alfredo) - these shouldn't be different in windows
  {
    std::string args = "user@(\\\\.\\named.pipe)/schema";

    auto dict = _shell->parse_uri(args);

    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kScheme));
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kUser));
    EXPECT_STREQ("user", dict->get_string(mysqlshdk::db::kUser).c_str());
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kPassword));
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kHost));
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kPort));
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSocket));
    EXPECT_STREQ("named.pipe",
                 dict->get_string(mysqlshdk::db::kSocket).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSchema));
    EXPECT_STREQ("schema", dict->get_string(mysqlshdk::db::kSchema).c_str());
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kSslMode));
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kAuthMethod));

    // Note: not sure if this is supposed to be user@(\\\\.\\named.pipe)/schema
    // or user@\\\\.\\named.pipe/schema"
    EXPECT_EQ("user@\\\\.\\named.pipe/schema", _shell->unparse_uri(dict));
  }
#endif

#ifndef _WIN32
  {
    std::string args = "user@(/path/to/socket.sock)/schema";

    auto dict = _shell->parse_uri(args);

    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kScheme));
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kUser));
    EXPECT_STREQ("user", dict->get_string(mysqlshdk::db::kUser).c_str());
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kPassword));
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kHost));
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kPort));
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSocket));
    EXPECT_STREQ("/path/to/socket.sock",
                 dict->get_string(mysqlshdk::db::kSocket).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSchema));
    EXPECT_STREQ("schema", dict->get_string(mysqlshdk::db::kSchema).c_str());
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kSslMode));
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kAuthMethod));

    EXPECT_EQ("user@/path%2Fto%2Fsocket.sock/schema",
              _shell->unparse_uri(dict));
  }

  {
    std::string args = "user@/path%2fto%2fsocket.sock/schema";

    auto dict = _shell->parse_uri(args);

    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kScheme));
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kUser));
    EXPECT_STREQ("user", dict->get_string(mysqlshdk::db::kUser).c_str());
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kPassword));
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kHost));
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kPort));
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSocket));
    EXPECT_STREQ("/path/to/socket.sock",
                 dict->get_string(mysqlshdk::db::kSocket).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSchema));
    EXPECT_STREQ("schema", dict->get_string(mysqlshdk::db::kSchema).c_str());
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kSslMode));
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kAuthMethod));

    EXPECT_EQ("user@/path%2Fto%2Fsocket.sock/schema",
              _shell->unparse_uri(dict));
  }
#endif

  {
    std::string args;
    args =
        "user@host?ssl-mode=required&"
        "SSL-CA=%2fpath%2fto%2fca%2epem&"
        "ssl-caPath=%2fpath%2fto%2fcapath&"
        "ssl-cert=%2fpath%2fto%2fcert%2epem&"
        "ssl-key=%2fpath%2fto%2fkey%2epem&"
        "ssl-crl=%2fpath%2fto%2fcrl%2etxt&"
        "ssl-crlPATH=%2fpath%2fto%2fcrlpath&"
        "Ssl-Cipher=%2fpath%2fto%2fcipher&"
        "Tls-VERSION=TLSv1%2e0";

    auto dict = _shell->parse_uri(args);

    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kScheme));
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kUser));
    EXPECT_STREQ("user", dict->get_string(mysqlshdk::db::kUser).c_str());
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kPassword));
    EXPECT_STREQ("host", dict->get_string(mysqlshdk::db::kHost).c_str());
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kPort));
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSslMode));
    EXPECT_STREQ("required", dict->get_string(mysqlshdk::db::kSslMode).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSslCa));
    EXPECT_STREQ("/path/to/ca.pem",
                 dict->get_string(mysqlshdk::db::kSslCa).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSslCaPath));
    EXPECT_STREQ("/path/to/capath",
                 dict->get_string(mysqlshdk::db::kSslCaPath).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSslCert));
    EXPECT_STREQ("/path/to/cert.pem",
                 dict->get_string(mysqlshdk::db::kSslCert).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSslKey));
    EXPECT_STREQ("/path/to/key.pem",
                 dict->get_string(mysqlshdk::db::kSslKey).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSslCrl));
    EXPECT_STREQ("/path/to/crl.txt",
                 dict->get_string(mysqlshdk::db::kSslCrl).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSslCrlPath));
    EXPECT_STREQ("/path/to/crlpath",
                 dict->get_string(mysqlshdk::db::kSslCrlPath).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSslCipher));
    EXPECT_STREQ("/path/to/cipher",
                 dict->get_string(mysqlshdk::db::kSslCipher).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSslTlsVersion));
    EXPECT_STREQ("TLSv1.0",
                 dict->get_string(mysqlshdk::db::kSslTlsVersion).c_str());

    EXPECT_EQ(
        "user@host?ssl-ca=%2Fpath%2Fto%2Fca.pem&ssl-capath=%2Fpath%2Fto%"
        "2Fcapath&ssl-cert=%2Fpath%2Fto%2Fcert.pem&ssl-cipher=%2Fpath%2Fto%"
        "2Fcipher&ssl-crl=%2Fpath%2Fto%2Fcrl.txt&ssl-crlpath=%2Fpath%2Fto%"
        "2Fcrlpath&ssl-key=%2Fpath%2Fto%2Fkey.pem&ssl-mode=required&tls-"
        "version=TLSv1.0",
        _shell->unparse_uri(dict));
  }

  {
    std::string args;
    args =
        "user@host?tls-ciphersuites=ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-"
        "AES256-GCM-SHA384&tls-version=TLSv1.1,TLSv1.2";

    auto dict = _shell->parse_uri(args);

    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kScheme));
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kUser));
    EXPECT_STREQ("user", dict->get_string(mysqlshdk::db::kUser).c_str());
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kPassword));
    EXPECT_STREQ("host", dict->get_string(mysqlshdk::db::kHost).c_str());
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kPort));
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSslTlsCiphersuites));
    EXPECT_STREQ("ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384",
                 dict->get_string(mysqlshdk::db::kSslTlsCiphersuites).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSslTlsVersion));
    EXPECT_STREQ("TLSv1.1,TLSv1.2",
                 dict->get_string(mysqlshdk::db::kSslTlsVersion).c_str());

    EXPECT_EQ(
        "user@host?tls-ciphersuites=ECDHE-ECDSA-AES128-GCM-SHA256%3AECDHE-"
        "ECDSA-AES256-GCM-SHA384&tls-version=TLSv1.1%2CTLSv1.2",
        _shell->unparse_uri(dict));
  }

  {
    std::string args;
    args =
        "user@host?tls-ciphersuites=[ECDHE-ECDSA-AES128-GCM-SHA256,ECDHE-ECDSA-"
        "AES256-GCM-SHA384]&tls-versions=[TLSv1.1,TLSv1.2]";

    auto dict = _shell->parse_uri(args);

    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kScheme));
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kUser));
    EXPECT_STREQ("user", dict->get_string(mysqlshdk::db::kUser).c_str());
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kPassword));
    EXPECT_STREQ("host", dict->get_string(mysqlshdk::db::kHost).c_str());
    EXPECT_FALSE(dict->has_key(mysqlshdk::db::kPort));
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSslTlsCiphersuites));
    EXPECT_STREQ("ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384",
                 dict->get_string(mysqlshdk::db::kSslTlsCiphersuites).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSslTlsVersion));
    EXPECT_STREQ("TLSv1.1,TLSv1.2",
                 dict->get_string(mysqlshdk::db::kSslTlsVersion).c_str());

    EXPECT_EQ(
        "user@host?tls-ciphersuites=ECDHE-ECDSA-AES128-GCM-SHA256%3AECDHE-"
        "ECDSA-AES256-GCM-SHA384&tls-version=TLSv1.1%2CTLSv1.2",
        _shell->unparse_uri(dict));
  }
  // clang-format on

#ifdef _WIN32
  {
    std::string args;
    args = "user@host?plugin-authentication-kerberos-client-mode=SSPI";

    auto dict = _shell->parse_uri(args);

    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kKerberosClientAuthMode));
    EXPECT_STREQ(
        mysqlshdk::db::kKerberosAuthModeSSPI,
        dict->get_string(mysqlshdk::db::kKerberosClientAuthMode).c_str());

    // test for case-sensitivity
    args = "user@host?plugin-authentication-kerberos-client-mode=GsSapi";

    dict = _shell->parse_uri(args);

    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kKerberosClientAuthMode));
    EXPECT_STREQ(
        mysqlshdk::db::kKerberosAuthModeGSSAPI,
        dict->get_string(mysqlshdk::db::kKerberosClientAuthMode).c_str());
  }
#else
  {
    std::string args;
    args = "user@host?plugin-authentication-kerberos-client-mode=SSPI";

    EXPECT_THROW_MSG_CONTAINS(_shell->parse_uri(args), std::exception,
                              "Invalid URI: Invalid connection option "
                              "'plugin-authentication-kerberos-client-mode'.");
  }
#endif

  {
    std::string args;
    args =
        "user@host?oci-config-file=config.cfg&authentication-oci-client-config-"
        "profile=name";

    auto dict = _shell->parse_uri(args);

    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kOciConfigFile));
    EXPECT_TRUE(
        dict->has_key(mysqlshdk::db::kOciAuthenticationClientConfigProfile));

    EXPECT_STREQ("config.cfg",
                 dict->get_string(mysqlshdk::db::kOciConfigFile).c_str());
    EXPECT_STREQ(
        "name",
        dict->get_string(mysqlshdk::db::kOciAuthenticationClientConfigProfile)
            .c_str());
  }
}

TEST_F(mod_shell_test, connect) {
  EXPECT_CALL(*_backend, connect(_, true, _, _));

  _shell->connect(mysqlshdk::db::Connection_options{_mysql_uri});

  EXPECT_CALL(*_backend, connect(_, false, _, _));
  _shell->open_session(mysqlshdk::db::Connection_options{_mysql_uri});
}

TEST_F(mod_shell_test, disconnect) {
  // ensure that shell.disconnect() calls Mysql_shell::disconnect()
  EXPECT_CALL(*_backend, cmd_disconnect(_)).Times(1);
  _shell->disconnect();
}

TEST_F(mod_shell_test, dump_rows) {
  auto session =
      std::make_shared<mysqlsh::mysql::ClassicSession>(create_mysql_session());
  auto xsession =
      std::make_shared<mysqlsh::mysqlx::Session>(create_mysqlx_session());

  mysqlsh::ShellBaseSession *bsession = nullptr;

  auto query = [&bsession](const std::string &sql) {
    auto iresult = bsession->execute_sql(sql);
    std::shared_ptr<mysqlsh::ShellBaseResult> result;
    if (bsession->session_type() == mysqlsh::SessionType::Classic) {
      result = std::make_shared<mysqlsh::mysql::ClassicResult>(iresult);
    } else {
      result = std::make_shared<mysqlsh::mysqlx::SqlResult>(iresult);
    }

    return result;
  };

  for (int i = 0; i < 2; i++) {
    if (i == 0) {
      bsession = session.get();
    } else {
      bsession = xsession.get();
    }

    {
      auto result = query(
          "select 1, 'hello', 4.56, 'foobar' "
          "union all select 4, 'world', 6.32, 'bla'");
      _shell->dump_rows(result, "");
      MY_EXPECT_STDOUT_CONTAINS(
          "+---+-------+------+--------+\n"
          "| 1 | hello | 4.56 | foobar |\n"
          "+---+-------+------+--------+\n"
          "| 1 | hello | 4.56 | foobar |\n"
          "| 4 | world | 6.32 | bla    |\n"
          "+---+-------+------+--------+\n");
    }
    {
      auto result = query(
          "select 1, 'hello', 4.56, 'foobar' "
          "union all select 4, 'world', 6.32, 'bla'");
      _shell->dump_rows(result, "table");
      MY_EXPECT_STDOUT_CONTAINS(
          "+---+-------+------+--------+\n"
          "| 1 | hello | 4.56 | foobar |\n"
          "+---+-------+------+--------+\n"
          "| 1 | hello | 4.56 | foobar |\n"
          "| 4 | world | 6.32 | bla    |\n"
          "+---+-------+------+--------+\n");
      wipe_all();
    }
    {
      auto result = query(
          "select 1, 'hello', 4.56, 'foobar' "
          "union all select 4, 'world', 6.32, 'bla'");
      _shell->dump_rows(result, "tabbed");
      MY_EXPECT_STDOUT_CONTAINS(
          "1\thello\t4.56\tfoobar\n"
          "1\thello\t4.56\tfoobar\n"
          "4\tworld\t6.32\tbla\n");
      wipe_all();
    }
    {
      auto result = query(
          "select 1, 'hello', 4.56, 'foobar' "
          "union all select 4, 'world', 6.32, 'bla'");
      _shell->dump_rows(result, "vertical");
      MY_EXPECT_STDOUT_CONTAINS(
          "*************************** 1. row ***************************\n"
          "     1: 1\n"
          " hello: hello\n"
          "  4.56: 4.56\n"
          "foobar: foobar\n"
          "*************************** 2. row ***************************\n"
          "     1: 4\n"
          " hello: world\n"
          "  4.56: 6.32\n"
          "foobar: bla\n");
      wipe_all();
    }
    {
      auto result = query(
          "select 1, 'hello', 4.56, 'foobar' "
          "union all select 4, 'world', 6.32, 'bla'");
      _shell->dump_rows(result, "json");
      MY_EXPECT_STDOUT_CONTAINS(
          "{\n"
          "    \"1\": 1,\n"
          "    \"hello\": \"hello\",\n"
          "    \"4.56\": 4.559999942779541,\n"
          "    \"foobar\": \"foobar\"\n"
          "}\n"
          "{\n"
          "    \"1\": 4,\n"
          "    \"hello\": \"world\",\n"
          "    \"4.56\": 6.320000171661377,\n"
          "    \"foobar\": \"bla\"\n"
          "}\n");
      wipe_all();
    }
    {
      auto result = query(
          "select 1, 'hello', 4.56, 'foobar' "
          "union all select 4, 'world', 6.32, 'bla'");
      _shell->dump_rows(result, "json/raw");
      // clang-format off
      MY_EXPECT_STDOUT_CONTAINS(
          "{\"1\":1,\"hello\":\"hello\",\"4.56\":4.559999942779541,\"foobar\":\"foobar\"}\n"
          "{\"1\":4,\"hello\":\"world\",\"4.56\":6.320000171661377,\"foobar\":\"bla\"}\n");
      // clang-format on
      wipe_all();
    }
    {
      auto result = query(
          "select 1, 'hello', 4.56, 'foobar' "
          "union all select 4, 'world', 6.32, 'bla'");
      _shell->dump_rows(result, "json/array");
      // clang-format off
      MY_EXPECT_STDOUT_CONTAINS(
          "[\n"
          "{\"1\":1,\"hello\":\"hello\",\"4.56\":4.559999942779541,\"foobar\":\"foobar\"},\n"
          "{\"1\":4,\"hello\":\"world\",\"4.56\":6.320000171661377,\"foobar\":\"bla\"}\n"
          "]\n");
      // clang-format on
      wipe_all();
    }
    {
      auto result = query(
          "select 1, 'hello', 4.56, 'foobar' "
          "union all select 4, 'world', 6.32, 'bla'");
      EXPECT_THROW(_shell->dump_rows(result, "ertyuikjnbg"),
                   std::invalid_argument);
    }
  }
}
}  // namespace testing
