/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/mod_utils.h"
#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"

namespace testing {

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
  } catch (const std::exception& e) {
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
  } catch (const std::exception& e) {
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
  } catch (const std::exception& e) {
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
  } catch (const std::exception& e) {
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
  } catch (const std::exception& e) {
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
  } catch (const std::exception& e) {
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
  } catch (const std::exception& e) {
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
  } catch (const std::exception& e) {
    std::string error(e.what());
    EXPECT_EQ(
        "The Connection option 'password' is already defined as "
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
  } catch (const std::exception& e) {
    std::string error(e.what());
    EXPECT_EQ("The Connection option 'user' is already defined as 'root'.",
              error);
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
  } catch (const std::exception& e) {
    std::string error(e.what());
    EXPECT_EQ(
        "Conflicting connection options: port and socket defined"
        ", use either one or the other.",
        error);
  }
}

TEST(modules_mod_utils, resolve_connection_credentials_resolve_none) {
  mysqlshdk::db::Connection_options connection_options;
  connection_options.set_host("localhost");
  connection_options.set_user("guest");
  connection_options.set_password(mysqlshdk::db::kPassword);

  mysqlsh::resolve_connection_credentials(&connection_options);

  EXPECT_TRUE(connection_options.has_host());
  EXPECT_TRUE(connection_options.has_user());
  EXPECT_TRUE(connection_options.has_password());
  EXPECT_EQ("localhost", connection_options.get_host());
  EXPECT_EQ("guest", connection_options.get_user());
  EXPECT_EQ(mysqlshdk::db::kPassword, connection_options.get_password());
}

TEST(modules_mod_utils, resolve_connection_credentials_resolve_user) {
  mysqlshdk::db::Connection_options connection_options;
  connection_options.set_host("localhost");
  connection_options.set_password(mysqlshdk::db::kPassword);

  mysqlsh::resolve_connection_credentials(&connection_options);

  EXPECT_TRUE(connection_options.has_host());
  EXPECT_TRUE(connection_options.has_user());
  EXPECT_TRUE(connection_options.has_password());
  EXPECT_EQ("localhost", connection_options.get_host());
  EXPECT_EQ("root", connection_options.get_user());
  EXPECT_EQ(mysqlshdk::db::kPassword, connection_options.get_password());
}

TEST(modules_mod_utils,
     resolve_connection_credentials_resolve_password_error_no_delegate) {
  mysqlshdk::db::Connection_options connection_options;
  connection_options.set_host("localhost");

  try {
    mysqlsh::resolve_connection_credentials(&connection_options);
  } catch (const std::exception& e) {
    std::string error(e.what());
    EXPECT_EQ("Missing password for 'root@localhost'", error);
  }
}

TEST(modules_mod_utils,
     resolve_connection_credentials_resolve_password_error_delegate) {
  Shell_test_output_handler output_handler;

  mysqlshdk::db::Connection_options connection_options;
  connection_options.set_host("localhost");

  try {
    mysqlsh::resolve_connection_credentials(&connection_options,
                                            &output_handler.deleg);
  } catch (const std::exception& e) {
    std::string error(e.what());
    EXPECT_EQ("Missing password for 'root@localhost'", error);
  }
}

TEST(modules_mod_utils, resolve_connection_credentials_resolve_password) {
  Shell_test_output_handler output_handler;

  output_handler.passwords.push_back("resolved_password");

  mysqlshdk::db::Connection_options connection_options;
  connection_options.set_host("localhost");

  mysqlsh::resolve_connection_credentials(&connection_options,
                                          &output_handler.deleg);

  EXPECT_TRUE(connection_options.has_host());
  EXPECT_TRUE(connection_options.has_user());
  EXPECT_TRUE(connection_options.has_password());
  EXPECT_EQ("localhost", connection_options.get_host());
  EXPECT_EQ("root", connection_options.get_user());
  EXPECT_EQ("resolved_password", connection_options.get_password());
}

}  // namespace testing
