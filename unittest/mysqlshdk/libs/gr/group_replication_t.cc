/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
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
#include <vector>

#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/config/config_file_handler.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/repl_config.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/mysql/mock_instance.h"
#include "unittest/test_utils/shell_base_test.h"

namespace testing {

class Group_replication_test : public tests::Shell_base_test {
 protected:
  void SetUp() {
    tests::Shell_base_test::SetUp();

    // Create instance and Open the session for the tests.
    _connection_options = mysqlshdk::db::Connection_options(_mysql_uri);
    _session->connect(_connection_options);
    m_instance = new mysqlshdk::mysql::Instance(_session);

    // Get temp dir path
    m_tmpdir = getenv("TMPDIR");
    m_cfg_path = shcore::path::join_path(m_tmpdir, "my_gr_test.cnf");
  }

  void TearDown() {
    tests::Shell_base_test::TearDown();

    // Close the session.
    _session->close();
    delete m_instance;
  }

  std::shared_ptr<mysqlshdk::db::ISession> _session =
      mysqlshdk::db::mysql::Session::create();
  mysqlshdk::mysql::Instance *m_instance;
  mysqlshdk::db::Connection_options _connection_options;

  std::string m_tmpdir, m_cfg_path;
};

TEST_F(Group_replication_test, plugin_installation) {
  // Check if GR plugin is installed and uninstall it.
  std::optional<std::string> init_plugin_state =
      m_instance->get_plugin_status(mysqlshdk::gr::k_gr_plugin_name);
  if (init_plugin_state.has_value()) {
    // Test uninstall the plugin when available.
    bool res =
        mysqlshdk::gr::uninstall_group_replication_plugin(*m_instance, nullptr);
    EXPECT_TRUE(res);
    std::optional<std::string> plugin_state =
        m_instance->get_plugin_status(mysqlshdk::gr::k_gr_plugin_name);
    ASSERT_TRUE(!plugin_state.has_value());
    // Test trying to uninstall the plugin when not available.
    res =
        mysqlshdk::gr::uninstall_group_replication_plugin(*m_instance, nullptr);
    EXPECT_FALSE(res);
  }

  // Test installing the plugin (when not installed).
  if (init_plugin_state.has_value() && (init_plugin_state == "DISABLED")) {
    // An exception is expected if the plugin was disabled.
    EXPECT_THROW(
        mysqlshdk::gr::install_group_replication_plugin(*m_instance, nullptr),
        std::runtime_error);
  } else {
    // Requirements to install the GR plugin:
    // - server_id != 0
    // - master_info_repository=TABLE (if version is lower than 8.3.0)
    // - relay_log_info_repository=TABLE (if version is lower than 8.3.0)
    std::optional<int64_t> server_id = m_instance->get_sysvar_int("server_id");
    if (*server_id == 0) {
      SKIP_TEST("Test server does not meet GR requirements: server_id is 0.");
    }

    if (m_instance->get_version() < mysqlshdk::utils::Version(8, 3, 0)) {
      auto master_info_repository =
          m_instance->get_sysvar_string("master_info_repository");
      if (master_info_repository.value_or("") != "TABLE") {
        SKIP_TEST(
            "Test server does not meet GR requirements: master_info_repository "
            "must be 'TABLE'.");
      }
      auto relay_log_info_repository =
          m_instance->get_sysvar_string("relay_log_info_repository");
      if (relay_log_info_repository.value_or("") != "TABLE") {
        SKIP_TEST(
            "Test server does not meet GR requirements: "
            "relay_log_info_repository must be 'TABLE'.");
      }
    }

    // GR plugin is installed and activated (if not previously disabled).
    bool res =
        mysqlshdk::gr::install_group_replication_plugin(*m_instance, nullptr);
    ASSERT_TRUE(res)
        << "GR plugin was not installed (expected not to be available).";
    std::optional<std::string> plugin_state =
        m_instance->get_plugin_status(mysqlshdk::gr::k_gr_plugin_name);
    EXPECT_STREQ("ACTIVE", (*plugin_state).c_str());

    // Test installing the plugin when already installed.
    res = mysqlshdk::gr::install_group_replication_plugin(*m_instance, nullptr);
    EXPECT_FALSE(res)
        << "GR plugin was installed (expected to be already available).";
    plugin_state =
        m_instance->get_plugin_status(mysqlshdk::gr::k_gr_plugin_name);
    EXPECT_STREQ("ACTIVE", (*plugin_state).c_str());
  }

  // Restore initial state (uninstall plugin if needed).
  if (!init_plugin_state.has_value()) {
    // Test uninstall the plugin when available.
    bool res =
        mysqlshdk::gr::uninstall_group_replication_plugin(*m_instance, nullptr);
    EXPECT_TRUE(res);
    std::optional<std::string> plugin_state =
        m_instance->get_plugin_status(mysqlshdk::gr::k_gr_plugin_name);
    ASSERT_TRUE(!plugin_state.has_value());

    // Test trying to uninstall the plugin when not available.
    res =
        mysqlshdk::gr::uninstall_group_replication_plugin(*m_instance, nullptr);
    EXPECT_FALSE(res);
  }
}

TEST_F(Group_replication_test, create_recovery_user) {
  if (!Shell_test_env::check_min_version_skip_test()) return;

  // Confirm that there is no replication user.
  auto res =
      mysqlshdk::gr::check_replication_user(*m_instance, "test_gr_user", "%");
  EXPECT_FALSE(res.user_exists());
  EXPECT_EQ(std::set<std::string>{"REPLICATION SLAVE"},
            res.missing_privileges());
  EXPECT_TRUE(res.has_missing_privileges());
  EXPECT_FALSE(res.has_grant_option());

  // Create a recovery user with a random password.
  mysqlshdk::mysql::Auth_options creds;
  {
    std::vector<std::string> hosts;
    hosts.push_back("%");

    creds = mysqlshdk::gr::create_recovery_user("test_gr_user", *m_instance,
                                                hosts, {});
  }
  // Check replication user (now it exist and it has no missing privileges).
  res = mysqlshdk::gr::check_replication_user(*m_instance, "test_gr_user", "%");

  EXPECT_TRUE(res.user_exists());
  EXPECT_EQ(std::set<std::string>{}, res.missing_privileges());
  EXPECT_FALSE(res.has_missing_privileges());
  EXPECT_FALSE(res.has_grant_option());
  EXPECT_EQ(creds.user, "test_gr_user");
  // it is expected a random password is generated when using an empty password
  // as parameter.
  EXPECT_EQ(true, creds.password.has_value());

  // Drop user and recreate it with a given password
  {
    std::vector<std::string> hosts;
    hosts.push_back("%");

    mysqlshdk::gr::Create_recovery_user_options options;
    options.password = "password";

    creds = mysqlshdk::gr::create_recovery_user("test_gr_user", *m_instance,
                                                hosts, options);
  }

  // Check replication user (now it exist and it has no missing privileges).
  res = mysqlshdk::gr::check_replication_user(*m_instance, "test_gr_user", "%");

  EXPECT_TRUE(res.user_exists());
  EXPECT_EQ(std::set<std::string>{}, res.missing_privileges());
  EXPECT_FALSE(res.has_missing_privileges());
  EXPECT_FALSE(res.has_grant_option());
  EXPECT_EQ(creds.user, "test_gr_user");
  EXPECT_TRUE(static_cast<bool>(creds.password));
  EXPECT_EQ(*creds.password, "password");

  // Clean up (remove the create user at the end)
  m_instance->drop_user("test_gr_user", "%");
}

TEST_F(Group_replication_test, start_stop_gr) {
  // Beside the start_group_replication() and stop_group_replication()
  // functions, the function get_member_state() is also
  // tested here since the test scenario is the same, in order to avoid
  // additional execution time to run similar test cases.
  // NOTE: START and STOP GROUP_REPLICATION is slow.

  using mysqlshdk::gr::Member_state;
  using mysqlshdk::mysql::Var_qualifier;

  // Check if used server meets the requirements.
  std::optional<int64_t> server_id = m_instance->get_sysvar_int("server_id");
  if (*server_id == 0) {
    SKIP_TEST("Test server does not meet GR requirements: server_id is 0.");
  }
  bool log_bin = m_instance->get_sysvar_bool("log_bin", false);
  if (!log_bin) {
    SKIP_TEST("Test server does not meet GR requirements: log_bin must be ON.");
  }
  bool gtid_mode = m_instance->get_sysvar_bool("gtid_mode", false);
  if (!gtid_mode) {
    SKIP_TEST(
        "Test server does not meet GR requirements: gtid_mode must be ON.");
  }
  bool enforce_gtid_consistency =
      m_instance->get_sysvar_bool("enforce_gtid_consistency", false);
  if (!enforce_gtid_consistency) {
    SKIP_TEST(
        "Test server does not meet GR requirements: enforce_gtid_consistency "
        "must be ON.");
  }

  if (m_instance->get_version() < mysqlshdk::utils::Version(8, 3, 0)) {
    auto master_info_repository =
        m_instance->get_sysvar_string("master_info_repository");
    if (master_info_repository.value_or("") != "TABLE") {
      SKIP_TEST(
          "Test server does not meet GR requirements: master_info_repository "
          "must be 'TABLE'.");
    }
    auto relay_log_info_repository =
        m_instance->get_sysvar_string("relay_log_info_repository");
    if (relay_log_info_repository.value_or("") != "TABLE") {
      SKIP_TEST(
          "Test server does not meet GR requirements: "
          "relay_log_info_repository must be 'TABLE'.");
    }
  }

  std::optional<std::string> binlog_checksum =
      m_instance->get_sysvar_string("binlog_checksum");
  if ((*binlog_checksum).compare("NONE") != 0 &&
      m_instance->get_version() < mysqlshdk::utils::Version(8, 0, 21)) {
    SKIP_TEST(
        "Test server does not meet GR requirements: binlog_checksum must be "
        "'NONE'.");
  }
  bool log_slave_updates =
      m_instance->get_sysvar_bool("log_slave_updates", false);
  if (!log_slave_updates) {
    SKIP_TEST(
        "Test server does not meet GR requirements: log_slave_updates must "
        "be ON.");
  }
  std::optional<std::string> binlog_format =
      m_instance->get_sysvar_string("binlog_format");
  if ((*binlog_format).compare("ROW") != 0) {
    SKIP_TEST(
        "Test server does not meet GR requirements: binlog_format must be "
        "'ROW'.");
  }

  // Test: member is not part of any group, state must be MISSING.
  Member_state state_res = mysqlshdk::gr::get_member_state(*m_instance);
  EXPECT_EQ(state_res, Member_state::MISSING);

  // Install GR plugin if needed.
  std::optional<std::string> init_plugin_state =
      m_instance->get_plugin_status(mysqlshdk::gr::k_gr_plugin_name);
  if (!init_plugin_state.has_value()) {
    mysqlshdk::gr::install_group_replication_plugin(*m_instance, nullptr);
  }

  // Get initial value of GR variables (to restore at the end).
  std::optional<std::string> gr_group_name =
      m_instance->get_sysvar_string("group_replication_group_name");
  std::optional<std::string> gr_local_address =
      m_instance->get_sysvar_string("group_replication_local_address");

  // Set GR variable to start GR.
  std::string group_name = m_instance->generate_uuid();
  m_instance->set_sysvar("group_replication_group_name", group_name,
                         Var_qualifier::GLOBAL);
  std::string local_address = "localhost:13013";
  m_instance->set_sysvar("group_replication_local_address", local_address,
                         Var_qualifier::GLOBAL);

  // Test: Start Group Replication.
  mysqlshdk::gr::start_group_replication(*m_instance, true);

  // SUPER READ ONLY must be OFF (verify wait for it to be disable).
  bool read_only = m_instance->get_sysvar_bool("super_read_only", false);
  EXPECT_FALSE(read_only);

  // Test: member is part of GR group, state must be RECOVERING or ONLINE.
  state_res = mysqlshdk::gr::get_member_state(*m_instance);
  if (state_res == Member_state::ONLINE ||
      state_res == Member_state::RECOVERING)
    SUCCEED();
  else
    ADD_FAILURE() << "Unexpected status after starting GR, member state must "
                     "be ONLINE or RECOVERING";

  // Check GR server status (must be RECOVERING or ONLINE).
  auto session = m_instance->get_session();
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
  EXPECT_THROW(mysqlshdk::gr::start_group_replication(*m_instance, true),
               std::exception);

  // Test: Stop Group Replication.
  mysqlshdk::gr::stop_group_replication(*m_instance);

  // Starting from MySQL 5.7.20 GR automatically enables super_read_only after
  // stop. Thus, always disable read_only ro consider this situation.
  m_instance->set_sysvar("super_read_only", false, Var_qualifier::GLOBAL);
  m_instance->set_sysvar("read_only", false, Var_qualifier::GLOBAL);

  // Test: member is still part of the group, but its state is OFFLINE.
  state_res = mysqlshdk::gr::get_member_state(*m_instance);
  EXPECT_EQ(state_res, Member_state::OFFLINE);

  // Clean up (restore initial server state).
  if (!gr_group_name->empty())
    // NOTE: The group_name cannot be set with an empty value.
    m_instance->set_sysvar("group_replication_group_name", *gr_group_name,
                           Var_qualifier::GLOBAL);
  m_instance->set_sysvar("group_replication_local_address", *gr_local_address,
                         Var_qualifier::GLOBAL);
  if (!init_plugin_state.has_value()) {
    mysqlshdk::gr::uninstall_group_replication_plugin(*m_instance, nullptr);
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

TEST_F(Group_replication_test, is_running_gr_auto_rejoin) {
  using mysqlshdk::db::Type;

  std::shared_ptr<Mock_session> mock_session = std::make_shared<Mock_session>();
  mysqlshdk::mysql::Instance instance{mock_session};

  mock_session
      ->expect_query(
          "SELECT PROCESSLIST_STATE FROM performance_schema.threads WHERE NAME "
          "= 'thread/group_rpl/THD_autorejoin'")
      .then_return({{"",
                     {"PROCESSLIST_STATE"},
                     {Type::String},
                     {{"Undergoing auto-rejoin procedure"}}}});
  EXPECT_TRUE(mysqlshdk::gr::is_running_gr_auto_rejoin(instance));

  mock_session
      ->expect_query(
          "SELECT PROCESSLIST_STATE FROM performance_schema.threads WHERE NAME "
          "= 'thread/group_rpl/THD_autorejoin'")
      .then_return({{"", {"PROCESSLIST_STATE"}, {Type::String}, {}}});
  EXPECT_FALSE(mysqlshdk::gr::is_running_gr_auto_rejoin(instance));
}

TEST_F(Group_replication_test, get_all_configurations) {
  std::map<std::string, std::optional<std::string>> res =
      mysqlshdk::gr::get_all_configurations(*m_instance);

  // NOTE: Only auto_increment variables are returned if GR is not configured.
  //       Check only those values to avoid non-deterministic issues, depending
  //       on the server version and/or configuration (e.g., new
  //       group_replication_consistency variable added for 8.0.14 servers).
  std::vector<std::string> vars;
  std::vector<std::string> values;
  for (const auto &[name, val] : res) {
    vars.push_back(name);
    values.push_back(*val);
  }
  EXPECT_THAT(vars, Contains("auto_increment_increment"));
  EXPECT_THAT(vars, Contains("auto_increment_offset"));
}

TEST_F(Group_replication_test, check_log_bin_compatibility_disabled_57) {
  using mysqlshdk::mysql::Config_type;
  using mysqlshdk::mysql::Config_types;
  using mysqlshdk::mysql::Invalid_config;
  using mysqlshdk::mysql::Var_qualifier;
  using namespace std::literals;

  mysqlshdk::mysql::Mock_instance inst;

  std::vector<Invalid_config> res;

  // Test everything assuming 5.7, with binlog default OFF and no SET PERSIST
  EXPECT_CALL(inst, get_sysvar_bool("log_bin"sv, Var_qualifier::GLOBAL))
      .WillRepeatedly(Return(std::optional<bool>(false)));
  EXPECT_CALL(inst, get_sysvar_string("log_bin"sv, Var_qualifier::GLOBAL))
      .WillRepeatedly(Return(std::optional<std::string>("OFF")));
  EXPECT_CALL(inst, get_version())
      .WillRepeatedly(Return(mysqlshdk::utils::Version(5, 7, 24)));

  // Create config object (only with a server handler).
  mysqlshdk::config::Config cfg;
  cfg.add_handler(mysqlshdk::config::k_dft_cfg_server_handler,
                  std::make_unique<mysqlshdk::config::Config_server_handler>(
                      &inst, Var_qualifier::GLOBAL));

  mysqlshdk::mysql::check_log_bin_compatibility(inst, cfg, &res);
  // should have 3 issues in 5.7, since binary log is disabled.
  ASSERT_EQ(3, res.size());
  EXPECT_STREQ(res.at(0).var_name.c_str(), "log_bin");
  EXPECT_STREQ(res.at(0).current_val.c_str(),
               mysqlshdk::mysql::k_value_not_set);
  EXPECT_STREQ(res.at(0).required_val.c_str(), mysqlshdk::mysql::k_no_value);
  EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(0).restart, true);
  EXPECT_STREQ(res.at(1).var_name.c_str(), "skip_log_bin");
  EXPECT_STREQ(res.at(1).current_val.c_str(), "<not set>");
  EXPECT_STREQ(res.at(1).required_val.c_str(), "<not set>");
  EXPECT_EQ(res.at(1).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(1).restart, true);
  EXPECT_STREQ(res.at(2).var_name.c_str(), "disable_log_bin");
  EXPECT_STREQ(res.at(2).current_val.c_str(), "<not set>");
  EXPECT_STREQ(res.at(2).required_val.c_str(), "<not set>");
  EXPECT_EQ(res.at(2).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(2).restart, true);

  // Create an empty test option file and add the option file config handler.
  create_file(m_cfg_path, "");
  cfg.add_handler(mysqlshdk::config::k_dft_cfg_file_handler,
                  std::unique_ptr<mysqlshdk::config::IConfig_handler>(
                      new mysqlshdk::config::Config_file_handler(
                          "uuid1", m_cfg_path, m_cfg_path)));

  // Issues found on both server and option file (with no values set).
  res.clear();
  mysqlshdk::mysql::check_log_bin_compatibility(inst, cfg, &res);

  ASSERT_EQ(1, res.size());
  EXPECT_STREQ(res.at(0).var_name.c_str(), "log_bin");
  EXPECT_STREQ(res.at(0).current_val.c_str(),
               mysqlshdk::mysql::k_value_not_set);
  EXPECT_STREQ(res.at(0).required_val.c_str(), mysqlshdk::mysql::k_no_value);
  EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(0).restart, true);

  // Set incompatible values on file, issues found.
  cfg.set_for_handler("skip_log_bin", std::optional<std::string>(),
                      mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.set_for_handler("disable_log_bin", std::optional<std::string>(),
                      mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.apply();

  res.clear();
  mysqlshdk::mysql::check_log_bin_compatibility(inst, cfg, &res);

  ASSERT_EQ(3, res.size());
  EXPECT_STREQ(res.at(0).var_name.c_str(), "log_bin");
  EXPECT_STREQ(res.at(0).current_val.c_str(),
               mysqlshdk::mysql::k_value_not_set);
  EXPECT_STREQ(res.at(0).required_val.c_str(), mysqlshdk::mysql::k_no_value);
  EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(0).restart, true);
  EXPECT_STREQ(res.at(1).var_name.c_str(), "disable_log_bin");
  EXPECT_STREQ(res.at(1).current_val.c_str(), mysqlshdk::mysql::k_no_value);
  EXPECT_STREQ(res.at(1).required_val.c_str(),
               mysqlshdk::mysql::k_value_not_set);
  EXPECT_EQ(res.at(1).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(1).restart, true);
  EXPECT_STREQ(res.at(2).var_name.c_str(), "skip_log_bin");
  EXPECT_STREQ(res.at(2).current_val.c_str(), mysqlshdk::mysql::k_no_value);
  EXPECT_STREQ(res.at(2).required_val.c_str(),
               mysqlshdk::mysql::k_value_not_set);
  EXPECT_EQ(res.at(2).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(2).restart, true);

  // Delete the config file.
  shcore::delete_file(m_cfg_path, true);
}

TEST_F(Group_replication_test, check_log_bin_compatibility_disabled_80) {
  using mysqlshdk::mysql::Config_type;
  using mysqlshdk::mysql::Config_types;
  using mysqlshdk::mysql::Invalid_config;
  using mysqlshdk::mysql::Var_qualifier;
  using namespace std::literals;

  mysqlshdk::mysql::Mock_instance inst;

  std::vector<Invalid_config> res;

  // With 8.0, "log_bin" is read-only and cannot be persisted. The only way to
  // disable it is with skip_log_bin or disable_log_bin
  EXPECT_CALL(inst, get_sysvar_bool("log_bin"sv, Var_qualifier::GLOBAL))
      .WillRepeatedly(Return(std::optional<bool>(false)));
  EXPECT_CALL(inst, get_sysvar_string("log_bin"sv, Var_qualifier::GLOBAL))
      .WillRepeatedly(Return(std::optional<std::string>("OFF")));
  EXPECT_CALL(inst, get_version())
      .WillRepeatedly(Return(mysqlshdk::utils::Version(8, 0, 3)));

  // Create config object (only with a server handler).
  mysqlshdk::config::Config cfg;
  cfg.add_handler(mysqlshdk::config::k_dft_cfg_server_handler,
                  std::make_unique<mysqlshdk::config::Config_server_handler>(
                      &inst, Var_qualifier::GLOBAL));

  mysqlshdk::mysql::check_log_bin_compatibility(inst, cfg, &res);
  EXPECT_STREQ(res.at(0).var_name.c_str(), "skip_log_bin");
  EXPECT_STREQ(res.at(0).current_val.c_str(),
               mysqlshdk::mysql::k_value_not_set);
  EXPECT_STREQ(res.at(0).required_val.c_str(),
               mysqlshdk::mysql::k_value_not_set);
  EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(0).restart, true);
  EXPECT_STREQ(res.at(1).var_name.c_str(), "disable_log_bin");
  EXPECT_STREQ(res.at(1).current_val.c_str(),
               mysqlshdk::mysql::k_value_not_set);
  EXPECT_STREQ(res.at(1).required_val.c_str(),
               mysqlshdk::mysql::k_value_not_set);
  EXPECT_EQ(res.at(1).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(1).restart, true);

  // Create an empty test option file and add the option file config handler.
  create_file(m_cfg_path, "");
  cfg.add_handler(mysqlshdk::config::k_dft_cfg_file_handler,
                  std::unique_ptr<mysqlshdk::config::IConfig_handler>(
                      new mysqlshdk::config::Config_file_handler(
                          "uuid1", m_cfg_path, m_cfg_path)));

  // Issues found on both server and option file (with no values set).
  res.clear();
  mysqlshdk::mysql::check_log_bin_compatibility(inst, cfg, &res);

  // if server version is >=8.0.3 then the log_bin is enabled by default so
  // there is no need to be the log_bin option on the file
  ASSERT_EQ(1, res.size());
  EXPECT_STREQ(res.at(0).var_name.c_str(), "log_bin");
  EXPECT_STREQ(res.at(0).current_val.c_str(), "OFF");
  EXPECT_STREQ(res.at(0).required_val.c_str(), "ON");
  EXPECT_EQ(res.at(0).types, Config_type::RESTART_ONLY);
  EXPECT_EQ(res.at(0).restart, true);

  // Set incompatible values on file, issues found.
  cfg.set_for_handler("skip_log_bin", std::optional<std::string>(),
                      mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.set_for_handler("disable_log_bin", std::optional<std::string>(),
                      mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.apply();

  res.clear();
  mysqlshdk::mysql::check_log_bin_compatibility(inst, cfg, &res);
  ASSERT_EQ(2, res.size());
  EXPECT_STREQ(res.at(0).var_name.c_str(), "disable_log_bin");
  EXPECT_STREQ(res.at(0).current_val.c_str(), mysqlshdk::mysql::k_no_value);
  EXPECT_STREQ(res.at(0).required_val.c_str(),
               mysqlshdk::mysql::k_value_not_set);
  EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(0).restart, true);
  EXPECT_STREQ(res.at(1).var_name.c_str(), "skip_log_bin");
  EXPECT_STREQ(res.at(1).current_val.c_str(), mysqlshdk::mysql::k_no_value);
  EXPECT_STREQ(res.at(1).required_val.c_str(),
               mysqlshdk::mysql::k_value_not_set);
  EXPECT_EQ(res.at(1).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(1).restart, true);

  // Delete the config file.
  shcore::delete_file(m_cfg_path, true);
}

TEST_F(Group_replication_test, check_log_bin_compatibility_enabled) {
  using mysqlshdk::mysql::Config_type;
  using mysqlshdk::mysql::Config_types;
  using mysqlshdk::mysql::Invalid_config;
  using mysqlshdk::mysql::Var_qualifier;

  bool log_bin = m_instance->get_sysvar_bool("log_bin", false);
  if (!log_bin) {
    SKIP_TEST(
        "Test server does not meet GR requirements: binary_log is disabled and "
        "should be enabled.")
  }
  // Create config object (only with a server handler).
  mysqlshdk::config::Config cfg;
  cfg.add_handler(
      mysqlshdk::config::k_dft_cfg_server_handler,
      std::unique_ptr<mysqlshdk::config::IConfig_handler>(
          std::make_unique<mysqlshdk::config::Config_server_handler>(
              m_instance, Var_qualifier::GLOBAL)));

  // should have no issues, since binary log is enabled.
  std::vector<Invalid_config> res;
  mysqlshdk::mysql::check_log_bin_compatibility(*m_instance, cfg, &res);
  EXPECT_EQ(0, res.size());

  // Create an empty test option file and add the option file config handler.
  create_file(m_cfg_path, "");
  cfg.add_handler(mysqlshdk::config::k_dft_cfg_file_handler,
                  std::unique_ptr<mysqlshdk::config::IConfig_handler>(
                      new mysqlshdk::config::Config_file_handler(
                          "uuid1", m_cfg_path, m_cfg_path)));

  // Issues found on the option file (with no values set).
  res.clear();
  mysqlshdk::mysql::check_log_bin_compatibility(*m_instance, cfg, &res);
  // if server version is >=8.0.3 then the log_bin is enabled by default so
  // there is no need to be the log_bin option on the file
  if (m_instance->get_version() >= mysqlshdk::utils::Version(8, 0, 3)) {
    ASSERT_EQ(0, res.size());
  } else {
    ASSERT_EQ(1, res.size());
    EXPECT_STREQ(res.at(0).var_name.c_str(), "log_bin");
    EXPECT_STREQ(res.at(0).current_val.c_str(),
                 mysqlshdk::mysql::k_value_not_set);
    EXPECT_STREQ(res.at(0).required_val.c_str(), mysqlshdk::mysql::k_no_value);
    EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(0).restart, true);
  }

  // Set incompatible values on file, issues found.
  cfg.set_for_handler("skip_log_bin", std::optional<std::string>(),
                      mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.set_for_handler("disable_log_bin", std::optional<std::string>(),
                      mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.apply();

  res.clear();
  mysqlshdk::mysql::check_log_bin_compatibility(*m_instance, cfg, &res);
  if (m_instance->get_version() >= mysqlshdk::utils::Version(8, 0, 3)) {
    ASSERT_EQ(2, res.size());
    EXPECT_STREQ(res.at(0).var_name.c_str(), "disable_log_bin");
    EXPECT_STREQ(res.at(0).current_val.c_str(), mysqlshdk::mysql::k_no_value);
    EXPECT_STREQ(res.at(0).required_val.c_str(),
                 mysqlshdk::mysql::k_value_not_set);
    EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(0).restart, true);
    EXPECT_STREQ(res.at(1).var_name.c_str(), "skip_log_bin");
    EXPECT_STREQ(res.at(1).current_val.c_str(), mysqlshdk::mysql::k_no_value);
    EXPECT_STREQ(res.at(1).required_val.c_str(),
                 mysqlshdk::mysql::k_value_not_set);
    EXPECT_EQ(res.at(1).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(1).restart, true);
  } else {
    ASSERT_EQ(3, res.size());
    EXPECT_STREQ(res.at(0).var_name.c_str(), "log_bin");
    EXPECT_STREQ(res.at(0).current_val.c_str(),
                 mysqlshdk::mysql::k_value_not_set);
    EXPECT_STREQ(res.at(0).required_val.c_str(), mysqlshdk::mysql::k_no_value);
    EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(0).restart, true);
    EXPECT_STREQ(res.at(1).var_name.c_str(), "disable_log_bin");
    EXPECT_STREQ(res.at(1).current_val.c_str(), mysqlshdk::mysql::k_no_value);
    EXPECT_STREQ(res.at(1).required_val.c_str(),
                 mysqlshdk::mysql::k_value_not_set);
    EXPECT_EQ(res.at(1).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(1).restart, true);
    EXPECT_STREQ(res.at(2).var_name.c_str(), "skip_log_bin");
    EXPECT_STREQ(res.at(2).current_val.c_str(), mysqlshdk::mysql::k_no_value);
    EXPECT_STREQ(res.at(2).required_val.c_str(),
                 mysqlshdk::mysql::k_value_not_set);
    EXPECT_EQ(res.at(2).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(2).restart, true);
  }

  // Set compatible values on file, no issues found.
  // Get the config file handler reference (to remove option from config file).
  mysqlshdk::config::Config_file_handler *file_cfg_handler =
      static_cast<mysqlshdk::config::Config_file_handler *>(
          cfg.get_handler(mysqlshdk::config::k_dft_cfg_file_handler));
  file_cfg_handler->remove("skip_log_bin");
  file_cfg_handler->remove("disable_log_bin");
  if (m_instance->get_version() < mysqlshdk::utils::Version(8, 0, 3)) {
    file_cfg_handler->set("log_bin", std::optional<std::string>());
  }
  cfg.apply();
  res.clear();
  mysqlshdk::mysql::check_log_bin_compatibility(*m_instance, cfg, &res);
  EXPECT_EQ(0, res.size());

  cfg.set_for_handler("log_bin", std::optional<std::string>("Some text"),
                      mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.apply();
  res.clear();
  mysqlshdk::mysql::check_log_bin_compatibility(*m_instance, cfg, &res);
  EXPECT_EQ(0, res.size());

  // Delete the config file.
  shcore::delete_file(m_cfg_path, true);
}

TEST_F(Group_replication_test, check_server_id_compatibility) {
  using mysqlshdk::mysql::Config_type;
  using mysqlshdk::mysql::Config_types;
  using mysqlshdk::mysql::Invalid_config;
  using mysqlshdk::mysql::Var_qualifier;
  using mysqlshdk::utils::Version;

  // Get current server_id variable value to restore at the end.
  std::optional<int64_t> cur_server_id =
      m_instance->get_sysvar_int("server_id", Var_qualifier::GLOBAL);

  // Create config object (only with a server handler).
  mysqlshdk::config::Config cfg;
  cfg.add_handler(
      mysqlshdk::config::k_dft_cfg_server_handler,
      std::unique_ptr<mysqlshdk::config::IConfig_handler>(
          std::make_unique<mysqlshdk::config::Config_server_handler>(
              m_instance, Var_qualifier::GLOBAL)));
  std::vector<Invalid_config> res;
  // If server_version >= 8.0.3 and the server_id is the default compiled
  // value, there should be an issue reported.
  if (m_instance->get_version() >= mysqlshdk::utils::Version(8, 0, 3) &&
      m_instance->has_variable_compiled_value("server_id")) {
    mysqlshdk::mysql::check_server_id_compatibility(*m_instance, cfg, &res);
    ASSERT_EQ(1, res.size());
    EXPECT_STREQ(res.at(0).var_name.c_str(), "server_id");
    EXPECT_STREQ(res.at(0).required_val.c_str(), "<unique ID>");
    EXPECT_EQ(res.at(0).types, Config_type::SERVER);
    EXPECT_EQ(res.at(0).restart, true);
  }

  // No issues reported if server_id != 0 and has been changed.
  m_instance->set_sysvar("server_id", static_cast<int64_t>(1),
                         Var_qualifier::GLOBAL);
  res.clear();
  mysqlshdk::mysql::check_server_id_compatibility(*m_instance, cfg, &res);
  EXPECT_EQ(0, res.size());

  // Set server_id=0, and issues will be found
  m_instance->set_sysvar("server_id", static_cast<int64_t>(0),
                         Var_qualifier::GLOBAL);
  res.clear();
  mysqlshdk::mysql::check_server_id_compatibility(*m_instance, cfg, &res);
  ASSERT_EQ(1, res.size());
  EXPECT_STREQ(res.at(0).var_name.c_str(), "server_id");
  EXPECT_STREQ(res.at(0).current_val.c_str(), "0");
  EXPECT_STREQ(res.at(0).required_val.c_str(), "<unique ID>");
  EXPECT_EQ(res.at(0).types, Config_type::SERVER);
  EXPECT_EQ(res.at(0).restart, true);

  // Create an empty test option file and add the option file config handler.
  create_file(m_cfg_path, "");
  cfg.add_handler(mysqlshdk::config::k_dft_cfg_file_handler,
                  std::unique_ptr<mysqlshdk::config::IConfig_handler>(
                      new mysqlshdk::config::Config_file_handler(
                          "uuid1", m_cfg_path, m_cfg_path)));

  // Issues found on the option file (with no values set).
  // The current value should be the one from the server.
  res.clear();
  mysqlshdk::mysql::check_server_id_compatibility(*m_instance, cfg, &res);
  ASSERT_EQ(1, res.size());
  EXPECT_STREQ(res.at(0).var_name.c_str(), "server_id");
  EXPECT_STREQ(res.at(0).current_val.c_str(), "0");
  EXPECT_STREQ(res.at(0).required_val.c_str(), "<unique ID>");
  EXPECT_TRUE(res.at(0).types.is_set(Config_type::CONFIG));
  EXPECT_TRUE(res.at(0).types.is_set(Config_type::SERVER));
  EXPECT_EQ(res.at(0).restart, true);

  // Fixing the value on the server will still leave the warning for the empty
  // config file
  m_instance->set_sysvar("server_id", static_cast<int64_t>(1),
                         Var_qualifier::GLOBAL);
  res.clear();
  mysqlshdk::mysql::check_server_id_compatibility(*m_instance, cfg, &res);
  ASSERT_EQ(1, res.size());
  EXPECT_STREQ(res.at(0).var_name.c_str(), "server_id");
  EXPECT_STREQ(res.at(0).current_val.c_str(),
               mysqlshdk::mysql::k_value_not_set);
  EXPECT_STREQ(res.at(0).required_val.c_str(), "<unique ID>");
  EXPECT_EQ(res.at(0).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(0).restart, false);

  // Fixing the value on the config will clear all warnings.
  cfg.set_for_handler("server_id", std::optional<std::string>("1"),
                      mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.apply();
  res.clear();
  mysqlshdk::mysql::check_server_id_compatibility(*m_instance, cfg, &res);
  EXPECT_EQ(0, res.size());

  // Delete the config file.
  shcore::delete_file(m_cfg_path, true);

  // Restore initial values.
  m_instance->set_sysvar("server_id", *cur_server_id, Var_qualifier::GLOBAL);
}

TEST_F(Group_replication_test, check_replication_option_keyword) {
  using namespace mysqlshdk::mysql;

  {
    mysqlshdk::utils::Version version(8, 0, 25);

    EXPECT_EQ(get_replication_option_keyword(version, "replica"), "slave");

    EXPECT_EQ(get_replication_option_keyword(version, "replica foo"),
              "slave foo");
    EXPECT_EQ(get_replication_option_keyword(version, "replica_foo"),
              "slave_foo");
    EXPECT_EQ(get_replication_option_keyword(version, "replica-foo"),
              "slave-foo");

    EXPECT_EQ(get_replication_option_keyword(version, "foo source"),
              "foo master");
    EXPECT_EQ(get_replication_option_keyword(version, "foo_source"),
              "foo_master");
    EXPECT_EQ(get_replication_option_keyword(version, "foo-source"),
              "foo-master");

    EXPECT_EQ(get_replication_option_keyword(version, "foo source foo"),
              "foo master foo");
    EXPECT_EQ(get_replication_option_keyword(version, "foo_source_foo"),
              "foo_master_foo");
    EXPECT_EQ(get_replication_option_keyword(version, "foo-source-foo"),
              "foo-master-foo");

    EXPECT_EQ(get_replication_option_keyword(version, "replication"),
              "replication");
    EXPECT_EQ(get_replication_option_keyword(version, " replication "),
              " replication ");
    EXPECT_EQ(get_replication_option_keyword(version, "_replication_"),
              "_replication_");
    EXPECT_EQ(get_replication_option_keyword(version, "_replicafoo"),
              "_replicafoo");

    EXPECT_EQ(get_replication_option_keyword(version, "sources"), "sources");
    EXPECT_EQ(get_replication_option_keyword(version, " sources "),
              " sources ");
    EXPECT_EQ(get_replication_option_keyword(version, "_sources_"),
              "_sources_");
    EXPECT_EQ(get_replication_option_keyword(version, "_sourcefoo"),
              "_sourcefoo");
  }

  {
    mysqlshdk::utils::Version version(8, 0, 26);

    EXPECT_EQ(get_replication_option_keyword(version, "slave"), "replica");

    EXPECT_EQ(get_replication_option_keyword(version, "slave foo"),
              "replica foo");
    EXPECT_EQ(get_replication_option_keyword(version, "slave_foo"),
              "replica_foo");
    EXPECT_EQ(get_replication_option_keyword(version, "slave-foo"),
              "replica-foo");

    EXPECT_EQ(get_replication_option_keyword(version, "foo master"),
              "foo source");
    EXPECT_EQ(get_replication_option_keyword(version, "foo_master"),
              "foo_source");
    EXPECT_EQ(get_replication_option_keyword(version, "foo-master"),
              "foo-source");

    EXPECT_EQ(get_replication_option_keyword(version, "foo master foo"),
              "foo source foo");
    EXPECT_EQ(get_replication_option_keyword(version, "foo_master_foo"),
              "foo_source_foo");
    EXPECT_EQ(get_replication_option_keyword(version, "foo-master-foo"),
              "foo-source-foo");

    EXPECT_EQ(get_replication_option_keyword(version, "slaver"), "slaver");
    EXPECT_EQ(get_replication_option_keyword(version, " slaver "), " slaver ");
    EXPECT_EQ(get_replication_option_keyword(version, "_slaver_"), "_slaver_");
    EXPECT_EQ(get_replication_option_keyword(version, "_slavefoo"),
              "_slavefoo");

    EXPECT_EQ(get_replication_option_keyword(version, "masters"), "masters");
    EXPECT_EQ(get_replication_option_keyword(version, " masters "),
              " masters ");
    EXPECT_EQ(get_replication_option_keyword(version, "_masters_"),
              "_masters_");
    EXPECT_EQ(get_replication_option_keyword(version, "_masterfoo"),
              "_masterfoo");
  }
}

TEST_F(Group_replication_test, check_server_variables_compatibility) {
  using mysqlshdk::mysql::Config_type;
  using mysqlshdk::mysql::Config_types;
  using mysqlshdk::mysql::Invalid_config;
  using mysqlshdk::mysql::Var_qualifier;
  using mysqlshdk::utils::Version;

  std::string instance_port =
      std::to_string(m_instance->get_connection_options().get_port());
  // we will just modify and test the dynamic server variables as well as
  // one of the non dynamic variables from the server. Testing all the non
  // dynamic variables would be impossible as we do not control the state of
  // the instance that is provided and we don't want to restart it.

  // Get the values of the dynamic variables.
  std::optional<std::string> cur_binlog_format =
      m_instance->get_sysvar_string("binlog_format", Var_qualifier::GLOBAL);
  std::optional<std::string> cur_binlog_checksum =
      m_instance->get_sysvar_string("binlog_checksum", Var_qualifier::GLOBAL);
  // now get the value of one of the other variables
  // Note, picked this one because this is the first returned variable on the
  // vector of invalid configs after the two dynamic variables, so if both
  // have correct values, this will be the invalid config at index 0.
  std::optional<std::string> cur_log_slave_updates =
      m_instance->get_sysvar_string("log_slave_updates", Var_qualifier::GLOBAL);
  bool log_slave_updates_correct =
      (*cur_log_slave_updates == "1") || (*cur_log_slave_updates == "ON");
  bool binlog_checksum_allowed =
      m_instance->get_version() >= mysqlshdk::utils::Version(8, 0, 21);

  // Create config object (only with a server handler).
  mysqlshdk::config::Config cfg;
  cfg.add_handler(
      mysqlshdk::config::k_dft_cfg_server_handler,
      std::unique_ptr<mysqlshdk::config::IConfig_handler>(
          std::make_unique<mysqlshdk::config::Config_server_handler>(
              m_instance, Var_qualifier::GLOBAL)));

  // change the dynamic variables so there are no server issues
  m_instance->set_sysvar("binlog_format", static_cast<std::string>("ROW"),
                         Var_qualifier::GLOBAL);
  if (!binlog_checksum_allowed) {
    // binlog_checksum is only required to be NONE on server versions
    // below 8.0.21
    m_instance->set_sysvar("binlog_checksum", static_cast<std::string>("NONE"),
                           Var_qualifier::GLOBAL);
  } else {
    m_instance->set_sysvar("binlog_checksum", static_cast<std::string>("CRC32"),
                           Var_qualifier::GLOBAL);
  }
  std::vector<Invalid_config> res;
  mysqlshdk::mysql::check_server_variables_compatibility(*m_instance, cfg, true,
                                                         &res);
  if (!log_slave_updates_correct) {
    // if the log_slave_updates is not correct, the issues list must at least
    // have the log_slave_updates invalid config
    ASSERT_GE(res.size(), 1);

    auto it =
        std::find_if(res.begin(), res.end(), [](const Invalid_config &ic) {
          return ic.var_name == "log_slave_updates";
        });

    ASSERT_TRUE(it != res.end());

    EXPECT_EQ(it->current_val, *cur_log_slave_updates);
    EXPECT_STREQ(it->required_val.c_str(), "ON");
    EXPECT_EQ(it->types, Config_type::SERVER);
    EXPECT_EQ(it->restart, true);
  }
  // Create an empty test option file and add the option file config handler.
  create_file(m_cfg_path, "");
  mysqlshdk::config::Config cfg_file_only;
  cfg_file_only.add_handler(mysqlshdk::config::k_dft_cfg_file_handler,
                            std::unique_ptr<mysqlshdk::config::IConfig_handler>(
                                new mysqlshdk::config::Config_file_handler(
                                    "uuid1", m_cfg_path, m_cfg_path)));

  // Issues found on the option file only (with no values set).
  res.clear();

  auto find = [&res](const std::string &opt) {
    size_t i = 0;
    for (const auto &r : res) {
      if (r.var_name == opt) return i;
      ++i;
    }
    ADD_FAILURE() << "Couldn't find " << opt;
    return i;
  };

  mysqlshdk::mysql::check_server_variables_compatibility(
      *m_instance, cfg_file_only, true, &res);
  // if the config file is empty, there should be an issue for each of the
  // tested variables
  size_t i = 0;
  bool parallel_appliers_required =
      m_instance->get_version() >= mysqlshdk::utils::Version(8, 0, 23);

  if (!binlog_checksum_allowed) {
    if (parallel_appliers_required) {
      ASSERT_EQ(11, res.size());
    } else {
      ASSERT_EQ(8, res.size());
    }
    i = find("binlog_checksum");
    EXPECT_STREQ(res.at(i).var_name.c_str(), "binlog_checksum");
    EXPECT_STREQ(res.at(i).current_val.c_str(),
                 mysqlshdk::mysql::k_value_not_set);
    EXPECT_STREQ(res.at(i).required_val.c_str(), "NONE");
    EXPECT_EQ(res.at(i).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(i).restart, false);
  } else {
    if (!parallel_appliers_required ||
        (m_instance->get_version() >= mysqlshdk::utils::Version(8, 4, 0))) {
      ASSERT_EQ(4, res.size());
    } else {
      ASSERT_EQ(7, res.size());
    }
  }
  i = find("binlog_format");
  EXPECT_STREQ(res.at(i).var_name.c_str(), "binlog_format");
  EXPECT_STREQ(res.at(i).current_val.c_str(),
               mysqlshdk::mysql::k_value_not_set);
  EXPECT_STREQ(res.at(i).required_val.c_str(), "ROW");
  EXPECT_EQ(res.at(i).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(i).restart, false);

  if (m_instance->get_version() < mysqlshdk::utils::Version(8, 0, 3)) {
    i = find("log_slave_updates");
    EXPECT_STREQ(res.at(i).var_name.c_str(), "log_slave_updates");
    EXPECT_STREQ(res.at(i).current_val.c_str(),
                 mysqlshdk::mysql::k_value_not_set);
    EXPECT_STREQ(res.at(i).required_val.c_str(), "ON");
    EXPECT_EQ(res.at(i).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(i).restart, false);
  }

  i = find("enforce_gtid_consistency");
  EXPECT_STREQ(res.at(i).var_name.c_str(), "enforce_gtid_consistency");
  EXPECT_STREQ(res.at(i).current_val.c_str(),
               mysqlshdk::mysql::k_value_not_set);
  EXPECT_STREQ(res.at(i).required_val.c_str(), "ON");
  EXPECT_EQ(res.at(i).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(i).restart, false);

  i = find("gtid_mode");
  EXPECT_STREQ(res.at(i).var_name.c_str(), "gtid_mode");
  EXPECT_STREQ(res.at(i).current_val.c_str(),
               mysqlshdk::mysql::k_value_not_set);
  EXPECT_STREQ(res.at(i).required_val.c_str(), "ON");
  EXPECT_EQ(res.at(i).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(i).restart, false);

  if (m_instance->get_version() < mysqlshdk::utils::Version(8, 0, 23)) {
    i = find("master_info_repository");
    EXPECT_STREQ(res.at(i).var_name.c_str(), "master_info_repository");
    EXPECT_STREQ(res.at(i).current_val.c_str(),
                 mysqlshdk::mysql::k_value_not_set);
    EXPECT_STREQ(res.at(i).required_val.c_str(), "TABLE");
    EXPECT_EQ(res.at(i).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(i).restart, false);

    i = find("relay_log_info_repository");
    EXPECT_STREQ(res.at(i).var_name.c_str(), "relay_log_info_repository");
    EXPECT_STREQ(res.at(i).current_val.c_str(),
                 mysqlshdk::mysql::k_value_not_set);
    EXPECT_STREQ(res.at(i).required_val.c_str(), "TABLE");
    EXPECT_EQ(res.at(i).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(i).restart, false);
  }

  if (m_instance->get_version() < mysqlshdk::utils::Version(8, 3, 0)) {
    i = find("transaction_write_set_extraction");
    EXPECT_STREQ(res.at(i).var_name.c_str(),
                 "transaction_write_set_extraction");
    EXPECT_STREQ(res.at(i).current_val.c_str(),
                 mysqlshdk::mysql::k_value_not_set);
    EXPECT_STREQ(res.at(i).required_val.c_str(), "XXHASH64");
    EXPECT_EQ(res.at(i).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(i).restart, false);
  }

  if (parallel_appliers_required) {
    if (m_instance->get_version() < mysqlshdk::utils::Version(8, 4, 0)) {
      i = find("binlog_transaction_dependency_tracking");
      EXPECT_STREQ(res.at(i).var_name.c_str(),
                   "binlog_transaction_dependency_tracking");
      EXPECT_STREQ(res.at(i).current_val.c_str(),
                   mysqlshdk::mysql::k_value_not_set);
      EXPECT_STREQ(res.at(i).required_val.c_str(), "WRITESET");
      EXPECT_EQ(res.at(i).types, Config_type::CONFIG);
      EXPECT_EQ(res.at(i).restart, false);
    }

    if (m_instance->get_version() < mysqlshdk::utils::Version(8, 3, 0)) {
      i = find(mysqlshdk::mysql::get_replication_option_keyword(
          m_instance->get_version(), "slave_parallel_type"));
      EXPECT_STREQ(res.at(i).var_name.c_str(),
                   mysqlshdk::mysql::get_replication_option_keyword(
                       m_instance->get_version(), "slave_parallel_type")
                       .c_str());
      EXPECT_STREQ(res.at(i).current_val.c_str(),
                   mysqlshdk::mysql::k_value_not_set);
      EXPECT_STREQ(res.at(i).required_val.c_str(), "LOGICAL_CLOCK");
      EXPECT_EQ(res.at(i).types, Config_type::CONFIG);
      EXPECT_EQ(res.at(i).restart, false);
    }

    i = find(mysqlshdk::mysql::get_replication_option_keyword(
        m_instance->get_version(), "slave_preserve_commit_order"));
    EXPECT_STREQ(res.at(i).var_name.c_str(),
                 mysqlshdk::mysql::get_replication_option_keyword(
                     m_instance->get_version(), "slave_preserve_commit_order")
                     .c_str());
    EXPECT_STREQ(res.at(i).current_val.c_str(),
                 mysqlshdk::mysql::k_value_not_set);
    EXPECT_STREQ(res.at(i).required_val.c_str(), "ON");
    EXPECT_EQ(res.at(i).types, Config_type::CONFIG);
    EXPECT_EQ(res.at(i).restart, false);
  }

  // add the empty file as well to the cfg handler and check that incorrect
  // server results override the incorrect file results for the current value
  // field of the invalid config.
  cfg.add_handler(mysqlshdk::config::k_dft_cfg_file_handler,
                  std::unique_ptr<mysqlshdk::config::IConfig_handler>(
                      new mysqlshdk::config::Config_file_handler(
                          "uuid1", m_cfg_path, m_cfg_path)));

  // change the dynamic variables so there are server issues
  m_instance->set_sysvar("binlog_format", static_cast<std::string>("STATEMENT"),
                         Var_qualifier::GLOBAL);
  m_instance->set_sysvar("binlog_checksum", static_cast<std::string>("CRC32"),
                         Var_qualifier::GLOBAL);
  // Issues found on the option file only (with no values set).
  res.clear();
  mysqlshdk::mysql::check_server_variables_compatibility(*m_instance, cfg, true,
                                                         &res);
  // since all the file configurations are wrong, we know that even if some
  // variables have correct results on the server, they will still have a
  // invalid config. Since the cfg has a server handler, then it should also
  // have one more entry for the report_port option.
  if (!binlog_checksum_allowed) {
    if (parallel_appliers_required) {
      ASSERT_EQ(12, res.size());
    } else {
      ASSERT_EQ(9, res.size());
    }
    i = find("binlog_checksum");
    EXPECT_STREQ(res.at(i).var_name.c_str(), "binlog_checksum");
    EXPECT_STREQ(res.at(i).current_val.c_str(), "CRC32");
    EXPECT_STREQ(res.at(i).required_val.c_str(), "NONE");
    EXPECT_TRUE(res.at(i).types.is_set(Config_type::CONFIG));
    EXPECT_TRUE(res.at(i).types.is_set(Config_type::SERVER));
    EXPECT_EQ(res.at(i).restart, false);
  } else {
    if (parallel_appliers_required &&
        (m_instance->get_version() < mysqlshdk::utils::Version(8, 4, 0))) {
      ASSERT_EQ(8, res.size());
    } else {
      ASSERT_EQ(5, res.size());
    }
  }

  i = find("binlog_format");
  EXPECT_STREQ(res.at(i).var_name.c_str(), "binlog_format");
  EXPECT_STREQ(res.at(i).current_val.c_str(), "STATEMENT");
  EXPECT_STREQ(res.at(i).required_val.c_str(), "ROW");
  EXPECT_TRUE(res.at(i).types.is_set(Config_type::CONFIG));
  EXPECT_TRUE(res.at(i).types.is_set(Config_type::SERVER));
  EXPECT_EQ(res.at(i).restart, false);

  if (m_instance->get_version() < mysqlshdk::utils::Version(8, 0, 3)) {
    i = find("log_slave_updates");
    if (log_slave_updates_correct) {
      EXPECT_STREQ(res.at(i).var_name.c_str(), "log_slave_updates");
      EXPECT_STREQ(res.at(i).current_val.c_str(),
                   mysqlshdk::mysql::k_value_not_set);
      EXPECT_STREQ(res.at(i).required_val.c_str(), "ON");
      EXPECT_EQ(res.at(i).types, Config_type::CONFIG);
      EXPECT_EQ(res.at(i).restart, false);
    } else {
      EXPECT_STREQ(res.at(i).var_name.c_str(), "log_slave_updates");
      EXPECT_EQ(res.at(i).current_val, *cur_log_slave_updates);
      EXPECT_STREQ(res.at(i).required_val.c_str(), "ON");
      EXPECT_TRUE(res.at(i).types.is_set(Config_type::CONFIG));
      EXPECT_TRUE(res.at(i).types.is_set(Config_type::SERVER));
      EXPECT_EQ(res.at(i).restart, true);
    }
  }

  i = find("report_port");
  EXPECT_STREQ(res.at(i).var_name.c_str(), "report_port");
  EXPECT_STREQ(res.at(i).current_val.c_str(),
               mysqlshdk::mysql::k_value_not_set);
  EXPECT_EQ(res.at(i).required_val, instance_port);
  EXPECT_EQ(res.at(i).types, Config_type::CONFIG);
  EXPECT_EQ(res.at(i).restart, false);

  // Fixing all the config file incorrect values on both config objects
  cfg_file_only.set_for_handler("binlog_format",
                                std::optional<std::string>("ROW"),
                                mysqlshdk::config::k_dft_cfg_file_handler);
  if (!binlog_checksum_allowed) {
    cfg_file_only.set_for_handler("binlog_checksum",
                                  std::optional<std::string>("NONE"),
                                  mysqlshdk::config::k_dft_cfg_file_handler);
  }
  if (m_instance->get_version() < mysqlshdk::utils::Version(8, 0, 3)) {
    cfg_file_only.set_for_handler("log_slave_updates",
                                  std::optional<std::string>("ON"),
                                  mysqlshdk::config::k_dft_cfg_file_handler);
  }
  cfg_file_only.set_for_handler("enforce_gtid_consistency",
                                std::optional<std::string>("ON"),
                                mysqlshdk::config::k_dft_cfg_file_handler);
  cfg_file_only.set_for_handler("gtid_mode", std::optional<std::string>("ON"),
                                mysqlshdk::config::k_dft_cfg_file_handler);
  if (m_instance->get_version() < mysqlshdk::utils::Version(8, 0, 23)) {
    cfg_file_only.set_for_handler("master_info_repository",
                                  std::optional<std::string>("TABLE"),
                                  mysqlshdk::config::k_dft_cfg_file_handler);
    cfg_file_only.set_for_handler("relay_log_info_repository",
                                  std::optional<std::string>("TABLE"),
                                  mysqlshdk::config::k_dft_cfg_file_handler);
  }
  cfg_file_only.set_for_handler(
      "transaction_write_set_extraction",
      std::optional<std::string>("MURMUR32"),  // different but valid
      mysqlshdk::config::k_dft_cfg_file_handler);
  cfg_file_only.set_for_handler("report_port",
                                std::optional<std::string>(instance_port),
                                mysqlshdk::config::k_dft_cfg_file_handler);

  if (parallel_appliers_required) {
    cfg_file_only.set_for_handler("binlog_transaction_dependency_tracking",
                                  std::optional<std::string>("WRITESET"),
                                  mysqlshdk::config::k_dft_cfg_file_handler);

    if (m_instance->get_version() < mysqlshdk::utils::Version(8, 3, 0)) {
      cfg_file_only.set_for_handler(
          mysqlshdk::mysql::get_replication_option_keyword(
              m_instance->get_version(), "slave_parallel_type"),
          std::optional<std::string>("LOGICAL_CLOCK"),
          mysqlshdk::config::k_dft_cfg_file_handler);
    }

    cfg_file_only.set_for_handler(
        mysqlshdk::mysql::get_replication_option_keyword(
            m_instance->get_version(), "slave_preserve_commit_order"),
        std::optional<std::string>("ON"),
        mysqlshdk::config::k_dft_cfg_file_handler);
  }

  cfg_file_only.apply();

  cfg.set_for_handler("binlog_format", std::optional<std::string>("ROW"),
                      mysqlshdk::config::k_dft_cfg_file_handler);
  if (!binlog_checksum_allowed) {
    cfg.set_for_handler("binlog_checksum", std::optional<std::string>("NONE"),
                        mysqlshdk::config::k_dft_cfg_file_handler);
  }
  if (m_instance->get_version() < mysqlshdk::utils::Version(8, 0, 3)) {
    cfg.set_for_handler("log_slave_updates", std::optional<std::string>("ON"),
                        mysqlshdk::config::k_dft_cfg_file_handler);
  }
  cfg.set_for_handler("enforce_gtid_consistency",
                      std::optional<std::string>("ON"),
                      mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.set_for_handler("gtid_mode", std::optional<std::string>("ON"),
                      mysqlshdk::config::k_dft_cfg_file_handler);

  if (m_instance->get_version() < mysqlshdk::utils::Version(8, 0, 23)) {
    cfg.set_for_handler("master_info_repository",
                        std::optional<std::string>("TABLE"),
                        mysqlshdk::config::k_dft_cfg_file_handler);
    cfg.set_for_handler("relay_log_info_repository",
                        std::optional<std::string>("TABLE"),
                        mysqlshdk::config::k_dft_cfg_file_handler);
  }
  cfg.set_for_handler(
      "transaction_write_set_extraction",
      std::optional<std::string>("MURMUR32"),  // different but valid
      mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.set_for_handler("report_port", std::optional<std::string>(instance_port),
                      mysqlshdk::config::k_dft_cfg_file_handler);
  cfg.apply();

  // Since all incorrect values have been fixed on the configuration file
  // and the cfg_file_only config object only had a config_file handler then it
  // should have no issues
  res.clear();
  mysqlshdk::mysql::check_server_variables_compatibility(
      *m_instance, cfg_file_only, true, &res);
  EXPECT_EQ(0, res.size());

  // Since all incorrect values have been fixed on the configuration file
  // but the cfg config object also had a server handler, then it should at
  // least have the issues that are still present on the server.

  res.clear();
  mysqlshdk::mysql::check_server_variables_compatibility(*m_instance, cfg, true,
                                                         &res);
  if (!log_slave_updates_correct) {
    ASSERT_GE(res.size(), 3);
    i = find("log_slave_updates");
    EXPECT_STREQ(res.at(i).var_name.c_str(), "log_slave_updates");
    EXPECT_EQ(res.at(i).current_val, *cur_log_slave_updates);
    EXPECT_STREQ(res.at(i).required_val.c_str(), "ON");
    EXPECT_EQ(res.at(i).types, Config_type::SERVER);
    EXPECT_EQ(res.at(i).restart, true);
  }
  i = find("binlog_format");
  EXPECT_STREQ(res.at(i).var_name.c_str(), "binlog_format");
  EXPECT_STREQ(res.at(i).current_val.c_str(), "STATEMENT");
  EXPECT_STREQ(res.at(i).required_val.c_str(), "ROW");
  EXPECT_EQ(res.at(i).types, Config_type::SERVER);
  EXPECT_EQ(res.at(i).restart, false);
  if (!binlog_checksum_allowed) {
    i = find("binlog_checksum");
    EXPECT_STREQ(res.at(i).var_name.c_str(), "binlog_checksum");
    EXPECT_STREQ(res.at(i).current_val.c_str(), "CRC32");
    EXPECT_STREQ(res.at(i).required_val.c_str(), "NONE");
    EXPECT_EQ(res.at(i).types, Config_type::SERVER);
    EXPECT_EQ(res.at(i).restart, false);
  }

  // Delete the config file.
  shcore::delete_file(m_cfg_path, true);

  // Restore initial values.
  m_instance->set_sysvar("binlog_format", *cur_binlog_format,
                         Var_qualifier::GLOBAL);
  m_instance->set_sysvar("binlog_checksum", *cur_binlog_checksum,
                         Var_qualifier::GLOBAL);
}

TEST_F(Group_replication_test, is_active_member) {
  using mysqlshdk::db::Type;

  std::shared_ptr<Mock_session> mock_session = std::make_shared<Mock_session>();
  mysqlshdk::mysql::Instance instance{mock_session};

  mock_session
      ->expect_query(
          "SELECT count(*) "
          "FROM performance_schema.replication_group_members "
          "WHERE MEMBER_HOST = 'localhost' AND MEMBER_PORT = 3306 "
          "AND MEMBER_STATE NOT IN ('OFFLINE', 'UNREACHABLE')")
      .then_return({{"", {"count(*)"}, {Type::String}, {{"0"}}}});

  mock_session->expect_query(
      {"SELECT COALESCE(@@report_host, @@hostname),  "
       "COALESCE(@@report_port, @@port)",
       {"Host", "Port"},
       {mysqlshdk::db::Type::String, mysqlshdk::db::Type::Integer},
       {{"mock@localhost", "3306"}}});

  EXPECT_FALSE(mysqlshdk::gr::is_active_member(instance, "localhost", 3306));

  mock_session
      ->expect_query(
          "SELECT count(*) "
          "FROM performance_schema.replication_group_members "
          "WHERE MEMBER_HOST = 'localhost' AND MEMBER_PORT = 3306 "
          "AND MEMBER_STATE NOT IN ('OFFLINE', 'UNREACHABLE')")
      .then_return({{"", {"count(*)"}, {Type::String}, {{"1"}}}});

  mock_session->expect_query(
      {"SELECT COALESCE(@@report_host, @@hostname),  "
       "COALESCE(@@report_port, @@port)",
       {"Host", "Port"},
       {mysqlshdk::db::Type::String, mysqlshdk::db::Type::Integer},
       {{"mock@localhost", "3306"}}});

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
            std::make_unique<mysqlshdk::config::Config_server_handler>(
                &instance, Var_qualifier::GLOBAL)));
  }

  // Set auto-increment for single-primary (3 instances).
  for (int i = 0; i < 3; i++) {
    mock_session
        ->expect_query(
            "show GLOBAL variables where `variable_name` in "
            "('auto_increment_increment')")
        .then({""});
    mock_session->expect_query("SET GLOBAL `auto_increment_increment` = 1")
        .then({""});
    mock_session
        ->expect_query(
            "show GLOBAL variables where `variable_name` in "
            "('auto_increment_offset')")
        .then({""});
    mock_session->expect_query("SET GLOBAL `auto_increment_offset` = 2")
        .then({""});
  }

  mysqlshdk::gr::update_auto_increment(&cfg_global,
                                       Topology_mode::SINGLE_PRIMARY);
  cfg_global.apply();

  // Create config objects with 10 server handlers with PERSIST support.
  mysqlshdk::config::Config cfg_persist;
  for (int i = 0; i < 10; i++) {
    cfg_persist.add_handler(
        "instance_" + std::to_string(i),
        std::unique_ptr<mysqlshdk::config::IConfig_handler>(
            std::make_unique<mysqlshdk::config::Config_server_handler>(
                &instance, Var_qualifier::PERSIST)));
  }

  // Set auto-increment for single-primary (10 instances with PERSIST support).
  for (int i = 0; i < 10; i++) {
    mock_session
        ->expect_query(
            "show GLOBAL variables where `variable_name` in "
            "('auto_increment_increment')")
        .then({""});
    mock_session->expect_query("SET PERSIST `auto_increment_increment` = 1")
        .then({""});
    mock_session
        ->expect_query(
            "show GLOBAL variables where `variable_name` in "
            "('auto_increment_offset')")
        .then({""});
    mock_session->expect_query("SET PERSIST `auto_increment_offset` = 2")
        .then({""});
  }

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
  mock_session
      ->expect_query(
          "show GLOBAL variables where `variable_name` in ('server_id')")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"server_id", "4"}}}});

  mock_session
      ->expect_query(
          "show GLOBAL variables where `variable_name` in "
          "('auto_increment_increment')")
      .then({""});
  mock_session->expect_query("SET GLOBAL `auto_increment_increment` = 7")
      .then({""});
  mock_session
      ->expect_query(
          "show GLOBAL variables where `variable_name` in "
          "('auto_increment_offset')")
      .then({""});
  mock_session->expect_query("SET GLOBAL `auto_increment_offset` = 5")
      .then({""});
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
  mock_session
      ->expect_query(
          "show GLOBAL variables where `variable_name` in ('server_id')")
      .then_return({{"",
                     {"Variable_name", "Value"},
                     {Type::String, Type::String},
                     {{"server_id", "7"}}}});

  mock_session
      ->expect_query(
          "show GLOBAL variables where `variable_name` in "
          "('auto_increment_increment')")
      .then({""});
  mock_session->expect_query("SET PERSIST `auto_increment_increment` = 7")
      .then({""});
  mock_session
      ->expect_query(
          "show GLOBAL variables where `variable_name` in "
          "('auto_increment_offset')")
      .then({""});
  mock_session->expect_query("SET PERSIST `auto_increment_offset` = 1")
      .then({""});
  mysqlshdk::gr::update_auto_increment(&cfg_persist,
                                       Topology_mode::MULTI_PRIMARY);
  cfg_persist.apply();
}

TEST_F(Group_replication_test, get_group_protocol_version) {
  using mysqlshdk::db::Type;

  std::shared_ptr<Mock_session> mock_session = std::make_shared<Mock_session>();
  mysqlshdk::mysql::Instance instance{mock_session};

  EXPECT_CALL(*mock_session, get_server_version())
      .WillOnce(Return(mysqlshdk::utils::Version("8.0.16")));

  mock_session
      ->expect_query("SELECT group_replication_get_communication_protocol()")
      .then_return({{"",
                     {"group_replication_get_communication_protocol()"},
                     {Type::String},
                     {{"8.0.16"}}}});
  EXPECT_EQ(mysqlshdk::gr::get_group_protocol_version(instance),
            mysqlshdk::utils::Version("8.0.16"));
}

TEST_F(Group_replication_test, is_protocol_upgrade_possible) {
  using mysqlshdk::db::Type;

  mysqlshdk::utils::Version out_protocol_version;
  std::string server_uuid = "2aebeab3-39d1-11e9-b4e9-9ed7ce0b544f";

  std::shared_ptr<Mock_session> mock_session = std::make_shared<Mock_session>();
  mysqlshdk::mysql::Instance instance{mock_session};

  // Protocol version 8.0.16, one of the members is running 8.0.15 so the
  // upgrade is not possible
  mock_session
      ->expect_query(
          "SELECT m.member_id, m.member_state, m.member_host, m.member_port, "
          "m.member_role, m.member_version, s.view_id FROM "
          "performance_schema.replication_group_members m LEFT JOIN "
          "performance_schema.replication_group_member_stats s   ON "
          "m.member_id = s.member_id      AND s.channel_name = "
          "'group_replication_applier' WHERE m.member_id = @@server_uuid OR "
          "m.member_state <> 'ERROR' ORDER BY m.member_id")
      .then_return(
          {{"",
            {"member_id", "member_state", "member_host", "member_port",
             "member_role", "member_version", "view_id", "single_primary"},
            {Type::String, Type::String, Type::String, Type::String,
             Type::String, Type::String, Type::String, Type::UInteger},
            {{"2aebeab3-39d1-11e9-b4e9-9ed7ce0b544d", "ONLINE", "T480", "3310",
              "PRIMARY", "8.0.16", "11111-", "1"},
             {"2aebeab3-39d1-11e9-b4e9-9ed7ce0b544e", "ONLINE", "T480", "3320",
              "SECONDARY", "8.0.16", "11111-", "1"},
             {"2aebeab3-39d1-11e9-b4e9-9ed7ce0b544f", "ONLINE", "T480", "3330",
              "SECONDARY", "8.0.15", "11111-", "1"}}}});

  EXPECT_CALL(*mock_session, get_server_version())
      .WillOnce(Return(mysqlshdk::utils::Version("8.0.16")));

  mock_session
      ->expect_query("SELECT group_replication_get_communication_protocol()")
      .then_return({{"",
                     {"group_replication_get_communication_protocol()"},
                     {Type::String},
                     {{"8.0.16"}}}});

  EXPECT_FALSE(mysqlshdk::gr::is_protocol_upgrade_possible(
      instance, "", &out_protocol_version));

  // Protocol version 8.0.16, removing the instance with 8.0.15 from the group
  mock_session
      ->expect_query(
          "SELECT m.member_id, m.member_state, m.member_host, m.member_port, "
          "m.member_role, m.member_version, s.view_id FROM "
          "performance_schema.replication_group_members m LEFT JOIN "
          "performance_schema.replication_group_member_stats s   ON "
          "m.member_id = s.member_id      AND s.channel_name = "
          "'group_replication_applier' WHERE m.member_id = @@server_uuid OR "
          "m.member_state <> 'ERROR' ORDER BY m.member_id")
      .then_return(
          {{"",
            {"member_id", "member_state", "member_host", "member_port",
             "member_role", "member_version", "view_id", "single_primary"},
            {Type::String, Type::String, Type::String, Type::String,
             Type::String, Type::String, Type::String, Type::UInteger},
            {{"2aebeab3-39d1-11e9-b4e9-9ed7ce0b544d", "ONLINE", "T480", "3310",
              "PRIMARY", "8.0.16", "11111-", "1"},
             {"2aebeab3-39d1-11e9-b4e9-9ed7ce0b544e", "ONLINE", "T480", "3320",
              "SECONDARY", "8.0.16", "11111-", "1"},
             {"2aebeab3-39d1-11e9-b4e9-9ed7ce0b544f", "ONLINE", "T480", "3330",
              "SECONDARY", "8.0.15", "11111-", "1"}}}});

  mock_session
      ->expect_query("SELECT group_replication_get_communication_protocol()")
      .then_return({{"",
                     {"group_replication_get_communication_protocol()"},
                     {Type::String},
                     {{"8.0.15"}}}});

  mock_session
      ->expect_query(
          "show GLOBAL variables where `variable_name` in ('server_uuid')")
      .then_return(
          {{"",
            {"Variable_name", "Value"},
            {Type::String, Type::String},
            {{"server_uuid", "2aebeab3-39d1-11e9-b4e9-9ed7ce0b544f"}}}});

  EXPECT_TRUE(mysqlshdk::gr::is_protocol_upgrade_possible(
      instance, server_uuid, &out_protocol_version));

  EXPECT_EQ(out_protocol_version, mysqlshdk::utils::Version("8.0.16"));

  // Protocol version 8.0.27
  mock_session = std::make_shared<Mock_session>();
  instance = mysqlshdk::mysql::Instance(mock_session);

  mock_session
      ->expect_query(
          "SELECT m.member_id, m.member_state, m.member_host, m.member_port, "
          "m.member_role, m.member_version, s.view_id FROM "
          "performance_schema.replication_group_members m LEFT JOIN "
          "performance_schema.replication_group_member_stats s   ON "
          "m.member_id = s.member_id      AND s.channel_name = "
          "'group_replication_applier' WHERE m.member_id = @@server_uuid OR "
          "m.member_state <> 'ERROR' ORDER BY m.member_id")
      .then_return(
          {{"",
            {"member_id", "member_state", "member_host", "member_port",
             "member_role", "member_version", "view_id", "single_primary"},
            {Type::String, Type::String, Type::String, Type::String,
             Type::String, Type::String, Type::String, Type::UInteger},
            {{"2aebeab3-39d1-11e9-b4e9-9ed7ce0b544d", "ONLINE", "T480", "3310",
              "PRIMARY", "8.0.27", "11111-", "1"},
             {"2aebeab3-39d1-11e9-b4e9-9ed7ce0b544e", "ONLINE", "T480", "3320",
              "SECONDARY", "8.0.28", "11111-", "1"},
             {"2aebeab3-39d1-11e9-b4e9-9ed7ce0b544f", "ONLINE", "T480", "3330",
              "SECONDARY", "8.0.29", "11111-", "1"}}}});

  EXPECT_CALL(*mock_session, get_server_version())
      .WillOnce(Return(mysqlshdk::utils::Version("8.0.16")));

  mock_session
      ->expect_query("SELECT group_replication_get_communication_protocol()")
      .then_return({{"",
                     {"group_replication_get_communication_protocol()"},
                     {Type::String},
                     {{"8.0.16"}}}});

  mock_session
      ->expect_query(
          "show GLOBAL variables where `variable_name` in ('server_uuid')")
      .then_return(
          {{"",
            {"Variable_name", "Value"},
            {Type::String, Type::String},
            {{"server_uuid", "2aebeab3-39d1-11e9-b4e9-9ed7ce0b544f"}}}});

  out_protocol_version = mysqlshdk::utils::Version(0, 0, 0);

  EXPECT_TRUE(mysqlshdk::gr::is_protocol_upgrade_possible(
      instance, "", &out_protocol_version));

  EXPECT_EQ(out_protocol_version, mysqlshdk::utils::Version("8.0.27"));
}

TEST_F(Group_replication_test, is_protocol_upgrade_not_required) {
  using mysqlshdk::db::Type;

  mysqlshdk::utils::Version out_protocol_version;
  std::string server_uuid = "2aebeab3-39d1-11e9-b4e9-9ed7ce0b544f";

  std::shared_ptr<Mock_session> mock_session = std::make_shared<Mock_session>();
  mysqlshdk::mysql::Instance instance{mock_session};

  mock_session
      ->expect_query(
          "SELECT m.member_id, m.member_state, m.member_host, m.member_port, "
          "m.member_role, m.member_version, s.view_id FROM "
          "performance_schema.replication_group_members m LEFT JOIN "
          "performance_schema.replication_group_member_stats s   ON "
          "m.member_id = s.member_id      AND s.channel_name = "
          "'group_replication_applier' WHERE m.member_id = @@server_uuid OR "
          "m.member_state <> 'ERROR' ORDER BY m.member_id")
      .then_return(
          {{"",
            {"member_id", "member_state", "member_host", "member_port",
             "member_role", "member_version", "view_id", "single_primary"},
            {Type::String, Type::String, Type::String, Type::String,
             Type::String, Type::String, Type::String, Type::UInteger},
            {{"2aebeab3-39d1-11e9-b4e9-9ed7ce0b544d", "ONLINE", "T480", "3310",
              "PRIMARY", "8.0.15", "11111-", "1"},
             {"2aebeab3-39d1-11e9-b4e9-9ed7ce0b544e", "ONLINE", "T480", "3320",
              "SECONDARY", "8.0.15", "11111-", "1"},
             {"2aebeab3-39d1-11e9-b4e9-9ed7ce0b544f", "ONLINE", "T480", "3330",
              "SECONDARY", "8.0.16", "11111-", "1"}}}});

  EXPECT_CALL(*mock_session, get_server_version())
      .WillOnce(Return(mysqlshdk::utils::Version("8.0.16")));

  mock_session
      ->expect_query("SELECT group_replication_get_communication_protocol()")
      .then_return({{"",
                     {"group_replication_get_communication_protocol()"},
                     {Type::String},
                     {{"8.0.15"}}}});

  mock_session
      ->expect_query(
          "show GLOBAL variables where `variable_name` in ('server_uuid')")
      .then_return(
          {{"",
            {"Variable_name", "Value"},
            {Type::String, Type::String},
            {{"server_uuid", "2aebeab3-39d1-11e9-b4e9-9ed7ce0b544e"}}}});

  EXPECT_FALSE(mysqlshdk::gr::is_protocol_upgrade_possible(
      instance, server_uuid, &out_protocol_version));

  EXPECT_EQ(out_protocol_version, mysqlshdk::utils::Version());
}

TEST_F(Group_replication_test, check_instance_version_compatibility) {
  using mysqlshdk::db::Type;
  using mysqlshdk::utils::Version;

  auto test = [](std::optional<bool> gr_allow_lower_version_join,
                 const Version &instance_version,
                 const Version &lowest_cluster_version, bool is_compatible) {
    std::string compatible = (is_compatible) ? "COMPATIBLE" : "NOT COMPATIBLE";

    std::string str_gr_allow_lower_version_join;
    std::vector<std::vector<std::string>> rows_gr_allow_lower_version_join;

    if (!gr_allow_lower_version_join.has_value()) {
      str_gr_allow_lower_version_join = "NULL";
      rows_gr_allow_lower_version_join = {};
    } else {
      if (*gr_allow_lower_version_join) {
        str_gr_allow_lower_version_join = "ON";
        rows_gr_allow_lower_version_join = {
            {"group_replication_allow_local_lower_version_join", "ON"}};
      } else {
        str_gr_allow_lower_version_join = "ON";
        rows_gr_allow_lower_version_join = {
            {"group_replication_allow_local_lower_version_join", "OFF"}};
      }
    }

    SCOPED_TRACE("Test version " + compatible + ":  instance version '" +
                 instance_version.get_base() + "', lower cluster version '" +
                 lowest_cluster_version.get_base() +
                 "', gr_allow_lower_version_join '" +
                 str_gr_allow_lower_version_join + "'");

    auto mock_session = std::make_shared<Mock_session>();
    mysqlshdk::mysql::Instance instance{mock_session};

    if (lowest_cluster_version < mysqlshdk::utils::Version(8, 4, 0)) {
      mock_session
          ->expect_query(
              "show GLOBAL variables where `variable_name` in "
              "('group_replication_allow_local_lower_version_join')")
          .then_return({{"",
                         {"Variable_name", "Value"},
                         {Type::String, Type::String},
                         rows_gr_allow_lower_version_join}});
    }

    if (!gr_allow_lower_version_join.has_value() ||
        !*gr_allow_lower_version_join) {
      EXPECT_CALL(*mock_session, get_server_version())
          .WillOnce(Return(instance_version));
    }

    if (is_compatible) {
      EXPECT_NO_THROW(mysqlshdk::gr::check_instance_version_compatibility(
          instance, lowest_cluster_version));
    } else {
      std::string major, str_instance_version, str_cluster_version;
      if (instance_version <= mysqlshdk::utils::Version(8, 0, 16)) {
        major = "major ";
        str_instance_version = std::to_string(instance_version.get_major());
        str_cluster_version =
            std::to_string(lowest_cluster_version.get_major());
      } else {
        major = "";
        str_instance_version = instance_version.get_base();
        str_cluster_version = lowest_cluster_version.get_base();
      }

      EXPECT_THROW_LIKE(mysqlshdk::gr::check_instance_version_compatibility(
                            instance, lowest_cluster_version),
                        std::runtime_error,
                        "Instance " + major + "version '" +
                            str_instance_version +
                            "' cannot be lower than the "
                            "cluster lowest " +
                            major + "version '" + str_cluster_version + "'.");
    }
  };

#define ALLOWED(gr_allow_lower_version_join, instance_version, \
                lowest_cluster_version)                        \
  do {                                                         \
    SCOPED_TRACE("ALLOWED");                                   \
    test(gr_allow_lower_version_join, instance_version,        \
         lowest_cluster_version, true);                        \
  } while (0);

#define BLOCKED(gr_allow_lower_version_join, instance_version, \
                lowest_cluster_version)                        \
  do {                                                         \
    SCOPED_TRACE("BLOCKED");                                   \
    test(gr_allow_lower_version_join, instance_version,        \
         lowest_cluster_version, false);                       \
  } while (0);

  // major upgrades
  ALLOWED(false, Version(8, 1, 0), Version(8, 0, 40));
  ALLOWED(false, Version(8, 2, 0), Version(8, 1, 0));
  ALLOWED(false, Version(8, 4, 0), Version(8, 1, 0));

  // upgrades that are not not supported but we don't check for
  ALLOWED(false, Version(8, 3, 0), Version(8, 1, 0));
  ALLOWED(false, Version(12, 0, 0), Version(8, 1, 0));

  // basic downgrade
  ALLOWED(false, Version(8, 1, 0), Version(8, 1, 2));
  ALLOWED(false, Version(8, 4, 0), Version(8, 4, 1));

  BLOCKED(false, Version(8, 1, 0), Version(8, 2, 0));
  BLOCKED(false, Version(8, 3, 0), Version(8, 4, 0));

  // Test: group_replication_allow_local_lower_version_join = ON
  // - Version compatible, independently of the cluster lowest
  // version.
  ALLOWED(std::optional<bool>(true), Version(8, 0, 1), Version(8, 0, 20));
  ALLOWED(std::optional<bool>(true), Version(8, 0, 21), Version(8, 0, 20));

  // Test: group_replication_allow_local_lower_version_join = OFF
  //       instance_version <= 8.0.16
  // Version compatible, if MAJOR(version) >=
  // MAJOR(lowest_cluster_version)
  ALLOWED(false, Version(8, 0, 16), Version(8, 0, 16));
  ALLOWED(false, Version(8, 0, 15), Version(8, 0, 16));

  // Test: group_replication_allow_local_lower_version_join = OFF
  //       instance_version <= 8.0.16
  // Version incompatible, if MAJOR(version) <
  // MAJOR(lowest_cluster_version)
  BLOCKED(false, Version(5, 7, 26), Version(8, 0, 16));

  // Test: group_replication_allow_local_lower_version_join = OFF
  //       instance_version > 8.0.16
  // Version compatible, if version >= lowest_cluster_version
  ALLOWED(false, Version(8, 0, 17), Version(8, 0, 17));
  ALLOWED(false, Version(8, 0, 18), Version(8, 0, 17));

  // Test: group_replication_allow_local_lower_version_join = OFF
  //       instance_version > 8.0.16
  // Version incompatible, if version < lowest_cluster_version
  BLOCKED(false, Version(8, 0, 17), Version(8, 0, 18));

  // Test: group_replication_allow_local_lower_version_join = OFF
  //       instance_version >= 8.0.35
  // Version compatible, if version < lowest_cluster_version
  // && lowest_cluster_version.series() == version.series()
  ALLOWED(false, Version(8, 0, 35), Version(8, 0, 35));
  ALLOWED(false, Version(8, 4, 35), Version(8, 4, 35));

  // Test: group_replication_allow_local_lower_version_join = OFF
  //       instance_version non-LTS
  // Version compatible, if version < lowest_cluster_version
  // && lowest_cluster_version.series() == version.series()
  ALLOWED(false, Version(8, 2, 0), Version(8, 1, 0));
  ALLOWED(false, Version(8, 1, 0), Version(8, 0, 35));
  ALLOWED(false, Version(8, 1, 1), Version(8, 1, 0));
  ALLOWED(false, Version(8, 1, 0), Version(8, 1, 1));

  // Test: group_replication_allow_local_lower_version_join = OFF
  //       instance_version >= 8.0.35
  // Version compatible, if version < lowest_cluster_version &&
  // && lowest_cluster_version.series() == version.series()
  ALLOWED(false, Version(8, 0, 36), Version(8, 0, 35));
  ALLOWED(false, Version(8, 4, 36), Version(8, 4, 35));

  // Test: group_replication_allow_local_lower_version_join = OFF
  //       instance_version >= 8.0.35
  // Version compatible, if version < lowest_cluster_version
  // && lowest_cluster_version.series() == version.series()
  ALLOWED(false, Version(8, 0, 38), Version(8, 0, 37));
  ALLOWED(false, Version(8, 4, 38), Version(8, 4, 37));

  // Test: group_replication_allow_local_lower_version_join = OFF
  //       instance_version >= 8.0.35
  // Version compatible, if version < lowest_cluster_version
  // && lowest_cluster_version.series() == version.series()
  BLOCKED(false, Version(8, 0, 34), Version(8, 0, 35));
  BLOCKED(false, Version(8, 3, 1), Version(8, 4, 0));
  ALLOWED(false, Version(8, 0, 35), Version(8, 0, 36));

  // Test: group_replication_allow_local_lower_version_join = OFF
  //       instance_version non-LTS
  // Version compatible, if version < lowest_cluster_version
  // && lowest_cluster_version.series() == version.series()
  BLOCKED(false, Version(8, 1, 0), Version(8, 2, 18));
  BLOCKED(false, Version(8, 0, 35), Version(8, 1, 0));

  // Test: group_replication_allow_local_lower_version_join = OFF
  //       instance_version >= 8.0.35
  // Version compatible, if version < lowest_cluster_version
  // && lowest_cluster_version.series() == version.series()
  BLOCKED(false, Version(8, 0, 33), Version(8, 0, 34));
  ALLOWED(false, Version(8, 3, 2), Version(8, 3, 34));

  // Test: group_replication_allow_local_lower_version_join = OFF
  //       instance_version >= 8.0.35
  // Version compatible, if version < lowest_cluster_version
  // && lowest_cluster_version.series() == version.series()
  BLOCKED(false, Version(8, 0, 35), Version(8, 1, 0));
  BLOCKED(false, Version(8, 4, 2), Version(9, 0, 0));

  // Test: group_replication_allow_local_lower_version_join = OFF
  //       instance_version >= 8.0.35
  // Version compatible, if version < lowest_cluster_version
  // && lowest_cluster_version.series() == version.series()
  BLOCKED(false, Version(8, 0, 36), Version(8, 2, 0));
  BLOCKED(false, Version(8, 4, 36), Version(9, 2, 0));

  //

  // Test: group_replication_allow_local_lower_version_join is not
  // defined
  //       instance_version <= 8.0.16
  // Version compatible, if MAJOR(version) >=
  // MAJOR(lowest_cluster_version)
  ALLOWED(std::nullopt, Version(8, 0, 16), Version(8, 0, 16));
  ALLOWED(std::nullopt, Version(8, 0, 15), Version(8, 0, 16));

  // Test: group_replication_allow_local_lower_version_join is not
  // defined
  //       instance_version <= 8.0.16
  // Version incompatible, if MAJOR(version) <
  // MAJOR(lowest_cluster_version)
  BLOCKED(std::nullopt, Version(5, 7, 26), Version(8, 0, 16));

  // Test: group_replication_allow_local_lower_version_join is not
  // defined
  //       instance_version > 8.0.16
  // Version compatible, if version >= lowest_cluster_version
  ALLOWED(std::nullopt, Version(8, 0, 17), Version(8, 0, 17));
  ALLOWED(std::nullopt, Version(8, 0, 18), Version(8, 0, 17));

  // Test: group_replication_allow_local_lower_version_join is not
  // defined.
  //       instance_version > 8.0.16
  // Version incompatible, if version < lowest_cluster_version
  BLOCKED(std::nullopt, Version(8, 0, 17), Version(8, 0, 18));

  // Test: group_replication_allow_local_lower_version_join is not
  // defined.
  //       instance_version >= 8.0.35
  // Version compatible, if version < lowest_cluster_version
  // && lowest_cluster_version.series() == version.series()
  ALLOWED(std::nullopt, Version(8, 0, 35), Version(8, 0, 36));
  ALLOWED(std::nullopt, Version(8, 4, 35), Version(8, 4, 36));

  // Test: group_replication_allow_local_lower_version_join is not
  // defined.
  //       instance_version >= 8.0.35
  // Version compatible, if version < lowest_cluster_version
  // && lowest_cluster_version.series() == version.series()
  BLOCKED(std::nullopt, Version(8, 0, 36), Version(8, 2, 0));
  BLOCKED(std::nullopt, Version(8, 4, 36), Version(9, 2, 0));

  // Test: group_replication_allow_local_lower_version_join is not
  // defined.
  //       instance_version non-LTS
  // Version compatible, if version < lowest_cluster_version
  // && lowest_cluster_version.series() == version.series()
  BLOCKED(std::nullopt, Version(8, 1, 0), Version(8, 2, 18));
  BLOCKED(std::nullopt, Version(8, 0, 35), Version(8, 1, 0));
  ALLOWED(std::nullopt, Version(8, 1, 0), Version(8, 1, 1));

  // Test: group_replication_allow_local_lower_version_join is not
  // defined.
  //       instance_version non-LTS
  // Version compatible, if version < lowest_cluster_version
  // && lowest_cluster_version.series() == version.series()
  ALLOWED(std::nullopt, Version(8, 2, 0), Version(8, 1, 0));
  ALLOWED(std::nullopt, Version(8, 1, 0), Version(8, 0, 35));
  ALLOWED(std::nullopt, Version(8, 1, 1), Version(8, 1, 0));

  // Test: group_replication_allow_local_lower_version_join is meaningless for
  // cluster versions >= 8.4.0
  ALLOWED(std::nullopt, Version(8, 4, 35), Version(8, 4, 36));
  ALLOWED(false, Version(8, 5, 0), Version(8, 4, 36));

#undef ALLOWED
#undef BLOCKED
}

TEST_F(Group_replication_test, is_instance_only_read_compatible) {
  using mysqlshdk::db::Type;
  using mysqlshdk::utils::Version;

  auto test = [](const Version &instance_version,
                 const Version &lowest_cluster_version,
                 bool is_only_read_compatible) {
    std::string read_compatible = (is_only_read_compatible)
                                      ? "ONLY R/O COMPATIBLE"
                                      : "NOT ONLY R/O COMPATIBLE";

    SCOPED_TRACE("Test " + read_compatible + ":  instance version '" +
                 instance_version.get_base() + "', lower cluster version '" +
                 lowest_cluster_version.get_base() + "'.");

    std::shared_ptr<Mock_session> mock_session =
        std::make_shared<Mock_session>();
    mysqlshdk::mysql::Instance instance{mock_session};

    EXPECT_CALL(*mock_session, get_server_version())
        .WillOnce(Return(instance_version));

    if (is_only_read_compatible) {
      EXPECT_TRUE(mysqlshdk::gr::is_instance_only_read_compatible(
          instance, lowest_cluster_version));
    } else {
      EXPECT_FALSE(mysqlshdk::gr::is_instance_only_read_compatible(
          instance, lowest_cluster_version));
    }
  };

  // Test: instance_version >= 8.0.17 and lowest_cluster_version >= 8.0.0
  // Only read compatible, if version > lowest_cluster_version
  test(Version(8, 0, 17), Version(8, 0, 0), true);
  test(Version(8, 0, 18), Version(8, 0, 0), true);
  test(Version(8, 0, 17), Version(8, 0, 15), true);
  test(Version(8, 0, 17), Version(8, 0, 16), true);

  // BUG#30896344: CHECK FOR READ-ONLY COMPATIBILITY IN ADDINSTANCE USES WRONG
  // MINIMUM VERSION
  test(Version(8, 0, 16), Version(8, 0, 15), false);

  // Test: instance_version >= 8.0.17 and lowest_cluster_version >= 8.0.0
  // Not only read compatible, if version <= lowest_cluster_version
  test(Version(8, 0, 17), Version(8, 0, 17), false);
  test(Version(8, 0, 17), Version(8, 0, 18), false);
  test(Version(8, 0, 18), Version(8, 0, 18), false);
  test(Version(8, 0, 18), Version(8, 0, 19), false);

  // Test: instance_version < 8.0.17
  // Not only read compatible, independently of the lowest_cluster_version
  test(Version(8, 0, 16), Version(8, 0, 0), false);
  test(Version(8, 0, 16), Version(8, 0, 16), false);
  test(Version(8, 0, 16), Version(8, 0, 17), false);
  test(Version(8, 0, 16), Version(5, 7, 26), false);
  test(Version(8, 0, 0), Version(8, 0, 0), false);
  test(Version(8, 0, 0), Version(8, 0, 15), false);
  test(Version(8, 0, 0), Version(8, 0, 16), false);
  test(Version(8, 0, 0), Version(5, 7, 26), false);
  test(Version(5, 7, 26), Version(8, 0, 0), false);
  test(Version(5, 7, 26), Version(8, 0, 15), false);
  test(Version(5, 7, 26), Version(8, 0, 16), false);
  test(Version(5, 7, 26), Version(5, 7, 26), false);
  test(Version(5, 7, 26), Version(5, 7, 25), false);
  test(Version(5, 7, 26), Version(5, 7, 27), false);

  // Test: lowest_cluster_version < 8.0.0
  // Not only read compatible, independently of the instance_version
  test(Version(8, 0, 16), Version(5, 99, 99), false);
  test(Version(8, 0, 0), Version(5, 99, 99), false);
  test(Version(8, 0, 17), Version(5, 99, 99), false);
  test(Version(5, 99, 99), Version(5, 99, 99), false);
  test(Version(5, 7, 26), Version(5, 99, 99), false);
  test(Version(8, 0, 16), Version(5, 7, 26), false);
  test(Version(8, 0, 0), Version(5, 7, 26), false);
  test(Version(8, 0, 17), Version(5, 7, 26), false);
  test(Version(5, 7, 27), Version(5, 7, 26), false);
  test(Version(5, 7, 26), Version(5, 7, 26), false);
  test(Version(5, 7, 25), Version(5, 7, 26), false);
}

TEST_F(Group_replication_test, is_address_supported_by_gr) {
  using mysqlshdk::db::Type;
  using mysqlshdk::utils::Version;

  auto test = [](const std::string &address, const Version &instance_version,
                 bool is_supported) {
    std::string supported = (is_supported) ? "SUPPORTED" : "NOT SUPPORTED";
    SCOPED_TRACE("Test gr_address " + supported + " with address '" + address +
                 "' and instance version '" + instance_version.get_base() +
                 "'.");

    if (is_supported) {
      EXPECT_TRUE(mysqlshdk::gr::is_endpoint_supported_by_gr(address,
                                                             instance_version));
    } else {
      EXPECT_FALSE(mysqlshdk::gr::is_endpoint_supported_by_gr(
          address, instance_version));
    }
  };

  // ipv6 must have square brackets around them
  EXPECT_THROW(
      mysqlshdk::gr::is_endpoint_supported_by_gr("::1:3301", Version(8, 0, 14)),
      shcore::Exception);
  // Must provide a port number
  EXPECT_THROW(
      mysqlshdk::gr::is_endpoint_supported_by_gr("[::1]", Version(8, 0, 14)),
      shcore::Exception);

  // IPv6 are only supported from 8.0.14 onwards
  test("[::1]:3301", Version(8, 0, 14), true);
  test("[::1]:3301", Version(8, 0, 13), false);

  test("[fe80::929b:542:427e:4db4]:3306", Version(8, 0, 14), true);
  test("[fe80::929b:542:427e:4db4]:3306", Version(8, 0, 13), false);
  test("[2001:0db8:85a3:0000:0000:8a2e:0370:7334]:3306", Version(8, 0, 14),
       true);
  test("[2001:0db8:85a3:0000:0000:8a2e:0370:7334]:3306", Version(8, 0, 13),
       false);

  // IPv4 are always supported
  test("192.168.4.4:3301", Version(8, 0, 14), true);
  test("192.168.4.4:3301", Version(8, 0, 13), true);

  // hostnames are not resolved and assumed to be true
  test("sample_hostname:3302", Version(8, 0, 14), true);
  test("localhost:8888", Version(8, 0, 13), true);
  test("this_is_home:3393", Version(8, 0, 13), true);
}

}  // namespace testing
