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

  TEST_F(Shell_py_dev_api_sample_tester, transaction_handling)
  {
    validate_interactive("transaction_handling.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, processing_warnings)
  {
    validate_interactive("processing_warnings.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, connecting_via_datasource_file)
  {
    create_connection();
    validate_interactive("connecting_via_datasource_file.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, setting_the_current_schema_2)
  {
    create_connection();
    validate_interactive("setting_the_current_schema_2.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, dynamic_sql)
  {
    create_connection();
    validate_interactive("dynamic_sql.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, connecting_to_a_session_2)
  {
    output_handler.ret_pwd = "s3cr3t!";
    validate_interactive("connecting_to_a_session_2.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, database_connection_example)
  {
    validate_interactive("database_connection_example.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, connecting_to_a_session)
  {
    validate_interactive("connecting_to_a_session.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, working_with_a_session_object)
  {
    validate_interactive("working_with_a_session_object.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, using_sql)
  {
    validate_interactive("using_sql.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, setting_the_current_schema)
  {
    validate_interactive("setting_the_current_schema.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, parameter_binding)
  {
    validate_interactive("parameter_binding.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, preparing_crud_statements)
  {
    validate_interactive("preparing_crud_statements.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, working_with_collections)
  {
    validate_interactive("working_with_collections.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, collection_add)
  {
    validate_interactive("collection_add.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, document_identity_1)
  {
    validate_interactive("document_identity_1.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, document_identity_2_3)
  {
    validate_interactive("document_identity_2_3.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, collection_find)
  {
    validate_interactive("collection_find.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, table_insert)
  {
    validate_interactive("table_insert.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, working_with_datasets)
  {
    validate_interactive("working_with_datasets.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, working_with_datasets_2)
  {
    validate_interactive("working_with_datasets_2.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, working_with_results)
  {
    validate_interactive("working_with_results.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, working_with_results_2)
  {
    validate_interactive("working_with_results_2.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, error_handling)
  {
    validate_interactive("error_handling.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, working_with_sql_results)
  {
    validate_interactive("working_with_sql_results.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, working_with_sql_results_2)
  {
    validate_interactive("working_with_sql_results_2.py");
  }

  TEST_F(Shell_py_dev_api_sample_tester, method_chaining)
  {
    validate_interactive("method_chaining.py");
  }
}