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

#include <gtest_clean.h>
#include <stdexcept>
#include "mysqlshdk/libs/db/uri_encoder.h"

using mysqlshdk::db::uri::Uri_encoder;
using mysqlshdk::db::uri::SUBDELIMITERS;
using mysqlshdk::db::uri::ALPHANUMERIC;
using mysqlshdk::db::uri::DELIMITERS;
using mysqlshdk::db::uri::UNRESERVED;
using mysqlshdk::db::uri::HEXDIG;
using mysqlshdk::db::uri::DIGIT;

namespace testing {

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

TEST(Uri_encoder, encode_scheme) {
  Uri_encoder encoder;

  EXPECT_EQ("mysqlx", encoder.encode_scheme("mysqlx"));
  EXPECT_EQ("mysql", encoder.encode_scheme("mysql"));

  MY_EXPECT_THROW(std::invalid_argument,
                  "Scheme extension [ssh] is not supported",
                  encoder.encode_scheme("mysql+ssh"));

  MY_EXPECT_THROW(std::invalid_argument,
                  "Invalid scheme format [mysql+ssh+], only one extension "
                  "is supported",
                  encoder.encode_scheme("mysql+ssh+"));

  MY_EXPECT_THROW(std::invalid_argument,
                  "Invalid scheme [sample], supported schemes include: "
                  "mysql, mysqlx",
                  encoder.encode_scheme("sample"));
}

TEST(Uri_encoder, encode_user_info) {
  Uri_encoder encoder;

  EXPECT_EQ("guest", encoder.encode_userinfo("guest"));

  // Subdelimiters are allowed
  EXPECT_EQ(SUBDELIMITERS, encoder.encode_userinfo(SUBDELIMITERS));

  // Alphanumerics are allowed
  EXPECT_EQ(ALPHANUMERIC, encoder.encode_userinfo(ALPHANUMERIC));

  // Delimiters are pct-encoded
  EXPECT_EQ("%3A%2F%3F%23%5B%5D%40", encoder.encode_userinfo(DELIMITERS));

  // Anything pct-encoded is not touched
  EXPECT_EQ("mysql%6c", encoder.encode_userinfo("mysql%6c"));

  // Anything else is pct-encoded
  EXPECT_EQ("%22%25%3C%3E%5C%5E%60%7B%7D%7C",
            encoder.encode_userinfo("\"%<>\\^`{}|"));
}

TEST(Uri_encoder, encode_host) {
  Uri_encoder encoder;

  EXPECT_EQ("guest", encoder.encode_userinfo("guest"));

  // Digits are allowed
  EXPECT_EQ(DIGIT, encoder.encode_userinfo(DIGIT));

  // Digits are allowed
  EXPECT_EQ(HEXDIG, encoder.encode_userinfo(HEXDIG));

  // Subdelimiters are allowed
  EXPECT_EQ(SUBDELIMITERS, encoder.encode_userinfo(SUBDELIMITERS));

  // Unreserved are allowed
  EXPECT_EQ(UNRESERVED, encoder.encode_userinfo(UNRESERVED));

  // Delimiters are pct-encoded
  EXPECT_EQ("%3A%2F%3F%23%5B%5D%40", encoder.encode_userinfo(DELIMITERS));

  // Anything pct-encoded is not touched
  EXPECT_EQ("localhos%74", encoder.encode_userinfo("localhos%74"));

  // Anything else is pct-encoded
  EXPECT_EQ("%22%25%3C%3E%5C%5E%60%7B%7D%7C",
            encoder.encode_userinfo("\"%<>\\^`{}|"));
}

TEST(Uri_encoder, encode_port) {
  Uri_encoder encoder;

  EXPECT_EQ("6500", encoder.encode_port(6500));
  EXPECT_EQ("0", encoder.encode_port(0));
  EXPECT_EQ("65535", encoder.encode_port(65535));

  MY_EXPECT_THROW(std::invalid_argument,
                  "Port is out of the valid range: 0 - 65535",
                  encoder.encode_port(65536));

  MY_EXPECT_THROW(std::invalid_argument,
                  "Port is out of the valid range: 0 - 65535",
                  encoder.encode_port(-1));

  EXPECT_EQ("6500", encoder.encode_port("6500"));
  EXPECT_EQ("0", encoder.encode_port("0"));
  EXPECT_EQ("65535", encoder.encode_port("65535"));

  MY_EXPECT_THROW(std::invalid_argument,
                  "Port is out of the valid range: 0 - 65535",
                  encoder.encode_port("65536"));

  MY_EXPECT_THROW(std::invalid_argument,
                  "Unexpected data [-] found in port definition",
                  encoder.encode_port("-1"));

  MY_EXPECT_THROW(std::invalid_argument,
                  "Unexpected data [somethingelse+123] found in port "
                  "definition",
                  encoder.encode_port("60somethingelse+123"));
}

TEST(Uri_encoder, encode_socket) {
  Uri_encoder encoder;

  EXPECT_EQ("/path%2Fto%2Fsocket.sock",
            encoder.encode_socket("/path/to/socket.sock"));

  // Unreserved are allowed
  EXPECT_EQ(UNRESERVED, encoder.encode_socket(UNRESERVED));

  // These sub-delimiters are allowed
  EXPECT_EQ("!$'()*+;=", encoder.encode_socket("!$'()*+;="));

  // These sub-delimiters are NOT allowed
  EXPECT_EQ("%26%2C", encoder.encode_socket("&,"));

  // Delimiters are pct-encoded
  EXPECT_EQ("%3A%2F%3F%23%5B%5D%40", encoder.encode_socket(DELIMITERS));

  // Anything pct-encoded is not touched
  EXPECT_EQ("socket%2Esock", encoder.encode_socket("socket%2Esock"));

  // Anything else is pct-encoded
  EXPECT_EQ("%22%25%3C%3E%5C%5E%60%7B%7D%7C",
            encoder.encode_socket("\"%<>\\^`{}|"));
}

TEST(Uri_encoder, encode_schema) {
  Uri_encoder encoder;

  EXPECT_EQ("my%20schema", encoder.encode_schema("my schema"));

  // Unreserved are allowed
  EXPECT_EQ(UNRESERVED, encoder.encode_schema(UNRESERVED));

  // The sub-delimiters are allowed
  EXPECT_EQ(SUBDELIMITERS, encoder.encode_schema(SUBDELIMITERS));

  // These delimiters are NOT allowed
  EXPECT_EQ("@:", encoder.encode_schema("@:"));

  // Delimiters are pct-encoded
  EXPECT_EQ(":%2F%3F%23%5B%5D@", encoder.encode_schema(DELIMITERS));

  // Anything pct-encoded is not touched
  EXPECT_EQ("my%20databas%65", encoder.encode_schema("my databas%65"));

  // Anything else is pct-encoded
  EXPECT_EQ("%22%25%3C%3E%5C%5E%60%7B%7D%7C",
            encoder.encode_schema("\"%<>\\^`{}|"));
}

TEST(Uri_encoder, encode_attribute) {
  Uri_encoder encoder;

  EXPECT_EQ("some%20attribute", encoder.encode_attribute("some attribute"));

  // Unreserved are allowed
  EXPECT_EQ(UNRESERVED, encoder.encode_attribute(UNRESERVED));

  // The sub-delimiters are allowed
  EXPECT_EQ("!$%26'()*+,;%3D", encoder.encode_attribute(SUBDELIMITERS));

  // Delimiters are pct-encoded
  EXPECT_EQ("%3A%2F%3F%23%5B%5D%40", encoder.encode_attribute(DELIMITERS));

  // Anything pct-encoded is not touched
  EXPECT_EQ("my%20attribut%65", encoder.encode_attribute("my attribut%65"));

  // Anything else is pct-encoded
  EXPECT_EQ("%22%25%3C%3E%5C%5E%60%7B%7D%7C",
            encoder.encode_attribute("\"%<>\\^`{}|"));
}

TEST(Uri_encoder, encode_value) {
  Uri_encoder encoder;

  EXPECT_EQ("some%20attribute", encoder.encode_value("some attribute"));

  // Unreserved are allowed
  EXPECT_EQ(UNRESERVED, encoder.encode_value(UNRESERVED));

  // These sub-delimiters are allowed
  EXPECT_EQ("!$'()*+;=", encoder.encode_socket("!$'()*+;="));

  // Delimiters are pct-encoded
  EXPECT_EQ("%3A%2F%3F%23%5B%5D%40", encoder.encode_value(DELIMITERS));

  // Anything pct-encoded is not touched
  EXPECT_EQ("my%20attribut%65", encoder.encode_value("my attribut%65"));

  // Anything else is pct-encoded
  EXPECT_EQ("%22%25%3C%3E%5C%5E%60%7B%7D%7C",
            encoder.encode_value("\"%<>\\^`{}|"));
}

TEST(Uri_encoder, encode_values) {
  Uri_encoder encoder;

  EXPECT_EQ("some%20attribute", encoder.encode_values({"some attribute"}));

  EXPECT_EQ("[first,second]", encoder.encode_values({"first", "second"}));

  EXPECT_EQ("", encoder.encode_values({}));

  EXPECT_EQ("[some%20attribute]",
            encoder.encode_values({"some attribute"}, true));

  EXPECT_EQ("[]", encoder.encode_values({}, true));
}

}  // namespace testing
