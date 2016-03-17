/* Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include "shell_script_tester.h"
#include "shellcore/server_registry.h"
#include "utils/utils_general.h"

namespace shcore
{
  class Shell_py_dev_api_sample_tester : public Shell_py_script_tester
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_py_script_tester::SetUp();

      int port = 33060, pwd_found;
      std::string protocol, user, password, host, sock, schema, ssl_ca, ssl_cert, ssl_key;
      shcore::parse_mysql_connstring(_uri, protocol, user, password, host, port, sock, schema, pwd_found, ssl_ca, ssl_cert, ssl_key);

      std::string code = "__uripwd = '" + user + ":" + password + "@" + host + ":33060';";
      exec_and_out_equals(code);

      set_config_folder("py_dev_api_examples");
      set_setup_script("setup.py");

      _extension = "py";
      _new_format = true;
    }

    void create_connection()
    {
      Server_registry* sr = new Server_registry("mysqlxconfig.json");
      sr->load();
      Connection_options cs;
      try
      {
        cs = sr->get_connection_options("myapp");
      }
      catch (std::runtime_error &e)
      {
        cs = sr->add_connection_options("myapp", "host=localhost; dbUser=mike; schema=test;");
        sr->merge();
      }
    }
  };

  //==================>>> building_expressions
  TEST_F(Shell_py_dev_api_sample_tester, Expression_Strings)
  {
    validate_interactive("building_expressions/Expression_Strings");
  }

  //==================>>> concepts
  TEST_F(Shell_py_dev_api_sample_tester, Connecting_to_a_Single_MySQL_Server)
  {
    validate_interactive("concepts/Connecting_to_a_Single_MySQL_Server");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Connecting_to_a_Single_MySQL_Server_1)
  {
    output_handler.prompts.push_back("mike");
    output_handler.ret_pwd = "s3cr3t!";

    validate_interactive("concepts/Connecting_to_a_Single_MySQL_Server_1");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Database_Connection_Example)
  {
    validate_interactive("concepts/Database_Connection_Example");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Dynamic_SQL)
  {
    validate_interactive("concepts/Dynamic_SQL");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Setting_the_Current_Schema)
  {
    validate_interactive("concepts/Setting_the_Current_Schema");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Using_SQL_with_NodeSession)
  {
    validate_interactive("concepts/Using_SQL_with_NodeSession");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Working_with_a_Session_Object)
  {
    validate_interactive("concepts/Working_with_a_Session_Object");
  }

  //==================>>> crud_operations
  TEST_F(Shell_py_dev_api_sample_tester, Interactive_Shell_Commands)
  {
    validate_interactive("crud_operations/Interactive_Shell_Commands");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Method_Chaining)
  {
    validate_interactive("crud_operations/Method_Chaining");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Parameter_Binding)
  {
    validate_interactive("crud_operations/Parameter_Binding");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Parameter_Binding_1)
  {
    validate_interactive("crud_operations/Parameter_Binding_1");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Preparing_CRUD_Statements)
  {
    validate_interactive("crud_operations/Preparing_CRUD_Statements");
  }

  //==================>>> results
  TEST_F(Shell_py_dev_api_sample_tester, Fetching_All_Data_Items_at_Once)
  {
    validate_interactive("results/Fetching_All_Data_Items_at_Once");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Working_with_Data_Sets)
  {
    validate_interactive("results/Working_with_Data_Sets");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Working_with_Data_Sets_1)
  {
    validate_interactive("results/Working_with_Data_Sets_1");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Working_with_Results)
  {
    validate_interactive("results/Working_with_Results");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Working_with_Results_1)
  {
    validate_interactive("results/Working_with_Results_1");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Working_with_SQL_Result_Sets)
  {
    validate_interactive("results/Working_with_SQL_Result_Sets");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Working_with_SQL_Result_Sets_1)
  {
    validate_interactive("results/Working_with_SQL_Result_Sets_1");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Working_with_SQL_Result_Sets_2)
  {
    validate_interactive("results/Working_with_SQL_Result_Sets_2");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Working_with_SQL_Result_Sets_3)
  {
    validate_interactive("results/Working_with_SQL_Result_Sets_3");
  }

  //==================>>> statement_execution
  // This specific example does not work for python 2.6 so we disable it for it (OEL6)
#if PY_MAJOR_VERSION > 2 || (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION > 6)

  TEST_F(Shell_py_dev_api_sample_tester, Error_Handling)
  {
    validate_interactive("statement_execution/Error_Handling");
  }
#endif

  /*
  TEST_F(Shell_py_dev_api_sample_tester, Processing_Warnings)
  {
  validate_interactive("statement_execution/Processing_Warnings");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Processing_Warnings_1)
  {
  validate_interactive("statement_execution/Processing_Warnings_1");
  }
  */
  TEST_F(Shell_py_dev_api_sample_tester, Transaction_Handling)
  {
    validate_interactive("statement_execution/Transaction_Handling");
  }

  //==================>>> working_with_collections
  TEST_F(Shell_py_dev_api_sample_tester, Basic_CRUD_Operations_on_Collections)
  {
    validate_interactive("working_with_collections/Basic_CRUD_Operations_on_Collections");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Collection_add)
  {
    validate_interactive("working_with_collections/Collection_add");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Collection_find)
  {
    validate_interactive("working_with_collections/Collection_find");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Creating_a_Collection)
  {
    validate_interactive("working_with_collections/Creating_a_Collection");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Document_Identity)
  {
    validate_interactive("working_with_collections/Document_Identity");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Document_Identity_1)
  {
    validate_interactive("working_with_collections/Document_Identity_1");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Document_Identity_2)
  {
    validate_interactive("working_with_collections/Document_Identity_2");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Working_with_Existing_Collections)
  {
    validate_interactive("working_with_collections/Working_with_Existing_Collections");
  }

  //==================>>> working_with_documents
  TEST_F(Shell_py_dev_api_sample_tester, Document_Object___Class_Diagram)
  {
    validate_interactive("working_with_documents/Document_Object___Class_Diagram");
  }

  //==================>>> working_with_relational_tables
  TEST_F(Shell_py_dev_api_sample_tester, Table_insert)
  {
    validate_interactive("working_with_relational_tables/Table_insert");
  }

  TEST_F(Shell_py_dev_api_sample_tester, Working_with_Relational_Tables)
  {
    validate_interactive("working_with_relational_tables/Working_with_Relational_Tables");
  }

  //==================>>> working_with_tables_documents
  TEST_F(Shell_py_dev_api_sample_tester, Collections_as_Relational_Tables)
  {
    validate_interactive("working_with_tables_documents/Collections_as_Relational_Tables");
  }
}