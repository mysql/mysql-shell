/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/config/config_file.h"
#include "mysqlshdk/libs/config/config_file_handler.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

using mysqlshdk::config::Config_file;
using mysqlshdk::config::Config_file_handler;
using mysqlshdk::utils::nullable;
extern "C" const char *g_test_home;

namespace testing {

class Config_file_handler_test : public tests::Shell_base_test {
 protected:
  std::string m_option_files_basedir, m_tmpdir, m_base_cfg_path,
      m_no_mysql_section_cfg_path;

  void SetUp() override {
    tests::Shell_base_test::SetUp();
    m_option_files_basedir =
        shcore::path::join_path(g_test_home, "data", "config");
    m_tmpdir = getenv("TMPDIR");
    m_base_cfg_path =
        shcore::path::join_path(m_option_files_basedir, "file_handler.cnf");
    m_no_mysql_section_cfg_path =
        shcore::path::join_path(m_option_files_basedir, "my_include.cnf");
  }
};

TEST_F(Config_file_handler_test, test_get_bool) {
  SCOPED_TRACE("Testing get_bool_method of handler");
  Config_file_handler cfg_handler =
      Config_file_handler("uuid1", m_base_cfg_path, m_base_cfg_path);
  Config_file_handler cfg_handler_no_mysql_section = Config_file_handler(
      "uuid1", m_no_mysql_section_cfg_path, m_no_mysql_section_cfg_path);
  mysqlshdk::utils::nullable<bool> res = cfg_handler.get_bool("bool_true1");
  EXPECT_TRUE(*res);
  res = cfg_handler.get_bool("bool_true2");
  EXPECT_TRUE(*res);
  res = cfg_handler.get_bool("bool_true3");
  EXPECT_TRUE(*res);
  res = cfg_handler.get_bool("bool_false1");
  EXPECT_FALSE(*res);
  res = cfg_handler.get_bool("bool_false2");
  EXPECT_FALSE(*res);
  res = cfg_handler.get_bool("bool_false3");
  EXPECT_FALSE(*res);
  // option without value should return null boolean
  res = cfg_handler.get_bool("no_comment_no-value");
  EXPECT_TRUE(res.is_null());
  // getting a bool value for option that cannot be converted
  EXPECT_THROW_LIKE(cfg_handler.get_bool("datadir"), std::runtime_error,
                    "The value of option 'datadir' cannot be converted to a "
                    "boolean.");
  // getting a bool value for option that doesn't exist
  EXPECT_THROW_LIKE(cfg_handler.get_bool("not there"), std::out_of_range,
                    "Option 'not there' does not exist in group 'mysqld'.")
  // getting bool and mysqld group doesn't exist
  EXPECT_THROW_LIKE(cfg_handler_no_mysql_section.get_bool("not there"),
                    std::out_of_range,
                    "Option 'not there' does not exist in group 'mysqld'.")
}

TEST_F(Config_file_handler_test, test_get_int) {
  SCOPED_TRACE("Testing get_int_method of handler");
  Config_file_handler cfg_handler =
      Config_file_handler("uuid1", m_base_cfg_path, m_base_cfg_path);
  Config_file_handler cfg_handler_no_mysql_section = Config_file_handler(
      "uuid1", m_no_mysql_section_cfg_path, m_no_mysql_section_cfg_path);
  mysqlshdk::utils::nullable<int64_t> res = cfg_handler.get_int("positive_int");
  EXPECT_EQ(*res, 123456);
  res = cfg_handler.get_int("negative_int");
  EXPECT_EQ(*res, -123456);
  res = cfg_handler.get_int("zero_int");
  EXPECT_EQ(*res, 0);
  res = cfg_handler.get_int("zero_int2");
  EXPECT_EQ(*res, 0);
  // option without value should return null int
  res = cfg_handler.get_int("no_comment_no-value");
  EXPECT_TRUE(res.is_null());
  // getting an int value for option that cannot be converted
  EXPECT_THROW_LIKE(cfg_handler.get_int("datadir"), std::runtime_error,
                    "The value '/var/lib/mysql' for option 'datadir' cannot be "
                    "converted to an integer.");
  // getting an int value for option that cannot be converted
  EXPECT_THROW_LIKE(cfg_handler.get_int("invalid_int"), std::runtime_error,
                    "The value '123 letters' for option 'invalid_int' cannot "
                    "be converted to an integer.");
  // getting an int value for option that doesn't exist
  EXPECT_THROW_LIKE(cfg_handler.get_int("not there"), std::out_of_range,
                    "Option 'not there' does not exist in group 'mysqld'.")
  // getting an int value and mysqld group doesn't exist
  EXPECT_THROW_LIKE(cfg_handler_no_mysql_section.get_int("not there"),
                    std::out_of_range,
                    "Option 'not there' does not exist in group 'mysqld'.")
}

TEST_F(Config_file_handler_test, test_get_string) {
  SCOPED_TRACE("Testing get_string_method of handler");
  Config_file_handler cfg_handler =
      Config_file_handler("uuid1", m_base_cfg_path, m_base_cfg_path);
  Config_file_handler cfg_handler_no_mysql_section = Config_file_handler(
      "uuid1", m_no_mysql_section_cfg_path, m_no_mysql_section_cfg_path);
  mysqlshdk::utils::nullable<std::string> res = cfg_handler.get_string("port");
  EXPECT_STREQ((*res).c_str(), "1001");
  res = cfg_handler.get_string("backspace");
  EXPECT_STREQ((*res).c_str(), "\b");
  res = cfg_handler.get_string("tab");
  EXPECT_STREQ((*res).c_str(), "\t");
  res = cfg_handler.get_string("backslash");
  EXPECT_STREQ((*res).c_str(), "\\");
  res = cfg_handler.get_string("multivalue");
  EXPECT_STREQ((*res).c_str(), "Noooooooooooooooo");
  res = cfg_handler.get_string("bind_address");
  EXPECT_STREQ((*res).c_str(), "127.0.0.1");
  res = cfg_handler.get_string("pid_file");
  EXPECT_STREQ((*res).c_str(), "/var/run/mysqld/mysqld.pid");
  res = cfg_handler.get_string("bool_true1");
  EXPECT_STREQ((*res).c_str(), "Yes");
  res = cfg_handler.get_string("bool_false2");
  EXPECT_STREQ((*res).c_str(), "False");
  res = cfg_handler.get_string("zero_int");
  EXPECT_STREQ((*res).c_str(), "0");
  res = cfg_handler.get_string("zero_int2");
  EXPECT_STREQ((*res).c_str(), "-0");
  res = cfg_handler.get_string("invalid_int");
  EXPECT_STREQ((*res).c_str(), "123 letters");

  // option without value should return null string
  res = cfg_handler.get_string("no_comment_no-value");
  EXPECT_TRUE(res.is_null());
  // getting a string value for option that doesn't exist
  EXPECT_THROW_LIKE(cfg_handler.get_string("not there"), std::out_of_range,
                    "Option 'not there' does not exist in group 'mysqld'.")
  // getting a string value and mysqld group doesn't exist
  EXPECT_THROW_LIKE(cfg_handler_no_mysql_section.get_string("not there"),
                    std::out_of_range,
                    "Option 'not there' does not exist in group 'mysqld'.")
}

TEST_F(Config_file_handler_test, test_set) {
  // Check that values can be set even if they don't exist or the
  // section doesn't exist
  Config_file_handler cfg_handler =
      Config_file_handler("uuid1", m_base_cfg_path, m_base_cfg_path);
  Config_file_handler cfg_handler_no_mysql_section = Config_file_handler(
      "uuid1", m_no_mysql_section_cfg_path, m_no_mysql_section_cfg_path);
  {
    SCOPED_TRACE("Testing set method with non existing boolean");
    EXPECT_THROW_LIKE(
        cfg_handler_no_mysql_section.get_bool("set_bool_true"),
        std::out_of_range,
        "Option 'set_bool_true' does not exist in group 'mysqld'.");
    cfg_handler_no_mysql_section.set("set_bool_true",
                                     mysqlshdk::utils::nullable<bool>(true));
    EXPECT_TRUE(*(cfg_handler_no_mysql_section.get_bool("set_bool_true")));

    EXPECT_THROW_LIKE(
        cfg_handler_no_mysql_section.get_bool("set_bool_false"),
        std::out_of_range,
        "Option 'set_bool_false' does not exist in group 'mysqld'.");
    cfg_handler_no_mysql_section.set("set_bool_false",
                                     mysqlshdk::utils::nullable<bool>(false));
    EXPECT_FALSE(*(cfg_handler_no_mysql_section.get_bool("set_bool_false")));
  }
  {
    SCOPED_TRACE("Testing set method with non existing int");
    EXPECT_THROW_LIKE(cfg_handler_no_mysql_section.get_int("set_int_123"),
                      std::out_of_range,
                      "Option 'set_int_123' does not exist in group 'mysqld'.");
    cfg_handler_no_mysql_section.set("set_int_123",
                                     mysqlshdk::utils::nullable<int64_t>(123));
    EXPECT_EQ(*(cfg_handler_no_mysql_section.get_int("set_int_123")), 123);
  }
  {
    SCOPED_TRACE("Testing set method with non existing string");
    EXPECT_THROW_LIKE(
        cfg_handler_no_mysql_section.get_string("set_string_val"),
        std::out_of_range,
        "Option 'set_string_val' does not exist in group 'mysqld'.");
    cfg_handler_no_mysql_section.set(
        "set_string_val", mysqlshdk::utils::nullable<std::string>("val"));
    EXPECT_STREQ(
        (*(cfg_handler_no_mysql_section.get_string("set_string_val"))).c_str(),
        "val");
  }
  // when the value already exists
  {
    // Test set with bool
    SCOPED_TRACE("Testing set method with existing bool");
    EXPECT_TRUE(*(cfg_handler.get_bool("bool-true3")));
    cfg_handler.set("bool-true3", mysqlshdk::utils::nullable<bool>(false));
    EXPECT_FALSE(*(cfg_handler.get_bool("bool-true3")));
    cfg_handler.set("bool-true3", mysqlshdk::utils::nullable<bool>(true));
    EXPECT_TRUE(*(cfg_handler.get_bool("bool-true3")));
  }
  {
    // test set int
    SCOPED_TRACE("Testing set method with existing int");
    EXPECT_EQ(*(cfg_handler.get_int("positive-int")), 123456);
    cfg_handler.set("positive-int", mysqlshdk::utils::nullable<int64_t>(-25));
    EXPECT_EQ(*(cfg_handler.get_int("positive-int")), -25);
  }
  {
    // test set string
    SCOPED_TRACE("Testing set method with existing string");
    EXPECT_STREQ((*(cfg_handler.get_string("space"))).c_str(), " ");
    cfg_handler.set("space",
                    mysqlshdk::utils::nullable<std::string>("hey space"));
    EXPECT_STREQ((*(cfg_handler.get_string("space"))).c_str(), "hey space");
  }
  {
    // setting a value to null
    SCOPED_TRACE("Testing set method with null value");
    cfg_handler.set("null", mysqlshdk::utils::nullable<std::string>());
    EXPECT_TRUE(cfg_handler.get_bool("null").is_null());
    EXPECT_TRUE(cfg_handler.get_int("null").is_null());
    EXPECT_TRUE(cfg_handler.get_string("null").is_null());
  }
}

TEST_F(Config_file_handler_test, test_apply) {
  std::string res_cfg_path =
      shcore::path::join_path(m_tmpdir, "config_handler_apply_test.cnf");
  {
    SCOPED_TRACE(
        "Testing apply method without output cnf path on the constructor");
    // Create a copy of the original test file to always keep it unchanged.
    shcore::copy_file(m_base_cfg_path, res_cfg_path);

    Config_file_handler cfg_handler =
        Config_file_handler("uuid1", res_cfg_path, res_cfg_path);

    // set some existing options
    cfg_handler.set("bool-true3", mysqlshdk::utils::nullable<bool>(false));
    cfg_handler.set("positive-int", mysqlshdk::utils::nullable<int64_t>(-25));
    cfg_handler.set("space",
                    mysqlshdk::utils::nullable<std::string>("hey space"));
    cfg_handler.set("binlog", mysqlshdk::utils::nullable<std::string>());

    // set new options
    cfg_handler.set("new_bool_true", mysqlshdk::utils::nullable<bool>(true));
    cfg_handler.set("new_negative_int",
                    mysqlshdk::utils::nullable<int64_t>(-93));
    cfg_handler.set("new_positive_int",
                    mysqlshdk::utils::nullable<int64_t>(93));

    cfg_handler.set("null", mysqlshdk::utils::nullable<std::string>());
    // open the file on another handler and check changes have not been
    // persisted before the apply
    Config_file_handler cfg_handler_cbefore =
        Config_file_handler("uuid1", res_cfg_path, res_cfg_path);
    EXPECT_TRUE(*(cfg_handler_cbefore.get_bool("bool_true3")));
    EXPECT_EQ(*(cfg_handler_cbefore.get_int("positive_int")), 123456);
    EXPECT_STREQ((*(cfg_handler_cbefore.get_string("space"))).c_str(), " ");
    EXPECT_STREQ((*(cfg_handler_cbefore.get_string("binlog"))).c_str(), "True");
    EXPECT_THROW_LIKE(
        cfg_handler_cbefore.get_bool("new_bool_value"), std::out_of_range,
        "Option 'new_bool_value' does not exist in group 'mysqld'.");
    EXPECT_THROW_LIKE(
        cfg_handler_cbefore.get_int("new_negative_int"), std::out_of_range,
        "Option 'new_negative_int' does not exist in group 'mysqld'.");
    EXPECT_THROW_LIKE(
        cfg_handler_cbefore.get_int("new_positive_int"), std::out_of_range,
        "Option 'new_positive_int' does not exist in group 'mysqld'.");
    EXPECT_THROW_LIKE(cfg_handler_cbefore.get_string("null"), std::out_of_range,
                      "Option 'null' does not exist in group 'mysqld'.");

    cfg_handler.apply();
    // open the file on another handler and check changes have been persisted
    // after the apply
    Config_file_handler cfg_handler_cafter =
        Config_file_handler("uuid1", res_cfg_path, res_cfg_path);
    EXPECT_EQ(*(cfg_handler_cafter.get_bool("bool_true3")),
              *(cfg_handler.get_bool("bool_true3")));
    EXPECT_EQ(*(cfg_handler_cafter.get_int("positive_int")),
              *(cfg_handler.get_int("positive_int")));
    EXPECT_EQ(*(cfg_handler_cafter.get_string("space")),
              *(cfg_handler.get_string("space")));
    EXPECT_EQ((cfg_handler_cafter.get_string("binlog")).is_null(),
              (cfg_handler.get_string("binlog")).is_null());
    EXPECT_EQ(*(cfg_handler_cafter.get_bool("new_bool_true")),
              *(cfg_handler.get_bool("new_bool_true")));
    EXPECT_EQ(*(cfg_handler_cafter.get_int("new_negative_int")),
              *(cfg_handler.get_int("new_negative_int")));
    EXPECT_EQ(*(cfg_handler_cafter.get_int("new_positive_int")),
              *(cfg_handler.get_int("new_positive_int")));
    EXPECT_EQ((cfg_handler_cafter.get_string("null")).is_null(),
              (cfg_handler.get_string("null")).is_null());

    // Check that the other sections were not deleted after the apply
    // (BUG#29349014)
    // Note that the mysqld section is already being tested above
    Config_file cfg_file_cafter = Config_file();
    cfg_file_cafter.read(res_cfg_path);
    std::vector<std::string> groups = cfg_file_cafter.groups();
    EXPECT_EQ(groups.size(), 3);
    EXPECT_THAT(groups, UnorderedElementsAre("mysqld", "client", "group1"));
    EXPECT_EQ(cfg_file_cafter.options("client").size(), 1);
    EXPECT_EQ(cfg_file_cafter.get("client", "port"), "3306");
    EXPECT_EQ(cfg_file_cafter.options("group1").size(), 1);
    EXPECT_EQ(cfg_file_cafter.get("group1", "option"), "value");

    // Delete configuration file copy we created
    shcore::delete_file(res_cfg_path, true);
  }

  {
    SCOPED_TRACE("Testing apply method with input and output cnf paths");
    // Create a copy of the original test file to always keep it unchanged.
    shcore::copy_file(m_base_cfg_path, res_cfg_path);
    std::string new_res_cfg_path =
        shcore::path::join_path(m_tmpdir, "config_handler_apply_test_new.cnf");
    // delete file if it exists to make sure the test will create a new one
    shcore::delete_file(new_res_cfg_path, true);

    Config_file_handler cfg_handler_out =
        Config_file_handler("uuid1", res_cfg_path, new_res_cfg_path);

    // set some existing options
    cfg_handler_out.set("bool-true3", mysqlshdk::utils::nullable<bool>(false));
    cfg_handler_out.set("positive-int",
                        mysqlshdk::utils::nullable<int64_t>(-25));
    cfg_handler_out.set("space",
                        mysqlshdk::utils::nullable<std::string>("hey space"));
    cfg_handler_out.set("binlog", mysqlshdk::utils::nullable<std::string>());

    // set new options
    cfg_handler_out.set("new_bool_true",
                        mysqlshdk::utils::nullable<bool>(true));
    cfg_handler_out.set("new_negative_int",
                        mysqlshdk::utils::nullable<int64_t>(-93));
    cfg_handler_out.set("new_positive_int",
                        mysqlshdk::utils::nullable<int64_t>(93));

    cfg_handler_out.set("null", mysqlshdk::utils::nullable<std::string>());

    // apply changes
    cfg_handler_out.apply();
    // Check that after apply, original configuration file (first argument
    // to the constructor) was not changed.
    Config_file_handler cfg_handler_original =
        Config_file_handler("uuid1", res_cfg_path, res_cfg_path);
    EXPECT_TRUE(*(cfg_handler_original.get_bool("bool_true3")));
    EXPECT_EQ(*(cfg_handler_original.get_int("positive_int")), 123456);
    EXPECT_STREQ((*(cfg_handler_original.get_string("space"))).c_str(), " ");
    EXPECT_STREQ((*(cfg_handler_original.get_string("binlog"))).c_str(),
                 "True");
    EXPECT_THROW_LIKE(
        cfg_handler_original.get_bool("new_bool_value"), std::out_of_range,
        "Option 'new_bool_value' does not exist in group 'mysqld'.");
    EXPECT_THROW_LIKE(
        cfg_handler_original.get_int("new_negative_int"), std::out_of_range,
        "Option 'new_negative_int' does not exist in group 'mysqld'.");
    EXPECT_THROW_LIKE(
        cfg_handler_original.get_int("new_positive_int"), std::out_of_range,
        "Option 'new_positive_int' does not exist in group 'mysqld'.");
    EXPECT_THROW_LIKE(cfg_handler_original.get_string("null"),
                      std::out_of_range,
                      "Option 'null' does not exist in group 'mysqld'.");
    // Check that changes have been applied at the output cnf file
    Config_file_handler cfg_handler_out_cafter =
        Config_file_handler("uuid1", new_res_cfg_path, new_res_cfg_path);
    EXPECT_EQ(*(cfg_handler_out_cafter.get_bool("bool_true3")),
              *(cfg_handler_out.get_bool("bool_true3")));
    EXPECT_EQ(*(cfg_handler_out_cafter.get_int("positive_int")),
              *(cfg_handler_out.get_int("positive_int")));
    EXPECT_EQ(*(cfg_handler_out_cafter.get_string("space")),
              *(cfg_handler_out.get_string("space")));
    EXPECT_EQ((cfg_handler_out_cafter.get_string("binlog")).is_null(),
              (cfg_handler_out.get_string("binlog")).is_null());
    EXPECT_EQ(*(cfg_handler_out_cafter.get_bool("new_bool_true")),
              *(cfg_handler_out.get_bool("new_bool_true")));
    EXPECT_EQ(*(cfg_handler_out_cafter.get_int("new_negative_int")),
              *(cfg_handler_out.get_int("new_negative_int")));
    EXPECT_EQ(*(cfg_handler_out_cafter.get_int("new_positive_int")),
              *(cfg_handler_out.get_int("new_positive_int")));
    EXPECT_EQ((cfg_handler_out_cafter.get_string("null")).is_null(),
              (cfg_handler_out.get_string("null")).is_null());

    // Delete configuration files that were created
    shcore::delete_file(res_cfg_path, true);
    shcore::delete_file(new_res_cfg_path, true);
  }
}

TEST_F(Config_file_handler_test, test_remove) {
  using mysqlshdk::utils::nullable;
  {
    SCOPED_TRACE("Testing removing an option from 'mysqld' group.");
    Config_file_handler cfg_handler =
        Config_file_handler("uuid1", m_base_cfg_path, m_base_cfg_path);

    cfg_handler.remove("option_to_delete_with_value");
    EXPECT_THROW_LIKE(cfg_handler.get_string("option_to_delete_with_value"),
                      std::out_of_range,
                      "Option 'option_to_delete_with_value' does not exist in "
                      "group 'mysqld'.");
    cfg_handler.remove("option_to_delete_without_value");
    EXPECT_THROW_LIKE(
        cfg_handler.get_string("option_to_delete_without_value"),
        std::out_of_range,
        "Option 'option_to_delete_without_value' does not exist in "
        "group 'mysqld'.");
  }
  {
    SCOPED_TRACE(
        "Testing removing an option if 'mysqld' group does not exist.");
    Config_file_handler cfg_handler_no_mysqld_grp = Config_file_handler(
        "uuid1", m_no_mysql_section_cfg_path, m_no_mysql_section_cfg_path);

    EXPECT_THROW_LIKE(
        cfg_handler_no_mysqld_grp.remove("option_to_delete_with_value"),
        std::out_of_range, "Group 'mysqld' does not exist.");
    EXPECT_THROW_LIKE(
        cfg_handler_no_mysqld_grp.remove("option_to_delete_without_value"),
        std::out_of_range, "Group 'mysqld' does not exist.");
  }
}

TEST_F(Config_file_handler_test, test_set_now) {
  SCOPED_TRACE("Testing set_now() function");
  std::string res_cfg_path =
      shcore::path::join_path(m_tmpdir, "config_handler_set_now_test.cnf");

  // Create a config file handler with a non-existing output file.
  Config_file_handler cfg_handler = Config_file_handler("abc1", res_cfg_path);

  // Set new options immediately (do not wait for apply()).
  cfg_handler.set_now("bool_true", nullable<bool>(true));
  cfg_handler.set_now("positive-int", nullable<int64_t>(25));
  cfg_handler.set_now("negative_int", nullable<int64_t>(-93));
  cfg_handler.set_now("space", nullable<std::string>("hey space"));
  cfg_handler.set_now("null", nullable<std::string>());
  EXPECT_EQ("abc1", cfg_handler.get_server_uuid());

  // Open the file on another handler and confirm changes have been applied.
  Config_file_handler cfg_handler_res =
      Config_file_handler("abc2", res_cfg_path, res_cfg_path);
  nullable<bool> bool_val = cfg_handler_res.get_bool("bool_true");
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_TRUE(*bool_val);
  nullable<int64_t> int_val = cfg_handler_res.get_int("positive_int");
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(25, *int_val);
  int_val = cfg_handler_res.get_int("negative_int");
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(-93, *int_val);
  nullable<std::string> str_val = cfg_handler_res.get_string("space");
  EXPECT_FALSE(str_val.is_null());
  EXPECT_STREQ("hey space", (*str_val).c_str());
  str_val = cfg_handler_res.get_string("null");
  EXPECT_TRUE(str_val.is_null());
  EXPECT_EQ("abc2", cfg_handler_res.get_server_uuid());

  // Set some existing options immediately (do not wait for apply()).
  cfg_handler.set_now("bool_true", nullable<bool>(false));
  cfg_handler.set_now("positive-int", nullable<int64_t>(52));
  cfg_handler.set_now("negative_int", nullable<int64_t>(-39));
  cfg_handler.set_now("space", nullable<std::string>("hey still space"));
  cfg_handler.set_now("null", nullable<std::string>());

  // Open the file on another handler and confirm changes have been applied.
  cfg_handler_res = Config_file_handler("abc3", res_cfg_path, res_cfg_path);
  bool_val = cfg_handler_res.get_bool("bool_true");
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_FALSE(*bool_val);
  int_val = cfg_handler_res.get_int("positive_int");
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(52, *int_val);
  int_val = cfg_handler_res.get_int("negative_int");
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(-39, *int_val);
  str_val = cfg_handler_res.get_string("space");
  EXPECT_FALSE(str_val.is_null());
  EXPECT_STREQ("hey still space", (*str_val).c_str());
  str_val = cfg_handler_res.get_string("null");
  EXPECT_TRUE(str_val.is_null());
  EXPECT_EQ("abc3", cfg_handler_res.get_server_uuid());

  // Delete option file that was created.
  shcore::delete_file(res_cfg_path, true);
}

}  // namespace testing