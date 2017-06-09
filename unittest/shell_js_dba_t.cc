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

#include <algorithm>
#include "modules/adminapi/mod_dba_sql.h"
#include "shellcore/base_session.h"
#include "modules/mod_mysql_session.h"
#include "shell_script_tester.h"
#include "utils/utils_general.h"
#include "mysqlshdk/libs/db/ssl_info.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "utils/utils_file.h"

namespace shcore {
class Shell_js_dba_tests : public Shell_js_script_tester {
protected:
  bool _have_ssl;
  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp() {
    Shell_js_script_tester::SetUp();

    // All of the test cases share the same config folder
    // and setup script
    set_config_folder("js_devapi");
    set_setup_script("setup.js");
  }

  virtual void set_defaults() {
    Shell_js_script_tester::set_defaults();

    int port = 33060, pwd_found;
    std::string protocol, user, password, host, sock, schema;
    mysqlshdk::utils::Ssl_info ssl_info;
    shcore::parse_mysql_connstring(_uri, protocol, user, password, host, port, sock, schema, pwd_found, ssl_info);
    std::string mysql_uri = "mysql://";
    shcore::Argument_list session_args;
    std::shared_ptr<mysqlsh::ShellBaseSession> session;
    mysqlsh::mysql::ClassicSession *classic;
    std::string have_ssl;
    _have_ssl = false;

    if (_port.empty())
      _port = "33060";

    // Connect to test server and check if SSL is enabled
    mysql_uri.append(_mysql_uri);
    if (_mysql_port.empty()) {
      _mysql_port = "3306";
      mysql_uri.append(_mysql_port);
    }
    session_args.push_back(Value(mysql_uri));
    try {
      session = mysqlsh::Shell::connect_session(session_args, mysqlsh::SessionType::Classic);
      classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(session.get());
      mysqlsh::dba::get_server_variable(classic->connection(), "have_ssl",
                                        have_ssl);
      std::transform(have_ssl.begin(), have_ssl.end(), have_ssl.begin(), toupper);
      _have_ssl = (have_ssl.compare("YES") == 0) ? true : false;
      classic->close();
    } catch(shcore::Exception &e) {
      std::string error ("Connection to the base server failed: ");
      error.append(e.what());
      output_handler.debug_print(error);
      output_handler.debug_print("Unable to determine if SSL is available, disabling it by default");
      output_handler.flush_debug_log();
    }

    std::string code = "var __user = '" + user + "';";
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
      code = "__host_port = '" + host + ":3306';";
      exec_and_out_equals(code);
      code = "__mysql_port = 3306;";
      exec_and_out_equals(code);
    } else {
      code = "__host_port = '" + host + ":" + _mysql_port + "';";
      exec_and_out_equals(code);
      code = "__mysql_port = " + _mysql_port + ";";
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
    code = "localhost = 'localhost'";
    exec_and_out_equals(code);
    code = "add_instance_options = {host:localhost, port: 0000, password:'root'};";
    exec_and_out_equals(code);


    if (_have_ssl)
      code = "add_instance_extra_opts = {memberSslMode: 'REQUIRED'};";
    else
      code = "add_instance_extra_opts = {memberSslMode: 'AUTO'};";

    exec_and_out_equals(code);


#ifdef _WIN32
    code = "var __path_splitter = '\\\\';";
    exec_and_out_equals(code);
    auto tokens = shcore::split_string(_sandbox_dir, "\\");
    if (!tokens.at(tokens.size() - 1).empty()) {
      tokens.push_back("");
    }

    _sandbox_dir = shcore::str_join(tokens, "\\\\");
    code = "var __sandbox_dir = '" + _sandbox_dir + "';";
    exec_and_out_equals(code);

    // output sandbox dir
    code =
        "var __output_sandbox_dir = '" + _sandbox_dir + "';";
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
#endif

    code = "var __uripwd = '" + user + ":" + password + "@" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "var __mysqluripwd = '" + user + ":" + password + "@" + host + ":" + _mysql_port + "';";
    exec_and_out_equals(code);
    code = "var __displayuri = '" + user + "@" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "var __displayuridb = '" + user + "@" + host + ":" + _port + "/mysql';";
    exec_and_out_equals(code);
  }

};

TEST_F(Shell_js_dba_tests, no_interactive_sandboxes) {
  _options->wizards = false;
  reset_shell();

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
}

TEST_F(Shell_js_dba_tests, dba_help) {
  validate_interactive("dba_help.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_help) {
  validate_interactive("dba_cluster_help.js");
}

// Regression test for a bug on checkInstanceConfiguration() which
// was requiring an active session to the metadata which is not
// required by design
TEST_F(Shell_js_dba_tests, dba_check_instance_configuration_session) {
  validate_interactive("dba_check_instance_configuration_session.js");
}

TEST_F(Shell_js_dba_tests, no_interactive_deploy_instances) {
  _options->wizards = false;
  reset_shell();

  execute("dba.verbose = true;");

  validate_interactive("dba_reset_or_deploy.js");
}

TEST_F(Shell_js_dba_tests, no_interactive_classic_global_dba) {
  std::string bad_config = "[mysqld]\ngtid_mode = OFF\n";
  create_file("mybad.cnf", bad_config);

  _options->wizards = false;
  reset_shell();

  execute("\\connect -c root:root@localhost:" + _mysql_sandbox_port1 + "");

  // Validates error conditions on create, get and drop cluster
  // Lets the cluster created
  validate_interactive("dba_no_interactive.js");

  execute("session.close();");
}

TEST_F(Shell_js_dba_tests, no_interactive_classic_global_cluster) {
  _options->wizards = false;
  reset_shell();

  execute("\\connect -c root:root@localhost:" + _mysql_sandbox_port1 + "");
  // Tests cluster functionality, adding, removing instances
  // error conditions
  // Lets the cluster empty
  validate_interactive("dba_cluster_no_interactive.js");

  execute("session.close();");
}

TEST_F(Shell_js_dba_tests, DISABLED_no_interactive_classic_global_cluster_multimaster) {
  _options->wizards = false;
  reset_shell();

  execute("\\connect -c root:root@localhost:" + _mysql_sandbox_port1 + "");
  // Tests cluster functionality, adding, removing instances
  // error conditions
  // Lets the cluster empty
  validate_interactive("dba_cluster_multimaster_no_interactive.js");

  execute("session.close();");
}

TEST_F(Shell_js_dba_tests, interactive_classic_global_dba) {
  //IMPORTANT NOTE: This test fixture requires non sandbox server as the base
  // server.
  std::string bad_config = "[mysqld]\ngtid_mode = OFF\n";
  create_file("mybad.cnf", bad_config);

  execute("\\connect -c root:root@localhost:" + _mysql_sandbox_port1 + "");

  //@# Dba: checkInstanceConfiguration error
  output_handler.passwords.push_back("root");

  //@<OUT> Dba: checkInstanceConfiguration ok 1
  output_handler.passwords.push_back("root");

  //@<OUT> Dba: checkInstanceConfiguration report with errors
  output_handler.passwords.push_back("root");

  //@<OUT> Dba: configureLocalInstance error 2
  output_handler.passwords.push_back(_pwd);
  output_handler.prompts.push_back("");

  //@<OUT> Dba: configureLocalInstance error 3
  output_handler.passwords.push_back("root");

  //@ Dba: configureLocalInstance not enough privileges 1
  output_handler.passwords.push_back(""); // Please provide the password for missingprivileges@...
  output_handler.prompts.push_back("1"); // Please select an option [1]: 1
  output_handler.passwords.push_back(""); // Password for new account:
  output_handler.passwords.push_back(""); // confirm password

  //@ Dba: configureLocalInstance not enough privileges 2
  output_handler.passwords.push_back(""); // Please provide the password for missingprivileges@...

  //@ Dba: configureLocalInstance not enough privileges 3
  output_handler.passwords.push_back("");  // Please provide the password for missingprivileges@...
  output_handler.prompts.push_back("2"); // Please select an option [1]: 2
  output_handler.prompts.push_back("missingprivileges@'%'"); // Please provide an account name (e.g: icroot@%) to have it created with the necessary privileges or leave empty and press Enter to cancel.
  output_handler.passwords.push_back(""); // Password for new account:
  output_handler.passwords.push_back(""); // confirm password

  //@<OUT> Dba: configureLocalInstance updating config file
  output_handler.passwords.push_back("root");

  // Validates error conditions on create, get and drop cluster
  // Lets the cluster created
  validate_interactive("dba_interactive.js");

  execute("session.close();");
}

TEST_F(Shell_js_dba_tests, interactive_classic_global_cluster) {
  execute("\\connect -c root:root@localhost:" + _mysql_sandbox_port1 + "");

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

  execute("session.close();");
}

TEST_F(Shell_js_dba_tests, DISABLED_interactive_classic_global_cluster_multimaster) {
  execute("\\connect -c root:root@localhost:" + _mysql_sandbox_port1 + "");

  //@<OUT> Dba: createCluster multiMaster with interaction, cancel
  output_handler.prompts.push_back("no");

  //@<OUT> Dba: createCluster multiMaster with interaction, ok
  output_handler.prompts.push_back("yes");

  //@# Cluster: rejoin_instance with interaction, error
  output_handler.passwords.push_back("n");

  //@# Cluster: rejoin_instance with interaction, error 2
  output_handler.passwords.push_back("n");

  //@<OUT> Cluster: rejoin_instance with interaction, ok
  output_handler.passwords.push_back("root");

  // Tests cluster functionality, adding, removing instances
  // error conditions
  // Lets the cluster empty
  validate_interactive("dba_cluster_multimaster_interactive.js");

  execute("session.close();");
}

TEST_F(Shell_js_dba_tests, configure_local_instance) {
  _options->wizards = false;
  reset_shell();


  // Ensures the three sandboxes contain no group group_replication
  // configuration
  remove_from_cfg_file(_sandbox_cnf_1, "group_replication");
  remove_from_cfg_file(_sandbox_cnf_2, "group_replication");
  remove_from_cfg_file(_sandbox_cnf_3, "group_replication");

  validate_interactive("dba_configure_local_instance.js");

  // Cleans up the cfg file for the third instance
  remove_from_cfg_file(_sandbox_cnf_3, "group_replication");
}



TEST_F(Shell_js_dba_tests, force_quorum) {
  _options->wizards = false;
  reset_shell();

  validate_interactive("dba_cluster_force_quorum.js");
}

TEST_F(Shell_js_dba_tests, force_quorum_interactive) {
  //@ Cluster.forceQuorumUsingPartitionOf error interactive
  output_handler.passwords.push_back("root");

  //@ Cluster.forceQuorumUsingPartitionOf success
  output_handler.passwords.push_back("root");

  validate_interactive("dba_cluster_force_quorum_interactive.js");
}

TEST_F(Shell_js_dba_tests, reboot_cluster) {
  _options->wizards = false;
  reset_shell();

  validate_interactive("dba_reboot_cluster.js");
}

TEST_F(Shell_js_dba_tests, reboot_cluster_interactive) {
  //@ Dba.rebootClusterFromCompleteOutage success
  output_handler.prompts.push_back("y");
  output_handler.prompts.push_back("y");

  validate_interactive("dba_reboot_cluster_interactive.js");
}

TEST_F(Shell_js_dba_tests, cluster_misconfigurations) {
  _options->wizards = false;
  reset_shell();
  output_handler.set_log_level(ngcommon::Logger::LOG_WARNING);

  validate_interactive("dba_cluster_misconfigurations.js");

  std::vector<std::string> log = {
    "DBA: root@localhost:" + _mysql_sandbox_port1 + " : Server variable binlog_format was changed from 'MIXED' to 'ROW'",
    "DBA: root@localhost:" + _mysql_sandbox_port1 + " : Server variable binlog_checksum was changed from 'CRC32' to 'NONE'"};

  MY_EXPECT_LOG_CONTAINS(log);
}

TEST_F(Shell_js_dba_tests, cluster_misconfigurations_interactive) {
  output_handler.set_log_level(ngcommon::Logger::LOG_WARNING);

  //@<OUT> Dba.createCluster: cancel
  output_handler.prompts.push_back("n");

  //@<OUT> Dba.createCluster: ok
  output_handler.prompts.push_back("y");

  validate_interactive("dba_cluster_misconfigurations_interactive.js");

  std::vector<std::string> log = {
    "DBA: root@localhost:" + _mysql_sandbox_port1 + " : Server variable binlog_format was changed from 'MIXED' to 'ROW'",
    "DBA: root@localhost:" + _mysql_sandbox_port1 + " : Server variable binlog_checksum was changed from 'CRC32' to 'NONE'"};

  MY_EXPECT_LOG_CONTAINS(log);
}

TEST_F(Shell_js_dba_tests, cluster_no_misconfigurations) {
  _options->wizards = false;
  reset_shell();
  output_handler.set_log_level(ngcommon::Logger::LOG_WARNING);

  validate_interactive("dba_cluster_no_misconfigurations.js");

  std::vector<std::string> log = {
    "DBA: root@localhost:" + _mysql_sandbox_port1 + " : Server variable binlog_format was changed from 'MIXED' to 'ROW'",
    "DBA: root@localhost:" + _mysql_sandbox_port1 + " : Server variable binlog_checksum was changed from 'CRC32' to 'NONE'"};

  MY_EXPECT_LOG_NOT_CONTAINS(log);
}

TEST_F(Shell_js_dba_tests, cluster_no_misconfigurations_interactive) {
  output_handler.set_log_level(ngcommon::Logger::LOG_WARNING);

  validate_interactive("dba_cluster_no_misconfigurations_interactive.js");

  std::vector<std::string> log = {
    "DBA: root@localhost:" + _mysql_sandbox_port1 + " : Server variable binlog_format was changed from 'MIXED' to 'ROW'",
    "DBA: root@localhost:" + _mysql_sandbox_port1 + " : Server variable binlog_checksum was changed from 'CRC32' to 'NONE'"};

  MY_EXPECT_LOG_NOT_CONTAINS(log);
}

TEST_F(Shell_js_dba_tests, function_preconditions) {
  _options->wizards = false;
  reset_shell();

  validate_interactive("dba_preconditions.js");
}

TEST_F(Shell_js_dba_tests, no_interactive_drop_metadata_schema) {
  _options->wizards = false;
  reset_shell();

  validate_interactive("dba_drop_metadata_no_interactive.js");
}

TEST_F(Shell_js_dba_tests, function_preconditions_interactive) {
  validate_interactive("dba_preconditions.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_add_instance) {
  _options->wizards = false;
  reset_shell();
  validate_interactive("dba_cluster_add_instance.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_remove_instance) {
  _options->wizards = false;
  reset_shell();
  // Regression for Bug #25404009
  validate_interactive("dba_cluster_remove_instance.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_rejoin_instance) {
  _options->wizards = false;
  reset_shell();
  // Regression for Bug #25786495
  validate_interactive("dba_cluster_rejoin_instance.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_check_instance_state) {
  validate_interactive("dba_cluster_check_instance_state.js");
}

TEST_F(Shell_js_dba_tests, interactive_drop_metadata_schema) {
  //@# drop metadata: no user response
  output_handler.prompts.push_back("");

  //@# drop metadata: user response no
  output_handler.prompts.push_back("n");

  //@# drop metadata: user response yes
  output_handler.prompts.push_back("y");

  validate_interactive("dba_drop_metadata_interactive.js");
}

TEST_F(Shell_js_dba_tests, no_interactive_rpl_filter_check) {
  _options->wizards = false;
  reset_shell();

  // Execute setup script to be able to use smart deployment functions.
  execute_setup();

  // Deployment of new sandbox instances.
  // In order to avoid the following bug we ensure the sandboxes are freshly deployed:
  // BUG #25071492: SERVER SESSION ASSERT FAILURE ON SERVER RESTART
  execute("cleanup_sandboxes(true);");
  execute("var deployed1 = reset_or_deploy_sandbox(" + _mysql_sandbox_port1 + ");");
  execute("var deployed2 = reset_or_deploy_sandbox(" + _mysql_sandbox_port2 + ");");
  execute("var deployed3 = reset_or_deploy_sandbox(" + _mysql_sandbox_port3 + ");");

#ifdef _WIN32
  std::string path_splitter = "\\";
#else
  std::string path_splitter = "/";
#endif

  // Restart sandbox instances with specific binlog filtering option.
  std::string stop_options = "{password: 'root'";
  if (!_sandbox_dir.empty()) {
    stop_options.append(", sandboxDir: '" + _sandbox_dir + "'");
  }
  stop_options.append("}");
  std::string cfgpath1 = _sandbox_dir + path_splitter + _mysql_sandbox_port1
      + path_splitter + "my.cnf";
  execute("dba.stopSandboxInstance(" + _mysql_sandbox_port1 + ", " + stop_options + ");");
  add_to_cfg_file(cfgpath1, "[mysqld]\nbinlog-do-db=db1,mysql_innodb_cluster_metadata,db2");
  execute("try_restart_sandbox(" + _mysql_sandbox_port1 + ");");
  std::string cfgpath2 = _sandbox_dir + path_splitter + _mysql_sandbox_port2
      + path_splitter + "my.cnf";
  execute("dba.stopSandboxInstance(" + _mysql_sandbox_port2 + ", " + stop_options + ");");
  add_to_cfg_file(cfgpath2, "[mysqld]\nbinlog-do-db=db1,db2");
  execute("try_restart_sandbox(" + _mysql_sandbox_port2 + ");");
  std::string cfgpath3 = _sandbox_dir + path_splitter + _mysql_sandbox_port3
      + path_splitter + "my.cnf";
  execute("dba.stopSandboxInstance(" + _mysql_sandbox_port3 + ", " + stop_options + ");");
  add_to_cfg_file(cfgpath3, "[mysqld]\nbinlog-ignore-db=db1,mysql_innodb_cluster_metadata,db2");
  execute("try_restart_sandbox(" + _mysql_sandbox_port3 + ");");

  // Validate test script.
  validate_interactive("dba_rpl_filter_check_no_interactive.js");

  // Restart sandbox instances without specific binlog filtering option.
  execute("dba.stopSandboxInstance(" + _mysql_sandbox_port1 + ", " + stop_options + ");");
  remove_from_cfg_file(cfgpath1, "binlog-do-db");
  execute("try_restart_sandbox(" + _mysql_sandbox_port1 + ");");
  execute("dba.stopSandboxInstance(" + _mysql_sandbox_port2 + ", " + stop_options + ");");
  remove_from_cfg_file(cfgpath2, "binlog-do-db");
  execute("try_restart_sandbox(" + _mysql_sandbox_port2 + ");");
  execute("dba.stopSandboxInstance(" + _mysql_sandbox_port3 + ", " + stop_options + ");");
  remove_from_cfg_file(cfgpath3, "binlog-ignore-db");
  execute("try_restart_sandbox(" + _mysql_sandbox_port3 + ");");

  // Clean deployed sandboxes.
  execute("cleanup_or_reset_sandbox(" + _mysql_sandbox_port1 + ", deployed1);");
  execute("cleanup_or_reset_sandbox(" + _mysql_sandbox_port2 + ", deployed2);");
  execute("cleanup_or_reset_sandbox(" + _mysql_sandbox_port3 + ", deployed3);");
}

TEST_F(Shell_js_dba_tests, dba_cluster_change_topology_type) {
  _options->wizards = false;
  reset_shell();

  validate_interactive("dba_cluster_change_topology_type.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_rpl_user_password) {
  _options->wizards = false;
  reset_shell();

  validate_interactive("dba_cluster_rpl_user_password.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_session){
  _options->wizards = false;
  reset_shell();

  validate_interactive("dba_cluster_session.js");
}

TEST_F(Shell_js_dba_tests, dba_cluster_mts) {
  _options->wizards = false;
  reset_shell();

  std::string bad_config = "[mysqld]\ngtid_mode = OFF\n";
  create_file("mybad.cnf", bad_config);

  validate_interactive("dba_cluster_mts.js");
}

TEST_F(Shell_js_dba_tests, no_interactive_delete_instances) {
  _options->wizards = false;
  reset_shell();

  enable_debug();

  // Execute setup script to be able to use smart deployment functions.
  execute_setup();

  execute("cleanup_sandboxes(true);");
}
}
