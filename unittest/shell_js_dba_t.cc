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

#include "shell_script_tester.h"
#include "utils/utils_general.h"

#ifdef WIN32
#define strerror_r(errno,buf,len) strerror_s(buf,len,errno)
#endif

static std::string get_my_hostname() {
  char hostname[1024];
  if (gethostname(hostname, sizeof(hostname)) < 0) {
    char msg[1024];
    auto dummy = strerror_r(errno, msg, sizeof(msg));
    (void)dummy;
    log_error("Could not get hostname: %s", msg);
    throw std::runtime_error("Could not get local hostname");
  }
  return hostname;
}


namespace shcore {
class Shell_js_dba_tests : public Shell_js_script_tester {
protected:
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

    if (_port.empty())
      _port = "33060";

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
    }
    code = "var __uripwd = '" + user + ":" + password + "@" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "var __displayuri = '" + user + "@" + host + ":" + _port + "';";
    exec_and_out_equals(code);
    code = "var __displayuridb = '" + user + "@" + host + ":" + _port + "/mysql';";
    exec_and_out_equals(code);
    code = "var __hostname = '" + get_my_hostname() + "'";
    exec_and_out_equals(code);
  }
};

TEST_F(Shell_js_dba_tests, no_interactive_deploy_instances) {
  _options->wizards = false;
  reset_shell();

  execute("dba.verbose = true;");

  if (_sandbox_dir.empty()) {
    execute("dba.deploySandboxInstance(" + _mysql_sandbox_port1 + ", {password: \"root\"});");
    execute("dba.deploySandboxInstance(" + _mysql_sandbox_port2 + ", {password: \"root\"});");
  } else {
    execute("dba.deploySandboxInstance(" + _mysql_sandbox_port1 + ", {password: \"root\", sandboxDir: \"" + _sandbox_dir + "\"});");
    execute("dba.deploySandboxInstance(" + _mysql_sandbox_port2 + ", {password: \"root\", sandboxDir: \"" + _sandbox_dir + "\"});");
  }
}

TEST_F(Shell_js_dba_tests, no_interactive_classic_global_dba) {
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

TEST_F(Shell_js_dba_tests, no_interactive_classic_custom_dba) {
  _options->wizards = false;
  reset_shell();

  execute("var mysql = require('mysql');");
  execute("var mySession = mysql.getClassicSession('root:root@localhost:" + _mysql_sandbox_port1 + "');");
  execute("dba.resetSession(mySession);");

  // Validates error conditions on create, get and drop cluster
  // Lets the cluster created
  validate_interactive("dba_no_interactive.js");

  execute("mySession.close();");
}

TEST_F(Shell_js_dba_tests, no_interactive_classic_custom_cluster) {
  _options->wizards = false;
  reset_shell();

  execute("var mysql = require('mysql');");
  execute("var mySession = mysql.getClassicSession('root:root@localhost:" + _mysql_sandbox_port1 + "');");
  execute("dba.resetSession(mySession);");
  // Tests cluster functionality, adding, removing instances
  // error conditions
  // Lets the cluster empty
  validate_interactive("dba_cluster_no_interactive.js");

  execute("mySession.close();");
}

TEST_F(Shell_js_dba_tests, interactive_classic_global_dba) {
  execute("\\connect -c root:root@localhost:" + _mysql_sandbox_port1 + "");

  //@<OUT> Dba: createCluster with interaction
  output_handler.passwords.push_back("testing");

  //@<OUT> Dba: getCluster with interaction
  output_handler.passwords.push_back("testing");

  //@<OUT> Dba: getCluster with interaction (default)
  output_handler.passwords.push_back("testing");

  //@<OUT> Dba: dropCluster interaction no options, cancel
  output_handler.prompts.push_back("n");

  //@<OUT> Dba: dropCluster interaction missing option, ok error
  output_handler.prompts.push_back("y");

  // Validates error conditions on create, get and drop cluster
  // Lets the cluster created
  validate_interactive("dba_interactive.js");

  execute("session.close();");
}

TEST_F(Shell_js_dba_tests, interactive_classic_global_cluster) {
  execute("\\connect -c root:root@localhost:" + _mysql_sandbox_port1 + "");

  //@<OUT> Cluster: getCluster with interaction
  output_handler.passwords.push_back("testing");

  //@# Cluster: addInstance errors: missing host interactive, cancel
  output_handler.prompts.push_back("3");

  //@# Cluster: addInstance errors: invalid attributes, cancel
  output_handler.prompts.push_back("n");

  //@# Cluster: addInstance errors: missing host interactive, cancel 2
  output_handler.prompts.push_back("3");

  //@# Cluster: addInstance with interaction, error
  output_handler.passwords.push_back("root");

  //@<OUT> Cluster: addInstance with interaction, ok
  output_handler.passwords.push_back("root");

  //@<OUT> Cluster: addInstance with interaction, ok 2
  output_handler.passwords.push_back("root");

  //@<OUT> Cluster: addInstance with interaction, ok 3
  output_handler.passwords.push_back("root");

  // Tests cluster functionality, adding, removing instances
  // error conditions
  // Lets the cluster empty
  validate_interactive("dba_cluster_interactive.js");

  execute("session.close();");
}

TEST_F(Shell_js_dba_tests, interactive_custom_global_dba) {
  execute("var mysql = require('mysql');");
  execute("var mySession = mysql.getClassicSession('root:root@localhost:" + _mysql_sandbox_port1 + "');");
  execute("dba.resetSession(mySession);");

  //@<OUT> Dba: createCluster with interaction
  output_handler.passwords.push_back("testing");

  //@<OUT> Dba: getCluster with interaction
  output_handler.passwords.push_back("testing");

  //@<OUT> Dba: getCluster with interaction (default)
  output_handler.passwords.push_back("testing");

  //@<OUT> Dba: dropCluster interaction no options, cancel
  output_handler.prompts.push_back("n");

  //@<OUT> Dba: dropCluster interaction missing option, ok error
  output_handler.prompts.push_back("y");

  // Validates error conditions on create, get and drop cluster
  // Lets the cluster created
  validate_interactive("dba_interactive.js");

  execute("session.close();");
}

TEST_F(Shell_js_dba_tests, interactive_custom_global_cluster) {
  execute("var mysql = require('mysql');");
  execute("var mySession = mysql.getClassicSession('root:root@localhost:" + _mysql_sandbox_port1 + "');");
  execute("dba.resetSession(mySession);");

  //@<OUT> Cluster: getCluster with interaction
  output_handler.passwords.push_back("testing");

  //@# Cluster: addInstance errors: missing host interactive, cancel
  output_handler.prompts.push_back("3");

  //@# Cluster: addInstance errors: invalid attributes, cancel
  output_handler.prompts.push_back("n");

  //@# Cluster: addInstance errors: missing host interactive, cancel 2
  output_handler.prompts.push_back("3");

  //@# Cluster: addInstance with interaction, error
  output_handler.passwords.push_back("root");

  //@<OUT> Cluster: addInstance with interaction, ok
  output_handler.passwords.push_back("root");

  //@<OUT> Cluster: addInstance with interaction, ok 2
  output_handler.passwords.push_back("root");

  //@<OUT> Cluster: addInstance with interaction, ok 3
  output_handler.passwords.push_back("root");

  // Tests cluster functionality, adding, removing instances
  // error conditions
  // Lets the cluster empty
  validate_interactive("dba_cluster_interactive.js");

  execute("session.close();");
}

TEST_F(Shell_js_dba_tests, no_interactive_delete_instances) {
  _options->wizards = false;
  reset_shell();

  if (_sandbox_dir.empty()) {
    execute("dba.stopSandboxInstance(" + _mysql_sandbox_port1 + ");");
    execute("dba.deleteSandboxInstance(" + _mysql_sandbox_port1 + ");");
    execute("dba.stopSandboxInstance(" + _mysql_sandbox_port2 + ");");
    execute("dba.deleteSandboxInstance(" + _mysql_sandbox_port2 + ");");
  } else {
    execute("dba.stopSandboxInstance(" + _mysql_sandbox_port1 + ", {sandboxDir: \"" + _sandbox_dir + "\"});");
    execute("dba.deleteSandboxInstance(" + _mysql_sandbox_port1 + ", {sandboxDir: \"" + _sandbox_dir + "\"});");
    execute("dba.stopSandboxInstance(" + _mysql_sandbox_port2 + ", {sandboxDir: \"" + _sandbox_dir + "\"});");
    execute("dba.deleteSandboxInstance(" + _mysql_sandbox_port2 + ", {sandboxDir: \"" + _sandbox_dir + "\"});");
  }
}
}
