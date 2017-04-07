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
#include "modules/base_session.h"
#include "modules/mod_mysql_session.h"
#include "shell_script_tester.h"
#include "utils/utils_general.h"
#include "utils/utils_connection.h"

namespace shcore {
class Shell_py_dba_tests : public Shell_py_script_tester {
protected:
  bool _have_ssl;
  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp() {
    Shell_py_script_tester::SetUp();

    // All of the test cases share the same config folder
    // and setup script
    set_config_folder("py_devapi");
    set_setup_script("setup.py");
  }

  virtual void set_defaults() {
    Shell_py_script_tester::set_defaults();

    int port = 33060, pwd_found;
    std::string protocol, user, password, host, sock, schema;
    struct SslInfo ssl_info;
    shcore::parse_mysql_connstring(_uri, protocol, user, password, host, port, sock, schema, pwd_found, ssl_info);
    std::string mysql_uri = "mysql://";
    shcore::Argument_list session_args;
    std::shared_ptr<mysqlsh::ShellDevelopmentSession> session;
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
      output_handler.debug_print("Connecting to the base server...");
      session = mysqlsh::connect_session(session_args, mysqlsh::SessionType::Classic);
      output_handler.debug_print("Connection succeeded...");

      classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(session.get());
      mysqlsh::dba::get_server_variable(classic->connection(), "have_ssl",
                                        have_ssl);
      std::transform(have_ssl.begin(), have_ssl.end(), have_ssl.begin(), toupper);
      _have_ssl = (have_ssl.compare("YES") == 0) ? true : false;
      shcore::Argument_list args;
      classic->close(args);

    } catch(shcore::Exception &e){
      std::string error ("Connection to the base server failed: ");
      error.append(e.what());
      output_handler.debug_print(error);
      output_handler.debug_print("Unable to determine if SSL is available, disabling it by default");
      output_handler.flush_debug_log();
    }

    std::string code = "__user = '" + user + "';";
    exec_and_out_equals(code);
    code = "__pwd = '" + password + "';";
    exec_and_out_equals(code);
    code = "__host = '" + host + "';";
    exec_and_out_equals(code);
    code = "__port = " + _port + ";";
    exec_and_out_equals(code);
    code = "__schema = 'mysql';";
    exec_and_out_equals(code);
    code = "__uri = '" + user + "@" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "__xhost_port = '" + host + ":" + _port + "';";
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
      code = "__mysql_sandbox_port1 = " + _mysql_sandbox_port1 + ";";
      exec_and_out_equals(code);
      code = "__mysql_sandbox_port2 = " + _mysql_sandbox_port2 + ";";
      exec_and_out_equals(code);
      code = "__mysql_sandbox_port3 = " + _mysql_sandbox_port3 + ";";
      exec_and_out_equals(code);
      code = "uri1 = 'localhost:" + _mysql_sandbox_port1 + "'";
      exec_and_out_equals(code);
      code = "uri2 = 'localhost:" + _mysql_sandbox_port2 + "'";
      exec_and_out_equals(code);
      code = "uri3 = 'localhost:" + _mysql_sandbox_port3 + "'";
      exec_and_out_equals(code);
    }
    std::string str_have_ssl = _have_ssl ? "True" : "False";
    code = "__have_ssl = " + str_have_ssl + ";";
    exec_and_out_equals(code);

    code = "localhost = 'localhost'";
    exec_and_out_equals(code);
    code = "add_instance_options = {'host':localhost, 'port': 0000, 'password':'root'};";
    exec_and_out_equals(code);


    if (_have_ssl)
      code = "add_instance_extra_opts = {'memberSslMode': 'REQUIRED'}";
    else
      code = "add_instance_extra_opts = {'memberSslMode': 'AUTO'}";

    exec_and_out_equals(code);


#ifdef _WIN32
    code = "__path_splitter = '\\\\';";
    exec_and_out_equals(code);
    auto tokens = shcore::split_string(_sandbox_dir, "\\");
    if (!tokens.at(tokens.size() - 1).empty()) {
      tokens.push_back("");
      tokens.push_back("");
    }

    _sandbox_dir = shcore::join_strings(tokens, "\\\\");
    code = "__sandbox_dir = '" + _sandbox_dir + "';";
    exec_and_out_equals(code);

    // output sandbox dir
    code = "__output_sandbox_dir = '" + shcore::join_strings(tokens, "\\") + "';";
    exec_and_out_equals(code);
#else
    code = "__path_splitter = '/';";
    exec_and_out_equals(code);
    if (_sandbox_dir.back() != '/') {
      code = "__sandbox_dir = '" + _sandbox_dir + "/';";
      exec_and_out_equals(code);
      code = "__output_sandbox_dir = '" + _sandbox_dir + "/';";
      exec_and_out_equals(code);
    } else {
      code = "__sandbox_dir = '" + _sandbox_dir + "';";
      exec_and_out_equals(code);
      code = "__output_sandbox_dir = '" + _sandbox_dir + "';";
      exec_and_out_equals(code);
    }
#endif

    code = "__uripwd = '" + user + ":" + password + "@" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "__displayuri = '" + user + "@" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "__displayuridb = '" + user + "@" + host + ":" + _port + "/mysql';";
    exec_and_out_equals(code);
    code = "import os";
  }

};

TEST_F(Shell_py_dba_tests, dba_help) {
  validate_interactive("dba_help.py");
}

TEST_F(Shell_py_dba_tests, no_interactive_deploy_instances) {
  _options->wizards = false;
  reset_shell();

  execute("dba.verbose = True");

  validate_interactive("dba_reset_or_deploy.py");
}

TEST_F(Shell_py_dba_tests, no_interactive_classic_global_dba) {
  std::string bad_config = "[mysqld]\ngtid_mode = OFF\n";
  create_file("mybad.cnf", bad_config);

  _options->wizards = false;
  reset_shell();

  execute("\\connect -c root:root@localhost:" + _mysql_sandbox_port1 + "");

  // Validates error conditions on create, get and drop cluster
  // Lets the cluster created
  validate_interactive("dba_no_interactive.py");

  execute("session.close();");
}

TEST_F(Shell_py_dba_tests, no_interactive_classic_global_cluster) {
  _options->wizards = false;
  reset_shell();

  execute("\\connect -c root:root@localhost:" + _mysql_sandbox_port1 + "");
  // Tests cluster functionality, adding, removing instances
  // error conditions
  // Lets the cluster empty
  validate_interactive("dba_cluster_no_interactive.py");

  execute("session.close()");
}

TEST_F(Shell_py_dba_tests, DISABLED_no_interactive_classic_global_cluster_multimaster) {
  _options->wizards = false;
  reset_shell();

  execute("\\connect -c root:root@localhost:" + _mysql_sandbox_port1 + "");
  // Tests cluster functionality, adding, removing instances
  // error conditions
  // Lets the cluster empty
  validate_interactive("dba_cluster_multimaster_no_interactive.py");

  execute("session.close();");
}

TEST_F(Shell_py_dba_tests, interactive_classic_global_dba) {
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
  output_handler.prompts.push_back("mybad.cnf");

  //@<OUT> Dba: configureLocalInstance updating config file
  output_handler.passwords.push_back("root");

  // Validates error conditions on create, get and drop cluster
  // Lets the cluster created
  validate_interactive("dba_interactive.py");

  execute("session.close();");
}

TEST_F(Shell_py_dba_tests, interactive_classic_global_cluster) {
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
  validate_interactive("dba_cluster_interactive.py");

  execute("session.close();");
}

TEST_F(Shell_py_dba_tests, DISABLED_interactive_classic_global_cluster_multimaster) {
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
  validate_interactive("dba_cluster_multimaster_interactive.py");

  execute("session.close();");
}

TEST_F(Shell_py_dba_tests, configure_local_instance) {
  _options->wizards = false;
  reset_shell();


  // Ensures the three sandboxes contain no group group_replication
  // configuration
  remove_from_cfg_file(_sandbox_cnf_1, "group_replication");
  remove_from_cfg_file(_sandbox_cnf_2, "group_replication");
  remove_from_cfg_file(_sandbox_cnf_3, "group_replication");

  validate_interactive("dba_configure_local_instance.py");

  // Cleans up the cfg file for the third instance
  remove_from_cfg_file(_sandbox_cnf_3, "group_replication");
}

TEST_F(Shell_py_dba_tests, force_quorum) {
  _options->wizards = false;
  reset_shell();

  validate_interactive("dba_cluster_force_quorum.py");
}

TEST_F(Shell_py_dba_tests, force_quorum_interactive) {

  //@ Cluster.forceQuorumUsingPartitionOf error interactive
  output_handler.passwords.push_back("root");

  //@ Cluster.forceQuorumUsingPartitionOf success
  output_handler.passwords.push_back("root");

  validate_interactive("dba_cluster_force_quorum_interactive.py");
}

TEST_F(Shell_py_dba_tests, reboot_cluster) {
  _options->wizards = false;
  reset_shell();

  validate_interactive("dba_reboot_cluster.py");
}

TEST_F(Shell_py_dba_tests, reboot_cluster_interactive) {
  //@ Dba.rebootClusterFromCompleteOutage success
  output_handler.prompts.push_back("y");
  output_handler.prompts.push_back("y");

  validate_interactive("dba_reboot_cluster_interactive.py");
}

TEST_F(Shell_py_dba_tests, cluster_misconfigurations) {
  _options->wizards = false;
  reset_shell();
  output_handler.set_log_level(ngcommon::Logger::LOG_WARNING);

  validate_interactive("dba_cluster_misconfigurations.py");

  std::vector<std::string> log = {
    "DBA: root@localhost:" + _mysql_sandbox_port1 + " : Server variable binlog_format was changed from 'MIXED' to 'ROW'",
    "DBA: root@localhost:" + _mysql_sandbox_port1 + " : Server variable binlog_checksum was changed from 'CRC32' to 'NONE'"};

  MY_EXPECT_LOG_CONTAINS(log);
}

TEST_F(Shell_py_dba_tests, cluster_misconfigurations_interactive) {
  output_handler.set_log_level(ngcommon::Logger::LOG_WARNING);

  //@<OUT> Dba.createCluster: cancel
  output_handler.prompts.push_back("n");

  //@<OUT> Dba.createCluster: ok
  output_handler.prompts.push_back("y");

  validate_interactive("dba_cluster_misconfigurations_interactive.py");

  std::vector<std::string> log = {
    "DBA: root@localhost:" + _mysql_sandbox_port1 + " : Server variable binlog_format was changed from 'MIXED' to 'ROW'",
    "DBA: root@localhost:" + _mysql_sandbox_port1 + " : Server variable binlog_checksum was changed from 'CRC32' to 'NONE'"};

  MY_EXPECT_LOG_CONTAINS(log);
}

TEST_F(Shell_py_dba_tests, cluster_no_misconfigurations) {
  _options->wizards = false;
  reset_shell();
  output_handler.set_log_level(ngcommon::Logger::LOG_WARNING);

  validate_interactive("dba_cluster_no_misconfigurations.py");

  std::vector<std::string> log = {
    "DBA: root@localhost:" + _mysql_sandbox_port1 + " : Server variable binlog_format was changed from 'MIXED' to 'ROW'",
    "DBA: root@localhost:" + _mysql_sandbox_port1 + " : Server variable binlog_checksum was changed from 'CRC32' to 'NONE'"};

  MY_EXPECT_LOG_NOT_CONTAINS(log);
}

TEST_F(Shell_py_dba_tests, cluster_no_misconfigurations_interactive) {
  output_handler.set_log_level(ngcommon::Logger::LOG_WARNING);

  validate_interactive("dba_cluster_no_misconfigurations_interactive.py");

  std::vector<std::string> log = {
    "DBA: root@localhost:" + _mysql_sandbox_port1 + " : Server variable binlog_format was changed from 'MIXED' to 'ROW'",
    "DBA: root@localhost:" + _mysql_sandbox_port1 + " : Server variable binlog_checksum was changed from 'CRC32' to 'NONE'"};

  MY_EXPECT_LOG_NOT_CONTAINS(log);
}

TEST_F(Shell_py_dba_tests, function_preconditions) {
  _options->wizards = false;
  reset_shell();

  validate_interactive("dba_preconditions.py");
}

TEST_F(Shell_py_dba_tests, no_interactive_drop_metadata_schema) {
  _options->wizards = false;
  reset_shell();

  validate_interactive("dba_drop_metadata_no_interactive.py");
}

TEST_F(Shell_py_dba_tests, function_preconditions_interactive) {
  validate_interactive("dba_preconditions.py");
}

TEST_F(Shell_py_dba_tests, dba_cluster_add_instance) {
  _options->wizards = false;
  reset_shell();
  validate_interactive("dba_cluster_add_instance.py");
}

TEST_F(Shell_py_dba_tests, dba_cluster_check_instance_state) {
  validate_interactive("dba_cluster_check_instance_state.py");
}

TEST_F(Shell_py_dba_tests, interactive_drop_metadata_schema) {
  //@# drop metadata: no user response
  output_handler.prompts.push_back("");

  //@# drop metadata: user response no
  output_handler.prompts.push_back("n");

  //@# drop metadata: user response yes
  output_handler.prompts.push_back("y");

  validate_interactive("dba_drop_metadata_interactive.py");
}

TEST_F(Shell_py_dba_tests, dba_cluster_change_topology_type) {
  _options->wizards = false;
  reset_shell();

  validate_interactive("dba_cluster_change_topology_type.py");
}

TEST_F(Shell_py_dba_tests, no_interactive_rpl_filter_check) {
  _options->wizards = false;
  reset_shell();

  // Execute setup script to be able to use smart deployment functions.
  execute_setup();

  // Deployment of new sandbox instances.
  // In order to avoid the following bug we ensure the sandboxes are freshly deployed:
  // BUG #25071492: SERVER SESSION ASSERT FAILURE ON SERVER RESTART
  execute("cleanup_sandboxes(True)");
  execute("deployed1 = reset_or_deploy_sandbox(" + _mysql_sandbox_port1 + ")");
  execute("deployed2 = reset_or_deploy_sandbox(" + _mysql_sandbox_port2 + ")");
  execute("deployed3 = reset_or_deploy_sandbox(" + _mysql_sandbox_port3 + ")");

#ifdef _WIN32
  std::string path_splitter = "\\";
#else
  std::string path_splitter = "/";
#endif

  // Restart sandbox instances with specific binlog filtering option.
  std::string stop_options = "{'password': 'root'";
  if (!_sandbox_dir.empty()) {
    stop_options.append(", 'sandboxDir': '" + _sandbox_dir + "'");
  }
  stop_options.append("}");
  std::string cfgpath1 = _sandbox_dir + path_splitter + _mysql_sandbox_port1
      + path_splitter + "my.cnf";
  execute("dba.stop_sandbox_instance(" + _mysql_sandbox_port1 + ", " + stop_options + ")");
  add_to_cfg_file(cfgpath1, "binlog-do-db=db1,mysql_innodb_cluster_metadata,db2");
  execute("try_restart_sandbox(" + _mysql_sandbox_port1 + ")");
  std::string cfgpath2 = _sandbox_dir + path_splitter + _mysql_sandbox_port2
      + path_splitter + "my.cnf";
  execute("dba.stop_sandbox_instance(" + _mysql_sandbox_port2 + ", " + stop_options + ")");
  add_to_cfg_file(cfgpath2, "binlog-do-db=db1,db2");
  execute("try_restart_sandbox(" + _mysql_sandbox_port2 + ")");
  std::string cfgpath3 = _sandbox_dir + path_splitter + _mysql_sandbox_port3
      + path_splitter + "my.cnf";
  execute("dba.stop_sandbox_instance(" + _mysql_sandbox_port3 + ", " + stop_options + ")");
  add_to_cfg_file(cfgpath3, "binlog-ignore-db=db1,mysql_innodb_cluster_metadata,db2");
  execute("try_restart_sandbox(" + _mysql_sandbox_port3 + ")");

  // Validate test script.
  validate_interactive("dba_rpl_filter_check_no_interactive.py");

  // Restart sandbox instances without specific binlog filtering option.
  execute("dba.stop_sandbox_instance(" + _mysql_sandbox_port1 + ", " + stop_options + ")");
  remove_from_cfg_file(cfgpath1, "binlog-do-db");
  execute("try_restart_sandbox(" + _mysql_sandbox_port1 + ")");
  execute("dba.stop_sandbox_instance(" + _mysql_sandbox_port2 + ", " + stop_options + ")");
  remove_from_cfg_file(cfgpath2, "binlog-do-db");
  execute("try_restart_sandbox(" + _mysql_sandbox_port2 + ")");
  execute("dba.stop_sandbox_instance(" + _mysql_sandbox_port3 + ", " + stop_options + ")");
  remove_from_cfg_file(cfgpath3, "binlog-ignore-db");
  execute("try_restart_sandbox(" + _mysql_sandbox_port3 + ")");

  // Clean deployed sandbox.
  execute("cleanup_or_reset_sandbox(" + _mysql_sandbox_port1 + ", deployed1)");
  execute("cleanup_or_reset_sandbox(" + _mysql_sandbox_port2 + ", deployed2)");
  execute("cleanup_or_reset_sandbox(" + _mysql_sandbox_port3 + ", deployed3)");
}

TEST_F(Shell_py_dba_tests, no_interactive_delete_instances) {
  _options->wizards = false;
  reset_shell();

  enable_debug();

  // Execute setup script to be able to use smart deployment functions.
  execute_setup();

  execute("cleanup_sandboxes(True)");
}
}
