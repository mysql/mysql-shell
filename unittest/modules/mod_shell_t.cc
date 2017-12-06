/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/mod_shell.h"
#include "scripting/types.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

namespace testing {

class Mock_mysql_shell : public mysqlsh::Mysql_shell {
 public:
  Mock_mysql_shell(std::shared_ptr<mysqlsh::Shell_options> options,
                   shcore::Interpreter_delegate* custom_delegate)
      : mysqlsh::Mysql_shell(options, custom_delegate) {
  }

  MOCK_METHOD2(connect, void(const mysqlshdk::db::Connection_options&, bool));
};

class mod_shell_test : public Shell_core_test_wrapper {
  void SetUp() override {
    Shell_core_test_wrapper::SetUp();

    _backend.reset(new Mock_mysql_shell(_opts, &output_handler.deleg));
    _shell.reset(new mysqlsh::Shell(_backend.get()));
  }

 protected:
  std::shared_ptr<Mock_mysql_shell> _backend;
  std::shared_ptr<mysqlsh::Shell> _shell;
};

TEST_F(mod_shell_test, parse_uri) {
  {
    shcore::Argument_list args;
    args.push_back(
        shcore::Value("mysql://user:password@localhost:1234/schema"
                      "?Ssl-MoDe=REQUIRED&auth-method=PLAIN"));

    auto value = _shell->parse_uri(args);
    auto dict = value.as_map();

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
  }

  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("user@(/path/to/socket.sock)/schema"));

    auto value = _shell->parse_uri(args);
    auto dict = value.as_map();

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
  }

  {
    shcore::Argument_list args;
    args.push_back(shcore::Value("user@/path%2fto%2fsocket.sock/schema"));

    auto value = _shell->parse_uri(args);
    auto dict = value.as_map();

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
  }

  {
    shcore::Argument_list args;
    args.push_back(
        shcore::Value("user@host?ssl-mode=required&"
                      "SSL-CA=%2fpath%2fto%2fca%2epem&"
                      "ssl-caPath=%2fpath%2fto%2fcapath&"
                      "ssl-cert=%2fpath%2fto%2fcert%2epem&"
                      "ssl-key=%2fpath%2fto%2fkey%2epem&"
                      "ssl-crl=%2fpath%2fto%2fcrl%2etxt&"
                      "ssl-crlPATH=%2fpath%2fto%2fcrlpath&"
                      "Ssl-Ciphers=%2fpath%2fto%2fciphers&"
                      "Tls-VERSION=TLSv1%2e0"));

    auto value = _shell->parse_uri(args);
    auto dict = value.as_map();

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
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSslCiphers));
    EXPECT_STREQ("/path/to/ciphers",
                 dict->get_string(mysqlshdk::db::kSslCiphers).c_str());
    EXPECT_TRUE(dict->has_key(mysqlshdk::db::kSslTlsVersion));
    EXPECT_STREQ("TLSv1.0",
                 dict->get_string(mysqlshdk::db::kSslTlsVersion).c_str());
  }
}

TEST_F(mod_shell_test, connect) {
  // ensure that shell.connect() calls Mysql_shell::connect()

  EXPECT_CALL(*_backend, connect(_, false));

  shcore::Argument_list args;
  args.push_back(shcore::Value(_mysql_uri));
  _shell->connect(args);
}

}  // namespace testing
