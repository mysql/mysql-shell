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

#include <gtest/gtest.h>
#include "modules/mod_utils.h"
#include "unittest/test_utils.h"

namespace testing {

TEST(modules_mod_utils, get_connection_data_uri) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:password@localhost"));

  shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::NONE);

  EXPECT_EQ(3, options->size());
  EXPECT_TRUE(options->has_key("host"));
  EXPECT_TRUE(options->has_key("dbUser"));
  EXPECT_TRUE(options->has_key("dbPassword"));
  EXPECT_EQ("localhost", options->get_string("host"));
  EXPECT_EQ("root", options->get_string("dbUser"));
  EXPECT_EQ("password", options->get_string("dbPassword"));
}

TEST(modules_mod_utils, get_connection_data_dictionary) {
  shcore::Argument_list args;
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["host"]=shcore::Value("localhost");
  (*map)["user"]=shcore::Value("root");
  (*map)["password"]=shcore::Value("password");
  args.push_back(shcore::Value(map));

  shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::NONE);

  EXPECT_EQ(3, options->size());
  EXPECT_TRUE(options->has_key("host"));
  EXPECT_TRUE(options->has_key("user"));
  EXPECT_TRUE(options->has_key("password"));
  EXPECT_EQ("localhost", options->get_string("host"));
  EXPECT_EQ("root", options->get_string("user"));
  EXPECT_EQ("password", options->get_string("password"));
}

TEST(modules_mod_utils, get_connection_data_invalid_uri) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:p@ssword@localhost"));

  try {
    shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::NONE);
  } catch (const std::exception& e) {
    std::string error(e.what());

    EXPECT_EQ(0, error.find("Invalid connection data, expected a URI"));
  }
}

TEST(modules_mod_utils, get_connection_data_invalid_connection_data) {
  shcore::Argument_list args;
  args.push_back(shcore::Value(1));

  try {
    shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::NONE);
  } catch (const std::exception& e) {
    std::string error(e.what());

    EXPECT_EQ("Invalid connection data, expected either a URI or a Dictionary", error);
  }
}

TEST(modules_mod_utils, get_connection_data_empty_uri_connection_data) {
  shcore::Argument_list args;
  args.push_back(shcore::Value(""));

  try {
    shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::NONE);
  } catch (const std::exception& e) {
    std::string error(e.what());

    EXPECT_EQ("Connection definition is empty", error);
  }
}

TEST(modules_mod_utils, get_connection_data_empty_options_connection_data) {
  shcore::Argument_list args;
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  args.push_back(shcore::Value(map));

  try {
    shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::NONE);
  } catch (const std::exception& e) {
    std::string error(e.what());

    EXPECT_EQ("Connection definition is empty", error);
  }
}

TEST(modules_mod_utils, get_connection_data_ignore_additional_string_password) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:password@localhost"));
  args.push_back(shcore::Value("overwritten_password"));

  shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::NONE);

  EXPECT_EQ(3, options->size());
  EXPECT_TRUE(options->has_key("host"));
  EXPECT_TRUE(options->has_key("dbUser"));
  EXPECT_TRUE(options->has_key("dbPassword"));
  EXPECT_EQ("localhost", options->get_string("host"));
  EXPECT_EQ("root", options->get_string("dbUser"));
  EXPECT_EQ("password", options->get_string("dbPassword"));
}

TEST(modules_mod_utils, get_connection_data_ignore_additional_options_password) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:password@localhost"));

  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["password"] = shcore::Value("overwritten_password");
  args.push_back(shcore::Value(map));

  shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::NONE);

  EXPECT_EQ(3, options->size());
  EXPECT_TRUE(options->has_key("host"));
  EXPECT_TRUE(options->has_key("dbUser"));
  EXPECT_TRUE(options->has_key("dbPassword"));
  EXPECT_EQ("localhost", options->get_string("host"));
  EXPECT_EQ("root", options->get_string("dbUser"));
  EXPECT_EQ("password", options->get_string("dbPassword"));
}


TEST(modules_mod_utils, get_connection_data_override_db_password_from_string) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:password@localhost"));
  args.push_back(shcore::Value("overwritten_password"));

  shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::STRING);

  EXPECT_EQ(3, options->size());
  EXPECT_TRUE(options->has_key("host"));
  EXPECT_TRUE(options->has_key("dbUser"));
  EXPECT_TRUE(options->has_key("dbPassword"));
  EXPECT_EQ("localhost", options->get_string("host"));
  EXPECT_EQ("root", options->get_string("dbUser"));
  EXPECT_EQ("overwritten_password", options->get_string("dbPassword"));
}

TEST(modules_mod_utils, get_connection_data_override_db_password_from_options_password) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:password@localhost"));

  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["password"] = shcore::Value("overwritten_password");
  args.push_back(shcore::Value(map));

  shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::OPTIONS);

  EXPECT_EQ(3, options->size());
  EXPECT_TRUE(options->has_key("host"));
  EXPECT_TRUE(options->has_key("dbUser"));
  EXPECT_TRUE(options->has_key("dbPassword"));
  EXPECT_EQ("localhost", options->get_string("host"));
  EXPECT_EQ("root", options->get_string("dbUser"));
  EXPECT_EQ("overwritten_password", options->get_string("dbPassword"));
}

TEST(modules_mod_utils, get_connection_data_override_db_password_from_options_db_password) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:password@localhost"));

  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["dbPassword"] = shcore::Value("overwritten_password");
  args.push_back(shcore::Value(map));

  shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::OPTIONS);

  EXPECT_EQ(3, options->size());
  EXPECT_TRUE(options->has_key("host"));
  EXPECT_TRUE(options->has_key("dbUser"));
  EXPECT_TRUE(options->has_key("dbPassword"));
  EXPECT_EQ("localhost", options->get_string("host"));
  EXPECT_EQ("root", options->get_string("dbUser"));
  EXPECT_EQ("overwritten_password", options->get_string("dbPassword"));
}

TEST(modules_mod_utils, get_connection_data_override_password_from_options_db_password) {
  shcore::Argument_list args;
  shcore::Value::Map_type_ref connection(new shcore::Value::Map_type());
  (*connection)["host"]=shcore::Value("localhost");
  (*connection)["user"]=shcore::Value("root");
  (*connection)["password"]=shcore::Value("password");
  args.push_back(shcore::Value(connection));

  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["dbPassword"] = shcore::Value("overwritten_password");
  args.push_back(shcore::Value(map));

  shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::OPTIONS);

  EXPECT_EQ(3, options->size());
  EXPECT_TRUE(options->has_key("host"));
  EXPECT_TRUE(options->has_key("user"));
  EXPECT_TRUE(options->has_key("password"));
  EXPECT_EQ("localhost", options->get_string("host"));
  EXPECT_EQ("root", options->get_string("user"));
  EXPECT_EQ("overwritten_password", options->get_string("password"));
}


TEST(modules_mod_utils, get_connection_data_invalid_password_param) {
  shcore::Argument_list args;
  args.push_back(shcore::Value("root:password@localhost"));

  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["password"] = shcore::Value("overwritten_password");
  args.push_back(shcore::Value(map));

  try {
    shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::STRING);
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
    shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::OPTIONS);
  } catch (const std::exception& e) {
    std::string error(e.what());
    EXPECT_EQ("Argument #2 is expected to be a map", error);
  }
}

TEST(modules_mod_utils, get_connection_data_invalid_connection_options) {
  shcore::Argument_list args;
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["host"]=shcore::Value("localhost");
  (*map)["user"]=shcore::Value("root");
  (*map)["password"]=shcore::Value("password");
  (*map)["invalid_option"]=shcore::Value("guess_what");
  args.push_back(shcore::Value(map));

  try {
    shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::NONE);
  } catch (const std::exception& e) {
    std::string error(e.what());
    EXPECT_EQ("Invalid values in connection data: invalid_option", error);
  }
}

TEST(modules_mod_utils, get_connection_data_conflicting_password_db_password) {
  shcore::Argument_list args;
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["host"]=shcore::Value("localhost");
  (*map)["user"]=shcore::Value("root");
  (*map)["password"]=shcore::Value("password");
  (*map)["dbPassword"]=shcore::Value("password");
  args.push_back(shcore::Value(map));

  try {
    shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::NONE);
  } catch (const std::exception& e) {
    std::string error(e.what());
    EXPECT_EQ("Conflicting connection options: password and dbPassword defined"
    ", use either one or the other.", error);
  }
}

TEST(modules_mod_utils, get_connection_data_conflicting_user_db_user) {
  shcore::Argument_list args;
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["host"]=shcore::Value("localhost");
  (*map)["user"]=shcore::Value("root");
  (*map)["dbUser"]=shcore::Value("root");
  (*map)["dbPassword"]=shcore::Value("password");
  args.push_back(shcore::Value(map));

  try {
    shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::NONE);
  } catch (const std::exception& e) {
    std::string error(e.what());
    EXPECT_EQ("Conflicting connection options: user and dbUser defined"
    ", use either one or the other.", error);
  }
}

TEST(modules_mod_utils, get_connection_data_conflicting_port_socket) {
  shcore::Argument_list args;
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["host"]=shcore::Value("localhost");
  (*map)["user"]=shcore::Value("root");
  (*map)["port"]=shcore::Value(3310);
  (*map)["socket"]=shcore::Value("/some/socket/path");
  args.push_back(shcore::Value(map));

  try {
    shcore::Value::Map_type_ref options = mysqlsh::get_connection_data(args, mysqlsh::PasswordFormat::NONE);
  } catch (const std::exception& e) {
    std::string error(e.what());
    EXPECT_EQ("Conflicting connection options: port and socket defined"
    ", use either one or the other.", error);
  }
}

TEST(modules_mod_utils, resolve_connection_credentials_resolve_none) {
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["host"]=shcore::Value("localhost");
  (*map)["user"]=shcore::Value("guest");
  (*map)["password"]=shcore::Value("password");

  mysqlsh::resolve_connection_credentials(map);

  EXPECT_EQ(3, map->size());
  EXPECT_TRUE(map->has_key("host"));
  EXPECT_TRUE(map->has_key("user"));
  EXPECT_TRUE(map->has_key("password"));
  EXPECT_EQ("localhost", map->get_string("host"));
  EXPECT_EQ("guest", map->get_string("user"));
  EXPECT_EQ("password", map->get_string("password"));
}

TEST(modules_mod_utils, resolve_connection_credentials_resolve_user) {
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["host"]=shcore::Value("localhost");
  (*map)["dbPassword"]=shcore::Value("password");

  mysqlsh::resolve_connection_credentials(map);

  EXPECT_EQ(3, map->size());
  EXPECT_TRUE(map->has_key("host"));
  EXPECT_TRUE(map->has_key("dbUser"));
  EXPECT_TRUE(map->has_key("dbPassword"));
  EXPECT_EQ("localhost", map->get_string("host"));
  EXPECT_EQ("root", map->get_string("dbUser"));
  EXPECT_EQ("password", map->get_string("dbPassword"));
}

TEST(modules_mod_utils, resolve_connection_credentials_resolve_password_error_no_delegate) {
  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["host"]=shcore::Value("localhost");

  try {
    mysqlsh::resolve_connection_credentials(map);
  } catch (const std::exception& e) {
    std::string error(e.what());
    EXPECT_EQ("Missing password for 'root@localhost'", error);
  }
}

TEST(modules_mod_utils, resolve_connection_credentials_resolve_password_error_delegate) {
  Shell_test_output_handler output_handler;

  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["host"]=shcore::Value("localhost");

  try {
    mysqlsh::resolve_connection_credentials(map, &output_handler.deleg);
  } catch (const std::exception& e) {
    std::string error(e.what());
    EXPECT_EQ("Missing password for 'root@localhost'", error);
  }
}

TEST(modules_mod_utils, resolve_connection_credentials_resolve_password) {
  Shell_test_output_handler output_handler;

  output_handler.passwords.push_back("resolved_password");

  shcore::Value::Map_type_ref map(new shcore::Value::Map_type());
  (*map)["host"]=shcore::Value("localhost");

  mysqlsh::resolve_connection_credentials(map, &output_handler.deleg);

  EXPECT_EQ(3, map->size());
  EXPECT_TRUE(map->has_key("host"));
  EXPECT_TRUE(map->has_key("dbUser"));
  EXPECT_TRUE(map->has_key("dbPassword"));
  EXPECT_EQ("localhost", map->get_string("host"));
  EXPECT_EQ("root", map->get_string("dbUser"));
  EXPECT_EQ("resolved_password", map->get_string("dbPassword"));
}

}