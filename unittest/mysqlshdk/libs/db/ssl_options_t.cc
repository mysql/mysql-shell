/* Copyright (c) 2017, 2026, Oracle and/or its affiliates.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2.0,
 as published by the Free Software Foundation.

 This program is designed to work with certain software (including
 but not limited to OpenSSL) that is licensed under separate terms,
 as designated in a particular file or component or in included license
 documentation.  The authors of MySQL hereby grant you an additional
 permission to link the program and your derivative works with the
 separately licensed software that they have either included with
 the program or referenced in the documentation.

 This program is distributed in the hope that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 the GNU General Public License, version 2.0, for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include "unittest/gprod_clean.h"
#include "unittest/gtest_clean.h"

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/db/ssl_options.h"

#ifndef MY_EXPECT_THROW
#define MY_EXPECT_THROW(e, m, c)         \
  EXPECT_THROW(                          \
      {                                  \
        try {                            \
          c;                             \
        } catch (const e &error) {       \
          EXPECT_STREQ(m, error.what()); \
          throw;                         \
        }                                \
      },                                 \
      e)
#endif

using mysqlshdk::db::Ssl_options;
namespace testing {

namespace {

class Recording_xsession : public xcl::XSession {
 public:
  xcl::XProtocol::Client_id client_id() const override { return 0; }

  xcl::XProtocol &get_protocol() override {
    throw std::logic_error("get_protocol() is not used by this test");
  }

  xcl::XError set_mysql_option(Mysqlx_option option, bool value) override {
    bool_options.emplace_back(option, value);
    return {};
  }

  xcl::XError set_mysql_option(Mysqlx_option option,
                               const std::string &value) override {
    string_options.emplace_back(option, value);
    return {};
  }

  xcl::XError set_mysql_option(Mysqlx_option,
                               const std::vector<std::string> &) override {
    return {};
  }

  xcl::XError set_mysql_option(Mysqlx_option option,
                               const char *value) override {
    return set_mysql_option(option, std::string(value));
  }

  xcl::XError set_mysql_option(Mysqlx_option, int64_t) override { return {}; }

  xcl::XError set_capability(Mysqlx_capability, bool, bool) override {
    return {};
  }
  xcl::XError set_capability(Mysqlx_capability, const std::string &,
                             bool) override {
    return {};
  }
  xcl::XError set_capability(Mysqlx_capability, const char *, bool) override {
    return {};
  }
  xcl::XError set_capability(Mysqlx_capability, int64_t, bool) override {
    return {};
  }
  xcl::XError set_capability(Mysqlx_capability, const xcl::Argument_object &,
                             bool) override {
    return {};
  }
  xcl::XError set_capability(Mysqlx_capability, const xcl::Argument_uobject &,
                             bool) override {
    return {};
  }

  xcl::XError connect(const char *, uint16_t, const char *, const char *,
                      const char *) override {
    return {};
  }
  xcl::XError connect(const char *, const char *, const char *,
                      const char *) override {
    return {};
  }
  xcl::XError reauthenticate(const char *, const char *,
                             const char *) override {
    return {};
  }

  std::unique_ptr<xcl::XQuery_result> execute_sql(const std::string &,
                                                  xcl::XError *) override {
    return {};
  }
  std::unique_ptr<xcl::XQuery_result> execute_stmt(const std::string &,
                                                   const std::string &,
                                                   const xcl::Argument_array &,
                                                   xcl::XError *) override {
    return {};
  }

  void close() override {}
  xcl::Argument_uobject get_connect_attrs() const override { return {}; }

  std::vector<std::pair<Mysqlx_option, bool>> bool_options;
  std::vector<std::pair<Mysqlx_option, std::string>> string_options;
};

}  // namespace

void test_option(const std::string &option) {
  Ssl_options options;

  SCOPED_TRACE("SSL Testing: " + option);

  EXPECT_TRUE(options.has(option));
  EXPECT_FALSE(options.has_value(option));
  std::string msg = "The SSL Connection option '";
  msg.append(option).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                  options.get_value(option));
  EXPECT_NO_THROW(options.set(option, "value"));
  EXPECT_TRUE(options.has_value(option));
  EXPECT_STREQ("value", options.get_value(option).c_str());
  EXPECT_NO_THROW(options.clear_value(option));
  EXPECT_FALSE(options.has_value(option));
}

TEST(Ssl_options, initialization) {
  Ssl_options options;

  // Specific attributes function work like the has_value
  EXPECT_FALSE(options.has_ca());
  EXPECT_FALSE(options.has_capath());
  EXPECT_FALSE(options.has_cert());
  EXPECT_FALSE(options.has_cipher());
  EXPECT_FALSE(options.has_crl());
  EXPECT_FALSE(options.has_crlpath());
  EXPECT_FALSE(options.has_key());
  EXPECT_FALSE(options.has_mode());
  EXPECT_FALSE(options.has_tls_version());
  EXPECT_FALSE(options.has_tls_ciphersuites());

  // WL17311-AUTO-01
  EXPECT_FALSE(options.has_tls_kex());
  EXPECT_FALSE(options.has_force_pqc());
  EXPECT_FALSE(options.has_use_pqc_sign());
  EXPECT_FALSE(options.get_force_pqc());
  EXPECT_FALSE(options.get_use_pqc_sign());

  // has verifies the existence of the option
  EXPECT_TRUE(options.has(mysqlshdk::db::kSslCa));
  EXPECT_TRUE(options.has(mysqlshdk::db::kSslCaPath));
  EXPECT_TRUE(options.has(mysqlshdk::db::kSslCert));
  EXPECT_TRUE(options.has(mysqlshdk::db::kSslCipher));
  EXPECT_TRUE(options.has(mysqlshdk::db::kSslCrl));
  EXPECT_TRUE(options.has(mysqlshdk::db::kSslCrlPath));
  EXPECT_TRUE(options.has(mysqlshdk::db::kSslKey));
  EXPECT_TRUE(options.has(mysqlshdk::db::kSslMode));
  EXPECT_TRUE(options.has(mysqlshdk::db::kSslTlsVersion));
  EXPECT_TRUE(options.has(mysqlshdk::db::kSslTlsCiphersuites));

  // WL17311-AUTO-01
  EXPECT_TRUE(options.has(mysqlshdk::db::kSslTlsKex));
  EXPECT_TRUE(options.has(mysqlshdk::db::kSslForcePqc));
  EXPECT_TRUE(options.has(mysqlshdk::db::kSslUsePqcSign));

  // has_value verifies the existence of a value for the option
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSslCa));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSslCaPath));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSslCert));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSslCipher));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSslCrl));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSslCrlPath));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSslKey));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSslMode));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSslTlsVersion));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSslTlsCiphersuites));

  // WL17311-AUTO-01
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSslTlsKex));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSslForcePqc));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSslUsePqcSign));
  EXPECT_FALSE(options.has_data());
}

// WL17311-AUTO-01
TEST(Ssl_options, pqc_functions) {
  Ssl_options options;

  options.set_tls_kex("X25519MLKEM768:X25519");
  options.set_force_pqc(true);
  options.set_use_pqc_sign(false);

  EXPECT_TRUE(options.has_data());
  EXPECT_EQ("X25519MLKEM768:X25519", options.get_tls_kex());
  EXPECT_TRUE(options.get_force_pqc());
  EXPECT_FALSE(options.get_use_pqc_sign());

  options.clear_tls_kex();
  options.clear_force_pqc();
  options.clear_use_pqc_sign();

  EXPECT_FALSE(options.has_tls_kex());
  EXPECT_FALSE(options.has_force_pqc());
  EXPECT_FALSE(options.has_use_pqc_sign());
  EXPECT_FALSE(options.get_force_pqc());
  EXPECT_FALSE(options.get_use_pqc_sign());
  EXPECT_FALSE(options.has_data());
}

// WL17311-AUTO-01
TEST(Ssl_options, pqc_generic_set) {
  Ssl_options options;

  options.set(mysqlshdk::db::kSslTlsKex, "SecP384r1MLKEM1024:X25519");
  options.set(mysqlshdk::db::kSslForcePqc, "1");
  options.set(mysqlshdk::db::kSslUsePqcSign, "FALSE");

  EXPECT_EQ("SecP384r1MLKEM1024:X25519", options.get_tls_kex());
  EXPECT_TRUE(options.get_force_pqc());
  EXPECT_FALSE(options.get_use_pqc_sign());

  MY_EXPECT_THROW(std::invalid_argument,
                  "Invalid value 'yes' for 'force-pqc'. Allowed values: true, "
                  "false, 1, 0.",
                  options.set(mysqlshdk::db::kSslForcePqc, "yes"));
}

TEST(Ssl_options, ca_functions) {
  Ssl_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_ca("value"));
  options.set_ca("value2");
  EXPECT_TRUE(options.has_ca());
  EXPECT_STREQ("value2", options.get_ca().c_str());
  EXPECT_NO_THROW(options.clear_ca());
  EXPECT_FALSE(options.has_ca());
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslCa).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_ca());
}

TEST(Ssl_options, capath_functions) {
  Ssl_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_capath("value"));
  options.set_capath("value2");
  EXPECT_TRUE(options.has_capath());
  EXPECT_STREQ("value2", options.get_capath().c_str());
  EXPECT_NO_THROW(options.clear_capath());
  EXPECT_FALSE(options.has_capath());
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslCaPath).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_capath());
}

TEST(Ssl_options, cert_functions) {
  Ssl_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_cert("value"));
  options.set_cert("value2");
  EXPECT_TRUE(options.has_cert());
  EXPECT_STREQ("value2", options.get_cert().c_str());
  EXPECT_NO_THROW(options.clear_cert());
  EXPECT_FALSE(options.has_cert());
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslCert).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_cert());
}

TEST(Ssl_options, cipher_functions) {
  Ssl_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_cipher("value"));
  options.set_cipher("value2");
  EXPECT_TRUE(options.has_cipher());
  EXPECT_STREQ("value2", options.get_cipher().c_str());
  EXPECT_NO_THROW(options.clear_cipher());
  EXPECT_FALSE(options.has_cipher());
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslCipher).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_cipher());
}

TEST(Ssl_options, crl_functions) {
  Ssl_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_crl("value"));
  options.set_crl("value2");
  EXPECT_TRUE(options.has_crl());
  EXPECT_STREQ("value2", options.get_crl().c_str());
  EXPECT_NO_THROW(options.clear_crl());
  EXPECT_FALSE(options.has_crl());
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslCrl).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_crl());
}

TEST(Ssl_options, crlpath_functions) {
  Ssl_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_crlpath("value"));
  options.set_crlpath("value2");
  EXPECT_TRUE(options.has_crlpath());
  EXPECT_STREQ("value2", options.get_crlpath().c_str());
  EXPECT_NO_THROW(options.clear_crlpath());
  EXPECT_FALSE(options.has_crlpath());
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslCrlPath).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_crlpath());
}

TEST(Ssl_options, key_functions) {
  Ssl_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_key("value"));
  options.set_key("value2");
  EXPECT_TRUE(options.has_key());
  EXPECT_STREQ("value2", options.get_key().c_str());
  EXPECT_NO_THROW(options.clear_key());
  EXPECT_FALSE(options.has_key());
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslKey).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_key());
}

TEST(Ssl_options, mode_functions) {
  Ssl_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_mode(mysqlshdk::db::Ssl_mode::Disabled));
  options.set_mode(mysqlshdk::db::Ssl_mode::Preferred);
  EXPECT_TRUE(options.has_mode());
  EXPECT_EQ(mysqlshdk::db::Ssl_mode::Preferred, options.get_mode());
  EXPECT_NO_THROW(options.clear_mode());
  EXPECT_FALSE(options.has_mode());
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslMode).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.get_mode());
}

TEST(Ssl_options, generic_functions) {
  test_option(mysqlshdk::db::kSslCa);
  test_option(mysqlshdk::db::kSslCaPath);
  test_option(mysqlshdk::db::kSslCert);
  test_option(mysqlshdk::db::kSslCipher);
  test_option(mysqlshdk::db::kSslCrl);
  test_option(mysqlshdk::db::kSslCrlPath);
  test_option(mysqlshdk::db::kSslKey);
}

TEST(Ssl_options, generic_set_mode) {
  Ssl_options options;

  std::string msg = "Invalid value 'whatever' for '";
  msg.append(mysqlshdk::db::kSslMode)
      .append(
          "'. Allowed "
          "values: DISABLED, PREFERRED, REQUIRED, VERIFY_CA, VERIFY_IDENTITY.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                  options.set(mysqlshdk::db::kSslMode, "whatever"));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSslMode));
  EXPECT_NO_THROW(options.set(mysqlshdk::db::kSslMode, "disabled"));
  EXPECT_TRUE(options.has(mysqlshdk::db::kSslMode));
  EXPECT_EQ("disabled", options.get_value(mysqlshdk::db::kSslMode));
  EXPECT_NO_THROW(options.clear_value(mysqlshdk::db::kSslMode));
  EXPECT_FALSE(options.has_value(mysqlshdk::db::kSslMode));
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslMode).append("' has no value.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                  options.get_value(mysqlshdk::db::kSslMode));
}

}  // namespace testing

namespace mysqlshdk {
namespace db {
namespace mysql {

// WL17311-AUTO-05
TEST(Pqc_connection, classic_forwarding) {
  Session_impl session;
  session._mysql = mysql_init(nullptr);
  ASSERT_NE(nullptr, session._mysql);

  Ssl_options options;
  session.setup_ssl(options);

  char *tls_kex = nullptr;
  bool force_pqc = true;
  bool use_pqc_sign = true;
  EXPECT_EQ(0, mysql_get_option(session._mysql, MYSQL_OPT_TLS_KEX, &tls_kex));
  EXPECT_EQ(0,
            mysql_get_option(session._mysql, MYSQL_OPT_FORCE_PQC, &force_pqc));
  EXPECT_EQ(0, mysql_get_option(session._mysql, MYSQL_OPT_USE_PQC_SIGN,
                                &use_pqc_sign));
  EXPECT_EQ(nullptr, tls_kex);
  EXPECT_FALSE(force_pqc);
  EXPECT_FALSE(use_pqc_sign);

  options.set_tls_kex("X25519MLKEM768:X25519");
  options.set_force_pqc(true);
  options.set_use_pqc_sign(true);
  session.setup_ssl(options);

  EXPECT_EQ(0, mysql_get_option(session._mysql, MYSQL_OPT_TLS_KEX, &tls_kex));
  EXPECT_EQ(0,
            mysql_get_option(session._mysql, MYSQL_OPT_FORCE_PQC, &force_pqc));
  EXPECT_EQ(0, mysql_get_option(session._mysql, MYSQL_OPT_USE_PQC_SIGN,
                                &use_pqc_sign));
  ASSERT_NE(nullptr, tls_kex);
  EXPECT_STREQ("X25519MLKEM768:X25519", tls_kex);
  EXPECT_TRUE(force_pqc);
  EXPECT_TRUE(use_pqc_sign);
}

}  // namespace mysql

namespace mysqlx {

// WL17311-AUTO-06
TEST(Pqc_connection, x_forwarding) {
  XSession_impl session;
  auto driver = std::make_unique<::testing::Recording_xsession>();
  auto *recording = driver.get();
  session._mysql = std::move(driver);

  Ssl_options options;
  session.setup_ssl(options);

  EXPECT_TRUE(recording->string_options.empty());
  EXPECT_EQ((std::vector<std::pair<xcl::XSession::Mysqlx_option, bool>>{
                {xcl::XSession::Mysqlx_option::Force_pqc, false},
                {xcl::XSession::Mysqlx_option::Use_pqc_sign, false}}),
            recording->bool_options);

  recording->bool_options.clear();
  options.set_tls_kex("SecP384r1MLKEM1024:X25519");
  options.set_force_pqc(true);
  options.set_use_pqc_sign(true);
  session.setup_ssl(options);

  EXPECT_EQ((std::vector<std::pair<xcl::XSession::Mysqlx_option, std::string>>{
                {xcl::XSession::Mysqlx_option::Tls_kex,
                 "SecP384r1MLKEM1024:X25519"}}),
            recording->string_options);
  EXPECT_EQ((std::vector<std::pair<xcl::XSession::Mysqlx_option, bool>>{
                {xcl::XSession::Mysqlx_option::Force_pqc, true},
                {xcl::XSession::Mysqlx_option::Use_pqc_sign, true}}),
            recording->bool_options);
}

}  // namespace mysqlx
}  // namespace db
}  // namespace mysqlshdk
