/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <memory>

#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

using mysqlshdk::config::Config_server_handler;
using mysqlshdk::mysql::Var_qualifier;
using mysqlshdk::utils::nullable;

namespace testing {

class Config_server_handler_test : public tests::Shell_base_test {
 protected:
  void SetUp() {
    tests::Shell_base_test::SetUp();

    // Create instance and Open the session for the tests.
    m_connection_options = shcore::get_connection_options(_mysql_uri);
    m_session->connect(m_connection_options);
  }

  void TearDown() {
    tests::Shell_base_test::TearDown();

    // Close the session.
    m_session->close();
  }

  std::shared_ptr<mysqlshdk::db::ISession> m_session =
      mysqlshdk::db::mysql::Session::create();
  mysqlshdk::db::Connection_options m_connection_options;
};

TEST_F(Config_server_handler_test, config_interface_global_vars) {
  mysqlshdk::mysql::Instance instance(m_session);
  Config_server_handler cfg_h(&instance, Var_qualifier::GLOBAL);

  // Test getting a bool value (default: false).
  SCOPED_TRACE("Test get_bool().");
  nullable<bool> bool_val = cfg_h.get_bool("sql_warnings");
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_FALSE(*bool_val);
  bool_val = cfg_h.get_bool("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_FALSE(*bool_val);

  // Test setting a bool value.
  SCOPED_TRACE("Test set() of a bool.");
  cfg_h.set("sql_warnings", nullable<bool>(true));
  bool_val = cfg_h.get_bool("sql_warnings");
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_TRUE(*bool_val);
  bool_val = cfg_h.get_bool("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_TRUE(*bool_val);

  // No changes applied yet to bool : get_now() return the initial value).
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_FALSE(*bool_val);

  // Test getting a int value (use current server value as default).
  SCOPED_TRACE("Test get_int().");
  nullable<int64_t> wait_timeout =
      instance.get_sysvar_int("wait_timeout", Var_qualifier::GLOBAL);
  nullable<int64_t> int_val = cfg_h.get_int("wait_timeout");
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(*wait_timeout, *int_val);
  int_val = cfg_h.get_int("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(*wait_timeout, *int_val);

  // Test setting a int value.
  SCOPED_TRACE("Test set() of a int.");
  cfg_h.set("wait_timeout", nullable<int64_t>(30000));
  int_val = cfg_h.get_int("wait_timeout");
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(30000, *int_val);
  int_val = cfg_h.get_int("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(30000, *int_val);

  // No changes applied yet to int: get_now() return the initial value).
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(*wait_timeout, *int_val);

  // Test getting a string value (default: en_US).
  SCOPED_TRACE("Test get_string().");
  nullable<std::string> string_val = cfg_h.get_string("lc_messages");
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("en_US", (*string_val).c_str());
  string_val = cfg_h.get_string("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("en_US", (*string_val).c_str());

  // Test setting a string value.
  SCOPED_TRACE("Test set() of a string.");
  cfg_h.set("lc_messages", nullable<std::string>("pt_PT"));
  string_val = cfg_h.get_string("lc_messages");
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());
  string_val = cfg_h.get_string("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());

  // No changes applied yet to string: get_now() return the initial value).
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("en_US", (*string_val).c_str());

  // Apply all changes at once.
  SCOPED_TRACE("Test apply().");
  cfg_h.apply();

  // Verify changes getting value directly from the server, using get_now().
  SCOPED_TRACE("Verify changes were applied using get_now().");
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_TRUE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(30000, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());

  // Verify that values with a different scope (i.e., SESSION) were not changed.
  SCOPED_TRACE("Verify SESSION values were not changed.");
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_FALSE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(*wait_timeout, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("en_US", (*string_val).c_str());

  // Changes values with a different scope  (use delay 1 ms for one of them).
  SCOPED_TRACE("Change settings with SESSION scope: set() and apply().");
  cfg_h.set("sql_warnings", nullable<bool>(true), Var_qualifier::SESSION, 1);
  bool_val = cfg_h.get_bool("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_TRUE(*bool_val);
  cfg_h.set("wait_timeout", nullable<int64_t>(30000), Var_qualifier::SESSION);
  int_val = cfg_h.get_int("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(30000, *int_val);
  cfg_h.set("lc_messages", nullable<std::string>("pt_PT"),
            Var_qualifier::SESSION);
  string_val = cfg_h.get_string("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());
  cfg_h.apply();

  // Verify that values with a different scope (i.e., SESSION) were changed.
  SCOPED_TRACE("Verify SESSION values were now changed.");
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_TRUE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(30000, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());

  // Restore previous setting directly on the server, using set_now().
  // NOTE: Use 1 ms delay to set one of the variables.
  SCOPED_TRACE("Restore initial settings using set_now().");
  cfg_h.set_now("sql_warnings", nullable<bool>(false), Var_qualifier::GLOBAL,
                1);
  cfg_h.set_now("wait_timeout", wait_timeout, Var_qualifier::GLOBAL);
  cfg_h.set_now("lc_messages", nullable<std::string>("en_US"),
                Var_qualifier::GLOBAL);
  cfg_h.set_now("sql_warnings", nullable<bool>(false), Var_qualifier::SESSION,
                1);
  cfg_h.set_now("wait_timeout", wait_timeout, Var_qualifier::SESSION);
  cfg_h.set_now("lc_messages", nullable<std::string>("en_US"),
                Var_qualifier::SESSION);

  // Confirm values where restored properly.
  SCOPED_TRACE("Verify initail settings were restored.");
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_FALSE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(*wait_timeout, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("en_US", (*string_val).c_str());
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_FALSE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(*wait_timeout, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("en_US", (*string_val).c_str());

  // Test get_default_var_qualifier(), must be GLOBAL.
  EXPECT_EQ(Var_qualifier::GLOBAL, cfg_h.get_default_var_qualifier());
}

TEST_F(Config_server_handler_test, config_interface_session_vars) {
  mysqlshdk::mysql::Instance instance(m_session);
  Config_server_handler cfg_h(&instance, Var_qualifier::SESSION);

  // Test getting a bool value (default: false).
  SCOPED_TRACE("Test get_bool().");
  nullable<bool> bool_val = cfg_h.get_bool("sql_warnings");
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_FALSE(*bool_val);
  bool_val = cfg_h.get_bool("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_FALSE(*bool_val);

  // Test setting a bool value.
  SCOPED_TRACE("Test set() of a bool.");
  cfg_h.set("sql_warnings", nullable<bool>(true));
  bool_val = cfg_h.get_bool("sql_warnings");
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_TRUE(*bool_val);
  bool_val = cfg_h.get_bool("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_TRUE(*bool_val);

  // No changes applied yet to bool : get_now() return the initial value).
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_FALSE(*bool_val);

  // Test getting a int value (use current server value as default).
  SCOPED_TRACE("Test get_int().");
  nullable<int64_t> wait_timeout =
      instance.get_sysvar_int("wait_timeout", Var_qualifier::SESSION);
  nullable<int64_t> int_val = cfg_h.get_int("wait_timeout");
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(*wait_timeout, *int_val);
  int_val = cfg_h.get_int("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(*wait_timeout, *int_val);

  // Test setting a int value.
  SCOPED_TRACE("Test set() of a int.");
  cfg_h.set("wait_timeout", nullable<int64_t>(30000));
  int_val = cfg_h.get_int("wait_timeout");
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(30000, *int_val);
  int_val = cfg_h.get_int("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(30000, *int_val);

  // No changes applied yet to int: get_now() return the initial value).
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(*wait_timeout, *int_val);

  // Test getting a string value (default: en_US).
  SCOPED_TRACE("Test get_string().");
  nullable<std::string> string_val = cfg_h.get_string("lc_messages");
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("en_US", (*string_val).c_str());
  string_val = cfg_h.get_string("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("en_US", (*string_val).c_str());

  // Test setting a string value.
  SCOPED_TRACE("Test set() of a string.");
  cfg_h.set("lc_messages", nullable<std::string>("pt_PT"));
  string_val = cfg_h.get_string("lc_messages");
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());
  string_val = cfg_h.get_string("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());

  // No changes applied yet to string: get_now() return the initial value).
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("en_US", (*string_val).c_str());

  // Apply all changes at once.
  SCOPED_TRACE("Test apply().");
  cfg_h.apply();

  // Verify changes getting value directly from the server, using get_now().
  SCOPED_TRACE("Verify changes were applied using get_now().");
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_TRUE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(30000, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());

  // Verify that values with a different scope (i.e., GLOBAL) were not changed.
  SCOPED_TRACE("Verify GLOBAL values were not changed.");
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_FALSE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(*wait_timeout, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("en_US", (*string_val).c_str());

  // Changes values with a different scope (use delay 1 ms for one of them).
  SCOPED_TRACE("Change settings with GLOBAL scope: set() and apply().");
  cfg_h.set("sql_warnings", nullable<bool>(true), Var_qualifier::GLOBAL);
  bool_val = cfg_h.get_bool("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_TRUE(*bool_val);
  cfg_h.set("wait_timeout", nullable<int64_t>(30000), Var_qualifier::GLOBAL, 1);
  int_val = cfg_h.get_int("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(30000, *int_val);
  cfg_h.set("lc_messages", nullable<std::string>("pt_PT"),
            Var_qualifier::GLOBAL);
  string_val = cfg_h.get_string("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());
  cfg_h.apply();

  // Verify that values with a different scope (i.e., GLOBAL) were changed.
  SCOPED_TRACE("Verify GLOBAL values were now changed.");
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_TRUE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(30000, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());

  // Restore previous setting directly on the server, using set_now().
  // NOTE: Use 1 ms delay to set one of the variables.
  SCOPED_TRACE("Restore initial settings using set_now().");
  cfg_h.set_now("sql_warnings", nullable<bool>(false), Var_qualifier::SESSION);
  cfg_h.set_now("wait_timeout", wait_timeout, Var_qualifier::SESSION, 1);
  cfg_h.set_now("lc_messages", nullable<std::string>("en_US"),
                Var_qualifier::SESSION);
  cfg_h.set_now("sql_warnings", nullable<bool>(false), Var_qualifier::GLOBAL);
  cfg_h.set_now("wait_timeout", wait_timeout, Var_qualifier::GLOBAL, 1);
  cfg_h.set_now("lc_messages", nullable<std::string>("en_US"),
                Var_qualifier::GLOBAL);

  // Confirm values where restored properly.
  SCOPED_TRACE("Verify initail settings were restored.");
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_FALSE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(*wait_timeout, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("en_US", (*string_val).c_str());
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_FALSE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(*wait_timeout, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(string_val.is_null());
  EXPECT_STREQ("en_US", (*string_val).c_str());

  // Test get_default_var_qualifier(), must be SESSION.
  EXPECT_EQ(Var_qualifier::SESSION, cfg_h.get_default_var_qualifier());
}

TEST_F(Config_server_handler_test, errors) {
  mysqlshdk::mysql::Instance instance(m_session);
  Config_server_handler cfg_h(&instance, Var_qualifier::GLOBAL);

  // Error if trying to get a variable that does not exists.
  EXPECT_THROW_LIKE(cfg_h.get_bool("not-exist"), std::out_of_range,
                    "Variable 'not-exist' does not exist.");
  EXPECT_THROW_LIKE(cfg_h.get_int("not-exist"), std::out_of_range,
                    "Variable 'not-exist' does not exist.");
  EXPECT_THROW_LIKE(cfg_h.get_string("not-exist"), std::out_of_range,
                    "Variable 'not-exist' does not exist.");
  EXPECT_THROW_LIKE(cfg_h.get_bool("not-exist", Var_qualifier::SESSION),
                    std::out_of_range, "Variable 'not-exist' does not exist.");
  EXPECT_THROW_LIKE(cfg_h.get_int("not-exist", Var_qualifier::SESSION),
                    std::out_of_range, "Variable 'not-exist' does not exist.");
  EXPECT_THROW_LIKE(cfg_h.get_string("not-exist", Var_qualifier::SESSION),
                    std::out_of_range, "Variable 'not-exist' does not exist.");
  EXPECT_THROW_LIKE(cfg_h.get_bool_now("not-exist", Var_qualifier::GLOBAL),
                    std::out_of_range, "Variable 'not-exist' does not exist.");
  EXPECT_THROW_LIKE(cfg_h.get_int_now("not-exist", Var_qualifier::GLOBAL),
                    std::out_of_range, "Variable 'not-exist' does not exist.");
  EXPECT_THROW_LIKE(cfg_h.get_string_now("not-exist", Var_qualifier::GLOBAL),
                    std::out_of_range, "Variable 'not-exist' does not exist.");

  // Error if trying to set a variable with a null value.
  EXPECT_THROW_LIKE(cfg_h.set("sql_warnings", nullable<bool>()),
                    std::invalid_argument,
                    "The value parameter cannot be null.");
  EXPECT_THROW_LIKE(cfg_h.set("wait_timeout", nullable<int64_t>()),
                    std::invalid_argument,
                    "The value parameter cannot be null.");
  EXPECT_THROW_LIKE(cfg_h.set("lc_messages", nullable<std::string>()),
                    std::invalid_argument,
                    "The value parameter cannot be null.");
  EXPECT_THROW_LIKE(
      cfg_h.set("sql_warnings", nullable<bool>(), Var_qualifier::SESSION),
      std::invalid_argument, "The value parameter cannot be null.");
  EXPECT_THROW_LIKE(
      cfg_h.set("wait_timeout", nullable<int64_t>(), Var_qualifier::SESSION),
      std::invalid_argument, "The value parameter cannot be null.");
  EXPECT_THROW_LIKE(
      cfg_h.set("lc_messages", nullable<std::string>(), Var_qualifier::SESSION),
      std::invalid_argument, "The value parameter cannot be null.");
  EXPECT_THROW_LIKE(
      cfg_h.set_now("sql_warnings", nullable<bool>(), Var_qualifier::GLOBAL),
      std::invalid_argument, "The value parameter cannot be null.");
  EXPECT_THROW_LIKE(
      cfg_h.set_now("wait_timeout", nullable<int64_t>(), Var_qualifier::GLOBAL),
      std::invalid_argument, "The value parameter cannot be null.");
  EXPECT_THROW_LIKE(cfg_h.set_now("lc_messages", nullable<std::string>(),
                                  Var_qualifier::GLOBAL),
                    std::invalid_argument,
                    "The value parameter cannot be null.");

  // Error if trying to immediately set a variable that does not exists.
  EXPECT_THROW_LIKE(
      cfg_h.set_now("not-exist", nullable<bool>(true), Var_qualifier::GLOBAL),
      mysqlshdk::db::Error, "Unknown system variable 'not-exist'");
  EXPECT_THROW_LIKE(cfg_h.set_now("not-exist", nullable<int64_t>(1234),
                                  Var_qualifier::GLOBAL),
                    mysqlshdk::db::Error,
                    "Unknown system variable 'not-exist'");
  EXPECT_THROW_LIKE(cfg_h.set_now("not-exist", nullable<std::string>("mystr"),
                                  Var_qualifier::GLOBAL),
                    mysqlshdk::db::Error,
                    "Unknown system variable 'not-exist'");

  // Error if trying to apply a variable that does not exists.
  cfg_h.set("not-exist-bool", nullable<bool>(true));
  EXPECT_THROW_LIKE(cfg_h.apply(), mysqlshdk::db::Error,
                    "Unknown system variable 'not-exist-bool'");
  Config_server_handler cfg_h2(&instance, Var_qualifier::GLOBAL);
  cfg_h2.set("not-exist-int", nullable<int64_t>(1234));
  EXPECT_THROW_LIKE(cfg_h2.apply(), mysqlshdk::db::Error,
                    "Unknown system variable 'not-exist-int'");
  Config_server_handler cfg_h3(&instance, Var_qualifier::GLOBAL);
  cfg_h3.set("not-exist-string", nullable<std::string>("mystr"));
  EXPECT_THROW_LIKE(cfg_h3.apply(), mysqlshdk::db::Error,
                    "Unknown system variable 'not-exist-string'");
}

TEST_F(Config_server_handler_test, apply) {
  // Test that variables are applied in the same order they are set.
  // NOTE: Using gtid_mode (default: OFF) to test since there is an order
  // dependency to change it: OFF <-> OFF_PERMISSIVE <-> ON_PERMISSIVE <-> ON
  mysqlshdk::mysql::Instance instance(m_session);
  Config_server_handler cfg_h(&instance, Var_qualifier::GLOBAL);

  // Verify if the GTID_MODE is the one expected to run the test else skip test.
  nullable<std::string> init_gtid_mode =
      instance.get_sysvar_string("gtid_mode");
  if (*init_gtid_mode != "OFF") {
    SKIP_TEST("Test server must have GTID_MODE=OFF, current value: " +
              *init_gtid_mode);
  }

  // Cannot change gtid_mode directly from OFF to ON
  cfg_h.set("gtid_mode", nullable<std::string>("ON"));
  EXPECT_THROW_LIKE(cfg_h.apply(), mysqlshdk::db::Error,
                    "The value of @@GLOBAL.GTID_MODE can only be changed one "
                    "step at a time: OFF <-> OFF_PERMISSIVE <-> ON_PERMISSIVE "
                    "<-> ON.");

  // Enable gtid_mode following the allowed steps order.
  Config_server_handler cfg_h2(&instance, Var_qualifier::GLOBAL);
  cfg_h2.set("gtid_mode", nullable<std::string>("OFF_PERMISSIVE"));
  cfg_h2.set("gtid_mode", nullable<std::string>("ON_PERMISSIVE"));
  cfg_h2.set("enforce_gtid_consistency", nullable<std::string>("ON"));
  cfg_h2.set("gtid_mode", nullable<std::string>("ON"));
  cfg_h2.apply();

  nullable<std::string> gtid_mode =
      instance.get_sysvar_string("gtid_mode", Var_qualifier::GLOBAL);
  EXPECT_FALSE(gtid_mode.is_null());
  EXPECT_STREQ("ON", (*gtid_mode).c_str());

  // Error if the wrong order is used.
  cfg_h2.set("gtid_mode", nullable<std::string>("OFF_PERMISSIVE"));
  cfg_h2.set("enforce_gtid_consistency", nullable<std::string>("OFF"));
  cfg_h2.set("gtid_mode", nullable<std::string>("ON_PERMISSIVE"));
  cfg_h2.set("gtid_mode", nullable<std::string>("OFF"));
  EXPECT_THROW_LIKE(cfg_h2.apply(), mysqlshdk::db::Error,
                    "The value of @@GLOBAL.GTID_MODE can only be changed one "
                    "step at a time: OFF <-> OFF_PERMISSIVE <-> ON_PERMISSIVE "
                    "<-> ON.");

  // Disable gtid_mode (back to OFF) following the allowed steps order.
  Config_server_handler cfg_h3(&instance, Var_qualifier::GLOBAL);
  cfg_h3.set("gtid_mode", nullable<std::string>("ON_PERMISSIVE"));
  cfg_h3.set("enforce_gtid_consistency", nullable<std::string>("OFF"));
  cfg_h3.set("gtid_mode", nullable<std::string>("OFF_PERMISSIVE"));
  cfg_h3.set("gtid_mode", nullable<std::string>("OFF"));
  cfg_h3.apply();

  gtid_mode = instance.get_sysvar_string("gtid_mode", Var_qualifier::GLOBAL);
  EXPECT_FALSE(gtid_mode.is_null());
  EXPECT_STREQ("OFF", (*gtid_mode).c_str());
}

}  // namespace testing
