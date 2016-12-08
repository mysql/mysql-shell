/*
* Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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
    std::string protocol, user, password, host, sock, schema, ssl_ca, ssl_cert, ssl_key;
    shcore::parse_mysql_connstring(_uri, protocol, user, password, host, port, sock, schema, pwd_found, ssl_ca, ssl_cert, ssl_key);
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
    session = mysqlsh::connect_session(session_args, mysqlsh::SessionType::Classic);
    classic = dynamic_cast<mysqlsh::mysql::ClassicSession*>(session.get());
    mysqlsh::dba::get_server_variable(classic->connection(), "have_ssl",
                                      have_ssl);
    std::transform(have_ssl.begin(), have_ssl.end(), have_ssl.begin(), toupper);
    _have_ssl = (have_ssl.compare("YES") == 0) ? true : false;
    shcore::Argument_list args;
    classic->close(args);
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

    if (!_have_ssl) {
      code = "add_instance_options['memberSsl'] = false;";
      exec_and_out_equals(code);
    }

#ifdef _WIN32
    code = "var __path_splitter = '\\\\';";
    exec_and_out_equals(code);
    auto tokens = shcore::split_string(_sandbox_dir, "\\");
    if (!tokens.at(tokens.size() - 1).empty()) {
      tokens.push_back("");
      tokens.push_back("");
    }

    _sandbox_dir = shcore::join_strings(tokens, "\\\\");
    code = "var __sandbox_dir = '" + _sandbox_dir + "';";
    exec_and_out_equals(code);

    // output sandbox dir
    code = "var __output_sandbox_dir = '" + shcore::join_strings(tokens, "\\") + "';";
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

  void clean_and_deploy() {
    if (_sandbox_dir.empty()) {
      execute("dba.stopSandboxInstance(" + _mysql_sandbox_port1 + ");");
      execute("dba.deleteSandboxInstance(" + _mysql_sandbox_port1 + ");");
      execute("dba.stopSandboxInstance(" + _mysql_sandbox_port2 + ");");
      execute("dba.deleteSandboxInstance(" + _mysql_sandbox_port2 + ");");
      execute("dba.stopSandboxInstance(" + _mysql_sandbox_port3 + ");");
      execute("dba.deleteSandboxInstance(" + _mysql_sandbox_port3 + ");");
    } else {
      execute("dba.stopSandboxInstance(" + _mysql_sandbox_port1 + ", {sandboxDir: \"" + _sandbox_dir + "\"});");
      execute("dba.deleteSandboxInstance(" + _mysql_sandbox_port1 + ", {sandboxDir: \"" + _sandbox_dir + "\"});");
      execute("dba.stopSandboxInstance(" + _mysql_sandbox_port2 + ", {sandboxDir: \"" + _sandbox_dir + "\"});");
      execute("dba.deleteSandboxInstance(" + _mysql_sandbox_port2 + ", {sandboxDir: \"" + _sandbox_dir + "\"});");
      execute("dba.stopSandboxInstance(" + _mysql_sandbox_port3 + ", {sandboxDir: \"" + _sandbox_dir + "\"});");
      execute("dba.deleteSandboxInstance(" + _mysql_sandbox_port3 + ", {sandboxDir: \"" + _sandbox_dir + "\"});");
    }

    std::string deploy_options = "{password: \"root\", allowRootFrom: '%'";
    if (!_sandbox_dir.empty())
      deploy_options.append(", sandboxDir: \"" + _sandbox_dir + "\"");
    if (!_have_ssl)
      deploy_options.append(", ignoreSslError: true");
    deploy_options.append("}");

    execute("dba.deploySandboxInstance(" + _mysql_sandbox_port1 + ", "
            + deploy_options + ");");
    execute("dba.deploySandboxInstance(" + _mysql_sandbox_port2 + ", "
            + deploy_options + ");");
    execute("dba.deploySandboxInstance(" + _mysql_sandbox_port3 + ", "
            + deploy_options + ");");
  }
};

TEST_F(Shell_js_dba_tests, no_interactive_deploy_instances) {
  _options->wizards = false;
  reset_shell();

  execute("dba.verbose = true;");

  std::string deploy_options = "{password: \"root\", allowRootFrom: '%'";
  if (!_sandbox_dir.empty())
    deploy_options.append(", sandboxDir: '" + _sandbox_dir + "'");
  if (!_have_ssl)
    deploy_options.append(", ignoreSslError: true");
  deploy_options.append("}");

  execute("dba.deploySandboxInstance(" + _mysql_sandbox_port1 + ", "
              + deploy_options + ");");

  execute("dba.deploySandboxInstance(" + _mysql_sandbox_port2 + ", "
              + deploy_options + ");");
  execute("dba.deploySandboxInstance(" + _mysql_sandbox_port3 + ", "
              + deploy_options + ");");
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
  std::string bad_config = "[mysqld]\ngtid_mode = OFF\n";
  create_file("mybad.cnf", bad_config);

  execute("\\connect -c root:root@localhost:" + _mysql_sandbox_port1 + "");

  //@# Dba: checkInstanceConfig error
  output_handler.passwords.push_back("root");

  //@<OUT> Dba: checkInstanceConfig ok 1
  output_handler.passwords.push_back("root");

  //@<OUT> Dba: checkInstanceConfig report with errors
  output_handler.passwords.push_back("root");

  //@<OUT> Dba: configLocalInstance error 2
  output_handler.passwords.push_back(_pwd);
  output_handler.prompts.push_back("");

  //@<OUT> Dba: configLocalInstance error 3
  output_handler.passwords.push_back("root");
  output_handler.prompts.push_back("mybad.cnf");

  //@<OUT> Dba: configLocalInstance updating config file
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

TEST_F(Shell_js_dba_tests, interactive_drop_metadata_schema) {
  //@# drop metadata: no user response
  output_handler.prompts.push_back("");

  //@# drop metadata: user response no
  output_handler.prompts.push_back("n");

  //@# drop metadata: user response yes
  output_handler.prompts.push_back("y");

  validate_interactive("dba_drop_metadata_interactive.js");
}

TEST_F(Shell_js_dba_tests, no_interactive_delete_instances) {
  _options->wizards = false;
  reset_shell();

  if (_sandbox_dir.empty()) {
    execute("dba.stopSandboxInstance(" + _mysql_sandbox_port1 + ");");
    execute("dba.deleteSandboxInstance(" + _mysql_sandbox_port1 + ");");
    execute("dba.stopSandboxInstance(" + _mysql_sandbox_port2 + ");");
    execute("dba.deleteSandboxInstance(" + _mysql_sandbox_port2 + ");");
    execute("dba.stopSandboxInstance(" + _mysql_sandbox_port3 + ");");
    execute("dba.deleteSandboxInstance(" + _mysql_sandbox_port3 + ");");
  } else {
    execute("dba.stopSandboxInstance(" + _mysql_sandbox_port1 + ", {sandboxDir: \"" + _sandbox_dir + "\"});");
    execute("dba.deleteSandboxInstance(" + _mysql_sandbox_port1 + ", {sandboxDir: \"" + _sandbox_dir + "\"});");
    execute("dba.stopSandboxInstance(" + _mysql_sandbox_port2 + ", {sandboxDir: \"" + _sandbox_dir + "\"});");
    execute("dba.deleteSandboxInstance(" + _mysql_sandbox_port2 + ", {sandboxDir: \"" + _sandbox_dir + "\"});");
    execute("dba.stopSandboxInstance(" + _mysql_sandbox_port3 + ", {sandboxDir: \"" + _sandbox_dir + "\"});");
    execute("dba.deleteSandboxInstance(" + _mysql_sandbox_port3 + ", {sandboxDir: \"" + _sandbox_dir + "\"});");
  }
}
}
