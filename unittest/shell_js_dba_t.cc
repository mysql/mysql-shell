/*
* Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

namespace shcore
{
  class Shell_js_dba_tests : public Shell_js_script_tester
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_js_script_tester::SetUp();

      // All of the test cases share the same config folder
      // and setup script
      set_config_folder("js_devapi");
      set_setup_script("setup.js");
    }

    virtual void set_defaults()
    {
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
      if (_mysql_port.empty())
      {
        code = "__host_port = '" + host + ":3306';";
        exec_and_out_equals(code);
        code = "__mysql_port = 3306;";
        exec_and_out_equals(code);
      }
      else
      {
        code = "__host_port = '" + host + ":" + _mysql_port + "';";
        exec_and_out_equals(code);
        code = "__mysql_port = " + _mysql_port + ";";
        exec_and_out_equals(code);
        code = "var __mysql_port_adminapi = " + std::to_string(atoi(_mysql_port.c_str()) + 10) + ";";
        exec_and_out_equals(code);
      }
      code = "var __uripwd = '" + user + ":" + password + "@" + host + ":" + _port + "';";
      exec_and_out_equals(code);
      code = "var __displayuri = '" + user + "@" + host + ":" + _port + "';";
      exec_and_out_equals(code);
      code = "var __displayuridb = '" + user + "@" + host + ":" + _port + "/mysql';";
      exec_and_out_equals(code);
    }
  };
/*
  TEST_F(Shell_js_dba_tests, admin_no_interactive_global_session_x)
  {
    _options->wizards = false;
    reset_shell();

    execute("\\connect " + _uri);
    validate_interactive("dba_no_interactive.js");
    execute("session.close();");
  }

  TEST_F(Shell_js_dba_tests, admin_no_interactive_global_session_node)
  {
    _options->wizards = false;
    reset_shell();

    execute("\\connect -n " + _uri);
    validate_interactive("dba_no_interactive.js");
    execute("session.close();");
  }
*/
  TEST_F(Shell_js_dba_tests, dba_no_interactive_global_session_classic)
  {
    _options->wizards = false;
    reset_shell();

    execute("dba.deployLocalInstance(" + _mysql_port_adminapi + ", {password: \"root\"});");
    execute("\\connect -c root:root@localhost:" + _mysql_port_adminapi + "");
    validate_interactive("dba_no_interactive.js");
    execute("session.close();");
    execute("dba.killLocalInstance(" + _mysql_port_adminapi + ");");
    execute("dba.deleteLocalInstance(" + _mysql_port_adminapi+ ");");
  }

/*
  TEST_F(Shell_js_dba_tests, admin_no_interactive_custom_session_x)
  {
    _options->wizards = false;
    reset_shell();

    execute("var mysqlx = require('mysqlx');");
    execute("var mySession = mysqlx.getSession('" + _uri + "');");
    execute("dba.resetSession(mySession);");
    validate_interactive("dba_no_interactive.js");
    execute("mySession.close();");
  }

  TEST_F(Shell_js_dba_tests, admin_no_interactive_custom_session_node)
  {
    _options->wizards = false;
    reset_shell();

    execute("var mysqlx = require('mysqlx');");
    execute("var mySession = mysqlx.getNodeSession('" + _uri + "');");
    execute("dba.resetSession(mySession);");
    validate_interactive("dba_no_interactive.js");
    execute("mySession.close();");
  }
*/

  TEST_F(Shell_js_dba_tests, dba_no_interactive_custom_session_classic)
  {
    _options->wizards = false;
    reset_shell();

    execute("dba.deployLocalInstance(" + _mysql_port_adminapi + ", {password: \"root\"});");
    execute("var mysql = require('mysql');");
    execute("var mySession = mysql.getClassicSession('root:root@localhost:" + _mysql_port_adminapi +"');");
    execute("dba.resetSession(mySession);");
    validate_interactive("dba_no_interactive.js");
    execute("mySession.close();");
    execute("dba.killLocalInstance(" + _mysql_port_adminapi + ");");
    execute("dba.deleteLocalInstance(" + _mysql_port_adminapi+ ");");
  }
/*
  TEST_F(Shell_js_dba_tests, admin_interactive_custom_session_x)
  {
    // Fills the required prompts and passwords...
    //@ Initialization
    output_handler.prompts.push_back("y");

    //@# Dba: createFarm with interaction
    output_handler.passwords.push_back("testing");

    //@ Dba: dropFarm interaction no options, cancel
    output_handler.passwords.push_back("n");

    //@ Dba: dropFarm interaction missing option, ok error
    output_handler.passwords.push_back("y");

    //@ Dba: dropFarm interaction no options, ok success
    output_handler.passwords.push_back("y");

    execute("var mysqlx = require('mysqlx');");
    execute("var mySession = mysqlx.getSession('" + _uri + "');");
    execute("dba.resetSession(mySession);");
    validate_interactive("dba_interactive.js");
    execute("mySession.close();");
  }

  TEST_F(Shell_js_dba_tests, admin_interactive_custom_session_node)
  {
    // Fills the required prompts and passwords...
    //@ Initialization
    output_handler.prompts.push_back("y");

    //@# Dba: createFarm with interaction
    output_handler.passwords.push_back("testing");

    //@ Dba: dropFarm interaction no options, cancel
    output_handler.passwords.push_back("n");

    //@ Dba: dropFarm interaction missing option, ok error
    output_handler.passwords.push_back("y");

    //@ Dba: dropFarm interaction no options, ok success
    output_handler.passwords.push_back("y");

    execute("var mysqlx = require('mysqlx');");
    execute("var mySession = mysqlx.getNodeSession('" + _uri + "');");
    execute("dba.resetSession(mySession);");
    validate_interactive("dba_interactive.js");
    execute("mySession.close();");
  }
*/

  TEST_F(Shell_js_dba_tests, dba_interactive_custom_session_classic)
  {
    // Fills the required prompts and passwords...
    //@ Initialization
    output_handler.prompts.push_back("y");

    //@# Dba: createFarm with interaction
    output_handler.passwords.push_back("testing");

    //@ Dba: dropFarm interaction no options, cancel
    output_handler.passwords.push_back("n");

    //@ Dba: dropFarm interaction missing option, ok error
    output_handler.passwords.push_back("y");

    //@ Dba: dropFarm interaction no options, ok success
    output_handler.passwords.push_back("y");

    execute("dba.deployLocalInstance(" + _mysql_port_adminapi + ", {password: \"root\"});");
    execute("var mysql = require('mysql');");
    execute("var mySession = mysql.getClassicSession('root:root@localhost:" + _mysql_port_adminapi +"');");
    execute("dba.resetSession(mySession);");
    validate_interactive("dba_interactive.js");
    execute("mySession.close();");
    execute("dba.killLocalInstance(" + _mysql_port_adminapi + ");");
    execute("dba.deleteLocalInstance(" + _mysql_port_adminapi+ ");");
  }

/*
  TEST_F(Shell_js_dba_tests, admin_interactive_global_session_x)
  {
    // Fills the required prompts and passwords...
    //@ Initialization
    output_handler.prompts.push_back("y");

    //@# Dba: createFarm with interaction
    output_handler.passwords.push_back("testing");

    //@ Dba: dropFarm interaction no options, cancel
    output_handler.passwords.push_back("n");

    //@ Dba: dropFarm interaction missing option, ok error
    output_handler.passwords.push_back("y");

    //@ Dba: dropFarm interaction no options, ok success
    output_handler.passwords.push_back("y");

    execute("\\connect " + _uri);
    validate_interactive("dba_interactive.js");
    execute("mySession.close();");
  }

  TEST_F(Shell_js_dba_tests, admin_interactive_global_session_node)
  {
    // Fills the required prompts and passwords...
    //@ Initialization
    output_handler.prompts.push_back("y");

    //@# Dba: createFarm with interaction
    output_handler.passwords.push_back("testing");

    //@ Dba: dropFarm interaction no options, cancel
    output_handler.passwords.push_back("n");

    //@ Dba: dropFarm interaction missing option, ok error
    output_handler.passwords.push_back("y");

    //@ Dba: dropFarm interaction no options, ok success
    output_handler.passwords.push_back("y");

    execute("\\connect -n " + _uri);
    validate_interactive("dba_interactive.js");
    execute("mySession.close();");
  }

*/

  TEST_F(Shell_js_dba_tests, dba_interactive_global_session_classic)
  {
    // Fills the required prompts and passwords...
    //@ Initialization
    output_handler.prompts.push_back("y");

    //@# Dba: createFarm with interaction
    output_handler.passwords.push_back("testing");

    //@ Dba: dropFarm interaction no options, cancel
    output_handler.passwords.push_back("n");

    //@ Dba: dropFarm interaction missing option, ok error
    output_handler.passwords.push_back("y");

    //@ Dba: dropFarm interaction no options, ok success
    output_handler.passwords.push_back("y");

    execute("dba.deployLocalInstance(" + _mysql_port_adminapi + ", {password: \"root\"});");
    execute("\\connect -c root:root@localhost:" + _mysql_port_adminapi + "");
    validate_interactive("dba_interactive.js");
    execute("dba.killLocalInstance(" + _mysql_port_adminapi + ");");
    execute("dba.deleteLocalInstance(" + _mysql_port_adminapi + ");");
  }

/*

  TEST_F(Shell_js_dba_tests, farm_no_interactive_global_session_x)
  {
    _options->wizards = false;
    reset_shell();

    execute("\\connect " + _uri);
    validate_interactive("dba_farm_no_interactive.js");
    execute("session.close();");
  }

  TEST_F(Shell_js_dba_tests, farm_no_interactive_global_session_node)
  {
    _options->wizards = false;
    reset_shell();

    execute("\\connect -n " + _uri);
    validate_interactive("dba_farm_no_interactive.js");
    execute("session.close();");
  }

*/

  TEST_F(Shell_js_dba_tests, cluster_no_interactive_global_session_classic)
  {
    _options->wizards = false;
    reset_shell();

    execute("dba.deployLocalInstance(" + _mysql_port_adminapi + ", {password: \"root\"});");
    execute("\\connect -c root:root@localhost:" + _mysql_port_adminapi + "");
    validate_interactive("dba_cluster_no_interactive.js");
    execute("session.close();");
    execute("dba.killLocalInstance(" + _mysql_port_adminapi + ");");
    execute("dba.deleteLocalInstance(" + _mysql_port_adminapi + ");");
  }

/*

  TEST_F(Shell_js_dba_tests, farm_no_interactive_custom_session_x)
  {
    _options->wizards = false;
    reset_shell();

    execute("var mysqlx = require('mysqlx');");
    execute("var mySession = mysqlx.getSession('" + _uri + "');");
    execute("dba.resetSession(mySession);");
    validate_interactive("dba_farm_no_interactive.js");
    execute("mySession.close();");
  }

  TEST_F(Shell_js_dba_tests, farm_no_interactive_custom_session_node)
  {
    _options->wizards = false;
    reset_shell();

    execute("var mysqlx = require('mysqlx');");
    execute("var mySession = mysqlx.getNodeSession('" + _uri + "');");
    execute("dba.resetSession(mySession);");
    validate_interactive("dba_farm_no_interactive.js");
    execute("mySession.close();");
  }

*/

  TEST_F(Shell_js_dba_tests, cluster_no_interactive_custom_session_classic)
  {
    _options->wizards = false;
    reset_shell();

    execute("dba.deployLocalInstance(" + _mysql_port_adminapi + ", {password: \"root\"});");
    execute("var mysql = require('mysql');");
    execute("var mySession = mysql.getClassicSession('root:root@localhost:" + _mysql_port_adminapi +"');");
    execute("dba.resetSession(mySession);");
    validate_interactive("dba_cluster_no_interactive.js");
    execute("mySession.close();");
    execute("dba.killLocalInstance(" + _mysql_port_adminapi + ");");
    execute("dba.deleteLocalInstance(" + _mysql_port_adminapi + ");");
  }

  //TEST_F(Shell_js_dba_tests, farm_interactive_custom_session_x)
  //{
  //  // Fills the required prompts and passwords...

  //  //@ Farm: addInstance, no seed instance answer no
  //  output_handler.prompts.push_back("n");

  //  //@ Farm: addInstance, no seed instance answer yes
  //  output_handler.prompts.push_back("y");

  //  //@ Farm: addInstance, ignore invalid attributes no ignore
  //  output_handler.prompts.push_back("y");
  //  output_handler.prompts.push_back("n");

  //  //@ Farm: addInstance, ignore invalid attributes ignore
  //  output_handler.prompts.push_back("y");
  //  output_handler.prompts.push_back("y");

  //  //@ Farm: addSeedInstance, it already initialized, answer no
  //  output_handler.prompts.push_back("n");

  //  //@ Farm: addSeedInstance, it already initialized, answer yes
  //  output_handler.prompts.push_back("y");

  //  execute("var mysqlx = require('mysqlx');");
  //  execute("var mySession = mysqlx.getSession('" + _uri + "');");
  //  execute("dba.resetSession(mySession);");
  //  validate_interactive("dba_farm_interactive.js");
  //  execute("mySession.close();");
  //}

  //TEST_F(Shell_js_dba_tests, farm_interactive_custom_session_node)
  //{
  //  // Fills the required prompts and passwords...
  //  //@ Initialization
  //  output_handler.prompts.push_back("y");

  //  //@# Dba: createFarm with interaction
  //  output_handler.passwords.push_back("testing");

  //  //@ Dba: dropFarm interaction no options, cancel
  //  output_handler.passwords.push_back("n");

  //  //@ Dba: dropFarm interaction missing option, ok error
  //  output_handler.passwords.push_back("y");

  //  //@ Dba: dropFarm interaction no options, ok success
  //  output_handler.passwords.push_back("y");

  //  execute("var mysqlx = require('mysqlx');");
  //  execute("var mySession = mysqlx.getNodeSession('" + _uri + "');");
  //  execute("dba.resetSession(mySession);");
  //  validate_interactive("dba_farm_interactive.js");
  //  execute("mySession.close();");
  //}

  //TEST_F(Shell_js_dba_tests, farm_interactive_custom_session_classic)
  //{
  //  // Fills the required prompts and passwords...
  //  //@ Initialization
  //  output_handler.prompts.push_back("y");

  //  //@# Dba: createFarm with interaction
  //  output_handler.passwords.push_back("testing");

  //  //@ Dba: dropFarm interaction no options, cancel
  //  output_handler.passwords.push_back("n");

  //  //@ Dba: dropFarm interaction missing option, ok error
  //  output_handler.passwords.push_back("y");

  //  //@ Dba: dropFarm interaction no options, ok success
  //  output_handler.passwords.push_back("y");

  //  execute("var mysql = require('mysql');");
  //  execute("var mySession = mysql.getClassicSession('" + _mysql_uri + "');");
  //  execute("dba.resetSession(mySession);");
  //  validate_interactive("dba_farm_interactive.js");
  //  execute("mySession.close();");
  //}

  //TEST_F(Shell_js_dba_tests, farm_interactive_global_session_x)
  //{
  //  // Fills the required prompts and passwords...
  //  //@ Initialization
  //  output_handler.prompts.push_back("y");

  //  //@# Dba: createFarm with interaction
  //  output_handler.passwords.push_back("testing");

  //  //@ Dba: dropFarm interaction no options, cancel
  //  output_handler.passwords.push_back("n");

  //  //@ Dba: dropFarm interaction missing option, ok error
  //  output_handler.passwords.push_back("y");

  //  //@ Dba: dropFarm interaction no options, ok success
  //  output_handler.passwords.push_back("y");

  //  execute("\\connect " + _uri);
  //  validate_interactive("dba_farm_interactive.js");
  //  execute("mySession.close();");
  //}

  //TEST_F(Shell_js_dba_tests, farm_interactive_global_session_node)
  //{
  //  // Fills the required prompts and passwords...
  //  //@ Initialization
  //  output_handler.prompts.push_back("y");

  //  //@# Dba: createFarm with interaction
  //  output_handler.passwords.push_back("testing");

  //  //@ Dba: dropFarm interaction no options, cancel
  //  output_handler.passwords.push_back("n");

  //  //@ Dba: dropFarm interaction missing option, ok error
  //  output_handler.passwords.push_back("y");

  //  //@ Dba: dropFarm interaction no options, ok success
  //  output_handler.passwords.push_back("y");

  //  execute("\\connect -n " + _uri);
  //  validate_interactive("dba_farm_interactive.js");
  //  execute("mySession.close();");
  //}

  //TEST_F(Shell_js_dba_tests, farm_interactive_global_session_classic)
  //{
  //  // Fills the required prompts and passwords...
  //  //@ Initialization
  //  output_handler.prompts.push_back("y");

  //  //@# Dba: createFarm with interaction
  //  output_handler.passwords.push_back("testing");

  //  //@ Dba: dropFarm interaction no options, cancel
  //  output_handler.passwords.push_back("n");

  //  //@ Dba: dropFarm interaction missing option, ok error
  //  output_handler.passwords.push_back("y");

  //  //@ Dba: dropFarm interaction no options, ok success
  //  output_handler.passwords.push_back("y");

  //  execute("\\connect -c " + _mysql_uri);
  //  validate_interactive("dba_farm_interactive.js");
  //}

/*
  TEST_F(Shell_js_dba_tests, replica_set_no_interactive_global_session_x)
  {
    _options->wizards = false;
    reset_shell();

    execute("\\connect " + _uri);
    validate_interactive("dba_replica_set_no_interactive.js");
    execute("session.close();");
  }

  TEST_F(Shell_js_dba_tests, replica_set_no_interactive_global_session_node)
  {
    _options->wizards = false;
    reset_shell();

    execute("\\connect -n " + _uri);
    validate_interactive("dba_replica_set_no_interactive.js");
    execute("session.close();");
  }

*/

  TEST_F(Shell_js_dba_tests, replica_set_no_interactive_global_session_classic)
  {
    _options->wizards = false;
    reset_shell();

    execute("dba.deployLocalInstance(" + _mysql_port_adminapi + ", {password: \"root\"});");
    execute("var mySession = mysql.getClassicSession('root:root@localhost:" + _mysql_port_adminapi +"');");
    execute("\\connect -c root:root@localhost:" + _mysql_port_adminapi + "");
    validate_interactive("dba_replica_set_no_interactive.js");
    execute("session.close();");
    execute("dba.killLocalInstance(" + _mysql_port_adminapi + ");");
    execute("dba.deleteLocalInstance(" + _mysql_port_adminapi + ");");
  }

/*

  TEST_F(Shell_js_dba_tests, replica_set_no_interactive_custom_session_x)
  {
    _options->wizards = false;
    reset_shell();

    execute("var mysqlx = require('mysqlx');");
    execute("var mySession = mysqlx.getSession('" + _uri + "');");
    execute("dba.resetSession(mySession);");
    validate_interactive("dba_replica_set_no_interactive.js");
    execute("mySession.close();");
  }

  TEST_F(Shell_js_dba_tests, replica_set_no_interactive_custom_session_node)
  {
    _options->wizards = false;
    reset_shell();

    execute("var mysqlx = require('mysqlx');");
    execute("var mySession = mysqlx.getNodeSession('" + _uri + "');");
    execute("dba.resetSession(mySession);");
    validate_interactive("dba_replica_set_no_interactive.js");
    execute("mySession.close();");
  }

*/

  TEST_F(Shell_js_dba_tests, replica_set_no_interactive_custom_session_classic)
  {
    _options->wizards = false;
    reset_shell();

    execute("dba.deployLocalInstance(" + _mysql_port_adminapi + ", {password: \"root\"});");
    execute("var mysql = require('mysql');");
    execute("var mySession = mysql.getClassicSession('root:root@localhost:" + _mysql_port_adminapi +"');");
    execute("dba.resetSession(mySession);");
    validate_interactive("dba_replica_set_no_interactive.js");
    execute("mySession.close();");
    execute("dba.killLocalInstance(" + _mysql_port_adminapi + ");");
    execute("dba.deleteLocalInstance(" + _mysql_port_adminapi + ");");
  }
}
