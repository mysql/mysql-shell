/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/mod_utils.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

namespace mysqlsh {

TEST(modules_mod_utils, get_connection_data_uri) {
  auto options =
      mysqlsh::get_connection_options(shcore::Value("root:password@localhost"));

  EXPECT_TRUE(options.has_host());
  EXPECT_TRUE(options.has_user());
  EXPECT_TRUE(options.has_password());
  EXPECT_STREQ("localhost", options.get_host().c_str());
  EXPECT_STREQ("root", options.get_user().c_str());
  EXPECT_STREQ(mysqlshdk::db::kPassword, options.get_password().c_str());
}

TEST(modules_mod_utils, get_connection_data_dictionary) {
  const auto map = shcore::make_dict();
  (*map)[mysqlshdk::db::kHost] = shcore::Value("localhost");
  (*map)[mysqlshdk::db::kUser] = shcore::Value("root");
  (*map)[mysqlshdk::db::kPassword] = shcore::Value(mysqlshdk::db::kPassword);

  auto options = mysqlsh::get_connection_options(shcore::Value(map));

  EXPECT_TRUE(options.has_host());
  EXPECT_TRUE(options.has_user());
  EXPECT_TRUE(options.has_password());
  EXPECT_EQ("localhost", options.get_host());
  EXPECT_EQ("root", options.get_user());
  EXPECT_EQ(mysqlshdk::db::kPassword, options.get_password());
}

TEST(modules_mod_utils, get_connection_data_invalid_uri) {
  EXPECT_THROW_LIKE(
      mysqlsh::get_connection_options(shcore::Value("root:p@ssword@localhost")),
      std::invalid_argument, "Invalid URI: ");
}

TEST(modules_mod_utils, get_connection_data_invalid_connection_data) {
  EXPECT_THROW_LIKE(
      mysqlsh::get_connection_options(shcore::Value(1)), std::invalid_argument,
      "Invalid connection options, expected either a URI or a Dictionary.");
}

TEST(modules_mod_utils, get_connection_data_empty_uri_connection_data) {
  EXPECT_THROW_LIKE(mysqlsh::get_connection_options(shcore::Value("")),
                    std::invalid_argument, "Invalid URI: empty.");
}

TEST(modules_mod_utils, get_connection_data_empty_options_connection_data) {
  EXPECT_THROW_LIKE(
      mysqlsh::get_connection_options(shcore::Value(shcore::make_dict())),
      std::invalid_argument,
      "Invalid connection options, no options provided.");
}

TEST(modules_mod_utils, get_connection_data_override_db_password_from_string) {
  auto options =
      mysqlsh::get_connection_options(shcore::Value("root:password@localhost"));
  set_password_from_string(&options, "overwritten_password");

  EXPECT_TRUE(options.has_host());
  EXPECT_TRUE(options.has_user());
  EXPECT_TRUE(options.has_password());
  EXPECT_EQ("localhost", options.get_host());
  EXPECT_EQ("root", options.get_user());
  EXPECT_EQ("overwritten_password", options.get_password());
}

TEST(modules_mod_utils,
     get_connection_data_override_db_password_from_options_password) {
  const auto map = shcore::make_dict();
  (*map)[mysqlshdk::db::kPassword] = shcore::Value("overwritten_password");

  auto options =
      mysqlsh::get_connection_options(shcore::Value("root:password@localhost"));
  set_password_from_map(&options, map);

  EXPECT_TRUE(options.has_host());
  EXPECT_TRUE(options.has_user());
  EXPECT_TRUE(options.has_password());
  EXPECT_EQ("localhost", options.get_host());
  EXPECT_EQ("root", options.get_user());
  EXPECT_EQ("overwritten_password", options.get_password());
}

TEST(modules_mod_utils,
     get_connection_data_override_db_password_from_options_db_password) {
  const auto map = shcore::make_dict();
  (*map)[mysqlshdk::db::kDbPassword] = shcore::Value("overwritten_password");

  auto options =
      mysqlsh::get_connection_options(shcore::Value("root:password@localhost"));
  set_password_from_map(&options, map);

  EXPECT_TRUE(options.has_host());
  EXPECT_TRUE(options.has_user());
  EXPECT_TRUE(options.has_password());
  EXPECT_EQ("localhost", options.get_host());
  EXPECT_EQ("root", options.get_user());
  EXPECT_EQ("overwritten_password", options.get_password());
}

TEST(modules_mod_utils,
     get_connection_data_override_password_from_options_db_password) {
  const auto connection = shcore::make_dict();
  (*connection)[mysqlshdk::db::kHost] = shcore::Value("localhost");
  (*connection)[mysqlshdk::db::kUser] = shcore::Value("root");
  (*connection)[mysqlshdk::db::kPassword] =
      shcore::Value(mysqlshdk::db::kPassword);

  const auto map = shcore::make_dict();
  (*map)[mysqlshdk::db::kDbPassword] = shcore::Value("overwritten_password");

  auto options = mysqlsh::get_connection_options(shcore::Value(connection));
  set_password_from_map(&options, map);

  EXPECT_TRUE(options.has_host());
  EXPECT_TRUE(options.has_user());
  EXPECT_TRUE(options.has_password());
  EXPECT_EQ("localhost", options.get_host());
  EXPECT_EQ("root", options.get_user());
  EXPECT_EQ("overwritten_password", options.get_password());
}

TEST(modules_mod_utils, get_connection_data_invalid_connection_options) {
  const auto map = shcore::make_dict();
  (*map)[mysqlshdk::db::kHost] = shcore::Value("localhost");
  (*map)[mysqlshdk::db::kUser] = shcore::Value("root");
  (*map)[mysqlshdk::db::kPassword] = shcore::Value(mysqlshdk::db::kPassword);
  (*map)["invalid_option"] = shcore::Value("guess_what");

  EXPECT_THROW_LIKE(mysqlsh::get_connection_options(shcore::Value(map)),
                    shcore::Exception,
                    "Invalid values in connection options: invalid_option");
}

TEST(modules_mod_utils, get_connection_data_conflicting_password_db_password) {
  const auto map = shcore::make_dict();
  (*map)[mysqlshdk::db::kHost] = shcore::Value("localhost");
  (*map)[mysqlshdk::db::kUser] = shcore::Value("root");
  (*map)[mysqlshdk::db::kPassword] = shcore::Value(mysqlshdk::db::kPassword);
  (*map)[mysqlshdk::db::kDbPassword] = shcore::Value(mysqlshdk::db::kPassword);

  EXPECT_THROW_LIKE(
      mysqlsh::get_connection_options(shcore::Value(map)),
      std::invalid_argument,
      "The connection option 'password' is already defined as 'password'.");
}

TEST(modules_mod_utils, get_connection_data_conflicting_user_db_user) {
  const auto map = shcore::make_dict();
  (*map)[mysqlshdk::db::kHost] = shcore::Value("localhost");
  (*map)[mysqlshdk::db::kUser] = shcore::Value("root");
  (*map)[mysqlshdk::db::kDbUser] = shcore::Value("root");
  (*map)[mysqlshdk::db::kDbPassword] = shcore::Value(mysqlshdk::db::kPassword);

  EXPECT_THROW_LIKE(
      mysqlsh::get_connection_options(shcore::Value(map)),
      std::invalid_argument,
      "The connection option 'user' is already defined as 'root'.");
}

TEST(modules_mod_utils, get_connection_data_connect_timeout) {
  const auto map = shcore::make_dict();
  (*map)[mysqlshdk::db::kHost] = shcore::Value("localhost");
  (*map)[mysqlshdk::db::kUser] = shcore::Value("root");

  auto invalid_values = {shcore::Value(-1), shcore::Value(-100),
                         shcore::Value(-10.0), shcore::Value("-1"),
                         shcore::Value("-100"), shcore::Value("10.0"),
                         shcore::Value("whatever"),
                         // When using a dictionary, connect_timeout must be
                         // given as an integer value
                         shcore::Value("1000")};

  for (auto value : invalid_values) {
    (*map)[mysqlshdk::db::kConnectTimeout] = value;

    std::string msg("Invalid value '");
    msg.append(value.descr());
    msg.append(
        "' for 'connect-timeout'. The connection timeout value must "
        "be a positive integer (including 0).");

    EXPECT_THROW_LIKE(mysqlsh::get_connection_options(shcore::Value(map)),
                      std::invalid_argument, msg);
  }

  auto valid_values = {shcore::Value(0), shcore::Value(1000)};

  for (auto value : valid_values) {
    (*map)[mysqlshdk::db::kConnectTimeout] = value;

    EXPECT_NO_THROW(mysqlsh::get_connection_options(shcore::Value(map)));
  }
}

#ifdef _WIN32
#define SOCKET_NAME "pipe"
#else  // !_WIN32
#define SOCKET_NAME "socket"
#endif  // !_WIN32

TEST(modules_mod_utils, get_connection_data_conflicting_port_socket) {
  const auto map = shcore::make_dict();
  (*map)[mysqlshdk::db::kHost] = shcore::Value("localhost");
  (*map)[mysqlshdk::db::kUser] = shcore::Value("root");
  (*map)["port"] = shcore::Value(3310);
  (*map)["socket"] = shcore::Value("/some/socket/path");

  try {
    auto options = mysqlsh::get_connection_options(shcore::Value(map));
  } catch (const std::exception &e) {
    std::string error(e.what());
    EXPECT_EQ("Unable to set a " SOCKET_NAME
              " connection to '/some/socket/path', a tcp connection to "
              "'localhost:3310' is already defined.",
              error);
  }

  EXPECT_THROW_LIKE(mysqlsh::get_connection_options(shcore::Value(map)),
                    std::invalid_argument,
                    "Unable to set a " SOCKET_NAME
                    " connection to '/some/socket/path', a tcp connection to "
                    "'localhost:3310' is already defined.");
}

#undef SOCKET_NAME

TEST(modules_mod_utils, unpack_options_validation) {
  shcore::Dictionary_t options = shcore::make_dict();
  (*options)["opt1"] = shcore::Value(2);
  (*options)["opt2"] = shcore::Value(2);
  (*options)["opt3"] = shcore::Value(2);

  {
    int64_t o1 = 0, o2 = 1, o3 = 3;
    Unpack_options(options)
        .required("opt1", &o1)
        .required("opt2", &o2)
        .required("opt3", &o3)
        .end();
    EXPECT_EQ(2, o1);
    EXPECT_EQ(2, o2);
    EXPECT_EQ(2, o3);
  }
  {
    int64_t o1 = 0, o2 = 1, o3 = 3;
    Unpack_options(options)
        .required("opt1", &o1)
        .optional("opt2", &o2)
        .optional("opt3", &o3)
        .end();
    EXPECT_EQ(2, o1);
    EXPECT_EQ(2, o2);
    EXPECT_EQ(2, o3);
  }
  {
    int64_t o1 = 0, o2 = 1, o3 = 3, o4 = 42;
    Unpack_options(options)
        .required("opt1", &o1)
        .required("opt2", &o2)
        .optional("opt3", &o3)
        .optional("opt4", &o4)
        .end();
    EXPECT_EQ(2, o1);
    EXPECT_EQ(2, o2);
    EXPECT_EQ(2, o3);
    EXPECT_EQ(42, o4);
  }
  {  // missing option
    int64_t o1 = 0, o2 = 1, o3 = 3, o4 = 42;
    EXPECT_THROW_LIKE(Unpack_options(options)
                          .required("opt1", &o1)
                          .required("opt2", &o2)
                          .optional("opt3", &o3)
                          .required("opt4", &o4)
                          .end(),
                      shcore::Exception, "Missing required options: opt4");
  }
  {  // unexpected option
    int64_t o1 = 0, o2 = 1;
    EXPECT_THROW_LIKE(Unpack_options(options)
                          .required("opt1", &o1)
                          .required("opt2", &o2)
                          .end(),
                      shcore::Exception, "Invalid options: opt3");
  }
  {  // missing and unexpected option
    int64_t o1 = 0, o2 = 1, o5 = 1;
    EXPECT_THROW_LIKE(
        Unpack_options(options)
            .required("opt1", &o1)
            .required("opt2", &o2)
            .required("opt5", &o5)
            .end(),
        shcore::Exception,
        "Invalid and missing options (invalid: opt3), (missing: opt5)");
  }
  {
    const auto opts = shcore::make_dict();
    (*opts)["opt1"] = shcore::Value(2);
    (*opts)["str"] = shcore::Value("foo");
    (*opts)["b"] = shcore::Value::False();

    int64_t o1 = 0;
    std::string str;
    bool b = false;
    Unpack_options(opts)
        .required("opt1", &o1)
        .optional_ci("str", &str)
        .optional("b", &b)
        .end();
    EXPECT_EQ(2, o1);
    EXPECT_EQ("foo", str);
    EXPECT_FALSE(b);
  }
}

TEST(modules_mod_utils, unpack_options_types) {
  auto maked = [](const std::string &name, shcore::Value value) {
    shcore::Dictionary_t options = shcore::make_dict();
    (*options)[name] = value;
    return options;
  };

  bool b;
  uint64_t ui;
  int64_t i;
  double d;
  std::string str;
  mysqlsh::Connection_options copts;

  b = true;
  Unpack_options(maked("bool", shcore::Value::False()))
      .required("bool", &b)
      .end();
  EXPECT_FALSE(b);

  bool bb;
  b = true;
  Unpack_options(maked("bool", shcore::Value::False()))
      .optional("boolx", &b)
      .optional("bool", &bb)
      .end();
  EXPECT_TRUE(b);

  b = false;
  Unpack_options(maked("int", shcore::Value(123))).optional("int", &b).end();
  EXPECT_TRUE(b);

  ui = 0;
  Unpack_options(maked("uint", shcore::Value(123))).optional("uint", &ui).end();
  EXPECT_EQ(123, ui);

  i = 0;
  Unpack_options(maked("int", shcore::Value(-123))).optional("int", &i).end();
  EXPECT_EQ(-123, i);

  d = 0;
  Unpack_options(maked("double", shcore::Value(123.456)))
      .optional("double", &d)
      .end();
  EXPECT_EQ(123.456, d);

  str = "";
  Unpack_options(maked("str", shcore::Value("string")))
      .optional("str", &str)
      .end();
  EXPECT_EQ("string", str);

  str = "xxx";
  Unpack_options(maked("STRIng", shcore::Value("qq")))
      .optional_ci("string", &str)
      .end();
  EXPECT_EQ("qq", str);

  Unpack_options(maked("uri", shcore::Value("root@localhost:3456")))
      .optional_obj("uri", &copts)
      .end();
  EXPECT_EQ("root@localhost:3456", copts.as_uri());

  Unpack_options(maked("uri", shcore::Value("root@localhost:3456")))
      .optional("uri", &str)
      .end();
  EXPECT_EQ("root@localhost:3456", str);

  // conversions
  Unpack_options(maked("num", shcore::Value(555U))).optional("num", &i).end();
  EXPECT_EQ(555, i);

  Unpack_options(maked("num", shcore::Value(555))).optional("num", &ui).end();
  EXPECT_EQ(555, ui);

  b = false;
  Unpack_options(maked("n", shcore::Value(1))).optional("n", &b).end();
  EXPECT_TRUE(b);

  b = true;
  Unpack_options(maked("n", shcore::Value(0))).optional("n", &b).end();
  EXPECT_FALSE(b);

  b = false;
  Unpack_options(maked("n", shcore::Value("true"))).optional("n", &b).end();
  EXPECT_TRUE(b);

  b = true;
  Unpack_options(maked("n", shcore::Value("false"))).optional("n", &b).end();
  EXPECT_FALSE(b);

  d = 0;
  Unpack_options(maked("n", shcore::Value(32))).optional("n", &d).end();
  EXPECT_EQ(32.0, d);

  // invalid type
  EXPECT_THROW_LIKE(Unpack_options(maked("neg", shcore::Value("-555")))
                        .optional("neg", &ui)
                        .end(),
                    shcore::Exception,
                    "Option 'neg' UInteger expected, but value is String");

  str = "xxx";
  EXPECT_THROW_LIKE(
      Unpack_options(maked("int", shcore::Value(1234)))
          .optional("int", &str)
          .end(),
      shcore::Exception,
      "Option 'int' is expected to be of type String, but is Integer");
  EXPECT_EQ("xxx", str);

  i = 0;
  EXPECT_THROW_LIKE(
      Unpack_options(maked("str", shcore::Value(""))).optional("str", &i).end(),
      shcore::Exception, "Option 'str' Integer expected, but value is String");
  EXPECT_EQ(0, i);

  EXPECT_THROW_LIKE(
      Unpack_options(maked("bool", shcore::Value::False()))
          .optional_obj("bool", &copts)
          .end(),
      shcore::Exception,
      "Option 'bool' is expected to be of type String, but is Bool");

  EXPECT_THROW_LIKE(
      Unpack_options(maked("int", shcore::Value(123)))
          .optional_obj("int", &copts)
          .end(),
      shcore::Exception,
      "Option 'int' is expected to be of type String, but is Integer");

  EXPECT_THROW_LIKE(Unpack_options(maked("str", shcore::Value("whatever")))
                        .optional("str", &b)
                        .end(),
                    shcore::Exception,
                    "Option 'str' Bool expected, but value is String");
}

}  // namespace mysqlsh
