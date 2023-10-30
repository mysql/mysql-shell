/*
 * Copyright (c) 2018, 2023, Oracle and/or its affiliates.
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

namespace testing {

class Config_server_handler_test : public tests::Shell_base_test {
 protected:
  void SetUp() {
    tests::Shell_base_test::SetUp();

    // Create instance and Open the session for the tests.
    m_connection_options = shcore::get_connection_options(_mysql_uri);
    m_session->connect(m_connection_options);

    m_report_host =
        m_session->query("SELECT coalesce(@@report_host, @@hostname)")
            ->fetch_one()
            ->get_string(0);
  }

  void TearDown() {
    tests::Shell_base_test::TearDown();

    // Close the session.
    m_session->close();
  }

  std::shared_ptr<mysqlshdk::db::ISession> m_session =
      mysqlshdk::db::mysql::Session::create();
  mysqlshdk::db::Connection_options m_connection_options;

  std::string m_report_host;
};

TEST_F(Config_server_handler_test, config_interface_global_vars) {
  mysqlshdk::mysql::Instance instance(m_session);
  Config_server_handler cfg_h(&instance, Var_qualifier::GLOBAL);

  // Test getting a bool value (default: false).
  SCOPED_TRACE("Test get_bool().");
  std::optional<bool> bool_val = cfg_h.get_bool("sql_warnings");
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_FALSE(*bool_val);
  bool_val = cfg_h.get_bool("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_FALSE(*bool_val);

  // Test setting a bool value.
  SCOPED_TRACE("Test set() of a bool.");
  cfg_h.set("sql_warnings", std::optional<bool>(true));
  bool_val = cfg_h.get_bool("sql_warnings");
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_TRUE(*bool_val);
  bool_val = cfg_h.get_bool("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_TRUE(*bool_val);

  // No changes applied yet to bool : get_now() return the initial value).
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_FALSE(*bool_val);

  // Test getting a int value (use current server value as default).
  SCOPED_TRACE("Test get_int().");
  std::optional<int64_t> wait_timeout =
      instance.get_sysvar_int("wait_timeout", Var_qualifier::GLOBAL);
  std::optional<int64_t> int_val = cfg_h.get_int("wait_timeout");
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(*wait_timeout, *int_val);
  int_val = cfg_h.get_int("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(*wait_timeout, *int_val);

  // Test setting a int value.
  SCOPED_TRACE("Test set() of a int.");
  cfg_h.set("wait_timeout", std::optional<int64_t>(30000));
  int_val = cfg_h.get_int("wait_timeout");
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(30000, *int_val);
  int_val = cfg_h.get_int("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(30000, *int_val);

  // No changes applied yet to int: get_now() return the initial value).
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(*wait_timeout, *int_val);

  // Test getting a string value (default: en_US).
  SCOPED_TRACE("Test get_string().");
  std::optional<std::string> string_val = cfg_h.get_string("lc_messages");
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("en_US", (*string_val).c_str());
  string_val = cfg_h.get_string("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("en_US", (*string_val).c_str());

  // Test setting a string value.
  SCOPED_TRACE("Test set() of a string.");
  cfg_h.set("lc_messages", std::optional<std::string>("pt_PT"));
  string_val = cfg_h.get_string("lc_messages");
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());
  string_val = cfg_h.get_string("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());

  // No changes applied yet to string: get_now() return the initial value).
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("en_US", (*string_val).c_str());

  // Apply all changes at once.
  SCOPED_TRACE("Test apply().");
  cfg_h.apply();

  // Verify changes getting value directly from the server, using get_now().
  SCOPED_TRACE("Verify changes were applied using get_now().");
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_TRUE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(30000, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());

  // Verify that values with a different scope (i.e., SESSION) were not changed.
  SCOPED_TRACE("Verify SESSION values were not changed.");
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_FALSE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(*wait_timeout, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("en_US", (*string_val).c_str());

  // Changes values with a different scope  (use delay 1 ms for one of them).
  SCOPED_TRACE("Change settings with SESSION scope: set() and apply().");
  cfg_h.set("sql_warnings", std::optional<bool>(true), Var_qualifier::SESSION,
            std::chrono::milliseconds{1});
  bool_val = cfg_h.get_bool("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_TRUE(*bool_val);
  cfg_h.set("wait_timeout", std::optional<int64_t>(30000),
            Var_qualifier::SESSION);
  int_val = cfg_h.get_int("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(30000, *int_val);
  cfg_h.set("lc_messages", std::optional<std::string>("pt_PT"),
            Var_qualifier::SESSION);
  string_val = cfg_h.get_string("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());
  cfg_h.apply();

  // Verify that values with a different scope (i.e., SESSION) were changed.
  SCOPED_TRACE("Verify SESSION values were now changed.");
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_TRUE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(30000, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());

  // Restore previous setting directly on the server.
  SCOPED_TRACE("Restore initial settings.");
  cfg_h.set("sql_warnings", std::optional<bool>(false), Var_qualifier::GLOBAL);
  cfg_h.set("wait_timeout", wait_timeout, Var_qualifier::GLOBAL);
  cfg_h.set("lc_messages", std::optional<std::string>("en_US"),
            Var_qualifier::GLOBAL);
  cfg_h.set("sql_warnings", std::optional<bool>(false), Var_qualifier::SESSION);
  cfg_h.set("wait_timeout", wait_timeout, Var_qualifier::SESSION);
  cfg_h.set("lc_messages", std::optional<std::string>("en_US"),
            Var_qualifier::SESSION);
  cfg_h.apply();

  // Confirm values where restored properly.
  SCOPED_TRACE("Verify initail settings were restored.");
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_FALSE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(*wait_timeout, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("en_US", (*string_val).c_str());
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_FALSE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(*wait_timeout, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("en_US", (*string_val).c_str());

  // Test get_default_var_qualifier(), must be GLOBAL.
  EXPECT_EQ(Var_qualifier::GLOBAL, cfg_h.get_default_var_qualifier());
}

TEST_F(Config_server_handler_test, config_interface_session_vars) {
  mysqlshdk::mysql::Instance instance(m_session);
  Config_server_handler cfg_h(&instance, Var_qualifier::SESSION);

  // Test getting a bool value (default: false).
  SCOPED_TRACE("Test get_bool().");
  std::optional<bool> bool_val = cfg_h.get_bool("sql_warnings");
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_FALSE(*bool_val);
  bool_val = cfg_h.get_bool("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_FALSE(*bool_val);

  // Test setting a bool value.
  SCOPED_TRACE("Test set() of a bool.");
  cfg_h.set("sql_warnings", std::optional<bool>(true));
  bool_val = cfg_h.get_bool("sql_warnings");
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_TRUE(*bool_val);
  bool_val = cfg_h.get_bool("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_TRUE(*bool_val);

  // No changes applied yet to bool : get_now() return the initial value).
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_FALSE(*bool_val);

  // Test getting a int value (use current server value as default).
  SCOPED_TRACE("Test get_int().");
  std::optional<int64_t> wait_timeout =
      instance.get_sysvar_int("wait_timeout", Var_qualifier::SESSION);
  std::optional<int64_t> int_val = cfg_h.get_int("wait_timeout");
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(*wait_timeout, *int_val);
  int_val = cfg_h.get_int("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(*wait_timeout, *int_val);

  // Test setting a int value.
  SCOPED_TRACE("Test set() of a int.");
  cfg_h.set("wait_timeout", std::optional<int64_t>(30000));
  int_val = cfg_h.get_int("wait_timeout");
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(30000, *int_val);
  int_val = cfg_h.get_int("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(30000, *int_val);

  // No changes applied yet to int: get_now() return the initial value).
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(*wait_timeout, *int_val);

  // Test getting a string value (default: en_US).
  SCOPED_TRACE("Test get_string().");
  std::optional<std::string> string_val = cfg_h.get_string("lc_messages");
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("en_US", (*string_val).c_str());
  string_val = cfg_h.get_string("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("en_US", (*string_val).c_str());

  // Test setting a string value.
  SCOPED_TRACE("Test set() of a string.");
  cfg_h.set("lc_messages", std::optional<std::string>("pt_PT"));
  string_val = cfg_h.get_string("lc_messages");
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());
  string_val = cfg_h.get_string("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());

  // No changes applied yet to string: get_now() return the initial value).
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("en_US", (*string_val).c_str());

  // Apply all changes at once.
  SCOPED_TRACE("Test apply().");
  cfg_h.apply();

  // Verify changes getting value directly from the server, using get_now().
  SCOPED_TRACE("Verify changes were applied using get_now().");
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_TRUE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(30000, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());

  // Verify that values with a different scope (i.e., GLOBAL) were not changed.
  SCOPED_TRACE("Verify GLOBAL values were not changed.");
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_FALSE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(*wait_timeout, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("en_US", (*string_val).c_str());

  // Changes values with a different scope (use delay 1 ms for one of them).
  SCOPED_TRACE("Change settings with GLOBAL scope: set() and apply().");
  cfg_h.set("sql_warnings", std::optional<bool>(true), Var_qualifier::GLOBAL);
  bool_val = cfg_h.get_bool("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_TRUE(*bool_val);
  cfg_h.set("wait_timeout", std::optional<int64_t>(30000),
            Var_qualifier::GLOBAL, std::chrono::milliseconds{1});
  int_val = cfg_h.get_int("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(30000, *int_val);
  cfg_h.set("lc_messages", std::optional<std::string>("pt_PT"),
            Var_qualifier::GLOBAL);
  string_val = cfg_h.get_string("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());
  cfg_h.apply();

  // Verify that values with a different scope (i.e., GLOBAL) were changed.
  SCOPED_TRACE("Verify GLOBAL values were now changed.");
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_TRUE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(30000, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("pt_PT", (*string_val).c_str());

  // Restore previous setting directly on the server.
  SCOPED_TRACE("Restore initial settings using set_now().");
  cfg_h.set("sql_warnings", std::optional<bool>(false), Var_qualifier::SESSION);
  cfg_h.set("wait_timeout", wait_timeout, Var_qualifier::SESSION);
  cfg_h.set("lc_messages", std::optional<std::string>("en_US"),
            Var_qualifier::SESSION);
  cfg_h.set("sql_warnings", std::optional<bool>(false), Var_qualifier::GLOBAL);
  cfg_h.set("wait_timeout", wait_timeout, Var_qualifier::GLOBAL);
  cfg_h.set("lc_messages", std::optional<std::string>("en_US"),
            Var_qualifier::GLOBAL);
  cfg_h.apply();

  // Confirm values where restored properly.
  SCOPED_TRACE("Verify initail settings were restored.");
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::SESSION);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_FALSE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::SESSION);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(*wait_timeout, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::SESSION);
  EXPECT_FALSE(!string_val.has_value());
  EXPECT_STREQ("en_US", (*string_val).c_str());
  bool_val = cfg_h.get_bool_now("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!bool_val.has_value());
  EXPECT_FALSE(*bool_val);
  int_val = cfg_h.get_int_now("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!int_val.has_value());
  EXPECT_EQ(*wait_timeout, *int_val);
  string_val = cfg_h.get_string_now("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!string_val.has_value());
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
  EXPECT_THROW_LIKE(cfg_h.set("sql_warnings", std::optional<bool>()),
                    std::invalid_argument,
                    "The value parameter cannot be null.");
  EXPECT_THROW_LIKE(cfg_h.set("wait_timeout", std::optional<int64_t>()),
                    std::invalid_argument,
                    "The value parameter cannot be null.");
  EXPECT_THROW_LIKE(cfg_h.set("lc_messages", std::optional<std::string>()),
                    std::invalid_argument,
                    "The value parameter cannot be null.");
  EXPECT_THROW_LIKE(
      cfg_h.set("sql_warnings", std::optional<bool>(), Var_qualifier::SESSION),
      std::invalid_argument, "The value parameter cannot be null.");
  EXPECT_THROW_LIKE(cfg_h.set("wait_timeout", std::optional<int64_t>(),
                              Var_qualifier::SESSION),
                    std::invalid_argument,
                    "The value parameter cannot be null.");
  EXPECT_THROW_LIKE(cfg_h.set("lc_messages", std::optional<std::string>(),
                              Var_qualifier::SESSION),
                    std::invalid_argument,
                    "The value parameter cannot be null.");
  EXPECT_THROW_LIKE(
      cfg_h.set("sql_warnings", std::optional<bool>(), Var_qualifier::GLOBAL),
      std::invalid_argument, "The value parameter cannot be null.");
  EXPECT_THROW_LIKE(cfg_h.set("wait_timeout", std::optional<int64_t>(),
                              Var_qualifier::GLOBAL),
                    std::invalid_argument,
                    "The value parameter cannot be null.");
  EXPECT_THROW_LIKE(cfg_h.set("lc_messages", std::optional<std::string>(),
                              Var_qualifier::GLOBAL),
                    std::invalid_argument,
                    "The value parameter cannot be null.");

  // Error if trying to apply a variable that does not exists.
  cfg_h.set("not-exist-bool", std::optional<bool>(true));
  EXPECT_THROW_LIKE(cfg_h.apply(true), mysqlshdk::db::Error,
                    "Unknown system variable 'not-exist-bool'");
  Config_server_handler cfg_h2(&instance, Var_qualifier::GLOBAL);
  cfg_h2.set("not-exist-int", std::optional<int64_t>(1234));
  EXPECT_THROW_LIKE(cfg_h2.apply(true), mysqlshdk::db::Error,
                    "Unknown system variable 'not-exist-int'");
  Config_server_handler cfg_h3(&instance, Var_qualifier::GLOBAL);
  cfg_h3.set("not-exist-string", std::optional<std::string>("mystr"));
  EXPECT_THROW_LIKE(cfg_h3.apply(true), mysqlshdk::db::Error,
                    "Unknown system variable 'not-exist-string'");
}

TEST_F(Config_server_handler_test, apply) {
  // Test that variables are applied in the same order they are set.
  // NOTE: Using gtid_mode (default: OFF) to test since there is an order
  // dependency to change it: OFF <-> OFF_PERMISSIVE <-> ON_PERMISSIVE <-> ON
  mysqlshdk::mysql::Instance instance(m_session);
  Config_server_handler cfg_h(&instance, Var_qualifier::GLOBAL);

  // Verify if the GTID_MODE is the one expected to run the test else skip test.
  std::optional<std::string> init_gtid_mode =
      instance.get_sysvar_string("gtid_mode", Var_qualifier::SESSION);
  if (*init_gtid_mode != "OFF") {
    SKIP_TEST("Test server must have GTID_MODE=OFF, current value: " +
              *init_gtid_mode);
  }

  // Cannot change gtid_mode directly from OFF to ON
  cfg_h.set("gtid_mode", std::optional<std::string>("ON"));
  EXPECT_THROW_LIKE(cfg_h.apply(), mysqlshdk::db::Error,
                    "The value of @@GLOBAL.GTID_MODE can only be changed one "
                    "step at a time: OFF <-> OFF_PERMISSIVE <-> ON_PERMISSIVE "
                    "<-> ON.");

  // Enable gtid_mode following the allowed steps order.
  Config_server_handler cfg_h2(&instance, Var_qualifier::GLOBAL);
  cfg_h2.set("gtid_mode", std::optional<std::string>("OFF_PERMISSIVE"));
  cfg_h2.set("gtid_mode", std::optional<std::string>("ON_PERMISSIVE"));
  cfg_h2.set("enforce_gtid_consistency", std::optional<std::string>("ON"));
  cfg_h2.set("gtid_mode", std::optional<std::string>("ON"));
  cfg_h2.apply();

  std::optional<std::string> gtid_mode =
      instance.get_sysvar_string("gtid_mode", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!gtid_mode);
  EXPECT_STREQ("ON", (*gtid_mode).c_str());

  // Error if the wrong order is used.
  cfg_h2.set("gtid_mode", std::optional<std::string>("OFF_PERMISSIVE"));
  cfg_h2.set("enforce_gtid_consistency", std::optional<std::string>("OFF"));
  cfg_h2.set("gtid_mode", std::optional<std::string>("ON_PERMISSIVE"));
  cfg_h2.set("gtid_mode", std::optional<std::string>("OFF"));
  EXPECT_THROW_LIKE(cfg_h2.apply(), mysqlshdk::db::Error,
                    "The value of @@GLOBAL.GTID_MODE can only be changed one "
                    "step at a time: OFF <-> OFF_PERMISSIVE <-> ON_PERMISSIVE "
                    "<-> ON.");

  // Disable gtid_mode (back to OFF) following the allowed steps order.
  Config_server_handler cfg_h3(&instance, Var_qualifier::GLOBAL);
  cfg_h3.set("gtid_mode", std::optional<std::string>("ON_PERMISSIVE"));
  cfg_h3.set("enforce_gtid_consistency", std::optional<std::string>("OFF"));
  cfg_h3.set("gtid_mode", std::optional<std::string>("OFF_PERMISSIVE"));
  cfg_h3.set("gtid_mode", std::optional<std::string>("OFF"));
  cfg_h3.apply();

  gtid_mode = instance.get_sysvar_string("gtid_mode", Var_qualifier::GLOBAL);
  EXPECT_FALSE(!gtid_mode);
  EXPECT_STREQ("OFF", (*gtid_mode).c_str());
}

TEST_F(Config_server_handler_test, apply_errors) {
  // Test errors applying changes.
  mysqlshdk::mysql::Instance instance(m_session);

  {
    SCOPED_TRACE("Error applying bool value setting (no context).");
    Config_server_handler cfg_h(&instance, Var_qualifier::GLOBAL);
    cfg_h.set("not_exit_bool", std::optional<bool>(true));
    EXPECT_THROW_LIKE(cfg_h.apply(true), std::runtime_error,
                      "Unknown system variable 'not_exit_bool'");
  }

  {
    SCOPED_TRACE("Error applying bool value setting (with context).");
    Config_server_handler cfg_h(&instance, Var_qualifier::GLOBAL);
    cfg_h.set("not_exit_bool", std::optional<bool>(true), "notExistBool");
    EXPECT_THROW_LIKE(cfg_h.apply(true), std::runtime_error,
                      "Unable to set value 'true' for 'notExistBool': Unknown "
                      "system variable 'not_exit_bool'");
  }

  {
    SCOPED_TRACE("Error applying int value setting (no context).");
    Config_server_handler cfg_h(&instance, Var_qualifier::GLOBAL);
    cfg_h.set("not_exit_int", std::optional<int64_t>(1234));
    EXPECT_THROW_LIKE(cfg_h.apply(true), std::runtime_error,
                      "Unknown system variable 'not_exit_int'");
  }

  {
    SCOPED_TRACE("Error applying int value setting (with context).");
    Config_server_handler cfg_h(&instance, Var_qualifier::GLOBAL);
    cfg_h.set("not_exit_int", std::optional<int64_t>(1234), "notExistInt");
    EXPECT_THROW_LIKE(cfg_h.apply(true), std::runtime_error,
                      "Unable to set value '1234' for 'notExistInt': Unknown "
                      "system variable 'not_exit_int'");
  }

  {
    SCOPED_TRACE("Error applying string value setting (no context).");
    Config_server_handler cfg_h(&instance, Var_qualifier::GLOBAL);
    cfg_h.set("not_exit_string", std::optional<std::string>("mystr"));
    EXPECT_THROW_LIKE(cfg_h.apply(true), std::runtime_error,
                      "Unknown system variable 'not_exit_string'");
  }

  {
    SCOPED_TRACE("Error applying string value setting (with context).");
    Config_server_handler cfg_h(&instance, Var_qualifier::GLOBAL);
    cfg_h.set("not_exit_string", std::optional<std::string>("mystr"),
              "notExistString");
    EXPECT_THROW_LIKE(cfg_h.apply(true), std::runtime_error,
                      "Unable to set value 'mystr' for 'notExistString': "
                      "Unknown system variable 'not_exit_string'");
  }
}

TEST_F(Config_server_handler_test, get_persisted_value) {
  // Test getting persisted values.
  mysqlshdk::mysql::Instance instance(m_session);

  Config_server_handler cfg_h(&instance, Var_qualifier::PERSIST);

  if (instance.is_set_persist_supported()) {
    // No persisted value for variable.
    std::optional<std::string> res = cfg_h.get_persisted_value("gtid_mode");
    EXPECT_TRUE(!res);

    // Get variable with persisted value.
    instance.set_sysvar("gtid_mode", std::string{"ON"},
                        Var_qualifier::PERSIST_ONLY);
    res = cfg_h.get_persisted_value("gtid_mode");
    EXPECT_FALSE(!res);
    EXPECT_STREQ("ON", (*res).c_str());

    // Remove persisted variable (clean-up).
    instance.execute("RESET PERSIST gtid_mode");
  } else {
    // Exception thrown if SET PERSIST is not supported.
    EXPECT_THROW_LIKE(cfg_h.get_persisted_value("not_exist"),
                      std::runtime_error,
                      "Unable to get persisted value for 'not_exist': ");
  }
}

TEST_F(Config_server_handler_test, undo) {
  mysqlshdk::mysql::Instance instance(m_session);
  Config_server_handler cfg_h(&instance, Var_qualifier::GLOBAL);

  // default values
  instance.set_sysvar("auto_increment_increment", int64_t{11},
                      Var_qualifier::GLOBAL);
  instance.set_sysvar("enforce_gtid_consistency", bool{true},
                      Var_qualifier::GLOBAL);
  instance.set_sysvar("character_set_client", std::string{"greek"},
                      Var_qualifier::GLOBAL);

  cfg_h.set("auto_increment_increment", std::optional<int64_t>(10));
  cfg_h.set("auto_increment_increment", std::optional<int64_t>(9));
  cfg_h.set("enforce_gtid_consistency", std::optional<bool>(false));
  cfg_h.set("character_set_client", std::optional<std::string>("latin2"));
  cfg_h.apply();

  cfg_h.undo_changes();

  {
    auto var = instance.get_sysvar_int("auto_increment_increment");
    EXPECT_TRUE(var.has_value());
    EXPECT_EQ(var.value(), 11);
  }

  {
    auto var = instance.get_sysvar_bool("enforce_gtid_consistency");
    EXPECT_TRUE(var.has_value());
    EXPECT_EQ(var.value(), true);
  }

  {
    auto var = instance.get_sysvar_string("character_set_client");
    EXPECT_TRUE(var.has_value());
    EXPECT_EQ(var.value(), "greek");
  }

  // reset values
  instance.set_sysvar_default("auto_increment_increment",
                              Var_qualifier::GLOBAL);
  instance.set_sysvar_default("enforce_gtid_consistency",
                              Var_qualifier::GLOBAL);
  instance.set_sysvar_default("character_set_client", Var_qualifier::GLOBAL);
}

TEST_F(Config_server_handler_test, undo_ignore) {
  mysqlshdk::mysql::Instance instance(m_session);
  Config_server_handler cfg_h(&instance, Var_qualifier::GLOBAL);

  // default values
  instance.set_sysvar("auto_increment_increment", int64_t{11},
                      Var_qualifier::GLOBAL);
  instance.set_sysvar("enforce_gtid_consistency", bool{true},
                      Var_qualifier::GLOBAL);
  instance.set_sysvar("character_set_client", std::string{"greek"},
                      Var_qualifier::GLOBAL);

  cfg_h.remove_from_undo("enforce_gtid_consistency");

  cfg_h.set("auto_increment_increment", std::optional<int64_t>(10));
  cfg_h.set("auto_increment_increment", std::optional<int64_t>(9));
  cfg_h.set("enforce_gtid_consistency", std::optional<bool>(false));
  cfg_h.set("character_set_client", std::optional<std::string>("latin2"));
  cfg_h.apply();

  cfg_h.remove_from_undo("character_set_client");
  cfg_h.undo_changes();

  {
    auto var = instance.get_sysvar_int("auto_increment_increment");
    EXPECT_TRUE(var.has_value());
    EXPECT_EQ(var.value(), 11);
  }

  {
    auto var = instance.get_sysvar_bool("enforce_gtid_consistency");
    EXPECT_TRUE(var.has_value());
    EXPECT_EQ(var.value(), false);
  }

  {
    auto var = instance.get_sysvar_string("character_set_client");
    EXPECT_TRUE(var.has_value());
    EXPECT_EQ(var.value(), "latin2");
  }

  // reset values
  instance.set_sysvar_default("auto_increment_increment",
                              Var_qualifier::GLOBAL);
  instance.set_sysvar_default("enforce_gtid_consistency",
                              Var_qualifier::GLOBAL);
  instance.set_sysvar_default("character_set_client", Var_qualifier::GLOBAL);
}

TEST_F(Config_server_handler_test, undo_ignore_all) {
  mysqlshdk::mysql::Instance instance(m_session);
  Config_server_handler cfg_h(&instance, Var_qualifier::GLOBAL);

  // default values
  instance.set_sysvar("auto_increment_increment", int64_t{11},
                      Var_qualifier::GLOBAL);
  instance.set_sysvar("enforce_gtid_consistency", bool{false},
                      Var_qualifier::GLOBAL);
  instance.set_sysvar("character_set_client", std::string{"greek"},
                      Var_qualifier::GLOBAL);

  cfg_h.set("auto_increment_increment", std::optional<int64_t>(10));
  cfg_h.set("auto_increment_increment", std::optional<int64_t>(9));
  cfg_h.set("enforce_gtid_consistency", std::optional<bool>(true));
  cfg_h.set("character_set_client", std::optional<std::string>("latin2"));
  cfg_h.apply();

  cfg_h.remove_all_from_undo();
  cfg_h.undo_changes();

  {
    auto var = instance.get_sysvar_int("auto_increment_increment");
    EXPECT_TRUE(var.has_value());
    EXPECT_EQ(var.value(), 9);
  }

  {
    auto var = instance.get_sysvar_bool("enforce_gtid_consistency");
    EXPECT_TRUE(var.has_value());
    EXPECT_EQ(var.value(), true);
  }

  {
    auto var = instance.get_sysvar_string("character_set_client");
    EXPECT_TRUE(var.has_value());
    EXPECT_EQ(var.value(), "latin2");
  }

  // reset values
  instance.set_sysvar_default("auto_increment_increment",
                              Var_qualifier::GLOBAL);
  instance.set_sysvar_default("enforce_gtid_consistency",
                              Var_qualifier::GLOBAL);
  instance.set_sysvar_default("character_set_client", Var_qualifier::GLOBAL);
}

}  // namespace testing
