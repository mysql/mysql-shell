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

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include "modules/adminapi/mod_dba_sql.h"
#include "modules/mod_mysql_session.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "shell_script_tester.h"
#include "shellcore/base_session.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_path.h"

namespace shcore {


class Override_row_string : public mysqlshdk::db::replay::Row_hook {
public:
  Override_row_string(std::unique_ptr<mysqlshdk::db::IRow> source,
                      uint32_t column, const std::string& value)
    : mysqlshdk::db::replay::Row_hook(std::move(source)),
    _column(column), _value(value) {
  }

  std::string get_string(uint32_t index) const override {
    if (index == _column)
      return _value;
    return Row_hook::get_string(index);
  }

  uint32_t _column;
  std::string _value;
};


class Shell_js_dba_tests : public Shell_js_script_tester {
 protected:
  bool _have_ssl;
  std::string _sandbox_share;
  static bool have_sandboxes;

  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp() {
    // Force reset_shell() to happen when reset_shell() is called explicitly
    // in each test case
    _delay_reset_shell = true;
    Shell_js_script_tester::SetUp();

    // All of the test cases share the same config folder
    // and setup script
    set_config_folder("js_devapi");
    set_setup_script("setup.js");
  }

  void backup_sandbox_configurations() {
    shcore::copy_file(_sandbox_cnf_1, _sandbox_cnf_1_bkp);
    shcore::copy_file(_sandbox_cnf_2, _sandbox_cnf_2_bkp);
    shcore::copy_file(_sandbox_cnf_3, _sandbox_cnf_3_bkp);
  }

  void restore_sandbox_configuration(int port) {
    if (port == _mysql_sandbox_nport1)
      shcore::copy_file(_sandbox_cnf_1_bkp, _sandbox_cnf_1);
    else if (port == _mysql_sandbox_nport2)
      shcore::copy_file(_sandbox_cnf_2_bkp, _sandbox_cnf_2);
    else if (port == _mysql_sandbox_nport3)
      shcore::copy_file(_sandbox_cnf_3_bkp, _sandbox_cnf_3);
  }

  void reset_replayable_shell() {
    setup_recorder();  // must be called before set_defaults()
    reset_shell();
    enable_testutil();

#ifdef _WIN32
    mysqlshdk::db::replay::set_replay_query_hook([](const std::string &sql) {
      return shcore::str_replace(sql, ".dll", ".so");
    });
#endif

    // Intercept queries and hack their results so that we can have
    // recorded local sessions that match the actual local environment
    mysqlshdk::db::replay::set_replay_row_hook(
      [this](const mysqlshdk::db::Connection_options& target,
             const std::string& sql,
             std::unique_ptr<mysqlshdk::db::IRow> source)
      -> std::unique_ptr<mysqlshdk::db::IRow> {
      int datadir_column = -1;
      if (sql == "SELECT @@datadir") {
        datadir_column = 0;
      } else if (sql == "select @@port, @@datadir;") {
        datadir_column = 1;
      }

      if (datadir_column >= 0) {
        std::string prefix =
            path::dirname(path::dirname(source->get_string(datadir_column)));
        std::string suffix =
          source->get_string(datadir_column).substr(prefix.length() + 1);
        std::string datadir = path::join_path(_sandbox_dir, suffix);
#ifdef _WIN32
        datadir = str_replace(datadir, "/", "\\");
#endif
        return std::unique_ptr<mysqlshdk::db::IRow>{
          new Override_row_string(std::move(source), datadir_column, datadir)};
      }

      if (sql.find("@@datadir") != std::string::npos)
        throw std::logic_error("query contains datadir but is not intercepted");
      return source;
    });
  }

  virtual void set_defaults() {
    Shell_js_script_tester::set_defaults();

    std::string user, host, password;
    auto connection_options = shcore::get_connection_options(_uri);

    if (connection_options.has_user())
      user = connection_options.get_user();

    if (connection_options.has_host())
      host = connection_options.get_host();

    if (connection_options.has_password())
      password = connection_options.get_password();

    std::string mysql_uri = "mysql://";
    std::string have_ssl;
    _have_ssl = false;

    if (_port.empty())
      _port = "33060";

    if (_port.empty()) {
      _port = "33060";
    }
    if (_mysql_port.empty()) {
      _mysql_port = "3306";
    }

    std::string code = "var hostname = '" + _hostname + "';";
    exec_and_out_equals(code);
    code = "var hostname_ip = '" + _hostname_ip + "';";
    exec_and_out_equals(code);
    code = "var __user = '" + user + "';";
    exec_and_out_equals(code);
    code = "var __pwd = '" + password + "';";
    exec_and_out_equals(code);
    code = "var __host = '" + host + "';";
    exec_and_out_equals(code);
    code = "var __port = " + _port + ";";
    exec_and_out_equals(code);
    code = "var __schema = 'mysql';";
    exec_and_out_equals(code);
    code = "var __uri = '" + user + "@" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "var __xhost_port = '" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    if (_mysql_port.empty()) {
      code = "var __host_port = '" + host + ":3306';";
      exec_and_out_equals(code);
      code = "var __mysql_port = 3306;";
      exec_and_out_equals(code);
    } else {
      code = "var __host_port = '" + host + ":" + _mysql_port + "';";
      exec_and_out_equals(code);
      code = "var __mysql_port = " + _mysql_port + ";";
      exec_and_out_equals(code);
      code = "var __mysql_sandbox_port1 = " + _mysql_sandbox_port1 + ";";
      exec_and_out_equals(code);
      code = "var __mysql_sandbox_port2 = " + _mysql_sandbox_port2 + ";";
      exec_and_out_equals(code);
      code = "var __mysql_sandbox_port3 = " + _mysql_sandbox_port3 + ";";
      exec_and_out_equals(code);
      code = "var uri1 = 'localhost:" + _mysql_sandbox_port1 + "';";
      exec_and_out_equals(code);
      code = "var uri2 = 'localhost:" + _mysql_sandbox_port2 + "';";
      exec_and_out_equals(code);
      code = "var uri3 = 'localhost:" + _mysql_sandbox_port3 + "';";
      exec_and_out_equals(code);
    }
    std::string str_have_ssl = _have_ssl ? "true" : "false";
    code = "var __have_ssl = " + str_have_ssl + ";";
    exec_and_out_equals(code);
    code = "var localhost = 'localhost'";
    exec_and_out_equals(code);
    code =
        "var add_instance_options = {HoSt:localhost, port: 0000, "
        "PassWord:'root', scheme:'mysql'};";
    exec_and_out_equals(code);

    code = "var add_instance_extra_opts = {};";
    exec_and_out_equals(code);
    if (_have_ssl) {
      code = "var __ssl_mode = 'REQUIRED';";
    } else {
      code = "var __ssl_mode = 'DISABLED';";
    }
    exec_and_out_equals(code);

    _sandbox_share = _sandbox_dir + _path_splitter + "sandbox.share";

#ifdef _WIN32
    code = "var __path_splitter = '\\\\';";
    exec_and_out_equals(code);
    auto tokens = shcore::split_string(_sandbox_dir, "\\");
    if (!tokens.at(tokens.size() - 1).empty())
      tokens.push_back("");

    // The sandbox dir for C++
    _sandbox_dir = shcore::str_join(tokens, "\\");

    // The sandbox dir for JS
    code = "var __sandbox_dir = '" + shcore::str_join(tokens, "\\\\") + "';";
    exec_and_out_equals(code);

    // output sandbox dir
    _output_tokens["__output_sandbox_dir"] = shcore::str_join(tokens, "\\");

    tokens = shcore::split_string(_sandbox_share, "\\");
    code = "var __sandbox_share = '" + shcore::str_join(tokens, "\\\\") + "'";
    exec_and_out_equals(code);
#else
    code = "var __path_splitter = '/';";
    exec_and_out_equals(code);
    if (_sandbox_dir.back() != '/') {
      code = "var __sandbox_dir = '" + _sandbox_dir + "/';";
      exec_and_out_equals(code);
      code = "var __output_sandbox_dir = '" + _sandbox_dir + "/';";
      exec_and_out_equals(code);
    } else {
      code = "var __sandbox_dir = '" + _sandbox_dir + "';";
      exec_and_out_equals(code);
      code = "var __output_sandbox_dir = '" + _sandbox_dir + "';";
      exec_and_out_equals(code);
    }

    code = "var __sandbox_share = '" + _sandbox_share + "';";
    exec_and_out_equals(code);
#endif

    code = "var __uripwd = '" + user + ":" + password + "@" + host + ":" +
           _port + "';";
    exec_and_out_equals(code);
    code = "var __mysqluripwd = '" + user + ":" + password + "@" + host + ":" +
           _mysql_port + "';";
    exec_and_out_equals(code);
    code = "var __displayuri = '" + user + "@" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code =
        "var __displayuridb = '" + user + "@" + host + ":" + _port + "/mysql';";
    exec_and_out_equals(code);
  }
};

bool Shell_js_dba_tests::have_sandboxes = true;

TEST_F(Shell_js_dba_tests, no_active_session_error) {
  _options->wizards = false;
  reset_shell();

  execute("var c = dba.getCluster()");
  MY_EXPECT_STDERR_CONTAINS(
      "The Metadata is inaccessible, an active session is required "
      "(LogicError)");

  execute("dba.verbose = true;");
  MY_EXPECT_STDERR_CONTAINS(
      "The Metadata is inaccessible, an active session is required "
      "(LogicError)");
}

// Sandbox specific tests should not do session replay
TEST_F(Shell_js_dba_tests, no_interactive_sandboxes) {
  _options->wizards = false;
  reset_shell();
  output_handler.set_log_level(ngcommon::Logger::LOG_WARNING);
  execute("dba.verbose = true;");

// Create directory with space and quotes in name to test.
#ifdef _WIN32
  std::string path_splitter = "\\";
#else
  std::string path_splitter = "/";
#endif
  // Note: not tested with " in the folder name because windows does not
  // support the creation of directories with: <>:"/\|?*
  std::string dir = _sandbox_dir + path_splitter + "foo \' bar";
  shcore::ensure_dir_exists(dir);

  // Create directory with long name (> 89 char).
  std::string dir_long = _sandbox_dir + path_splitter +
                         "01234567891123456789212345678931234567894123456789"
                         "5123456789612345678971234567898123456789";
  shcore::ensure_dir_exists(dir_long);

  validate_interactive("dba_sandboxes.js");

  // Remove previously created directories.
  shcore::remove_directory(dir);
  shcore::remove_directory(dir_long);
  // BUG#26393614
  std::vector<std::string> log{
      "Warning: Sandbox instances are only suitable for deploying and "
      "running on your local machine for testing purposes and are not "
      "accessible from external networks."};
  MY_EXPECT_LOG_CONTAINS(log);
}

// Regression test for a bug on checkInstanceConfiguration() which
// was requiring an active session to the metadata which is not
// required by design
TEST_F(Shell_js_dba_tests, dba_check_instance_configuration_session) {
  reset_replayable_shell();
  validate_interactive("dba_check_instance_configuration_session.js");
}

#ifdef obsolete
// This will deploy the sandbox instances to be recycled by all tests
// NOTE the previous tests require the sandboxes to NOT be deployed, that's why
// this test is not the first one
TEST_F(Shell_js_dba_tests, no_interactive_deploy_instances) {
  _options->wizards = false;
  reset_shell();

  execute("dba.verbose = true;");

  validate_interactive("dba_reset_or_deploy.js");

  backup_sandbox_configurations();
  shcore::create_file(_sandbox_share, "");

  if (::testing::Test::HasFailure())
    have_sandboxes = false;
}
#endif

TEST_F(Shell_js_dba_tests, interactive_deploy_instance) {
  _options->interactive = true;
  reset_shell();
  output_handler.set_log_level(ngcommon::Logger::LOG_WARNING);
  // BUG 26830224
  // Please enter a MySQL root password for the new instance:
  output_handler.passwords.push_back("root");
  validate_interactive("dba_deploy_sandbox.js");
  // BUG#26393614
  std::vector<std::string> log{
      "Warning: Sandbox instances are only suitable for deploying and "
      "running on your local machine for testing purposes and are not "
      "accessible from external networks."};
  MY_EXPECT_LOG_CONTAINS(log);
}

TEST_F(Shell_js_dba_tests, no_interactive) {
  std::string bad_config = "[mysqld]\ngtid_mode = OFF\n";
  create_file("mybad.cnf", bad_config);

  _options->wizards = false;
  reset_replayable_shell();

  // Validates error conditions on create, get and drop cluster
  // Lets the cluster created
  validate_interactive("dba_no_interactive.js");
}

TEST_F(Shell_js_dba_tests, cluster_no_interactive) {
  _options->wizards = false;
  reset_replayable_shell();

  // Tests cluster functionality, adding, removing instances
  // error conditions
  // Lets the cluster empty
  validate_interactive("dba_cluster_no_interactive.js");
}

TEST_F(Shell_js_dba_tests, cluster_multimaster_no_interactive) {
  _options->wizards = false;
  reset_replayable_shell();

  // Tests cluster functionality, adding, removing instances
  // error conditions
  // Lets the cluster empty
  validate_interactive("dba_cluster_multimaster_no_interactive.js");
}


TEST_F(Shell_js_dba_tests, interactive) {
  // IMPORTANT NOTE: This test fixture requires non sandbox server as the base
  // server.
  std::string bad_config = "[mysqld]\ngtid_mode = OFF\n";
  create_file("mybad.cnf", bad_config);

  _options->interactive = true;
  reset_replayable_shell();

  //@# Dba: checkInstanceConfiguration error
  output_handler.passwords.push_back("root");

  //@<OUT> Dba: checkInstanceConfiguration ok 1
  output_handler.passwords.push_back("root");

  //@<OUT> Dba: checkInstanceConfiguration report with errors
  output_handler.passwords.push_back("root");

  // TODO(rennox): This test case is not reliable since requires
  // that no my.cnf exist on the default paths
  //@<OUT> Dba: configureLocalInstance error 2
  // output_handler.passwords.push_back(_pwd);
  // output_handler.prompts.push_back("");

  //@<OUT> Dba: configureLocalInstance error 3
  output_handler.passwords.push_back("root");

  //@ Dba: configureLocalInstance not enough privileges 1
  output_handler.passwords.push_back(
      "");  // Please provide the password for missingprivileges@...
  output_handler.prompts.push_back("1");   // Please select an option [1]: 1
  output_handler.passwords.push_back("");  // Password for new account:
  output_handler.passwords.push_back("");  // confirm password

  //@ Dba: configureLocalInstance not enough privileges 2
  output_handler.passwords.push_back(
      "");  // Please provide the password for missingprivileges@...

  //@ Dba: configureLocalInstance not enough privileges 3
  output_handler.passwords.push_back(
      "");  // Please provide the password for missingprivileges@...
  output_handler.prompts.push_back("2");  // Please select an option [1]: 2
  output_handler.prompts.push_back(
      "missingprivileges@'%'");  // Please provide an account name (e.g:
                                 // icroot@%) to have it created with the
                                 // necessary privileges or leave empty and
                                 // press Enter to cancel.
  output_handler.passwords.push_back("");  // Password for new account:
  output_handler.passwords.push_back("");  // confirm password

  //@<OUT> Dba: configureLocalInstance updating config file
  output_handler.passwords.push_back("root");

  //@<OUT> Dba: configureLocalInstance create different admin user
  output_handler.passwords.push_back("");  // Pass for mydba
  output_handler.prompts.push_back("2");   // Option (account with diff name)
  output_handler.prompts.push_back("dba_test");  // account name
  output_handler.passwords.push_back("");        // account pass
  output_handler.passwords.push_back("");        // account pass confirmation

  //@<OUT> Dba: configureLocalInstance create existing valid admin user
  output_handler.passwords.push_back("");  // Pass for mydba
  output_handler.prompts.push_back("2");   // Option (account with diff name)
  output_handler.prompts.push_back("dba_test");  // account name
  output_handler.passwords.push_back("");        // account pass
  output_handler.passwords.push_back("");        // account pass confirmation

  //@<OUT> Dba: configureLocalInstance create existing invalid admin user
  output_handler.passwords.push_back("");  // Pass for mydba
  output_handler.prompts.push_back("2");   // Option (account with diff name)
  output_handler.prompts.push_back("dba_test");  // account name
  output_handler.passwords.push_back("");        // account pass
  output_handler.passwords.push_back("");        // account pass confirmation

  // Validates error conditions on create, get and drop cluster
  // Lets the cluster created
  validate_interactive("dba_interactive.js");
}

TEST_F(Shell_js_dba_tests, cluster_interactive) {
  _options->interactive = true;
  reset_replayable_shell();

  //@# Cluster: rejoin_instance with interaction, error
  output_handler.passwords.push_back("n");

  //@# Cluster: rejoin_instance with interaction, error 2
  output_handler.passwords.push_back("n");

  //@<OUT> Cluster: rejoin_instance with interaction, ok
  output_handler.passwords.push_back("root");

  // Tests cluster functionality, adding, removing instances
  // error conditions
  // Lets the cluster empty
  validate_interactive("dba_cluster_interactive.js");
}

TEST_F(Shell_js_dba_tests, cluster_multimaster_interactive) {
  _options->interactive = true;
  reset_replayable_shell();

  //@<OUT> Dba: createCluster multiMaster with interaction, cancel
  output_handler.prompts.push_back("no");

  //@<OUT> Dba: createCluster multiMaster with interaction, ok
  output_handler.prompts.push_back("yes");

  //@<OUT> Dba: createCluster multiMaster with interaction 2, ok
  output_handler.prompts.push_back("yes");

  //@# Cluster: rejoin_instance with interaction, error
  output_handler.passwords.push_back("n");

  //@# Cluster: rejoin_instance with interaction, error 2
  output_handler.passwords.push_back("n");

  //@<OUT> Cluster: rejoin_instance with interaction, ok
  output_handler.passwords.push_back("root");

  output_handler.set_log_level(ngcommon::Logger::LOG_INFO);

  // Tests cluster functionality, adding, removing instances
  // error conditions.
  validate_interactive("dba_cluster_multimaster_interactive.js");

  std::vector<std::string> log = {
      "The MySQL InnoDB cluster is going to be setup in advanced Multi-Master "
      "Mode. Consult its requirements and limitations in "
      "https://dev.mysql.com/doc/refman/en/group-replication-limitations.html"};

  MY_EXPECT_LOG_CONTAINS(log);
}

TEST_F(Shell_js_dba_tests, DISABLED_configure_local_instance) {
  _options->wizards = false;
  reset_shell();
  output_handler.set_log_level(ngcommon::Logger::LOG_INFO);

  // Execute setup script to be able to use smart deployment functions.
  execute_setup();

  // Clean and delete all sandboxes.
  execute("cleanup_sandboxes(true)");

  // Deploy new sandbox instances (required for this test).
  execute("deployed_here = reset_or_deploy_sandboxes()");

  // Ensures the three sandboxes contain no group group_replication
  // configuration
  remove_from_cfg_file(_sandbox_cnf_1, "group_replication");
  remove_from_cfg_file(_sandbox_cnf_2, "group_replication");
  remove_from_cfg_file(_sandbox_cnf_3, "group_replication");

  // Restart sandbox instances.
  std::string stop_options = "{password: 'root', sandboxDir: __sandbox_dir}";
  execute("dba.stopSandboxInstance(__mysql_sandbox_port1, " + stop_options +
          ");");
  execute("try_restart_sandbox(__mysql_sandbox_port1);");
  execute("dba.stopSandboxInstance(__mysql_sandbox_port2, " + stop_options +
          ");");
  execute("try_restart_sandbox(__mysql_sandbox_port2);");
  execute("dba.stopSandboxInstance(__mysql_sandbox_port3, " + stop_options +
          ");");
  execute("try_restart_sandbox(__mysql_sandbox_port3);");

  // Run the tests
  validate_interactive("dba_configure_local_instance.js");
  // MP should have been called with the cluster admin account that was created
  // (BUG#26979375)
#ifndef _WIN32
  MY_EXPECT_LOG_CONTAINS("mysqlprovision check --instance=gr_user2@", false);
#else
  MY_EXPECT_LOG_CONTAINS("mysqlprovision.zip check --instance=gr_user2@",
                         false);
#endif
  // MP should have been called with the cluster admin account that already
  // existed (BUG#26979375)
#ifndef _WIN32
  MY_EXPECT_LOG_CONTAINS("mysqlprovision check --instance=gr_user@", true);
#else
  MY_EXPECT_LOG_CONTAINS("mysqlprovision.zip check --instance=gr_user@", true);
#endif
  // Clean up sandboxes.
  execute("cleanup_sandboxes(true)");
}

TEST_F(Shell_js_dba_tests, configure_local_instance_errors) {
  _options->wizards = false;
  reset_replayable_shell();

  validate_interactive("dba_configure_local_instance_errors.js");
}

TEST_F(Shell_js_dba_tests, cluster_force_quorum) {
  _options->wizards = false;
  reset_replayable_shell();

  validate_interactive("dba_cluster_force_quorum.js");
}

TEST_F(Shell_js_dba_tests, cluster_force_quorum_interactive) {
  _options->interactive = true;
  reset_replayable_shell();

  //@ Cluster.forceQuorumUsingPartitionOf error interactive
  output_handler.passwords.push_back("root");

  //@ Cluster.forceQuorumUsingPartitionOf success
  output_handler.passwords.push_back("root");

  validate_interactive("dba_cluster_force_quorum_interactive.js");
}

TEST_F(Shell_js_dba_tests, reboot_cluster) {
  _options->wizards = false;
  reset_replayable_shell();

  validate_interactive("dba_reboot_cluster.js");
}

TEST_F(Shell_js_dba_tests, reboot_cluster_interactive) {
  _options->interactive = true;
  reset_replayable_shell();

  //@ Dba.rebootClusterFromCompleteOutage success
  output_handler.prompts.push_back("y");
  output_handler.prompts.push_back("y");

  validate_interactive("dba_reboot_cluster_interactive.js");
}

TEST_F(Shell_js_dba_tests, cluster_misconfigurations) {
  _options->wizards = false;
  reset_replayable_shell();
  output_handler.set_log_level(ngcommon::Logger::LOG_WARNING);

  validate_interactive("dba_cluster_misconfigurations.js");

  std::vector<std::string> log = {
      "DBA: root@localhost:" + _mysql_sandbox_port1 +
          " : Server variable binlog_format was changed from 'MIXED' to 'ROW'",
      "DBA: root@localhost:" + _mysql_sandbox_port1 +
          " : Server variable binlog_checksum was changed from 'CRC32' to "
          "'NONE'"};

  MY_EXPECT_LOG_CONTAINS(log);
  // Validate output for chunk: Create cluster fails
  // (one table is not compatible) - verbose mode
  // Regression for BUG#25966731 : ALLOW-NON-COMPATIBLE-TABLES OPTION DOES
  // NOT EXIST
  MY_EXPECT_STDOUT_NOT_CONTAINS("the --allow-non-compatible-tables option");
}

TEST_F(Shell_js_dba_tests, cluster_misconfigurations_interactive) {
  _options->interactive = true;
  reset_replayable_shell();

  output_handler.set_log_level(ngcommon::Logger::LOG_WARNING);

  //@<OUT> Dba.createCluster: cancel
  output_handler.prompts.push_back("n");

  //@<OUT> Dba.createCluster: ok
  output_handler.prompts.push_back("y");

  validate_interactive("dba_cluster_misconfigurations_interactive.js");

  std::vector<std::string> log = {
      "DBA: root@localhost:" + _mysql_sandbox_port1 +
          " : Server variable binlog_format was changed from 'MIXED' to 'ROW'",
      "DBA: root@localhost:" + _mysql_sandbox_port1 +
          " : Server variable binlog_checksum was changed from 'CRC32' to "
          "'NONE'"};

  MY_EXPECT_LOG_CONTAINS(log);
}

TEST_F(Shell_js_dba_tests, cluster_no_misconfigurations) {
  _options->wizards = false;
  reset_replayable_shell();
  output_handler.set_log_level(ngcommon::Logger::LOG_WARNING);

  validate_interactive("dba_cluster_no_misconfigurations.js");

  std::vector<std::string> log = {
      "DBA: root@localhost:" + _mysql_sandbox_port1 +
          " : Server variable binlog_format was changed from 'MIXED' to 'ROW'",
      "DBA: root@localhost:" + _mysql_sandbox_port1 +
          " : Server variable binlog_checksum was changed from 'CRC32' to "
          "'NONE'"};

  MY_EXPECT_LOG_NOT_CONTAINS(log);
}

TEST_F(Shell_js_dba_tests, cluster_no_misconfigurations_interactive) {
  _options->interactive = true;
  reset_replayable_shell();

  output_handler.set_log_level(ngcommon::Logger::LOG_WARNING);

  validate_interactive("dba_cluster_no_misconfigurations_interactive.js");

  std::vector<std::string> log = {
      "DBA: root@localhost:" + _mysql_sandbox_port1 +
          " : Server variable binlog_format was changed from 'MIXED' to 'ROW'",
      "DBA: root@localhost:" + _mysql_sandbox_port1 +
          " : Server variable binlog_checksum was changed from 'CRC32' to "
          "'NONE'"};

  MY_EXPECT_LOG_NOT_CONTAINS(log);
}

TEST_F(Shell_js_dba_tests, no_interactive_drop_metadata_schema) {
  _options->wizards = false;
  reset_replayable_shell();

  validate_interactive("dba_drop_metadata_no_interactive.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_add_instance) {
  _options->wizards = false;
  reset_replayable_shell();
  output_handler.set_log_level(ngcommon::Logger::LOG_WARNING);

  validate_interactive("dba_cluster_add_instance.js");

  // BUG#26393614
  std::string sandbox_path = shcore::path::join_path(
      {_sandbox_dir, _mysql_sandbox_port2, "sandboxdata"});
  std::vector<std::string> log{
      "'localhost' (" + sandbox_path +
      ") detected as local sandbox. "
      "Sandbox instances are only suitable for deploying and "
      "running on your local machine for testing purposes and are not "
      "accessible from external networks."};
  MY_EXPECT_LOG_CONTAINS(log);
}

TEST_F(Shell_js_dba_tests, dba_cluster_remove_instance) {
  _options->wizards = false;
  reset_replayable_shell();
  // Regression for Bug #25404009
  validate_interactive("dba_cluster_remove_instance.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_rejoin_instance) {
  _options->wizards = false;
  reset_replayable_shell();
  // Regression for Bug #25786495
  validate_interactive("dba_cluster_rejoin_instance.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_check_instance_state) {
  reset_replayable_shell();
  validate_interactive("dba_cluster_check_instance_state.js");
}

TEST_F(Shell_js_dba_tests, dba_drop_metadata_interactive) {
  _options->interactive = true;
  reset_replayable_shell();

  //@# drop metadata: no user response
  output_handler.prompts.push_back("");

  //@# drop metadata: user response no
  output_handler.prompts.push_back("n");

  //@# drop metadata: user response yes
  output_handler.prompts.push_back("y");

  validate_interactive("dba_drop_metadata_interactive.js");
}

TEST_F(Shell_js_dba_tests, rpl_filter_check_no_interactive) {
  _options->wizards = false;
  reset_replayable_shell();

  // Validate test script.
  validate_interactive("dba_rpl_filter_check_no_interactive.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_change_topology_type) {
  _options->wizards = false;
  reset_replayable_shell();

  validate_interactive("dba_cluster_change_topology_type.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_rpl_user_password) {
  _options->wizards = false;
  reset_replayable_shell();

#ifdef _WIN32
  execute("var __plugin='validate_password.dll'");
#else
  execute("var __plugin='validate_password.so'");
#endif

  validate_interactive("dba_cluster_rpl_user_password.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_session) {
  _options->wizards = false;
  reset_replayable_shell();

  validate_interactive("dba_cluster_session.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_mts) {
  _options->wizards = false;
  reset_replayable_shell();

  std::string bad_config = "[mysqld]\ngtid_mode = OFF\n";
  create_file("mybad.cnf", bad_config);

  validate_interactive("dba_cluster_mts.js");
}

TEST_F(Shell_js_dba_tests, dba_configure_new_instance) {
  // Regression for BUG#26818744 : MYSQL SHELL DOESN'T ADD THE SERVER_ID ANYMORE
  _options->wizards = false;
  reset_replayable_shell();

  validate_interactive("dba_configure_new_instance.js");
}

TEST_F(Shell_js_dba_tests, dba_help) {
  reset_shell();
  validate_interactive("dba_help.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_help) {
  reset_replayable_shell();
  validate_interactive("dba_cluster_help.js");
}

TEST_F(Shell_js_dba_tests, super_read_only_handling) {
  reset_replayable_shell();
  //@<OUT> Configures the instance, answers 'yes' on the read only prompt
  output_handler.prompts.push_back("y");

  //@<OUT> Creates Cluster succeeds, answers 'yes' on read only prompt
  output_handler.prompts.push_back("y");

  //@ Reboot the cluster
  // Confirms addition of second instance
  output_handler.prompts.push_back("y");

  // Confirms addition of third instance
  output_handler.prompts.push_back("y");

  // Confirms clean up of read only
  output_handler.prompts.push_back("y");

  validate_interactive("dba_super_read_only_handling.js");
}

TEST_F(Shell_js_dba_tests, adopt_from_gr) {
  _options->wizards = false;
  reset_replayable_shell();

  validate_interactive("dba_adopt_from_gr.js");
}

TEST_F(Shell_js_dba_tests, adopt_from_gr_interactive) {
  reset_replayable_shell();
  // Are you sure you want to remove the Metadata? [y|N]:
  output_handler.prompts.push_back("y");

  // Do you want to setup an InnoDB cluster based on this replication
  // group? [Y|n]:
  output_handler.prompts.push_back("y");

  validate_interactive("dba_adopt_from_gr_interactive.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_dissolve) {
  _options->wizards = false;
  reset_replayable_shell();

  validate_interactive("dba_cluster_dissolve.js");
}

TEST_F(Shell_js_dba_tests, advanced_options) {
  _options->wizards = false;
  reset_replayable_shell();

  validate_interactive("dba_advanced_options.js");
}

#ifdef obsolete
TEST_F(Shell_js_dba_tests, no_interactive_delete_instances) {
  _options->wizards = false;
  reset_shell();

  enable_debug();

  // Execute setup script to be able to use smart deployment functions.
  execute_setup();

  execute("cleanup_sandboxes(true);");

  shcore::delete_file(_sandbox_share);
  shcore::delete_file(_sandbox_cnf_1_bkp);
  shcore::delete_file(_sandbox_cnf_2_bkp);
  shcore::delete_file(_sandbox_cnf_3_bkp);
}
#endif

}  // namespace shcore
