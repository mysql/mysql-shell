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

#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/config/config_file_handler.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/mocks/gmock_clean.h"

using mysqlshdk::config::Config;
using mysqlshdk::config::Config_file_handler;
using mysqlshdk::config::Config_server_handler;
using mysqlshdk::config::IConfig_handler;
using mysqlshdk::mysql::Var_qualifier;
using mysqlshdk::utils::nullable;

namespace testing {

class Config_test : public tests::Shell_base_test {
 protected:
  void SetUp() {
    tests::Shell_base_test::SetUp();

    // Create instance and Open the session for the tests.
    m_connection_options = shcore::get_connection_options(_mysql_uri);
    m_session->connect(m_connection_options);
    m_tmpdir = getenv("TMPDIR");
    m_cfg_path = shcore::path::join_path(m_tmpdir, "my_test.cnf");
  }

  void TearDown() {
    tests::Shell_base_test::TearDown();

    // Close the session.
    m_session->close();
    // delete the config file
    shcore::delete_file(m_cfg_path, true);
  }

  std::shared_ptr<mysqlshdk::db::ISession> m_session =
      mysqlshdk::db::mysql::Session::create();
  mysqlshdk::db::Connection_options m_connection_options;
  std::string m_tmpdir, m_cfg_path;
};

TEST_F(Config_test, config_handlers) {
  Config cfg;
  mysqlshdk::mysql::Instance instance(m_session);

  // No handler added yet
  EXPECT_FALSE(cfg.has_handler("server_global"));
  EXPECT_FALSE(cfg.has_handler("server_session"));

  // Test adding handlers
  bool res = cfg.add_handler(
      "server_global",
      std::unique_ptr<IConfig_handler>(
          new Config_server_handler(&instance, Var_qualifier::GLOBAL)));
  EXPECT_TRUE(res);
  EXPECT_TRUE(cfg.has_handler("server_global"));
  res = cfg.add_handler(
      "server_session",
      std::unique_ptr<IConfig_handler>(
          new Config_server_handler(&instance, Var_qualifier::SESSION)));
  EXPECT_TRUE(res);
  EXPECT_TRUE(cfg.has_handler("server_session"));

  // Handler with the same name is not added (already exists).
  res = cfg.add_handler(
      "server_global",
      std::unique_ptr<IConfig_handler>(
          new Config_server_handler(&instance, Var_qualifier::PERSIST)));
  EXPECT_FALSE(res);
  EXPECT_TRUE(cfg.has_handler("server_global"));

  // Get a specific handler.
  Config_server_handler *handler =
      static_cast<Config_server_handler *>(cfg.get_handler("server_global"));
  EXPECT_EQ(Var_qualifier::GLOBAL, handler->get_default_var_qualifier());
  handler =
      static_cast<Config_server_handler *>(cfg.get_handler("server_session"));
  EXPECT_EQ(Var_qualifier::SESSION, handler->get_default_var_qualifier());

  // Error trying to get a handler that does not exist.
  EXPECT_THROW(cfg.get_handler("not-exist"), std::out_of_range);

  // Cannot remove the default handler (first added by default).
  EXPECT_THROW_LIKE(
      cfg.remove_handler("server_global"), std::runtime_error,
      "Cannot remove the default configuration handler 'server_global'.");

  // Change the default handler.
  cfg.set_default_handler("server_session");

  // Error trying to set as default an handler that does not exist.
  EXPECT_THROW_LIKE(
      cfg.set_default_handler("not_an_handler"), std::out_of_range,
      "The specified configuration handler 'not_an_handler' does not exist.");

  // Now, the previous handler can be removed.
  cfg.remove_handler("server_global");
  EXPECT_FALSE(cfg.has_handler("server_global"));
  EXPECT_TRUE(cfg.has_handler("server_session"));

  // Error trying to remove an handler that does not exist.
  EXPECT_THROW_LIKE(
      cfg.remove_handler("not-exist"), std::out_of_range,
      "The specified configuration handler 'not-exist' does not exist.");

  // Test clear handlers
  cfg.clear_handlers();
  EXPECT_FALSE(cfg.has_handler("server_global"));
  EXPECT_FALSE(cfg.has_handler("server_session"));
}

TEST_F(Config_test, config_interface) {
  Config cfg;

  // Add server handler for global variables (used as default handler).
  mysqlshdk::mysql::Instance instance(m_session);
  bool res = cfg.add_handler(
      "server_global",
      std::unique_ptr<IConfig_handler>(
          new Config_server_handler(&instance, Var_qualifier::GLOBAL)));
  EXPECT_TRUE(res);
  // create empty config file
  create_file(m_cfg_path, "");
  res = cfg.add_handler("config_file",
                        std::unique_ptr<IConfig_handler>(
                            new Config_file_handler(m_cfg_path, m_cfg_path)));
  EXPECT_TRUE(res);

  // Test getting a bool value (default: false).
  nullable<bool> bool_val = cfg.get_bool("sql_warnings");
  EXPECT_FALSE(*bool_val);

  // Test setting a bool value.
  cfg.set("sql_warnings", nullable<bool>(true));
  bool_val = cfg.get_bool("sql_warnings");
  EXPECT_TRUE(*bool_val);

  // No changes applied yet to sql_warnings on the server and option file.
  bool_val = instance.get_sysvar_bool("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(*bool_val);

  Config_file_handler cfg_handler_tmp =
      Config_file_handler(m_cfg_path, m_cfg_path);
  // since the config file started empty and the changes haven't yet been
  // applied an exception should be thrown
  EXPECT_THROW(cfg_handler_tmp.get_bool("sql_warnings"), std::out_of_range);

  // Test getting an int value (use current value as default).
  nullable<int64_t> wait_timeout =
      instance.get_sysvar_int("wait_timeout", Var_qualifier::GLOBAL);
  nullable<int64_t> int_val = cfg.get_int("wait_timeout");
  EXPECT_EQ(*wait_timeout, *int_val);

  // Test setting a int value.
  cfg.set("wait_timeout", nullable<int64_t>(30000));
  int_val = cfg.get_int("wait_timeout");
  EXPECT_EQ(30000, *int_val);

  // No changes applied yet to wait_timeout on the server and option file.
  int_val = instance.get_sysvar_int("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_EQ(*wait_timeout, *int_val);
  cfg_handler_tmp = Config_file_handler(m_cfg_path, m_cfg_path);
  // since the config file started empty and the changes haven't yet been
  // applied an exception should be thrown
  EXPECT_THROW(cfg_handler_tmp.get_int("wait_timeout"), std::out_of_range);

  // Test getting a string value (default: en_US).
  nullable<std::string> string_val = cfg.get_string("lc_messages");
  EXPECT_STREQ("en_US", (*string_val).c_str());

  // Test setting a string value.
  cfg.set("lc_messages", nullable<std::string>("pt_PT"));
  string_val = cfg.get_string("lc_messages");
  EXPECT_STREQ("pt_PT", (*string_val).c_str());

  // No changes applied yet to lc_messages on the server and option file.
  string_val = instance.get_sysvar_string("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_STREQ("en_US", (*string_val).c_str());
  cfg_handler_tmp = Config_file_handler(m_cfg_path, m_cfg_path);
  // since the config file started empty and the changes haven't yet been
  // applied an exception should be thrown
  EXPECT_THROW(cfg_handler_tmp.get_string("lc_messages"), std::out_of_range);

  // Apply all changes at once.
  cfg.apply();

  // Verify all previous changes are now applied on the server and option file.
  bool_val = instance.get_sysvar_bool("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_TRUE(*bool_val);
  int_val = instance.get_sysvar_int("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_EQ(30000, *int_val);
  string_val = instance.get_sysvar_string("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_STREQ("pt_PT", (*string_val).c_str());

  cfg_handler_tmp = Config_file_handler(m_cfg_path, m_cfg_path);
  bool_val = cfg_handler_tmp.get_bool("sql_warnings");
  EXPECT_TRUE(*bool_val);
  int_val = cfg_handler_tmp.get_int("wait_timeout");
  EXPECT_EQ(30000, *int_val);
  string_val = cfg_handler_tmp.get_string("lc_messages");
  EXPECT_STREQ("pt_PT", (*string_val).c_str());

  // Restore previous setting individually at each handler.
  cfg.set("sql_warnings", nullable<bool>(false), "server_global");
  cfg.set("sql_warnings", nullable<bool>(false), "config_file");
  bool_val = cfg.get_bool("sql_warnings", "server_global");
  EXPECT_FALSE(*bool_val);
  bool_val = cfg.get_bool("sql_warnings", "config_file");
  EXPECT_FALSE(*bool_val);
  cfg.set("wait_timeout", wait_timeout, "server_global");
  cfg.set("wait_timeout", wait_timeout, "config_file");
  int_val = cfg.get_int("wait_timeout", "server_global");
  EXPECT_FALSE(int_val.is_null());
  EXPECT_EQ(*wait_timeout, *int_val);
  int_val = cfg.get_int("wait_timeout", "config_file");
  EXPECT_EQ(*wait_timeout, *int_val);
  cfg.set("lc_messages", nullable<std::string>("en_US"), "server_global");
  cfg.set("lc_messages", nullable<std::string>("en_US"), "config_file");
  string_val = cfg.get_string("lc_messages", "server_global");
  EXPECT_STREQ("en_US", (*string_val).c_str());
  string_val = cfg.get_string("lc_messages", "config_file");
  EXPECT_STREQ("en_US", (*string_val).c_str());
  cfg.apply();

  // Confirm values where restored properly, on both server and option file.
  cfg_handler_tmp = Config_file_handler(m_cfg_path, m_cfg_path);
  bool_val = instance.get_sysvar_bool("sql_warnings", Var_qualifier::GLOBAL);
  EXPECT_FALSE(*bool_val);
  bool_val = cfg_handler_tmp.get_bool("sql_warnings");
  EXPECT_FALSE(bool_val.is_null());
  EXPECT_FALSE(*bool_val);

  int_val = instance.get_sysvar_int("wait_timeout", Var_qualifier::GLOBAL);
  EXPECT_EQ(*wait_timeout, *int_val);
  int_val = cfg_handler_tmp.get_int("wait_timeout");
  EXPECT_EQ(*wait_timeout, *int_val);

  string_val = instance.get_sysvar_string("lc_messages", Var_qualifier::GLOBAL);
  EXPECT_STREQ("en_US", (*string_val).c_str());
  string_val = cfg_handler_tmp.get_string("lc_messages");
  EXPECT_STREQ("en_US", (*string_val).c_str());
}

}  // namespace testing
