/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:password@localhost"));

  auto options =
      mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::NONE);

  EXPECT_TRUE(options.has_host());
  EXPECT_TRUE(options.has_user());
  EXPECT_TRUE(options.has_password());
  EXPECT_STREQ("localhost", options.get_host().c_str());
  EXPECT_STREQ("root", options.get_user().c_str());
  EXPECT_STREQ(mysqlshdk::db::kPassword, options.get_password().c_str());
}

TEST(modules_mod_utils, get_connection_data_dictionary) {
  shcore::Argument_list args;
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)[mysqlshdk::db::kHost] = shcore::Value("localhost");
  (*map)[mysqlshdk::db::kUser] = shcore::Value("root");
  (*map)[mysqlshdk::db::kPassword] = shcore::Value(mysqlshdk::db::kPassword);
  args.push_back(shcore::Value(map));

  auto options =
      mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::NONE);

  EXPECT_TRUE(options.has_host());
  EXPECT_TRUE(options.has_user());
  EXPECT_TRUE(options.has_password());
  EXPECT_EQ("localhost", options.get_host());
  EXPECT_EQ("root", options.get_user());
  EXPECT_EQ(mysqlshdk::db::kPassword, options.get_password());
}

TEST(modules_mod_utils, get_connection_data_invalid_uri) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:p@ssword@localhost"));

  try {
    auto options =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::NONE);
  } catch (const std::exception &e) {
    std::string error(e.what());

    EXPECT_EQ(0, error.find("Invalid URI: "));
  }
}

TEST(modules_mod_utils, get_connection_data_invalid_connection_data) {
  shcore::Argument_list args;
  args.push_back(shcore::Value(1));

  try {
    auto options =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::NONE);
  } catch (const std::exception &e) {
    std::string error(e.what());

    EXPECT_EQ(
        "Invalid connection options, expected either a URI or a "
        "Dictionary.",
        error);
  }
}

TEST(modules_mod_utils, get_connection_data_empty_uri_connection_data) {
  shcore::Argument_list args;
  args.push_back(shcore::Value(""));

  try {
    auto options =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::NONE);
  } catch (const std::exception &e) {
    std::string error(e.what());

    EXPECT_EQ("Invalid URI: empty.", error);
  }
}

TEST(modules_mod_utils, get_connection_data_empty_options_connection_data) {
  shcore::Argument_list args;
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  args.push_back(shcore::Value(map));

  try {
    auto options =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::NONE);
  } catch (const std::exception &e) {
    std::string error(e.what());

    EXPECT_EQ("Invalid connection options, no options provided.", error);
  }
}

TEST(modules_mod_utils, get_connection_data_ignore_additional_string_password) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:password@localhost"));
  args.push_back(shcore::Value("overwritten_password"));

  auto options =
      mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::NONE);

  EXPECT_TRUE(options.has_host());
  EXPECT_TRUE(options.has_user());
  EXPECT_TRUE(options.has_password());
  EXPECT_EQ("localhost", options.get_host());
  EXPECT_EQ("root", options.get_user());
  EXPECT_EQ("password", options.get_password());
}

TEST(modules_mod_utils,
     get_connection_data_ignore_additional_options_password) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:password@localhost"));

  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)[mysqlshdk::db::kPassword] = shcore::Value("overwritten_password");
  args.push_back(shcore::Value(map));

  auto options =
      mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::NONE);

  EXPECT_TRUE(options.has_host());
  EXPECT_TRUE(options.has_user());
  EXPECT_TRUE(options.has_password());
  EXPECT_EQ("localhost", options.get_host());
  EXPECT_EQ("root", options.get_user());
  EXPECT_EQ("password", options.get_password());
}

TEST(modules_mod_utils, get_connection_data_override_db_password_from_string) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:password@localhost"));
  args.push_back(shcore::Value("overwritten_password"));

  auto options =
      mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::STRING);

  EXPECT_TRUE(options.has_host());
  EXPECT_TRUE(options.has_user());
  EXPECT_TRUE(options.has_password());
  EXPECT_EQ("localhost", options.get_host());
  EXPECT_EQ("root", options.get_user());
  EXPECT_EQ("overwritten_password", options.get_password());
}

TEST(modules_mod_utils,
     get_connection_data_override_db_password_from_options_password) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:password@localhost"));

  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)[mysqlshdk::db::kPassword] = shcore::Value("overwritten_password");
  args.push_back(shcore::Value(map));

  auto options =
      mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);

  EXPECT_TRUE(options.has_host());
  EXPECT_TRUE(options.has_user());
  EXPECT_TRUE(options.has_password());
  EXPECT_EQ("localhost", options.get_host());
  EXPECT_EQ("root", options.get_user());
  EXPECT_EQ("overwritten_password", options.get_password());
}

TEST(modules_mod_utils,
     get_connection_data_override_db_password_from_options_db_password) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:password@localhost"));

  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)[mysqlshdk::db::kDbPassword] = shcore::Value("overwritten_password");
  args.push_back(shcore::Value(map));

  auto options =
      mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);

  EXPECT_TRUE(options.has_host());
  EXPECT_TRUE(options.has_user());
  EXPECT_TRUE(options.has_password());
  EXPECT_EQ("localhost", options.get_host());
  EXPECT_EQ("root", options.get_user());
  EXPECT_EQ("overwritten_password", options.get_password());
}

TEST(modules_mod_utils,
     get_connection_data_override_password_from_options_db_password) {
  shcore::Argument_list args;
  shcore::Value::Map_type_ref connection(new shcore::Value::Map_type());
  (*connection)[mysqlshdk::db::kHost] = shcore::Value("localhost");
  (*connection)[mysqlshdk::db::kUser] = shcore::Value("root");
  (*connection)[mysqlshdk::db::kPassword] =
      shcore::Value(mysqlshdk::db::kPassword);
  args.push_back(shcore::Value(connection));

  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)[mysqlshdk::db::kDbPassword] = shcore::Value("overwritten_password");
  args.push_back(shcore::Value(map));

  auto options =
      mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);

  EXPECT_TRUE(options.has_host());
  EXPECT_TRUE(options.has_user());
  EXPECT_TRUE(options.has_password());
  EXPECT_EQ("localhost", options.get_host());
  EXPECT_EQ("root", options.get_user());
  EXPECT_EQ("overwritten_password", options.get_password());
}

TEST(modules_mod_utils, get_connection_data_invalid_password_param) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:password@localhost"));

  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)[mysqlshdk::db::kPassword] = shcore::Value("overwritten_password");
  args.push_back(shcore::Value(map));

  try {
    auto options =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::STRING);
  } catch (const std::exception &e) {
    std::string error(e.what());
    EXPECT_EQ("Argument #2 is expected to be a string", error);
  }
}

TEST(modules_mod_utils, get_connection_data_invalid_options_param) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:password@localhost"));
  args.push_back(shcore::Value("overwritten_password"));

  try {
    auto options =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::OPTIONS);
  } catch (const std::exception &e) {
    std::string error(e.what());
    EXPECT_EQ("Argument #2 is expected to be a map", error);
  }
}

TEST(modules_mod_utils, get_connection_data_invalid_connection_options) {
  shcore::Argument_list args;
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)[mysqlshdk::db::kHost] = shcore::Value("localhost");
  (*map)[mysqlshdk::db::kUser] = shcore::Value("root");
  (*map)[mysqlshdk::db::kPassword] = shcore::Value(mysqlshdk::db::kPassword);
  (*map)["invalid_option"] = shcore::Value("guess_what");
  args.push_back(shcore::Value(map));

  try {
    auto options =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::NONE);
  } catch (const std::exception &e) {
    std::string error(e.what());
    EXPECT_EQ("Invalid values in connection options: invalid_option", error);
  }
}

TEST(modules_mod_utils, get_connection_data_conflicting_password_db_password) {
  shcore::Argument_list args;
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)[mysqlshdk::db::kHost] = shcore::Value("localhost");
  (*map)[mysqlshdk::db::kUser] = shcore::Value("root");
  (*map)[mysqlshdk::db::kPassword] = shcore::Value(mysqlshdk::db::kPassword);
  (*map)[mysqlshdk::db::kDbPassword] = shcore::Value(mysqlshdk::db::kPassword);
  args.push_back(shcore::Value(map));

  try {
    auto options =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::NONE);
  } catch (const std::exception &e) {
    std::string error(e.what());
    EXPECT_EQ(
        "The connection option 'password' is already defined as "
        "'password'.",
        error);
  }
}

TEST(modules_mod_utils, get_connection_data_conflicting_user_db_user) {
  shcore::Argument_list args;
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)[mysqlshdk::db::kHost] = shcore::Value("localhost");
  (*map)[mysqlshdk::db::kUser] = shcore::Value("root");
  (*map)[mysqlshdk::db::kDbUser] = shcore::Value("root");
  (*map)[mysqlshdk::db::kDbPassword] = shcore::Value(mysqlshdk::db::kPassword);
  args.push_back(shcore::Value(map));

  try {
    auto options =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::NONE);
  } catch (const std::exception &e) {
    std::string error(e.what());
    EXPECT_EQ("The connection option 'user' is already defined as 'root'.",
              error);
  }
}

TEST(modules_mod_utils, get_connection_data_connect_timeout) {
  shcore::Argument_list args;
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)[mysqlshdk::db::kHost] = shcore::Value("localhost");
  (*map)[mysqlshdk::db::kUser] = shcore::Value("root");
  args.push_back(shcore::Value(map));

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

    EXPECT_THROW_LIKE(
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::NONE),
        shcore::Exception, msg);
  }

  auto valid_values = {shcore::Value(0), shcore::Value(1000)};

  for (auto value : valid_values) {
    (*map)[mysqlshdk::db::kConnectTimeout] = value;

    EXPECT_NO_THROW(
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::NONE));
  }
}

TEST(modules_mod_utils, get_connection_data_conflicting_port_socket) {
  shcore::Argument_list args;
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)[mysqlshdk::db::kHost] = shcore::Value("localhost");
  (*map)[mysqlshdk::db::kUser] = shcore::Value("root");
  (*map)["port"] = shcore::Value(3310);
  (*map)["socket"] = shcore::Value("/some/socket/path");
  args.push_back(shcore::Value(map));

  try {
    auto options =
        mysqlsh::get_connection_options(args, mysqlsh::PasswordFormat::NONE);
  } catch (const std::exception &e) {
    std::string error(e.what());
    EXPECT_EQ(
        "Conflicting connection options: port and socket defined"
        ", use either one or the other.",
        error);
  }
}

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
    shcore::Dictionary_t options = shcore::make_dict();
    (*options)["opt1"] = shcore::Value(2);
    (*options)["str"] = shcore::Value("foo");
    (*options)["b"] = shcore::Value::False();

    int64_t o1 = 0;
    std::string str;
    bool b = false;
    Unpack_options(options)
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

  d = 0;
  Unpack_options(maked("n", shcore::Value(32))).optional("n", &d).end();
  EXPECT_EQ(32.0, d);

  // invalid type
  EXPECT_THROW_LIKE(
      Unpack_options(maked("neg", shcore::Value("-555")))
          .optional("neg", &ui)
          .end(),
      shcore::Exception,
      "Option 'neg': Invalid typecast: UInteger expected, but value is String");

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
      shcore::Exception,
      "Option 'str': Invalid typecast: Integer expected, but value is String");
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
}

}  // namespace mysqlsh
