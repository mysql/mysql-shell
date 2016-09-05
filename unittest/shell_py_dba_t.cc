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
  class Shell_py_dba_tests : public Shell_py_script_tester
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_py_script_tester::SetUp();

      // All of the test cases share the same config folder
      // and setup script
      set_config_folder("py_devapi");
      set_setup_script("setup.py");
    }

    virtual void set_defaults()
    {
      Shell_py_script_tester::set_defaults();

      int port = 33060, pwd_found;
      std::string protocol, user, password, host, sock, schema, ssl_ca, ssl_cert, ssl_key;
      shcore::parse_mysql_connstring(_uri, protocol, user, password, host, port, sock, schema, pwd_found, ssl_ca, ssl_cert, ssl_key);

      if (_port.empty())
        _port = "33060";

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
        code = "__mysql_sandbox_port1 = " + _mysql_sandbox_port1 + ";";
        exec_and_out_equals(code);
        code = "__mysql_sandbox_port2 = " + _mysql_sandbox_port2 + ";";
        exec_and_out_equals(code);
        code = "__mysql_sandbox_port3 = " + _mysql_sandbox_port3 + ";";
        exec_and_out_equals(code);
      }

      code = "__uripwd = '" + user + ":" + password + "@" + host + ":" + _port + "';";
      exec_and_out_equals(code);
      code = "__displayuri = '" + user + "@" + host + ":" + _port + "';";
      exec_and_out_equals(code);
      code = "__displayuridb = '" + user + "@" + host + ":" + _port + "/mysql';";
      exec_and_out_equals(code);
      code = "import os";
    }
  };

/*
  TEST_F(Shell_py_dba_tests, admin_no_interactive_global_session_x)
  {
    _options->wizards = false;
    reset_shell();

    execute("\\connect " + _uri);
    validate_interactive("dba_no_interactive.py");
    execute("session.close()");
  }

  TEST_F(Shell_py_dba_tests, admin_no_interactive_global_session_node)
  {
    _options->wizards = false;
    reset_shell();

    execute("\\connect -n " + _uri);
    validate_interactive("dba_no_interactive.py");
    execute("session.close()");
  }

*/

  TEST_F(Shell_py_dba_tests, admin_no_interactive_global_session_classic)
  {
    _options->wizards = false;
    reset_shell();

    if (_sandbox_dir.empty())
      execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'verbose': True});");
    else
      execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");

    execute("\\connect -c root:root@127.0.0.1:" + _mysql_sandbox_port1 + "");
    validate_interactive("dba_no_interactive.py");
    execute("session.close()");

    if (_sandbox_dir.empty())
    {
      execute("dba.kill_local_instance(" + _mysql_sandbox_port1 + ");");
      execute("dba.delete_local_instance(" + _mysql_sandbox_port1+ ");");
    }
    else
    {
      execute("dba.kill_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
      execute("dba.delete_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
    }
  }

/*

  TEST_F(Shell_py_dba_tests, admin_no_interactive_custom_session_x)
  {
    _options->wizards = false;
    reset_shell();

    execute("import mysqlx");
    execute("mySession = mysqlx.get_session('" + _uri + "')");
    execute("dba.reset_session(mySession)");
    validate_interactive("dba_no_interactive.py");
    execute("mySession.close()");
  }

  TEST_F(Shell_py_dba_tests, admin_no_interactive_custom_session_node)
  {
    _options->wizards = false;
    reset_shell();

    execute("import mysqlx");
    execute("mySession = mysqlx.get_node_session('" + _uri + "')");
    execute("dba.reset_session(mySession)");
    validate_interactive("dba_no_interactive.py");
    execute("mySession.close()");
  }

*/

  TEST_F(Shell_py_dba_tests, admin_no_interactive_custom_session_classic)
  {
    _options->wizards = false;
    reset_shell();

    if (_sandbox_dir.empty())
      execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'verbose': True});");
    else
      execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");

    execute("import mysql");
    execute("mySession = mysql.get_classic_session('root:root@127.0.0.1:" + _mysql_sandbox_port1 +"');");
    execute("dba.reset_session(mySession)");
    validate_interactive("dba_no_interactive.py");
    execute("mySession.close()");

    if (_sandbox_dir.empty())
    {
      execute("dba.kill_local_instance(" + _mysql_sandbox_port1 + ");");
      execute("dba.delete_local_instance(" + _mysql_sandbox_port1+ ");");
    }
    else
    {
      execute("dba.kill_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
      execute("dba.delete_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
    }
  }

/*

  TEST_F(Shell_py_dba_tests, admin_interactive_custom_session_x)
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

    execute("import mysqlx");
    execute("mySession = mysqlx.get_session('" + _uri + "')");
    execute("dba.reset_session(mySession)");
    validate_interactive("dba_interactive.py");
    execute("mySession.close()");
  }

  TEST_F(Shell_py_dba_tests, admin_interactive_custom_session_node)
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

    execute("import mysqlx");
    execute("mySession = mysqlx.get_node_session('" + _uri + "')");
    execute("dba.reset_session(mySession)");
    validate_interactive("dba_interactive.py");
    execute("mySession.close()");
  }

*/

  TEST_F(Shell_py_dba_tests, admin_interactive_custom_session_classic)
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

    if (_sandbox_dir.empty())
      execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'verbose': True});");
    else
      execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");

    execute("import mysql");
    execute("mySession = mysql.get_classic_session('root:root@127.0.0.1:" + _mysql_sandbox_port1 +"');");
    execute("dba.reset_session(mySession)");
    validate_interactive("dba_interactive.py");
    execute("mySession.close()");

    if (_sandbox_dir.empty())
    {
      execute("dba.kill_local_instance(" + _mysql_sandbox_port1 + ");");
      execute("dba.delete_local_instance(" + _mysql_sandbox_port1+ ");");
    }
    else
    {
      execute("dba.kill_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
      execute("dba.delete_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
    }
  }

/*

  TEST_F(Shell_py_dba_tests, admin_interactive_global_session_x)
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
    validate_interactive("dba_interactive.py");
    execute("mySession.close()");
  }

  TEST_F(Shell_py_dba_tests, admin_interactive_global_session_node)
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
    validate_interactive("dba_interactive.py");
    execute("mySession.close()");
  }

*/

  TEST_F(Shell_py_dba_tests, admin_interactive_global_session_classic)
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

    if (_sandbox_dir.empty())
      execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'verbose': True});");
    else
      execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");

    execute("\\connect -c root:root@127.0.0.1:" + _mysql_sandbox_port1 + "");
    validate_interactive("dba_interactive.py");

    if (_sandbox_dir.empty())
    {
      execute("dba.kill_local_instance(" + _mysql_sandbox_port1 + ");");
      execute("dba.delete_local_instance(" + _mysql_sandbox_port1+ ");");
    }
    else
    {
      execute("dba.kill_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
      execute("dba.delete_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
    }
  }

/*

  TEST_F(Shell_py_dba_tests, farm_no_interactive_global_session_x)
  {
    _options->wizards = false;
    reset_shell();

    execute("\\connect " + _uri);
    validate_interactive("dba_farm_no_interactive.py");
    execute("session.close()");
  }

  TEST_F(Shell_py_dba_tests, farm_no_interactive_global_session_node)
  {
    _options->wizards = false;
    reset_shell();

    execute("\\connect -n " + _uri);
    validate_interactive("dba_farm_no_interactive.py");
    execute("session.close()");
  }

*/

  TEST_F(Shell_py_dba_tests, cluster_no_interactive_global_session_classic)
  {
    _options->wizards = false;
    reset_shell();

    if (_sandbox_dir.empty())
      execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'verbose': True});");
    else
      execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");

    execute("\\connect -c root:root@127.0.0.1:" + _mysql_sandbox_port1 + "");
    validate_interactive("dba_cluster_no_interactive.py");
    execute("session.close()");

    if (_sandbox_dir.empty())
    {
      execute("dba.kill_local_instance(" + _mysql_sandbox_port1 + ");");
      execute("dba.delete_local_instance(" + _mysql_sandbox_port1+ ");");
    }
    else
    {
      execute("dba.kill_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
      execute("dba.delete_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
    }
  }

/*

  TEST_F(Shell_py_dba_tests, farm_no_interactive_custom_session_x)
  {
    _options->wizards = false;
    reset_shell();

    execute("import mysqlx");
    execute("mySession = mysqlx.get_session('" + _uri + "')");
    execute("dba.reset_session(mySession)");
    validate_interactive("dba_farm_no_interactive.py");
    execute("mySession.close()");
  }

  TEST_F(Shell_py_dba_tests, farm_no_interactive_custom_session_node)
  {
    _options->wizards = false;
    reset_shell();

    execute("import mysqlx");
    execute("mySession = mysqlx.get_node_session('" + _uri + "')");
    execute("dba.reset_session(mySession)");
    validate_interactive("dba_farm_no_interactive.py");
    execute("mySession.close()");
  }

*/

  TEST_F(Shell_py_dba_tests, cluster_no_interactive_custom_session_classic)
  {
    _options->wizards = false;
    reset_shell();

    if (_sandbox_dir.empty())
      execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'verbose': True});");
    else
      execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");

    execute("import mysql");
    execute("mySession = mysql.get_classic_session('root:root@127.0.0.1:" + _mysql_sandbox_port1 +"');");
    execute("dba.reset_session(mySession)");
    validate_interactive("dba_cluster_no_interactive.py");
    execute("mySession.close()");

    if (_sandbox_dir.empty())
    {
      execute("dba.kill_local_instance(" + _mysql_sandbox_port1 + ");");
      execute("dba.delete_local_instance(" + _mysql_sandbox_port1+ ");");
    }
    else
    {
      execute("dba.kill_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
      execute("dba.delete_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
    }
  }

  //TEST_F(Shell_py_dba_tests, farm_interactive_custom_session_x)
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

  //  execute("import mysqlx");
  //  execute("mySession = mysqlx.get_session('" + _uri + "')");
  //  execute("dba.reset_session(mySession)");
  //  validate_interactive("dba_farm_interactive.py");
  //  execute("mySession.close()");
  //}

  //TEST_F(Shell_py_dba_tests, farm_interactive_custom_session_node)
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

  //  execute("import mysqlx");
  //  execute("mySession = mysqlx.get_node_session('" + _uri + "')");
  //  execute("dba.reset_session(mySession)");
  //  validate_interactive("dba_farm_interactive.py");
  //  execute("mySession.close()");
  //}

  //TEST_F(Shell_py_dba_tests, farm_interactive_custom_session_classic)
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

  //  execute("mysql = require('mysql')");
  //  execute("mySession = mysql.get_classic_session('" + _mysql_uri + "')");
  //  execute("dba.reset_session(mySession)");
  //  validate_interactive("dba_farm_interactive.py");
  //  execute("mySession.close()");
  //}

  //TEST_F(Shell_py_dba_tests, farm_interactive_global_session_x)
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
  //  validate_interactive("dba_farm_interactive.py");
  //  execute("mySession.close()");
  //}

  //TEST_F(Shell_py_dba_tests, farm_interactive_global_session_node)
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
  //  validate_interactive("dba_farm_interactive.py");
  //  execute("mySession.close()");
  //}

  //TEST_F(Shell_py_dba_tests, farm_interactive_global_session_classic)
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
  //  validate_interactive("dba_farm_interactive.py");
  //}

/*

  TEST_F(Shell_py_dba_tests, replica_set_no_interactive_global_session_x)
  {
    _options->wizards = false;
    reset_shell();

    execute("\\connect " + _uri);
    validate_interactive("dba_replica_set_no_interactive.py");
    execute("session.close()");
  }

  TEST_F(Shell_py_dba_tests, replica_set_no_interactive_global_session_node)
  {
    _options->wizards = false;
    reset_shell();

    execute("\\connect -n " + _uri);
    validate_interactive("dba_replica_set_no_interactive.py");
    execute("session.close()");
  }

*/

  TEST_F(Shell_py_dba_tests, replica_set_no_interactive_global_session_classic)
  {
    _options->wizards = false;
    reset_shell();

    if (_sandbox_dir.empty())
      execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'verbose': True});");
    else
      execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");

    execute("\\connect -c root:root@127.0.0.1:" + _mysql_sandbox_port1 + "");
    validate_interactive("dba_replica_set_no_interactive.py");
    execute("session.close()");

    if (_sandbox_dir.empty())
    {
      execute("dba.kill_local_instance(" + _mysql_sandbox_port1 + ");");
      execute("dba.delete_local_instance(" + _mysql_sandbox_port1+ ");");
    }
    else
    {
      execute("dba.kill_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
      execute("dba.delete_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
    }
  }

/*

  TEST_F(Shell_py_dba_tests, replica_set_no_interactive_custom_session_x)
  {
    _options->wizards = false;
    reset_shell();

    execute("import mysqlx");
    execute("mySession = mysqlx.get_session('" + _uri + "')");
    execute("dba.reset_session(mySession)");
    validate_interactive("dba_replica_set_no_interactive.py");
    execute("mySession.close()");
  }

  TEST_F(Shell_py_dba_tests, replica_set_no_interactive_custom_session_node)
  {
    _options->wizards = false;
    reset_shell();

    execute("import mysqlx");
    execute("mySession = mysqlx.get_node_session('" + _uri + "')");
    execute("dba.reset_session(mySession)");
    validate_interactive("dba_replica_set_no_interactive.py");
    execute("mySession.close()");
  }

*/

  TEST_F(Shell_py_dba_tests, replica_set_no_interactive_custom_session_classic)
  {
    _options->wizards = false;
    reset_shell();

    if (_sandbox_dir.empty())
      execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'verbose': True});");
    else
      execute("dba.deploy_local_instance(" + _mysql_sandbox_port1 + ", {'password': 'root', 'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");

    execute("import mysql");
    execute("mySession = mysql.get_classic_session('root:root@127.0.0.1:" + _mysql_sandbox_port1 +"');");
    execute("dba.reset_session(mySession)");
    validate_interactive("dba_replica_set_no_interactive.py");
    execute("mySession.close()");

    if (_sandbox_dir.empty())
    {
      execute("dba.kill_local_instance(" + _mysql_sandbox_port1 + ");");
      execute("dba.delete_local_instance(" + _mysql_sandbox_port1+ ");");
    }
    else
    {
      execute("dba.kill_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
      execute("dba.delete_local_instance(" + _mysql_sandbox_port1 + ", {'sandboxDir': '" + _sandbox_dir + "', 'verbose': True});");
    }
  }
}