/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/devapi/mod_mysqlx_resultset.h"
#include "modules/devapi/mod_mysqlx_session.h"
#include "modules/mod_mysql_resultset.h"
#include "modules/mod_mysql_session.h"
#include "modules/mod_shell.h"
#include "scripting/types.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

namespace testing {

class Mock_mysql_shell : public mysqlsh::Mysql_shell {
 public:
  Mock_mysql_shell(std::shared_ptr<mysqlsh::Shell_options> options,
                   shcore::Interpreter_delegate *custom_delegate)
      : mysqlsh::Mysql_shell(options, custom_delegate) {}

  MOCK_METHOD3(connect,
               std::shared_ptr<mysqlsh::ShellBaseSession>(
                   const mysqlshdk::db::Connection_options &, bool, bool));
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
        "2Fcapath&ssl-cert=%2Fpath%2Fto%2Fcert.pem&ssl-key=%2Fpath%2Fto%2Fkey."
        "pem&ssl-crl=%2Fpath%2Fto%2Fcrl.txt&ssl-crlpath=%2Fpath%2Fto%2Fcrlpath&"
        "ssl-cipher=%2Fpath%2Fto%2Fcipher&tls-version=TLSv1.0&ssl-mode="
        "required",
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
        "user@host?tls-version=TLSv1.1%2CTLSv1.2&tls-ciphersuites=ECDHE-ECDSA-"
        "AES128-GCM-SHA256%3AECDHE-ECDSA-AES256-GCM-SHA384",
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
        "user@host?tls-version=TLSv1.1%2CTLSv1.2&tls-ciphersuites=ECDHE-ECDSA-"
        "AES128-GCM-SHA256%3AECDHE-ECDSA-AES256-GCM-SHA384",
        _shell->unparse_uri(dict));
  }
}

TEST_F(mod_shell_test, connect) {
  // ensure that shell.connect() calls Mysql_shell::connect()

  EXPECT_CALL(*_backend, connect(_, false, true));

  _shell->connect({_mysql_uri});

  EXPECT_CALL(*_backend, connect(_, false, false));
  _shell->open_session({_mysql_uri});
}

TEST_F(mod_shell_test, dump_rows) {
  auto session = mysqlsh::mysql::ClassicSession(create_mysql_session());
  auto xsession = mysqlsh::mysqlx::Session(create_mysqlx_session());

  mysqlsh::ShellBaseSession *bsession = nullptr;

  auto query = [&bsession](const std::string &sql) {
    auto iresult = bsession->execute_sql(sql);
    std::shared_ptr<mysqlsh::ShellBaseResult> result;
    auto mysql_result =
        std::dynamic_pointer_cast<mysqlshdk::db::mysql::Result>(iresult);
    if (mysql_result) {
      result = std::make_shared<mysqlsh::mysql::ClassicResult>(mysql_result);
    } else {
      auto mysqlx_result =
          std::dynamic_pointer_cast<mysqlshdk::db::mysqlx::Result>(iresult);
      result = std::make_shared<mysqlsh::mysqlx::SqlResult>(mysqlx_result);
    }

    return result;
  };

  for (int i = 0; i < 2; i++) {
    if (i == 0) {
      bsession = &session;
    } else {
      bsession = &xsession;
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
