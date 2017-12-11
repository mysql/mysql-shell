/* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

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

#include "unittest/gtest_clean.h"
#include "mysqlshdk/libs/db/ssl_options.h"

#ifndef MY_EXPECT_THROW
#define MY_EXPECT_THROW(e, m, c)         \
  EXPECT_THROW(                          \
      {                                  \
        try {                            \
          c;                             \
        } catch (const e& error) {       \
          EXPECT_STREQ(m, error.what()); \
          throw;                         \
        }                                \
      },                                 \
      e)
#endif

using mysqlshdk::db::Ssl_options;
namespace testing {

void test_option(const std::string& option) {
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
}

TEST(Ssl_options, ca_functions) {
  Ssl_options options;
  std::string msg;

  EXPECT_NO_THROW(options.set_ca("value"));
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslCa);
  msg.append("' is already defined as 'value'.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.set_ca("value2"));
  EXPECT_TRUE(options.has_ca());
  EXPECT_STREQ("value", options.get_ca().c_str());
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
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslCaPath);
  msg.append("' is already defined as 'value'.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                  options.set_capath("value2"));
  EXPECT_TRUE(options.has_capath());
  EXPECT_STREQ("value", options.get_capath().c_str());
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
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslCert);
  msg.append("' is already defined as 'value'.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                  options.set_cert("value2"));
  EXPECT_TRUE(options.has_cert());
  EXPECT_STREQ("value", options.get_cert().c_str());
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
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslCipher);
  msg.append("' is already defined as 'value'.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                  options.set_cipher("value2"));
  EXPECT_TRUE(options.has_cipher());
  EXPECT_STREQ("value", options.get_cipher().c_str());
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
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslCrl);
  msg.append("' is already defined as 'value'.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                  options.set_crl("value2"));
  EXPECT_TRUE(options.has_crl());
  EXPECT_STREQ("value", options.get_crl().c_str());
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
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslCrlPath);
  msg.append("' is already defined as 'value'.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                  options.set_crlpath("value2"));
  EXPECT_TRUE(options.has_crlpath());
  EXPECT_STREQ("value", options.get_crlpath().c_str());
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
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslKey);
  msg.append("' is already defined as 'value'.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(),
                  options.set_key("value2"));
  EXPECT_TRUE(options.has_key());
  EXPECT_STREQ("value", options.get_key().c_str());
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
  msg = "The SSL Connection option '";
  msg.append(mysqlshdk::db::kSslMode);
  msg.append("' is already defined as '");
  msg.append(mysqlshdk::db::MapSslModeNameToValue::get_value(1)).append("'.");
  MY_EXPECT_THROW(std::invalid_argument, msg.c_str(), options.set_mode(mysqlshdk::db::Ssl_mode::Preferred));
  EXPECT_TRUE(options.has_mode());
  EXPECT_EQ(mysqlshdk::db::Ssl_mode::Disabled, options.get_mode());
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
