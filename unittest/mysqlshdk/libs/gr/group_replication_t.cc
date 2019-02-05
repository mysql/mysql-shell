/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/config/config_file_handler.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/mysql/mock_instance.h"
#include "unittest/test_utils/shell_base_test.h"

namespace testing {

class Group_replication_test : public tests::Shell_base_test {
 protected:
  void SetUp() {
    tests::Shell_base_test::SetUp();

    // Create instance and Open the session for the tests.
    _connection_options = shcore::get_connection_options(_mysql_uri);
    _session->connect(_connection_options);
    instance = new mysqlshdk::mysql::Instance(_session);

    // Get temp dir path
    m_tmpdir = getenv("TMPDIR");
    m_cfg_path = shcore::path::join_path(m_tmpdir, "my_gr_test.cnf");
  }

  void TearDown() {
    tests::Shell_base_test::TearDown();

    // Close the session.
    _session->close();
    delete instance;
  }

  std::shared_ptr<mysqlshdk::db::ISession> _session =
      mysqlshdk::db::mysql::Session::create();
  mysqlshdk::mysql::Instance *instance;
  mysqlshdk::db::Connection_options _connection_options;

  std::string m_tmpdir, m_cfg_path;
};

TEST_F(Group_replication_test, plugin_installation) {
  using mysqlshdk::utils::nullable;

  // Check if GR plugin is installed and uninstall it.
  nullable<std::string> init_plugin_state =
      instance->get_plugin_status(mysqlshdk::gr::kPluginName);
  if (!init_plugin_state.is_null()) {
    // Test uninstall the plugin when available.
    bool res = mysqlshdk::gr::uninstall_plugin(*instance, nullptr);
    EXPECT_TRUE(res);
    nullable<std::string> plugin_state =
        instance->get_plugin_status(mysqlshdk::gr::kPluginName);
    ASSERT_TRUE(plugin_state.is_null());
    // Test trying to uninstall the plugin when not available.
    res = mysqlshdk::gr::uninstall_plugin(*instance, nullptr);
    EXPECT_FALSE(res);
  }

  // Test installing the plugin (when not installed).
  if (!init_plugin_state.is_null() &&
      (*init_plugin_state).compare(mysqlshdk::gr::kPluginDisabled) == 0) {
    // An exception is expected if the plugin was disabled.
    EXPECT_THROW(mysqlshdk::gr::install_plugin(*instance, nullptr),
                 std::runtime_error);
  } else {
    // Requirements to install the GR plugin:
    // - server_id != 0
    // - master_info_repository=TABLE
    // - relay_log_info_repository=TABLE
    nullable<int64_t> server_id = instance->get_sysvar_int("server_id");
    if (*server_id == 0) {
      SKIP_TEST("Test server does not meet GR requirements: server_id is 0.");
    }
    nullable<std::string> master_info_repository =
        instance->get_sysvar_string("master_info_repository");
    if ((*master_info_repository).compare("TABLE") != 0) {
      SKIP_TEST(
          "Test server does not meet GR requirements: master_info_repository "
          "must be 'TABLE'.");
    }
    nullable<std::string> relay_log_info_repository =
        instance->get_sysvar_string("relay_log_info_repository");
    if ((*relay_log_info_repository).compare("TABLE") != 0) {
      SKIP_TEST(
          "Test server does not meet GR requirements: "
          "relay_log_info_repository "
          "must be 'TABLE'.");
    }

    // GR plugin is installed and activated (if not previously disabled).
    bool res = mysqlshdk::gr::install_plugin(*instance, nullptr);
    ASSERT_TRUE(res)
        << "GR plugin was not installed (expected not to be available).";
    nullable<std::string> plugin_state =
        instance->get_plugin_status(mysqlshdk::gr::kPluginName);
    EXPECT_STREQ(mysqlshdk::gr::kPluginActive, (*plugin_state).c_str());

    // Test installing the plugin when already installed.
    res = mysqlshdk::gr::install_plugin(*instance, nullptr);
    EXPECT_FALSE(res)
        << "GR plugin was installed (expected to be already available).";
    plugin_state = instance->get_plugin_status(mysqlshdk::gr::kPluginName);
    EXPECT_STREQ(mysqlshdk::gr::kPluginActive, (*plugin_state).c_str());
  }

  // Restore initial state (uninstall plugin if needed).
  if (init_plugin_state.is_null()) {
    // Test uninstall the plugin when available.
    bool res = mysqlshdk::gr::uninstall_plugin(*instance, nullptr);
    EXPECT_TRUE(res);
    nullable<std::string> plugin_state =
        instance->get_plugin_status(mysqlshdk::gr::kPluginName);
    ASSERT_TRUE(plugin_state.is_null());

    // Test trying to uninstall the plugin when not available.
    res = mysqlshdk::gr::uninstall_plugin(*instance, nullptr);
    EXPECT_FALSE(res);
  }
}

TEST_F(Group_replication_test, generate_group_name) {
  std::string name1 = mysqlshdk::gr::generate_group_name();
  std::string name2 = mysqlshdk::gr::generate_group_name();
  // Generated group names must be different.
  EXPECT_STRNE(name1.c_str(), name2.c_str());
}

TEST_F(Group_replication_test, replication_user) {
  // Confirm that there is no replication user.
  auto res =
      mysqlshdk::gr::check_replication_user(*instance, "test_gr_user", "%");
  EXPECT_FALSE(res.user_exists());
  EXPECT_EQ(std::set<std::string>{"REPLICATION SLAVE"},
            res.get_missing_privileges());
  EXPECT_TRUE(res.has_missing_privileges());
  EXPECT_FALSE(res.has_grant_option());

  // Create a replication user.
  std::string passwd;
  mysqlshdk::gr::create_replication_user_random_pass(*instance, "test_gr_user",
                                                     {"%"}, &passwd);
  // Check replication user (now it exist and it has no missing privileges).
  res = mysqlshdk::gr::check_replication_user(*instance, "test_gr_user", "%");

  EXPECT_TRUE(res.user_exists());
  EXPECT_EQ(std::set<std::string>{}, res.get_missing_privileges());
  EXPECT_FALSE(res.has_missing_privileges());
  EXPECT_FALSE(res.has_grant_option());

  // Clean up (remove the create user at the end)
  instance->drop_user("test_gr_user", "%");
}

TEST_F(Group_replication_test, start_stop_gr) {
  // Beside the start_group_replication() and stop_group_replication()
  // functions, the functions is_member() and get_member_state() are also
  // tested here since the test scenario is the same, in order to avoid
  // additional execution time to run similar test cases.
  // NOTE: START and STOP GROUP_REPLICATION is slow.

  using mysqlshdk::gr::Member_state;
  using mysqlshdk::mysql::Var_qualifier;
  using mysqlshdk::utils::nullable;

  // Check if used server meets the requirements.
  nullable<int64_t> server_id = instance->get_sysvar_int("server_id");
  if (*server_id == 0) {
    SKIP_TEST("Test server does not meet GR requirements: server_id is 0.");
  }
  nullable<bool> log_bin = instance->get_sysvar_bool("log_bin");
  if (*log_bin != true) {
    SKIP_TEST("Test server does not meet GR requirements: log_bin must be ON.");
  }
  nullable<bool> gtid_mode = instance->get_sysvar_bool("gtid_mode");
  if (*gtid_mode != true) {
    SKIP_TEST(
        "Test server does not meet GR requirements: gtid_mode must be ON.");
  }
  nullable<bool> enforce_gtid_consistency =
      instance->get_sysvar_bool("enforce_gtid_consistency");
  if (*enforce_gtid_consistency != true) {
    SKIP_TEST(
        "Test server does not meet GR requirements: enforce_gtid_consistency "
        "must be ON.");
  }
  nullable<std::string> master_info_repository =
      instance->get_sysvar_string("master_info_repository");
  if ((*master_info_repository).compare("TABLE") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: master_info_repository "
        "must be 'TABLE'.");
  }
  nullable<std::string> relay_log_info_repository =
      instance->get_sysvar_string("relay_log_info_repository");
  if ((*relay_log_info_repository).compare("TABLE") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: relay_log_info_repository "
        "must be 'TABLE'.");
  }
  nullable<std::string> binlog_checksum =
      instance->get_sysvar_string("binlog_checksum");
  if ((*binlog_checksum).compare("NONE") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: binlog_checksum must be "
        "'NONE'.");
  }
  nullable<bool> log_slave_updates =
      instance->get_sysvar_bool("log_slave_updates");
  if (*log_slave_updates != true) {
    SKIP_TEST(
        "Test server does not meet GR requirements: log_slave_updates must "
        "be ON.");
  }
  nullable<std::string> binlog_format =
      instance->get_sysvar_string("binlog_format");
  if ((*binlog_format).compare("ROW") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: binlog_format must be "
        "'ROW'.");
  }

  // Test: member is not part of any group, state must be MISSING.
  bool res = mysqlshdk::gr::is_member(*instance);
  EXPECT_FALSE(res);
  res = mysqlshdk::gr::is_member(*instance, "not_the_group_name");
  EXPECT_FALSE(res);
  Member_state state_res = mysqlshdk::gr::get_member_state(*instance);
  EXPECT_EQ(state_res, Member_state::MISSING);

  // Install GR plugin if needed.
  nullable<std::string> init_plugin_state =
      instance->get_plugin_status(mysqlshdk::gr::kPluginName);
  if (init_plugin_state.is_null()) {
    mysqlshdk::gr::install_plugin(*instance, nullptr);
  }

  // Get initial value of GR variables (to restore at the end).
  nullable<std::string> gr_group_name =
      instance->get_sysvar_string("group_replication_group_name");
  nullable<std::string> gr_local_address =
      instance->get_sysvar_string("group_replication_local_address");

  // Set GR variable to start GR.
  std::string group_name = mysqlshdk::gr::generate_group_name();
  instance->set_sysvar("group_replication_group_name", group_name,
                       Var_qualifier::GLOBAL);
  std::string local_address = "localhost:13013";
  instance->set_sysvar("group_replication_local_address", local_address,
                       Var_qualifier::GLOBAL);

  // Test: Start Group Replication.
  mysqlshdk::gr::start_group_replication(*instance, true);

  // SUPER READ ONLY must be OFF (verify wait for it to be disable).
  nullable<bool> read_only =
      instance->get_sysvar_bool("super_read_only", Var_qualifier::GLOBAL);
  EXPECT_FALSE(*read_only);

  // Test: member is part of GR group, state must be RECOVERING or ONLINE.
  res = mysqlshdk::gr::is_member(*instance);
  EXPECT_TRUE(res);
  res = mysqlshdk::gr::is_member(*instance, group_name);
  EXPECT_TRUE(res);
  state_res = mysqlshdk::gr::get_member_state(*instance);
  if (state_res == Member_state::ONLINE ||
      state_res == Member_state::RECOVERING)
    SUCCEED();
  else
    ADD_FAILURE() << "Unexpected status after starting GR, member state must "
                     "be ONLINE or RECOVERING";

  // Check GR server status (must be RECOVERING or ONLINE).
  auto session = instance->get_session();
  std::string gr_status_stmt =
      "SELECT MEMBER_STATE "
      "FROM performance_schema.replication_group_members "
      "WHERE MEMBER_ID = @@server_uuid";
  auto resultset = session->query(gr_status_stmt);
  auto row = resultset->fetch_one();
  std::string status = row ? row->get_string(0) : "(empty)";
  if (status.compare("ONLINE") != 0 && status.compare("RECOVERING"))
    ADD_FAILURE() << "Unexpected status after starting GR: " << status;

  // Test: Start Group Replication fails for group already running.
  EXPECT_THROW(mysqlshdk::gr::start_group_replication(*instance, true),
               std::exception);

  // Test: Stop Group Replication.
  mysqlshdk::gr::stop_group_replication(*instance);

  // Starting from MySQL 5.7.20 GR automatically enables super_read_only after
  // stop. Thus, always disable read_only ro consider this situation.
  instance->set_sysvar("super_read_only", false, Var_qualifier::GLOBAL);
  instance->set_sysvar("read_only", false, Var_qualifier::GLOBAL);

  // Test: member is still part of the group, but its state is OFFLINE.
  res = mysqlshdk::gr::is_member(*instance);
  EXPECT_TRUE(res);
  res = mysqlshdk::gr::is_member(*instance, group_name);
  EXPECT_TRUE(res);
  state_res = mysqlshdk::gr::get_member_state(*instance);
  EXPECT_EQ(state_res, Member_state::OFFLINE);

  // Clean up (restore initial server state).
  if (!(*gr_group_name).empty())
    // NOTE: The group_name cannot be set with an empty value.
    instance->set_sysvar("group_replication_group_name", *gr_group_name,
                         Var_qualifier::GLOBAL);
  instance->set_sysvar("group_replication_local_address", *gr_local_address,
                       Var_qualifier::GLOBAL);
  if (init_plugin_state.is_null()) {
    mysqlshdk::gr::uninstall_plugin(*instance, nullptr);
  }
}

TEST_F(Group_replication_test, members_state) {
  using mysqlshdk::gr::Member_state;

  // Test to_string() function
  SCOPED_TRACE("to_string() function test");
  std::string str_res = mysqlshdk::gr::to_string(Member_state::ONLINE);
  EXPECT_STREQ("ONLINE", str_res.c_str());
  str_res = mysqlshdk::gr::to_string(Member_state::RECOVERING);
  EXPECT_STREQ("RECOVERING", str_res.c_str());
  str_res = mysqlshdk::gr::to_string(Member_state::OFFLINE);
  EXPECT_STREQ("OFFLINE", str_res.c_str());
  str_res = mysqlshdk::gr::to_string(Member_state::ERROR);
  EXPECT_STREQ("ERROR", str_res.c_str());
  str_res = mysqlshdk::gr::to_string(Member_state::UNREACHABLE);
  EXPECT_STREQ("UNREACHABLE", str_res.c_str());
  str_res = mysqlshdk::gr::to_string(Member_state::MISSING);
  EXPECT_STREQ("(MISSING)", str_res.c_str());

  // Test to_member_state() function
  SCOPED_TRACE("to_member_state() function test");
  Member_state state_res = mysqlshdk::gr::to_member_state("ONLINE");
  EXPECT_EQ(state_res, Member_state::ONLINE);
  state_res = mysqlshdk::gr::to_member_state("RECOVERING");
  EXPECT_EQ(state_res, Member_state::RECOVERING);
  state_res = mysqlshdk::gr::to_member_state("Offline");
  EXPECT_EQ(state_res, Member_state::OFFLINE);
  state_res = mysqlshdk::gr::to_member_state("error");
  EXPECT_EQ(state_res, Member_state::ERROR);
  state_res = mysqlshdk::gr::to_member_state("uNREACHABLE");
  EXPECT_EQ(state_res, Member_state::UNREACHABLE);
  state_res = mysqlshdk::gr::to_member_state("MISSING");
  EXPECT_EQ(state_res, Member_state::MISSING);
  state_res = mysqlshdk::gr::to_member_state("(MISSING)");
  EXPECT_EQ(state_res, Member_state::MISSING);
  EXPECT_THROW(mysqlshdk::gr::to_member_state("invalid"), std::runtime_error);
}

TEST_F(Group_replication_test, get_replication_user) {
  using mysqlshdk::utils::nullable;

  // Check if used server meets the requirements.
  nullable<int64_t> server_id = instance->get_sysvar_int("server_id");
  if (*server_id == 0) {
    SKIP_TEST("Test server does not meet GR requirements: server_id is 0.");
  }
  nullable<bool> log_bin = instance->get_sysvar_bool("log_bin");
  if (*log_bin != true) {
    SKIP_TEST("Test server does not meet GR requirements: log_bin must be ON.");
  }
  nullable<bool> gtid_mode = instance->get_sysvar_bool("gtid_mode");
  if (*gtid_mode != true) {
    SKIP_TEST(
        "Test server does not meet GR requirements: gtid_mode must be ON.");
  }
  nullable<bool> enforce_gtid_consistency =
      instance->get_sysvar_bool("enforce_gtid_consistency");
  if (*enforce_gtid_consistency != true) {
    SKIP_TEST(
        "Test server does not meet GR requirements: enforce_gtid_consistency "
        "must be ON.");
  }
  nullable<std::string> master_info_repository =
      instance->get_sysvar_string("master_info_repository");
  if ((*master_info_repository).compare("TABLE") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: master_info_repository "
        "must be 'TABLE'.");
  }
  nullable<std::string> relay_log_info_repository =
      instance->get_sysvar_string("relay_log_info_repository");
  if ((*relay_log_info_repository).compare("TABLE") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: relay_log_info_repository "
        "must be 'TABLE'.");
  }
  nullable<std::string> binlog_checksum =
      instance->get_sysvar_string("binlog_checksum");
  if ((*binlog_checksum).compare("NONE") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: binlog_checksum must be "
        "'NONE'.");
  }
  nullable<bool> log_slave_updates =
      instance->get_sysvar_bool("log_slave_updates");
  if (*log_slave_updates != true) {
    SKIP_TEST(
        "Test server does not meet GR requirements: log_slave_updates "
        "must be ON.");
  }
  nullable<std::string> binlog_format =
      instance->get_sysvar_string("binlog_format");
  if ((*binlog_format).compare("ROW") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: binlog_format must be "
        "'ROW'.");
  }

  // Install GR plugin if needed.
  nullable<std::string> init_plugin_state =
      instance->get_plugin_status(mysqlshdk::gr::kPluginName);
  if (init_plugin_state.is_null()) {
    mysqlshdk::gr::install_plugin(*instance, nullptr);
  }

  // Test: empty string returned if no replication user was defined (or empty).
  std::string res = mysqlshdk::gr::get_recovery_user(*instance);
  EXPECT_TRUE(res.empty());

  // Set replication user
  auto session = instance->get_session();
  std::string change_master_stmt =
      "CHANGE MASTER TO MASTER_USER = 'test_user' "
      "FOR CHANNEL 'group_replication_recovery'";
  session->execute(change_master_stmt);

  // Test: correct replication user is returned.
  res = mysqlshdk::gr::get_recovery_user(*instance);
  EXPECT_STREQ("test_user", res.c_str());

  // Test: Start Group Replication fails for group already running.
  EXPECT_THROW(mysqlshdk::gr::start_group_replication(*instance, true),
               std::exception);

  // Clean up (restore initial server state).
  if (session)
    // Set user to empty value.
    session->execute(
        "CHANGE MASTER TO MASTER_USER = '' "
        "FOR CHANNEL 'group_replication_recovery'");
  if (init_plugin_state.is_null()) {
    mysqlshdk::gr::uninstall_plugin(*instance, nullptr);
  }
}

TEST_F(Group_replication_test, is_group_replication_delayed_starting) {
  using mysqlshdk::db::Type;

  std::shared_ptr<Mock_session> mock_session = std::make_shared<Mock_session>();
  mysqlshdk::mysql::Instance instance{mock_session};

  mock_session
      ->expect_query(
          "SELECT COUNT(*) FROM performance_schema.threads WHERE NAME = "
          "'thread/group_rpl/THD_delayed_initialization'")
      .then_return({{"", {"COUNT(*)"}, {Type::UInteger}, {{"1"}}}});
  EXPECT_TRUE(mysqlshdk::gr::is_group_replication_delayed_starting(instance));

  mock_session
      ->expect_query(
          "SELECT COUNT(*) FROM performance_schema.threads WHERE NAME = "
          "'thread/group_rpl/THD_delayed_initialization'")
      .then_return({{"", {"COUNT(*)"}, {Type::UInteger}, {{"0"}}}});
  EXPECT_FALSE(mysqlshdk::gr::is_group_replication_delayed_starting(instance));
}

TEST_F(Group_replication_test, get_all_configurations) {
  std::map<std::string, mysqlshdk::utils::nullable<std::string>> res =
      mysqlshdk::gr::get_all_configurations(*instance);

  // NOTE: Only auto_increment variables are returned if GR is not configured.
  //       Check only those values to avoid non-deterministic issues, depending
  //       on the server version and/or configuration (e.g., new
  //       group_replication_consistency variable added for 8.0.14 servers).
  std::vector<std::string> vars;
  std::vector<std::string> values;
  for (const auto &element : res) {
    vars.push_back(element.first);
    values.push_back(*element.second);
  }
  EXPECT_THAT(vars, Contains("auto_increment_increment"));
  EXPECT_THAT(vars, Contains("auto_increment_offset"));
}

TEST_F(Group_replication_test, check_log_bin_compatibility_disabled_57) {
  using mysqlshdk::gr::Config_type;
  using mysqlshdk::gr::Config_types;
  using mysqlshdk::gr::Invalid_config;
  using mysqlshdk::mysql::Var_qualifier;
  using mysqlshdk::utils::nullable;

  mysqlshdk::mysql::Mock_instance inst;

  std::vector<Invalid_config> res;

  // Test everything assuming 5.7, with binlog default OFF and no SET PERSIST
  EXPECT_CALL(inst, get_sysvar_bool("log_bin", Var_qualifier::GLOBAL))
      .WillRepeatedly(Return(nullable<bool>(false)));
  EXPECT_CALL(inst, get_sysvar_string("log_bin", Var_qualifier::GLOBAL))
      .WillRepeatedly(Return(nullable<std::string>("OFF")));
  EXPECT_CALL(inst, get_version())
      .WillRepeatedly(Return(mysqlshdk::utils::Version(5, 7, 24)));

  // Create config object (only with a server handler).
  mysqlshdk::config::Config cfg;
  cfg.add_handler(mysqlshdk::config::k_dft_cfg_server_handler,
                  shcore::make_unique<mysqlshdk::config::Config_server_handler>(
                      &inst, Var_qualifier::GLOBAL));

  mysqlshdk::gr::check_log_bin_compatibility(inst, cfg, &res);
  // should have 2 issues in 5.7 and 1 in 8.0, since binary log is disabled.
  EXPECT_STREQ(res.at(0).var_name.c_str(), "log_bin");
  EXPECT_STREQ(res.at(0).current_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_STREQ(res.at(0).required_val.c_str(), mysqlshdk::gr::k_no_value);
  EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(0).restart, false);
  EXPECT_STREQ(res.at(1).var_name.c_str(), "log_bin");
  EXPECT_STREQ(res.at(1).current_val.c_str(), "OFF");
  EXPECT_STREQ(res.at(1).required_val.c_str(), "ON");
  EXPECT_EQ(res.at(1).types, Config_type::SERVER);
  EXPECT_EQ(res.at(1).restart, true);

  // Create an empty test option file and add the option file config handler.
  create_file(m_cfg_path, "");
  cfg.add_handler(
      mysqlshdk::config::k_dft_cfg_file_handler,
      std::unique_ptr<mysqlshdk::config::IConfig_handler>(
          new mysqlshdk::config::Config_file_handler(m_cfg_path, m_cfg_path)));

  // Issues found on both server and option file (with no values set).
  res.clear();
  mysqlshdk::gr::check_log_bin_compatibility(inst, cfg, &res);

  ASSERT_EQ(2, res.size());
  EXPECT_STREQ(res.at(0).var_name.c_str(), "log_bin");
  EXPECT_STREQ(res.at(0).current_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_STREQ(res.at(0).required_val.c_str(), mysqlshdk::gr::k_no_value);
  EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(0).restart, false);
  EXPECT_STREQ(res.at(1).var_name.c_str(), "log_bin");
  EXPECT_STREQ(res.at(1).current_val.c_str(), "OFF");
  EXPECT_STREQ(res.at(1).required_val.c_str(), "ON");
  EXPECT_EQ(res.at(1).types, Config_type::SERVER);
  EXPECT_EQ(res.at(1).restart, true);

  // Set incompatible values on file, issues found.
  cfg.set("skip_log_bin", nullable<std::string>(),
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.set("disable_log_bin", nullable<std::string>(),
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.apply();

  res.clear();
  mysqlshdk::gr::check_log_bin_compatibility(inst, cfg, &res);

  ASSERT_EQ(4, res.size());
  EXPECT_STREQ(res.at(1).var_name.c_str(), "disable_log_bin");
  EXPECT_STREQ(res.at(1).current_val.c_str(), mysqlshdk::gr::k_no_value);
  EXPECT_STREQ(res.at(1).required_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_EQ(res.at(1).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(1).restart, false);
  EXPECT_STREQ(res.at(2).var_name.c_str(), "skip_log_bin");
  EXPECT_STREQ(res.at(2).current_val.c_str(), mysqlshdk::gr::k_no_value);
  EXPECT_STREQ(res.at(2).required_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_EQ(res.at(2).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(2).restart, false);

  // Delete the config file.
  shcore::delete_file(m_cfg_path, true);
}

TEST_F(Group_replication_test, check_log_bin_compatibility_disabled_80) {
  using mysqlshdk::gr::Config_type;
  using mysqlshdk::gr::Config_types;
  using mysqlshdk::gr::Invalid_config;
  using mysqlshdk::mysql::Var_qualifier;
  using mysqlshdk::utils::nullable;

  mysqlshdk::mysql::Mock_instance inst;

  std::vector<Invalid_config> res;

  // Test everything assuming 8.0, with binlog OFF but SET PERSISTable
  EXPECT_CALL(inst, get_sysvar_bool("log_bin", Var_qualifier::GLOBAL))
      .WillRepeatedly(Return(nullable<bool>(false)));
  EXPECT_CALL(inst, get_sysvar_string("log_bin", Var_qualifier::GLOBAL))
      .WillRepeatedly(Return(nullable<std::string>("OFF")));
  EXPECT_CALL(inst, get_version())
      .WillRepeatedly(Return(mysqlshdk::utils::Version(8, 0, 3)));

  // Create config object (only with a server handler).
  mysqlshdk::config::Config cfg;
  cfg.add_handler(mysqlshdk::config::k_dft_cfg_server_handler,
                  shcore::make_unique<mysqlshdk::config::Config_server_handler>(
                      &inst, Var_qualifier::GLOBAL));

  mysqlshdk::gr::check_log_bin_compatibility(inst, cfg, &res);
  EXPECT_STREQ(res.at(0).var_name.c_str(), "log_bin");
  EXPECT_STREQ(res.at(0).current_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_STREQ(res.at(0).required_val.c_str(), mysqlshdk::gr::k_no_value);
  EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(0).restart, false);
  EXPECT_STREQ(res.at(1).var_name.c_str(), "log_bin");
  EXPECT_STREQ(res.at(1).current_val.c_str(), "OFF");
  EXPECT_STREQ(res.at(1).required_val.c_str(), "ON");
  EXPECT_EQ(res.at(1).types, Config_type::SERVER);
  EXPECT_EQ(res.at(1).restart, true);

  // Create an empty test option file and add the option file config handler.
  create_file(m_cfg_path, "");
  cfg.add_handler(
      mysqlshdk::config::k_dft_cfg_file_handler,
      std::unique_ptr<mysqlshdk::config::IConfig_handler>(
          new mysqlshdk::config::Config_file_handler(m_cfg_path, m_cfg_path)));

  // Issues found on both server and option file (with no values set).
  res.clear();
  mysqlshdk::gr::check_log_bin_compatibility(inst, cfg, &res);

  // if server version is >=8.0.3 then the log_bin is enabled by default so
  // there is no need to be the log_bin option on the file
  ASSERT_EQ(1, res.size());
  EXPECT_STREQ(res.at(0).var_name.c_str(), "log_bin");
  EXPECT_STREQ(res.at(0).current_val.c_str(), "OFF");
  EXPECT_STREQ(res.at(0).required_val.c_str(), "ON");
  EXPECT_EQ(res.at(0).types, Config_type::SERVER);
  EXPECT_EQ(res.at(0).restart, true);

  // Set incompatible values on file, issues found.
  cfg.set("skip_log_bin", nullable<std::string>(),
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.set("disable_log_bin", nullable<std::string>(),
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.apply();

  res.clear();
  mysqlshdk::gr::check_log_bin_compatibility(inst, cfg, &res);
  ASSERT_EQ(3, res.size());
  EXPECT_STREQ(res.at(0).var_name.c_str(), "disable_log_bin");
  EXPECT_STREQ(res.at(0).current_val.c_str(), mysqlshdk::gr::k_no_value);
  EXPECT_STREQ(res.at(0).required_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(0).restart, false);
  EXPECT_STREQ(res.at(1).var_name.c_str(), "skip_log_bin");
  EXPECT_STREQ(res.at(1).current_val.c_str(), mysqlshdk::gr::k_no_value);
  EXPECT_STREQ(res.at(1).required_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_EQ(res.at(1).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(1).restart, false);

  // Delete the config file.
  shcore::delete_file(m_cfg_path, true);
}

TEST_F(Group_replication_test, check_log_bin_compatibility_enabled) {
  using mysqlshdk::gr::Config_type;
  using mysqlshdk::gr::Config_types;
  using mysqlshdk::gr::Invalid_config;
  using mysqlshdk::mysql::Var_qualifier;
  using mysqlshdk::utils::nullable;
  nullable<bool> log_bin = instance->get_sysvar_bool("log_bin");
  if (!*log_bin) {
    SKIP_TEST(
        "Test server does not meet GR requirements: binary_log is disabled and "
        "should be enabled.")
  }
  // Create config object (only with a server handler).
  mysqlshdk::config::Config cfg;
  cfg.add_handler(
      mysqlshdk::config::k_dft_cfg_server_handler,
      std::unique_ptr<mysqlshdk::config::IConfig_handler>(
          shcore::make_unique<mysqlshdk::config::Config_server_handler>(
              instance, Var_qualifier::GLOBAL)));

  // should have no issues, since binary log is enabled.
  std::vector<Invalid_config> res;
  mysqlshdk::gr::check_log_bin_compatibility(*instance, cfg, &res);
  EXPECT_EQ(0, res.size());

  // Create an empty test option file and add the option file config handler.
  create_file(m_cfg_path, "");
  cfg.add_handler(
      mysqlshdk::config::k_dft_cfg_file_handler,
      std::unique_ptr<mysqlshdk::config::IConfig_handler>(
          new mysqlshdk::config::Config_file_handler(m_cfg_path, m_cfg_path)));

  // Issues found on the option file (with no values set).
  res.clear();
  mysqlshdk::gr::check_log_bin_compatibility(*instance, cfg, &res);
  // if server version is >=8.0.3 then the log_bin is enabled by default so
  // there is no need to be the log_bin option on the file
  if (instance->get_version() >= mysqlshdk::utils::Version(8, 0, 3)) {
    ASSERT_EQ(0, res.size());
  } else {
    ASSERT_EQ(1, res.size());
    EXPECT_STREQ(res.at(0).var_name.c_str(), "log_bin");
    EXPECT_STREQ(res.at(0).current_val.c_str(), mysqlshdk::gr::k_value_not_set);
    EXPECT_STREQ(res.at(0).required_val.c_str(), mysqlshdk::gr::k_no_value);
    EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(0).restart, false);
  }

  // Set incompatible values on file, issues found.
  cfg.set("skip_log_bin", nullable<std::string>(),
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.set("disable_log_bin", nullable<std::string>(),
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.apply();

  res.clear();
  mysqlshdk::gr::check_log_bin_compatibility(*instance, cfg, &res);
  if (instance->get_version() >= mysqlshdk::utils::Version(8, 0, 3)) {
    ASSERT_EQ(2, res.size());
    EXPECT_STREQ(res.at(0).var_name.c_str(), "disable_log_bin");
    EXPECT_STREQ(res.at(0).current_val.c_str(), mysqlshdk::gr::k_no_value);
    EXPECT_STREQ(res.at(0).required_val.c_str(),
                 mysqlshdk::gr::k_value_not_set);
    EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(0).restart, false);
    EXPECT_STREQ(res.at(1).var_name.c_str(), "skip_log_bin");
    EXPECT_STREQ(res.at(1).current_val.c_str(), mysqlshdk::gr::k_no_value);
    EXPECT_STREQ(res.at(1).required_val.c_str(),
                 mysqlshdk::gr::k_value_not_set);
    EXPECT_EQ(res.at(1).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(1).restart, false);
  } else {
    ASSERT_EQ(3, res.size());
    EXPECT_STREQ(res.at(0).var_name.c_str(), "log_bin");
    EXPECT_STREQ(res.at(0).current_val.c_str(), mysqlshdk::gr::k_value_not_set);
    EXPECT_STREQ(res.at(0).required_val.c_str(), mysqlshdk::gr::k_no_value);
    EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(0).restart, false);
    EXPECT_STREQ(res.at(1).var_name.c_str(), "disable_log_bin");
    EXPECT_STREQ(res.at(1).current_val.c_str(), mysqlshdk::gr::k_no_value);
    EXPECT_STREQ(res.at(1).required_val.c_str(),
                 mysqlshdk::gr::k_value_not_set);
    EXPECT_EQ(res.at(1).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(1).restart, false);
    EXPECT_STREQ(res.at(2).var_name.c_str(), "skip_log_bin");
    EXPECT_STREQ(res.at(2).current_val.c_str(), mysqlshdk::gr::k_no_value);
    EXPECT_STREQ(res.at(2).required_val.c_str(),
                 mysqlshdk::gr::k_value_not_set);
    EXPECT_EQ(res.at(2).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(2).restart, false);
  }

  // Set compatible values on file, no issues found.
  // Get the config file handler reference (to remove option from config file).
  mysqlshdk::config::Config_file_handler *file_cfg_handler =
      static_cast<mysqlshdk::config::Config_file_handler *>(
          cfg.get_handler(mysqlshdk::config::k_dft_cfg_file_handler));
  file_cfg_handler->remove("skip_log_bin");
  file_cfg_handler->remove("disable_log_bin");
  if (instance->get_version() < mysqlshdk::utils::Version(8, 0, 3)) {
    file_cfg_handler->set("log_bin", nullable<std::string>());
  }
  cfg.apply();
  res.clear();
  mysqlshdk::gr::check_log_bin_compatibility(*instance, cfg, &res);
  EXPECT_EQ(0, res.size());

  cfg.set("log_bin", nullable<std::string>("Some text"),
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.apply();
  res.clear();
  mysqlshdk::gr::check_log_bin_compatibility(*instance, cfg, &res);
  EXPECT_EQ(0, res.size());

  // Delete the config file.
  shcore::delete_file(m_cfg_path, true);
}

TEST_F(Group_replication_test, check_server_id_compatibility) {
  using mysqlshdk::gr::Config_type;
  using mysqlshdk::gr::Config_types;
  using mysqlshdk::gr::Invalid_config;
  using mysqlshdk::mysql::Var_qualifier;
  using mysqlshdk::utils::Version;
  using mysqlshdk::utils::nullable;

  // Get current server_id variable value to restore at the end.
  nullable<int64_t> cur_server_id =
      instance->get_sysvar_int("server_id", Var_qualifier::GLOBAL);

  // Create config object (only with a server handler).
  mysqlshdk::config::Config cfg;
  cfg.add_handler(
      mysqlshdk::config::k_dft_cfg_server_handler,
      std::unique_ptr<mysqlshdk::config::IConfig_handler>(
          shcore::make_unique<mysqlshdk::config::Config_server_handler>(
              instance, Var_qualifier::GLOBAL)));
  std::vector<Invalid_config> res;
  // If server_version >= 8.0.3 and the server_id is the default compiled
  // value, there should be an issue reported.
  if (instance->get_version() >= mysqlshdk::utils::Version(8, 0, 3) &&
      instance->has_variable_compiled_value("server_id")) {
    mysqlshdk::gr::check_server_id_compatibility(*instance, cfg, &res);
    ASSERT_EQ(1, res.size());
    EXPECT_STREQ(res.at(0).var_name.c_str(), "server_id");
    EXPECT_STREQ(res.at(0).required_val.c_str(), "<unique ID>");
    EXPECT_EQ(res.at(0).types, Config_type::SERVER);
    EXPECT_EQ(res.at(0).restart, true);
  }

  // No issues reported if server_id != 0 and has been changed.
  instance->set_sysvar("server_id", static_cast<int64_t>(1),
                       Var_qualifier::GLOBAL);
  res.clear();
  mysqlshdk::gr::check_server_id_compatibility(*instance, cfg, &res);
  EXPECT_EQ(0, res.size());

  // Set server_id=0, and issues will be found
  instance->set_sysvar("server_id", static_cast<int64_t>(0),
                       Var_qualifier::GLOBAL);
  res.clear();
  mysqlshdk::gr::check_server_id_compatibility(*instance, cfg, &res);
  ASSERT_EQ(1, res.size());
  EXPECT_STREQ(res.at(0).var_name.c_str(), "server_id");
  EXPECT_STREQ(res.at(0).current_val.c_str(), "0");
  EXPECT_STREQ(res.at(0).required_val.c_str(), "<unique ID>");
  EXPECT_EQ(res.at(0).types, Config_type::SERVER);
  EXPECT_EQ(res.at(0).restart, true);

  // Create an empty test option file and add the option file config handler.
  create_file(m_cfg_path, "");
  cfg.add_handler(
      mysqlshdk::config::k_dft_cfg_file_handler,
      std::unique_ptr<mysqlshdk::config::IConfig_handler>(
          new mysqlshdk::config::Config_file_handler(m_cfg_path, m_cfg_path)));

  // Issues found on the option file (with no values set).
  // The current value should be the one from the server.
  res.clear();
  mysqlshdk::gr::check_server_id_compatibility(*instance, cfg, &res);
  ASSERT_EQ(1, res.size());
  EXPECT_STREQ(res.at(0).var_name.c_str(), "server_id");
  EXPECT_STREQ(res.at(0).current_val.c_str(), "0");
  EXPECT_STREQ(res.at(0).required_val.c_str(), "<unique ID>");
  EXPECT_TRUE(res.at(0).types.is_set(Config_type::CONFIG));
  EXPECT_TRUE(res.at(0).types.is_set(Config_type::SERVER));
  EXPECT_EQ(res.at(0).restart, true);

  // Fixing the value on the server will still leave the warning for the empty
  // config file
  instance->set_sysvar("server_id", static_cast<int64_t>(1),
                       Var_qualifier::GLOBAL);
  res.clear();
  mysqlshdk::gr::check_server_id_compatibility(*instance, cfg, &res);
  ASSERT_EQ(1, res.size());
  EXPECT_STREQ(res.at(0).var_name.c_str(), "server_id");
  EXPECT_STREQ(res.at(0).current_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_STREQ(res.at(0).required_val.c_str(), "<unique ID>");
  EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(0).restart, false);

  // Fixing the value on the config will clear all warnings.
  cfg.set("server_id", nullable<std::string>("1"),
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.apply();
  res.clear();
  mysqlshdk::gr::check_server_id_compatibility(*instance, cfg, &res);
  EXPECT_EQ(0, res.size());

  // Delete the config file.
  shcore::delete_file(m_cfg_path, true);

  // Restore initial values.
  instance->set_sysvar("server_id", *cur_server_id, Var_qualifier::GLOBAL);
}

TEST_F(Group_replication_test, check_server_variables_compatibility) {
  using mysqlshdk::gr::Config_type;
  using mysqlshdk::gr::Config_types;
  using mysqlshdk::gr::Invalid_config;
  using mysqlshdk::mysql::Var_qualifier;
  using mysqlshdk::utils::Version;
  using mysqlshdk::utils::nullable;

  std::string instance_port =
      std::to_string(instance->get_connection_options().get_port());
  // we will just modify and test the dynamic server variables as well as
  // one of the non dynamic variables from the server. Testing all the non
  // dynamic variables would be impossible as we do not control the state of
  // the instance that is provided and we don't want to restart it.

  // Get the values of the dynamic variables.
  nullable<std::string> cur_binlog_format =
      instance->get_sysvar_string("binlog_format", Var_qualifier::GLOBAL);
  nullable<std::string> cur_binlog_checksum =
      instance->get_sysvar_string("binlog_checksum", Var_qualifier::GLOBAL);
  // now get the value of one of the other variables
  // Note, picked this one because this is the first returned variable on the
  // vector of invalid configs after the two dynamic variables, so if both
  // have correct values, this will be the invalid config at index 0.
  nullable<std::string> cur_log_slave_updates =
      instance->get_sysvar_string("log_slave_updates", Var_qualifier::GLOBAL);
  bool log_slave_updates_correct =
      (*cur_log_slave_updates == "1") || (*cur_log_slave_updates == "ON");

  // Create config object (only with a server handler).
  mysqlshdk::config::Config cfg;
  cfg.add_handler(
      mysqlshdk::config::k_dft_cfg_server_handler,
      std::unique_ptr<mysqlshdk::config::IConfig_handler>(
          shcore::make_unique<mysqlshdk::config::Config_server_handler>(
              instance, Var_qualifier::GLOBAL)));

  // change the dynamic variables so there are no server issues
  instance->set_sysvar("binlog_format", static_cast<std::string>("ROW"),
                       Var_qualifier::GLOBAL);
  instance->set_sysvar("binlog_checksum", static_cast<std::string>("NONE"),
                       Var_qualifier::GLOBAL);
  std::vector<Invalid_config> res;
  mysqlshdk::gr::check_server_variables_compatibility(cfg, &res);
  if (!log_slave_updates_correct) {
    // if the log_slave_updates is not correct, the issues list must at least
    // have the log_slave_updates invalid config  and it must be at the position
    // 0, since the two variables that could appear before are the dynamic ones
    // and those have been set to correct values.
    ASSERT_GE(res.size(), 1);
    EXPECT_STREQ(res.at(0).var_name.c_str(), "log_slave_updates");
    EXPECT_EQ(res.at(0).current_val, *cur_log_slave_updates);
    EXPECT_STREQ(res.at(0).required_val.c_str(), "ON");
    EXPECT_EQ(res.at(0).types, Config_type::SERVER);
    EXPECT_EQ(res.at(0).restart, true);
  }
  // Create an empty test option file and add the option file config handler.
  create_file(m_cfg_path, "");
  mysqlshdk::config::Config cfg_file_only;
  cfg_file_only.add_handler(
      mysqlshdk::config::k_dft_cfg_file_handler,
      std::unique_ptr<mysqlshdk::config::IConfig_handler>(
          new mysqlshdk::config::Config_file_handler(m_cfg_path, m_cfg_path)));

  // Issues found on the option file only (with no values set).
  res.clear();
  mysqlshdk::gr::check_server_variables_compatibility(cfg_file_only, &res);
  // if the config file is empty, there should be an issue for each of the
  // tested variables
  ASSERT_EQ(8, res.size());
  EXPECT_STREQ(res.at(0).var_name.c_str(), "binlog_format");
  EXPECT_STREQ(res.at(0).current_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_STREQ(res.at(0).required_val.c_str(), "ROW");
  EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(0).restart, false);
  EXPECT_STREQ(res.at(1).var_name.c_str(), "binlog_checksum");
  EXPECT_STREQ(res.at(1).current_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_STREQ(res.at(1).required_val.c_str(), "NONE");
  EXPECT_EQ(res.at(1).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(1).restart, false);
  EXPECT_STREQ(res.at(2).var_name.c_str(), "log_slave_updates");
  EXPECT_STREQ(res.at(2).current_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_STREQ(res.at(2).required_val.c_str(), "ON");
  EXPECT_EQ(res.at(2).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(2).restart, false);
  EXPECT_STREQ(res.at(3).var_name.c_str(), "enforce_gtid_consistency");
  EXPECT_STREQ(res.at(3).current_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_STREQ(res.at(3).required_val.c_str(), "ON");
  EXPECT_EQ(res.at(3).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(3).restart, false);
  EXPECT_STREQ(res.at(4).var_name.c_str(), "gtid_mode");
  EXPECT_STREQ(res.at(4).current_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_STREQ(res.at(4).required_val.c_str(), "ON");
  EXPECT_EQ(res.at(4).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(4).restart, false);
  EXPECT_STREQ(res.at(5).var_name.c_str(), "master_info_repository");
  EXPECT_STREQ(res.at(5).current_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_STREQ(res.at(5).required_val.c_str(), "TABLE");
  EXPECT_EQ(res.at(5).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(5).restart, false);
  EXPECT_STREQ(res.at(6).var_name.c_str(), "relay_log_info_repository");
  EXPECT_STREQ(res.at(6).current_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_STREQ(res.at(6).required_val.c_str(), "TABLE");
  EXPECT_EQ(res.at(6).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(6).restart, false);
  EXPECT_STREQ(res.at(7).var_name.c_str(), "transaction_write_set_extraction");
  EXPECT_STREQ(res.at(7).current_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_STREQ(res.at(7).required_val.c_str(), "XXHASH64");
  EXPECT_EQ(res.at(7).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(7).restart, false);

  // add the empty file as well to the cfg handler and check that incorrect
  // server results override the incorrect file results for the current value
  // field of the invalid config.
  cfg.add_handler(
      mysqlshdk::config::k_dft_cfg_file_handler,
      std::unique_ptr<mysqlshdk::config::IConfig_handler>(
          new mysqlshdk::config::Config_file_handler(m_cfg_path, m_cfg_path)));

  // change the dynamic variables so there are server issues
  instance->set_sysvar("binlog_format", static_cast<std::string>("STATEMENT"),
                       Var_qualifier::GLOBAL);
  instance->set_sysvar("binlog_checksum", static_cast<std::string>("CRC32"),
                       Var_qualifier::GLOBAL);
  // Issues found on the option file only (with no values set).
  res.clear();
  mysqlshdk::gr::check_server_variables_compatibility(cfg, &res);
  // since all the file configurations are wrong, we know that even if some
  // variables have correct results on the server, they will still have a
  // invalid config. Since the cfg has a server handler, then it should also
  // have one more entry for the report_port option.
  ASSERT_EQ(9, res.size());
  EXPECT_STREQ(res.at(0).var_name.c_str(), "binlog_format");
  EXPECT_STREQ(res.at(0).current_val.c_str(), "STATEMENT");
  EXPECT_STREQ(res.at(0).required_val.c_str(), "ROW");
  EXPECT_TRUE(res.at(0).types.is_set(Config_type::CONFIG));
  EXPECT_TRUE(res.at(0).types.is_set(Config_type::SERVER));
  EXPECT_EQ(res.at(0).restart, false);
  EXPECT_STREQ(res.at(1).var_name.c_str(), "binlog_checksum");
  EXPECT_STREQ(res.at(1).current_val.c_str(), "CRC32");
  EXPECT_STREQ(res.at(1).required_val.c_str(), "NONE");
  EXPECT_TRUE(res.at(1).types.is_set(Config_type::CONFIG));
  EXPECT_TRUE(res.at(1).types.is_set(Config_type::SERVER));
  EXPECT_EQ(res.at(1).restart, false);
  if (log_slave_updates_correct) {
    EXPECT_STREQ(res.at(2).var_name.c_str(), "log_slave_updates");
    EXPECT_STREQ(res.at(2).current_val.c_str(), mysqlshdk::gr::k_value_not_set);
    EXPECT_STREQ(res.at(2).required_val.c_str(), "ON");
    EXPECT_EQ(res.at(2).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(2).restart, false);
  } else {
    EXPECT_STREQ(res.at(2).var_name.c_str(), "log_slave_updates");
    EXPECT_EQ(res.at(2).current_val, *cur_log_slave_updates);
    EXPECT_STREQ(res.at(2).required_val.c_str(), "ON");
    EXPECT_TRUE(res.at(2).types.is_set(Config_type::CONFIG));
    EXPECT_TRUE(res.at(2).types.is_set(Config_type::SERVER));
    EXPECT_EQ(res.at(2).restart, true);
  }
  EXPECT_STREQ(res.at(8).var_name.c_str(), "report_port");
  EXPECT_STREQ(res.at(8).current_val.c_str(), mysqlshdk::gr::k_value_not_set);
  EXPECT_EQ(res.at(8).required_val, instance_port);
  EXPECT_EQ(res.at(8).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(8).restart, false);

  // Fixing all the config file incorrect values on both config objects
  cfg_file_only.set("binlog_format", nullable<std::string>("ROW"),
                    mysqlshdk::config::k_dft_cfg_file_handler);
  cfg_file_only.set("binlog_checksum", nullable<std::string>("NONE"),
                    mysqlshdk::config::k_dft_cfg_file_handler);
  cfg_file_only.set("log_slave_updates", nullable<std::string>("ON"),
                    mysqlshdk::config::k_dft_cfg_file_handler);
  cfg_file_only.set("enforce_gtid_consistency", nullable<std::string>("ON"),
                    mysqlshdk::config::k_dft_cfg_file_handler);
  cfg_file_only.set("gtid_mode", nullable<std::string>("ON"),
                    mysqlshdk::config::k_dft_cfg_file_handler);
  cfg_file_only.set("master_info_repository", nullable<std::string>("TABLE"),
                    mysqlshdk::config::k_dft_cfg_file_handler);
  cfg_file_only.set("relay_log_info_repository", nullable<std::string>("TABLE"),
                    mysqlshdk::config::k_dft_cfg_file_handler);
  cfg_file_only.set("transaction_write_set_extraction",
                    nullable<std::string>("MURMUR32"),  // different but valid
                    mysqlshdk::config::k_dft_cfg_file_handler);
  cfg_file_only.set("report_port", nullable<std::string>(instance_port),
                    mysqlshdk::config::k_dft_cfg_file_handler);
  cfg_file_only.apply();

  cfg.set("binlog_format", nullable<std::string>("ROW"),
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.set("binlog_checksum", nullable<std::string>("NONE"),
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.set("log_slave_updates", nullable<std::string>("ON"),
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.set("enforce_gtid_consistency", nullable<std::string>("ON"),
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.set("gtid_mode", nullable<std::string>("ON"),
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.set("master_info_repository", nullable<std::string>("TABLE"),
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.set("relay_log_info_repository", nullable<std::string>("TABLE"),
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.set("transaction_write_set_extraction",
          nullable<std::string>("MURMUR32"),  // different but valid
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.set("report_port", nullable<std::string>(instance_port),
          mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.apply();

  // Since all incorrect values have been fixed on the configuration file
  // and the cfg_file_only config object only had a config_file handler then it
  // should have no issues
  res.clear();
  mysqlshdk::gr::check_server_variables_compatibility(cfg_file_only, &res);
  EXPECT_EQ(0, res.size());

  // Since all incorrect values have been fixed on the configuration file
  // but the cfg config object also had a server handler, then it should at
  // least have the issues that are still present on the server.

  res.clear();
  mysqlshdk::gr::check_server_variables_compatibility(cfg, &res);
  if (log_slave_updates_correct) {
    ASSERT_GE(res.size(), 2);
  } else {
    ASSERT_GE(res.size(), 3);
    EXPECT_STREQ(res.at(2).var_name.c_str(), "log_slave_updates");
    EXPECT_EQ(res.at(2).current_val, *cur_log_slave_updates);
    EXPECT_STREQ(res.at(2).required_val.c_str(), "ON");
    EXPECT_EQ(res.at(2).types, Config_type::SERVER);
    EXPECT_EQ(res.at(2).restart, true);
  }
  EXPECT_STREQ(res.at(0).var_name.c_str(), "binlog_format");
  EXPECT_STREQ(res.at(0).current_val.c_str(), "STATEMENT");
  EXPECT_STREQ(res.at(0).required_val.c_str(), "ROW");
  EXPECT_EQ(res.at(0).types, Config_type::SERVER);
  EXPECT_EQ(res.at(0).restart, false);
  EXPECT_STREQ(res.at(1).var_name.c_str(), "binlog_checksum");
  EXPECT_STREQ(res.at(1).current_val.c_str(), "CRC32");
  EXPECT_STREQ(res.at(1).required_val.c_str(), "NONE");
  EXPECT_EQ(res.at(1).types, Config_type::SERVER);
  EXPECT_EQ(res.at(1).restart, false);

  // Delete the config file.
  shcore::delete_file(m_cfg_path, true);

  // Restore initial values.
  instance->set_sysvar("binlog_format", *cur_binlog_format,
                       Var_qualifier::GLOBAL);
  instance->set_sysvar("binlog_checksum", *cur_binlog_checksum,
                       Var_qualifier::GLOBAL);
}

TEST_F(Group_replication_test, is_active_member) {
  using mysqlshdk::db::Type;

  std::shared_ptr<Mock_session> mock_session = std::make_shared<Mock_session>();
  mysqlshdk::mysql::Instance instance{mock_session};

  mock_session
      ->expect_query(
          "SELECT Member_state "
          "FROM performance_schema.replication_group_members "
          "WHERE Member_host = 'localhost' AND Member_port = 3306 "
          "AND Member_state NOT IN ('OFFLINE', 'UNREACHABLE')")
      .then_return({{"", {"Member_state"}, {Type::String}, {}}});
  EXPECT_FALSE(mysqlshdk::gr::is_active_member(instance, "localhost", 3306));

  mock_session
      ->expect_query(
          "SELECT Member_state "
          "FROM performance_schema.replication_group_members "
          "WHERE Member_host = 'localhost' AND Member_port = 3306 "
          "AND Member_state NOT IN ('OFFLINE', 'UNREACHABLE')")
      .then_return({{"", {"Member_state"}, {Type::String}, {{"ONLINE"}}}});
  EXPECT_TRUE(mysqlshdk::gr::is_active_member(instance, "localhost", 3306));
}

TEST_F(Group_replication_test, update_auto_increment) {
  using mysqlshdk::db::Type;
  using mysqlshdk::gr::Topology_mode;
  using mysqlshdk::mysql::Var_qualifier;

  std::shared_ptr<Mock_session> mock_session = std::make_shared<Mock_session>();
  mysqlshdk::mysql::Instance instance{mock_session};

  // Create config objects with 3 server handlers.
  mysqlshdk::config::Config cfg_global;
  for (int i = 0; i < 3; i++) {
    cfg_global.add_handler(
        "instance_" + std::to_string(i),
        std::unique_ptr<mysqlshdk::config::IConfig_handler>(
            shcore::make_unique<mysqlshdk::config::Config_server_handler>(
                &instance, Var_qualifier::GLOBAL)));
  }

  // Set auto-increment for single-primary (3 instances).
  EXPECT_CALL(*mock_session,
              execute("SET GLOBAL `auto_increment_increment` = 1"))
      .Times(3);
  EXPECT_CALL(*mock_session, execute("SET GLOBAL `auto_increment_offset` = 2"))
      .Times(3);
  mysqlshdk::gr::update_auto_increment(&cfg_global,
                                       Topology_mode::SINGLE_PRIMARY);
  cfg_global.apply();

  // Create config objects with 10 server handlers with PERSIST support.
  mysqlshdk::config::Config cfg_persist;
  for (int i = 0; i < 10; i++) {
    cfg_persist.add_handler(
        "instance_" + std::to_string(i),
        std::unique_ptr<mysqlshdk::config::IConfig_handler>(
            shcore::make_unique<mysqlshdk::config::Config_server_handler>(
                &instance, Var_qualifier::PERSIST)));
  }

  // Set auto-increment for single-primary (10 instances with PERSIST support).
  EXPECT_CALL(*mock_session,
              execute("SET PERSIST `auto_increment_increment` = 1"))
      .Times(10);
  EXPECT_CALL(*mock_session, execute("SET PERSIST `auto_increment_offset` = 2"))
      .Times(10);
  mysqlshdk::gr::update_auto_increment(&cfg_persist,
                                       Topology_mode::SINGLE_PRIMARY);
  cfg_persist.apply();

  // Remove server handlers to leave only one.
  // NOTE: There is a limitation for Mock_session::expect_query(), not allowing
  // the same query to properly return a result when used consecutively (only
  // the first execution of the query will work and no rows are returned by the
  // following). Thus, getting the required server_id for more than one server
  // will not work using the same Mock_session.
  for (int i = 1; i < 3; i++) {
    std::string instance_address = "instance_" + std::to_string(i);
    cfg_global.remove_handler(instance_address);
  }

  // Set auto-increment for multi-primary with 1 server handler.
  EXPECT_CALL(*mock_session,
              execute("SET GLOBAL `auto_increment_increment` = 7"));
  mock_session
      ->expect_query(
          "show GLOBAL variables where `variable_name` in ('server_id')")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"server_id", "4"}}}});
  EXPECT_CALL(*mock_session, execute("SET GLOBAL `auto_increment_offset` = 5"));
  mysqlshdk::gr::update_auto_increment(&cfg_global,
                                       Topology_mode::MULTI_PRIMARY);
  cfg_global.apply();

  // Remove server handlers to leave only one.
  // NOTE: There is a limitation for Mock_session::expect_query(), not allowing
  // the same query to properly return a result when used consecutively (only
  // the first execution of the query will work and no rows are returned by the
  // following). Thus, getting the required server_id for more than one server
  // will not work using the same Mock_session.
  for (int i = 1; i < 10; i++) {
    cfg_persist.remove_handler("instance_" + std::to_string(i));
  }

  // Set auto-increment for multi-primary with 1 server handler with PERSIST.
  EXPECT_CALL(*mock_session,
              execute("SET PERSIST `auto_increment_increment` = 7"));
  mock_session
      ->expect_query(
          "show GLOBAL variables where `variable_name` in ('server_id')")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"server_id", "7"}}}});
  EXPECT_CALL(*mock_session,
              execute("SET PERSIST `auto_increment_offset` = 1"));
  mysqlshdk::gr::update_auto_increment(&cfg_persist,
                                       Topology_mode::MULTI_PRIMARY);
  cfg_persist.apply();
}

}  // namespace testing
